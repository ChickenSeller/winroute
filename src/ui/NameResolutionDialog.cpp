#include "NameResolutionDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QVBoxLayout>

NameResolutionDialog::NameResolutionDialog(const ProfileOptions &o, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Name Resolution"));
    resize(620, 420);
    auto *layout = new QVBoxLayout(this);

    auto *dnsBox = new QGroupBox(tr("WinRoute DNS settings"));
    auto *dl = new QGridLayout(dnsBox);
    m_auto = new QCheckBox(tr("Detect DNS settings automatically"));
    m_auto->setChecked(o.dnsAutoDetect);
    m_viaProxy = new QCheckBox(tr("Resolve hostnames through proxy"));
    m_viaProxy->setChecked(o.dnsViaProxy);
    m_localFirst = new QCheckBox(tr("Try to resolve via local DNS service first\n(may cause significant delays if local DNS is unavailable!)"));
    m_localFirst->setChecked(o.dnsTryLocalFirst);
    m_localFirst->setVisible(false);   // not implemented by the fake DNS responder
    dl->addWidget(m_auto, 0, 0);
    dl->addWidget(m_viaProxy, 0, 1);
    dl->addWidget(m_localFirst, 1, 1);
    layout->addWidget(dnsBox);

    auto *noteBox = new QGroupBox(tr("Note"));
    auto *nl = new QVBoxLayout(noteBox);
    auto *note = new QLabel(tr("It is impossible to get real IP addresses through proxy.\n"
                            "WinRoute assigns \"fake\" IP addresses (198.18.*.*) that are only valid when it is running."));
    note->setWordWrap(true);
    nl->addWidget(note);
    layout->addWidget(noteBox);

    auto *listBox = new QGroupBox(tr("DNS name list"));
    auto *ll = new QVBoxLayout(listBox);
    auto *radios = new QHBoxLayout;
    m_exclude = new QRadioButton(tr("Do NOT resolve the following"));
    m_only = new QRadioButton(tr("Resolve ONLY the following"));
    (o.dnsOnlyFromList ? m_only : m_exclude)->setChecked(true);
    radios->addWidget(m_exclude);
    radios->addWidget(m_only);
    radios->addStretch();
    ll->addLayout(radios);
    m_list = new QPlainTextEdit(o.dnsExclusionList);
    ll->addWidget(m_list);
    auto *example = new QLabel(tr("Example:\nlocalhost; *.example.com; %ComputerName%; %SimpleHostnames%"));
    example->setEnabled(false);
    ll->addWidget(example);
    layout->addWidget(listBox, 1);

    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_auto, &QCheckBox::toggled, this, &NameResolutionDialog::update);
    connect(m_viaProxy, &QCheckBox::toggled, this, &NameResolutionDialog::update);
    update();
}

void NameResolutionDialog::update()
{
    // Like Proxifier: automatic mode decides by itself, so the manual switch is disabled.
    m_viaProxy->setEnabled(!m_auto->isChecked());
    const bool lists = m_viaProxy->isChecked() || m_auto->isChecked();
    m_exclude->setEnabled(lists);
    m_only->setEnabled(lists);
    m_list->setEnabled(lists);
}

void NameResolutionDialog::apply(ProfileOptions *o) const
{
    o->dnsAutoDetect = m_auto->isChecked();
    o->dnsViaProxy = m_viaProxy->isChecked();
    o->dnsTryLocalFirst = m_localFirst->isChecked();
    o->dnsOnlyFromList = m_only->isChecked();
    o->dnsExclusionList = m_list->toPlainText().simplified();
}
