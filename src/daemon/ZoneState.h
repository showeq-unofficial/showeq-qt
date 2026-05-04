#pragma once
#include "seq/v1/events.pb.h"
#include <QObject>
#include <QString>
#include <unordered_map>

class ZoneState : public QObject {
    Q_OBJECT
public:
    explicit ZoneState(QObject* parent = nullptr);

    QString zoneShort() const { return m_zoneShort; }
    QString zoneLong()  const { return m_zoneLong; }
    quint32 playerId()  const { return m_playerId; }

    // Returns world-unit coordinate from raw fixed-point EQ value.
    // EQ positions are int16 with a 3-bit fractional shift.
    static float toWorld(int raw) { return raw / 8.0f; }

public slots:
    void applySnapshot(const seq::v1::Snapshot&);
    void applyZoneChanged(const seq::v1::ZoneChanged&);

signals:
    void zoneChanged(const QString& zoneShort, const QString& zoneLong);

private:
    QString m_zoneShort;
    QString m_zoneLong;
    quint32 m_playerId{0};
};
