#include "CaptureTab.h"
#include "CaptureModel.h"
#include "Icons.h"

#include "core/Capture.h"
#include "core/CertAuthority.h"
#include "core/DnsMessage.h"
#include "core/HttpParse.h"
#include "core/Utils.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFont>
#include <QFontDatabase>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QTableWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSaveFile>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QTableView>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

// True when the bytes are mostly printable text (so they are worth showing as
// text rather than a hex dump).
bool looksTextual(const QByteArray &b)
{
    if (b.isEmpty())
        return true;
    int printable = 0;
    const int n = qMin(b.size(), 4096);
    for (int i = 0; i < n; ++i) {
        const unsigned char c = b[i];
        if (c == '\t' || c == '\r' || c == '\n' || (c >= 0x20 && c < 0x7f) || c >= 0x80)
            ++printable;
    }
    return printable >= n * 9 / 10;
}

QString headersText(const HttpMessage &m)
{
    QString out;
    for (const HttpHeader &h : m.headers)
        out += h.name + ": " + h.value + "\n";
    return out;
}

QString bodyText(const HttpMessage &m)
{
    if (m.body.isEmpty())
        return QString();
    if (looksTextual(m.body))
        return "\n" + QString::fromUtf8(m.body) + "\n";
    return QCoreApplication::translate("CaptureTab", "\n[%1 bytes of binary body]\n").arg(m.body.size());
}

QString hexDump(const QByteArray &data)
{
    QString out;
    const int limit = qMin(data.size(), 256 * 1024);
    for (int off = 0; off < limit; off += 16) {
        QString hex, asc;
        for (int i = 0; i < 16; ++i) {
            if (off + i < limit) {
                const unsigned char c = data[off + i];
                hex += QString("%1 ").arg(c, 2, 16, QChar('0'));
                asc += (c >= 0x20 && c < 0x7f) ? QChar(c) : QChar('.');
            } else {
                hex += "   ";
            }
        }
        out += QString("%1  %2 %3\n").arg(off, 8, 16, QChar('0')).arg(hex, asc);
    }
    if (data.size() > limit)
        out += QCoreApplication::translate("CaptureTab", "... (%1 more bytes)\n").arg(data.size() - limit);
    return out;
}

QString dnsTypeText(quint16 t)
{
    switch (t) {
    case dns::TypeA: return QStringLiteral("A");
    case dns::TypeCNAME: return QStringLiteral("CNAME");
    case dns::TypeAAAA: return QStringLiteral("AAAA");
    case dns::TypeHTTPS: return QStringLiteral("HTTPS");
    case 2: return QStringLiteral("NS");
    case 6: return QStringLiteral("SOA");
    case 12: return QStringLiteral("PTR");
    case 15: return QStringLiteral("MX");
    case 16: return QStringLiteral("TXT");
    case 33: return QStringLiteral("SRV");
    case 64: return QStringLiteral("SVCB");
    default: return QStringLiteral("TYPE%1").arg(t);
    }
}

QString dnsRcodeText(int rcode)
{
    switch (rcode) {
    case 0: return QStringLiteral("NOERROR");
    case 1: return QStringLiteral("FORMERR");
    case 2: return QStringLiteral("SERVFAIL");
    case 3: return QStringLiteral("NXDOMAIN");
    case 4: return QStringLiteral("NOTIMP");
    case 5: return QStringLiteral("REFUSED");
    default: return QStringLiteral("RCODE%1").arg(rcode);
    }
}

// One DNS message as readable lines: the question, and for an answer its
// status and records. Falls back to a hex dump when it does not parse.
QString dnsText(const QByteArray &msg)
{
    dns::Question q;
    if (!dns::parseQuestion(msg, &q))
        return hexDump(msg);
    const bool response = q.flags & 0x8000;
    QString out = QStringLiteral("  #%1  %2 %3\n").arg(q.id).arg(q.name, dnsTypeText(q.qtype));
    if (!response)
        return out;
    QList<dns::Answer> answers;
    dns::Question q2;
    dns::parseAnswers(msg, &q2, &answers);
    out += QStringLiteral("  %1\n").arg(dnsRcodeText(q.flags & 0x000f));
    for (const dns::Answer &a : answers) {
        const QString value = !a.address.isNull() ? a.address.toString()
                              : !a.target.isEmpty() ? a.target : QString();
        out += QStringLiteral("    %1  %2  %3  TTL %4\n").arg(a.name, dnsTypeText(a.type), value).arg(a.ttl);
    }
    if (answers.isEmpty())
        out += QCoreApplication::translate("CaptureTab", "    (no records)\n");
    return out;
}

