#pragma once

#include <QByteArray>
#include <QHash>
#include <QMutex>
#include <QString>

// A small local certificate authority for the optional HTTPS decryption
// (MITM) feature. It owns a self-signed root CA, kept in %APPDATA%\WinRoute\ca,
// and signs a short-lived leaf certificate for each host the relay terminates
// TLS for. The root can be exported and installed into the Windows "Trusted
// Root Certification Authorities" store so the machine's browsers trust the
// leaves WinRoute presents.
//
// The CA private key never leaves this machine; installing the root is the
// user's decision about their own computer, and decryption is off by default.
//
// Thread-safe: leafFor() is called from the relay thread while the UI calls the
// status/install helpers; all state is guarded by one mutex.
class CertAuthority {
public:
    CertAuthority();
    ~CertAuthority();

    CertAuthority(const CertAuthority &) = delete;
    CertAuthority &operator=(const CertAuthority &) = delete;

    // The subject written into a newly generated root certificate. The defaults
    // are used when the CA is created implicitly (e.g. by turning decryption on).
    struct CaInfo {
        QString commonName = QStringLiteral("WinRoute Root CA");
        QString organization = QStringLiteral("WinRoute");
        int validityDays = 3650;   // ~10 years
    };
    static CaInfo defaultInfo() { return {}; }

    // Loads the root CA from disk, generating and saving a new one (with the
    // default subject) when none exists. Safe to call repeatedly. Returns false
    // (and sets *error) on failure; thereafter the CA is ready for leafFor().
    bool ensureCa(QString *error = nullptr);

    // Generates a brand-new root CA with the given subject, replacing any
    // existing one on disk and dropping every cached leaf. Use this to create
    // the CA with custom details, or to regenerate it. Returns false on error.
    bool createCa(const CaInfo &info, QString *error = nullptr);

    // True when the root CA key and certificate are present on disk.
    bool exists() const;

    // True when a root certificate with WinRoute's CA name is present in the
    // current user's Trusted Root store.
    bool isInstalled() const;

    // The root certificate in PEM, for display or export. Empty until ensureCa.
    QByteArray caCertPem() const;
    // Where the CA files live on disk.
    static QString caDir();

    // Writes the root certificate (PEM) to path. Returns false (sets *error) on
    // failure.
    bool exportCaCert(const QString &path, QString *error) const;

    // Adds / removes the root certificate in the current user's Trusted Root
    // store. Returns false and sets *error on failure.
    bool install(QString *error);
    bool uninstall(QString *error);

    // A leaf key + certificate chain (PEM) for a host, signed by the root. The
    // leaf key is shared across hosts and generated once; only the certificate
    // is minted per host, and results are cached. Returns empty PEM on error.
    struct Leaf {
        QByteArray certPem;   // the leaf certificate (PEM)
        QByteArray keyPem;    // its private key (PEM)
    };
    Leaf leafFor(const QString &host);

private:
    bool loadOrCreateLocked(const CaInfo &info, QString *error);   // m_mutex held
    Leaf makeLeafLocked(const QString &host);   // m_mutex held
    QByteArray caCertDerLocked() const;         // m_mutex held; loads if present

    mutable QMutex m_mutex;
    bool m_loaded = false;
    void *m_caCert = nullptr;    // X509*
    void *m_caKey = nullptr;     // EVP_PKEY*
    void *m_leafKey = nullptr;   // EVP_PKEY* (shared across leaves)
    QHash<QString, Leaf> m_leafCache;
};
