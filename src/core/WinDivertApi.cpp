#include "WinDivertApi.h"

#include <QCoreApplication>
#include <QFileInfo>

namespace {

// WinDivert.dll must be loaded from next to the executable, where the build and
// the installer place it; LoadLibrary's default search would also accept a DLL
// planted in the working directory, which we avoid for a driver-loading library.
QString dllPath()
{
    const QString dir = QFileInfo(QCoreApplication::applicationFilePath()).absolutePath();
    return dir + "/WinDivert.dll";
}

} // namespace

WinDivertApi::WinDivertApi()
{
    const QString path = dllPath();
    if (!QFileInfo::exists(path))
        return;
    m_module = LoadLibraryW(reinterpret_cast<const wchar_t *>(path.utf16()));
    if (!m_module)
        return;

    open = reinterpret_cast<OpenFn>(GetProcAddress(m_module, "WinDivertOpen"));
    recv = reinterpret_cast<RecvFn>(GetProcAddress(m_module, "WinDivertRecv"));
    send = reinterpret_cast<SendFn>(GetProcAddress(m_module, "WinDivertSend"));
    shutdown = reinterpret_cast<ShutdownFn>(GetProcAddress(m_module, "WinDivertShutdown"));
    close = reinterpret_cast<CloseFn>(GetProcAddress(m_module, "WinDivertClose"));
    setParam = reinterpret_cast<SetParamFn>(GetProcAddress(m_module, "WinDivertSetParam"));
    getParam = reinterpret_cast<GetParamFn>(GetProcAddress(m_module, "WinDivertGetParam"));

    m_loaded = open && recv && send && shutdown && close && setParam && getParam;
}

const WinDivertApi *WinDivertApi::instance()
{
    static WinDivertApi api;
    return &api;
}

QString WinDivertApi::driverPath()
{
    const QString dir = QFileInfo(QCoreApplication::applicationFilePath()).absolutePath();
    const QString sys = dir + "/WinDivert64.sys";
    return QFileInfo::exists(sys) ? sys : QString();
}
