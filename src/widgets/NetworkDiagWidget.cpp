#include "NetworkDiagWidget.h"
#include <QLabel>
#include <QVBoxLayout>

NetworkDiagWidget::NetworkDiagWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Network Diagnostics (coming soon)", this));
    layout->addStretch();
}
