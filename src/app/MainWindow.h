#pragma once
#include <QMainWindow>

class DaemonConnection;
class SpawnModel;
class ZoneState;
class PlayerState;
class SpawnListWidget;
class MapWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent*) override;

private slots:
    void onConnectionStateChanged(bool connected);
    void onZoneChanged(const QString& zoneShort, const QString& zoneLong);
    void openConnectDialog();

private:
    void setupDocks();
    void setupMenus();
    void connectDaemonSignals();
    void updateTitle();

    DaemonConnection* m_conn;
    SpawnModel*       m_spawnModel;
    ZoneState*        m_zoneState;
    PlayerState*      m_playerState;
    SpawnListWidget*  m_spawnList;
    MapWidget*        m_map;

    bool    m_connected{false};
    QString m_zoneShort;
};
