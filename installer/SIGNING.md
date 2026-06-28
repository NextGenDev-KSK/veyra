# Veyra Sounds — Packaging & Signing

Veyra ships three ways. Pick by audience:

| Form | Who | Signing |
| --- | --- | --- |
| **Portable ZIP** | devs / power users | none needed (SmartScreen may warn) |
| **MSIX** | end users / Store | must be signed by a cert the machine trusts |
| **APO driver package** | the low-latency audio path | must be signed; test-signing for dev |

CI builds the **portable ZIP** on every push (artifact `veyra-portable-x64`).
MSIX + driver signing are local/release steps documented below.

---

## 1. Portable ZIP

```powershell
pwsh installer/portable/make-portable.ps1 -BinDir build/windows-release/bin -Version 0.9.0
# -> dist-portable/veyra-portable-0.9.0-x64.zip
```
Unzip anywhere, run `veyra-service.exe` then `veyra.exe`. No admin for the basics.

---

## 2. MSIX

```powershell
pwsh installer/msix/make-msix.ps1 -BinDir build/windows-release/bin -Version 0.9.0
# -> dist-msix/veyra-0.9.0-x64.msix   (unsigned)
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
pwsh installer/msix/make-msix.ps1 -Version 0.9.0 -CertPath veyra-test.pfx -CertPassword test
Add-AppxPackage dist-msix/veyra-0.9.0-x64.msix
```

### Production signing (pick one)
1. **Microsoft Trusted Signing** (Azure) — cheapest trusted path for OSS.
2. **EV / OV code-signing certificate** from a CA, used with `signtool`/the
   `-CertPath` arg above (HSM/USB token in CI via a cloud signer).
3. **Microsoft Store** submission (Store re-signs).

---

## 3. APO driver package (audio path)

The system-effect APO must be in a signed driver package to load into the
protected `audiodg.exe`. For development use **test-signing** (see
[BUILD_GUIDE.md](../BUILD_GUIDE.md) §2 and `installer/driver/register-apo.ps1`).
For release, the package needs a WHQL/attestation signature via the Microsoft
Hardware Dev Center. The portable build's **Audio Bridge** path needs none of
this and is the default way to hear effects.

> Turn test-signing off when done: `bcdedit /set testsigning off` + reboot.
