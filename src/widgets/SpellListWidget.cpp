#include "SpellListWidget.h"
#include <QLabel>
#include <QVBoxLayout>

SpellListWidget::SpellListWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Spell List (coming soon)", this));
    layout->addStretch();
}
