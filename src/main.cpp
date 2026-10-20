#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTimer>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QDir>
#include <QSysInfo>
#include <QThreadPool>
#include "scanner.h"
#include "theme.h"
#include "diagnosticlog.h"
int main(int argc,char **argv) {
 QGuiApplication app(argc,argv);
 app.setApplicationName("Lantern"); app.setOrganizationName("Lantern"); app.setApplicationVersion(LANTERN_VERSION);
 app.setDesktopFileName("lantern");
 if(app.arguments().contains("--verbose-log")) {
  const auto path=startDiagnosticLog();
  if(path.isEmpty()) qWarning("Could not create the diagnostic log");
  else {
   diagnosticLog(QStringLiteral("app_start version=%1 os=%2 kernel=%3 arch=%4 ideal_threads=%5 pool_max_threads=%6")
    .arg(app.applicationVersion(),QSysInfo::prettyProductName(),QSysInfo::kernelVersion(),QSysInfo::currentCpuArchitecture())
    .arg(QThread::idealThreadCount()).arg(QThreadPool::globalInstance()->maxThreadCount()));
   diagnosticLog(QStringLiteral("graphics_env QT_QUICK_BACKEND=%1 QSG_RHI_BACKEND=%2 QSG_RHI_PREFER_SOFTWARE_RENDERER=%3")
    .arg(qEnvironmentVariable("QT_QUICK_BACKEND","(unset)"),qEnvironmentVariable("QSG_RHI_BACKEND","(unset)"),qEnvironmentVariable("QSG_RHI_PREFER_SOFTWARE_RENDERER","(unset)")));
   diagnosticLog(QStringLiteral("log_path=%1").arg(path));
   qInfo().noquote() << "Lantern diagnostic log:" << path;
  }
  QObject::connect(&app,&QCoreApplication::aboutToQuit,[] { flushDiagnosticLog(); });
 }
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
 if(app.arguments().contains("--verbose-log")) {
  diagnosticLog(QStringLiteral("qml_loaded root_objects=%1").arg(engine.rootObjects().size()));
  QTimer::singleShot(1500,&app,[&engine]() {
   auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().value(0));
   if(!window) { diagnosticLog("graphics_api=unavailable_no_window"); return; }
   const auto api=window->rendererInterface()->graphicsApi();
   QString name;
   switch(api) {
    case QSGRendererInterface::Software: name="Software"; break;
    case QSGRendererInterface::OpenGL: name="OpenGL"; break;
    case QSGRendererInterface::Direct3D11: name="Direct3D11"; break;
    case QSGRendererInterface::Vulkan: name="Vulkan"; break;
    case QSGRendererInterface::Metal: name="Metal"; break;
    case QSGRendererInterface::Null: name="Null"; break;
    default: name="Unknown"; break;
   }
   diagnosticLog(QStringLiteral("graphics_api=%1").arg(name));
  });
 }
 if(app.arguments().contains("--smoke-test")) QTimer::singleShot(700,&app,[&]() {
  auto *window=qobject_cast<QQuickWindow*>(engine.rootObjects().value(0));
  if(!window) { app.exit(1); return; }
  const auto frame=window->grabWindow();
  app.exit(!frame.isNull() && frame.save(QDir::temp().filePath("lantern-preview.png")) ? 0 : 1);
 });
 return app.exec();
}
