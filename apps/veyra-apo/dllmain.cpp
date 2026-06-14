// Veyra APO DLL entry point: COM plumbing (class factory + exports) and
// self-registration of the CLSID. The endpoint-to-APO association itself is
// done by the driver INF (see installer/driver/); DllRegisterServer only
// registers the in-proc COM server.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <atomic>
#include <new>

#include "VeyraApo.h"

// {7E9C2B14-3F6A-4D8E-9B21-5C0A1F2E3D44}
const CLSID CLSID_VeyraApoEfx =
    {0x7e9c2b14, 0x3f6a, 0x4d8e, {0x9b, 0x21, 0x5c, 0x0a, 0x1f, 0x2e, 0x3d, 0x44}};

std::atomic<LONG> g_apoObjectCount{0};
namespace {
std::atomic<LONG> g_apoLockCount{0};
HMODULE g_module = nullptr;

// ---- Class factory ---------------------------------------------------------

class VeyraClassFactory final : public IClassFactory {
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override
    {
        if (!ppv)
            return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IClassFactory))
        {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return 2; }   // static lifetime
    STDMETHODIMP_(ULONG) Release() override { return 1; }

    STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv) override
    {
        if (ppv)
            *ppv = nullptr;
        if (pUnkOuter)
            return CLASS_E_NOAGGREGATION;

        auto* apo = new (std::nothrow) VeyraApoEfx();
        if (!apo)
            return E_OUTOFMEMORY;

        const HRESULT hr = apo->QueryInterface(riid, ppv);
        apo->Release(); // balance the constructor's ref==1
        return hr;
    }
    STDMETHODIMP LockServer(BOOL fLock) override
    {
        g_apoLockCount.fetch_add(fLock ? 1 : -1, std::memory_order_relaxed);
        return S_OK;
    }
};

VeyraClassFactory g_classFactory;

bool regSetString(HKEY key, const wchar_t* name, const wchar_t* value)
{
    const DWORD bytes = static_cast<DWORD>((wcslen(value) + 1) * sizeof(wchar_t));
    return RegSetValueExW(key, name, 0, REG_SZ,
                          reinterpret_cast<const BYTE*>(value), bytes) == ERROR_SUCCESS;
}

} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_module = module;
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    if (rclsid == CLSID_VeyraApoEfx)
        return g_classFactory.QueryInterface(riid, ppv);
    if (ppv)
        *ppv = nullptr;
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow()
{
    return (g_apoObjectCount.load(std::memory_order_relaxed) == 0 &&
            g_apoLockCount.load(std::memory_order_relaxed) == 0)
               ? S_OK
               : S_FALSE;
}

STDAPI DllRegisterServer()
{
    wchar_t clsidStr[64] = {};
    StringFromGUID2(CLSID_VeyraApoEfx, clsidStr, ARRAYSIZE(clsidStr));

    wchar_t modulePath[MAX_PATH] = {};
    if (GetModuleFileNameW(g_module, modulePath, ARRAYSIZE(modulePath)) == 0)
        return HRESULT_FROM_WIN32(GetLastError());

    wchar_t keyPath[128] = {};
    swprintf_s(keyPath, ARRAYSIZE(keyPath), L"CLSID\\%s", clsidStr);

    HKEY clsidKey = nullptr;
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, keyPath, 0, nullptr, 0,
                        KEY_WRITE, nullptr, &clsidKey, nullptr) != ERROR_SUCCESS)
        return SELFREG_E_CLASS;

    regSetString(clsidKey, nullptr, L"Veyra Output Effect APO");

    HKEY inprocKey = nullptr;
    HRESULT hr = SELFREG_E_CLASS;
    if (RegCreateKeyExW(clsidKey, L"InprocServer32", 0, nullptr, 0,
                        KEY_WRITE, nullptr, &inprocKey, nullptr) == ERROR_SUCCESS)
    {
        if (regSetString(inprocKey, nullptr, modulePath) &&
            regSetString(inprocKey, L"ThreadingModel", L"Both"))
            hr = S_OK;
        RegCloseKey(inprocKey);
    }
    RegCloseKey(clsidKey);
    return hr;
}

STDAPI DllUnregisterServer()
{
    wchar_t clsidStr[64] = {};
    StringFromGUID2(CLSID_VeyraApoEfx, clsidStr, ARRAYSIZE(clsidStr));

    wchar_t keyPath[128] = {};
    swprintf_s(keyPath, ARRAYSIZE(keyPath), L"CLSID\\%s", clsidStr);
    RegDeleteTreeW(HKEY_CLASSES_ROOT, keyPath);
    return S_OK;
}
