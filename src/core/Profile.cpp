#include "Profile.h"

#include "Utils.h"

#include <QCoreApplication>
#include <QFile>
#include <QSaveFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <windows.h>
#include <dpapi.h>

QString proxyTypeToString(ProxyType t)
{
    switch (t) {
    case ProxyType::Socks5: return "SOCKS5";
    case ProxyType::Socks4: return "SOCKS4";
    case ProxyType::Https: return "HTTPS";
    case ProxyType::Http: return "HTTP";
    }
    return "SOCKS5";
}

ProxyType proxyTypeFromString(const QString &s)
{
    const QString u = s.toUpper();
    if (u == "SOCKS4") return ProxyType::Socks4;
    if (u == "HTTPS") return ProxyType::Https;
    if (u == "HTTP") return ProxyType::Http;
    return ProxyType::Socks5;
}

QString proxyTypeDisplayName(ProxyType t)
{
    switch (t) {
    case ProxyType::Socks5: return "SOCKS Version 5";
    case ProxyType::Socks4: return "SOCKS Version 4";
    case ProxyType::Https: return "HTTPS";
    case ProxyType::Http: return "HTTP";
    }
    return {};
}

QString ProxyServer::displayName() const
{
    if (!label.isEmpty())
        return label;
    return QString("%1:%2").arg(address).arg(port);
}

// ---- password encoding -----------------------------------------------------
// Proxifier's own "basic" key is not public, so passwords written by WinRoute
// carry a "WR1:" prefix. Foreign encrypted passwords are dropped on import.

static const QByteArray kStaticKey = "WinRoute-static-profile-key-v1";

static QString encodePassword(const QString &plain, PasswordEncryption mode)
{
    if (plain.isEmpty() || mode == PasswordEncryption::Disabled || mode == PasswordEncryption::Master)
        return plain;
    QByteArray data = plain.toUtf8();
    if (mode == PasswordEncryption::Basic) {
        for (int i = 0; i < data.size(); ++i)
            data[i] = char(data[i] ^ kStaticKey[i % kStaticKey.size()]);
        return "WR1:" + QString::fromLatin1(data.toBase64());
    }
    DATA_BLOB in{DWORD(data.size()), reinterpret_cast<BYTE *>(data.data())};
    DATA_BLOB out{};
    if (!CryptProtectData(&in, L"WinRoute", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out))
        return plain;
    QByteArray enc(reinterpret_cast<const char *>(out.pbData), int(out.cbData));
    LocalFree(out.pbData);
    return "WR1:" + QString::fromLatin1(enc.toBase64());
}

static QString decodePassword(const QString &stored, PasswordEncryption mode, bool *ok)
{
    *ok = true;
    if (stored.isEmpty() || mode == PasswordEncryption::Disabled)
        return stored;
    if (!stored.startsWith("WR1:")) {
        *ok = false;
        return {};
    }
    QByteArray data = QByteArray::fromBase64(stored.mid(4).toLatin1());
    if (mode == PasswordEncryption::Basic) {
        for (int i = 0; i < data.size(); ++i)
            data[i] = char(data[i] ^ kStaticKey[i % kStaticKey.size()]);
        return QString::fromUtf8(data);
    }
    DATA_BLOB in{DWORD(data.size()), reinterpret_cast<BYTE *>(data.data())};
    DATA_BLOB out{};
    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out)) {
        *ok = false;
        return {};
    }
    QString res = QString::fromUtf8(reinterpret_cast<const char *>(out.pbData), int(out.cbData));
    LocalFree(out.pbData);
    return res;
}

static QString encryptionToString(PasswordEncryption e)
{
    switch (e) {
    case PasswordEncryption::Disabled: return "disabled";
    case PasswordEncryption::Basic: return "basic";
    case PasswordEncryption::User: return "user";
    case PasswordEncryption::Master: return "master";
    }
    return "disabled";
}

static PasswordEncryption encryptionFromString(const QString &s)
{
    if (s == "basic") return PasswordEncryption::Basic;
    if (s == "user") return PasswordEncryption::User;
    if (s == "master") return PasswordEncryption::Master;
    return PasswordEncryption::Disabled;
}

static QString chainTypeToString(ChainType t)
{
    switch (t) {
    case ChainType::Simple: return "simple";
    case ChainType::Redundancy: return "redundancy";
    case ChainType::LoadBalancing: return "load_balancing";
    }
    return "simple";
}

static ChainType chainTypeFromString(const QString &s)
{
    if (s == "redundancy") return ChainType::Redundancy;
    if (s == "load_balancing") return ChainType::LoadBalancing;
    return ChainType::Simple;
}

