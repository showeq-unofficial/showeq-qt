#include "SpawnModel.h"
#include "daemon/ZoneState.h"
#include <QColor>

static const char* kClassNames[] = {
    "Unknown","Warrior","Cleric","Paladin","Ranger","SK","Druid",
    "Monk","Bard","Rogue","Shaman","Necro","Wiz","Mage","Enc","BST","Ber"
};
static const int kClassCount = static_cast<int>(sizeof(kClassNames)/sizeof(kClassNames[0]));

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
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ColName:  return r.name;
        case ColLevel: return r.level;
        case ColClass: {
            int c = static_cast<int>(r.classId);
            return (c > 0 && c < kClassCount) ? QString(kClassNames[c]) : QString::number(c);
        }
        case ColRace:  return r.raceId;
        case ColHP:
            return r.hpMax > 0
                ? QString::number(100u * r.hpCur / r.hpMax) + "%"
                : QString("?");
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
    emit dataChanged(index(row, 0), index(row, ColCount - 1));
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
