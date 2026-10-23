#include "NetFilterApi.h"

#include <QCoreApplication>
#include <QDir>

#include <type_traits>

const NetFilterApi *NetFilterApi::load(QString *error)
{
    static NetFilterApi api;
    static bool loaded = false;
    if (loaded)
        return &api;

    const QString path = QDir::toNativeSeparators(QDir(QCoreApplication::applicationDirPath()).filePath("nfapi.dll"));
    HMODULE h = LoadLibraryW(reinterpret_cast<LPCWSTR>(path.utf16()));
    if (!h) {
        if (error)
            *error = QCoreApplication::translate("Interceptor", "Cannot load %1 (error %2)").arg(path).arg(GetLastError());
        return nullptr;
    }
    QStringList missing;
    auto get = [&](auto &fn, const char *name) {
        fn = reinterpret_cast<std::remove_reference_t<decltype(fn)>>(reinterpret_cast<void *>(GetProcAddress(h, name)));
        if (!fn)
            missing << name;
    };
    get(api.init, "nf_init");
    get(api.free, "nf_free");
    get(api.registerDriver, "nf_registerDriver");
    get(api.unRegisterDriver, "nf_unRegisterDriver");
    get(api.setOptions, "nf_setOptions");
    get(api.addRule, "nf_addRule");
    get(api.deleteRules, "nf_deleteRules");
    get(api.tcpClose, "nf_tcpClose");
    get(api.tcpPostSend, "nf_tcpPostSend");
    get(api.tcpPostReceive, "nf_tcpPostReceive");
    get(api.udpPostSend, "nf_udpPostSend");
    get(api.udpPostReceive, "nf_udpPostReceive");
    get(api.getProcessNameFromKernel, "nf_getProcessNameFromKernel");
    get(api.adjustProcessPriviledges, "nf_adjustProcessPriviledges");
    get(api.getDriverType, "nf_getDriverType");
    if (!missing.isEmpty()) {
        if (error)
            *error = QCoreApplication::translate("Interceptor", "nfapi.dll is missing exports: ") + missing.join(", ") + QCoreApplication::translate("Interceptor", " (the C API build is required)");
        return nullptr;
    }
    loaded = true;
    return &api;
}

QString nfStatusString(NF_STATUS s)
{
    switch (s) {
    case NF_STATUS_SUCCESS: return QCoreApplication::translate("Interceptor", "success");
    case NF_STATUS_FAIL: return QCoreApplication::translate("Interceptor", "failed to open the driver");
    case NF_STATUS_INVALID_ENDPOINT_ID: return QCoreApplication::translate("Interceptor", "invalid endpoint id");
    case NF_STATUS_NOT_INITIALIZED: return QCoreApplication::translate("Interceptor", "not initialized");
    case NF_STATUS_IO_ERROR: return QCoreApplication::translate("Interceptor", "driver I/O error");
    case NF_STATUS_REBOOT_REQUIRED: return QCoreApplication::translate("Interceptor", "reboot required");
    }
    return QCoreApplication::translate("Interceptor", "status %1").arg(int(s));
}
