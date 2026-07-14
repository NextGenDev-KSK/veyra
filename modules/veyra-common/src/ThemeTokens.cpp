#include "veyra/ThemeTokens.h"

#include <algorithm>
#include <cmath>

#include <nlohmann/json.hpp>

namespace veyra::theme {

// --- colour math -------------------------------------------------------------

namespace {

struct Rgb { float r, g, b; };

Rgb toRgb(Argb c)
{
    return {((c >> 16) & 0xFF) / 255.0f, ((c >> 8) & 0xFF) / 255.0f, (c & 0xFF) / 255.0f};
}

Argb fromRgb(Rgb v, std::uint8_t alpha)
{
    auto ch = [](float x) {
        return (std::uint32_t) std::lround(std::clamp(x, 0.0f, 1.0f) * 255.0f);
    };
    return ((Argb) alpha << 24) | (ch(v.r) << 16) | (ch(v.g) << 8) | ch(v.b);
}

std::uint8_t alphaOf(Argb c) { return (std::uint8_t) (c >> 24); }

// RGB <-> HSL for the lightness scaling used by the Custom derivation.
struct Hsl { float h, s, l; };

Hsl rgbToHsl(Rgb c)
{
    const float mx = std::max({c.r, c.g, c.b});
    const float mn = std::min({c.r, c.g, c.b});
    const float l  = (mx + mn) * 0.5f;
    if (mx == mn)
        return {0.0f, 0.0f, l};
    const float d = mx - mn;
    const float s = l > 0.5f ? d / (2.0f - mx - mn) : d / (mx + mn);
    float h;
    if (mx == c.r)      h = (c.g - c.b) / d + (c.g < c.b ? 6.0f : 0.0f);
    else if (mx == c.g) h = (c.b - c.r) / d + 2.0f;
    else                h = (c.r - c.g) / d + 4.0f;
    return {h / 6.0f, s, l};
}

float hueToRgb(float p, float q, float t)
{
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1.0f / 6) return p + (q - p) * 6 * t;
    if (t < 1.0f / 2) return q;
    if (t < 2.0f / 3) return p + (q - p) * (2.0f / 3 - t) * 6;
    return p;
}

Rgb hslToRgb(Hsl c)
{
    if (c.s == 0.0f)
        return {c.l, c.l, c.l};
    const float q = c.l < 0.5f ? c.l * (1 + c.s) : c.l + c.s - c.l * c.s;
    const float p = 2 * c.l - q;
    return {hueToRgb(p, q, c.h + 1.0f / 3), hueToRgb(p, q, c.h), hueToRgb(p, q, c.h - 1.0f / 3)};
}

} // namespace

Argb withAlpha(Argb c, float alpha)
{
    const auto a = (std::uint32_t) std::lround(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);
    return (c & 0x00FFFFFF) | (a << 24);
}

Argb scaleLightness(Argb c, float factor)
{
    Hsl h = rgbToHsl(toRgb(c));
    // Pure black has zero lightness; give the scale something to grow from so
    // "lighten black x2" is a visible ramp instead of a no-op.
    if (h.l <= 0.0f && factor > 1.0f)
        h.l = 0.04f;
    h.l = std::clamp(h.l * factor, 0.0f, 1.0f);
    return fromRgb(hslToRgb(h), alphaOf(c));
}

float relativeLuminance(Argb c)
{
    auto lin = [](float u) {
        return u <= 0.03928f ? u / 12.92f : std::pow((u + 0.055f) / 1.055f, 2.4f);
    };
    const Rgb v = toRgb(c);
    return 0.2126f * lin(v.r) + 0.7152f * lin(v.g) + 0.0722f * lin(v.b);
}

float contrastRatio(Argb a, Argb b)
{
    const float la = relativeLuminance(a), lb = relativeLuminance(b);
    const float hi = std::max(la, lb), lo = std::min(la, lb);
    return (hi + 0.05f) / (lo + 0.05f);
}

