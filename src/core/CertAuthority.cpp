#include "CertAuthority.h"
#include "Utils.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QSaveFile>

#include <functional>

// Windows cert-store headers clash with OpenSSL (both define X509_NAME etc.);
// include wincrypt first and drop the clashing macros before OpenSSL.
#include <winsock2.h>
#include <windows.h>
#include <wincrypt.h>
#undef X509_NAME
#undef X509_EXTENSIONS
#undef X509_CERT_PAIR
#undef PKCS7_ISSUER_AND_SERIAL
#undef PKCS7_SIGNER_INFO
#undef OCSP_REQUEST
#undef OCSP_RESPONSE

#include <openssl/bn.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

namespace {

constexpr int kKeyBits = 2048;

QString caKeyPath() { return QDir(CertAuthority::caDir()).filePath("winroute-ca.key"); }
QString caCertPath() { return QDir(CertAuthority::caDir()).filePath("winroute-ca.crt"); }
QString leafKeyPath() { return QDir(CertAuthority::caDir()).filePath("winroute-leaf.key"); }

EVP_PKEY *genRsaKey()
{
    EVP_PKEY *pkey = nullptr;
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    if (!ctx)
        return nullptr;
    if (EVP_PKEY_keygen_init(ctx) > 0 && EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, kKeyBits) > 0)
        EVP_PKEY_keygen(ctx, &pkey);
    EVP_PKEY_CTX_free(ctx);
    return pkey;
}

QByteArray toPem(const std::function<int(BIO *)> &writer)
{
    BIO *bio = BIO_new(BIO_s_mem());
    if (!bio)
        return {};
    QByteArray out;
    if (writer(bio) == 1) {
        char *data = nullptr;
        const long len = BIO_get_mem_data(bio, &data);
        if (data && len > 0)
            out = QByteArray(data, int(len));
    }
    BIO_free(bio);
    return out;
}

QByteArray certToPem(X509 *x)
{
    return toPem([x](BIO *bio) { return PEM_write_bio_X509(bio, x); });
}

QByteArray keyToPem(EVP_PKEY *k)
{
    return toPem([k](BIO *bio) {
        return PEM_write_bio_PrivateKey(bio, k, nullptr, nullptr, 0, nullptr, nullptr);
    });
}

X509 *pemToCert(const QByteArray &pem)
{
    BIO *bio = BIO_new_mem_buf(pem.constData(), int(pem.size()));
    if (!bio)
        return nullptr;
    X509 *x = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    return x;
}

EVP_PKEY *pemToKey(const QByteArray &pem)
{
    BIO *bio = BIO_new_mem_buf(pem.constData(), int(pem.size()));
    if (!bio)
        return nullptr;
    EVP_PKEY *k = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    return k;
}

QByteArray certToDer(X509 *x)
{
    const int len = i2d_X509(x, nullptr);
    if (len <= 0)
        return {};
    QByteArray out(len, Qt::Uninitialized);
    auto *p = reinterpret_cast<unsigned char *>(out.data());
    i2d_X509(x, &p);
    return out;
}

// A random positive serial number.
void setRandomSerial(X509 *x)
{
    BIGNUM *bn = BN_new();
    if (bn && BN_rand(bn, 64, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY)) {
        ASN1_INTEGER *serial = BN_to_ASN1_INTEGER(bn, nullptr);
        if (serial) {
            X509_set_serialNumber(x, serial);
            ASN1_INTEGER_free(serial);
        }
    }
    BN_free(bn);
}

bool addExt(X509 *cert, X509 *issuer, int nid, const QByteArray &value)
{
    X509V3_CTX ctx;
    X509V3_set_ctx_nodb(&ctx);
    X509V3_set_ctx(&ctx, issuer, cert, nullptr, nullptr, 0);
    X509_EXTENSION *ex = X509V3_EXT_conf_nid(nullptr, &ctx, nid, const_cast<char *>(value.constData()));
    if (!ex)
        return false;
    X509_add_ext(cert, ex, -1);
    X509_EXTENSION_free(ex);
    return true;
}

void setSubject(X509_NAME *name, const QByteArray &org, const QByteArray &cn)
{
    if (!org.isEmpty())
        X509_NAME_add_entry_by_txt(name, "O", MBSTRING_UTF8, reinterpret_cast<const unsigned char *>(org.constData()), -1, -1, 0);
    if (!cn.isEmpty())
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_UTF8, reinterpret_cast<const unsigned char *>(cn.constData()), -1, -1, 0);
}

