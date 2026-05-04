#include "MessageWindow.h"
#include <QLabel>
#include <QVBoxLayout>

MessageWindow::MessageWindow(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Messages (coming soon)", this));
    layout->addStretch();
}
