#pragma once
#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include "networkprobe.h"
#include <atomic>
#include <memory>
#include <QSet>
#include <QUrl>
#include <QElapsedTimer>
#include <QTimer>
#include <QThreadPool>

struct Host { QString ip, status = "Queued", hostname, latency, ports, detail, mac; quint32 address; };
class Scanner : public QAbstractListModel {
 Q_OBJECT
 Q_PROPERTY(bool running READ running NOTIFY changed)
 Q_PROPERTY(int completed READ completed NOTIFY changed)
 Q_PROPERTY(int total READ total NOTIFY changed)
 Q_PROPERTY(int alive READ alive NOTIFY changed)
 Q_PROPERTY(int silent READ silent NOTIFY changed)
 Q_PROPERTY(qint64 elapsedMs READ elapsedMs NOTIFY changed)
 Q_PROPERTY(QString message READ message NOTIFY changed)
 Q_PROPERTY(QString localRange READ localRange CONSTANT)
public:
 static constexpr int MaxTargets = 65536;
 enum Roles { Ip = Qt::UserRole+1, Status, Hostname, Latency, Ports, Detail, Address, Mac };
 explicit Scanner(QObject *parent=nullptr, PingFunction ping=pingHost);
 ~Scanner() override;
 int rowCount(const QModelIndex &parent={}) const override { return parent.isValid()?0:m_hosts.size(); }
 QVariant data(const QModelIndex &, int) const override;
 QHash<int,QByteArray> roleNames() const override;
 bool running() const { return m_running; }
 int completed() const { return m_completed; }
 int total() const { return m_hosts.size(); }
 int alive() const { return m_alive; }
 int silent() const { return m_silent; }
 qint64 elapsedMs() const { return m_running ? m_clock.elapsed() : m_elapsedMs; }
 static QString macFromArp(const QString &table, const QString &ip);
 QString message() const { return m_message; }
 QString localRange() const;
 static QList<quint32> parseRange(const QString &, QString &error);
 Q_INVOKABLE void start(const QString &range, bool dns=true, bool ports=true, int timeout=800, int concurrency=32);
 Q_INVOKABLE void stop();
 Q_INVOKABLE bool exportCsv(const QUrl &);
 Q_INVOKABLE void copy(const QString &);
signals: void changed();
private:
 QThreadPool m_pool;
 PingFunction m_ping;
 std::shared_ptr<std::atomic_bool> m_cancelled;
 QElapsedTimer m_clock;
 QTimer m_tick;
 qint64 m_elapsedMs=0;
 void pump();
 void probe(int row);
 void finish(int row);
 void update(int row);
 QList<Host> m_hosts;
 QSet<QObject*> m_jobs;
 int m_next=0, m_completed=0, m_alive=0, m_silent=0, m_active=0, m_limit=32, m_timeout=800;
 qint64 m_lastDiagnosticHeartbeat=0;
 bool m_running=false, m_dns=true, m_ports=true;
 QString m_message;
};
class HostFilter : public QSortFilterProxyModel {
 Q_OBJECT
public:
 explicit HostFilter(QObject *parent=nullptr) : QSortFilterProxyModel(parent) { setDynamicSortFilter(true); setSortRole(Scanner::Address); sort(0); }
 Q_INVOKABLE void filter(const QString &text, const QString &status, bool hideDead=false) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 10, 0)
 beginFilterChange(); m_text=text; m_status=status; m_hideDead=hideDead; endFilterChange(Direction::Rows);
#else
 m_text=text; m_status=status; m_hideDead=hideDead; invalidateFilter();
#endif
  }
 Q_INVOKABLE void order(int column, bool descending) { const int roles[]={Scanner::Address,Scanner::Status,Scanner::Hostname,Scanner::Latency,Scanner::Ports,Scanner::Mac}; setSortRole(roles[qBound(0,column,5)]); sort(0,descending?Qt::DescendingOrder:Qt::AscendingOrder); }
protected:
 bool filterAcceptsRow(int row,const QModelIndex &parent) const override;
private: QString m_text, m_status; bool m_hideDead=false;
};
