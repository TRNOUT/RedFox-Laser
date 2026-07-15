#include "show/ShowFile.hpp"

#include "ilda/IldaCodec.hpp"

#include <cstring>
#include <fstream>

namespace redfox::show {
namespace {

constexpr char kMagic[4] = {'R', 'F', 'S', 'H'};
constexpr std::uint32_t kVersion = 1;

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
        putBytes(out, ilda::writeIlda(cue.frames));
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
    if (version != kVersion) {
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
