#include "PlayerState.h"

PlayerState::PlayerState(QObject* parent) : QObject(parent) {}

void PlayerState::applyStats(const seq::v1::PlayerStats& s) {
    m_stats = s;
    emit statsChanged();
}
