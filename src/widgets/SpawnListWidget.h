#pragma once
#include <QWidget>

class SpawnModel;
class QSortFilterProxyModel;
class QTreeView;
class QLineEdit;

class SpawnListWidget : public QWidget {
    Q_OBJECT
public:
    explicit SpawnListWidget(SpawnModel* model, QWidget* parent = nullptr);

    void saveHeaderState();
    void restoreHeaderState();

signals:
    void centerOnSpawn(quint32 id);

private slots:
    void onFilterChanged(const QString& text);
    void onRowDoubleClicked(const QModelIndex& proxyIndex);

private:
    QTreeView*              m_view;
    QLineEdit*              m_filterEdit;
    SpawnModel*             m_model;
    QSortFilterProxyModel*  m_proxy;
};
