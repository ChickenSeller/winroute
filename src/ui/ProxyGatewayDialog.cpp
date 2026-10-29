#include "ProxyGatewayDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

ProxyGatewayDialog::ProxyGatewayDialog(const ProxyGateway::Config &c, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Proxy Service"));
    resize(560, 420);
    auto *layout = new QVBoxLayout(this);

    m_enabled = new QCheckBox(tr("Run a proxy server so other machines can reach the network through this one"));
    m_enabled->setChecked(c.enabled);
    layout->addWidget(m_enabled);

    auto *listenBox = new QGroupBox(tr("Listen"));
    auto *ll = new QGridLayout(listenBox);
    m_listen = new QLineEdit(c.listenAddress.isNull() ? QString() : c.listenAddress.toString());
    m_listen->setPlaceholderText(tr("0.0.0.0 (all network interfaces)"));
    ll->addWidget(new QLabel(tr("Address:")), 0, 0);
    ll->addWidget(m_listen, 0, 1, 1, 2);

    // One row per protocol: each can be enabled on its own port.
    m_httpEnabled = new QCheckBox(tr("HTTP / HTTPS"));
    m_httpEnabled->setChecked(c.httpEnabled);
    m_httpPort = new QSpinBox;
    m_httpPort->setRange(1, 65535);
    m_httpPort->setValue(c.httpPort ? c.httpPort : 8080);
    ll->addWidget(m_httpEnabled, 1, 0);
    ll->addWidget(new QLabel(tr("Port:")), 1, 1, Qt::AlignRight);
    ll->addWidget(m_httpPort, 1, 2);

    m_socksEnabled = new QCheckBox(tr("SOCKS5"));
    m_socksEnabled->setChecked(c.socksEnabled);
    m_socksPort = new QSpinBox;
    m_socksPort->setRange(1, 65535);
    m_socksPort->setValue(c.socksPort ? c.socksPort : 1080);
    ll->addWidget(m_socksEnabled, 2, 0);
    ll->addWidget(new QLabel(tr("Port:")), 2, 1, Qt::AlignRight);
    ll->addWidget(m_socksPort, 2, 2);
    layout->addWidget(listenBox);

    auto *modeBox = new QGroupBox(tr("Outgoing connections"));
    auto *ml = new QVBoxLayout(modeBox);
    m_modeRules = new QRadioButton(tr("Follow this machine's proxy rules"));
    m_modeDirect = new QRadioButton(tr("Always connect directly"));
    (c.mode == ProxyGateway::Mode::Direct ? m_modeDirect : m_modeRules)->setChecked(true);
    ml->addWidget(m_modeRules);
    ml->addWidget(m_modeDirect);
    auto *missing = new QHBoxLayout;
    missing->addSpacing(20);
    missing->addWidget(new QLabel(tr("If a rule's proxy is missing:")));
    m_missingBlock = new QRadioButton(tr("Block"));
    m_missingDirect = new QRadioButton(tr("Connect directly"));
    (c.onMissing == ProxyGateway::OnMissing::Direct ? m_missingDirect : m_missingBlock)->setChecked(true);
    missing->addWidget(m_missingBlock);
    missing->addWidget(m_missingDirect);
    missing->addStretch();
    ml->addLayout(missing);
    layout->addWidget(modeBox);

    auto *authBox = new QGroupBox(tr("Authentication"));
    auto *al = new QFormLayout(authBox);
    m_user = new QLineEdit(c.username);
    m_pass = new QLineEdit(c.password);
    m_pass->setEchoMode(QLineEdit::Password);
    al->addRow(tr("User name:"), m_user);
    al->addRow(tr("Password:"), m_pass);
    auto *hint = new QLabel(tr("Leave the user name empty to allow clients without authentication."));
    hint->setEnabled(false);
    hint->setWordWrap(true);
    al->addRow(hint);
    layout->addWidget(authBox);

    layout->addStretch();
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_modeRules, &QRadioButton::toggled, this, &ProxyGatewayDialog::update);
    connect(m_httpEnabled, &QCheckBox::toggled, this, &ProxyGatewayDialog::update);
    connect(m_socksEnabled, &QCheckBox::toggled, this, &ProxyGatewayDialog::update);
    update();
}

void ProxyGatewayDialog::update()
{
    m_httpPort->setEnabled(m_httpEnabled->isChecked());
    m_socksPort->setEnabled(m_socksEnabled->isChecked());
    // "If a rule's proxy is missing" only matters when following the rules.
    const bool rules = m_modeRules->isChecked();
    m_missingBlock->setEnabled(rules);
    m_missingDirect->setEnabled(rules);
}

ProxyGateway::Config ProxyGatewayDialog::config() const
{
    ProxyGateway::Config c;
    c.enabled = m_enabled->isChecked();
    const QString addr = m_listen->text().trimmed();
    c.listenAddress = addr.isEmpty() ? QHostAddress(QHostAddress::Any) : QHostAddress(addr);
    if (c.listenAddress.isNull())            // unparseable text: fall back to all interfaces
        c.listenAddress = QHostAddress(QHostAddress::Any);
    c.httpEnabled = m_httpEnabled->isChecked();
    c.httpPort = quint16(m_httpPort->value());
    c.socksEnabled = m_socksEnabled->isChecked();
    c.socksPort = quint16(m_socksPort->value());
    c.mode = m_modeDirect->isChecked() ? ProxyGateway::Mode::Direct : ProxyGateway::Mode::Rules;
    c.onMissing = m_missingDirect->isChecked() ? ProxyGateway::OnMissing::Direct : ProxyGateway::OnMissing::Block;
    c.username = m_user->text();
    c.password = m_pass->text();
    return c;
}
