#include "scanner.h"
#include <QHostAddress>
#include <QNetworkInterface>
#include <QHostInfo>
#include <QTcpSocket>
#include <QTimer>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSaveFile>
#include <QTextStream>
#include <memory>
#include <QGuiApplication>
#include <QClipboard>
#include <QFile>
#include <QFutureWatcher>
#include <QtConcurrentRun>

Scanner::Scanner(QObject *p, PingFunction ping):QAbstractListModel(p),m_ping(std::move(ping)) { m_tick.setInterval(100); connect(&m_tick,&QTimer::timeout,this,&Scanner::changed); }
Scanner::~Scanner() { stop(); }
QHash<int,QByteArray> Scanner::roleNames() const { return {{Ip,"ip"},{Status,"status"},{Hostname,"hostname"},{Latency,"latency"},{Ports,"ports"},{Detail,"detail"},{Address,"address"},{Mac,"mac"}}; }
QVariant Scanner::data(const QModelIndex &i,int role) const {
 if(!i.isValid()||i.row()<0||i.row()>=m_hosts.size()) return {};
 const auto &h=m_hosts[i.row()];
 switch(role) { case Mac:return h.mac; case Ip:return h.ip; case Status:return h.status; case Hostname:return h.hostname; case Latency:return h.latency; case Ports:return h.ports; case Detail:return h.detail; case Address:return h.address; } return {};
}
QString Scanner::macFromArp(const QString &table,const QString &ip) {
 for(const auto &line:table.split('\n')) {
  const auto fields=line.simplified().split(' ');
  if(fields.size()<6 || fields[0]!=ip) continue;
  bool ok=false; const int flags=fields[2].toInt(&ok,16);
  const QString mac=fields[3].toUpper();
  if(ok && (flags & 2) && mac!="00:00:00:00:00:00" && QRegularExpression("^([0-9A-F]{2}:){5}[0-9A-F]{2}$").match(mac).hasMatch()) return mac;
 }
 return {};
}
QString Scanner::localRange() const {
 for(const auto &iface:QNetworkInterface::allInterfaces()) {
  if(!(iface.flags()&QNetworkInterface::IsUp)||(iface.flags()&QNetworkInterface::IsLoopBack)) continue;
  for(const auto &entry:iface.addressEntries()) if(entry.ip().protocol()==QAbstractSocket::IPv4Protocol) {
   // Default to a manageable /24 even on larger routed networks.
   return QHostAddress(entry.ip().toIPv4Address() & 0xffffff00u).toString()+"/24";
  }
 } return "192.168.1.0/24";
}
QList<quint32> Scanner::parseRange(const QString &input,QString &error) {
 error.clear(); QString s=input.trimmed(); quint32 first=0,last=0;
 auto parse=[](QString text,quint32 &n) { QHostAddress a; if(!a.setAddress(text.trimmed())||a.protocol()!=QAbstractSocket::IPv4Protocol) return false; n=a.toIPv4Address(); return true; };
 if(s.contains('/')) {
  const auto parts=s.split('/'); bool ok=false; int prefix=parts.size()==2?parts[1].toInt(&ok):-1;
  if(parts.size()!=2||!ok||prefix<0||prefix>32||!parse(parts[0],first)) error="Enter a valid IPv4 CIDR, for example 192.168.1.0/24.";
  else { quint32 mask=prefix==0?0:(0xffffffffu<<(32-prefix)); first &= mask; last=first|~mask; if(prefix<31) { ++first; --last; } }
 } else if(s.contains('-')) { auto p=s.split('-'); if(p.size()!=2||!parse(p[0],first)||!parse(p[1],last)||first>last) error="Enter an ascending range: 192.168.1.1-192.168.1.254."; }
 else if(!parse(s,first)) error="Enter an IPv4 address, range, or CIDR."; else last=first;
 if(!error.isEmpty()) return {};
 if(quint64(last)-first+1>MaxTargets) { error="Choose a range of at most 65,536 addresses (a /16 subnet or smaller)."; return {}; }
 QList<quint32> result; result.reserve(qsizetype(quint64(last)-first+1)); for(quint64 n=first;n<=last;++n) result.append(quint32(n)); return result;
}
void Scanner::update(int r) { emit dataChanged(index(r),index(r)); }
void Scanner::start(const QString &range,bool dns,bool ports,int timeout,int concurrency) {
 if(m_running) return;
 QString error; auto addresses=parseRange(range,error);
 if(!error.isEmpty()) { m_message=error; emit changed(); return; }
 m_cancelled=std::make_shared<std::atomic_bool>(false);
 beginResetModel(); m_hosts.clear(); m_hosts.reserve(addresses.size()); for(auto a:addresses) { Host h; h.address=a; h.ip=QHostAddress(a).toString(); m_hosts.append(h); } endResetModel();
 m_dns=dns; m_ports=ports; m_timeout=qBound(200,timeout,5000); m_limit=qBound(1,concurrency,64);
 m_next=m_completed=m_alive=m_silent=m_active=0; m_clock.start(); m_elapsedMs=0; m_running=true; m_tick.start(); m_message="Scanning…"; emit changed(); pump();
}
void Scanner::pump() {
 while(m_running && m_active<m_limit && m_next<m_hosts.size()) { ++m_active; probe(m_next++); }
 if(m_running && m_completed==m_hosts.size()) { m_elapsedMs=m_clock.elapsed(); m_running=false; m_tick.stop(); m_message="Scan complete"; emit changed(); }
}
void Scanner::probe(int row) {
 auto *job=new QObject(this); m_jobs.insert(job);
 auto &host=m_hosts[row]; host.status="Scanning"; update(row);
 struct State { int pending=1; bool live=false; QStringList open; };
 auto state=std::make_shared<State>();
 auto complete=[this,row,job,state]() {
  if(--state->pending) return;
  auto &h=m_hosts[row]; h.status=state->live?"Live":"No reply"; std::sort(state->open.begin(),state->open.end(),[](const QString &a,const QString &b) { return a.toInt()<b.toInt(); }); h.ports=state->open.join(", ");
  if(h.ports.isEmpty()) h.ports=m_ports?"None detected":"Not checked";
  auto done=[this,row,job]() { m_jobs.remove(job); job->deleteLater(); finish(row); };
  if(m_dns && state->live) {
   auto *timer=new QTimer(job); timer->setSingleShot(true);
   const int lookup=QHostInfo::lookupHost(h.ip,job,[this,row,timer,done](const QHostInfo &info) {
    if(!timer->isActive()) return; timer->stop();
    if(info.error()==QHostInfo::NoError && info.hostName()!=m_hosts[row].ip) m_hosts[row].hostname=info.hostName();
    done();
   });
   connect(timer,&QTimer::timeout,job,[lookup,done]() { QHostInfo::abortHostLookup(lookup); done(); }); timer->start(2000);
  } else done();
 };
 // Workers own their arguments and resources. Destroying the job disconnects the
 // watcher; cancelled or superseded scans can never update model rows.
 auto *watcher=new QFutureWatcher<PingResult>(job);
 connect(watcher,&QFutureWatcher<PingResult>::finished,job,[this,row,watcher,state,complete]() {
  const auto result=watcher->result();
  state->live=state->live || result.live;
  m_hosts[row].latency=result.latency;
  m_hosts[row].detail=result.error;
  complete();
 });
 const auto cancelled=m_cancelled;
 watcher->setFuture(QtConcurrent::run([ping=m_ping,ip=host.ip,timeout=m_timeout,cancelled]() {
  return cancelled->load() ? PingResult{} : ping(ip,timeout);
 }));
 if(m_ports) {
  const QList<int> ports={21,22,23,25,53,80,110,139,143,443,445,587,631,993,995,1433,3306,3389,5432,5900,8000,8080,8443};
  state->pending+=ports.size();
  for(int port:ports) {
   auto *socket=new QTcpSocket(job); auto *timer=new QTimer(socket); timer->setSingleShot(true);
   auto ended=std::make_shared<bool>(false);
   auto finishSocket=[socket,timer,ended,complete]() { if(*ended) return; *ended=true; timer->stop(); socket->abort(); socket->deleteLater(); complete(); };
   connect(socket,&QTcpSocket::connected,job,[state,port,finishSocket]() { state->live=true; state->open.append(QString::number(port)); finishSocket(); });
   connect(socket,&QTcpSocket::errorOccurred,job,[finishSocket](QAbstractSocket::SocketError) { finishSocket(); });
   connect(timer,&QTimer::timeout,job,finishSocket); timer->start(m_timeout); socket->connectToHost(host.ip,port);
  }
 }
}
void Scanner::finish(int row) {
 auto &host=m_hosts[row];
 host.mac=neighborMac(host.ip);
 --m_active; ++m_completed;
 if(m_hosts[row].status=="Live") ++m_alive; else if(!m_hosts[row].detail.isEmpty()) m_hosts[row].status="Error"; else ++m_silent;
 update(row); emit changed(); pump();
}
void Scanner::stop() {
 if(!m_running) return;
 m_elapsedMs=m_clock.elapsed(); m_tick.stop(); m_running=false;
 m_cancelled->store(true);
 for(auto *job:std::as_const(m_jobs)) {
  // Disconnect callbacks before killing probes; preserve completed results.
  for(auto *child:job->findChildren<QObject*>()) child->disconnect();
  job->disconnect();
  delete job;
 }
 m_jobs.clear(); m_active=0;
 int firstCancelled=-1, lastCancelled=-1;
 for(int i=0;i<m_hosts.size();++i) if(m_hosts[i].status=="Queued"||m_hosts[i].status=="Scanning") {
  m_hosts[i].status="Cancelled";
  if(firstCancelled<0) firstCancelled=i;
  lastCancelled=i;
 }
 // One notification keeps cancellation responsive with tens of thousands of rows.
 if(firstCancelled>=0) emit dataChanged(index(firstCancelled),index(lastCancelled),{Status});
 m_message="Scan stopped · partial results retained"; emit changed();
}
bool Scanner::exportCsv(const QUrl &url) {
 QSaveFile file(url.toLocalFile());
 if(!url.isLocalFile()||!file.open(QIODevice::WriteOnly|QIODevice::Text)) { m_message="Cannot open export file: "+file.errorString(); emit changed(); return false; }
 QTextStream out(&file); out<<"IP,Status,Hostname,Latency,Open ports,MAC address,Detail\n";
 auto quote=[](QString s) { s.replace('"',"\"\""); return '"'+s+'"'; };
 for(const auto &h:m_hosts) out<<quote(h.ip)<<','<<quote(h.status)<<','<<quote(h.hostname)<<','<<quote(h.latency)<<','<<quote(h.ports)<<','<<quote(h.mac)<<','<<quote(h.detail)<<'\n';
 out.flush(); bool ok=file.commit(); m_message=ok?"CSV exported":"Export failed: "+file.errorString(); emit changed(); return ok;
}
bool HostFilter::filterAcceptsRow(int row,const QModelIndex &parent) const {
 auto i=sourceModel()->index(row,0,parent);
 if(m_hideDead && sourceModel()->data(i,Scanner::Status).toString()=="No reply") return false;
 if(!m_status.isEmpty() && sourceModel()->data(i,Scanner::Status).toString()!=m_status) return false;
 for(int role:{Scanner::Ip,Scanner::Hostname,Scanner::Ports,Scanner::Status,Scanner::Mac}) if(sourceModel()->data(i,role).toString().contains(m_text,Qt::CaseInsensitive)) return true;
 return false;
}

void Scanner::copy(const QString &text) { QGuiApplication::clipboard()->setText(text); }
