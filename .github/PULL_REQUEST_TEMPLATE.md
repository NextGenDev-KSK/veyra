## What & why

<!-- What does this change do, and what problem does it solve?
     Link the issue if one exists: Fixes #123 -->

## How was it tested?

<!-- e.g. "ctest passes", "installed and uninstalled on Win11 24H2",
     "played audio through the Bridge and heard the EQ change".
     For audio-path changes, say which hardware you verified on. -->

## Checklist

- [ ] `cmake --build --preset windows-release` succeeds
- [ ] `ctest` passes (configure with `-DVEYRA_BUILD_TESTS=ON`)
- [ ] No behavior change hidden in a "cleanup" commit
- [ ] Docs updated if user-visible behavior changed (README / CHANGELOG)
