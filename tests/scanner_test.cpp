#include <QtTest>
#include <QTemporaryDir>
#include <QFile>
#include <QTcpServer>
#include <QThread>
#include "scanner.h"

class ScannerTest:public QObject {
 Q_OBJECT
 static PingResult live(const QString &, int) { return {true,"0.123 ms",{}}; }
 static PingResult silent(const QString &, int) { return {}; }
 static PingResult slow(const QString &, int) { QThread::msleep(300); return {true,"1 ms",{}}; }
private slots:
 void arpEntries() {
  const QString table="IP address HW type Flags HW address Mask Device\n"
   "192.168.1.2 0x1 0x2 aa:bb:cc:dd:ee:ff * eth0\n"
   "192.168.1.3 0x1 0x0 00:00:00:00:00:00 * eth0\n"
   "192.168.1.4 0x1 0x2 invalid * eth0\n";
  QCOMPARE(Scanner::macFromArp(table,"192.168.1.2"),QString("AA:BB:CC:DD:EE:FF"));
  QVERIFY(Scanner::macFromArp(table,"192.168.1.3").isEmpty());
  QVERIFY(Scanner::macFromArp(table,"192.168.1.4").isEmpty());
 }
 void ranges() {
  QString error;
  auto addresses=Scanner::parseRange("192.168.7.123/24",error);
  QCOMPARE(addresses.size(),254); QCOMPARE(addresses.first(),quint32(0xc0a80701));
  QCOMPARE(Scanner::parseRange("10.0.0.0/31",error).size(),2);
  QCOMPARE(Scanner::parseRange("255.255.255.255/32",error).size(),1);
  QCOMPARE(Scanner::parseRange("127.0.0.1-127.0.0.3",error).size(),3);
  for(auto bad:{"10.0.0.0/8","1.2.3.4/33","1.2.3.5-1.2.3.4","localhost","::1","1.2.3.4;echo bad"}) {
   QVERIFY(Scanner::parseRange(bad,error).isEmpty()); QVERIFY(!error.isEmpty());
  }
  const auto subnet=Scanner::parseRange("192.168.42.99/16",error);
  QCOMPARE(subnet.size(),65534); QCOMPARE(subnet.first(),quint32(0xc0a80001));
  QCOMPARE(subnet.last(),quint32(0xc0a8fffe));
  const auto full=Scanner::parseRange("255.255.0.0-255.255.255.255",error);
  QCOMPARE(full.size(),Scanner::MaxTargets); QCOMPARE(full.last(),quint32(0xffffffff));
  QVERIFY(Scanner::parseRange("0.0.0.0/0",error).isEmpty());
 }
 void elapsedTime() {
  Scanner scanner(nullptr,slow);
  scanner.start("127.0.0.1",false,false,5000,1);
  QTest::qWait(120); QVERIFY(scanner.elapsedMs()>=100);
  scanner.stop(); const auto stopped=scanner.elapsedMs();
  QTest::qWait(120); QCOMPARE(scanner.elapsedMs(),stopped);
 }
 void cancellationAndRestart() {
  // A delayed result from the first scan must not overwrite the new scan.
  auto probe=[](const QString &ip,int) {
   if(ip=="127.0.0.1") { QThread::msleep(300); return PingResult{true,"old",{}}; }
   return PingResult{};
  };
  Scanner scanner(nullptr,probe); HostFilter filter; filter.setSourceModel(&scanner);
  scanner.start("127.0.0.0/16",false,false,5000,2);
  QVERIFY(scanner.running()); QCOMPARE(scanner.total(),65534);
  QSignalSpy updates(&scanner,&QAbstractItemModel::dataChanged);
  scanner.stop(); QVERIFY(!scanner.running()); QCOMPARE(scanner.completed(),0);
  QCOMPARE(updates.size(),1);
  for(int i=0;i<scanner.total();++i)
   QCOMPARE(scanner.data(scanner.index(i),Scanner::Status).toString(),QString("Cancelled"));
  scanner.start("127.0.0.2",false,false,200,1);
  QTRY_VERIFY_WITH_TIMEOUT(!scanner.running(),3000);
  QTest::qWait(400);
  QCOMPARE(scanner.completed(),1); QCOMPARE(scanner.alive(),0); QCOMPARE(scanner.silent(),1);
  QCOMPARE(scanner.data(scanner.index(0),Scanner::Latency).toString(),QString());
 }
 void scanAndExport() {
  Scanner scanner(nullptr,live); scanner.start("127.0.0.1",false,false,300,1);
  QTRY_VERIFY_WITH_TIMEOUT(!scanner.running(),3000);
  QCOMPARE(scanner.completed(),1); QCOMPARE(scanner.alive(),1);
  QTemporaryDir dir;
  const auto filename=dir.filePath(QString::fromUtf8("hosts café.csv"));
  QVERIFY(scanner.exportCsv(QUrl::fromLocalFile(filename)));
  QFile csv(filename); QVERIFY(csv.open(QIODevice::ReadOnly));
  QVERIFY(csv.readAll().contains("\"127.0.0.1\",\"Live\""));
  QVERIFY(!scanner.exportCsv(QUrl("https://example.com/hosts.csv")));
  HostFilter filter; filter.setSourceModel(&scanner);
  filter.filter("127.0","Live"); QCOMPARE(filter.rowCount(),1);
  filter.filter("","No reply"); QCOMPARE(filter.rowCount(),0);
 }
 void tcpWithoutPing() {
  QTcpServer server;
  if(!server.listen(QHostAddress::LocalHost,8080)) QSKIP(qPrintable(server.errorString()));
  Scanner scanner(nullptr,silent); scanner.start("127.0.0.1",false,true,200,1);
  QTRY_VERIFY_WITH_TIMEOUT(!scanner.running(),3000);
  QCOMPARE(scanner.alive(),1);
  QVERIFY(scanner.data(scanner.index(0),Scanner::Ports).toString().contains("8080"));
 }
 void errorsAndSilent() {
  Scanner quiet(nullptr,silent); quiet.start("127.0.0.1",false,false,200,1);
  QTRY_VERIFY_WITH_TIMEOUT(!quiet.running(),3000); QCOMPARE(quiet.silent(),1);
  Scanner broken(nullptr,[](const QString &,int) { return PingResult{false,{},"ICMP unavailable"}; });
  broken.start("127.0.0.1",false,false,200,1);
  QTRY_VERIFY_WITH_TIMEOUT(!broken.running(),3000);
  QCOMPARE(broken.data(broken.index(0),Scanner::Status).toString(),QString("Error"));
  QCOMPARE(broken.silent(),0);
 }
 void nativeLoopback() {
#ifdef Q_OS_WIN
  // Real IP Helper API exercise, independent of injected fixture results.
  const auto reply=pingHost("127.0.0.1",1000);
  QVERIFY2(reply.live,qPrintable(reply.error)); QVERIFY(!reply.latency.isEmpty());
#else
  QSKIP("Native Windows ICMP is exercised by Windows CI");
#endif
 }
};
QTEST_GUILESS_MAIN(ScannerTest)
#include "scanner_test.moc"
