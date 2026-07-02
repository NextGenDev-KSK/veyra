# Veyra Sounds — Packaging & Signing

## Open-source release strategy

Veyra 1.0 is an **open-source release on GitHub**. Binaries are unsigned.
This means:

- **VeyraSetup.exe** may trigger a Windows SmartScreen warning on first run. This is normal for any new open-source project that has not yet built up SmartScreen reputation. Users click **"More info → Run anyway"** to proceed.
- **veyra-apo.dll** must be **signed** to load into `audiodg.exe` — either (a) signed with a local test certificate for dev builds (with test-signing mode enabled), or (b) a production-signed release for end users. An *unsigned* DLL never loads: test-signing mode alone does not admit unsigned binaries, and the legacy `DisableProtectedAudioDG` override no longer disables the protection on current Windows 11 (verified on build 26200.8655, 2026-07-01). See §3 below.

**Signing is not a blocker for the open-source release.** The installer and application work correctly, and the **Audio Bridge** provides the working audio path without any signing (it runs the identical DSP chain in the service). The APO path activates on developer machines with a test-certificate-signed DLL, and for end users in a future signed release once the project qualifies for [SignPath Foundation](https://signpath.io/foundation) or another free signing solution.

Veyra ships three ways. Pick by audience:

| Form | Who | Signing |
| --- | --- | --- |
| **VeyraSetup.exe** | end users | none (SmartScreen may warn on first run) |
| **Portable ZIP** | devs / power users | none needed (SmartScreen may warn) |
| **MSIX** | end users / Store | must be signed by a cert the machine trusts |
| **APO driver package** | the low-latency audio path | must be signed; test-signing for dev |

CI builds the **portable ZIP** on every push (artifact `veyra-portable-x64`).
MSIX + driver signing are local/release steps documented below.

---

## 1. Portable ZIP

```powershell
pwsh installer/portable/make-portable.ps1 -BinDir build/windows-release/bin -Version 1.0.0
# -> dist-portable/veyra-portable-1.0.0-x64.zip
```
Unzip anywhere, run `veyra-service.exe` then `veyra.exe`. No admin for the basics.

---

## 2. MSIX

```powershell
pwsh installer/msix/make-msix.ps1 -BinDir build/windows-release/bin -Version 1.0.0
# -> dist-msix/veyra-1.0.0-x64.msix   (unsigned)
```
The package is a **full-trust desktop** MSIX (`runFullTrust`); the UI is the
entry point. `Identity/@Publisher` in `AppxManifest.xml` **must equal the signing
certificate subject** or install fails.

### Dev / test install (self-signed)
```powershell
# 1. Create a test cert whose subject matches the manifest Publisher.
$cert = New-SelfSignedCertificate -Type Custom -Subject "CN=Veyra Sounds" `
    -KeyUsage DigitalSignature -CertStoreLocation "Cert:\CurrentUser\My" `
    -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3")
# 2. Export + trust it (Local Machine\TrustedPeople), elevated:
$pwd = ConvertTo-SecureString -String "test" -Force -AsPlainText
Export-PfxCertificate -Cert "Cert:\CurrentUser\My\$($cert.Thumbprint)" -FilePath veyra-test.pfx -Password $pwd
Import-PfxCertificate -FilePath veyra-test.pfx -Password $pwd -CertStoreLocation Cert:\LocalMachine\TrustedPeople
# 3. Sign + install:
pwsh installer/msix/make-msix.ps1 -Version 1.0.0 -CertPath veyra-test.pfx -CertPassword test
Add-AppxPackage dist-msix/veyra-1.0.0-x64.msix
```

### Production signing (pick one)
1. **Microsoft Trusted Signing** (Azure) — cheapest trusted path for OSS.
2. **EV / OV code-signing certificate** from a CA, used with `signtool`/the
   `-CertPath` arg above (HSM/USB token in CI via a cloud signer).
3. **Microsoft Store** submission (Store re-signs).

---

## 3. APO driver package (audio path)

The system-effect APO must be in a signed driver package to load into the
protected `audiodg.exe`. This is the **primary audio path** — the same mechanism
used by Dolby Atmos and DTS. For development use **test-signing** (see
[BUILD_GUIDE.md](../BUILD_GUIDE.md) §2 and `installer/driver/register-apo.ps1`).
For release, the package needs a WHQL/attestation signature via the Microsoft
Hardware Dev Center.

The **Audio Bridge** is an advanced fallback for Bluetooth endpoints that reject
the APO; it is not the default path. Users on wired or USB-C audio will use the
APO exclusively.

### Production signing path (post-1.0 roadmap)
1. **[SignPath Foundation](https://signpath.io/foundation)** — free code signing for open-source projects. Apply once the project is established.
2. **EV / OV code-signing certificate** from a CA (DigiCert, Sectigo, GlobalSign — ~$300–500/year). Sign `veyra-apo.dll` and `VeyraSetup.exe` with `signtool.exe`. This is the fastest commercial path.
3. **Microsoft Trusted Signing** (Azure) — cheapest trusted path for OSS; integrates with GitHub Actions.
4. **WHQL/attestation signing** via the Windows Hardware Dev Center — free for verified accounts; strongest trust level; longest lead time.

> For development: turn test-signing off when done: `bcdedit /set testsigning off` + reboot.
