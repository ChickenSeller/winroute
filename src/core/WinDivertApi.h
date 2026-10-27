#pragma once

#include <QString>

#include <windows.h>

// windivert.h declares the entry points as __declspec(dllimport); we never call
// them directly (we resolve them from the DLL at run time, so WinRoute still
// runs when WinDivert.dll is absent), only use its structs, enums and macros.
#include "windivert.h"

// Under MinGW, windivert.h #defines the SAL annotations (__in/__out/...) to
// nothing. Those names are also used as ordinary identifiers inside libstdc++
// headers (e.g. <bits/chrono_io.h>), so leaving them defined breaks any standard
// header included afterwards. Undefine them again now that windivert.h is parsed.
#undef __in
#undef __in_opt
#undef __out
#undef __out_opt
#undef __inout
#undef __inout_opt

// Thin run-time loader for WinDivert.dll. The DLL is an optional backend, so it
// is opened with LoadLibrary rather than linked: available() is false when it or
// its driver is missing, and the NetFilter backend keeps working.
class WinDivertApi {
public:
    // Loads WinDivert.dll from next to the executable (falling back to the
    // search path). Returns the shared, lazily-loaded instance, or nullptr when
    // the DLL cannot be found or is missing an entry point.
    static const WinDivertApi *instance();

    // The DLL was found and all entry points resolved.
    bool loaded() const { return m_loaded; }

    using OpenFn = HANDLE(__stdcall *)(const char *, WINDIVERT_LAYER, INT16, UINT64);
    using RecvFn = BOOL(__stdcall *)(HANDLE, void *, UINT, UINT *, WINDIVERT_ADDRESS *);
    using SendFn = BOOL(__stdcall *)(HANDLE, const void *, UINT, UINT *, const WINDIVERT_ADDRESS *);
    using ShutdownFn = BOOL(__stdcall *)(HANDLE, WINDIVERT_SHUTDOWN);
    using CloseFn = BOOL(__stdcall *)(HANDLE);
    using SetParamFn = BOOL(__stdcall *)(HANDLE, WINDIVERT_PARAM, UINT64);
    using GetParamFn = BOOL(__stdcall *)(HANDLE, WINDIVERT_PARAM, UINT64 *);

    OpenFn open = nullptr;
    RecvFn recv = nullptr;
    SendFn send = nullptr;
    ShutdownFn shutdown = nullptr;
    CloseFn close = nullptr;
    SetParamFn setParam = nullptr;
    GetParamFn getParam = nullptr;

    // Absolute path of the WinDivert64.sys that ships next to the DLL, or {} if
    // it is not there. Used for the driver's version and signature in the UI.
    static QString driverPath();

private:
    WinDivertApi();
    bool m_loaded = false;
    HMODULE m_module = nullptr;
};
