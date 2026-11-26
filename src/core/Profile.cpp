#include "Profile.h"

#include "Utils.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <yaml-cpp/yaml.h>

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
    // Manual name resolution, off, by default: WinRoute never silently flips
    // every name to proxy resolution. Automatic detection (which overrides the
    // "resolve via proxy" switch from a single DNS probe, so a flaky probe can
    // force all traffic through a proxy with no way to turn it off) is opt-in.
    p.options.dnsAutoDetect = false;
    p.options.dnsViaProxy = false;
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

int Profile::defaultDnsId() const
{
    const int id = options.dnsDirectProxyId;
    if (id && (proxyById(id) || chainById(id)))
        return id;
    if (!proxies.isEmpty())
        return proxies.first().id;
    if (!chains.isEmpty())
        return chains.first().id;
    return 0;
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
                        } else if (rn == QLatin1String("WinRouteDnsDirectProxy")) {   // WinRoute extensions
                            p.options.dnsDirectProxyId = r.readElementText().toInt();
                        } else if (rn == QLatin1String("WinRouteDnsServer")) {
                            const QString s = r.readElementText().trimmed();
                            if (!s.isEmpty())
                                p.options.dnsProxyServer = s;
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
                // Proxifier 4's per-rule network interface ("Advanced"), kept as
                // attributes of <Rule>: interfaceId="{GUID}" skipIfInterfaceDown="false".
                rule.action.interfaceId = r.attributes().value("interfaceId").toString().trimmed();
                rule.action.skipIfInterfaceDown = attrBool(r, "skipIfInterfaceDown");
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
                    } else if (n == QLatin1String("WinRouteResolve")) {
                        // WinRoute extension; Proxifier ignores unknown elements.
                        rule.resolveProxyId = r.attributes().value("proxy").toInt();
                        const QString v = r.readElementText().trimmed();
                        rule.resolve = v == QLatin1String("proxy") ? DnsResolve::Proxy
                                     : v == QLatin1String("local") ? DnsResolve::Local
                                                                   : DnsResolve::Inherit;
                    } else if (n == QLatin1String("WinRouteUdp")) {
                        // WinRoute extension; Proxifier ignores unknown elements.
                        const QString v = r.readElementText().trimmed();
                        rule.udp = v == QLatin1String("enable") ? UdpControl::Enable
                                 : v == QLatin1String("disable") ? UdpControl::Disable
                                                                 : UdpControl::Inherit;
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
                if (rule.action.type == ActionType::Block || rule.action.interfaceId.isEmpty()) {
                    rule.action.interfaceId.clear();   // a blocked connection has no adapter
                    rule.action.skipIfInterfaceDown = false;
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
    // WinRoute extensions; Proxifier ignores unknown elements.
    if (options.dnsDirectProxyId)
        w.writeTextElement("WinRouteDnsDirectProxy", QString::number(options.dnsDirectProxyId));
    if (options.dnsProxyServer != ProfileOptions().dnsProxyServer)
        w.writeTextElement("WinRouteDnsServer", options.dnsProxyServer);
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
        if (!rule.action.interfaceId.isEmpty() && rule.action.type != ActionType::Block) {
            w.writeAttribute("interfaceId", rule.action.interfaceId);
            w.writeAttribute("skipIfInterfaceDown", boolStr(rule.action.skipIfInterfaceDown));
        }
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
        if (rule.resolve != DnsResolve::Inherit) {   // WinRoute extension
            w.writeStartElement("WinRouteResolve");
            if (rule.resolve == DnsResolve::Proxy && rule.resolveProxyId)
                w.writeAttribute("proxy", QString::number(rule.resolveProxyId));
            w.writeCharacters(rule.resolve == DnsResolve::Proxy ? "proxy" : "local");
            w.writeEndElement();
        }
        if (rule.udp != UdpControl::Inherit) {   // WinRoute extension
            w.writeTextElement("WinRouteUdp", rule.udp == UdpControl::Enable ? "enable" : "disable");
        }
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

// ---- native format (YAML) --------------------------------------------------
// WinRoute's own profile format, independent of the Proxifier .ppx schema.

static const char *actionTypeToString(ActionType t)
{
    switch (t) {
    case ActionType::Direct: return "direct";
    case ActionType::Block:  return "block";
    case ActionType::Proxy:  return "proxy";
    case ActionType::Chain:  return "chain";
    }
    return "direct";
}

static ActionType actionTypeFromString(const QString &s)
{
    if (s == "proxy") return ActionType::Proxy;
    if (s == "chain") return ActionType::Chain;
    if (s == "block") return ActionType::Block;
    return ActionType::Direct;
}

QByteArray Profile::toYaml() const
{
    YAML::Emitter e;
    auto s = [](const QString &x) { return x.toStdString(); };
    e << YAML::BeginMap;
    e << YAML::Key << "version" << YAML::Value << 1;

    e << YAML::Key << "options" << YAML::Value << YAML::BeginMap;
    e << YAML::Key << "dns" << YAML::Value << YAML::BeginMap;
    e << YAML::Key << "autoDetect" << YAML::Value << options.dnsAutoDetect;
    e << YAML::Key << "viaProxy" << YAML::Value << options.dnsViaProxy;
    e << YAML::Key << "tryLocalFirst" << YAML::Value << options.dnsTryLocalFirst;
    e << YAML::Key << "blockNonATypes" << YAML::Value << options.dnsBlockNonATypes;
    e << YAML::Key << "onlyFromList" << YAML::Value << options.dnsOnlyFromList;
    e << YAML::Key << "exclusionList" << YAML::Value << s(options.dnsExclusionList);
    e << YAML::Key << "udpMode" << YAML::Value << options.dnsUdpMode;
    e << YAML::Key << "directProxy" << YAML::Value << options.dnsDirectProxyId;
    e << YAML::Key << "server" << YAML::Value << s(options.dnsProxyServer);
    e << YAML::Key << "log" << YAML::Value << options.logDns;
    e << YAML::EndMap;
    e << YAML::Key << "encryption" << YAML::Value << s(encryptionToString(options.encryption));
    e << YAML::Key << "loopDetection" << YAML::Value << options.loopDetection;
    e << YAML::Key << "loopDetectionResolve" << YAML::Value << options.loopDetectionResolve;
    e << YAML::Key << "udpMode" << YAML::Value << s(options.udpMode);
    e << YAML::Key << "leakPrevention" << YAML::Value << options.leakPrevention;
    e << YAML::Key << "processOtherUsers" << YAML::Value << options.processOtherUsers;
    e << YAML::Key << "processServices" << YAML::Value << options.processServices;
    e << YAML::Key << "handleDirectConnections" << YAML::Value << options.handleDirectConnections;
    e << YAML::Key << "httpProxiesSupport" << YAML::Value << options.httpProxiesSupport;
    e << YAML::EndMap;

    e << YAML::Key << "proxies" << YAML::Value << YAML::BeginSeq;
    for (const auto &p : proxies) {
        e << YAML::BeginMap;
        e << YAML::Key << "id" << YAML::Value << p.id;
        e << YAML::Key << "type" << YAML::Value << s(proxyTypeToString(p.type));
        e << YAML::Key << "address" << YAML::Value << s(p.address);
        e << YAML::Key << "port" << YAML::Value << p.port;
        if (!p.label.isEmpty())
            e << YAML::Key << "label" << YAML::Value << s(p.label);
        if (p.authEnabled || !p.username.isEmpty()) {
            e << YAML::Key << "auth" << YAML::Value << p.authEnabled;
            if (!p.username.isEmpty())
                e << YAML::Key << "username" << YAML::Value << s(p.username);
            if (!p.password.isEmpty())
                e << YAML::Key << "password" << YAML::Value << s(encodePassword(p.password, options.encryption));
        }
        if (p.type == ProxyType::Socks4)
            e << YAML::Key << "socks4a" << YAML::Value << p.socks4a;
        if (!p.useHostname)
            e << YAML::Key << "useHostname" << YAML::Value << p.useHostname;
        e << YAML::Key << "options" << YAML::Value << p.options;
        e << YAML::EndMap;
    }
    e << YAML::EndSeq;

    e << YAML::Key << "chains" << YAML::Value << YAML::BeginSeq;
    for (const auto &c : chains) {
        e << YAML::BeginMap;
        e << YAML::Key << "id" << YAML::Value << c.id;
        e << YAML::Key << "name" << YAML::Value << s(c.name);
        e << YAML::Key << "type" << YAML::Value << s(chainTypeToString(c.type));
        e << YAML::Key << "members" << YAML::Value << YAML::BeginSeq;
        for (const auto &m : c.members) {
            e << YAML::Flow << YAML::BeginMap;
            e << YAML::Key << "id" << YAML::Value << m.proxyId;
            e << YAML::Key << "enabled" << YAML::Value << m.enabled;
            e << YAML::EndMap;
        }
        e << YAML::EndSeq;
        if (c.type == ChainType::Redundancy) {
            e << YAML::Key << "redundancyTimeout" << YAML::Value << c.redundancyTimeout;
            e << YAML::Key << "redundancyRecheck" << YAML::Value << c.redundancyRecheck;
            e << YAML::Key << "redundancyRecheckSec" << YAML::Value << c.redundancyRecheckSec;
            e << YAML::Key << "redundancyTryDirect" << YAML::Value << c.redundancyDirectFallback;
        } else if (c.type == ChainType::LoadBalancing) {
            e << YAML::Key << "loadBalancingSamePid" << YAML::Value << c.lbSamePidSameProxy;
        }
        e << YAML::EndMap;
    }
    e << YAML::EndSeq;

    e << YAML::Key << "rules" << YAML::Value << YAML::BeginSeq;
    for (const auto &r : rules) {
        e << YAML::BeginMap;
        e << YAML::Key << "name" << YAML::Value << s(r.name);
        e << YAML::Key << "enabled" << YAML::Value << r.enabled;
        if (!r.applications.isEmpty())
            e << YAML::Key << "applications" << YAML::Value << s(util::joinAppList(r.applications));
        if (!r.targets.isEmpty())
            e << YAML::Key << "targets" << YAML::Value << s(r.targets);
        if (!r.ports.isEmpty())
            e << YAML::Key << "ports" << YAML::Value << s(r.ports);
        e << YAML::Key << "action" << YAML::Value << YAML::Flow << YAML::BeginMap;
        e << YAML::Key << "type" << YAML::Value << actionTypeToString(r.action.type);
        if (r.action.type == ActionType::Proxy || r.action.type == ActionType::Chain)
            e << YAML::Key << "id" << YAML::Value << r.action.id;
        e << YAML::EndMap;
        if (!r.action.interfaceId.isEmpty() && r.action.type != ActionType::Block) {
            e << YAML::Key << "interface" << YAML::Value << s(r.action.interfaceId);
            e << YAML::Key << "skipIfInterfaceDown" << YAML::Value << r.action.skipIfInterfaceDown;
        }
        if (r.resolve != DnsResolve::Inherit) {
            e << YAML::Key << "resolve" << YAML::Value << (r.resolve == DnsResolve::Proxy ? "proxy" : "local");
            if (r.resolve == DnsResolve::Proxy && r.resolveProxyId)
                e << YAML::Key << "resolveProxy" << YAML::Value << r.resolveProxyId;
        }
        if (r.udp != UdpControl::Inherit)
            e << YAML::Key << "udp" << YAML::Value << (r.udp == UdpControl::Enable ? "enable" : "disable");
        e << YAML::EndMap;
    }
    e << YAML::EndSeq;

    e << YAML::EndMap;
    QByteArray out(e.c_str(), int(e.size()));
    out.append('\n');
    return out;
}

bool Profile::fromYaml(const QByteArray &yaml, QString *error)
{
    YAML::Node root;
    try {
        root = YAML::Load(std::string(yaml.constData(), size_t(yaml.size())));
    } catch (const std::exception &ex) {
        if (error)
            *error = QCoreApplication::translate("Profile", "YAML error: %1").arg(QString::fromUtf8(ex.what()));
        return false;
    }
    if (!root.IsMap()) {
        if (error)
            *error = QCoreApplication::translate("Profile", "Not a WinRoute profile");
        return false;
    }
    auto qs = [](const YAML::Node &n, const QString &def = QString()) {
        return n && n.IsScalar() ? QString::fromStdString(n.Scalar()) : def;
    };
    auto qb = [](const YAML::Node &n, bool def) { return n ? n.as<bool>(def) : def; };
    auto qi = [](const YAML::Node &n, int def) { return n ? n.as<int>(def) : def; };

    Profile p;
    if (const YAML::Node o = root["options"]) {
        if (const YAML::Node d = o["dns"]) {
            p.options.dnsAutoDetect = qb(d["autoDetect"], p.options.dnsAutoDetect);
            p.options.dnsViaProxy = qb(d["viaProxy"], p.options.dnsViaProxy);
            p.options.dnsTryLocalFirst = qb(d["tryLocalFirst"], p.options.dnsTryLocalFirst);
            p.options.dnsBlockNonATypes = qb(d["blockNonATypes"], p.options.dnsBlockNonATypes);
            p.options.dnsOnlyFromList = qb(d["onlyFromList"], p.options.dnsOnlyFromList);
            p.options.dnsExclusionList = qs(d["exclusionList"], p.options.dnsExclusionList);
            p.options.dnsUdpMode = qi(d["udpMode"], p.options.dnsUdpMode);
            p.options.dnsDirectProxyId = qi(d["directProxy"], p.options.dnsDirectProxyId);
            p.options.dnsProxyServer = qs(d["server"], p.options.dnsProxyServer);
            p.options.logDns = qb(d["log"], p.options.logDns);
        }
        p.options.encryption = encryptionFromString(qs(o["encryption"], encryptionToString(p.options.encryption)));
        p.options.loopDetection = qb(o["loopDetection"], p.options.loopDetection);
        p.options.loopDetectionResolve = qb(o["loopDetectionResolve"], p.options.loopDetectionResolve);
        p.options.udpMode = qs(o["udpMode"], p.options.udpMode);
        p.options.leakPrevention = qb(o["leakPrevention"], p.options.leakPrevention);
        p.options.processOtherUsers = qb(o["processOtherUsers"], p.options.processOtherUsers);
        p.options.processServices = qb(o["processServices"], p.options.processServices);
        p.options.handleDirectConnections = qb(o["handleDirectConnections"], p.options.handleDirectConnections);
        p.options.httpProxiesSupport = qb(o["httpProxiesSupport"], p.options.httpProxiesSupport);
    }

    if (const YAML::Node ps = root["proxies"]) {
        for (const auto &pn : ps) {
            ProxyServer sv;
            sv.id = qi(pn["id"], 0);
            sv.type = proxyTypeFromString(qs(pn["type"], "SOCKS5"));
            sv.address = qs(pn["address"]).trimmed();
            sv.port = quint16(qi(pn["port"], 1080));
            sv.label = qs(pn["label"]);
            sv.authEnabled = qb(pn["auth"], false);
            sv.username = qs(pn["username"]);
            const QString stored = qs(pn["password"]);
            if (!stored.isEmpty()) {
                bool ok = false;
                sv.password = decodePassword(stored, p.options.encryption, &ok);
            }
            sv.socks4a = qb(pn["socks4a"], true);
            sv.useHostname = qb(pn["useHostname"], true);
            sv.options = qi(pn["options"], 48);
            p.proxies.append(sv);
        }
    }

    if (const YAML::Node cs = root["chains"]) {
        for (const auto &cn : cs) {
            ProxyChain c;
            c.id = qi(cn["id"], 0);
            c.name = qs(cn["name"]);
            c.type = chainTypeFromString(qs(cn["type"], "simple"));
            if (const YAML::Node ms = cn["members"]) {
                for (const auto &mn : ms) {
                    ChainMember m;
                    m.proxyId = qi(mn["id"], 0);
                    m.enabled = qb(mn["enabled"], true);
                    c.members.append(m);
                }
            }
            c.redundancyTimeout = qi(cn["redundancyTimeout"], c.redundancyTimeout);
            c.redundancyRecheck = qb(cn["redundancyRecheck"], c.redundancyRecheck);
            c.redundancyRecheckSec = qi(cn["redundancyRecheckSec"], c.redundancyRecheckSec);
            c.redundancyDirectFallback = qb(cn["redundancyTryDirect"], c.redundancyDirectFallback);
            c.lbSamePidSameProxy = qb(cn["loadBalancingSamePid"], c.lbSamePidSameProxy);
            p.chains.append(c);
        }
    }

    if (const YAML::Node rs = root["rules"]) {
        for (const auto &rn : rs) {
            Rule rule;
            rule.name = qs(rn["name"]);
            rule.enabled = qb(rn["enabled"], true);
            rule.applications = qs(rn["applications"]);
            rule.targets = qs(rn["targets"]);
            rule.ports = qs(rn["ports"]);
            if (const YAML::Node a = rn["action"]) {
                rule.action.type = actionTypeFromString(qs(a["type"], "direct"));
                rule.action.id = qi(a["id"], 0);
            }
            rule.action.interfaceId = qs(rn["interface"]).trimmed();
            rule.action.skipIfInterfaceDown = qb(rn["skipIfInterfaceDown"], false);
            const QString res = qs(rn["resolve"]);
            rule.resolve = res == "proxy" ? DnsResolve::Proxy
                         : res == "local" ? DnsResolve::Local
                                          : DnsResolve::Inherit;
            rule.resolveProxyId = qi(rn["resolveProxy"], 0);
            const QString u = qs(rn["udp"]);
            rule.udp = u == "enable" ? UdpControl::Enable
                     : u == "disable" ? UdpControl::Disable
                                      : UdpControl::Inherit;
            if (rule.action.type == ActionType::Block || rule.action.interfaceId.isEmpty()) {
                rule.action.interfaceId.clear();
                rule.action.skipIfInterfaceDown = false;
            }
            p.rules.append(rule);
        }
    }

    if (p.rules.isEmpty()) {
        Rule def;
        def.name = "Default";
        p.rules.append(def);
    }
    *this = p;
    return true;
}

bool Profile::loadYaml(const QString &path, QString *error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error)
            *error = f.errorString();
        return false;
    }
    return fromYaml(f.readAll(), error);
}

bool Profile::loadAuto(const QString &path, QString *error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error)
            *error = f.errorString();
        return false;
    }
    const QByteArray data = f.readAll();
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == "ppx" || suffix == "xml")
        return fromXml(data, error);
    if (suffix == "yml" || suffix == "yaml")
        return fromYaml(data, error);
    // Unknown extension: sniff. A Proxifier/WinRoute .ppx is XML, so its first
    // non-blank byte (past any BOM) is '<'; anything else is treated as YAML.
    int i = 0;
    while (i < data.size()
           && (data[i] == ' ' || data[i] == '\t' || data[i] == '\r' || data[i] == '\n'
               || static_cast<unsigned char>(data[i]) >= 0x80))
        ++i;
    if (i < data.size() && data[i] == '<')
        return fromXml(data, error);
    return fromYaml(data, error);
}

bool Profile::saveYaml(const QString &path, QString *error) const
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        if (error)
            *error = f.errorString();
        return false;
    }
    f.write(toYaml());
    if (!f.commit()) {
        if (error)
            *error = f.errorString();
        return false;
    }
    return true;
}