// ---- Profile ---------------------------------------------------------------

Profile::Profile() = default;

Profile Profile::createDefault()
{
    Profile p;
    Rule localhost;
    localhost.name = "Localhost";
    localhost.targets = "localhost; 127.0.0.1; %ComputerName%";
    localhost.action.type = ActionType::Direct;
    Rule def;
    def.name = "Default";
    def.action.type = ActionType::Direct;
    p.rules << localhost << def;
    return p;
}

const ProxyServer *Profile::proxyById(int id) const
{
    for (const auto &p : proxies)
        if (p.id == id)
            return &p;
    return nullptr;
}

const ProxyChain *Profile::chainById(int id) const
{
    for (const auto &c : chains)
        if (c.id == id)
            return &c;
    return nullptr;
}

int Profile::nextId() const
{
    int id = 100;
    for (const auto &p : proxies)
        id = qMax(id, p.id + 1);
    for (const auto &c : chains)
        id = qMax(id, c.id + 1);
    return id;
}

QString Profile::actionDisplayName(const RuleAction &a) const
{
    switch (a.type) {
    case ActionType::Direct: return QCoreApplication::translate("Profile", "Direct");
    case ActionType::Block: return QCoreApplication::translate("Profile", "Block");
    case ActionType::Proxy:
        if (auto p = proxyById(a.id))
            return QCoreApplication::translate("Profile", "Proxy %1 %2").arg(proxyTypeToString(p->type), p->displayName());
        return QCoreApplication::translate("Profile", "Proxy <missing>");
    case ActionType::Chain:
        if (auto c = chainById(a.id))
            return QCoreApplication::translate("Profile", "Chain %1").arg(c->name);
        return QCoreApplication::translate("Profile", "Chain <missing>");
    }
    return {};
}

bool Profile::load(const QString &path, QString *error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error)
            *error = f.errorString();
        return false;
    }
    return fromXml(f.readAll(), error);
}

bool Profile::save(const QString &path, QString *error) const
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        if (error)
            *error = f.errorString();
        return false;
    }
    f.write(toXml());
    if (!f.commit()) {
        if (error)
            *error = f.errorString();
        return false;
    }
    return true;
}

static bool attrBool(const QXmlStreamReader &r, const char *name, bool def = false)
{
    const auto v = r.attributes().value(QLatin1String(name));
    if (v.isEmpty())
        return def;
    return v.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;
}

