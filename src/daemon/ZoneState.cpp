#include "ZoneState.h"

ZoneState::ZoneState(QObject* parent) : QObject(parent) {}

void ZoneState::applySnapshot(const seq::v1::Snapshot& snap) {
    m_zoneShort = QString::fromStdString(snap.zone_short());
    m_zoneLong  = QString::fromStdString(snap.zone_long());
    m_playerId  = snap.player_id();
    emit zoneChanged(m_zoneShort, m_zoneLong);
}

void ZoneState::applyZoneChanged(const seq::v1::ZoneChanged& zc) {
    m_zoneShort = QString::fromStdString(zc.zone_short());
    m_zoneLong  = QString::fromStdString(zc.zone_long());
    emit zoneChanged(m_zoneShort, m_zoneLong);
}
