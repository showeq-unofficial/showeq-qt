#pragma once
#include <QSettings>
#include <QString>
#include <QUrl>

// Thin wrapper around QSettings so all key strings live in one place.
class Settings {
public:
    static Settings& instance();

    QUrl daemonUrl() const;
    void setDaemonUrl(const QUrl&);

    // Recent daemon URLs, most-recent-first, capped at kHistoryMax.
    QStringList daemonHistory() const;
    void addDaemonHistory(const QUrl&);
    static constexpr int kHistoryMax = 10;

    // Persists QMainWindow::saveGeometry() / saveState() blobs.
    QByteArray mainWindowGeometry() const;
    void setMainWindowGeometry(const QByteArray&);
    QByteArray mainWindowState() const;
    void setMainWindowState(const QByteArray&);

    // Spawn list column widths/visibility packed as a QByteArray from
    // QHeaderView::saveState().
    QByteArray spawnListHeaderState() const;
    void setSpawnListHeaderState(const QByteArray&);

    QString spawnListFilter() const;
    void setSpawnListFilter(const QString&);

    // Map visual settings.
    QColor mapBackground() const;
    void setMapBackground(const QColor&);

private:
    Settings();
    QSettings m_settings;
};
