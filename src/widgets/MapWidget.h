#pragma once
#include "seq/v1/events.pb.h"
#include <QElapsedTimer>
#include <QGraphicsView>
#include <QHash>
#include <QList>
#include <array>

class QGraphicsScene;
class QGraphicsEllipseItem;
class QGraphicsItemGroup;
class QGraphicsLineItem;

class MapWidget : public QGraphicsView {
    Q_OBJECT
public:
    explicit MapWidget(QWidget* parent = nullptr);

public:
    void centerOnSpawn(quint32 id);
    void setShowGrid(bool on) { m_showGrid = on; viewport()->update(); }
    bool showGrid() const { return m_showGrid; }
    void setShowFps(bool on) { m_showFps = on; viewport()->update(); }
    bool showFps() const { return m_showFps; }

public slots:
    void applyGeometry(const seq::v1::MapGeometry&);
    void applySnapshot(const seq::v1::Snapshot&);
    void applySpawnAdded(const seq::v1::SpawnAdded&);
    void applySpawnUpdated(const seq::v1::SpawnUpdated&);
    void applySpawnRemoved(quint32 id);
    void setPlayerId(quint32 id);

protected:
    void paintEvent(QPaintEvent*) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;
    void wheelEvent(QWheelEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    static QPointF eqToScene(float x, float y);
    static QColor  colorForSpawnType(seq::v1::SpawnType);
    void drawPlayerMarker(QPainter* painter);

    QGraphicsScene* m_scene;
    quint32 m_playerId{0};

    // Spawns are rendered in drawForeground (batched via drawPoints) —
    // NOT as individual QGraphicsItems. With 5000+ spawns, individual
    // items with ItemIgnoresTransformations destroy frame rate (each
    // requires its own inverse-transform computation per paint). One
    // batched draw per color is much cheaper.
    struct SpawnRender {
        float              x{0}, y{0};
        seq::v1::SpawnType type{seq::v1::SPAWN_UNSPECIFIED};
    };
    QHash<quint32, SpawnRender> m_spawns;

    // Player position + heading, also rendered in drawForeground.
    bool  m_havePlayer{false};
    float m_playerX{0}, m_playerY{0};
    int   m_playerHeading{0};

    // Map geometry items (lines + locations) live in the scene because
    // they're static between zone changes. Tracked separately so
    // applyGeometry can wipe just these. Lines are batched into one
    // QGraphicsPathItem per color.
    QList<QGraphicsItem*> m_geometryItems;

    // Grid overlay (drawn in drawBackground so it sits under map lines).
    bool m_showGrid{true};

    // FPS overlay — ring buffer of recent inter-frame intervals (ms),
    // averaged for the on-screen readout. Sized for ~half-second window
    // at 60 FPS.
    bool                       m_showFps{true};
    QElapsedTimer              m_fpsTimer;
    std::array<qint64, 30>     m_frameTimes{};
    int                        m_frameTimesIdx{0};
    int                        m_frameTimesCount{0};

    // Diagnostic — last paintEvent / drawBackground / drawForeground
    // wall-clock durations in microseconds. Updated on each paint, shown
    // alongside the FPS readout.
    qint64 m_lastPaintUs{0};
    qint64 m_lastBackgroundUs{0};
    qint64 m_lastForegroundUs{0};

    // Pan state.
    QPoint  m_panStart;
    bool    m_panning{false};
};
