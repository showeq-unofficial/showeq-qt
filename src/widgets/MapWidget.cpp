#include "MapWidget.h"
#include "app/Settings.h"
#include "daemon/ZoneState.h"
#include <QColor>
#include <QGraphicsEllipseItem>
#include <QGraphicsItemGroup>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QPainterPath>
#include <QMouseEvent>
#include <QScrollBar>
#include <QWheelEvent>
#include <QtMath>

// Daemon ships coords in screen convention (+X right, +Y down) — see
// the Pos message comment in seq/v1/events.proto. The transform is
// identity; QGraphicsScene's default axes already match.
QPointF MapWidget::eqToScene(float x, float y) {
    return QPointF(static_cast<double>(x), static_cast<double>(y));
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
    // ItemIgnoresTransformations items (spawn dots, player dot) confuse
    // QGraphicsView's per-item dirty-rect calc when scrolling — the
    // pixel-sized bounding rect doesn't track scene scroll deltas, so
    // Qt's scrollContents fast path leaves stale pixels (streaks). Force
    // a full repaint on every viewport change to eliminate them.
    setViewportUpdateMode(FullViewportUpdate);
    // BSP indexing is rebuilt on every item move (spawn updates fire
    // dozens per second). With a moderate item count (batched lines +
    // dots) we don't benefit from the spatial index — linear scan is
    // cheaper than the rebuild cost.
    m_scene->setItemIndexMethod(QGraphicsScene::NoIndex);
}