// Edits one capture rule (enabled + the three match fields) in a small dialog.
// Returns true when accepted.
bool editCaptureRule(QWidget *parent, CaptureRule *rule)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(QCoreApplication::translate("CaptureTab", "Capture Rule"));
    auto *lay = new QVBoxLayout(&dlg);

    auto *enabled = new QCheckBox(QCoreApplication::translate("CaptureTab", "Enabled"));
    enabled->setChecked(rule->enabled);
    lay->addWidget(enabled);

    auto *form = new QFormLayout;
    auto *apps = new QLineEdit(rule->applications);
    apps->setPlaceholderText(QStringLiteral("chrome.exe; firefox.exe"));
    auto *targets = new QLineEdit(rule->targets);
    targets->setPlaceholderText(QStringLiteral("*.example.com; 10.0.0.0-10.255.255.255"));
    auto *ports = new QLineEdit(rule->ports);
    ports->setPlaceholderText(QStringLiteral("80; 443; 8000-9000"));
    form->addRow(QCoreApplication::translate("CaptureTab", "Applications:"), apps);
    form->addRow(QCoreApplication::translate("CaptureTab", "Target hosts:"), targets);
    form->addRow(QCoreApplication::translate("CaptureTab", "Ports:"), ports);
    lay->addLayout(form);

    auto *note = new QLabel(QCoreApplication::translate(
        "CaptureTab", "The rule matches when every field you fill in matches; leave a field blank to "
                      "ignore it. Uses the same syntax as rules."));
    note->setWordWrap(true);
    note->setEnabled(false);
    lay->addWidget(note);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    lay->addWidget(buttons);
    apps->setMinimumWidth(320);

    if (dlg.exec() != QDialog::Accepted)
        return false;
    rule->enabled = enabled->isChecked();
    rule->applications = apps->text().trimmed();
    rule->targets = targets->text().trimmed();
    rule->ports = ports->text().trimmed();
    return true;
}

// Asks for the subject of a new root certificate, pre-filled with the defaults.
// Returns true when accepted.
bool promptCaInfo(QWidget *parent, CertAuthority::CaInfo *info)
{
    QDialog dlg(parent);
    dlg.setWindowTitle(QCoreApplication::translate("CaptureTab", "New Root Certificate"));
    auto *lay = new QVBoxLayout(&dlg);

    auto *form = new QFormLayout;
    auto *cn = new QLineEdit(info->commonName);
    cn->setMinimumWidth(300);
    auto *org = new QLineEdit(info->organization);
    auto *years = new QSpinBox;
    years->setRange(1, 30);
    years->setValue(qMax(1, info->validityDays / 365));
    years->setSuffix(QCoreApplication::translate("CaptureTab", " years"));
    form->addRow(QCoreApplication::translate("CaptureTab", "Common name:"), cn);
    form->addRow(QCoreApplication::translate("CaptureTab", "Organization:"), org);
    form->addRow(QCoreApplication::translate("CaptureTab", "Valid for:"), years);
    lay->addLayout(form);

    auto *note = new QLabel(QCoreApplication::translate(
        "CaptureTab", "This name identifies WinRoute's certificate in your browser and the Windows "
                      "certificate store."));
    note->setWordWrap(true);
    note->setEnabled(false);
    lay->addWidget(note);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    lay->addWidget(buttons);

    if (dlg.exec() != QDialog::Accepted)
        return false;
    const QString name = cn->text().trimmed();
    info->commonName = name.isEmpty() ? CertAuthority::defaultInfo().commonName : name;
    info->organization = org->text().trimmed();
    info->validityDays = years->value() * 365;
    return true;
}

} // namespace

