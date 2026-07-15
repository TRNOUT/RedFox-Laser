#include "ilda/IldaCodec.hpp"

#include <cmath>
#include <fstream>
#include <string>
#include <utility>

namespace redfox::ilda {
namespace {

constexpr float kCoordScale = 32767.0f;

struct Rgb {
    std::uint8_t r, g, b;
};

// The standard ILDA default 64-colour palette, used to resolve indexed frames
// when no explicit palette (format-2) section precedes them.
const Rgb kDefaultPalette[64] = {
    {0, 0, 0},       {255, 255, 255}, {255, 0, 0},     {255, 255, 0},
    {0, 255, 0},     {0, 255, 255},   {0, 0, 255},     {255, 0, 255},
    {255, 128, 128}, {255, 140, 128}, {255, 151, 128}, {255, 163, 128},
    {255, 174, 128}, {255, 186, 128}, {255, 197, 128}, {255, 209, 128},
    {255, 220, 128}, {255, 232, 128}, {255, 243, 128}, {255, 255, 128},
    {243, 255, 128}, {232, 255, 128}, {220, 255, 128}, {209, 255, 128},
    {197, 255, 128}, {186, 255, 128}, {174, 255, 128}, {163, 255, 128},
    {151, 255, 128}, {140, 255, 128}, {128, 255, 128}, {128, 255, 140},
    {128, 255, 151}, {128, 255, 163}, {128, 255, 174}, {128, 255, 186},
    {128, 255, 197}, {128, 255, 209}, {128, 255, 220}, {128, 255, 232},
    {128, 255, 243}, {128, 255, 255}, {128, 243, 255}, {128, 232, 255},
    {128, 220, 255}, {128, 209, 255}, {128, 197, 255}, {128, 186, 255},
    {128, 174, 255}, {128, 163, 255}, {128, 151, 255}, {128, 140, 255},
    {128, 128, 255}, {140, 128, 255}, {151, 128, 255}, {163, 128, 255},
    {174, 128, 255}, {186, 128, 255}, {197, 128, 255}, {209, 128, 255},
    {220, 128, 255}, {232, 128, 255}, {243, 128, 255}, {255, 128, 255},
};

std::int16_t toInt16(float v) {
    float s = v * kCoordScale;
    if (s > 32767.0f) s = 32767.0f;
    if (s < -32768.0f) s = -32768.0f;
    return static_cast<std::int16_t>(std::lround(s));
}

float fromInt16(std::int16_t v) { return static_cast<float>(v) / kCoordScale; }

void putU16BE(std::vector<std::uint8_t>& out, std::uint16_t v) {
    out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>(v & 0xFF));
}

void putI16BE(std::vector<std::uint8_t>& out, std::int16_t v) {
    putU16BE(out, static_cast<std::uint16_t>(v));
}

std::uint16_t getU16BE(const std::uint8_t* p) {
    return static_cast<std::uint16_t>((p[0] << 8) | p[1]);
}

std::int16_t getI16BE(const std::uint8_t* p) {
    return static_cast<std::int16_t>(getU16BE(p));
}

void putName(std::vector<std::uint8_t>& out, const std::string& s) {
    for (int i = 0; i < 8; ++i) {
        out.push_back(i < static_cast<int>(s.size()) ? static_cast<std::uint8_t>(s[i]) : 0);
    }
}

std::string readName(const std::uint8_t* p) {
    std::string s;
    for (int i = 0; i < 8; ++i) {
        const char c = static_cast<char>(p[i]);
        if (c == '\0') break;
        s.push_back(c);
    }
    while (!s.empty() && s.back() == ' ') {
        s.pop_back();
    }
    return s;
}

void writeHeader(std::vector<std::uint8_t>& out, std::uint8_t format,
                 const std::string& name, const std::string& company,
                 std::uint16_t records, std::uint16_t frameNumber,
                 std::uint16_t totalFrames) {
    out.push_back('I');
    out.push_back('L');
    out.push_back('D');
    out.push_back('A');
    out.push_back(0);
    out.push_back(0);
    out.push_back(0);
    out.push_back(format);
    putName(out, name);
    putName(out, company);
    putU16BE(out, records);
    putU16BE(out, frameNumber);
    putU16BE(out, totalFrames);
    out.push_back(0); // projector/scanner number
    out.push_back(0); // reserved
}

} // namespace

