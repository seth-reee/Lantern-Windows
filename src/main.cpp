#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTimer>
#include <QQuickWindow>
#include <QDir>
#include "scanner.h"
#include "theme.h"
int main(int argc,char **argv) {
 QGuiApplication app(argc,argv);
 app.setApplicationName("Lantern"); app.setOrganizationName("Lantern"); app.setApplicationVersion(LANTERN_VERSION);
 app.setDesktopFileName("lantern");
 QQuickStyle::setStyle("Basic");
 Theme theme;
 auto updateIcon=[&app,&theme]() { app.setWindowIcon(QIcon(theme.iconSource().replace("qrc:", ":"))); };
 QObject::connect(&theme,&Theme::changed,&app,updateIcon);
 updateIcon();
 Scanner scanner; HostFilter hosts; hosts.setSourceModel(&scanner);
 QQmlApplicationEngine engine;
 engine.rootContext()->setContextProperty("theme",&theme);
 engine.rootContext()->setContextProperty("scanner",&scanner);
 engine.rootContext()->setContextProperty("hosts",&hosts);
 QObject::connect(&engine,&QQmlApplicationEngine::objectCreationFailed,&app,[] { QCoreApplication::exit(1); },Qt::QueuedConnection);
 engine.loadFromModule("Lantern","Main");
 if(app.arguments().contains("--smoke-test")) QTimer::singleShot(700,&app,[&]() {
  auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().value(0));
  if(!window) { app.exit(1); return; }
  const auto frame=window->grabWindow();
  app.exit(!frame.isNull() && frame.save(QDir::temp().filePath("lantern-preview.png")) ? 0 : 1);
 });
 return app.exec();
}
