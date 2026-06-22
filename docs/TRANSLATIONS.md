# Translating Veyra Sounds

Veyra's UI strings live in `resources/lang/<locale>.json` — a flat map of
`"key": "translation"`. English is the built-in base catalog (in
`modules/veyra-common/src/Localization.cpp`); a locale file is an **overlay**:
any key it provides wins, and any key it omits falls back to English. So a
partial translation is fine and never leaves blank UI.

## Status

| Locale | File | State |
| --- | --- | --- |
| English | (built-in base catalog) | ✅ complete |
| Spanish | `es.json` | ✅ sample translation |
| Simplified Chinese | `zh-CN.json` | 🟡 template (English placeholders) |
| Arabic | `ar.json` | 🟡 template (English placeholders, **RTL**) |
| Hindi | `hi.json` | 🟡 template (English placeholders) |
| Tamil | `ta.json` | 🟡 template (English placeholders) |

## How to translate

1. Open the locale's JSON. Replace each English **value** with the translation;
   **do not change the keys** (left of the colon).
2. Leave `app.title` (the brand "Veyra Sounds") as-is.
3. The `_status` / `_locale` / `_rtl` keys are metadata — leave or remove them;
   they're ignored by the app.
4. Keep it valid UTF-8 JSON. Untranslated keys can be deleted (they fall back to
   English) or left as-is.

## Arabic (RTL)

`ar.json` is right-to-left. The app mirrors layout and right-aligns text when the
active locale is RTL; translate the values normally (don't add direction marks).

## Adding a new language

Create `resources/lang/<bcp47>.json` (e.g. `de.json`) with the same keys. It's
picked up from the install's `resources/lang` folder at runtime — no rebuild
needed to drop in or update a catalog.
