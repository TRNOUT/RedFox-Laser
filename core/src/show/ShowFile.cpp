#include "show/ShowFile.hpp"

#include "ilda/IldaCodec.hpp"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace redfox::show {
namespace {

constexpr char kMagic[4] = {'R', 'F', 'S', 'H'};
constexpr std::uint32_t kVersion = 3;
// The lowest version this reader still understands. v2 shows have no timeline;
// they load with an empty one.
constexpr std::uint32_t kMinReadableVersion = 2;

void putU32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    out.push_back(static_cast<std::uint8_t>(v & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFF));
}

void putFloat(std::vector<std::uint8_t>& out, float f) {
    std::uint32_t bits;
    std::memcpy(&bits, &f, sizeof(bits));
    putU32(out, bits);
}

void putDouble(std::vector<std::uint8_t>& out, double d) {
    std::uint64_t bits;
    std::memcpy(&bits, &d, sizeof(bits));
    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<std::uint8_t>((bits >> (8 * i)) & 0xFF));
    }
}

void putString(std::vector<std::uint8_t>& out, const std::string& s) {
    putU32(out, static_cast<std::uint32_t>(s.size()));
    out.insert(out.end(), s.begin(), s.end());
}

void putBytes(std::vector<std::uint8_t>& out, const std::vector<std::uint8_t>& b) {
    putU32(out, static_cast<std::uint32_t>(b.size()));
    out.insert(out.end(), b.begin(), b.end());
}

// Cursor-based reader that fails safely on truncated input.
struct Reader {
    const std::uint8_t* p;
    std::size_t remaining;
    bool ok = true;

    std::uint32_t u32() {
        if (remaining < 4) {
            ok = false;
            return 0;
        }
        const std::uint32_t v = static_cast<std::uint32_t>(p[0]) |
                                (static_cast<std::uint32_t>(p[1]) << 8) |
                                (static_cast<std::uint32_t>(p[2]) << 16) |
                                (static_cast<std::uint32_t>(p[3]) << 24);
        p += 4;
        remaining -= 4;
        return v;
    }

    float f32() {
        const std::uint32_t bits = u32();
        float f = 0.0f;
        std::memcpy(&f, &bits, sizeof(f));
        return f;
    }

    double f64() {
        if (remaining < 8) {
            ok = false;
            return 0.0;
        }
        std::uint64_t bits = 0;
        for (int i = 0; i < 8; ++i) {
            bits |= static_cast<std::uint64_t>(p[i]) << (8 * i);
        }
        p += 8;
        remaining -= 8;
        double d = 0.0;
        std::memcpy(&d, &bits, sizeof(d));
        return d;
    }

    std::string str() {
        const std::uint32_t len = u32();
        if (!ok || remaining < len) {
            ok = false;
            return {};
        }
        std::string s(reinterpret_cast<const char*>(p), len);
        p += len;
        remaining -= len;
        return s;
    }

    std::vector<std::uint8_t> bytes() {
        const std::uint32_t len = u32();
        if (!ok || remaining < len) {
            ok = false;
            return {};
        }
        std::vector<std::uint8_t> b(p, p + len);
        p += len;
        remaining -= len;
        return b;
    }
};

} // namespace

std::vector<std::uint8_t> writeShow(const Show& show) {
    std::vector<std::uint8_t> out;
    out.insert(out.end(), kMagic, kMagic + 4);
    putU32(out, kVersion);
    putString(out, show.name);
    putU32(out, static_cast<std::uint32_t>(show.cues.size()));
    for (const Cue& cue : show.cues) {
        putString(out, cue.name);
        putFloat(out, cue.framesPerSecond);
        out.push_back(cue.loop ? 1 : 0);
        putFloat(out, cue.transform.offsetX);
        putFloat(out, cue.transform.offsetY);
        putFloat(out, cue.transform.scale);
        putFloat(out, cue.transform.rotationTurns);
        putFloat(out, cue.transform.brightness);
        putFloat(out, cue.spinTurnsPerSec);
        putBytes(out, ilda::writeIlda(cue.frames));
    }

    // Timeline block (version 3+).
    putString(out, show.timeline.name);
    putDouble(out, show.timeline.durationSeconds);
    out.push_back(show.timeline.loop ? 1 : 0);
    putU32(out, static_cast<std::uint32_t>(show.timeline.steps.size()));
    for (const TimelineStep& step : show.timeline.steps) {
        putDouble(out, step.timeSeconds);
        putU32(out, static_cast<std::uint32_t>(step.cueIndex));
    }
    return out;
}

ShowParseResult readShow(const std::vector<std::uint8_t>& bytes) {
    ShowParseResult result;
    if (bytes.size() < 8 || std::memcmp(bytes.data(), kMagic, 4) != 0) {
        result.error = "not a RedFox show file (bad magic)";
        return result;
    }

    Reader r{bytes.data() + 4, bytes.size() - 4};
    const std::uint32_t version = r.u32();
    if (version < kMinReadableVersion || version > kVersion) {
        result.error = "unsupported show file version";
        return result;
    }

    result.show.name = r.str();
    const std::uint32_t cueCount = r.u32();
    for (std::uint32_t i = 0; i < cueCount && r.ok; ++i) {
        Cue cue;
        cue.name = r.str();
        cue.framesPerSecond = r.f32();
        if (r.remaining < 1) {
            r.ok = false;
            break;
        }
        cue.loop = (*r.p++ != 0);
        r.remaining -= 1;
        cue.transform.offsetX = r.f32();
        cue.transform.offsetY = r.f32();
        cue.transform.scale = r.f32();
        cue.transform.rotationTurns = r.f32();
        cue.transform.brightness = r.f32();
        cue.spinTurnsPerSec = r.f32();

        const std::vector<std::uint8_t> ildaBytes = r.bytes();
        if (!r.ok) {
            break;
        }
        const ilda::ParseResult frames = ilda::readIlda(ildaBytes);
        if (!frames.ok) {
            result.error = "embedded ILDA parse failed: " + frames.error;
            return result;
        }
        cue.frames = frames.frames;
        result.show.cues.push_back(std::move(cue));
    }

    // Timeline block: present from version 3 on; v2 shows simply have none.
    if (r.ok && version >= 3) {
        result.show.timeline.name = r.str();
        result.show.timeline.durationSeconds = r.f64();
        if (r.remaining < 1) {
            r.ok = false;
        } else {
            result.show.timeline.loop = (*r.p++ != 0);
            r.remaining -= 1;
            const std::uint32_t stepCount = r.u32();
            for (std::uint32_t i = 0; i < stepCount && r.ok; ++i) {
                TimelineStep step;
                step.timeSeconds = r.f64();
                step.cueIndex = r.u32();
                result.show.timeline.steps.push_back(step);
            }
        }
    }

    if (!r.ok) {
        result.error = "truncated show file";
        return result;
    }
    result.ok = true;
    return result;
}

bool writeShowFile(const std::string& path, const Show& show) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    const std::vector<std::uint8_t> bytes = writeShow(show);
    file.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(file);
}

std::string defaultShowFilePath() {
    const char* appData = std::getenv("APPDATA");
    if (appData == nullptr || *appData == '\0') {
        return "show.rfsh";
    }
    const std::filesystem::path dir = std::filesystem::path(appData) / "RedFoxLaser";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        return "show.rfsh";
    }
    return (dir / "show.rfsh").string();
}

ShowParseResult readShowFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        ShowParseResult result;
        result.error = "could not open file: " + path;
        return result;
    }
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
    return readShow(bytes);
}

} // namespace redfox::show
