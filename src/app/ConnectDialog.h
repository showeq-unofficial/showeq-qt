#pragma once
#include <QDialog>
#include <QUrl>

class QLineEdit;

class ConnectDialog : public QDialog {
    Q_OBJECT
public:
    explicit ConnectDialog(QWidget* parent = nullptr);
    QUrl url() const;
    void setUrl(const QUrl&);

private:
    QLineEdit* m_urlEdit;
};
