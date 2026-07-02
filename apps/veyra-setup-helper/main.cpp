// VeyraSetupHelper.exe — Native installer helper
// Called silently by the NSIS installer (nsExec::ExecToLog, no console window shown).
//
// Usage:
//   VeyraSetupHelper.exe [--log <path>] --list-devices <outfile>
//   VeyraSetupHelper.exe [--log <path>] --associate <endpoint-guid>
//   VeyraSetupHelper.exe [--log <path>] --unassociate-all
//   VeyraSetupHelper.exe [--log <path>] --verify-com
//   VeyraSetupHelper.exe [--log <path>] --verify-association <endpoint-guid>
//   VeyraSetupHelper.exe [--log <path>] --verify-service
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

static const wchar_t kServiceName[]  = L"VeyraAudioService";

static const wchar_t kRenderClsid[]  = L"{7E9C2B14-3F6A-4D8E-9B21-5C0A1F2E3D44}";
static const wchar_t kCaptureClsid[] = L"{B2D4E6F8-1A3C-4E5D-8F09-2B4C6D8E0A12}";

// Windows 10 20H1+ / SDK 22621 property keys (audioenginebaseapo.h lines 1449-1456).
// FMTID {D04E05A6-594B-4FB6-A80D-01AF5EED7D1D} — note ED7D1D, NOT the old EC11D9.
// PID 6 = PKEY_FX_ModeEffectClsid — audiodg reads this for ALL endpoint types including
//         Bluetooth A2DP. PID 7 EndpointEffectClsid is silently skipped on A2DP Bluetooth
//         because the Bluetooth stack only declares Stream (PID 5) + Mode (PID 6) support.
// PID 1 = PKEY_FX_PreMixEffectClsid.
static const wchar_t kFxPostMixKey[] = L"{D04E05A6-594B-4FB6-A80D-01AF5EED7D1D},6";
static const wchar_t kFxPreMixKey[]  = L"{D04E05A6-594B-4FB6-A80D-01AF5EED7D1D},1";

// Where we back up the pre-existing ModeEffect CLSID so --unassociate-all can restore it.
static const wchar_t kVeyraDevicesKey[] = L"SOFTWARE\\Veyra\\Devices";
static const wchar_t kOriginalModeVal[] = L"OriginalModeEffect";

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
//  Audio service helpers
// =============================================================================

// Wait up to timeoutMs for the service to reach targetState. Returns true if
// reached, false on timeout or query failure.
static bool WaitForServiceState(SC_HANDLE svc, DWORD targetState, DWORD timeoutMs) {
    const DWORD interval = 200;
    DWORD elapsed = 0;
    SERVICE_STATUS ss = {};
    while (elapsed <= timeoutMs) {
        if (!QueryServiceStatus(svc, &ss)) return false;
        if (ss.dwCurrentState == targetState) return true;
        Sleep(interval);
        elapsed += interval;
    }
    return false;
}

