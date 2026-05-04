#include "MapWidget.h"
#include "app/Settings.h"
#include "daemon/ZoneState.h"
#include <QColor>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QWheelEvent>

// EQ coordinate → scene coordinate.
// EQ: +X=East, +Y=North; Scene: +X=right, +Y=down → negate Y.
QPointF MapWidget::eqToScene(float x, float y) {
    return QPointF(static_cast<double>(x), static_cast<double>(-y));
}

MapWidget::MapWidget(QWidget* parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);

    setBackgroundBrush(Settings::instance().mapBackground());
    setDragMode(NoDrag);
    setTransformationAnchor(AnchorUnderMouse);
    setRenderHint(QPainter::Antialiasing, false);
}

void MapWidget::applyGeometry(const seq::v1::MapGeometry& geo) {
    // Remove only geometry items; keep spawn dots.
    for (auto* item : m_scene->items()) {
        if (!dynamic_cast<QGraphicsEllipseItem*>(item))
            m_scene->removeItem(item);
    }

    for (const auto& line : geo.lines()) {
        QColor color(QString::fromStdString(line.color()));
        if (!color.isValid()) color = Qt::darkGray;
        QPen pen(color);
        pen.setCosmetic(true);

        int n = line.x_size();
        for (int i = 0; i + 1 < n; ++i) {
            float x0 = ZoneState::toWorld(line.x(i));
            float y0 = ZoneState::toWorld(line.y(i));
            float x1 = ZoneState::toWorld(line.x(i + 1));
            float y1 = ZoneState::toWorld(line.y(i + 1));
            auto* li = m_scene->addLine(
                eqToScene(x0, y0).x(), eqToScene(x0, y0).y(),
                eqToScene(x1, y1).x(), eqToScene(x1, y1).y(),
                pen);
            li->setZValue(0.0);
        }
    }

    for (const auto& loc : geo.locations()) {
        float x = ZoneState::toWorld(loc.x());
        float y = ZoneState::toWorld(loc.y());
        QColor color(QString::fromStdString(loc.color()));
        if (!color.isValid()) color = Qt::lightGray;
        auto* text = m_scene->addSimpleText(QString::fromStdString(loc.name()));
        text->setPos(eqToScene(x, y));
        text->setBrush(color);
        text->setZValue(1.0);
        // Make text scale-independent.
        text->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    }

    fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void MapWidget::applySnapshot(const seq::v1::Snapshot& snap) {
    m_playerId = snap.player_id();
    // Remove all spawn dots.
    for (auto* dot : m_dots)
        m_scene->removeItem(dot);
    m_dots.clear();

    for (const auto& s : snap.spawns()) {
        if (!s.has_pos()) continue;
        float x = ZoneState::toWorld(s.pos().x());
        float y = ZoneState::toWorld(s.pos().y());
        upsertSpawnDot(s.id(), x, y, s.type(), s.id() == m_playerId);
    }
}

void MapWidget::applySpawnAdded(const seq::v1::SpawnAdded& msg) {
    const auto& s = msg.spawn();
    if (!s.has_pos()) return;
    float x = ZoneState::toWorld(s.pos().x());
    float y = ZoneState::toWorld(s.pos().y());
    upsertSpawnDot(s.id(), x, y, s.type(), s.id() == m_playerId);
}

void MapWidget::applySpawnUpdated(const seq::v1::SpawnUpdated& msg) {
    if (!msg.has_pos()) return;
    float x = ZoneState::toWorld(msg.pos().x());
    float y = ZoneState::toWorld(msg.pos().y());
    auto it = m_dots.find(msg.id());
    if (it == m_dots.end()) return;
    QPointF p = eqToScene(x, y);
    constexpr double r = 4.0;
    it.value()->setPos(p.x() - r, p.y() - r);
}

void MapWidget::applySpawnRemoved(quint32 id) {
    auto it = m_dots.find(id);
    if (it == m_dots.end()) return;
    m_scene->removeItem(it.value());
    delete it.value();
    m_dots.erase(it);
}

void MapWidget::setPlayerId(quint32 id) {
    m_playerId = id;
}

void MapWidget::upsertSpawnDot(quint32 id, float x, float y,
                                seq::v1::SpawnType type, bool isPlayer)
{
    constexpr double r = 4.0;
    QColor color;
    double zval = 2.0;
    if (isPlayer) {
        color = Qt::yellow;
        zval = 3.0;
    } else {
        switch (type) {
        case seq::v1::PC:         color = Qt::cyan;     break;
        case seq::v1::CORPSE_PC:  color = Qt::gray;     break;
        case seq::v1::CORPSE_NPC: color = Qt::darkGray; break;
        default:                  color = Qt::red;       break;
        }
    }

    QPointF p = eqToScene(x, y);
    auto it = m_dots.find(id);
    QGraphicsEllipseItem* dot;
    if (it != m_dots.end()) {
        dot = it.value();
    } else {
        dot = m_scene->addEllipse(0, 0, r * 2, r * 2);
        m_dots[id] = dot;
    }
    dot->setPos(p.x() - r, p.y() - r);
    dot->setBrush(color);
    dot->setPen(Qt::NoPen);
    dot->setZValue(zval);
}

void MapWidget::wheelEvent(QWheelEvent* ev) {
    const double factor = ev->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    scale(factor, factor);
}

void MapWidget::mousePressEvent(QMouseEvent* ev) {
    if (ev->button() == Qt::MiddleButton || ev->button() == Qt::LeftButton) {
        m_panStart = ev->pos();
        m_panning = true;
        setCursor(Qt::ClosedHandCursor);
    }
    QGraphicsView::mousePressEvent(ev);
}

void MapWidget::mouseMoveEvent(QMouseEvent* ev) {
    if (m_panning) {
        QPoint delta = ev->pos() - m_panStart;
        m_panStart = ev->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
    }
    QGraphicsView::mouseMoveEvent(ev);
}

void MapWidget::mouseReleaseEvent(QMouseEvent* ev) {
    m_panning = false;
    setCursor(Qt::ArrowCursor);
    QGraphicsView::mouseReleaseEvent(ev);
}