std::optional<Argb> parseHexColor(const std::string& text)
{
    std::string t = text;
    if (!t.empty() && t.front() == '#')
        t.erase(t.begin());
    if (t.size() != 6 && t.size() != 8)
        return std::nullopt;
    Argb v = 0;
    for (const char ch : t)
    {
        v <<= 4;
        if (ch >= '0' && ch <= '9') v |= (Argb) (ch - '0');
        else if (ch >= 'a' && ch <= 'f') v |= (Argb) (ch - 'a' + 10);
        else if (ch >= 'A' && ch <= 'F') v |= (Argb) (ch - 'A' + 10);
        else return std::nullopt;
    }
    if (t.size() == 6)
        v |= 0xFF000000;
    return v;
}

std::string formatHexColor(Argb c)
{
    static const char* hex = "0123456789ABCDEF";
    std::string out = "#RRGGBB";
    const std::uint32_t rgb = c & 0x00FFFFFF;
    for (int i = 0; i < 6; ++i)
        out[(size_t) (6 - i)] = hex[(rgb >> (i * 4)) & 0xF];
    return out;
}

// --- curated palettes ----------------------------------------------------------

namespace {

constexpr Argb C(std::uint32_t rgb) { return 0xFF000000 | rgb; }

// Base helper: semantic trio + "meters follow semantics" + track = sunken.
void followSemantics(ThemeTokens& t)
{
    t.meterLow   = t.success;
    t.meterMid   = t.warning;
    t.meterHigh  = t.danger;
    t.meterTrack = t.surfaceSunken;
}

ThemeTokens violet()
{
    ThemeTokens t;
    t.dark = true;
    t.surfaceApp = C(0x0C0814); t.surfacePanel = C(0x140E24); t.surfaceRaised = C(0x1C1432);
    t.surfaceSunken = C(0x0F0A1E); t.surfaceNav = C(0x110C20);
    t.border = C(0x291F45); t.borderStrong = C(0x3C2F63);
    t.accent = C(0x8B5CF6); t.accentBright = C(0xA78BFA); t.accentDeep = C(0x7C3AED);
    t.success = C(0x2FB37F); t.warning = C(0xE6B325); t.danger = C(0xE5484D);
    t.meterLow = C(0x27C08A); t.meterMid = C(0xE6B325); t.meterHigh = C(0xE5484D);
    t.meterTrack = t.surfaceSunken;
    t.textPrimary = C(0xEAE4FA); t.textSecondary = C(0xA093BC); t.textMuted = C(0x675C80);
    t.textOnAccent = C(0xFFFFFF); t.textTitle = C(0xE9DCFF);
    return t;
}

ThemeTokens light()
{
    ThemeTokens t;
    t.dark = false;
    t.surfaceApp = C(0xF4F6FA); t.surfacePanel = C(0xFFFFFF); t.surfaceRaised = C(0xEDF1F7);
    t.surfaceSunken = C(0xE3E9F2); t.surfaceNav = C(0xEAEEF5);
    t.border = C(0xD4DCE8); t.borderStrong = C(0xB7C3D6);
    t.accent = C(0x7C3AED); t.accentBright = C(0x8B5CF6); t.accentDeep = C(0x6D28D9);
    t.success = C(0x188A5C); t.warning = C(0x9A6C00); t.danger = C(0xD02F36);
    t.meterLow = C(0x1FA870); t.meterMid = C(0xD99C00); t.meterHigh = C(0xE5484D);
    t.meterTrack = C(0xDCE3EE);
    t.textPrimary = C(0x1A2333); t.textSecondary = C(0x4A5A74); t.textMuted = C(0x7C8AA0);
    t.textOnAccent = C(0xFFFFFF); t.textTitle = C(0x16233F);
    return t;
}

ThemeTokens blue()
{
    ThemeTokens t;
    t.dark = true;
    t.surfaceApp = C(0x070B14); t.surfacePanel = C(0x0E1626); t.surfaceRaised = C(0x141E32);
    t.surfaceSunken = C(0x0A1120); t.surfaceNav = C(0x0B1322);
    t.border = C(0x1C2942); t.borderStrong = C(0x2A3B5C);
    t.accent = C(0x3B82F6); t.accentBright = C(0x5AA0FF); t.accentDeep = C(0x2563EB);
    t.success = C(0x2FB37F); t.warning = C(0xE6B325); t.danger = C(0xE5484D);
    t.textPrimary = C(0xE4ECFA); t.textSecondary = C(0x93A2BC); t.textMuted = C(0x586880);
    t.textOnAccent = C(0xFFFFFF); t.textTitle = C(0xDCE8FF);
    followSemantics(t);
    return t;
}

ThemeTokens midnight()
{
    ThemeTokens t = blue(); // semantics/meters as Blue
    t.surfaceApp = C(0x05070E); t.surfacePanel = C(0x0A0E1A); t.surfaceRaised = C(0x101627);
    t.surfaceSunken = C(0x04060C); t.surfaceNav = C(0x080C16);
    t.border = C(0x171F35); t.borderStrong = C(0x26304D);
    t.accent = C(0x6C8CFF); t.accentBright = C(0x8FA8FF); t.accentDeep = C(0x4E6EE3);
    t.textPrimary = C(0xE6EAF7); t.textSecondary = C(0x96A0BC); t.textMuted = C(0x5A6480);
    t.textOnAccent = C(0x0A0E1A); t.textTitle = C(0xE0E6FF);
    followSemantics(t);
    return t;
}

ThemeTokens oledBlack()
{
    ThemeTokens t = blue(); // accent + semantics as Blue
    t.surfaceApp = C(0x000000); t.surfacePanel = C(0x0A0A0C); t.surfaceRaised = C(0x141418);
    t.surfaceSunken = C(0x000000); t.surfaceNav = C(0x050507);
    t.border = C(0x1E1E24); t.borderStrong = C(0x32323C);
    t.textPrimary = C(0xEDEDF2); t.textSecondary = C(0x9A9AA6); t.textMuted = C(0x5E5E68);
    t.textOnAccent = C(0xFFFFFF); t.textTitle = C(0xFFFFFF);
    followSemantics(t);
    t.meterTrack = C(0x000000);
    return t;
}

ThemeTokens graphite()
{
    ThemeTokens t = blue();
    t.surfaceApp = C(0x121316); t.surfacePanel = C(0x1A1C20); t.surfaceRaised = C(0x22252B);
    t.surfaceSunken = C(0x0D0E11); t.surfaceNav = C(0x17191D);
    t.border = C(0x2A2E36); t.borderStrong = C(0x3D434E);
    t.accent = C(0x6B7FD7); t.accentBright = C(0x8B9BE8); t.accentDeep = C(0x5064C0);
    t.warning = C(0xD9A62E);
    t.textPrimary = C(0xE8EAED); t.textSecondary = C(0x9BA1AB); t.textMuted = C(0x62676F);
    t.textOnAccent = C(0xFFFFFF); t.textTitle = C(0xF0F2F5);
    followSemantics(t); // meterMid picks up the graphite warning
    return t;
}

ThemeTokens nord()
{
    ThemeTokens t;
    t.dark = true;
    t.surfaceApp = C(0x272C36); t.surfacePanel = C(0x2E3440); t.surfaceRaised = C(0x3B4252);
    t.surfaceSunken = C(0x232831); t.surfaceNav = C(0x2A303C);
    t.border = C(0x434C5E); t.borderStrong = C(0x5E6A82);
    t.accent = C(0x88C0D0); t.accentBright = C(0x9FD3E3); t.accentDeep = C(0x5E81AC);
    t.success = C(0xA3BE8C); t.warning = C(0xEBCB8B); t.danger = C(0xBF616A);
    t.textPrimary = C(0xECEFF4); t.textSecondary = C(0xD8DEE9); t.textMuted = C(0x7B88A1);
    t.textOnAccent = C(0x2E3440); // pale ice accent takes dark text
    t.textTitle = C(0xECEFF4);
    followSemantics(t);
    return t;
}

ThemeTokens dracula()
{
    ThemeTokens t;
    t.dark = true;
    t.surfaceApp = C(0x21222C); t.surfacePanel = C(0x282A36); t.surfaceRaised = C(0x343746);
    t.surfaceSunken = C(0x1D1E26); t.surfaceNav = C(0x242631);
    t.border = C(0x3C3F51); t.borderStrong = C(0x565973);
    t.accent = C(0xBD93F9); t.accentBright = C(0xD3B3FF); t.accentDeep = C(0x9A6EE8);
    t.success = C(0x50FA7B); t.warning = C(0xF1FA8C); t.danger = C(0xFF5555);
    t.textPrimary = C(0xF8F8F2); t.textSecondary = C(0xB9BACB); t.textMuted = C(0x7B7F95);
    t.textOnAccent = C(0x21222C); // pale purple accent takes dark text
    t.textTitle = C(0xF8F8F2);
    followSemantics(t);
    return t;
}

ThemeTokens catppuccin()
{
    ThemeTokens t;
    t.dark = true;
    t.surfaceApp = C(0x181825); t.surfacePanel = C(0x1E1E2E); t.surfaceRaised = C(0x313244);
    t.surfaceSunken = C(0x11111B); t.surfaceNav = C(0x1B1B29);
    t.border = C(0x313244); t.borderStrong = C(0x45475A);
    t.accent = C(0x89B4FA); t.accentBright = C(0xA5C8FF); t.accentDeep = C(0x6E94E8);
    t.success = C(0xA6E3A1); t.warning = C(0xF9E2AF); t.danger = C(0xF38BA8);
    t.textPrimary = C(0xCDD6F4); t.textSecondary = C(0xA6ADC8); t.textMuted = C(0x6C7086);
    t.textOnAccent = C(0x1E1E2E); // pale blue accent takes dark text
    t.textTitle = C(0xCDD6F4);
    followSemantics(t);
    return t;
}

ThemeTokens tokyoNight()
{
    ThemeTokens t;
    t.dark = true;
    t.surfaceApp = C(0x16161E); t.surfacePanel = C(0x1A1B26); t.surfaceRaised = C(0x24283B);
    t.surfaceSunken = C(0x101017); t.surfaceNav = C(0x14141C);
    t.border = C(0x292E42); t.borderStrong = C(0x3B4261);
    t.accent = C(0x7AA2F7); t.accentBright = C(0x9AB8FF); t.accentDeep = C(0x5A7EDB);
    t.success = C(0x9ECE6A); t.warning = C(0xE0AF68); t.danger = C(0xF7768E);
    t.textPrimary = C(0xC0CAF5); t.textSecondary = C(0x9AA5CE); t.textMuted = C(0x565F89);
    t.textOnAccent = C(0x16161E);
    t.textTitle = C(0xC0CAF5);
    followSemantics(t);
    return t;
}

ThemeTokens solarizedDark()
{
    ThemeTokens t;
    t.dark = true;
    t.surfaceApp = C(0x00212B); t.surfacePanel = C(0x002B36); t.surfaceRaised = C(0x073642);
    t.surfaceSunken = C(0x001B23); t.surfaceNav = C(0x00252F);
    t.border = C(0x0A3540); t.borderStrong = C(0x14505E);
    t.accent = C(0x268BD2); t.accentBright = C(0x4BA3E3); t.accentDeep = C(0x1E6FA8);
    t.success = C(0x859900); t.warning = C(0xB58900); t.danger = C(0xDC322F);
    t.textPrimary = C(0xB5C2C0); t.textSecondary = C(0x839496); t.textMuted = C(0x586E75);
    t.textOnAccent = C(0xFDF6E3);
    t.textTitle = C(0xEEE8D5);
    followSemantics(t);
    return t;
}

ThemeTokens oneDark()
{
    ThemeTokens t;
    t.dark = true;
    t.surfaceApp = C(0x21252B); t.surfacePanel = C(0x282C34); t.surfaceRaised = C(0x2F343F);
    t.surfaceSunken = C(0x1B1F24); t.surfaceNav = C(0x23272E);
    t.border = C(0x3A3F4B); t.borderStrong = C(0x4B5263);
    t.accent = C(0x61AFEF); t.accentBright = C(0x83C2FF); t.accentDeep = C(0x4A8FD0);
    t.success = C(0x98C379); t.warning = C(0xE5C07B); t.danger = C(0xE06C75);
    t.textPrimary = C(0xD7DDE8); t.textSecondary = C(0xABB2BF); t.textMuted = C(0x6F7683);
    t.textOnAccent = C(0x21252B);
    t.textTitle = C(0xD7DDE8);
    followSemantics(t);
    return t;
}

ThemeTokens githubDark()
{
    ThemeTokens t;
    t.dark = true;
    t.surfaceApp = C(0x010409); t.surfacePanel = C(0x0D1117); t.surfaceRaised = C(0x161B22);
    t.surfaceSunken = C(0x010409); t.surfaceNav = C(0x0A0E14);
    t.border = C(0x21262D); t.borderStrong = C(0x30363D);
    t.accent = C(0x1F6FEB); t.accentBright = C(0x388BFD); t.accentDeep = C(0x1158C7);
    t.success = C(0x3FB950); t.warning = C(0xD29922); t.danger = C(0xF85149);
    t.textPrimary = C(0xE6EDF3); t.textSecondary = C(0x8B949E); t.textMuted = C(0x6E7681);
    t.textOnAccent = C(0xFFFFFF);
    t.textTitle = C(0xE6EDF3);
    followSemantics(t);
    return t;
}

ThemeTokens highContrast()
{
    ThemeTokens t;
    t.dark = true;
    t.surfaceApp = C(0x000000); t.surfacePanel = C(0x0C0C10); t.surfaceRaised = C(0x1C1C22);
    t.surfaceSunken = C(0x000000); t.surfaceNav = C(0x0C0C10);
    t.border = C(0x6E6E78); t.borderStrong = C(0xFFFFFF);
    t.accent = C(0x4DA6FF); t.accentBright = C(0x85C4FF); t.accentDeep = C(0x2E7BE0);
    t.success = C(0x3BE38F); t.warning = C(0xFFD23F); t.danger = C(0xFF5C63);
    t.textPrimary = C(0xFFFFFF); t.textSecondary = C(0xDCDCE2); t.textMuted = C(0xB4B4BE);
    t.textOnAccent = C(0x04121E);
    t.textTitle = C(0xFFFFFF);
    followSemantics(t);
    return t;
}

} // namespace