// Builds a Windows cert context from DER; caller frees with CertFreeCertificateContext.
PCCERT_CONTEXT makeCertContext(const QByteArray &der)
{
    if (der.isEmpty())
        return nullptr;
    return CertCreateCertificateContext(X509_ASN_ENCODING, reinterpret_cast<const BYTE *>(der.constData()),
                                        DWORD(der.size()));
}

} // namespace

CertAuthority::CertAuthority() = default;

CertAuthority::~CertAuthority()
{
    if (m_caCert)
        X509_free(static_cast<X509 *>(m_caCert));
    if (m_caKey)
        EVP_PKEY_free(static_cast<EVP_PKEY *>(m_caKey));
    if (m_leafKey)
        EVP_PKEY_free(static_cast<EVP_PKEY *>(m_leafKey));
}

QString CertAuthority::caDir()
{
    return QDir(util::dataDir()).filePath("ca");
}

bool CertAuthority::exists() const
{
    return QFile::exists(caKeyPath()) && QFile::exists(caCertPath());
}

bool CertAuthority::ensureCa(QString *error)
{
    QMutexLocker lock(&m_mutex);
    return loadOrCreateLocked(CaInfo{}, error);
}

bool CertAuthority::createCa(const CaInfo &info, QString *error)
{
    QMutexLocker lock(&m_mutex);
    // Drop any loaded CA and remove the files so a fresh one is generated with
    // the requested subject.
    if (m_caCert) { X509_free(static_cast<X509 *>(m_caCert)); m_caCert = nullptr; }
    if (m_caKey) { EVP_PKEY_free(static_cast<EVP_PKEY *>(m_caKey)); m_caKey = nullptr; }
    if (m_leafKey) { EVP_PKEY_free(static_cast<EVP_PKEY *>(m_leafKey)); m_leafKey = nullptr; }
    m_leafCache.clear();
    m_loaded = false;
    QFile::remove(caKeyPath());
    QFile::remove(caCertPath());
    QFile::remove(leafKeyPath());
    return loadOrCreateLocked(info, error);
}

bool CertAuthority::loadOrCreateLocked(const CaInfo &info, QString *error)
{
    if (m_loaded)
        return true;

    QDir().mkpath(caDir());

    // Load an existing CA, or make a new one.
    if (QFile::exists(caKeyPath()) && QFile::exists(caCertPath())) {
        QFile kf(caKeyPath()), cf(caCertPath());
        if (kf.open(QIODevice::ReadOnly) && cf.open(QIODevice::ReadOnly)) {
            EVP_PKEY *key = pemToKey(kf.readAll());
            X509 *cert = pemToCert(cf.readAll());
            if (key && cert) {
                m_caKey = key;
                m_caCert = cert;
            } else {
                if (key)
                    EVP_PKEY_free(key);
                if (cert)
                    X509_free(cert);
            }
        }
    }

    if (!m_caKey || !m_caCert) {
        EVP_PKEY *key = genRsaKey();
        if (!key) {
            if (error)
                *error = QStringLiteral("could not generate the CA key");
            return false;
        }
        X509 *cert = X509_new();
        X509_set_version(cert, 2);   // X.509 v3
        setRandomSerial(cert);
        const long days = info.validityDays > 0 ? info.validityDays : 3650;
        X509_gmtime_adj(X509_getm_notBefore(cert), -24 * 3600);
        X509_gmtime_adj(X509_getm_notAfter(cert), days * 24LL * 3600);
        X509_set_pubkey(cert, key);
        const QByteArray cn = info.commonName.trimmed().isEmpty() ? QByteArrayLiteral("WinRoute Root CA")
                                                                  : info.commonName.trimmed().toUtf8();
        setSubject(X509_get_subject_name(cert), info.organization.trimmed().toUtf8(), cn);
        X509_set_issuer_name(cert, X509_get_subject_name(cert));
        addExt(cert, cert, NID_basic_constraints, "critical,CA:TRUE");
        addExt(cert, cert, NID_key_usage, "critical,keyCertSign,cRLSign");
        addExt(cert, cert, NID_subject_key_identifier, "hash");
        if (!X509_sign(cert, key, EVP_sha256())) {
            EVP_PKEY_free(key);
            X509_free(cert);
            if (error)
                *error = QStringLiteral("could not sign the CA certificate");
            return false;
        }

        const QByteArray keyPem = keyToPem(key);
        const QByteArray certPem = certToPem(cert);
        QSaveFile kf(caKeyPath()), cf(caCertPath());
        const bool ok = kf.open(QIODevice::WriteOnly) && kf.write(keyPem) == keyPem.size() && kf.commit()
                        && cf.open(QIODevice::WriteOnly) && cf.write(certPem) == certPem.size() && cf.commit();
        if (!ok) {
            EVP_PKEY_free(key);
            X509_free(cert);
            if (error)
                *error = QStringLiteral("could not save the CA to %1").arg(QDir::toNativeSeparators(caDir()));
            return false;
        }
        m_caKey = key;
        m_caCert = cert;
    }

    // One shared leaf key, kept on disk so exports/handshakes are stable.
    if (QFile::exists(leafKeyPath())) {
        QFile lf(leafKeyPath());
        if (lf.open(QIODevice::ReadOnly))
            m_leafKey = pemToKey(lf.readAll());
    }
    if (!m_leafKey) {
        EVP_PKEY *lk = genRsaKey();
        if (!lk) {
            if (error)
                *error = QStringLiteral("could not generate the leaf key");
            return false;
        }
        const QByteArray lkPem = keyToPem(lk);
        QSaveFile lf(leafKeyPath());
        if (lf.open(QIODevice::WriteOnly) && lf.write(lkPem) == lkPem.size())
            lf.commit();
        m_leafKey = lk;
    }

    m_loaded = true;
    return true;
}