bool Profile::fromXml(const QByteArray &xml, QString *error)
{
    Profile p;
    QXmlStreamReader r(xml);
    if (!r.readNextStartElement() || r.name() != QLatin1String("ProxifierProfile")) {
        if (error)
            *error = QCoreApplication::translate("Profile", "Not a Proxifier/WinRoute profile");
        return false;
    }
    struct PendingPassword { int proxyIndex; QString stored; };
    QList<PendingPassword> pending;

    while (r.readNextStartElement()) {
        if (r.name() == QLatin1String("Options")) {
            while (r.readNextStartElement()) {
                const auto n = r.name();
                if (n == QLatin1String("Resolve")) {
                    while (r.readNextStartElement()) {
                        const auto rn = r.name();
                        if (rn == QLatin1String("AutoModeDetection")) {
                            p.options.dnsAutoDetect = attrBool(r, "enabled");
                            r.skipCurrentElement();
                        } else if (rn == QLatin1String("ViaProxy")) {
                            p.options.dnsViaProxy = attrBool(r, "enabled");
                            while (r.readNextStartElement()) {
                                if (r.name() == QLatin1String("TryLocalDnsFirst"))
                                    p.options.dnsTryLocalFirst = attrBool(r, "enabled");
                                r.skipCurrentElement();
                            }
                        } else if (rn == QLatin1String("BlockNonATypes")) {
                            p.options.dnsBlockNonATypes = attrBool(r, "enabled");
                            r.skipCurrentElement();
                        } else if (rn == QLatin1String("ExclusionList")) {
                            p.options.dnsOnlyFromList = attrBool(r, "OnlyFromListMode");
                            p.options.dnsExclusionList = r.readElementText();
                        } else if (rn == QLatin1String("DnsUdpMode")) {
                            p.options.dnsUdpMode = r.readElementText().toInt();
                        } else {
                            r.skipCurrentElement();
                        }
                    }
                } else if (n == QLatin1String("Encryption")) {
                    p.options.encryption = encryptionFromString(r.attributes().value("mode").toString());
                    r.skipCurrentElement();
                } else if (n == QLatin1String("ConnectionLoopDetection")) {
                    p.options.loopDetection = attrBool(r, "enabled", true);
                    p.options.loopDetectionResolve = attrBool(r, "resolve", true);
                    r.skipCurrentElement();
                } else if (n == QLatin1String("Udp")) {
                    p.options.udpMode = r.attributes().value("mode").toString();
                    r.skipCurrentElement();
                } else if (n == QLatin1String("LeakPreventionMode")) {
                    p.options.leakPrevention = attrBool(r, "enabled");
                    r.skipCurrentElement();
                } else if (n == QLatin1String("ProcessOtherUsers")) {
                    p.options.processOtherUsers = attrBool(r, "enabled");
                    r.skipCurrentElement();
                } else if (n == QLatin1String("ProcessServices")) {
                    p.options.processServices = attrBool(r, "enabled");
                    r.skipCurrentElement();
                } else if (n == QLatin1String("HandleDirectConnections")) {
                    p.options.handleDirectConnections = attrBool(r, "enabled");
                    r.skipCurrentElement();
                } else if (n == QLatin1String("HttpProxiesSupport")) {
                    p.options.httpProxiesSupport = attrBool(r, "enabled");
                    r.skipCurrentElement();
                } else if (n == QLatin1String("WinRouteLogDns")) {
                    // WinRoute extension; Proxifier ignores unknown elements.
                    p.options.logDns = attrBool(r, "enabled");
                    r.skipCurrentElement();
                } else {
                    r.skipCurrentElement();
                }
            }
        } else if (r.name() == QLatin1String("ProxyList")) {
            while (r.readNextStartElement()) {
                if (r.name() != QLatin1String("Proxy")) {
                    r.skipCurrentElement();
                    continue;
                }
                ProxyServer s;
                s.id = r.attributes().value("id").toInt();
                s.type = proxyTypeFromString(r.attributes().value("type").toString());
                while (r.readNextStartElement()) {
                    const auto n = r.name();
                    if (n == QLatin1String("Address")) {
                        s.address = r.readElementText().trimmed();
                    } else if (n == QLatin1String("Port")) {
                        s.port = quint16(r.readElementText().toUInt());
                    } else if (n == QLatin1String("Options")) {
                        s.options = r.readElementText().toInt();
                    } else if (n == QLatin1String("Label")) {
                        s.label = r.readElementText();
                    } else if (n == QLatin1String("Socks4A")) {
                        s.socks4a = attrBool(r, "enabled", true);
                        r.skipCurrentElement();
                    } else if (n == QLatin1String("UseHostname")) {
                        s.useHostname = attrBool(r, "enabled", true);
                        r.skipCurrentElement();
                    } else if (n == QLatin1String("Authentication")) {
                        s.authEnabled = attrBool(r, "enabled");
                        while (r.readNextStartElement()) {
                            if (r.name() == QLatin1String("Username"))
                                s.username = r.readElementText();
                            else if (r.name() == QLatin1String("Password"))
                                pending.append({int(p.proxies.size()), r.readElementText()});
                            else
                                r.skipCurrentElement();
                        }
                    } else {
                        r.skipCurrentElement();
                    }
                }
                p.proxies.append(s);
            }
        } else if (r.name() == QLatin1String("ChainList")) {
            while (r.readNextStartElement()) {
                if (r.name() != QLatin1String("Chain")) {
                    r.skipCurrentElement();
                    continue;
                }
                ProxyChain c;
                c.id = r.attributes().value("id").toInt();
                c.type = chainTypeFromString(r.attributes().value("type").toString());
                while (r.readNextStartElement()) {
                    const auto n = r.name();
                    if (n == QLatin1String("Name")) {
                        c.name = r.readElementText();
                    } else if (n == QLatin1String("Proxy")) {
                        ChainMember m;
                        m.enabled = attrBool(r, "enabled", true);
                        m.proxyId = r.readElementText().toInt();
                        c.members.append(m);
                    } else if (n == QLatin1String("RedundancyTimeout")) {
                        c.redundancyTimeout = r.readElementText().toInt();
                    } else if (n == QLatin1String("RedundancyRecheck")) {
                        c.redundancyRecheck = attrBool(r, "enabled", true);
                        c.redundancyRecheckSec = r.readElementText().toInt();
                    } else if (n == QLatin1String("RedundancyTryDirect")) {
                        c.redundancyDirectFallback = attrBool(r, "enabled");
                        r.skipCurrentElement();
                    } else if (n == QLatin1String("LoadBalancingSamePid")) {
                        c.lbSamePidSameProxy = attrBool(r, "enabled");
                        r.skipCurrentElement();
                    } else {
                        r.skipCurrentElement();
                    }
                }
                p.chains.append(c);
            }
        } else if (r.name() == QLatin1String("RuleList")) {
            while (r.readNextStartElement()) {
                if (r.name() != QLatin1String("Rule")) {
                    r.skipCurrentElement();
                    continue;
                }
                Rule rule;
                rule.enabled = attrBool(r, "enabled", true);
                while (r.readNextStartElement()) {
                    const auto n = r.name();
                    if (n == QLatin1String("Name")) {
                        rule.name = r.readElementText();
                    } else if (n == QLatin1String("Applications")) {
                        rule.applications = r.readElementText();
                    } else if (n == QLatin1String("Targets")) {
                        rule.targets = r.readElementText();
                    } else if (n == QLatin1String("Ports")) {
                        rule.ports = r.readElementText();
                    } else if (n == QLatin1String("Action")) {
                        const QString t = r.attributes().value("type").toString();
                        const QString val = r.readElementText();
                        if (t == "Proxy") {
                            rule.action.type = ActionType::Proxy;
                            rule.action.id = val.toInt();
                        } else if (t == "Chain") {
                            rule.action.type = ActionType::Chain;
                            rule.action.id = val.toInt();
                        } else if (t == "Block") {
                            rule.action.type = ActionType::Block;
                        } else {
                            rule.action.type = ActionType::Direct;
                        }
                    } else {
                        r.skipCurrentElement();
                    }
                }
                p.rules.append(rule);
            }
        } else {
            r.skipCurrentElement();
        }
    }
    if (r.hasError()) {
        if (error)
            *error = QCoreApplication::translate("Profile", "XML error at line %1: %2").arg(r.lineNumber()).arg(r.errorString());
        return false;
    }

    // Proxy ids referenced by chains/rules must stay stable, so a Proxifier profile
    // with its "100", "101"... ids round-trips unchanged.
    for (const auto &pp : pending) {
        bool ok = false;
        QString pw = decodePassword(pp.stored, p.options.encryption, &ok);
        if (!ok && error)
            *error = QCoreApplication::translate("Profile", "Password of proxy %1 is encrypted in a format WinRoute cannot read; please re-enter it.")
                         .arg(p.proxies[pp.proxyIndex].displayName());
        p.proxies[pp.proxyIndex].password = pw;
    }

    if (p.rules.isEmpty()) {
        Rule def;
        def.name = "Default";
        p.rules.append(def);
    }
    *this = p;
    return true;
}