const std::vector<ThemeInfo>& themeCatalog()
{
    static const std::vector<ThemeInfo> catalog = {
        {"violet", "Violet"},
        {"light", "Light"},
        {"blue", "Blue"},
        {"midnight", "Midnight"},
        {"oled-black", "OLED Black"},
        {"graphite", "Graphite"},
        {"nord", "Nord"},
        {"dracula", "Dracula"},
        {"catppuccin", "Catppuccin"},
        {"tokyo-night", "Tokyo Night"},
        {"solarized", "Solarized"},
        {"one-dark", "One Dark"},
        {"github-dark", "GitHub Dark"},
        {"high-contrast", "High Contrast"},
        {"custom", "Custom"},
    };
    return catalog;
}

ThemeTokens themeTokens(const std::string& id)
{
    if (id == "violet")        return violet();
    if (id == "light")         return light();
    if (id == "blue")          return blue();
    if (id == "midnight")      return midnight();
    if (id == "oled-black")    return oledBlack();
    if (id == "graphite")      return graphite();
    if (id == "nord")          return nord();
    if (id == "dracula")       return dracula();
    if (id == "catppuccin")    return catppuccin();
    if (id == "tokyo-night")   return tokyoNight();
    if (id == "solarized")     return solarizedDark();
    if (id == "one-dark")      return oneDark();
    if (id == "github-dark")   return githubDark();
    if (id == "high-contrast") return highContrast();

    // Legacy pre-port ids from earlier releases map onto their closest new theme.
    if (id == "pure-black") return oledBlack();
    if (id == "daylight")   return light();
    if (id == "mono")       return graphite();
    if (id == "ocean")      return blue();

    return violet(); // the default, and the fallback for anything unknown
}

