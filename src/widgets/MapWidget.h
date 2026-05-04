#pragma once
#include "seq/v1/events.pb.h"
#include <QGraphicsView>
#include <QHash>

class QGraphicsScene;
class QGraphicsEllipseItem;
class QGraphicsLineItem;

class MapWidget : public QGraphicsView {
    Q_OBJECT
public:
    explicit MapWidget(QWidget* parent = nullptr);

public slots:
    void applyGeometry(const seq::v1::MapGeometry&);
    void applySnapshot(const seq::v1::Snapshot&);
    void applySpawnAdded(const seq::v1::SpawnAdded&);
    void applySpawnUpdated(const seq::v1::SpawnUpdated&);
    void applySpawnRemoved(quint32 id);
    void setPlayerId(quint32 id);

protected:
    void wheelEvent(QWheelEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    static QPointF eqToScene(float x, float y);
    void upsertSpawnDot(quint32 id, float x, float y, seq::v1::SpawnType type, bool isPlayer);

    QGraphicsScene* m_scene;
    quint32 m_playerId{0};

    // Spawn dots keyed by spawn id.
    QHash<quint32, QGraphicsEllipseItem*> m_dots;

    // Pan state.
    QPoint  m_panStart;
    bool    m_panning{false};
};
