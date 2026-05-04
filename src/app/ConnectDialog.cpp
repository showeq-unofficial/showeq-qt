#include "ConnectDialog.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

ConnectDialog::ConnectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Connect to Daemon");
    setModal(true);

    m_urlCombo = new QComboBox(this);
    m_urlCombo->setEditable(true);
    m_urlCombo->setInsertPolicy(QComboBox::NoInsert); // dedupe handled in Settings
    m_urlCombo->setMinimumWidth(360);
    m_urlCombo->lineEdit()->setPlaceholderText("ws://127.0.0.1:9090");

    auto* form = new QFormLayout;
    form->addRow("Daemon URL:", m_urlCombo);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

QUrl ConnectDialog::url() const {
    return QUrl(m_urlCombo->currentText().trimmed());
}

void ConnectDialog::setUrl(const QUrl& url) {
    m_urlCombo->setEditText(url.toString());
}

void ConnectDialog::setHistory(const QStringList& urls) {
    // Preserve any text the caller already typed.
    const QString current = m_urlCombo->currentText();
    m_urlCombo->clear();
    m_urlCombo->addItems(urls);
    m_urlCombo->setEditText(current);
}
