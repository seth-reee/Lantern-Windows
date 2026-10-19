#include "networkprobe.h"
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <netioapi.h>
#endif
#include <QHostAddress>
#include <QNetworkInterface>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QRegularExpression>
#include <vector>
#include "scanner.h"

PingResult pingHost(const QString &ip, int timeout) {
#ifdef Q_OS_WIN
    const HANDLE handle = IcmpCreateFile();
    if (handle == INVALID_HANDLE_VALUE)
        return {false, {}, QString("Cannot initialize ICMP (Windows error %1)").arg(GetLastError())};
    char payload[] = "Lantern";
    std::vector<unsigned char> buffer(sizeof(ICMP_ECHO_REPLY) + sizeof(payload) + 8);
    // Qt's IPv4 integer is host order; IP Helper expects network byte order.
    const DWORD count = IcmpSendEcho(handle, htonl(QHostAddress(ip).toIPv4Address()),
        payload, sizeof(payload), nullptr, buffer.data(), DWORD(buffer.size()), DWORD(timeout));
    const DWORD error = count ? reinterpret_cast<ICMP_ECHO_REPLY *>(buffer.data())->Status : GetLastError();
    PingResult result;
    if (count && error == IP_SUCCESS) {
        const auto reply = reinterpret_cast<ICMP_ECHO_REPLY *>(buffer.data());
        result.live = true;
        result.latency = reply->RoundTripTime ? QString::number(reply->RoundTripTime) + " ms" : "<1 ms";
    } else if (error != IP_REQ_TIMED_OUT && error != IP_DEST_NET_UNREACHABLE &&
               error != IP_DEST_HOST_UNREACHABLE && error != IP_DEST_PROT_UNREACHABLE &&
               error != IP_DEST_PORT_UNREACHABLE && error != IP_TTL_EXPIRED_TRANSIT &&
               error != IP_TTL_EXPIRED_REASSEM) {
        result.error = QString("ICMP failed (Windows error %1)").arg(error);
    }
    IcmpCloseHandle(handle);
    return result;
#else
    const QString executable = QStandardPaths::findExecutable("ping");
    if (executable.isEmpty()) return {false, {}, "The ping executable is required (install iputils)."};
    QProcess process;
    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert("LC_ALL", "C");
    process.setProcessEnvironment(environment);
    process.start(executable, {"-n", "-c", "1", "-W", QString::number(timeout / 1000.0, 'f', 3), ip});
    if (!process.waitForStarted(1000)) return {false, {}, process.errorString()};
    if (!process.waitForFinished(timeout + 500)) {
        process.kill(); process.waitForFinished(1000);
        return {false, {}, "Ping exceeded its deadline"};
    }
    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) {
        const auto match = QRegularExpression("time([=<])([0-9.]+)").match(QString::fromUtf8(process.readAllStandardOutput()));
        return {true, match.hasMatch() ? (match.captured(1) == "<" ? "<" : "") + match.captured(2) + " ms" : QString(), {}};
    }
    if (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 1) return {};
    const QString error = QString::fromUtf8(process.readAllStandardError()).trimmed();
    return {false, {}, error.isEmpty() ? "Ping failed" : error};
#endif
}

QString neighborMac(const QString &ip) {
    for (const auto &iface : QNetworkInterface::allInterfaces()) {
        if (iface.flags() & QNetworkInterface::IsLoopBack) continue;
        for (const auto &entry : iface.addressEntries())
            if (entry.ip().toString() == ip) return iface.hardwareAddress().toUpper();
    }
#ifdef Q_OS_WIN
    MIB_IPNET_TABLE2 *table = nullptr;
    if (GetIpNetTable2(AF_INET, &table) != NO_ERROR) return {};
    QString result;
    const auto address = htonl(QHostAddress(ip).toIPv4Address());
    for (ULONG i = 0; i < table->NumEntries; ++i) {
        const auto &entry = table->Table[i];
        if (entry.Address.Ipv4.sin_addr.S_un.S_addr != address || entry.PhysicalAddressLength != 6 ||
            entry.State == NlnsIncomplete || entry.State == NlnsUnreachable) continue;
        QStringList octets;
        bool nonzero = false;
        for (ULONG j = 0; j < entry.PhysicalAddressLength; ++j) {
            nonzero |= entry.PhysicalAddress[j] != 0;
            octets.append(QString::number(entry.PhysicalAddress[j], 16).rightJustified(2, '0'));
        }
        if (nonzero) { result = octets.join(':').toUpper(); break; }
    }
    FreeMibTable(table);
    return result;
#else
    QFile arp("/proc/net/arp");
    return arp.open(QIODevice::ReadOnly) ? Scanner::macFromArp(QString::fromUtf8(arp.readAll()), ip) : QString();
#endif
}
