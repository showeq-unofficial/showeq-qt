#include "SpawnModel.h"
#include "daemon/ZoneState.h"
#include <QColor>
#include <QTimer>
#include <climits>
#include <cmath>

static const char* kClassNames[] = {
    "Unknown","Warrior","Cleric","Paladin","Ranger","SK","Druid",
    "Monk","Bard","Rogue","Shaman","Necro","Wiz","Mage","Enc","BST","Ber"
};
static const int kClassCount = static_cast<int>(sizeof(kClassNames)/sizeof(kClassNames[0]));

// Race-name table from showeq-daemon/src/races.h (mirrored verbatim).
// Sparse: indices without an entry default to NULL → fall back to a
// numeric string in raceName().
static const char* kRaceNames[] = {
#include "daemon/races.h"
};
static const int kRaceCount = static_cast<int>(sizeof(kRaceNames)/sizeof(kRaceNames[0]));

static QString raceName(uint32_t race) {
    if (race < static_cast<uint32_t>(kRaceCount) && kRaceNames[race])
        return QString::fromLatin1(kRaceNames[race]);
    return QString::number(race);
}

SpawnModel::SpawnModel(QObject* parent) : QAbstractTableModel(parent) {}

int SpawnModel::rowCount(const QModelIndex&) const { return m_rows.size(); }
int SpawnModel::columnCount(const QModelIndex&) const { return ColCount; }

QVariant SpawnModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section) {
    case ColName:  return "Name";
    case ColLevel: return "Lvl";
    case ColClass: return "Class";
    case ColRace:  return "Race";
    case ColHP:    return "HP%";
    case ColDist:  return "Dist";
    case ColX:     return "X";
    case ColY:     return "Y";
    case ColZ:     return "Z";
    }
    return {};
}

QVariant SpawnModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const SpawnRow& r = m_rows[index.row()];

    // EditRole carries the raw numeric value so the proxy sorts
    // numerically (set sortRole = EditRole on the proxy).
    if (role == Qt::EditRole) {
        switch (index.column()) {
        case ColLevel: return r.level;
        case ColHP:    return r.hpMax > 0 ? (100.0 * r.hpCur / r.hpMax) : -1.0;
        case ColDist:  {
            float d = distanceFromPlayer(index.row());
            return d < 0 ? QVariant() : QVariant(d);
        }
        case ColX: return r.x;
        case ColY: return r.y;
        case ColZ: return r.z;
        }
    }

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColName:  return r.name;
        case ColLevel: return r.level;
        case ColClass: {
            int c = static_cast<int>(r.classId);
            return (c > 0 && c < kClassCount) ? QString(kClassNames[c]) : QString::number(c);
        }
        case ColRace:  return raceName(r.raceId);
        case ColHP:
            return r.hpMax > 0
                ? QString::number(100u * r.hpCur / r.hpMax) + "%"
                : QString("?");
        case ColDist: {
            float d = distanceFromPlayer(index.row());
            return d < 0 ? QString("—") : QString::number(d, 'f', 0);
        }
        case ColX:     return QString::number(r.x, 'f', 1);
        case ColY:     return QString::number(r.y, 'f', 1);
        case ColZ:     return QString::number(r.z, 'f', 1);
        }
    }
    if (role == Qt::ForegroundRole) {
        switch (r.type) {
        case seq::v1::PC:         return QColor(Qt::cyan);
        case seq::v1::CORPSE_PC:  return QColor(Qt::gray);
        case seq::v1::CORPSE_NPC: return QColor(Qt::darkGray);
        default:                  return QColor(Qt::white);
        }
    }
    return {};
}

quint32 SpawnModel::spawnIdAt(int row) const {
    if (row < 0 || row >= m_rows.size()) return 0;
    return m_rows[row].id;
}

void SpawnModel::clear() {
    beginResetModel();
    m_rows.clear();
    m_index.clear();
    endResetModel();
}

SpawnRow SpawnModel::rowFromProto(const seq::v1::Spawn& s) {
    SpawnRow r;
    r.id      = s.id();
    r.name    = QString::fromStdString(s.name());
    r.level   = s.level();
    r.classId = s.class_();
    r.raceId  = s.race();
    r.hpCur   = s.hp_cur();
    r.hpMax   = s.hp_max();
    r.type    = s.type();
    if (s.has_pos()) {
        r.x = ZoneState::toWorld(s.pos().x());
        r.y = ZoneState::toWorld(s.pos().y());
        r.z = ZoneState::toWorld(s.pos().z());
    }
    return r;
}