static bool ServiceOp(SC_HANDLE scm, const wchar_t* svcName, DWORD desiredAccess,
                      bool start, DWORD& outLastErr) {
    SC_HANDLE svc = OpenServiceW(scm, svcName, desiredAccess);
    if (!svc) { outLastErr = GetLastError(); return false; }

    bool ok = false;
    if (start) {
        ok = StartServiceW(svc, 0, nullptr) != FALSE;
        outLastErr = GetLastError();
        if (!ok && outLastErr == ERROR_SERVICE_ALREADY_RUNNING) ok = true;
        // Give it a moment to reach RUNNING
        if (ok) WaitForServiceState(svc, SERVICE_RUNNING, 5000);
    } else {
        SERVICE_STATUS ss = {};
        ok = ControlService(svc, SERVICE_CONTROL_STOP, &ss) != FALSE;
        outLastErr = GetLastError();
        // ERROR_SERVICE_NOT_ACTIVE is fine — it was already stopped
        if (!ok && outLastErr == ERROR_SERVICE_NOT_ACTIVE) ok = true;
        // Wait for STOPPED before returning so the caller can safely restart
        if (ok) WaitForServiceState(svc, SERVICE_STOPPED, 8000);
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
    // Stop AudioSrv first (it depends on AudioEndpointBuilder; stopping it
    // causes the endpoint builder to follow). Wait for STOPPED before proceeding.
    ServiceOp(scm, L"AudioSrv",             SERVICE_STOP | SERVICE_QUERY_STATUS,  false, err);
    ServiceOp(scm, L"AudioEndpointBuilder", SERVICE_STOP | SERVICE_QUERY_STATUS,  false, err);

    // Restart AudioEndpointBuilder first (AudioSrv depends on it).
    ServiceOp(scm, L"AudioEndpointBuilder", SERVICE_START | SERVICE_QUERY_STATUS, true,  err);
    bool ok = ServiceOp(scm, L"AudioSrv",   SERVICE_START | SERVICE_QUERY_STATUS, true,  err);

    CloseServiceHandle(scm);
    if (ok) Log(L"Windows Audio restarted successfully.");
    else    Log(L"AudioSrv start failed: Win32 error %lu", err);
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
//  Primary: IPropertyStore via IMMDevice::OpenPropertyStore (correct PROPVARIANT
//  binary format). Fallback: direct REG_SZ write (works on older Windows 10).
//  Both paths are verified by reading the value back before returning.
// =============================================================================

static int CmdAssociate(const wchar_t* guid) {
    if (!guid || guid[0] == L'\0') {
        Log(L"--associate: endpoint GUID required");
        return 1;
    }

    Log(L"Associating APO with endpoint: %s", guid);

    // Build the FxProperties registry path for direct access
    wchar_t fxPath[512] = {};
    _snwprintf_s(fxPath, _countof(fxPath), _TRUNCATE,
        L"%s\\%s\\FxProperties", kRenderRegBase, guid);
    Log(L"  FxProperties path: HKLM\\%s", fxPath);

    // Read and save the current PID 6 occupant BEFORE IPropertyStore overwrites it.
    // For ACTIVE Bluetooth endpoints, IPropertyStore.Commit() writes directly to the
    // physical registry, so the backup must happen first.
    {
        HKEY hFxRead = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, fxPath, 0, KEY_QUERY_VALUE, &hFxRead) == ERROR_SUCCESS) {
            wchar_t existingVal[256] = {};
            DWORD cbExisting = sizeof(existingVal);
            DWORD typeExisting = 0;
            LONG re = RegQueryValueExW(hFxRead, kFxPostMixKey, nullptr, &typeExisting,
                reinterpret_cast<LPBYTE>(existingVal), &cbExisting);
            RegCloseKey(hFxRead);
            if (re == ERROR_SUCCESS && typeExisting == REG_SZ
                && existingVal[0] != L'\0'
                && _wcsicmp(existingVal, kRenderClsid) != 0)
            {
                wchar_t backupPath[512] = {};
                _snwprintf_s(backupPath, _countof(backupPath), _TRUNCATE,
                    L"%s\\%s", kVeyraDevicesKey, guid);
                HKEY hSave = nullptr;
                DWORD disp = 0;
                if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, backupPath, 0, nullptr,
                        REG_OPTION_NON_VOLATILE, KEY_SET_VALUE,
                        nullptr, &hSave, &disp) == ERROR_SUCCESS) {
                    DWORD cbSave = static_cast<DWORD>((wcslen(existingVal) + 1) * sizeof(wchar_t));
                    RegSetValueExW(hSave, kOriginalModeVal, 0, REG_SZ,
                        reinterpret_cast<const BYTE*>(existingVal), cbSave);
                    RegCloseKey(hSave);
                    Log(L"  Saved original ModeEffect '%s' — will restore on uninstall.", existingVal);
                }
            }
        }
    }

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool comOk = SUCCEEDED(hr);

    // Primary path: IPropertyStore (writes correct PROPVARIANT binary format)
    if (comOk) {
        IMMDeviceEnumerator* pEnum = nullptr;
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                              CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                              reinterpret_cast<void**>(&pEnum));
        if (SUCCEEDED(hr)) {
            // Device ID format: "{0.0.0.00000000}.{GUID}"
            std::wstring deviceId = std::wstring(L"{0.0.0.00000000}.") + guid;
            IMMDevice* pDevice = nullptr;
            hr = pEnum->GetDevice(deviceId.c_str(), &pDevice);
            pEnum->Release();

            if (SUCCEEDED(hr)) {
                IPropertyStore* pStore = nullptr;
                hr = pDevice->OpenPropertyStore(STGM_READWRITE, &pStore);
                pDevice->Release();

                if (SUCCEEDED(hr)) {
                    static const PROPERTYKEY kFxPostMix = {
                        {0xD04E05A6, 0x594B, 0x4FB6,
                         {0xA8, 0x0D, 0x01, 0xAF, 0x5E, 0xED, 0x7D, 0x1D}}, 6};

                    PROPVARIANT pv;
                    PropVariantInit(&pv);
                    pv.vt      = VT_LPWSTR;
                    pv.pwszVal = const_cast<LPWSTR>(kRenderClsid);

                    hr = pStore->SetValue(kFxPostMix, pv);
                    if (SUCCEEDED(hr)) hr = pStore->Commit();
                    pStore->Release();

                    if (SUCCEEDED(hr)) {
                        Log(L"  IPropertyStore Commit: S_OK (Bluetooth may be volatile "
                            L"in-process only — writing directly to registry as well).");
                    } else {
                        Log(L"  IPropertyStore SetValue/Commit: 0x%08X", hr);
                    }
                } else {
                    Log(L"  OpenPropertyStore(STGM_READWRITE): 0x%08X — "
                        L"falling back to direct registry write", hr);
                }
            } else {
                Log(L"  GetDevice '%s': 0x%08X — falling back", deviceId.c_str(), hr);
            }
        } else {
            Log(L"  CoCreateInstance MMDeviceEnumerator: 0x%08X — falling back", hr);
        }
    }

    if (comOk) CoUninitialize();

    // Always write directly to the registry as REG_SZ.
    // IPropertyStore may create a volatile in-process key on Bluetooth endpoints
    // that evaporates when this process exits. The direct write is the only path
    // that survives to be verified by a separate --verify-association process.
    {
        HKEY hFx = nullptr;
        DWORD disposition = 0;
        LONG err = RegCreateKeyExW(HKEY_LOCAL_MACHINE, fxPath, 0, nullptr,
            REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &hFx, &disposition);
        if (err != ERROR_SUCCESS) {
            Log(L"  RegCreateKeyEx '%s' failed: Win32 %ld", fxPath, err);
            return 1;
        }

        DWORD cbVal = static_cast<DWORD>((wcslen(kRenderClsid) + 1) * sizeof(wchar_t));
        LONG r = RegSetValueExW(hFx, kFxPostMixKey, 0, REG_SZ,
            reinterpret_cast<const BYTE*>(kRenderClsid), cbVal);
        RegCloseKey(hFx);

        if (r != ERROR_SUCCESS) {
            Log(L"  RegSetValueEx failed: Win32 %ld", r);
            return 1;
        }
        Log(L"  Direct REG_SZ registry write succeeded.");
    }

    // Readback verification: confirm the value is readable and matches.
    {
        HKEY hVerify = nullptr;
        LONG err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, fxPath, 0, KEY_QUERY_VALUE, &hVerify);
        if (err != ERROR_SUCCESS) {
            Log(L"  VERIFY FAIL: cannot open FxProperties for read: Win32 %ld", err);
            return 1;
        }

        wchar_t readBack[256] = {};
        DWORD cbRead = sizeof(readBack);
        DWORD type   = 0;
        err = RegQueryValueExW(hVerify, kFxPostMixKey, nullptr, &type,
            reinterpret_cast<LPBYTE>(readBack), &cbRead);
        RegCloseKey(hVerify);

        if (err != ERROR_SUCCESS) {
            Log(L"  VERIFY FAIL: cannot read back FxProperties: Win32 %ld", err);
            return 1;
        }

        // For REG_BINARY (PROPVARIANT), the string starts at byte offset 4 (after
        // the DWORD VARTYPE). For REG_SZ, it starts at byte 0.
        const wchar_t* strVal = readBack;
        if (type == REG_BINARY && cbRead >= 4 + sizeof(wchar_t)) {
            strVal = reinterpret_cast<const wchar_t*>(
                reinterpret_cast<const BYTE*>(readBack) + 4);
        }

        if (_wcsicmp(strVal, kRenderClsid) != 0) {
            Log(L"  VERIFY FAIL: readback value '%s' does not match expected '%s'",
                strVal, kRenderClsid);
            return 1;
        }

        Log(L"  VERIFY OK: FxProperties readback confirmed (type=%lu, value=%s)",
            type, strVal);
    }

    Log(L"APO associated with endpoint: %s", guid);
    RestartAudioService();
    return 0;
}

