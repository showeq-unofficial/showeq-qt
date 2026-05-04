#include "ConnectDialog.h"
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

    m_urlEdit = new QLineEdit(this);
    m_urlEdit->setMinimumWidth(300);
    m_urlEdit->setPlaceholderText("ws://127.0.0.1:9090");

    auto* form = new QFormLayout;
    form->addRow("Daemon URL:", m_urlEdit);

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

QUrl ConnectDialog::url() const {
    return QUrl(m_urlEdit->text().trimmed());
}

void ConnectDialog::setUrl(const QUrl& url) {
    m_urlEdit->setText(url.toString());
}
