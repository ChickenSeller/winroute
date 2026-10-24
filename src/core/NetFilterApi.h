#pragma once

#include <QString>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

// The NetFilter SDK C API; declarations only, nfapi.dll is loaded at run time
// so the application can start and report a clear error without it.
#define _C_API
#define _NFAPI_STATIC_LIB
#include <nfapi.h>

struct NetFilterApi {
    decltype(&::nf_init) init = nullptr;
    decltype(&::nf_free) free = nullptr;
    decltype(&::nf_registerDriver) registerDriver = nullptr;
    decltype(&::nf_unRegisterDriver) unRegisterDriver = nullptr;
    decltype(&::nf_setOptions) setOptions = nullptr;
    decltype(&::nf_addRule) addRule = nullptr;
    decltype(&::nf_deleteRules) deleteRules = nullptr;
    decltype(&::nf_tcpClose) tcpClose = nullptr;
    decltype(&::nf_tcpPostSend) tcpPostSend = nullptr;
    decltype(&::nf_tcpPostReceive) tcpPostReceive = nullptr;
    decltype(&::nf_udpPostSend) udpPostSend = nullptr;
    decltype(&::nf_udpPostReceive) udpPostReceive = nullptr;
    decltype(&::nf_getUDPConnInfo) getUDPConnInfo = nullptr;
    decltype(&::nf_getProcessNameFromKernel) getProcessNameFromKernel = nullptr;
    decltype(&::nf_adjustProcessPriviledges) adjustProcessPriviledges = nullptr;
    decltype(&::nf_getDriverType) getDriverType = nullptr;

    static const NetFilterApi *load(QString *error);
};

QString nfStatusString(NF_STATUS s);
