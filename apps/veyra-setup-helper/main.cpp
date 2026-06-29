// VeyraSetupHelper.exe — Native installer helper
// Called silently by the NSIS installer (nsExec::ExecToLog, no console window shown).
//
// Usage:
//   VeyraSetupHelper.exe [--log <path>] --list-devices <outfile>
//   VeyraSetupHelper.exe [--log <path>] --associate <endpoint-guid>
//   VeyraSetupHelper.exe [--log <path>] --unassociate-all
//   VeyraSetupHelper.exe [--log <path>] --verify-com
//   VeyraSetupHelper.exe [--log <path>] --restart-audio
//
// Exit codes: 0 = success, non-zero = failure.
//
// Compiled with /MT (static CRT) — no VC++ redistributable required.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmdeviceapi.h>
#include <propsys.h>
#include <strsafe.h>

#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "propsys.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "advapi32.lib")

// =============================================================================
//  Veyra APO constants
// =============================================================================

static const wchar_t kRenderClsid[]  = L"{7E9C2B14-3F6A-4D8E-9B21-5C0A1F2E3D44}";
static const wchar_t kCaptureClsid[] = L"{B2D4E6F8-1A3C-4E5D-8F09-2B4C6D8E0A12}";

// PKEY_FX_PostMixEffectClsid  {D04E05A6-594B-4FB6-A80D-01AF5EEC11D9},6
// PKEY_FX_PreMixEffectClsid   {D04E05A6-594B-4FB6-A80D-01AF5EEC11D9},4
static const wchar_t kFxPostMixKey[] = L"{D04E05A6-594B-4FB6-A80D-01AF5EEC11D9},6";
static const wchar_t kFxPreMixKey[]  = L"{D04E05A6-594B-4FB6-A80D-01AF5EEC11D9},4";

static const wchar_t kRenderRegBase[] =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\MMDevices\\Audio\\Render";
static const wchar_t kCaptureRegBase[] =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\MMDevices\\Audio\\Capture";

// PKEY_Device_FriendlyName = {a45c254e-df1c-4efd-8020-67d146a850e0}, 14
static const PROPERTYKEY kPKEY_Device_FriendlyName = {
    {0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}}, 14
};

// =============================================================================
//  Logging — written to stdout (captured by nsExec) and optionally a file
// =============================================================================

static FILE* g_logFile = nullptr;

static void LogClose() {
    if (g_logFile) { fflush(g_logFile); fclose(g_logFile); g_logFile = nullptr; }
}

static void LogOpen(const wchar_t* path) {
    if (!path || path[0] == L'\0') return;
    _wfopen_s(&g_logFile, path, L"a");  // append, ANSI — log is installer-internal
}

static void Log(const wchar_t* fmt, ...) {
    SYSTEMTIME st = {};
    GetLocalTime(&st);

    wchar_t msg[1024] = {};
    va_list args;
    va_start(args, fmt);
    _vsnwprintf_s(msg, _countof(msg), _TRUNCATE, fmt, args);
    va_end(args);

    wchar_t line[1280] = {};
    _snwprintf_s(line, _countof(line), _TRUNCATE,
        L"[%04d-%02d-%02d %02d:%02d:%02d] [VeyraSetupHelper] %s\n",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond,
        msg);

    wprintf(L"%s", line);
    if (g_logFile) fwprintf(g_logFile, L"%s", line);
}

// =============================================================================
//  Audio service restart
// =============================================================================

static bool ServiceOp(SC_HANDLE scm, const wchar_t* svcName, DWORD desiredAccess,
                      bool start, DWORD& outLastErr) {
    SC_HANDLE svc = OpenServiceW(scm, svcName, desiredAccess);
    if (!svc) { outLastErr = GetLastError(); return false; }

    bool ok = false;
    if (start) {
        ok = StartServiceW(svc, 0, nullptr) != FALSE;
        outLastErr = GetLastError();
        if (!ok && outLastErr == ERROR_SERVICE_ALREADY_RUNNING) ok = true;
    } else {
        SERVICE_STATUS ss = {};
        ok = ControlService(svc, SERVICE_CONTROL_STOP, &ss) != FALSE;
        outLastErr = GetLastError();
        // ERROR_SERVICE_NOT_ACTIVE is fine — it was already stopped
        if (!ok && outLastErr == ERROR_SERVICE_NOT_ACTIVE) ok = true;
    }
    CloseServiceHandle(svc);
    return ok;
}