// --- Custom derivation -------------------------------------------------------

ThemeTokens deriveCustom(const CustomAnchors& a)
{
    ThemeTokens t = violet(); // the default base

    if (a.background)
    {
        const Argb bg   = *a.background;
        t.surfaceApp    = bg;
        t.surfacePanel  = scaleLightness(bg, 1.35f);
        t.surfaceRaised = scaleLightness(bg, 1.75f);
        t.surfaceSunken = scaleLightness(bg, 1.0f / 1.30f);
        t.surfaceNav    = scaleLightness(bg, 1.18f);
        t.border        = scaleLightness(bg, 2.1f);
        t.borderStrong  = scaleLightness(bg, 2.9f);
        t.meterTrack    = t.surfaceSunken;
        t.dark          = relativeLuminance(bg) < 0.5f;
    }

    if (a.accent)
    {
        t.accent       = *a.accent;
        t.accentBright = scaleLightness(*a.accent, 1.25f);
        t.accentDeep   = scaleLightness(*a.accent, 1.0f / 1.25f);
        // Never hardcode white-on-accent: pale (and mid-luminance) accents need
        // dark text. Pick whichever of white / the panel colour reads better.
        const Argb white = C(0xFFFFFF);
        t.textOnAccent = contrastRatio(*a.accent, white) >= contrastRatio(*a.accent, t.surfacePanel)
                             ? white
                             : t.surfacePanel;
    }

    if (a.success) t.success = *a.success;
    if (a.warning) t.warning = *a.warning;
    if (a.danger)  t.danger  = *a.danger;

    // Semantics drive the meters unless meter anchors are set explicitly.
    t.meterLow  = a.meterLow  ? *a.meterLow  : (a.success ? *a.success : t.meterLow);
    t.meterMid  = a.meterMid  ? *a.meterMid  : (a.warning ? *a.warning : t.meterMid);
    t.meterHigh = a.meterHigh ? *a.meterHigh : (a.danger  ? *a.danger  : t.meterHigh);

    return t;
}