CertAuthority::Leaf CertAuthority::makeLeafLocked(const QString &host)
{
    auto *caCert = static_cast<X509 *>(m_caCert);
    auto *caKey = static_cast<EVP_PKEY *>(m_caKey);
    auto *leafKey = static_cast<EVP_PKEY *>(m_leafKey);
    if (!caCert || !caKey || !leafKey)
        return {};

    X509 *x = X509_new();
    X509_set_version(x, 2);
    setRandomSerial(x);
    X509_gmtime_adj(X509_getm_notBefore(x), -24 * 3600);
    X509_gmtime_adj(X509_getm_notAfter(x), 398LL * 24 * 3600);   // <= 398 days
    X509_set_pubkey(x, leafKey);
    setSubject(X509_get_subject_name(x), {}, host.toUtf8());
    X509_set_issuer_name(x, X509_get_subject_name(caCert));

    addExt(x, caCert, NID_basic_constraints, "critical,CA:FALSE");
    addExt(x, caCert, NID_key_usage, "critical,digitalSignature,keyEncipherment");
    addExt(x, caCert, NID_ext_key_usage, "serverAuth");
    addExt(x, caCert, NID_subject_key_identifier, "hash");
    addExt(x, caCert, NID_authority_key_identifier, "keyid,issuer");

    const bool isIp = !QHostAddress(host).isNull();
    const QByteArray san = (isIp ? QByteArrayLiteral("IP:") : QByteArrayLiteral("DNS:")) + host.toUtf8();
    addExt(x, caCert, NID_subject_alt_name, san);

    Leaf leaf;
    if (X509_sign(x, caKey, EVP_sha256())) {
        leaf.certPem = certToPem(x);
        leaf.keyPem = keyToPem(leafKey);
    }
    X509_free(x);
    return leaf;
}

CertAuthority::Leaf CertAuthority::leafFor(const QString &host)
{
    QMutexLocker lock(&m_mutex);
    if (!loadOrCreateLocked(CaInfo{}, nullptr))
        return {};
    auto it = m_leafCache.constFind(host);
    if (it != m_leafCache.constEnd())
        return it.value();
    Leaf leaf = makeLeafLocked(host);
    if (!leaf.certPem.isEmpty())
        m_leafCache.insert(host, leaf);
    return leaf;
}

QByteArray CertAuthority::caCertPem() const
{
    QMutexLocker lock(&m_mutex);
    // Load an existing CA on demand (but never generate one as a side effect of
    // a read), so the certificate is available even if ensureCa() has not run
    // this session.
    if (!m_caCert && QFile::exists(caKeyPath()) && QFile::exists(caCertPath()))
        const_cast<CertAuthority *>(this)->loadOrCreateLocked(CaInfo{}, nullptr);
    if (!m_caCert)
        return {};
    return certToPem(static_cast<X509 *>(m_caCert));
}

