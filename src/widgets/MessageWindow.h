#pragma once
#include <QWidget>

class MessageWindow : public QWidget {
    Q_OBJECT
public:
    explicit MessageWindow(QWidget* parent = nullptr);
};
