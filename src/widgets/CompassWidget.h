#pragma once
#include <QWidget>

class CompassWidget : public QWidget {
    Q_OBJECT
public:
    explicit CompassWidget(QWidget* parent = nullptr);
protected:
    void paintEvent(QPaintEvent*) override;
};