// =============================================================================
//  --unassociate-all
//  Scan every render and capture endpoint; remove Veyra CLSIDs from FxProperties.
//  Handles both REG_SZ and REG_BINARY (PROPVARIANT) value formats.
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
                BYTE raw[512] = {};
                DWORD cbVal = sizeof(raw);
                DWORD type  = 0;
                if (RegQueryValueExW(hFx, key, nullptr, &type,
                        raw, &cbVal) != ERROR_SUCCESS)
                    continue;

                // Extract the CLSID string from either REG_SZ or REG_BINARY
                // (PROPVARIANT where first DWORD is VARTYPE, string follows).
                const wchar_t* strVal = reinterpret_cast<const wchar_t*>(raw);
                if (type == REG_BINARY && cbVal >= 4 + sizeof(wchar_t))
                    strVal = reinterpret_cast<const wchar_t*>(raw + 4);
                else if (type != REG_SZ)
                    continue;

                bool isOurs = false;
                for (const wchar_t* clsid : ourClsids)
                    if (_wcsicmp(strVal, clsid) == 0) { isOurs = true; break; }

                if (isOurs) {
                    bool restored = false;
                    // For the ModeEffect slot on render endpoints, restore the original
                    // APO that was displaced during --associate. For everything else, delete.
                    if (wcscmp(key, kFxPostMixKey) == 0 && base == kRenderRegBase) {
                        wchar_t backupPath[512] = {};
                        _snwprintf_s(backupPath, _countof(backupPath), _TRUNCATE,
                            L"%s\\%s", kVeyraDevicesKey, guidName);
                        wchar_t origVal[256] = {};
                        DWORD cbOrig = sizeof(origVal);
                        DWORD typeOrig = 0;
                        HKEY hBackup = nullptr;
                        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, backupPath, 0,
                                KEY_QUERY_VALUE | KEY_SET_VALUE, &hBackup) == ERROR_SUCCESS) {
                            if (RegQueryValueExW(hBackup, kOriginalModeVal, nullptr,
                                    &typeOrig, reinterpret_cast<LPBYTE>(origVal),
                                    &cbOrig) == ERROR_SUCCESS
                                && typeOrig == REG_SZ && origVal[0] != L'\0')
                            {
                                DWORD cbRestore = static_cast<DWORD>(
                                    (wcslen(origVal) + 1) * sizeof(wchar_t));
                                if (RegSetValueExW(hFx, key, 0, REG_SZ,
                                        reinterpret_cast<const BYTE*>(origVal),
                                        cbRestore) == ERROR_SUCCESS)
                                {
                                    Log(L"Restored original ModeEffect '%s' to endpoint %s",
                                        origVal, guidName);
                                    restored = true;
                                }
                                RegDeleteValueW(hBackup, kOriginalModeVal);
                            }
                            RegCloseKey(hBackup);
                        }
                    }
                    if (!restored) {
                        RegDeleteValueW(hFx, key);
                        Log(L"Removed %s from endpoint %s", key, guidName);
                    }
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
        Log(L"verify-com FAIL: CLSID not registered at HKLM\\%s", regPath);
        return 1;
    }

    wchar_t dll[MAX_PATH] = {};
    DWORD cbDll = sizeof(dll);
    LONG r = RegQueryValueExW(hKey, nullptr, nullptr, nullptr,
        reinterpret_cast<LPBYTE>(dll), &cbDll);
    RegCloseKey(hKey);

    if (r != ERROR_SUCCESS || dll[0] == L'\0') {
        Log(L"verify-com FAIL: InprocServer32 default value missing");
        return 1;
    }

    if (GetFileAttributesW(dll) == INVALID_FILE_ATTRIBUTES) {
        Log(L"verify-com FAIL: DLL not found on disk: %s  (Win32 %lu)",
            dll, GetLastError());
        return 1;
    }

    Log(L"verify-com OK: %s", dll);
    return 0;
}