static bool RestartAudioService() {
    Log(L"Restarting Windows Audio...");
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        Log(L"OpenSCManager failed: %lu", GetLastError());
        return false;
    }

    DWORD err = 0;
    ServiceOp(scm, L"AudioSrv",             SERVICE_STOP,  false, err);
    ServiceOp(scm, L"AudioEndpointBuilder", SERVICE_STOP,  false, err);
    Sleep(900);
    ServiceOp(scm, L"AudioEndpointBuilder", SERVICE_START, true,  err);
    Sleep(400);
    bool ok = ServiceOp(scm, L"AudioSrv",   SERVICE_START, true,  err);

    CloseServiceHandle(scm);
    if (ok) Log(L"AudioSrv restarted.");
    else    Log(L"AudioSrv start returned: %lu", err);
    return ok;
}

// =============================================================================
//  --list-devices
//  Enumerate active render endpoints via IMMDeviceEnumerator.
//  Writes a UTF-16 LE INI file that NSIS ReadINIStr can read.
// =============================================================================

struct Endpoint { std::wstring guid; std::wstring name; bool isDefault; };

static std::vector<Endpoint> EnumRenderEndpoints() {
    std::vector<Endpoint> eps;

    IMMDeviceEnumerator* pEnum = nullptr;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                  CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                  reinterpret_cast<void**>(&pEnum));
    if (FAILED(hr)) { Log(L"CoCreateInstance MMDeviceEnumerator: 0x%08X", hr); return eps; }

    // Default device ID for pre-selection
    IMMDevice* pDef = nullptr;
    wchar_t* defaultId = nullptr;
    pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDef);
    if (pDef) { pDef->GetId(&defaultId); pDef->Release(); }

    IMMDeviceCollection* pColl = nullptr;
    hr = pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pColl);
    if (SUCCEEDED(hr) && pColl) {
        UINT count = 0;
        pColl->GetCount(&count);
        for (UINT i = 0; i < count; ++i) {
            IMMDevice* pDev = nullptr;
            if (FAILED(pColl->Item(i, &pDev))) continue;

            wchar_t* pwId = nullptr;
            pDev->GetId(&pwId);

            // Friendly name from IPropertyStore
            std::wstring name;
            IPropertyStore* pProps = nullptr;
            if (SUCCEEDED(pDev->OpenPropertyStore(STGM_READ, &pProps))) {
                PROPVARIANT pv = {};
                if (SUCCEEDED(pProps->GetValue(kPKEY_Device_FriendlyName, &pv))) {
                    if (pv.vt == VT_LPWSTR && pv.pwszVal) name = pv.pwszVal;
                    PropVariantClear(&pv);
                }
                pProps->Release();
            }
            if (name.empty() && pwId) name = pwId;

            // Extract the endpoint GUID from the device ID.
            // MMDevice ID format: {0.0.0.00000000}.{GUID}
            // The registry key name is just the {GUID} portion.
            std::wstring guid;
            if (pwId) {
                std::wstring id = pwId;
                auto open  = id.rfind(L'{');
                auto close = id.rfind(L'}');
                if (open  != std::wstring::npos &&
                    close != std::wstring::npos &&
                    close > open)
                    guid = id.substr(open, close - open + 1);
            }

            bool isDef = defaultId && pwId && _wcsicmp(defaultId, pwId) == 0;

            if (!guid.empty()) eps.push_back({guid, name, isDef});

            if (pwId) CoTaskMemFree(pwId);
            pDev->Release();
        }
        pColl->Release();
    }

    if (defaultId) CoTaskMemFree(defaultId);
    pEnum->Release();
    return eps;
}

