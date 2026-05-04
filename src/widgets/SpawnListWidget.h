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

private slots:
    void onFilterChanged(const QString& text);

private:
    QTreeView*              m_view;
    QLineEdit*              m_filterEdit;
    SpawnModel*             m_model;
    QSortFilterProxyModel*  m_proxy;
};
