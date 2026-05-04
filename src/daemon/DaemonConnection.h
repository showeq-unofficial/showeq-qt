#pragma once
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QWebSocket>
#include <cstdint>

// Forward-declare proto types to avoid leaking generated headers into callers.
namespace seq { namespace v1 {
class Snapshot;
class SpawnAdded;
class SpawnUpdated;
class ZoneChanged;
class PlayerStats;
class MapGeometry;
class ChatMessage;
class BuffsUpdate;
} }

class DaemonConnection : public QObject {
    Q_OBJECT
public:
    explicit DaemonConnection(QObject* parent = nullptr);

    bool isConnected() const;
    QString sessionId() const { return m_sessionId; }

public slots:
    void connectTo(const QUrl& url);
    void disconnectFromDaemon();

signals:
    void connectionStateChanged(bool connected);
    void snapshotReceived(seq::v1::Snapshot);
    void spawnAdded(seq::v1::SpawnAdded);
    void spawnUpdated(seq::v1::SpawnUpdated);
    void spawnRemoved(quint32 id);
    void zoneChanged(seq::v1::ZoneChanged);
    void playerStatsUpdated(seq::v1::PlayerStats);
    void mapGeometryReceived(seq::v1::MapGeometry);
    void chatMessage(seq::v1::ChatMessage);
    void buffsUpdated(seq::v1::BuffsUpdate);

private slots:
    void onConnected();
    void onDisconnected();
    void onBinaryMessageReceived(const QByteArray& data);
    void onError(QAbstractSocket::SocketError);
    void onReconnectTimer();

private:
    void sendSubscribe();
    void scheduleReconnect();

    QWebSocket m_socket;
    QTimer     m_reconnectTimer;
    QUrl       m_url;
    QString    m_sessionId;
    quint64    m_lastSeq{0};
    int        m_reconnectDelayMs{1000};
    bool       m_intentionalDisconnect{false};
};