CaptureTab::CaptureTab(Capture *capture, CertAuthority *ca, QWidget *parent)
    : QWidget(parent), m_capture(capture), m_ca(ca)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);

    // Toolbar row.
    auto *bar = new QHBoxLayout;
    m_toggle = new QToolButton;
    m_toggle->setCheckable(true);
    m_toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(m_toggle, &QToolButton::toggled, this, &CaptureTab::onToggled);
    auto *clearBtn = new QPushButton(tr("Clear"));
    connect(clearBtn, &QPushButton::clicked, this, &CaptureTab::onClear);
    auto *exportBtn = new QPushButton(tr("Export..."));
    connect(exportBtn, &QPushButton::clicked, this, &CaptureTab::onExport);
    auto *filterBtn = new QPushButton(tr("Filter..."));
    connect(filterBtn, &QPushButton::clicked, this, &CaptureTab::onFilter);

    // A thin toolbar separator: a 1px line over the middle of the row, with
    // the same space on both sides.
    const auto addSeparator = [](QHBoxLayout *layout) {
        auto *line = new QFrame;
        line->setFixedSize(1, 18);
        line->setStyleSheet(QStringLiteral("background-color: palette(mid);"));
        layout->addSpacing(8);
        layout->addWidget(line, 0, Qt::AlignVCenter);
        layout->addSpacing(8);
    };

    // Whose traffic is captured; remembered, both on by default.
    auto *captureLabel = new QLabel(tr("Capture:"));
    m_local = new QCheckBox(tr("This computer"));
    m_local->setToolTip(tr("Capture the connections of this computer's programs"));
    m_gateway = new QCheckBox(tr("Proxy gateway"));
    m_gateway->setToolTip(tr("Capture the connections other devices make through the proxy gateway"));
    const struct { QCheckBox *box; Capture::Source source; const char *key; } sources[] = {
        {m_local, Capture::Source::Local, "capture/local"},
        {m_gateway, Capture::Source::Gateway, "capture/gateway"},
    };
    for (const auto &src : sources) {
        const bool srcOn = util::appSettings().value(src.key, true).toBool();
        src.box->setChecked(srcOn);
        m_capture->setSourceEnabled(src.source, srcOn);
        const Capture::Source source = src.source;
        const QString key = QString::fromLatin1(src.key);
        connect(src.box, &QCheckBox::toggled, this, [this, source, key](bool checked) {
            m_capture->setSourceEnabled(source, checked);
            util::appSettings().setValue(key, checked);
        });
    }

    // HTTPS decryption, for each source apart.
    auto *decryptLabel = new QLabel(tr("Decrypt HTTPS:"));
    decryptLabel->setToolTip(tr("Decrypt captured HTTPS (port 443) connections with WinRoute's root certificate. "
                                "Capture must be started as well."));
    m_decryptLocal = new QCheckBox(tr("This computer"));
    m_decryptLocal->setToolTip(tr("Decrypt the HTTPS connections of this computer's programs. "
                                  "This computer must trust WinRoute's root certificate."));
    m_decryptGateway = new QCheckBox(tr("Proxy gateway"));
    m_decryptGateway->setToolTip(tr("Decrypt the HTTPS connections other devices make through the proxy gateway. "
                                    "Those devices must trust WinRoute's root certificate."));
    connect(m_decryptLocal, &QCheckBox::toggled, this,
            [this](bool on) { onDecryptToggled(Capture::Source::Local, m_decryptLocal, on); });
    connect(m_decryptGateway, &QCheckBox::toggled, this,
            [this](bool on) { onDecryptToggled(Capture::Source::Gateway, m_decryptGateway, on); });
    auto *certBtn = new QPushButton(icons::certificate(), tr("Root Certificate..."));
    connect(certBtn, &QPushButton::clicked, this, &CaptureTab::onCertificate);

    bar->addWidget(m_toggle);
    bar->addWidget(clearBtn);
    bar->addWidget(exportBtn);
    bar->addWidget(filterBtn);
    addSeparator(bar);
    bar->addWidget(captureLabel);
    bar->addWidget(m_local);
    bar->addWidget(m_gateway);
    addSeparator(bar);
    bar->addWidget(decryptLabel);
    bar->addWidget(m_decryptLocal);
    bar->addWidget(m_decryptGateway);
    bar->addSpacing(8);
    bar->addWidget(certBtn);
    bar->addStretch();
    root->addLayout(bar);

    // List + detail.
    auto *split = new QSplitter(Qt::Horizontal);
    m_model = new CaptureModel(m_capture, this);
    m_view = new QTableView;
    m_view->setModel(m_model);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_view->verticalHeader()->setVisible(false);
    m_view->horizontalHeader()->setStretchLastSection(true);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setColumnWidth(CaptureModel::ColApp, 160);
    m_view->setColumnWidth(CaptureModel::ColTarget, 240);
    m_view->setColumnWidth(CaptureModel::ColProto, 80);
    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CaptureTab::onSelectionChanged);
    split->addWidget(m_view);

    m_detail = new QPlainTextEdit;
    m_detail->setReadOnly(true);
    m_detail->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_detail->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    split->addWidget(m_detail);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 2);
    root->addWidget(split, 1);

    // Restore the persisted toggle before the engine starts catching traffic.
    // Done with signals blocked so the one-time warning never fires at startup.
    const bool on = util::appSettings().value("capture/enabled", false).toBool();
    m_toggle->blockSignals(true);
    m_toggle->setChecked(on);
    m_toggle->blockSignals(false);
    m_capture->setEnabled(on);
    m_toggle->setIcon(on ? icons::pause() : icons::play());
    m_toggle->setText(on ? tr("Pause") : tr("Start"));

    // "capture/decrypt" is the setting from before there were two sources,
    // when only this computer's connections could be decrypted.
    const struct { QCheckBox *box; Capture::Source source; const char *key; } decrypts[] = {
        {m_decryptLocal, Capture::Source::Local, "capture/decrypt"},
        {m_decryptGateway, Capture::Source::Gateway, "capture/decryptGateway"},
    };
    for (const auto &d : decrypts) {
        const bool decrypt = util::appSettings().value(d.key, false).toBool();
        d.box->blockSignals(true);
        d.box->setChecked(decrypt);
        d.box->blockSignals(false);
        m_capture->setDecrypt(d.source, decrypt);
    }
    loadFilter();

    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &CaptureTab::refresh);
    m_timer->start();
    connect(m_capture, &Capture::changed, this, &CaptureTab::refresh);
    refresh();
}

