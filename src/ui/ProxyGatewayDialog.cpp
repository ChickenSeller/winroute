#include "ProxyGatewayDialog.h"

#include <QCheckBox>
#include <QComboBox>
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

ProxyGatewayDialog::ProxyGatewayDialog(const ProxyGateway::Config &c, const Profile &profile, QWidget *parent)
    : QDialog(parent)
{
    constexpr int kTypeRole = Qt::UserRole + 1;   // the exit combo stores id + type
    setWindowTitle(tr("Proxy Gateway"));
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
    m_modeProxy = new QRadioButton(tr("Send every connection through one proxy or chain"));
    ml->addWidget(m_modeRules);
    // Inbound connections come from other machines, so there is no local process
    // to attribute: rules keyed by application never match and are skipped.
    auto *rulesHint = new QLabel(tr("Rules that match by application are skipped — a remote connection has no local process."));
    rulesHint->setWordWrap(true);
    rulesHint->setIndent(22);
    rulesHint->setStyleSheet("color:#d33;");
    ml->addWidget(rulesHint);
    ml->addWidget(m_modeDirect);

    // The fixed exit: one proxy or chain of the current profile.
    m_exit = new QComboBox;
    for (const ProxyServer &p : profile.proxies) {
        m_exit->addItem(profile.actionDisplayName({ActionType::Proxy, p.id}), p.id);
        m_exit->setItemData(m_exit->count() - 1, int(ActionType::Proxy), kTypeRole);
    }
    for (const ProxyChain &ch : profile.chains) {
        m_exit->addItem(profile.actionDisplayName({ActionType::Chain, ch.id}), ch.id);
        m_exit->setItemData(m_exit->count() - 1, int(ActionType::Chain), kTypeRole);
    }
    for (int i = 0; i < m_exit->count(); ++i) {   // preselect the saved exit
        if (m_exit->itemData(i).toInt() == c.exit.id
            && m_exit->itemData(i, kTypeRole).toInt() == int(c.exit.type)) {
            m_exit->setCurrentIndex(i);
            break;
        }
    }
    m_modeProxy->setEnabled(m_exit->count() > 0);   // no proxies/chains → nothing to send through
    auto *exitRow = new QHBoxLayout;
    exitRow->addWidget(m_modeProxy);
    exitRow->addWidget(m_exit, 1);
    ml->addLayout(exitRow);

    QRadioButton *initial = m_modeRules;
    if (c.mode == ProxyGateway::Mode::Direct)
        initial = m_modeDirect;
    else if (c.mode == ProxyGateway::Mode::Proxy && m_modeProxy->isEnabled())
        initial = m_modeProxy;
    initial->setChecked(true);

    auto *missing = new QHBoxLayout;
    missing->addSpacing(20);
    missing->addWidget(new QLabel(tr("If the selected proxy cannot connect:")));
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
    connect(m_modeProxy, &QRadioButton::toggled, this, &ProxyGatewayDialog::update);
    connect(m_httpEnabled, &QCheckBox::toggled, this, &ProxyGatewayDialog::update);
    connect(m_socksEnabled, &QCheckBox::toggled, this, &ProxyGatewayDialog::update);
    update();
}

void ProxyGatewayDialog::update()
{
    m_httpPort->setEnabled(m_httpEnabled->isChecked());
    m_socksPort->setEnabled(m_socksEnabled->isChecked());
    m_exit->setEnabled(m_modeProxy->isChecked());
    // The missing-proxy fallback matters whenever a proxy might be used.
    const bool usesProxy = m_modeRules->isChecked() || m_modeProxy->isChecked();
    m_missingBlock->setEnabled(usesProxy);
    m_missingDirect->setEnabled(usesProxy);
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
    c.mode = m_modeDirect->isChecked()  ? ProxyGateway::Mode::Direct
             : m_modeProxy->isChecked() ? ProxyGateway::Mode::Proxy
                                        : ProxyGateway::Mode::Rules;
    if (m_exit->currentIndex() >= 0) {
        c.exit.id = m_exit->currentData().toInt();
        c.exit.type = ActionType(m_exit->currentData(Qt::UserRole + 1).toInt());
    }
    c.onMissing = m_missingDirect->isChecked() ? ProxyGateway::OnMissing::Direct : ProxyGateway::OnMissing::Block;
    c.username = m_user->text();
    c.password = m_pass->text();
    return c;
}
