# Security Policy

## Supported versions

| Version | Supported |
| ------- | --------- |
| 1.0.x   | ✅ |
| < 1.0   | ❌ |

## Reporting a vulnerability

Please **do not** open a public issue for security problems.

Report privately via [GitHub Security Advisories](https://github.com/NextGenDev-KSK/veyra/security/advisories/new)
(preferred) or by email to **krithiksendhilkumar@gmail.com** with the subject
`[VEYRA SECURITY]`.

Include what you can: affected version, reproduction steps, and impact. You
should receive an acknowledgement within 7 days. Please allow up to 90 days
for a fix before public disclosure.

## Scope notes

Veyra runs a Windows service as `NT AUTHORITY\LocalService`, exposes two named
pipes to the interactive console session, and shares read-only memory blocks
with the UI/overlay/APO. Reports about privilege escalation across those
boundaries, the installer/uninstaller (which run elevated), or the update check
are especially valuable. Audio-quality bugs and crashes without a security
boundary crossing belong in regular issues.