void CaptureTab::refresh()
{
    m_model->refresh();
    // Keep the selection pinned to the same connection across refreshes.
    if (m_selectedId) {
        const int row = m_model->rowOfId(m_selectedId);
        if (row >= 0 && !m_view->selectionModel()->isRowSelected(row, {}))
            m_view->selectRow(row);
    }
}

void CaptureTab::onSelectionChanged()
{
    const QModelIndexList sel = m_view->selectionModel()->selectedRows();
    m_selectedId = sel.isEmpty() ? 0 : m_model->idAt(sel.first().row());
    renderDetail(m_selectedId);
}

void CaptureTab::renderDetail(quint64 id)
{
    if (!id || !m_capture->hasEntry(id)) {
        m_detail->clear();
        return;
    }
    const CaptureEntry e = m_capture->entry(id);
    QString out;
    out += tr("%1  %2  (%3 sent, %4 received)%5\n")
               .arg(e.meta.appName, e.meta.target, util::formatBytes(e.up), util::formatBytes(e.down),
                    e.decrypted ? tr("  [decrypted]") : QString());
    out += QString(60, '-') + "\n\n";

    // Datagram flows: one block per datagram, DNS decoded.
    if (e.meta.udp) {
        if (e.synthesized)
            out += tr("WinRoute answered with a placeholder address (resolve host names through the proxy): "
                      "the proxy looks up the real name when the connection is made.\n\n");
        for (const CaptureSegment &seg : e.segs) {
            const QString time = QDateTime::fromMSecsSinceEpoch(seg.tsMs).toString("HH:mm:ss.zzz");
            const bool up = seg.dir == CaptureDir::Up;
            if (e.proto == QLatin1String("DNS")) {
                out += (up ? tr("--> query  %1\n") : tr("<-- answer  %1\n")).arg(time);
                out += dnsText(seg.data);
            } else {
                out += (up ? tr("--> %1 bytes  %2\n") : tr("<-- %1 bytes  %2\n")).arg(seg.data.size()).arg(time);
                out += hexDump(seg.data);
            }
            out += "\n";
        }
        if (e.truncated)
            out += tr("(later datagrams were not kept: the per-connection limit was reached)\n");
        m_detail->setPlainText(out);
        return;
    }

    const QList<HttpExchange> xs = parseHttp(e);
    if (!xs.isEmpty()) {
        for (const HttpExchange &ex : xs) {
            if (ex.request.valid) {
                out += "> " + ex.request.method + " " + ex.request.uri + " " + ex.request.version + "\n";
                out += headersText(ex.request);
                out += bodyText(ex.request);
                out += "\n";
            }
            if (ex.response.valid) {
                out += "< " + ex.response.version + " " + QString::number(ex.response.statusCode) + " "
                       + ex.response.reason + "\n";
                out += headersText(ex.response);
                out += bodyText(ex.response);
            }
            out += "\n" + QString(60, '-') + "\n\n";
        }
    } else {
        // Non-HTTP: a hex dump of each run, marked by direction.
        CaptureDir last = CaptureDir::Up;
        bool first = true;
        QByteArray run;
        auto flush = [&] {
            if (!run.isEmpty()) {
                out += (last == CaptureDir::Up ? tr("--> client to server\n") : tr("<-- server to client\n"));
                out += hexDump(run);
                out += "\n";
                run.clear();
            }
        };
        for (const CaptureSegment &seg : e.segs) {
            if (!first && seg.dir != last)
                flush();
            last = seg.dir;
            first = false;
            run.append(seg.data);
        }
        flush();
    }
    m_detail->setPlainText(out);
}

