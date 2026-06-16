#include "PresetLibrary.h"

#include <algorithm>
#include <system_error>

#include "veyra/Logging.h"

namespace veyra::service {

PresetLibrary::PresetLibrary(std::filesystem::path dir, Logger* log)
    : dir_(std::move(dir)), log_(log)
{
}

void PresetLibrary::load()
{
    std::lock_guard<std::mutex> lock(mutex_);
    user_.clear();

    std::error_code ec;
    std::filesystem::create_directories(dir_, ec);

    if (!std::filesystem::exists(dir_, ec))
        return;

    for (const auto& entry : std::filesystem::directory_iterator(dir_, ec))
    {
        if (ec || !entry.is_regular_file())
            continue;
        if (entry.path().extension() != ".vpreset")
            continue;
        if (auto p = Preset::load(entry.path()))
        {
            p->builtIn = false; // user dir is always user-owned
            user_.push_back(std::move(*p));
        }
        else if (log_)
            log_->warn("PresetLibrary: skipped unreadable preset file");
    }

    if (log_)
        log_->info("PresetLibrary: loaded " + std::to_string(user_.size()) + " user preset(s)");
}

std::vector<Preset> PresetLibrary::all() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Preset> out = builtInPresets();
    out.insert(out.end(), user_.begin(), user_.end());
    return out;
}

std::optional<Preset> PresetLibrary::find(const std::string& uuid) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& p : builtInPresets())
        if (p.uuid == uuid)
            return p;
    for (const auto& p : user_)
        if (p.uuid == uuid)
            return p;
    return std::nullopt;
}

bool PresetLibrary::isBuiltIn(const std::string& uuid) const
{
    for (const auto& p : builtInPresets())
        if (p.uuid == uuid)
            return true;
    return false;
}

std::filesystem::path PresetLibrary::fileFor(const std::string& uuid) const
{
    return dir_ / (uuid + ".vpreset");
}

bool PresetLibrary::saveUser(const Preset& preset)
{
    if (preset.uuid.empty() || preset.name.empty())
        return false;

    std::lock_guard<std::mutex> lock(mutex_);
    if (isBuiltIn(preset.uuid))
    {
        if (log_)
            log_->warn("PresetLibrary: refused to overwrite a built-in preset");
        return false;
    }

    Preset p = preset;
    p.builtIn = false;
    if (!p.save(fileFor(p.uuid)))
        return false;

    // Replace any existing user preset with the same uuid.
    for (auto& existing : user_)
        if (existing.uuid == p.uuid)
        {
            existing = p;
            return true;
        }
    user_.push_back(std::move(p));
    return true;
}

bool PresetLibrary::removeUser(const std::string& uuid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (isBuiltIn(uuid))
        return false;

    std::error_code ec;
    std::filesystem::remove(fileFor(uuid), ec);

    const auto before = user_.size();
    user_.erase(std::remove_if(user_.begin(), user_.end(),
                               [&](const Preset& p) { return p.uuid == uuid; }),
                user_.end());
    return user_.size() != before;
}

std::string PresetLibrary::toJsonArray() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Preset> out = builtInPresets();
    out.insert(out.end(), user_.begin(), user_.end());
    return presetsToJsonArray(out);
}

} // namespace veyra::service
