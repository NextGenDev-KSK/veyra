# winget packaging

Manifests for `winget install VeyraSounds.Veyra`. The three files mirror the
layout microsoft/winget-pkgs expects under
`manifests/v/VeyraSounds/Veyra/<version>/`.

## Submitting a new version

1. Publish the GitHub release and note the installer's SHA-256 from
   `SHA256SUMS.txt`.
2. Update `PackageVersion`, `InstallerUrl` and `InstallerSha256` in these
   files (hash uppercase).
3. Fork [microsoft/winget-pkgs](https://github.com/microsoft/winget-pkgs),
   copy the three files to
   `manifests/v/VeyraSounds/Veyra/<version>/`, and open a PR — or use
   [wingetcreate](https://github.com/microsoft/winget-create):
   ```
   wingetcreate update VeyraSounds.Veyra -u <installer-url> -v <version> --submit
   ```
4. The winget validation pipeline installs the package silently (`/S`) in a
   sandbox; the NSIS installer supports that out of the box.

Note: the first-ever submission creates the package (new-package review takes
a little longer). SmartScreen warnings do not apply to winget installs.
