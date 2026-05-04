#pragma once
#include "seq/v1/events.pb.h"
#include <QObject>

// Caches player stats received from the daemon. Stub in v1.
class PlayerState : public QObject {
    Q_OBJECT
public:
    explicit PlayerState(QObject* parent = nullptr);

    const seq::v1::PlayerStats& stats() const { return m_stats; }

public slots:
    void applyStats(const seq::v1::PlayerStats& s);

signals:
    void statsChanged();

private:
    seq::v1::PlayerStats m_stats;
};