// --- JSON ---------------------------------------------------------------------

namespace {

nlohmann::json anchorsToJsonObject(const CustomAnchors& a)
{
    nlohmann::json j = nlohmann::json::object();
    auto put = [&j](const char* key, const std::optional<Argb>& v) {
        if (v) j[key] = formatHexColor(*v);
    };
    put("accent", a.accent);
    put("background", a.background);
    put("success", a.success);
    put("warning", a.warning);
    put("danger", a.danger);
    put("meterLow", a.meterLow);
    put("meterMid", a.meterMid);
    put("meterHigh", a.meterHigh);
    return j;
}

CustomAnchors anchorsFromJsonObject(const nlohmann::json& j)
{
    CustomAnchors a;
    if (!j.is_object())
        return a;
    auto get = [&j](const char* key) -> std::optional<Argb> {
        const auto it = j.find(key);
        if (it == j.end() || !it->is_string())
            return std::nullopt;
        return parseHexColor(it->get<std::string>());
    };
    a.accent     = get("accent");
    a.background = get("background");
    a.success    = get("success");
    a.warning    = get("warning");
    a.danger     = get("danger");
    a.meterLow   = get("meterLow");
    a.meterMid   = get("meterMid");
    a.meterHigh  = get("meterHigh");
    return a;
}

bool anyAnchorSet(const CustomAnchors& a)
{
    return a.accent || a.background || a.success || a.warning || a.danger
        || a.meterLow || a.meterMid || a.meterHigh;
}

} // namespace

std::string anchorsToJson(const CustomAnchors& anchors)
{
    return anchorsToJsonObject(anchors).dump();
}

CustomAnchors anchorsFromJson(const std::string& jsonText)
{
    const auto j = nlohmann::json::parse(jsonText, nullptr, /*allow_exceptions=*/false);
    return anchorsFromJsonObject(j);
}

std::string exportThemeFile(const ThemeFile& file)
{
    nlohmann::json j;
    j["themeName"] = file.themeName;
    if (anyAnchorSet(file.custom))
        j["custom"] = anchorsToJsonObject(file.custom);
    return j.dump(2);
}

std::optional<ThemeFile> importThemeFile(const std::string& text)
{
    if (text.size() > kThemeFileMaxBytes)
        return std::nullopt;
    const auto j = nlohmann::json::parse(text, nullptr, /*allow_exceptions=*/false);
    if (!j.is_object())
        return std::nullopt;
    const auto name = j.find("themeName");
    if (name == j.end() || !name->is_string())
        return std::nullopt;

    ThemeFile file;
    file.themeName = name->get<std::string>();
    if (const auto custom = j.find("custom"); custom != j.end())
        file.custom = anchorsFromJsonObject(*custom);
    return file;
}

} // namespace veyra::theme