static int CmdListDevices(const wchar_t* outFile) {
    if (!outFile || outFile[0] == L'\0') {
        Log(L"--list-devices: output file path required");
        return 1;
    }

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool coinited = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;

    auto eps = EnumRenderEndpoints();
    Log(L"Found %d active render endpoint(s).", static_cast<int>(eps.size()));

    if (coinited && SUCCEEDED(hr)) CoUninitialize();

    // Find default index
    int defIdx = 0;
    for (int i = 0; i < static_cast<int>(eps.size()); ++i)
        if (eps[i].isDefault) { defIdx = i; break; }

    // Write via WritePrivateProfileStringW (creates UTF-16 LE BOM file on Win10+,
    // which NSIS 3.x ReadINIStr / GetPrivateProfileString reads correctly).
    DeleteFileW(outFile);  // start fresh

    auto WriteIni = [&](const wchar_t* section, const wchar_t* key, const wchar_t* val) {
        WritePrivateProfileStringW(section, key, val, outFile);
    };

    wchar_t buf[64] = {};
    _snwprintf_s(buf, _countof(buf), _TRUNCATE, L"%d", static_cast<int>(eps.size()));
    WriteIni(L"Devices", L"Count", buf);
    _snwprintf_s(buf, _countof(buf), _TRUNCATE, L"%d", defIdx);
    WriteIni(L"Devices", L"Default", buf);

    for (int i = 0; i < static_cast<int>(eps.size()); ++i) {
        wchar_t key[32] = {};
        _snwprintf_s(key, _countof(key), _TRUNCATE, L"Name%d", i);
        WriteIni(L"Devices", key, eps[i].name.c_str());
        _snwprintf_s(key, _countof(key), _TRUNCATE, L"Guid%d", i);
        WriteIni(L"Devices", key, eps[i].guid.c_str());
    }

    Log(L"Device list written to: %s", outFile);
    return 0;
}

// =============================================================================
//  --associate <guid>
//  Write PKEY_FX_PostMixEffectClsid to the endpoint's FxProperties.
// =============================================================================

static int CmdAssociate(const wchar_t* guid) {
    if (!guid || guid[0] == L'\0') {
        Log(L"--associate: endpoint GUID required");
        return 1;
    }

    wchar_t fxPath[512] = {};
    _snwprintf_s(fxPath, _countof(fxPath), _TRUNCATE,
        L"%s\\%s\\FxProperties", kRenderRegBase, guid);

    HKEY hFx = nullptr;
    DWORD disposition = 0;
    LONG err = RegCreateKeyExW(HKEY_LOCAL_MACHINE, fxPath, 0, nullptr,
        REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &hFx, &disposition);
    if (err != ERROR_SUCCESS) {
        Log(L"RegCreateKeyEx '%s' failed: %ld", fxPath, err);
        return 1;
    }

    DWORD cbVal = static_cast<DWORD>((wcslen(kRenderClsid) + 1) * sizeof(wchar_t));
    LONG r = RegSetValueExW(hFx, kFxPostMixKey, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(kRenderClsid), cbVal);
    RegCloseKey(hFx);

    if (r != ERROR_SUCCESS) {
        Log(L"RegSetValueEx failed: %ld", r);
        return 1;
    }

    Log(L"APO associated with endpoint: %s", guid);
    RestartAudioService();
    return 0;
}

// =============================================================================
//  --unassociate-all
//  Scan every render and capture endpoint; remove Veyra CLSIDs from FxProperties.
// =============================================================================

