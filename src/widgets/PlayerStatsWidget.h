#pragma once
#include <QWidget>

class PlayerState;
class QLabel;
class QProgressBar;

class PlayerStatsWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlayerStatsWidget(PlayerState* state, QWidget* parent = nullptr);

private slots:
    void refresh();

private:
    PlayerState*  m_state;
    QLabel*       m_nameLabel;
    QLabel*       m_classLabel;
    QProgressBar* m_hp;
    QProgressBar* m_mana;
    QProgressBar* m_end;
    QProgressBar* m_exp;
    QLabel*       m_aaLabel;
};
