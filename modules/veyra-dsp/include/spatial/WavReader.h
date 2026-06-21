#pragma once

// Minimal RIFF/WAVE reader for HRIR loading — no JUCE, no external deps, so it
// works inside the APO too. Handles PCM 16-bit (what the MIT KEMAR set uses);
// returns de-interleaved float channels in [-1, 1]. Little-endian host (x64).

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace veyra::dsp {

struct WavData {
    int channels = 0;
    int sampleRate = 0;
    std::vector<std::vector<float>> ch; // ch[channel][sample]
};

inline bool readWav(const std::filesystem::path& path, WavData& out)
{
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return false;

    auto rd16 = [&]() -> uint16_t { uint8_t b[2] = {0, 0}; f.read((char*) b, 2);
                                    return (uint16_t) (b[0] | (b[1] << 8)); };
    auto rd32 = [&]() -> uint32_t { uint8_t b[4] = {0, 0, 0, 0}; f.read((char*) b, 4);
                                    return (uint32_t) (b[0] | (b[1] << 8) | (b[2] << 16) | ((uint32_t) b[3] << 24)); };

    char tag[4];
    f.read(tag, 4); if (std::string(tag, 4) != "RIFF") return false;
    rd32();
    f.read(tag, 4); if (std::string(tag, 4) != "WAVE") return false;

    int channels = 0, sr = 0, bits = 0;
    uint16_t fmt = 1;
    std::vector<uint8_t> data;

    while (f && f.peek() != EOF)
    {
        char id[4];
        if (!f.read(id, 4)) break;
        const uint32_t sz = rd32();
        const std::string cid(id, 4);
        if (cid == "fmt ")
        {
            fmt = rd16();
            channels = rd16();
            sr = (int) rd32();
            rd32();          // byte rate
            rd16();          // block align
            bits = rd16();
            if (sz > 16) f.seekg((std::streamoff) (sz - 16), std::ios::cur);
        }
        else if (cid == "data")
        {
            data.resize(sz);
            f.read((char*) data.data(), (std::streamsize) sz);
        }
        else
        {
            f.seekg((std::streamoff) sz, std::ios::cur);
        }
        if (sz & 1u) f.seekg(1, std::ios::cur); // chunks are word-aligned
    }

    if (channels <= 0 || sr <= 0 || data.empty() || fmt != 1 || bits != 16)
        return false;

    const size_t frames = data.size() / (size_t) (2 * channels);
    out.channels = channels;
    out.sampleRate = sr;
    out.ch.assign((size_t) channels, std::vector<float>(frames, 0.0f));
    const int16_t* s = reinterpret_cast<const int16_t*>(data.data());
    for (size_t i = 0; i < frames; ++i)
        for (int c = 0; c < channels; ++c)
            out.ch[(size_t) c][i] = (float) s[i * (size_t) channels + (size_t) c] / 32768.0f;
    return true;
}

} // namespace veyra::dsp