void SpawnModel::applySnapshot(const seq::v1::Snapshot& snap) {
    beginResetModel();
    m_rows.clear();
    m_index.clear();
    m_playerId = snap.player_id();
    for (const auto& s : snap.spawns()) {
        m_index[s.id()] = m_rows.size();
        m_rows.push_back(rowFromProto(s));
    }
    endResetModel();
}

void SpawnModel::applySpawnAdded(const seq::v1::SpawnAdded& msg) {
    const auto& s = msg.spawn();
    if (m_index.contains(s.id()))
        return; // shouldn't happen, but guard duplicates
    beginInsertRows({}, m_rows.size(), m_rows.size());
    m_index[s.id()] = m_rows.size();
    m_rows.push_back(rowFromProto(s));
    endInsertRows();
}

void SpawnModel::applySpawnUpdated(const seq::v1::SpawnUpdated& msg) {
    int row = indexOf(msg.id());
    if (row < 0) return;
    SpawnRow& r = m_rows[row];
    if (msg.has_pos()) {
        r.x = ZoneState::toWorld(msg.pos().x());
        r.y = ZoneState::toWorld(msg.pos().y());
        r.z = ZoneState::toWorld(msg.pos().z());
    }
    if (msg.has_hp_cur()) r.hpCur = msg.hp_cur();
    if (msg.has_level())  r.level = msg.level();
    if (msg.has_name())   r.name  = QString::fromStdString(msg.name());

    scheduleRowDirty(row);
    // Player moved → every other row's distance is now stale.
    if (msg.has_pos() && msg.id() == m_playerId)
        m_dirtyAllDistances = true;
}

void SpawnModel::applySpawnRemoved(quint32 id) {
    int row = indexOf(id);
    if (row < 0) return;
    beginRemoveRows({}, row, row);
    m_index.remove(id);
    m_rows.remove(row);
    // Rebuild index for rows after the removed one.
    for (int i = row; i < m_rows.size(); ++i)
        m_index[m_rows[i].id] = i;
    endRemoveRows();
}

int SpawnModel::indexOf(quint32 id) const {
    auto it = m_index.find(id);
    return it != m_index.end() ? it.value() : -1;
}

float SpawnModel::distanceFromPlayer(int row) const {
    if (m_playerId == 0 || row < 0 || row >= m_rows.size()) return -1.0f;
    int playerRow = indexOf(m_playerId);
    if (playerRow < 0 || playerRow == row) return -1.0f;
    const SpawnRow& p = m_rows[playerRow];
    const SpawnRow& s = m_rows[row];
    float dx = s.x - p.x, dy = s.y - p.y, dz = s.z - p.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void SpawnModel::scheduleRowDirty(int row) {
    if (row < 0) return;
    m_dirtyRowMin = std::min(m_dirtyRowMin, row);
    m_dirtyRowMax = std::max(m_dirtyRowMax, row);
    if (!m_flushScheduled) {
        m_flushScheduled = true;
        QTimer::singleShot(50, this, &SpawnModel::flushDirtyRows);
    }
}

// 20 Hz flush — emits at most one dataChanged per coalescing window
// covering the full set of dirty rows. Keeps the QTreeView happy at
// 5000+ rows × 5 Hz spawn updates.
void SpawnModel::flushDirtyRows() {
    m_flushScheduled = false;
    if (m_dirtyRowMin <= m_dirtyRowMax) {
        const int lo = std::max(0, m_dirtyRowMin);
        const int hi = std::min(static_cast<int>(m_rows.size()) - 1, m_dirtyRowMax);
        if (lo <= hi)
            emit dataChanged(index(lo, 0), index(hi, ColCount - 1));
        m_dirtyRowMin = INT_MAX;
        m_dirtyRowMax = INT_MIN;
    }
    if (m_dirtyAllDistances && !m_rows.isEmpty()) {
        emit dataChanged(index(0, ColDist),
                         index(m_rows.size() - 1, ColDist));
        m_dirtyAllDistances = false;
    }
}