std::vector<std::uint8_t> writeIlda(const IldaShow& show) {
    std::vector<std::uint8_t> out;
    const std::uint16_t total = static_cast<std::uint16_t>(show.size());

    for (std::size_t f = 0; f < show.size(); ++f) {
        const IldaFrame& frame = show[f];
        const std::uint8_t format = frame.is3D ? 4 : 5; // true colour
        const std::uint16_t count = static_cast<std::uint16_t>(frame.points.size());
        writeHeader(out, format, frame.name, frame.company, count,
                    static_cast<std::uint16_t>(f), total);

        for (std::size_t i = 0; i < frame.points.size(); ++i) {
            const IldaPoint& p = frame.points[i];
            putI16BE(out, toInt16(p.x));
            putI16BE(out, toInt16(p.y));
            if (frame.is3D) {
                putI16BE(out, toInt16(p.z));
            }
            std::uint8_t status = 0;
            if (p.blanked) {
                status |= 0x40;
            }
            if (i + 1 == frame.points.size()) {
                status |= 0x80; // last-point flag
            }
            out.push_back(status);
            // ILDA true-colour byte order is B, G, R.
            out.push_back(p.b);
            out.push_back(p.g);
            out.push_back(p.r);
        }
    }

    writeHeader(out, 5, "", "", 0, 0, total); // zero-record EOF header
    return out;
}

ParseResult readIlda(const std::vector<std::uint8_t>& bytes) {
    ParseResult result;
    const std::size_t n = bytes.size();
    std::size_t pos = 0;
    bool sawAnyHeader = false;

    // Current palette for indexed formats; starts as the ILDA default and is
    // replaced by any format-2 palette section encountered.
    std::vector<Rgb> palette(kDefaultPalette, kDefaultPalette + 64);

    while (pos + 32 <= n) {
        const std::uint8_t* h = bytes.data() + pos;
        if (!(h[0] == 'I' && h[1] == 'L' && h[2] == 'D' && h[3] == 'A')) {
            result.ok = false;
            result.error = "missing ILDA header signature";
            return result;
        }
        sawAnyHeader = true;

        const std::uint8_t format = h[7];
        const std::string name = readName(h + 8);
        const std::string company = readName(h + 16);
        const std::uint16_t records = getU16BE(h + 24);
        pos += 32;

        if (records == 0) {
            result.ok = true; // EOF header
            return result;
        }

        // Format 2 is a palette section (R,G,B triples), not a frame.
        if (format == 2) {
            if (pos + static_cast<std::size_t>(records) * 3 > n) {
                result.ok = false;
                result.error = "truncated ILDA palette section";
                return result;
            }
            palette.clear();
            palette.reserve(records);
            for (std::uint16_t i = 0; i < records; ++i) {
                const std::uint8_t* c = bytes.data() + pos;
                palette.push_back(Rgb{c[0], c[1], c[2]});
                pos += 3;
            }
            continue;
        }

        std::size_t pointSize = 0;
        bool is3D = false;
        bool indexed = false;
        switch (format) {
            case 0: pointSize = 8; is3D = true; indexed = true; break;   // 3D indexed
            case 1: pointSize = 6; is3D = false; indexed = true; break;  // 2D indexed
            case 4: pointSize = 10; is3D = true; indexed = false; break; // 3D true colour
            case 5: pointSize = 8; is3D = false; indexed = false; break; // 2D true colour
            default:
                result.ok = false;
                result.error = "unsupported ILDA format code " + std::to_string(format);
                return result;
        }

        if (pos + pointSize * records > n) {
            result.ok = false;
            result.error = "truncated ILDA point data";
            return result;
        }

        IldaFrame frame;
        frame.name = name;
        frame.company = company;
        frame.is3D = is3D;
        frame.points.reserve(records);

        for (std::uint16_t i = 0; i < records; ++i) {
            const std::uint8_t* r = bytes.data() + pos;
            IldaPoint p;
            p.x = fromInt16(getI16BE(r));
            r += 2;
            p.y = fromInt16(getI16BE(r));
            r += 2;
            if (is3D) {
                p.z = fromInt16(getI16BE(r));
                r += 2;
            }
            const std::uint8_t status = *r++;
            p.blanked = (status & 0x40) != 0;
            if (indexed) {
                const std::uint8_t index = *r++;
                if (!palette.empty()) {
                    const Rgb& c = palette[index < palette.size() ? index : palette.size() - 1];
                    p.r = c.r;
                    p.g = c.g;
                    p.b = c.b;
                }
            } else {
                p.b = *r++;
                p.g = *r++;
                p.r = *r++;
            }
            frame.points.push_back(p);
            pos += pointSize;
        }
        result.frames.push_back(std::move(frame));
    }

    if (!sawAnyHeader) {
        result.ok = false;
        result.error = "input too small to contain an ILDA header";
        return result;
    }

    result.ok = true; // ended without an explicit EOF header; accept what we parsed
    return result;
}

bool writeIldaFile(const std::string& path, const IldaShow& show) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    const std::vector<std::uint8_t> bytes = writeIlda(show);
    file.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(file);
}

ParseResult readIldaFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        ParseResult result;
        result.ok = false;
        result.error = "could not open file: " + path;
        return result;
    }
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
    return readIlda(bytes);
}

} // namespace redfox::ilda