// =============================================================================
//  --verify-association <guid>
//  Exit 0 if the endpoint's FxProperties contains the Veyra render CLSID.
// =============================================================================

static int CmdVerifyAssociation(const wchar_t* guid) {
    if (!guid || guid[0] == L'\0') {
        Log(L"--verify-association: endpoint GUID required");
        return 1;
    }

    wchar_t fxPath[512] = {};
    _snwprintf_s(fxPath, _countof(fxPath), _TRUNCATE,
        L"%s\\%s\\FxProperties", kRenderRegBase, guid);

    HKEY hFx = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, fxPath, 0, KEY_QUERY_VALUE, &hFx) != ERROR_SUCCESS) {
        Log(L"verify-association FAIL: FxProperties key not found: HKLM\\%s", fxPath);
        return 1;
    }

    BYTE raw[512] = {};
    DWORD cbVal = sizeof(raw);
    DWORD type  = 0;
    LONG r = RegQueryValueExW(hFx, kFxPostMixKey, nullptr, &type, raw, &cbVal);
    RegCloseKey(hFx);

    if (r != ERROR_SUCCESS) {
        Log(L"verify-association FAIL: PostMixEffectClsid key absent (Win32 %ld)", r);
        return 1;
    }

    // Accept both REG_SZ and REG_BINARY (PROPVARIANT)
    const wchar_t* strVal = reinterpret_cast<const wchar_t*>(raw);
    if (type == REG_BINARY && cbVal >= 4 + sizeof(wchar_t))
        strVal = reinterpret_cast<const wchar_t*>(raw + 4);
    else if (type != REG_SZ) {
        Log(L"verify-association FAIL: unexpected registry type %lu", type);
        return 1;
    }

    if (_wcsicmp(strVal, kRenderClsid) != 0) {
        Log(L"verify-association FAIL: value is '%s', expected '%s'",
            strVal, kRenderClsid);
        return 1;
    }

    Log(L"verify-association OK: endpoint %s has PostMixEffectClsid = %s (type %lu)",
        guid, strVal, type);
    return 0;
}