QByteArray Profile::toXml() const
{
    QByteArray out;
    QXmlStreamWriter w(&out);
    w.setAutoFormatting(true);
    w.setAutoFormattingIndent(-1);   // tabs, like Proxifier 4
    w.writeStartDocument("1.0", true);
    w.writeStartElement("ProxifierProfile");
    w.writeAttribute("version", "102");
    w.writeAttribute("platform", "Windows");
    w.writeAttribute("product_id", "0");
    w.writeAttribute("product_minver", "400");

    auto boolStr = [](bool b) { return b ? QStringLiteral("true") : QStringLiteral("false"); };
    auto enabledElem = [&](const char *name, bool en) {
        w.writeEmptyElement(name);
        w.writeAttribute("enabled", boolStr(en));
    };

    w.writeStartElement("Options");
    w.writeStartElement("Resolve");
    enabledElem("AutoModeDetection", options.dnsAutoDetect);
    w.writeStartElement("ViaProxy");
    w.writeAttribute("enabled", boolStr(options.dnsViaProxy));
    enabledElem("TryLocalDnsFirst", options.dnsTryLocalFirst);
    w.writeEndElement();
    enabledElem("BlockNonATypes", options.dnsBlockNonATypes);
    w.writeStartElement("ExclusionList");
    w.writeAttribute("OnlyFromListMode", boolStr(options.dnsOnlyFromList));
    w.writeCharacters(options.dnsExclusionList);
    w.writeEndElement();
    w.writeTextElement("DnsUdpMode", QString::number(options.dnsUdpMode));
    w.writeEndElement(); // Resolve
    w.writeEmptyElement("Encryption");
    w.writeAttribute("mode", encryptionToString(options.encryption));
    w.writeEmptyElement("ConnectionLoopDetection");
    w.writeAttribute("enabled", boolStr(options.loopDetection));
    w.writeAttribute("resolve", boolStr(options.loopDetectionResolve));
    w.writeEmptyElement("Udp");
    w.writeAttribute("mode", options.udpMode);
    enabledElem("LeakPreventionMode", options.leakPrevention);
    enabledElem("ProcessOtherUsers", options.processOtherUsers);
    enabledElem("ProcessServices", options.processServices);
    enabledElem("HandleDirectConnections", options.handleDirectConnections);
    enabledElem("HttpProxiesSupport", options.httpProxiesSupport);
    enabledElem("WinRouteLogDns", options.logDns);   // WinRoute extension
    w.writeEndElement(); // Options

    w.writeStartElement("ProxyList");
    for (const auto &s : proxies) {
        w.writeStartElement("Proxy");
        w.writeAttribute("id", QString::number(s.id));
        w.writeAttribute("type", proxyTypeToString(s.type));
        if (s.authEnabled || !s.username.isEmpty()) {
            w.writeStartElement("Authentication");
            w.writeAttribute("enabled", boolStr(s.authEnabled));
            w.writeTextElement("Password", encodePassword(s.password, options.encryption));
            w.writeTextElement("Username", s.username);
            w.writeEndElement();
        }
        if (!s.label.isEmpty())
            w.writeTextElement("Label", s.label);
        if (s.type == ProxyType::Socks4)
            enabledElem("Socks4A", s.socks4a);
        if (!s.useHostname)
            enabledElem("UseHostname", false);
        w.writeTextElement("Options", QString::number(s.options));
        w.writeTextElement("Port", QString::number(s.port));
        w.writeTextElement("Address", s.address);
        w.writeEndElement();
    }
    w.writeEndElement();

    if (chains.isEmpty()) {
        w.writeEmptyElement("ChainList");
    } else {
        w.writeStartElement("ChainList");
        for (const auto &c : chains) {
            w.writeStartElement("Chain");
            w.writeAttribute("id", QString::number(c.id));
            w.writeAttribute("type", chainTypeToString(c.type));
            for (const auto &m : c.members) {
                w.writeStartElement("Proxy");
                w.writeAttribute("enabled", boolStr(m.enabled));
                w.writeCharacters(QString::number(m.proxyId));
                w.writeEndElement();
            }
            if (c.type == ChainType::Redundancy) {
                w.writeTextElement("RedundancyTimeout", QString::number(c.redundancyTimeout));
                w.writeStartElement("RedundancyRecheck");
                w.writeAttribute("enabled", boolStr(c.redundancyRecheck));
                w.writeCharacters(QString::number(c.redundancyRecheckSec));
                w.writeEndElement();
                enabledElem("RedundancyTryDirect", c.redundancyDirectFallback);
            } else if (c.type == ChainType::LoadBalancing) {
                enabledElem("LoadBalancingSamePid", c.lbSamePidSameProxy);
            }
            w.writeTextElement("Name", c.name);
            w.writeEndElement();
        }
        w.writeEndElement();
    }

    w.writeStartElement("RuleList");
    for (const auto &rule : rules) {
        w.writeStartElement("Rule");
        w.writeAttribute("enabled", boolStr(rule.enabled));
        switch (rule.action.type) {
        case ActionType::Direct:
            w.writeEmptyElement("Action");
            w.writeAttribute("type", "Direct");
            break;
        case ActionType::Block:
            w.writeEmptyElement("Action");
            w.writeAttribute("type", "Block");
            break;
        case ActionType::Proxy:
        case ActionType::Chain:
            w.writeStartElement("Action");
            w.writeAttribute("type", rule.action.type == ActionType::Proxy ? "Proxy" : "Chain");
            w.writeCharacters(QString::number(rule.action.id));
            w.writeEndElement();
            break;
        }
        if (!rule.ports.isEmpty())
            w.writeTextElement("Ports", rule.ports);
        if (!rule.targets.isEmpty())
            w.writeTextElement("Targets", rule.targets);
        if (!rule.applications.isEmpty())
            w.writeTextElement("Applications", util::joinAppList(rule.applications));
        w.writeTextElement("Name", rule.name);
        w.writeEndElement();
    }
    w.writeEndElement();

    w.writeEndElement(); // ProxifierProfile
    w.writeEndDocument();

    // Proxifier does not decode &quot; in an Applications list: it reads the
    // entity literally, so a quoted name with a space ("League of Legends.exe")
    // still trips "Unexpected blank space found". Its own profiles keep the
    // quotes literal, which is well-formed in element text, so undo the
    // escaping. No user text is ever written into an attribute (only fixed
    // enums, ids and booleans), so this only ever affects element content.
    out.replace("&quot;", "\"");
    return out;
}
