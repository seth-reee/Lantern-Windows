#include <QtTest>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQuickStyle>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include "scanner.h"
#include "theme.h"
class UiTest:public QObject {
 Q_OBJECT
private slots:
 void systemPaletteChanges() {
  Theme theme;
  const auto original=QGuiApplication::palette();
  QPalette palette=original;
  palette.setColor(QPalette::Window,QColor("#112233"));
  palette.setColor(QPalette::WindowText,QColor("#eeeeee"));
  palette.setColor(QPalette::Accent,QColor("#4488cc"));
  QGuiApplication::setPalette(palette);
  QCoreApplication::processEvents();
  const auto colors=theme.colors();
  const auto darkIcon=theme.iconSource();
  palette.setColor(QPalette::Window,QColor("#ffffff"));
  QGuiApplication::setPalette(palette);
  QCoreApplication::processEvents();
  const auto lightIcon=theme.iconSource();
  QGuiApplication::setPalette(original);
  QCoreApplication::processEvents();
  QCOMPARE(darkIcon,QString("qrc:/resources/icons/lantern-black.png"));
  QCOMPARE(lightIcon,QString("qrc:/resources/icons/lantern-white.png"));
  QCOMPARE(colors["background"].value<QColor>(),QColor("#112233"));
  QCOMPARE(colors["foreground"].value<QColor>(),QColor("#eeeeee"));
  QCOMPARE(colors["accent"].value<QColor>(),QColor("#4488cc"));
  QCOMPARE(theme.colors()["background"].value<QColor>(),original.window().color());
 }
 void populatedWindow() {
  QQuickStyle::setStyle("Basic");
  Scanner scanner(nullptr, [](const QString &, int) { return PingResult{true,"0.123 ms",{}}; }); scanner.start("127.0.0.1-127.0.0.3",false,false,200,1);
  QTRY_VERIFY_WITH_TIMEOUT(!scanner.running(),3000); QCOMPARE(scanner.alive(),3);
  Theme theme; HostFilter hosts; hosts.setSourceModel(&scanner);
  QQmlApplicationEngine engine;
  QStringList warnings;
  connect(&engine,&QQmlEngine::warnings,this,[&](const QList<QQmlError> &errors) { for(const auto &e:errors) warnings.append(e.toString()); });
  engine.rootContext()->setContextProperty("theme",&theme); engine.rootContext()->setContextProperty("scanner",&scanner); engine.rootContext()->setContextProperty("hosts",&hosts);
  engine.load(QUrl::fromLocalFile(LANTERN_QML));
  QVERIFY2(!engine.rootObjects().isEmpty(),qPrintable(warnings.join('\n')));
  auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().first()); QVERIFY(window);
  auto *results=window->findChild<QObject*>("results"); QVERIFY(results);
  QTRY_COMPARE(results->property("count").toInt(),3);
  QTest::qWait(150);
  QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
  auto *timer=window->findChild<QObject*>("scanTimer"); QVERIFY(timer);
  QVERIFY(timer->property("text").toString().startsWith("Scanned in: "));
  auto *about=window->findChild<QObject*>("aboutDialog"); QVERIFY(about);
  QVERIFY(QMetaObject::invokeMethod(about,"open"));
  QTest::qWait(100);
  QVERIFY(about->property("visible").toBool());
  QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
  QVERIFY(QMetaObject::invokeMethod(about,"close"));
  QTest::qWait(250);
  auto *listItem=qobject_cast<QQuickItem*>(results); QVERIFY(listItem);
  QTest::mouseClick(window,Qt::RightButton,Qt::NoModifier,listItem->mapToScene(QPointF(30,20)).toPoint());
  QQuickItem *firstRow=nullptr;
  QVERIFY(QMetaObject::invokeMethod(results,"itemAtIndex",Q_RETURN_ARG(QQuickItem*,firstRow),Q_ARG(int,0)));
  QVERIFY(firstRow);
  auto *menu=firstRow->findChild<QObject*>("hostContextMenu"); QVERIFY(menu);
  QTRY_VERIFY(menu->property("visible").toBool());
  QVERIFY(menu->property("width").toReal()>=220);
  auto *copy=menu->findChild<QQuickItem*>("copyIpAction"); QVERIFY(copy);
  QVERIFY(copy->height()>=36);
  QTest::qWait(200);
  menu->setProperty("currentIndex",-1);
  QTest::keyClick(copy->window(),Qt::Key_Down);
  QTRY_VERIFY(copy->property("highlighted").toBool());
  auto *label=copy->property("contentItem").value<QObject*>(); QVERIFY(label);
  QCOMPARE(label->property("color").value<QColor>(),QColor(theme.colors()["foreground"].toString()));
  auto *background=copy->property("background").value<QObject*>(); QVERIFY(background);
  QCOMPARE(background->property("color").value<QColor>(),QColor(theme.colors()["selection"].toString()));
  copy->window()->grabWindow().save(QDir::temp().filePath("lantern-menu-preview.png"));
  QVERIFY(QMetaObject::invokeMethod(menu,"close"));
  hosts.filter("nothing-matches",""); QTRY_COMPARE(results->property("count").toInt(),0);
  hosts.filter("127.0.0.2","Live"); QTRY_COMPARE(results->property("count").toInt(),1);
  scanner.start("127.42.0.0/16",false,false,200,1);
  QVERIFY(scanner.running()); QCOMPARE(scanner.total(),65534);
  scanner.stop(); hosts.filter("","");
  QTRY_COMPARE(results->property("count").toInt(),65534);
  hosts.filter("127.42.255.254","Cancelled");
  QTRY_COMPARE(results->property("count").toInt(),1);
  QVERIFY2(warnings.isEmpty(),qPrintable(warnings.join('\n')));
 }
};
QTEST_MAIN(UiTest)
#include "ui_test.moc"
