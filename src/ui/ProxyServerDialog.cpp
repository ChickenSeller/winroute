#include "ProxyServerDialog.h"
#include "ProxyCheckerDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

ProxyServerDialog::ProxyServerDialog(const ProxyServer &server, bool httpSupport, QWidget *parent)
    : QDialog(parent), m_server(server)
{
    setWindowTitle(tr("Proxy Server"));
    auto *layout = new QVBoxLayout(this);

    auto *serverBox = new QGroupBox(tr("Server"));
    auto *sl = new QGridLayout(serverBox);
    m_address = new QLineEdit(server.address);
    m_address->setPlaceholderText(tr("IP address or host name"));
    m_port = new QSpinBox;
    m_port->setRange(1, 65535);
    m_port->setValue(server.port ? server.port : 1080);
    m_port->setMinimumWidth(80);
    m_protocol = new QComboBox;
    m_protocol->addItem(tr("SOCKS Version 4"), int(ProxyType::Socks4));
    m_protocol->addItem(tr("SOCKS Version 5"), int(ProxyType::Socks5));
    m_protocol->addItem(tr("HTTPS"), int(ProxyType::Https));
    if (httpSupport || server.type == ProxyType::Http)
        m_protocol->addItem(tr("HTTP"), int(ProxyType::Http));
    m_protocol->setCurrentIndex(qMax(0, m_protocol->findData(int(server.type))));
    sl->addWidget(new QLabel(tr("Address:")), 0, 0);
    sl->addWidget(m_address, 0, 1);
    sl->addWidget(new QLabel(tr("Port:")), 0, 2);
    sl->addWidget(m_port, 0, 3);
    sl->addWidget(new QLabel(tr("Protocol:")), 1, 0);
    sl->addWidget(m_protocol, 1, 1);
    layout->addWidget(serverBox);

    m_auth = new QGroupBox(tr("Authentication"));
    auto *al = new QGridLayout(m_auth);
    m_authEnable = new QCheckBox(tr("Enable"));
    m_authEnable->setChecked(server.authEnabled);
    auto *ntlm = new QCheckBox(tr("Use NTLM"));
    auto *kerberos = new QCheckBox(tr("Use Kerberos"));
    ntlm->setEnabled(false);
    kerberos->setEnabled(false);
    ntlm->setToolTip(tr("Not supported"));
    kerberos->setToolTip(tr("Not supported"));
    m_user = new QLineEdit(server.username);
    m_pass = new QLineEdit(server.password);
    m_pass->setEchoMode(QLineEdit::Password);
    auto *row = new QHBoxLayout;
    row->addWidget(m_authEnable);
    row->addWidget(ntlm);
    row->addWidget(kerberos);
    row->addStretch();
    al->addLayout(row, 0, 0, 1, 2);
    al->addWidget(new QLabel(tr("Username:")), 1, 0);
    al->addWidget(m_user, 1, 1);
    al->addWidget(new QLabel(tr("Password:")), 2, 0);
    al->addWidget(m_pass, 2, 1);
    layout->addWidget(m_auth);

    auto *optBox = new QGroupBox(tr("Options"));
    auto *ol = new QVBoxLayout(optBox);
    m_socks4a = new QCheckBox(tr("Use SOCKS 4A extension (remote hostname resolving feature)"));
    m_socks4a->setChecked(server.socks4a);
    m_noOptions = new QLabel;
    ol->addWidget(m_socks4a);
    ol->addWidget(m_noOptions);
    layout->addWidget(optBox);

    auto *buttons = new QHBoxLayout;
    auto *ok = new QPushButton(tr("OK"));
    ok->setDefault(true);
    auto *check = new QPushButton(tr("Check"));
    auto *advanced = new QPushButton(tr("Advanced..."));
    auto *cancel = new QPushButton(tr("Cancel"));
    buttons->addWidget(ok);
    buttons->addWidget(check);
    buttons->addWidget(advanced);
    buttons->addStretch();
    buttons->addWidget(cancel);
    layout->addLayout(buttons);

    connect(ok, &QPushButton::clicked, this, &ProxyServerDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(check, &QPushButton::clicked, this, &ProxyServerDialog::onCheck);
    connect(advanced, &QPushButton::clicked, this, &ProxyServerDialog::onAdvanced);
    connect(m_protocol, &QComboBox::currentIndexChanged, this, &ProxyServerDialog::updateOptions);
    connect(m_authEnable, &QCheckBox::toggled, this, &ProxyServerDialog::updateOptions);
    updateOptions();
    setFixedWidth(sizeHint().width() + 60);
}

void ProxyServerDialog::updateOptions()
{
    const auto type = ProxyType(m_protocol->currentData().toInt());
    const bool socks4 = type == ProxyType::Socks4;
    // SOCKS4 has only a user id, no password.
    m_authEnable->setEnabled(true);
    m_user->setEnabled(m_authEnable->isChecked());
    m_pass->setEnabled(m_authEnable->isChecked() && !socks4);
    m_socks4a->setVisible(socks4);
    m_noOptions->setVisible(!socks4);
    if (type == ProxyType::Socks5)
        m_noOptions->setText(tr("No options for SOCKS5 are available."));
    else if (type == ProxyType::Https)
        m_noOptions->setText(tr("The proxy must support HTTP tunneling (CONNECT requests)."));
    else
        m_noOptions->setText(tr("HTTP proxy servers can be used for HTTP connections only."));
    if (m_port->value() == 1080 && (type == ProxyType::Https || type == ProxyType::Http) && m_server.address.isEmpty())
        m_port->setValue(8080);
    else if (m_port->value() == 8080 && (type == ProxyType::Socks4 || type == ProxyType::Socks5) && m_server.address.isEmpty())
        m_port->setValue(1080);
}

ProxyServer ProxyServerDialog::server() const
{
    ProxyServer s = m_server;
    s.address = m_address->text().trimmed();
    s.port = quint16(m_port->value());
    s.type = ProxyType(m_protocol->currentData().toInt());
    s.authEnabled = m_authEnable->isChecked();
    s.username = m_user->text();
    s.password = m_pass->text();
    s.socks4a = m_socks4a->isChecked();
    return s;
}

void ProxyServerDialog::accept()
{
    if (m_address->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Proxy Server"), tr("Please enter the proxy server address."));
        m_address->setFocus();
        return;
    }
    QDialog::accept();
}

void ProxyServerDialog::onCheck()
{
    if (m_address->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Proxy Server"), tr("Please enter the proxy server address."));
        return;
    }
    ProxyCheckerDialog dlg({server()}, this);
    dlg.startTest();
    dlg.exec();
}

void ProxyServerDialog::onAdvanced()
{
    ProxyAdvancedDialog dlg(m_server, this);
    if (dlg.exec() == QDialog::Accepted)
        dlg.apply(&m_server);
}

ProxyAdvancedDialog::ProxyAdvancedDialog(const ProxyServer &server, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Advanced Proxy Server Settings"));
    auto *layout = new QVBoxLayout(this);
    auto *appearance = new QGroupBox(tr("Appearance"));
    auto *al = new QGridLayout(appearance);
    m_label = new QLineEdit(server.label);
    auto *hint = new QLabel(tr("(\"address:port\" by default)"));
    hint->setEnabled(false);
    al->addWidget(new QLabel(tr("Custom Label:")), 0, 0);
    al->addWidget(m_label, 0, 1);
    al->addWidget(hint, 1, 1);
    layout->addWidget(appearance);

    auto *other = new QGroupBox(tr("Other"));
    auto *ol = new QVBoxLayout(other);
    m_useHostname = new QCheckBox(tr("Use target hostname in proxy request if available"));
    m_useHostname->setChecked(server.useHostname);
    ol->addWidget(m_useHostname);
    layout->addWidget(other);

    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(bb);
}

void ProxyAdvancedDialog::apply(ProxyServer *server) const
{
    server->label = m_label->text().trimmed();
    server->useHostname = m_useHostname->isChecked();
}
