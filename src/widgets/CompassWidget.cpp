#include "CompassWidget.h"
#include <QPainter>

CompassWidget::CompassWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(80, 80);
}

void CompassWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::black);
    p.setPen(Qt::darkGray);
    p.drawText(rect(), Qt::AlignCenter, "Compass\n(coming soon)");
}