void CaptureTab::onToggled(bool on)
{
    if (on) {
        // Warn about captured plaintext once, the first time capture is started.
        QSettings s = util::appSettings();
        if (!s.value("capture/warned", false).toBool()) {
            QMessageBox::information(
                this, tr("Packet Capture"),
                tr("Capturing records the traffic going through WinRoute, and with HTTPS decryption on it "
                   "can include plaintext such as passwords. It all stays on this computer; take care when "
                   "exporting or sharing a capture."));
            s.setValue("capture/warned", true);
        }
    }
    m_capture->setEnabled(on);
    util::appSettings().setValue("capture/enabled", on);
    m_toggle->setIcon(on ? icons::pause() : icons::play());
    m_toggle->setText(on ? tr("Pause") : tr("Start"));
}

void CaptureTab::setGatewayRunning(bool running)
{
    if (m_gatewayRunning == int(running))
        return;
    m_gatewayRunning = running;
    if (m_gatewayTip.isEmpty()) {
        m_gatewayTip = m_gateway->toolTip();
        m_decryptGatewayTip = m_decryptGateway->toolTip();
    }
    const QString off = tr("The proxy gateway is not enabled");
    m_gateway->setEnabled(running);
    m_decryptGateway->setEnabled(running);
    m_gateway->setToolTip(running ? m_gatewayTip : off);
    m_decryptGateway->setToolTip(running ? m_decryptGatewayTip : off);
}

