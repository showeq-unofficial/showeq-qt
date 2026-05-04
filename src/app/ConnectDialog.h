#pragma once
#include <QDialog>
#include <QStringList>
#include <QUrl>

class QComboBox;

class ConnectDialog : public QDialog {
    Q_OBJECT
public:
    explicit ConnectDialog(QWidget* parent = nullptr);
    QUrl url() const;
    void setUrl(const QUrl&);
    void setHistory(const QStringList& urls);

private:
    QComboBox* m_urlCombo;
};
