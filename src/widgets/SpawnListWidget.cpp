#include "SpawnListWidget.h"
#include "app/Settings.h"
#include "daemon/SpawnModel.h"
#include <QHeaderView>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QVBoxLayout>

SpawnListWidget::SpawnListWidget(SpawnModel* model, QWidget* parent)
    : QWidget(parent), m_model(model)
{
    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText("Filter spawns…");
    m_filterEdit->setClearButtonEnabled(true);

    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(model);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setFilterKeyColumn(SpawnModel::ColName);

    m_view = new QTreeView(this);
    m_view->setModel(m_proxy);
    m_view->setRootIsDecorated(false);
    m_view->setAlternatingRowColors(true);
    m_view->setSortingEnabled(true);
    m_view->sortByColumn(SpawnModel::ColName, Qt::AscendingOrder);
    m_view->header()->setStretchLastSection(false);
    m_view->header()->setSectionResizeMode(SpawnModel::ColName, QHeaderView::Stretch);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);
    layout->addWidget(m_filterEdit);
    layout->addWidget(m_view);

    connect(m_filterEdit, &QLineEdit::textChanged, this, &SpawnListWidget::onFilterChanged);
}

void SpawnListWidget::onFilterChanged(const QString& text) {
    m_proxy->setFilterFixedString(text);
    Settings::instance().setSpawnListFilter(text);
}

void SpawnListWidget::saveHeaderState() {
    Settings::instance().setSpawnListHeaderState(m_view->header()->saveState());
}

void SpawnListWidget::restoreHeaderState() {
    auto state = Settings::instance().spawnListHeaderState();
    if (!state.isEmpty())
        m_view->header()->restoreState(state);
    m_filterEdit->setText(Settings::instance().spawnListFilter());
}
