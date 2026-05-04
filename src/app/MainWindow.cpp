#include "MainWindow.h"
#include "ConnectDialog.h"
#include "Settings.h"
#include "daemon/DaemonConnection.h"
#include "daemon/PlayerState.h"
#include "daemon/SpawnModel.h"
#include "daemon/ZoneState.h"
#include "widgets/CompassWidget.h"
#include "widgets/MapWidget.h"
#include "widgets/MessageWindow.h"
#include "widgets/NetworkDiagWidget.h"
#include "widgets/PlayerStatsWidget.h"
#include "widgets/SpawnListWidget.h"
#include "widgets/SpellListWidget.h"

#include <QAction>
#include <QCloseEvent>
#include <QDockWidget>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("ShowEQ Qt");
    resize(1280, 800);

    m_conn        = new DaemonConnection(this);
    m_spawnModel  = new SpawnModel(this);
    m_zoneState   = new ZoneState(this);
    m_playerState = new PlayerState(this);

    setupDocks();
    setupMenus();
    connectDaemonSignals();

    auto& s = Settings::instance();
    if (!s.mainWindowGeometry().isEmpty())
        restoreGeometry(s.mainWindowGeometry());
    if (!s.mainWindowState().isEmpty())
        restoreState(s.mainWindowState());
    m_spawnList->restoreHeaderState();

    m_conn->connectTo(s.daemonUrl());
    statusBar()->showMessage("Connecting…");
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent* ev) {
    m_spawnList->saveHeaderState();
    auto& s = Settings::instance();
    s.setMainWindowGeometry(saveGeometry());
    s.setMainWindowState(saveState());
    QMainWindow::closeEvent(ev);
}

void MainWindow::setupDocks() {
    // Map is the central widget (takes remaining space).
    m_map = new MapWidget(this);
    setCentralWidget(m_map);

    // Spawn list — left dock.
    m_spawnList = new SpawnListWidget(m_spawnModel, this);
    auto* spawnDock = new QDockWidget("Spawns", this);
    spawnDock->setObjectName("SpawnsDock");
    spawnDock->setWidget(m_spawnList);
    addDockWidget(Qt::LeftDockWidgetArea, spawnDock);

    // Stub docks — right side.
    auto makeStub = [this](const QString& title, QWidget* w, const QString& objName) {
        auto* dock = new QDockWidget(title, this);
        dock->setObjectName(objName);
        dock->setWidget(w);
        addDockWidget(Qt::RightDockWidgetArea, dock);
        return dock;
    };

    makeStub("Player Stats", new PlayerStatsWidget(this),  "StatsDock");
    makeStub("Spells",       new SpellListWidget(this),    "SpellsDock");
    makeStub("Messages",     new MessageWindow(this),      "MessagesDock");
    makeStub("Compass",      new CompassWidget(this),      "CompassDock");
    makeStub("Net Diag",     new NetworkDiagWidget(this),  "NetDiagDock");
}

void MainWindow::setupMenus() {
    auto* fileMenu = menuBar()->addMenu("&File");
    auto* connectAct = fileMenu->addAction("&Connect…", this, &MainWindow::openConnectDialog);
    connectAct->setShortcut(QKeySequence("Ctrl+Shift+C"));
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence::Quit);

    auto* viewMenu = menuBar()->addMenu("&View");
    // Populate dock toggle actions.
    for (auto* dock : findChildren<QDockWidget*>())
        viewMenu->addAction(dock->toggleViewAction());
}

void MainWindow::connectDaemonSignals() {
    connect(m_conn, &DaemonConnection::connectionStateChanged,
            this, &MainWindow::onConnectionStateChanged);
    connect(m_conn, &DaemonConnection::zoneChanged,
            m_zoneState, &ZoneState::applyZoneChanged);
    connect(m_conn, &DaemonConnection::snapshotReceived,
            m_zoneState, &ZoneState::applySnapshot);
    connect(m_zoneState, &ZoneState::zoneChanged,
            this, &MainWindow::onZoneChanged);

    // SpawnModel wiring.
    connect(m_conn, &DaemonConnection::snapshotReceived,
            m_spawnModel, &SpawnModel::applySnapshot);
    connect(m_conn, &DaemonConnection::spawnAdded,
            m_spawnModel, &SpawnModel::applySpawnAdded);
    connect(m_conn, &DaemonConnection::spawnUpdated,
            m_spawnModel, &SpawnModel::applySpawnUpdated);
    connect(m_conn, &DaemonConnection::spawnRemoved,
            m_spawnModel, &SpawnModel::applySpawnRemoved);

    // Map wiring.
    connect(m_conn, &DaemonConnection::mapGeometryReceived,
            m_map, &MapWidget::applyGeometry);
    connect(m_conn, &DaemonConnection::snapshotReceived,
            m_map, &MapWidget::applySnapshot);
    connect(m_conn, &DaemonConnection::spawnAdded,
            m_map, &MapWidget::applySpawnAdded);
    connect(m_conn, &DaemonConnection::spawnUpdated,
            m_map, &MapWidget::applySpawnUpdated);
    connect(m_conn, &DaemonConnection::spawnRemoved,
            m_map, &MapWidget::applySpawnRemoved);

    // Player state.
    connect(m_conn, &DaemonConnection::playerStatsUpdated,
            m_playerState, &PlayerState::applyStats);
}

void MainWindow::onConnectionStateChanged(bool connected) {
    m_connected = connected;
    updateTitle();
    statusBar()->showMessage(connected ? "Connected" : "Disconnected — retrying…");
}

void MainWindow::onZoneChanged(const QString& zoneShort, const QString& /*zoneLong*/) {
    m_zoneShort = zoneShort;
    updateTitle();
}

void MainWindow::updateTitle() {
    QString title = "ShowEQ Qt";
    if (!m_zoneShort.isEmpty())
        title += " — " + m_zoneShort;
    if (!m_connected)
        title += " [disconnected]";
    setWindowTitle(title);
}

void MainWindow::openConnectDialog() {
    ConnectDialog dlg(this);
    dlg.setUrl(Settings::instance().daemonUrl());
    if (dlg.exec() != QDialog::Accepted)
        return;
    QUrl url = dlg.url();
    if (!url.isValid()) return;
    Settings::instance().setDaemonUrl(url);
    m_conn->disconnectFromDaemon();
    m_conn->connectTo(url);
}