static int CmdUnassociateAll() {
    const wchar_t* flows[] = { kRenderRegBase, kCaptureRegBase };
    const wchar_t* ourClsids[] = { kRenderClsid, kCaptureClsid };
    const wchar_t* fxKeys[]    = { kFxPostMixKey, kFxPreMixKey };

    int removed = 0;

    for (const wchar_t* base : flows) {
        HKEY hBase = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, base, 0, KEY_READ, &hBase) != ERROR_SUCCESS)
            continue;

        wchar_t guidName[128] = {};
        for (DWORD idx = 0; ; ++idx) {
            DWORD cch = _countof(guidName);
            if (RegEnumKeyExW(hBase, idx, guidName, &cch,
                    nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS)
                break;

            wchar_t fxPath[512] = {};
            _snwprintf_s(fxPath, _countof(fxPath), _TRUNCATE,
                L"%s\\%s\\FxProperties", base, guidName);

            HKEY hFx = nullptr;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, fxPath, 0,
                    KEY_QUERY_VALUE | KEY_SET_VALUE, &hFx) != ERROR_SUCCESS)
                continue;

            for (const wchar_t* key : fxKeys) {
                wchar_t val[256] = {};
                DWORD cbVal = sizeof(val);
                DWORD type  = REG_SZ;
                if (RegQueryValueExW(hFx, key, nullptr, &type,
                        reinterpret_cast<LPBYTE>(val), &cbVal) != ERROR_SUCCESS)
                    continue;

                bool isOurs = false;
                for (const wchar_t* clsid : ourClsids)
                    if (_wcsicmp(val, clsid) == 0) { isOurs = true; break; }

                if (isOurs) {
                    RegDeleteValueW(hFx, key);
                    Log(L"Removed %s from endpoint %s", key, guidName);
                    ++removed;
                }
            }
            RegCloseKey(hFx);
        }
        RegCloseKey(hBase);
    }

    Log(L"Removed APO from %d endpoint(s).", removed);
    if (removed > 0) RestartAudioService();
    return 0;
}

// =============================================================================
//  --verify-com
//  Exit 0 if the render CLSID is registered and the DLL file exists.
// =============================================================================

static int CmdVerifyCom() {
    wchar_t regPath[256] = {};
    _snwprintf_s(regPath, _countof(regPath), _TRUNCATE,
        L"SOFTWARE\\Classes\\CLSID\\%s\\InprocServer32", kRenderClsid);

    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        Log(L"verify-com: CLSID not found: %s", kRenderClsid);
        return 1;
    }

    wchar_t dll[MAX_PATH] = {};
    DWORD cbDll = sizeof(dll);
    LONG r = RegQueryValueExW(hKey, nullptr, nullptr, nullptr,
        reinterpret_cast<LPBYTE>(dll), &cbDll);
    RegCloseKey(hKey);

    if (r != ERROR_SUCCESS || dll[0] == L'\0') {
        Log(L"verify-com: InprocServer32 default value missing");
        return 1;
    }

    if (GetFileAttributesW(dll) == INVALID_FILE_ATTRIBUTES) {
        Log(L"verify-com: DLL not found on disk: %s", dll);
        return 1;
    }

    Log(L"verify-com: OK — %s", dll);
    return 0;
}

// =============================================================================
//  Entry point
// =============================================================================

int wmain(int argc, wchar_t* argv[]) {
    const wchar_t* logPath = nullptr;
    const wchar_t* action  = nullptr;
    const wchar_t* arg1    = nullptr;

    for (int i = 1; i < argc; ++i) {
        if (wcscmp(argv[i], L"--log") == 0 && i + 1 < argc) {
            logPath = argv[++i];
        } else if (argv[i][0] == L'-' && argv[i][1] == L'-' && !action) {
            action = argv[i];
        } else if (action && !arg1) {
            arg1 = argv[i];
        }
    }

    LogOpen(logPath);

    if (!action) {
        Log(L"VeyraSetupHelper.exe [--log <path>] --<action> [arg]");
        Log(L"  --list-devices <outfile>   Enumerate active render endpoints -> INI");
        Log(L"  --associate <guid>         Write APO CLSID to endpoint FxProperties");
        Log(L"  --unassociate-all          Remove Veyra CLSIDs from all endpoints");
        Log(L"  --verify-com               Exit 0 if COM CLSID registered + DLL present");
        Log(L"  --restart-audio            Restart AudioSrv and AudioEndpointBuilder");
        LogClose();
        return 1;
    }

    int result = 1;
    if      (wcscmp(action, L"--list-devices")   == 0) result = CmdListDevices(arg1);
    else if (wcscmp(action, L"--associate")       == 0) result = CmdAssociate(arg1);
    else if (wcscmp(action, L"--unassociate-all") == 0) result = CmdUnassociateAll();
    else if (wcscmp(action, L"--verify-com")      == 0) result = CmdVerifyCom();
    else if (wcscmp(action, L"--restart-audio")   == 0) result = RestartAudioService() ? 0 : 1;
    else { Log(L"Unknown action: %s", action); result = 1; }

    LogClose();
    return result;
}
