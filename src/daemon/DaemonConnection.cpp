#include "DaemonConnection.h"
#include "seq/v1/client.pb.h"
#include "seq/v1/events.pb.h"
#include <QDebug>

DaemonConnection::DaemonConnection(QObject* parent)
    : QObject(parent)
{
    connect(&m_socket, &QWebSocket::connected,    this, &DaemonConnection::onConnected);
    connect(&m_socket, &QWebSocket::disconnected, this, &DaemonConnection::onDisconnected);
    connect(&m_socket, &QWebSocket::binaryMessageReceived,
            this, &DaemonConnection::onBinaryMessageReceived);
    connect(&m_socket, &QWebSocket::errorOccurred, this, &DaemonConnection::onError);

    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &DaemonConnection::onReconnectTimer);
}

bool DaemonConnection::isConnected() const {
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

void DaemonConnection::connectTo(const QUrl& url) {
    m_url = url;
    m_intentionalDisconnect = false;
    m_reconnectDelayMs = 1000;
    m_socket.open(url);
}

void DaemonConnection::disconnectFromDaemon() {
    m_intentionalDisconnect = true;
    m_reconnectTimer.stop();
    m_socket.close();
}

void DaemonConnection::onConnected() {
    qDebug() << "DaemonConnection: connected to" << m_url;
    m_reconnectDelayMs = 1000;
    sendSubscribe();
    emit connectionStateChanged(true);
}

void DaemonConnection::onDisconnected() {
    qDebug() << "DaemonConnection: disconnected";
    emit connectionStateChanged(false);
    if (!m_intentionalDisconnect)
        scheduleReconnect();
}

void DaemonConnection::onError(QAbstractSocket::SocketError err) {
    qWarning() << "DaemonConnection: socket error" << err << m_socket.errorString();
}

void DaemonConnection::onReconnectTimer() {
    qDebug() << "DaemonConnection: reconnecting to" << m_url;
    m_socket.open(m_url);
}

void DaemonConnection::scheduleReconnect() {
    qDebug() << "DaemonConnection: will retry in" << m_reconnectDelayMs << "ms";
    m_reconnectTimer.start(m_reconnectDelayMs);
    m_reconnectDelayMs = qMin(m_reconnectDelayMs * 2, 30000);
}

void DaemonConnection::sendSubscribe() {
    seq::v1::ClientEnvelope env;
    auto* sub = env.mutable_subscribe();
    sub->add_topics(seq::v1::Topic::SPAWNS);
    sub->add_topics(seq::v1::Topic::ZONE);
    sub->add_topics(seq::v1::Topic::PLAYER);
    sub->add_topics(seq::v1::Topic::CHAT);

    if (!m_sessionId.isEmpty()) {
        sub->set_session_id(m_sessionId.toStdString());
        sub->set_last_seq(m_lastSeq);
    }

    std::string bytes;
    if (!env.SerializeToString(&bytes)) {
        qWarning() << "DaemonConnection: failed to serialize Subscribe";
        return;
    }
    m_socket.sendBinaryMessage(QByteArray(bytes.data(), static_cast<int>(bytes.size())));
}

void DaemonConnection::onBinaryMessageReceived(const QByteArray& data) {
    seq::v1::Envelope env;
    if (!env.ParseFromArray(data.constData(), data.size())) {
        qWarning() << "DaemonConnection: failed to parse Envelope (" << data.size() << " bytes)";
        return;
    }

    m_lastSeq = env.seq();

    switch (env.payload_case()) {
    case seq::v1::Envelope::kSnapshot: {
        const auto& snap = env.snapshot();
        if (!snap.session_id().empty())
            m_sessionId = QString::fromStdString(snap.session_id());
        if (snap.has_geometry())
            emit mapGeometryReceived(snap.geometry());
        emit snapshotReceived(snap);
        break;
    }
    case seq::v1::Envelope::kZoneChanged: {
        const auto& zc = env.zone_changed();
        if (zc.has_geometry())
            emit mapGeometryReceived(zc.geometry());
        emit zoneChanged(zc);
        break;
    }
    case seq::v1::Envelope::kSpawnAdded:
        emit spawnAdded(env.spawn_added());
        break;
    case seq::v1::Envelope::kSpawnUpdated:
        emit spawnUpdated(env.spawn_updated());
        break;
    case seq::v1::Envelope::kSpawnRemoved:
        emit spawnRemoved(env.spawn_removed().id());
        break;
    case seq::v1::Envelope::kPlayerStats:
        emit playerStatsUpdated(env.player_stats());
        break;
    case seq::v1::Envelope::kChat:
        emit chatMessage(env.chat());
        break;
    case seq::v1::Envelope::kBuffs:
        emit buffsUpdated(env.buffs());
        break;
    default:
        break;
    }
}
