#pragma once
#include "seq/v1/events.pb.h"
#include <QAbstractTableModel>
#include <QHash>
#include <QVector>

struct SpawnRow {
    quint32 id{0};
    QString name;
    quint32 level{0};
    quint32 classId{0};
    quint32 raceId{0};
    quint32 hpCur{0};
    quint32 hpMax{0};
    float   x{0.0f};
    float   y{0.0f};
    float   z{0.0f};
    seq::v1::SpawnType type{seq::v1::SPAWN_UNSPECIFIED};
};

class SpawnModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column { ColName, ColLevel, ColClass, ColRace, ColHP, ColX, ColY, ColZ, ColCount };

    explicit SpawnModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& = {}) const override;
    int columnCount(const QModelIndex& = {}) const override;
    QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const override;

    quint32 spawnIdAt(int row) const;

public slots:
    void clear();
    void applySnapshot(const seq::v1::Snapshot&);
    void applySpawnAdded(const seq::v1::SpawnAdded&);
    void applySpawnUpdated(const seq::v1::SpawnUpdated&);
    void applySpawnRemoved(quint32 id);

private:
    static SpawnRow rowFromProto(const seq::v1::Spawn&);
    int indexOf(quint32 id) const;

    QVector<SpawnRow>      m_rows;
    QHash<quint32, int>    m_index; // id → row position
};