QByteArray CertAuthority::caCertDerLocked() const
{
    if (!m_caCert && QFile::exists(caKeyPath()) && QFile::exists(caCertPath()))
        const_cast<CertAuthority *>(this)->loadOrCreateLocked(CaInfo{}, nullptr);
    if (!m_caCert)
        return {};
    return certToDer(static_cast<X509 *>(m_caCert));
}

bool CertAuthority::exportCaCert(const QString &path, QString *error) const
{
    const QByteArray pem = caCertPem();
    if (pem.isEmpty()) {
        if (error)
            *error = QStringLiteral("the root certificate is not available");
        return false;
    }
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly) || f.write(pem) != pem.size() || !f.commit()) {
        if (error)
            *error = f.errorString();
        return false;
    }
    return true;
}

bool CertAuthority::isInstalled() const
{
    QByteArray der;
    {
        QMutexLocker lock(&m_mutex);
        der = caCertDerLocked();
    }
    PCCERT_CONTEXT ours = makeCertContext(der);
    if (!ours)
        return false;
    HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0,
                                     CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG, L"Root");
    bool present = false;
    if (store) {
        // Match this exact certificate (not just a subject name), so a custom CA
        // name still works and we never touch another certificate.
        PCCERT_CONTEXT found = CertFindCertificateInStore(store, X509_ASN_ENCODING, 0, CERT_FIND_EXISTING, ours, nullptr);
        present = found != nullptr;
        if (found)
            CertFreeCertificateContext(found);
        CertCloseStore(store, 0);
    }
    CertFreeCertificateContext(ours);
    return present;
}

bool CertAuthority::install(QString *error)
{
    QByteArray der;
    {
        QMutexLocker lock(&m_mutex);
        if (!loadOrCreateLocked(CaInfo{}, error))
            return false;
        der = certToDer(static_cast<X509 *>(m_caCert));
    }
    if (der.isEmpty()) {
        if (error)
            *error = QStringLiteral("the root certificate is not available");
        return false;
    }

    HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0, CERT_SYSTEM_STORE_CURRENT_USER, L"Root");
    if (!store) {
        if (error)
            *error = QStringLiteral("could not open the Trusted Root store");
        return false;
    }
    const bool ok = CertAddEncodedCertificateToStore(store, X509_ASN_ENCODING,
                                                     reinterpret_cast<const BYTE *>(der.constData()),
                                                     DWORD(der.size()), CERT_STORE_ADD_REPLACE_EXISTING, nullptr);
    if (!ok && error)
        *error = QStringLiteral("could not add the certificate to the Trusted Root store (error %1)").arg(GetLastError());
    CertCloseStore(store, 0);
    return ok;
}

bool CertAuthority::uninstall(QString *error)
{
    QByteArray der;
    {
        QMutexLocker lock(&m_mutex);
        der = caCertDerLocked();
    }
    PCCERT_CONTEXT ours = makeCertContext(der);
    if (!ours) {
        if (error)
            *error = QStringLiteral("the root certificate is not available");
        return false;
    }
    HCERTSTORE store = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0, CERT_SYSTEM_STORE_CURRENT_USER, L"Root");
    if (!store) {
        CertFreeCertificateContext(ours);
        if (error)
            *error = QStringLiteral("could not open the Trusted Root store");
        return false;
    }
    bool removedAny = false, failed = false;
    PCCERT_CONTEXT found = nullptr;
    while ((found = CertFindCertificateInStore(store, X509_ASN_ENCODING, 0, CERT_FIND_EXISTING, ours, nullptr))
           != nullptr) {
        // CertDeleteCertificateFromStore frees the context it is given.
        if (!CertDeleteCertificateFromStore(found)) {
            failed = true;
            break;
        }
        removedAny = true;
    }
    CertCloseStore(store, 0);
    CertFreeCertificateContext(ours);
    if (failed) {
        if (error)
            *error = QStringLiteral("could not remove the certificate (error %1)").arg(GetLastError());
        return false;
    }
    if (!removedAny && error)
        *error = QStringLiteral("no matching root certificate was installed");
    return removedAny;
}
