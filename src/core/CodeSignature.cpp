#include "CodeSignature.h"

#include <QVarLengthArray>

#include <windows.h>
#include <wincrypt.h>
#include <winver.h>

#include <cstring>

namespace codesign {
namespace {

#ifndef szOID_NESTED_SIGNATURE
#define szOID_NESTED_SIGNATURE "1.3.6.1.4.1.311.2.4.1"
#endif

Cert certInfo(PCCERT_CONTEXT cert)
{
    Cert dc;
    wchar_t name[512];
    if (CertGetNameStringW(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, name, 512) > 1)
        dc.name = QString::fromWCharArray(name);
    if (CertGetNameStringW(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG, nullptr, name, 512) > 1)
        dc.issuer = QString::fromWCharArray(name);
    const DWORD want = CertNameToStrW(cert->dwCertEncodingType, &cert->pCertInfo->Subject,
                                      CERT_X500_NAME_STR | CERT_NAME_STR_CRLF_FLAG, nullptr, 0);
    if (want > 1) {
        const int sz = int(want);
        QVarLengthArray<wchar_t> s(sz);
        CertNameToStrW(cert->dwCertEncodingType, &cert->pCertInfo->Subject,
                       CERT_X500_NAME_STR | CERT_NAME_STR_CRLF_FLAG, s.data(), want);
        dc.subject = QString::fromWCharArray(s.data()).trimmed();
    }
    return dc;
}

// Appends the signer of this PKCS#7 message, then recurses into any nested
// signatures (dual-signed drivers carry the second signature here).
void collectSigners(HCRYPTMSG msg, HCERTSTORE store, QList<Cert> *out)
{
    DWORD n = 0;
    if (CryptMsgGetParam(msg, CMSG_SIGNER_CERT_INFO_PARAM, 0, nullptr, &n) && n) {
        QByteArray si(int(n), Qt::Uninitialized);
        if (CryptMsgGetParam(msg, CMSG_SIGNER_CERT_INFO_PARAM, 0, si.data(), &n)) {
            PCCERT_CONTEXT cert = CertFindCertificateInStore(store, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0,
                                                             CERT_FIND_SUBJECT_CERT,
                                                             reinterpret_cast<CERT_INFO *>(si.data()), nullptr);
            if (cert) {
                out->append(certInfo(cert));
                CertFreeCertificateContext(cert);
            }
        }
    }
    DWORD sn = 0;
    if (CryptMsgGetParam(msg, CMSG_SIGNER_INFO_PARAM, 0, nullptr, &sn) && sn) {
        QByteArray sb(int(sn), Qt::Uninitialized);
        if (CryptMsgGetParam(msg, CMSG_SIGNER_INFO_PARAM, 0, sb.data(), &sn)) {
            auto *info = reinterpret_cast<CMSG_SIGNER_INFO *>(sb.data());
            for (DWORD i = 0; i < info->UnauthAttrs.cAttr; ++i) {
                const CRYPT_ATTRIBUTE &a = info->UnauthAttrs.rgAttr[i];
                if (!a.pszObjId || strcmp(a.pszObjId, szOID_NESTED_SIGNATURE) != 0 || a.cValue == 0)
                    continue;
                HCRYPTMSG nmsg = CryptMsgOpenToDecode(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, 0, 0, nullptr, nullptr);
                if (nmsg && CryptMsgUpdate(nmsg, a.rgValue[0].pbData, a.rgValue[0].cbData, TRUE)) {
                    HCERTSTORE nstore = CertOpenStore(CERT_STORE_PROV_MSG, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                                      0, 0, nmsg);
                    if (nstore) {
                        collectSigners(nmsg, nstore, out);
                        CertCloseStore(nstore, 0);
                    }
                }
                if (nmsg)
                    CryptMsgClose(nmsg);
            }
        }
    }
}

} // namespace

QString fileVersion(const QString &path)
{
    const auto *wpath = reinterpret_cast<LPCWSTR>(path.utf16());
    DWORD handle = 0;
    const DWORD size = GetFileVersionInfoSizeW(wpath, &handle);
    if (!size)
        return {};
    QByteArray buf(int(size), Qt::Uninitialized);
    if (!GetFileVersionInfoW(wpath, 0, size, buf.data()))
        return {};
    VS_FIXEDFILEINFO *ffi = nullptr;
    UINT len = 0;
    if (!VerQueryValueW(buf.data(), L"\\", reinterpret_cast<LPVOID *>(&ffi), &len) || !ffi)
        return {};
    return QString("%1.%2.%3.%4")
        .arg(HIWORD(ffi->dwFileVersionMS))
        .arg(LOWORD(ffi->dwFileVersionMS))
        .arg(HIWORD(ffi->dwFileVersionLS))
        .arg(LOWORD(ffi->dwFileVersionLS));
}

QList<Cert> fileSigners(const QString &path)
{
    QList<Cert> out;
    const auto *wpath = reinterpret_cast<LPCWSTR>(path.utf16());
    HCERTSTORE store = nullptr;
    HCRYPTMSG msg = nullptr;
    if (CryptQueryObject(CERT_QUERY_OBJECT_FILE, wpath, CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
                         CERT_QUERY_FORMAT_FLAG_BINARY, 0, nullptr, nullptr, nullptr, &store, &msg, nullptr)) {
        collectSigners(msg, store, &out);
        if (store)
            CertCloseStore(store, 0);
        if (msg)
            CryptMsgClose(msg);
    }
    return out;
}

} // namespace codesign
