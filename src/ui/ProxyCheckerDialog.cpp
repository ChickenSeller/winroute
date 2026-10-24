#include "ProxyCheckerDialog.h"
#include "core/DnsClient.h"
#include "core/ProxyConnector.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTcpSocket>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

static const char *kOk = "<span style='color:#1a8a2a;font-weight:bold'>OK</span>";
static const char *kFailed = "<span style='color:#c0201a;font-weight:bold'>FAILED</span>";

ProxyCheckerDialog::ProxyCheckerDialog(const QList<ProxyServer> &proxies, QWidget *parent)
    : QDialog(parent)
{
    m_choices << qMakePair(proxies.size() == 1 ? proxies[0].displayName() : tr("Chain"), proxies);
    build();
}

ProxyCheckerDialog::ProxyCheckerDialog(const QList<QPair<QString, QList<ProxyServer>>> &choices, QWidget *parent)
    : QDialog(parent), m_choices(choices)
{
    build();
}

void ProxyCheckerDialog::build()
{
    setWindowTitle(tr("Proxy Checker"));
    resize(620, 460);
    m_dns = new DnsClient(this);
    m_dns->bind(0);

    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;
    m_choice = new QComboBox;
    for (const auto &c : m_choices)
        m_choice->addItem(c.first);
    form->addRow(tr("Proxy server:"), m_choice);
    auto *target = new QHBoxLayout;
    m_host = new QLineEdit("example.com");
    m_port = new QSpinBox;
    m_port->setRange(1, 65535);
    m_port->setValue(443);
    target->addWidget(m_host, 1);
    target->addWidget(new QLabel(tr("Port:")));
    target->addWidget(m_port);
    form->addRow(tr("Target host:"), target);
    layout->addLayout(form);

    m_output = new QTextEdit;
    m_output->setReadOnly(true);
    m_output->setFont(QFont("Consolas", 9));
    layout->addWidget(m_output, 1);

    auto *buttons = new QHBoxLayout;
    m_start = new QPushButton(tr("Start Testing"));
    auto *close = new QPushButton(tr("Close"));
    buttons->addWidget(m_start);
    buttons->addStretch();
    buttons->addWidget(close);
    layout->addLayout(buttons);
    connect(m_start, &QPushButton::clicked, this, &ProxyCheckerDialog::startTest);
    connect(close, &QPushButton::clicked, this, &QDialog::reject);
}

void ProxyCheckerDialog::out(const QString &html)
{
    m_output->append(html);
}

void ProxyCheckerDialog::startTest()
{
    if (m_running || m_choices.isEmpty())
        return;
    m_running = true;
    m_start->setEnabled(false);
    m_output->clear();
    m_hops = m_choices[qMax(0, m_choice->currentIndex())].second;
    out(tr("<b>Testing Started...</b>"));
    for (int i = 0; i < m_hops.size(); ++i) {
        const ProxyServer &p = m_hops[i];
        out(tr("Proxy Server%1<br>&nbsp;&nbsp;Address: %2:%3<br>&nbsp;&nbsp;Protocol: %4<br>&nbsp;&nbsp;Authentication: %5")
                .arg(m_hops.size() > 1 ? QString(" #%1").arg(i + 1) : QString())
                .arg(p.address.toHtmlEscaped())
                .arg(p.port)
                .arg(proxyTypeToString(p.type))
                .arg(p.authEnabled ? tr("YES") : tr("NO")));
    }
    out("");
    step1();
}

// Step 1: plain TCP connection to the first proxy.
void ProxyCheckerDialog::step1()
{
    out(tr("<b>Connection Test</b>"));
    out(tr("Connecting to %1:%2 ...").arg(m_hops[0].address.toHtmlEscaped()).arg(m_hops[0].port));
    m_socket = new QTcpSocket(this);
    m_timer.start();
    m_connector = new ProxyConnector(m_socket, {}, Endpoint{QHostAddress(m_hops[0].address).isNull() ? m_hops[0].address : QString(),
                                                            QHostAddress(m_hops[0].address), m_hops[0].port},
                                     m_dns, 10000, this);
    connect(m_connector, &ProxyConnector::connected, this, [this] {
        out(tr("Connection established %1 (%2 ms)").arg(kOk).arg(m_timer.elapsed()));
        m_connector->deleteLater();
        m_socket->abort();
        m_socket->deleteLater();
        out("");
        step2();
    });
    connect(m_connector, &ProxyConnector::failed, this, [this](const QString &err) {
        out(QString("%1 : %2").arg(kFailed, err.toHtmlEscaped()));
        finish(false);
    });
    m_connector->start();
}

// Step 2: tunnel to the target through the proxy (or chain) and send an HTTP request.
void ProxyCheckerDialog::step2()
{
    const QString host = m_host->text().trimmed();
    const quint16 port = quint16(m_port->value());
    out(tr("<b>Connection through the Proxy Server Test</b>"));
    out(tr("Connecting to %1:%2 through the proxy ...").arg(host.toHtmlEscaped()).arg(port));
    m_socket = new QTcpSocket(this);
    m_timer.start();
    Endpoint target;
    target.host = host;
    target.ip = QHostAddress(host);
    target.port = port;
    m_connector = new ProxyConnector(m_socket, m_hops, target, m_dns, 15000, this);
    connect(m_connector, &ProxyConnector::progress, this, [this](const QString &msg) { out(msg.toHtmlEscaped()); });
    connect(m_connector, &ProxyConnector::failed, this, [this](const QString &err) {
        out(QString("%1 : %2").arg(kFailed, err.toHtmlEscaped()));
        finish(false);
    });
    connect(m_connector, &ProxyConnector::connected, this, [this, host](const QByteArray &leftover) {
        out(tr("Connection to %1 established through the proxy %2 (%3 ms)").arg(host.toHtmlEscaped(), kOk).arg(m_timer.elapsed()));
        out("");
        out(tr("<b>HTTP Request Test</b>"));
        auto *buf = new QByteArray(leftover);
        connect(m_socket, &QTcpSocket::readyRead, this, [this, buf] {
            *buf += m_socket->readAll();
            const qsizetype eol = buf->indexOf("\r\n");
            if (eol < 0)
                return;
            out(tr("Response: %1 %2").arg(QString::fromLatin1(buf->left(eol)).toHtmlEscaped(), kOk));
            delete buf;
            m_socket->disconnect(this);
            m_socket->abort();
            finish(true);
        });
        connect(m_socket, &QTcpSocket::disconnected, this, [this] {
            out(tr("%1 : connection closed without a response").arg(kFailed));
            finish(false);
        });
        m_socket->write(QString("GET / HTTP/1.1\r\nHost: %1\r\nUser-Agent: WinRoute ProxyChecker\r\nConnection: close\r\n\r\n")
                            .arg(host)
                            .toLatin1());
        QTimer::singleShot(10000, this, [this] {
            if (m_running) {
                out(tr("%1 : no response within 10 seconds").arg(kFailed));
                finish(false);
            }
        });
    });
    m_connector->start();
}

void ProxyCheckerDialog::finish(bool ok)
{
    if (!m_running)
        return;
    m_running = false;
    out("");
    out(ok ? tr("<b>Testing Finished.</b> Proxy server is working correctly!")
           : tr("<b>Testing Finished.</b> <span style='color:#c0201a'>The proxy server test failed.</span>"));
    m_start->setEnabled(true);
    if (m_socket)
        m_socket->abort();
}
