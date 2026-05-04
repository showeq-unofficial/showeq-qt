#include "PlayerStatsWidget.h"
#include "daemon/PlayerState.h"
#include <QFormLayout>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

namespace {
// Mirrors the SpawnModel class table; kept private to this TU to avoid
// pulling SpawnModel headers in for a label lookup.
const char* className(uint32_t c) {
    static const char* names[] = {
        "Unknown","Warrior","Cleric","Paladin","Ranger","SK","Druid",
        "Monk","Bard","Rogue","Shaman","Necro","Wiz","Mage","Enc","BST","Ber"
    };
    constexpr int count = static_cast<int>(sizeof(names) / sizeof(names[0]));
    return (c < static_cast<uint32_t>(count)) ? names[c] : "?";
}

QProgressBar* makeBar(const QString& fmt, const QColor& color) {
    auto* bar = new QProgressBar;
    bar->setRange(0, 100);
    bar->setValue(0);
    bar->setTextVisible(true);
    bar->setFormat(fmt);
    bar->setStyleSheet(QString(
        "QProgressBar { border: 1px solid #555; border-radius: 2px; "
        "  text-align: center; background: #222; color: #fff; }"
        "QProgressBar::chunk { background: %1; }").arg(color.name()));
    return bar;
}
}

PlayerStatsWidget::PlayerStatsWidget(PlayerState* state, QWidget* parent)
    : QWidget(parent), m_state(state)
{
    m_nameLabel  = new QLabel("(no player)", this);
    m_classLabel = new QLabel("—", this);
    QFont nameFont = m_nameLabel->font();
    nameFont.setBold(true);
    nameFont.setPointSize(nameFont.pointSize() + 1);
    m_nameLabel->setFont(nameFont);

    m_hp   = makeBar("HP %v / %m",   QColor("#c0392b"));
    m_mana = makeBar("Mana %v / %m", QColor("#2980b9"));
    m_end  = makeBar("End %v / %m",  QColor("#d4ac0d"));
    m_exp  = makeBar("XP %p%",       QColor("#27ae60"));
    m_aaLabel = new QLabel("AA: 0 unspent", this);

    auto* form = new QFormLayout;
    form->addRow("Name",  m_nameLabel);
    form->addRow("Class", m_classLabel);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);
    layout->addLayout(form);
    layout->addWidget(m_hp);
    layout->addWidget(m_mana);
    layout->addWidget(m_end);
    layout->addWidget(m_exp);
    layout->addWidget(m_aaLabel);
    layout->addStretch();

    if (m_state)
        connect(m_state, &PlayerState::statsChanged, this, &PlayerStatsWidget::refresh);
    refresh();
}

void PlayerStatsWidget::refresh() {
    if (!m_state) return;
    const auto& s = m_state->stats();

    QString name = QString::fromStdString(s.name());
    if (name.isEmpty()) name = "(no player)";
    m_nameLabel->setText(name);
    m_classLabel->setText(QString("%1 / Level %2")
                           .arg(className(s.class_()))
                           .arg(s.level()));

    auto setBar = [](QProgressBar* bar, uint32_t cur, uint32_t max) {
        if (max == 0) {
            bar->setRange(0, 1);
            bar->setValue(0);
        } else {
            bar->setRange(0, static_cast<int>(max));
            bar->setValue(static_cast<int>(qMin(cur, max)));
        }
    };
    setBar(m_hp,   s.hp_cur(),        s.hp_max());
    setBar(m_mana, s.mana_cur(),      s.mana_max());
    setBar(m_end,  s.endurance_cur(), s.endurance_max());
    setBar(m_exp,  s.exp_cur(),       s.exp_max());

    m_aaLabel->setText(QString("AA: %1 unspent / %2 spent")
                       .arg(s.aa_unspent())
                       .arg(s.aa_points()));
}
