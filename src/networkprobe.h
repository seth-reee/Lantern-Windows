#pragma once
#include <QString>
#include <functional>

struct PingResult {
    bool live = false;
    QString latency;
    QString error;
};
using PingFunction = std::function<PingResult(const QString &, int)>;
PingResult pingHost(const QString &ip, int timeout);
QString neighborMac(const QString &ip);