void CaptureTab::onDecryptToggled(Capture::Source source, QCheckBox *box, bool on)
{
    auto revert = [box] {
        box->blockSignals(true);
        box->setChecked(false);
        box->blockSignals(false);
    };
    const bool gateway = source == Capture::Source::Gateway;
    if (on && gateway && !m_ca->exists()) {
        // The devices using the gateway are the ones that must trust the root
        // certificate; installing it here does not help them.
        const auto btn = QMessageBox::question(
            this, tr("Decrypt HTTPS"),
            tr("To decrypt HTTPS through the proxy gateway, the devices using it must trust WinRoute's root "
               "certificate.\n\nCreate the root certificate now? You can then export it with "
               "\"Root Certificate...\" and install it on those devices."),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Yes);
        if (btn != QMessageBox::Yes) {
            revert();
            return;
        }
        CertAuthority::CaInfo info = CertAuthority::defaultInfo();
        if (!promptCaInfo(this, &info)) {
            revert();
            return;
        }
        QString err;
        if (!m_ca->createCa(info, &err)) {
            QMessageBox::warning(this, tr("Decrypt HTTPS"), tr("Could not prepare the root certificate:\n%1").arg(err));
            revert();
            return;
        }
    } else if (on && !gateway) {
        if (!m_ca->exists()) {
            // No certificate yet: on Yes, let the user fill in its details, then
            // create that certificate and install it (never a silent default).
            const auto btn = QMessageBox::question(
                this, tr("Decrypt HTTPS"),
                tr("To decrypt HTTPS, this computer must trust WinRoute's root certificate. "
                   "Without it, browsers will warn about every decrypted site.\n\n"
                   "Create and install the root certificate now?"),
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);
            if (btn == QMessageBox::Cancel) {
                revert();
                return;
            }
            if (btn == QMessageBox::Yes) {
                CertAuthority::CaInfo info = CertAuthority::defaultInfo();
                if (!promptCaInfo(this, &info)) {
                    revert();
                    return;
                }
                QString err;
                if (!m_ca->createCa(info, &err)) {
                    QMessageBox::warning(this, tr("Decrypt HTTPS"),
                                         tr("Could not prepare the root certificate:\n%1").arg(err));
                    revert();
                    return;
                }
                QString ierr;
                if (!m_ca->install(&ierr))
                    QMessageBox::warning(this, tr("Decrypt HTTPS"),
                                         tr("Could not install the root certificate:\n%1").arg(ierr));
            }
            // "No": the certificate is created on first use and left untrusted.
        } else if (!m_ca->isInstalled()) {
            const auto btn = QMessageBox::question(
                this, tr("Decrypt HTTPS"),
                tr("WinRoute's root certificate is not trusted by this computer, so browsers will warn "
                   "about every decrypted site.\n\nInstall it now?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if (btn == QMessageBox::Yes) {
                QString ierr;
                if (!m_ca->install(&ierr))
                    QMessageBox::warning(this, tr("Decrypt HTTPS"),
                                         tr("Could not install the root certificate:\n%1").arg(ierr));
            }
        }
    }
    m_capture->setDecrypt(source, on);
    util::appSettings().setValue(gateway ? "capture/decryptGateway" : "capture/decrypt", on);
}

void CaptureTab::onCertificate()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("WinRoute Root Certificate"));
    dlg.resize(520, 0);
    auto *lay = new QVBoxLayout(&dlg);

    auto *status = new QLabel;
    status->setWordWrap(true);
    status->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lay->addWidget(status);

    auto *note = new QLabel(tr(
        "WinRoute uses this certificate to sign the per-site certificates it presents when decrypting "
        "HTTPS. Installing it marks it trusted on this computer, which covers browsers that use the "
        "Windows certificate store (Chrome, Edge). Firefox has its own store: import the exported "
        "certificate there manually."));
    note->setWordWrap(true);
    note->setEnabled(false);
    lay->addWidget(note);

    auto *btnRow = new QHBoxLayout;
    auto *genBtn = new QPushButton(tr("Generate..."));
    auto *installBtn = new QPushButton(tr("Install"));
    auto *removeBtn = new QPushButton(tr("Remove"));
    auto *exportBtn = new QPushButton(tr("Export..."));
    btnRow->addWidget(genBtn);
    btnRow->addWidget(installBtn);
    btnRow->addWidget(removeBtn);
    btnRow->addWidget(exportBtn);
    btnRow->addStretch();
    lay->addLayout(btnRow);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    lay->addWidget(buttons);

    auto updateStatus = [&] {
        const CertAuthority::CertDetails d = m_ca->caDetails();
        const bool exists = d.valid;
        const bool installed = exists && m_ca->isInstalled();
        QString text = tr("Status: %1").arg(
            !exists ? tr("not created yet")
                    : installed ? tr("created and trusted by this computer")
                                : tr("created, but not trusted by this computer"));
        if (exists) {
            text += tr("\nCommon name: %1").arg(d.commonName);
            if (!d.organization.isEmpty())
                text += tr("\nOrganization: %1").arg(d.organization);
            if (d.notBefore.isValid() && d.notAfter.isValid())
                text += tr("\nValid: %1 to %2").arg(d.notBefore.toLocalTime().toString("yyyy-MM-dd"),
                                                    d.notAfter.toLocalTime().toString("yyyy-MM-dd"));
            if (!d.sha256.isEmpty())
                text += tr("\nSHA-256: %1").arg(d.sha256);
        }
        text += tr("\nLocation: %1").arg(QDir::toNativeSeparators(CertAuthority::caDir()));
        status->setText(text);
        genBtn->setText(exists ? tr("Regenerate...") : tr("Generate..."));
        installBtn->setEnabled(exists && !installed);
        removeBtn->setEnabled(installed);
        exportBtn->setEnabled(exists);
    };
    updateStatus();

    connect(genBtn, &QPushButton::clicked, &dlg, [&] {
        if (m_ca->exists()) {
            const auto r = QMessageBox::warning(
                &dlg, tr("Regenerate Root Certificate"),
                tr("This replaces the current root certificate. You will need to install the new one, and "
                   "anything that trusted the old certificate will stop trusting WinRoute until then.\n\nContinue?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (r != QMessageBox::Yes)
                return;
        }
        CertAuthority::CaInfo info = CertAuthority::defaultInfo();
        if (!promptCaInfo(&dlg, &info))
            return;
        QString err;
        if (!m_ca->createCa(info, &err))
            QMessageBox::warning(&dlg, tr("Root Certificate"), tr("Could not create the certificate:\n%1").arg(err));
        updateStatus();
    });
    connect(installBtn, &QPushButton::clicked, &dlg, [&] {
        QString err;
        if (!m_ca->install(&err))
            QMessageBox::warning(&dlg, tr("Root Certificate"), tr("Could not install the certificate:\n%1").arg(err));
        updateStatus();
    });
    connect(removeBtn, &QPushButton::clicked, &dlg, [&] {
        QString err;
        if (!m_ca->uninstall(&err))
            QMessageBox::warning(&dlg, tr("Root Certificate"), tr("Could not remove the certificate:\n%1").arg(err));
        updateStatus();
    });
    connect(exportBtn, &QPushButton::clicked, &dlg, [&] {
        const QString path = QFileDialog::getSaveFileName(&dlg, tr("Export Root Certificate"),
                                                          QStringLiteral("winroute-ca.crt"),
                                                          tr("Certificate (*.crt *.pem)"));
        if (path.isEmpty())
            return;
        QString err;
        if (!m_ca->exportCaCert(path, &err))
            QMessageBox::warning(&dlg, tr("Root Certificate"), tr("Could not export the certificate:\n%1").arg(err));
    });

    dlg.exec();
}

void CaptureTab::onClear()
{
    m_capture->clear();
    m_selectedId = 0;
    m_detail->clear();
    refresh();
}

void CaptureTab::onExport()
{
    if (m_capture->count() == 0) {
        QMessageBox::information(this, tr("Export Capture"), tr("There is nothing captured to export."));
        return;
    }

    const QModelIndexList sel = m_view->selectionModel()->selectedRows();

    // Options go in a small dialog of their own so the file chooser can stay the
    // native Windows "Save As" dialog.
    QDialog opt(this);
    opt.setWindowTitle(tr("Export Capture"));
    auto *lay = new QVBoxLayout(&opt);

    QCheckBox *selOnly = nullptr;
    if (!sel.isEmpty()) {
        // Export everything by default; a stray row selection should not quietly
        // shrink the export.
        selOnly = new QCheckBox(tr("Only the %1 selected connection(s) (otherwise all %2)")
                                    .arg(sel.size()).arg(m_capture->count()));
        lay->addWidget(selOnly);
    }

    auto *portLabel = new QLabel(tr("Decrypted HTTPS in Wireshark (PCAPNG):"));
    lay->addWidget(portLabel);
    auto *realPort = new QRadioButton(tr("Keep real ports (443); use Wireshark's \"Decode As\" to read it as HTTP"));
    auto *http80 = new QRadioButton(tr("Use port 80 so Wireshark shows HTTP automatically"));
    auto *pduMode = new QRadioButton(
        tr("Keep real ports (443) and show HTTP automatically (Exported PDU; no TCP-stream view)"));
    const QString mode = util::appSettings().value("capture/exportMode", QStringLiteral("real")).toString();
    http80->setChecked(mode == QLatin1String("http80"));
    pduMode->setChecked(mode == QLatin1String("pdu"));
    realPort->setChecked(!http80->isChecked() && !pduMode->isChecked());
    lay->addWidget(realPort);
    lay->addWidget(http80);
    lay->addWidget(pduMode);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &opt, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &opt, &QDialog::reject);
    lay->addWidget(buttons);
    if (opt.exec() != QDialog::Accepted)
        return;

    const bool keepRealPorts = realPort->isChecked();
    const bool exportedPdu = pduMode->isChecked();
    const bool selectedOnly = selOnly && selOnly->isChecked();
    util::appSettings().setValue("capture/exportMode",
                                 http80->isChecked() ? QStringLiteral("http80")
                                                     : exportedPdu ? QStringLiteral("pdu") : QStringLiteral("real"));

    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
    QString selectedFilter;
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export Capture"), QStringLiteral("winroute-capture-%1").arg(stamp),
        tr("PCAPNG, for Wireshark (*.pcapng);;HAR, HTTP exchanges (*.har)"), &selectedFilter);
    if (path.isEmpty())
        return;
    const bool har = selectedFilter.contains("HAR") || path.endsWith(".har", Qt::CaseInsensitive);

    // All connections, or only the selected ones when that was chosen.
    QList<CaptureEntry> entries;
    if (selectedOnly) {
        for (const QModelIndex &idx : sel)
            if (const quint64 id = m_model->idAt(idx.row()))
                entries.append(m_capture->entry(id));
    } else {
        for (const CaptureSummary &s : m_capture->summaries())
            entries.append(m_capture->entry(s.meta.id));
    }

    QByteArray data;
    if (har)
        data = Capture::toHar(entries);
    else if (exportedPdu)
        data = Capture::toExportedPdu(entries);
    else
        data = Capture::toPcapng(entries, keepRealPorts);

    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly) || f.write(data) != data.size() || !f.commit()) {
        QMessageBox::warning(this, tr("Export Capture"), tr("Could not write the file:\n%1").arg(f.errorString()));
        return;
    }
}

