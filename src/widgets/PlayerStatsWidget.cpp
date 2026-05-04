#include "PlayerStatsWidget.h"
#include <QLabel>
#include <QVBoxLayout>

PlayerStatsWidget::PlayerStatsWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Player Stats (coming soon)", this));
    layout->addStretch();
}