// =============================================================================
//  --verify-service
//  Exit 0 if VeyraAudioService is in SERVICE_RUNNING state.
// =============================================================================

static int CmdVerifyService() {
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        Log(L"verify-service FAIL: OpenSCManager: Win32 %lu", GetLastError());
        return 1;
    }

    SC_HANDLE svc = OpenServiceW(scm, kServiceName, SERVICE_QUERY_STATUS);
    if (!svc) {
        DWORD err = GetLastError();
        CloseServiceHandle(scm);
        if (err == ERROR_SERVICE_DOES_NOT_EXIST)
            Log(L"verify-service FAIL: '%s' is not installed", kServiceName);
        else
            Log(L"verify-service FAIL: OpenService: Win32 %lu", err);
        return 1;
    }

    SERVICE_STATUS_PROCESS ss = {};
    DWORD needed = 0;
    BOOL ok = QueryServiceStatusEx(svc, SC_STATUS_PROCESS_INFO,
                                   reinterpret_cast<LPBYTE>(&ss), sizeof(ss), &needed);
    CloseServiceHandle(svc);
    CloseServiceHandle(scm);

    if (!ok) {
        Log(L"verify-service FAIL: QueryServiceStatus: Win32 %lu", GetLastError());
        return 1;
    }

    static const wchar_t* stateNames[] = {
        L"UNKNOWN(0)", L"STOPPED", L"START_PENDING", L"STOP_PENDING",
        L"RUNNING", L"CONTINUE_PENDING", L"PAUSE_PENDING", L"PAUSED"
    };
    DWORD state = ss.dwCurrentState;
    const wchar_t* stateName = (state < 8) ? stateNames[state] : L"UNKNOWN";

    if (state != SERVICE_RUNNING) {
        Log(L"verify-service FAIL: '%s' state is %s (%lu)",
            kServiceName, stateName, state);
        return 1;
    }

    Log(L"verify-service OK: '%s' is RUNNING (pid %lu)",
        kServiceName, ss.dwProcessId);
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
        Log(L"  --list-devices <outfile>       Enumerate active render endpoints -> INI");
        Log(L"  --associate <guid>             Write APO CLSID to endpoint FxProperties");
        Log(L"  --unassociate-all              Remove Veyra CLSIDs from all endpoints");
        Log(L"  --verify-com                   Exit 0 if COM CLSID registered + DLL present");
        Log(L"  --verify-association <guid>    Exit 0 if endpoint FxProperties has Veyra CLSID");
        Log(L"  --verify-service               Exit 0 if VeyraAudioService is RUNNING");
        Log(L"  --restart-audio                Restart AudioSrv and AudioEndpointBuilder");
        LogClose();
        return 1;
    }

    int result = 1;
    if      (wcscmp(action, L"--list-devices")       == 0) result = CmdListDevices(arg1);
    else if (wcscmp(action, L"--associate")           == 0) result = CmdAssociate(arg1);
    else if (wcscmp(action, L"--unassociate-all")     == 0) result = CmdUnassociateAll();
    else if (wcscmp(action, L"--verify-com")          == 0) result = CmdVerifyCom();
    else if (wcscmp(action, L"--verify-association")  == 0) result = CmdVerifyAssociation(arg1);
    else if (wcscmp(action, L"--verify-service")      == 0) result = CmdVerifyService();
    else if (wcscmp(action, L"--restart-audio")       == 0) result = RestartAudioService() ? 0 : 1;
    else { Log(L"Unknown action: %s", action); result = 1; }

    LogClose();
    return result;
}