void CaptureTab::loadFilter()
{
    QSettings s = util::appSettings();
    CaptureFilter f;
    f.mode = CaptureFilter::Mode(s.value("capture/filterMode", int(CaptureFilter::All)).toInt());
    const int n = s.beginReadArray("capture/rules");
    for (int i = 0; i < n; ++i) {
        s.setArrayIndex(i);
        CaptureRule r;
        r.enabled = s.value("enabled", true).toBool();
        r.applications = s.value("apps").toString();
        r.targets = s.value("targets").toString();
        r.ports = s.value("ports").toString();
        f.rules.append(r);
    }
    s.endArray();
    m_capture->setFilter(f);
}

void CaptureTab::onFilter()
{
    CaptureFilter f = m_capture->filter();

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Capture Filter"));
    dlg.resize(560, 360);
    auto *lay = new QVBoxLayout(&dlg);

    auto *modeRow = new QHBoxLayout;
    modeRow->addWidget(new QLabel(tr("Capture:")));
    auto *modeBox = new QComboBox;
    modeBox->addItem(tr("Everything"), int(CaptureFilter::All));
    modeBox->addItem(tr("Only connections matching a rule (whitelist)"), int(CaptureFilter::Only));
    modeBox->addItem(tr("All except connections matching a rule (blacklist)"), int(CaptureFilter::Except));
    modeBox->setCurrentIndex(modeBox->findData(int(f.mode)));
    modeRow->addWidget(modeBox, 1);
    lay->addLayout(modeRow);

    // The rule table + its buttons.
    auto *table = new QTableWidget(0, 4);
    table->setHorizontalHeaderLabels({tr("On"), tr("Applications"), tr("Target hosts"), tr("Ports")});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setColumnWidth(0, 36);
    table->setColumnWidth(1, 170);
    table->setColumnWidth(2, 200);

    auto fill = [&] {
        table->setRowCount(f.rules.size());
        for (int i = 0; i < f.rules.size(); ++i) {
            const CaptureRule &r = f.rules[i];
            auto set = [&](int col, const QString &text) {
                auto *it = new QTableWidgetItem(text);
                it->setFlags(it->flags() & ~Qt::ItemIsEditable);
                table->setItem(i, col, it);
            };
            set(0, r.enabled ? QStringLiteral("✓") : QString());
            set(1, r.applications);
            set(2, r.targets);
            set(3, r.ports);
        }
    };
    fill();

    auto *tableRow = new QHBoxLayout;
    tableRow->addWidget(table, 1);
    auto *btnCol = new QVBoxLayout;
    auto *addBtn = new QPushButton(tr("Add..."));
    auto *editBtn = new QPushButton(tr("Edit..."));
    auto *removeBtn = new QPushButton(tr("Remove"));
    btnCol->addWidget(addBtn);
    btnCol->addWidget(editBtn);
    btnCol->addWidget(removeBtn);
    btnCol->addStretch();
    tableRow->addLayout(btnCol);
    lay->addLayout(tableRow);

    auto editRow = [&](int row) {
        if (row < 0 || row >= f.rules.size())
            return;
        CaptureRule r = f.rules[row];
        if (editCaptureRule(&dlg, &r)) {
            f.rules[row] = r;
            fill();
            table->selectRow(row);
        }
    };
    QObject::connect(addBtn, &QPushButton::clicked, &dlg, [&] {
        CaptureRule r;
        if (editCaptureRule(&dlg, &r)) {
            f.rules.append(r);
            fill();
            table->selectRow(f.rules.size() - 1);
        }
    });
    QObject::connect(editBtn, &QPushButton::clicked, &dlg, [&] { editRow(table->currentRow()); });
    QObject::connect(table, &QTableWidget::doubleClicked, &dlg,
                     [&](const QModelIndex &idx) { editRow(idx.row()); });
    QObject::connect(removeBtn, &QPushButton::clicked, &dlg, [&] {
        const int row = table->currentRow();
        if (row >= 0 && row < f.rules.size()) {
            f.rules.removeAt(row);
            fill();
        }
    });

    // The rule table is only relevant for the whitelist/blacklist modes.
    auto modeChanged = [&] {
        const bool on = modeBox->currentData().toInt() != int(CaptureFilter::All);
        table->setEnabled(on);
        addBtn->setEnabled(on);
        editBtn->setEnabled(on);
        removeBtn->setEnabled(on);
    };
    modeChanged();
    QObject::connect(modeBox, &QComboBox::currentIndexChanged, &dlg, [&] { modeChanged(); });

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    lay->addWidget(buttons);

    if (dlg.exec() != QDialog::Accepted)
        return;

    f.mode = CaptureFilter::Mode(modeBox->currentData().toInt());
    m_capture->setFilter(f);

    QSettings s = util::appSettings();
    s.setValue("capture/filterMode", int(f.mode));
    s.beginWriteArray("capture/rules", f.rules.size());
    for (int i = 0; i < f.rules.size(); ++i) {
        s.setArrayIndex(i);
        s.setValue("enabled", f.rules[i].enabled);
        s.setValue("apps", f.rules[i].applications);
        s.setValue("targets", f.rules[i].targets);
        s.setValue("ports", f.rules[i].ports);
    }
    s.endArray();
}