void MapWidget::applyGeometry(const seq::v1::MapGeometry& geo) {
    // Wipe previous geometry items (lines + locations); keep spawn dots
    // and the player marker.
    for (auto* item : m_geometryItems) {
        m_scene->removeItem(item);
        delete item;
    }
    m_geometryItems.clear();

    // Batch line segments by color into one QPainterPath per color, then
    // create a single QGraphicsPathItem per color. A typical zone has
    // tens of thousands of segments — adding them as individual line
    // items wrecks paint performance even at idle (every paint walks the
    // scene graph). Grouping by color also lets Qt collapse pen state
    // changes into one draw call per group.
    QHash<QString, QPainterPath> pathsByColor;
    for (const auto& line : geo.lines()) {
        const int n = line.x_size();
        if (n < 2) continue;
        const QString colorKey = QString::fromStdString(line.color());
        QPainterPath& path = pathsByColor[colorKey];
        for (int i = 0; i + 1 < n; ++i) {
            QPointF a = eqToScene(ZoneState::toWorld(line.x(i)),
                                  ZoneState::toWorld(line.y(i)));
            QPointF b = eqToScene(ZoneState::toWorld(line.x(i + 1)),
                                  ZoneState::toWorld(line.y(i + 1)));
            path.moveTo(a);
            path.lineTo(b);
        }
    }
    for (auto it = pathsByColor.constBegin(); it != pathsByColor.constEnd(); ++it) {
        QColor color(it.key());
        if (!color.isValid()) color = Qt::darkGray;
        QPen pen(color);
        pen.setCosmetic(true);
        auto* item = new QGraphicsPathItem(it.value());
        item->setPen(pen);
        item->setBrush(Qt::NoBrush);
        item->setZValue(0.0);
        m_scene->addItem(item);
        m_geometryItems.push_back(item);
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
        text->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        m_geometryItems.push_back(text);
    }

    fitInView(m_scene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void MapWidget::applySnapshot(const seq::v1::Snapshot& snap) {
    m_playerId = snap.player_id();
    m_spawns.clear();
    m_havePlayer = false;

    for (const auto& s : snap.spawns()) {
        if (!s.has_pos()) continue;
        float x = ZoneState::toWorld(s.pos().x());
        float y = ZoneState::toWorld(s.pos().y());
        if (s.id() == m_playerId) {
            m_havePlayer    = true;
            m_playerX       = x;
            m_playerY       = y;
            m_playerHeading = s.pos().heading();
        } else {
            m_spawns.insert(s.id(), {x, y, s.type()});
        }
    }
    viewport()->update();
}

void MapWidget::applySpawnAdded(const seq::v1::SpawnAdded& msg) {
    const auto& s = msg.spawn();
    if (!s.has_pos()) return;
    float x = ZoneState::toWorld(s.pos().x());
    float y = ZoneState::toWorld(s.pos().y());
    if (s.id() == m_playerId) {
        m_havePlayer    = true;
        m_playerX       = x;
        m_playerY       = y;
        m_playerHeading = s.pos().heading();
    } else {
        m_spawns.insert(s.id(), {x, y, s.type()});
    }
    viewport()->update();
}

void MapWidget::applySpawnUpdated(const seq::v1::SpawnUpdated& msg) {
    if (!msg.has_pos()) return;
    float x = ZoneState::toWorld(msg.pos().x());
    float y = ZoneState::toWorld(msg.pos().y());
    if (msg.id() == m_playerId) {
        m_havePlayer    = true;
        m_playerX       = x;
        m_playerY       = y;
        m_playerHeading = msg.pos().heading();
    } else {
        auto it = m_spawns.find(msg.id());
        if (it == m_spawns.end()) return;
        it->x = x;
        it->y = y;
    }
    viewport()->update();
}

void MapWidget::applySpawnRemoved(quint32 id) {
    if (id == m_playerId) {
        m_havePlayer = false;
    } else {
        m_spawns.remove(id);
    }
    viewport()->update();
}

void MapWidget::setPlayerId(quint32 id) {
    m_playerId = id;
}

void MapWidget::centerOnSpawn(quint32 id) {
    if (id == m_playerId && m_havePlayer) {
        centerOn(eqToScene(m_playerX, m_playerY));
        return;
    }
    auto it = m_spawns.find(id);
    if (it == m_spawns.end()) return;
    centerOn(eqToScene(it->x, it->y));
}

QColor MapWidget::colorForSpawnType(seq::v1::SpawnType type) {
    switch (type) {
    case seq::v1::PC:         return Qt::cyan;
    case seq::v1::CORPSE_PC:  return Qt::gray;
    case seq::v1::CORPSE_NPC: return Qt::darkGray;
    default:                  return Qt::red;
    }
}

// Player marker is rendered with a manual rotate around the player's
// scene position. FoV ellipse + cone live in scene coords (scale with
// zoom); the player dot is drawn in pixel coords for constant size.
void MapWidget::drawPlayerMarker(QPainter* painter) {
    if (!m_havePlayer) return;

    const QPointF playerScene = eqToScene(m_playerX, m_playerY);

    // ===== FoV (scene coords) =====
    constexpr double fovRadius = 25.0;
    constexpr double coneHalf  = 45.0;

    painter->save();
    painter->translate(playerScene);
    painter->rotate(m_playerHeading);

    QPen ellipsePen(QColor(80, 80, 80, 153)); ellipsePen.setCosmetic(true);
    painter->setPen(ellipsePen);
    painter->setBrush(QColor(80, 80, 80, 56));
    painter->drawEllipse(QPointF(0, 0), fovRadius, fovRadius);

    QPen forwardPen(QColor("#ffd200"));
    forwardPen.setCosmetic(true);
    forwardPen.setWidth(2);
    painter->setPen(forwardPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawLine(QPointF(0, 0), QPointF(0, -fovRadius));

    const double rad = qDegreesToRadians(coneHalf);
    const double dx  = fovRadius * std::sin(rad);
    const double dy  = -fovRadius * std::cos(rad);
    QPen conePen(QColor("#e74c3c"));
    conePen.setCosmetic(true);
    painter->setPen(conePen);
    painter->drawLine(QPointF(0, 0), QPointF(-dx, dy));
    painter->drawLine(QPointF(0, 0), QPointF( dx, dy));
    painter->restore();

    // ===== Pixel-sized dot (viewport coords) =====
    painter->save();
    painter->resetTransform();
    constexpr int r = 5;
    QPoint vpPos = mapFromScene(playerScene);
    painter->setBrush(Qt::yellow);
    painter->setPen(QPen(Qt::black, 1));
    painter->drawEllipse(vpPos, r, r);
    painter->restore();
}


void MapWidget::paintEvent(QPaintEvent* ev) {
    QElapsedTimer t; t.start();
    QGraphicsView::paintEvent(ev);
    m_lastPaintUs = t.nsecsElapsed() / 1000;
}

// FPS + spawn-count overlay. Drawn in viewport pixel coords (top-right).
// Useful given the FullViewportUpdate mode — lets us spot frame-rate
// drops as spawn count grows.
void MapWidget::drawForeground(QPainter* painter, const QRectF& sceneRect) {
    QElapsedTimer fgTimer; fgTimer.start();
    QGraphicsView::drawForeground(painter, sceneRect);

    // ===== Player FoV (scene coords) + dot (pixel coords) =====
    drawPlayerMarker(painter);

    // ===== Batched spawn dots =====
    // One drawPoints call per color — handles thousands of spawns
    // cheaply. RoundCap + thick pen turns each "point" into a filled
    // circle of (penWidth) pixels. mapFromScene is a 2D matrix multiply,
    // negligible per-spawn even at 5000+.
    if (!m_spawns.isEmpty()) {
        QHash<QRgb, QVector<QPointF>> byColor;
        byColor.reserve(4);
        for (auto it = m_spawns.constBegin(); it != m_spawns.constEnd(); ++it) {
            const QPoint vp = mapFromScene(eqToScene(it->x, it->y));
            byColor[colorForSpawnType(it->type).rgb()].push_back(vp);
        }

        painter->save();
        painter->resetTransform();
        QPen dotPen;
        dotPen.setCapStyle(Qt::RoundCap);
        dotPen.setWidth(6);  // ~3px radius — matches showeq-web
        for (auto it = byColor.constBegin(); it != byColor.constEnd(); ++it) {
            dotPen.setColor(QColor::fromRgb(it.key()));
            painter->setPen(dotPen);
            painter->drawPoints(it.value().constData(), it.value().size());
        }
        painter->restore();
    }

    // Record this frame's interval into the ring buffer.
    if (m_fpsTimer.isValid()) {
        const qint64 elapsed = m_fpsTimer.restart();
        m_frameTimes[m_frameTimesIdx] = elapsed;
        m_frameTimesIdx = (m_frameTimesIdx + 1) % m_frameTimes.size();
        if (m_frameTimesCount < static_cast<int>(m_frameTimes.size()))
            m_frameTimesCount++;
    } else {
        m_fpsTimer.start();
    }

    if (!m_showFps) {
        m_lastForegroundUs = fgTimer.nsecsElapsed() / 1000;
        return;
    }

    double avgMs = 0;
    if (m_frameTimesCount > 0) {
        qint64 sum = 0;
        for (int i = 0; i < m_frameTimesCount; ++i) sum += m_frameTimes[i];
        avgMs = static_cast<double>(sum) / m_frameTimesCount;
    }
    const double fps = avgMs > 0 ? (1000.0 / avgMs) : 0.0;

    // Spawn count includes the player marker if present.
    const int spawnCount = m_spawns.size() + (m_havePlayer ? 1 : 0);

    painter->save();
    painter->resetTransform();
    QFont f = painter->font();
    f.setPointSize(9);
    painter->setFont(f);

    // Paint cost (ms) is the headline number — it reflects render
    // capability. The "rate" is paint frequency in Hz; for a passive
    // data-driven view that mostly tracks the daemon's spawn-update
    // tempo (~5 Hz when idle), so a low value is normal and not
    // indicative of a perf problem.
    const QString text = QString("Paint %1ms  |  Rate %2 Hz  |  Spawns %3")
                            .arg(QString::number(m_lastPaintUs / 1000.0, 'f', 1))
                            .arg(QString::number(fps, 'f', 0))
                            .arg(spawnCount);

    const QRect vp = viewport()->rect();
    QFontMetrics fm(f);
    constexpr int padX   = 8;
    constexpr int padY   = 4;
    constexpr int margin = 8;
    const int textW = fm.horizontalAdvance(text);
    const int textH = fm.height();
    const QRect textRect(vp.width() - textW - padX * 2 - margin,
                         margin,
                         textW + padX * 2,
                         textH + padY * 2);

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(0, 0, 0, 160));
    painter->drawRoundedRect(textRect, 4, 4);

    // Color the readout by paint *cost* (capability), not paint rate —
    // rate is data-driven and a low number is healthy.
    const double paintMs = m_lastPaintUs / 1000.0;
    painter->setPen(paintMs > 16.0 ? QColor("#ff7070") : QColor("#dddddd"));
    painter->drawText(textRect, Qt::AlignCenter, text);
    painter->restore();

    m_lastForegroundUs = fgTimer.nsecsElapsed() / 1000;
}

// Grid + coord labels overlay. Lines are drawn in scene coords (cosmetic
// pen → 1px regardless of zoom). Labels are drawn in viewport pixel
// coords so they stay readable at any zoom level. Mirrors showeq-web's
// MapCanvas grid rendering (mapcore.cpp:1666 + showeq-web/MapCanvas.tsx).
void MapWidget::drawBackground(QPainter* painter, const QRectF& sceneRect) {
    QElapsedTimer bgTimer; bgTimer.start();
    QGraphicsView::drawBackground(painter, sceneRect);
    if (!m_showGrid) {
        m_lastBackgroundUs = bgTimer.nsecsElapsed() / 1000;
        return;
    }

    // 50 world units between grid lines. Web uses 500 fixed-point units;
    // we divide raw EQ coords by 8 for world units, so 500/8 ≈ 62 — 50
    // is the nearest round number that gives readable density at typical
    // zone sizes (~600-800 world units across).
    constexpr double res = 50.0;

    QPen linePen(QColor(25, 72, 25));   // dark green
    linePen.setCosmetic(true);
    painter->setPen(linePen);

    const double startX = std::ceil(sceneRect.left() / res) * res;
    const double startY = std::ceil(sceneRect.top()  / res) * res;
    for (double x = startX; x <= sceneRect.right(); x += res) {
        painter->drawLine(QPointF(x, sceneRect.top()),
                          QPointF(x, sceneRect.bottom()));
    }
    for (double y = startY; y <= sceneRect.bottom(); y += res) {
        painter->drawLine(QPointF(sceneRect.left(),  y),
                          QPointF(sceneRect.right(), y));
    }

    // Labels — draw in viewport pixel coords so they stay readable.
    painter->save();
    painter->resetTransform();
    painter->setPen(QColor(225, 200, 25));  // muted yellow
    QFont f = painter->font();
    f.setPointSize(9);
    painter->setFont(f);

    const QRect vp = viewport()->rect();
    for (double x = startX; x <= sceneRect.right(); x += res) {
        QPoint vpPos = mapFromScene(QPointF(x, sceneRect.bottom()));
        painter->drawText(vpPos.x() + 2, vp.height() - 2,
                          QString::number(static_cast<int>(x)));
    }
    for (double y = startY; y <= sceneRect.bottom(); y += res) {
        QPoint vpPos = mapFromScene(QPointF(sceneRect.left(), y));
        painter->drawText(2, vpPos.y() - 2,
                          QString::number(static_cast<int>(y)));
    }
    painter->restore();
    m_lastBackgroundUs = bgTimer.nsecsElapsed() / 1000;
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
