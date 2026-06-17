// MIT License
//
// Copyright (c) 2026 Code Life
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// Jokes Image
//     by White Field Studios
//     generates a .jpg image of a joke
// Description: Outputs a joke as a framed .jpg image file using stb_truetype + stb_image_write.
//              Requires: stb_truetype.h, stb_image_write.h (drop in project directory)
//              Usage: ./jokes_image -p <number> [-o <filename>] [-f <fontpath>] [-t <theme>] [--noborder]

#define STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_truetype.h"
#include "stb_image_write.h"
#include "stb_image.h"

#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <cstdint>
#include "jokes_data.h"

using namespace std;

static const char* VERSION = "2.4";
static const char* CONFIG_FILE = ".jokes_image";

// ── Config file (.jokes_image in current directory) ───────────────────────────

static map<string, string> loadConfig() {
    map<string, string> cfg;
    ifstream f(CONFIG_FILE);
    if (!f.good()) return cfg;
    string line;
    while (getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        size_t s = line.find_first_not_of(" \t");
        if (s == string::npos || line[s] == '#') continue;
        line = line.substr(s);
        size_t eq = line.find('=');
        if (eq == string::npos) continue;
        string key = line.substr(0, eq);
        string val = line.substr(eq + 1);
        size_t ke = key.find_last_not_of(" \t");
        if (ke != string::npos) key = key.substr(0, ke + 1);
        size_t vs = val.find_first_not_of(" \t");
        val = (vs != string::npos) ? val.substr(vs) : "";
        if (!key.empty()) cfg[key] = val;
    }
    return cfg;
}

static string cfgGet(const map<string, string>& cfg, const string& key, const string& defaultVal) {
    auto it = cfg.find(key);
    return (it != cfg.end()) ? it->second : defaultVal;
}

// ----------------------------------------------------------------------------
// Theme
// ----------------------------------------------------------------------------

struct Color { uint8_t r, g, b; };

struct Theme {
    Color bg;        // background fill
    Color border;    // outer frame
    Color setup;     // setup text
    Color punchline; // punchline text
    Color label;     // footer label
};

static const map<string, Theme> THEMES = {
    // name        bg               border           setup            punchline        label
    { "classic", { {255,255,255}, {50, 100, 200}, {0,  140, 180}, {30, 160,  60}, {140,140,140} } },
    { "dark",    { {30,  30,  40}, {80, 160, 240}, {80, 200, 240}, {80, 210, 120}, {170,170,170} } },
    { "sunset",  { {255,240,220}, {200,  80,  40}, {180,  60,  20}, {210, 120,  10}, {160,100, 60} } },
    { "ocean",   { {220,240,255}, {20,  80, 160}, {10, 100, 180}, {20, 140, 160}, {100,140,170} } },
    { "retro",   { {255,255,200}, {160,  80,   0}, {120,  60,   0}, {180, 100,   0}, {140,120, 60} } },
    { "night",   { {15,  15,  30}, {100,  60, 200}, {160, 100, 255}, {100, 220, 180}, {130,120,160} } },
    { "white",   { {30,  30,  40}, {255, 255, 255}, {255, 255, 255}, {255, 255, 255}, {200,200,200} } },
    { "forest",  { {26,  58,  26}, {200, 160,  64}, {128, 224,  64}, {232, 232, 128}, {160,180,100} } },
    { "candy",   { {255,  26, 140}, {255, 255, 255}, {255, 255,  64}, { 64, 255, 255}, {255,180,220} } },
    { "neon",    { {0,    0,   0}, {  0, 255,  65}, {  0, 255,  65}, {255,   0, 160}, {100,200,100} } },
    { "parchment",{ {245, 230, 200}, {139,  94,  42}, { 90,  48,  16}, {139,  26,  16}, {160,120, 80} } },
    { "ice",     { {232, 244, 255}, { 64, 128, 192}, { 16,  64, 160}, {  0,  96, 128}, { 80,140,180} } },
    { "dusk",    { {26,  10,  48}, {224, 112,  48}, {192, 128, 255}, {255, 176,  96}, {160,120,180} } },
    // --- dark background themes ---
    { "ember",    { {20,  12,   8}, {200,  80,  20}, {240, 140,  40}, {255,  80,  20}, {160,100, 60} } },
    { "midnight", { { 8,  12,  40}, {200, 170,  60}, {220, 220, 240}, {200, 170,  60}, {100,120,160} } },
    { "galaxy",   { {12,   8,  28}, {160,  60, 255}, {200, 160, 255}, {255,  80, 200}, {140,100,180} } },
    { "coffee",   { {28,  16,   8}, {180, 130,  70}, {240, 220, 190}, {210, 160,  80}, {150,110, 70} } },
    { "slate",    { {25,  35,  50}, { 60, 180, 220}, {180, 210, 240}, { 60, 200, 220}, {120,150,170} } },
};

static void listThemes() {
    cout << "Available themes: ";
    bool first = true;
    for (auto& kv : THEMES) { if (!first) cout << ", "; cout << kv.first; first = false; }
    cout << "\n";
}

// ----------------------------------------------------------------------------
// Image
// ----------------------------------------------------------------------------

struct Image {
    int w, h;
    vector<uint8_t> pixels; // RGB, 3 bytes per pixel

    Image(int w, int h, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255)
        : w(w), h(h), pixels(w * h * 3) {
        for (int i = 0; i < w * h; i++) {
            pixels[i*3+0] = r;
            pixels[i*3+1] = g;
            pixels[i*3+2] = b;
        }
    }

    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (x < 0 || x >= w || y < 0 || y >= h) return;
        pixels[(y * w + x) * 3 + 0] = r;
        pixels[(y * w + x) * 3 + 1] = g;
        pixels[(y * w + x) * 3 + 2] = b;
    }

    void blendGlyph(const uint8_t* bitmap, int bw, int bh, int px, int py,
                    uint8_t r, uint8_t g, uint8_t b) {
        for (int row = 0; row < bh; row++) {
            for (int col = 0; col < bw; col++) {
                uint8_t alpha = bitmap[row * bw + col];
                if (alpha == 0) continue;
                int ix = px + col, iy = py + row;
                if (ix < 0 || ix >= w || iy < 0 || iy >= h) continue;
                float a = alpha / 255.0f;
                uint8_t* p = &pixels[(iy * w + ix) * 3];
                p[0] = (uint8_t)(p[0] * (1 - a) + r * a);
                p[1] = (uint8_t)(p[1] * (1 - a) + g * a);
                p[2] = (uint8_t)(p[2] * (1 - a) + b * a);
            }
        }
    }

    void drawRect(int x0, int y0, int x1, int y1,
                  uint8_t r, uint8_t g, uint8_t b, int thickness = 4,
                  int gapX0 = -1, int gapX1 = -1) {
        for (int t = 0; t < thickness; t++) {
            for (int x = x0 + t; x <= x1 - t; x++) {
                setPixel(x, y0 + t, r, g, b);
                if (gapX0 < 0 || x < gapX0 || x > gapX1)
                    setPixel(x, y1 - t, r, g, b);
            }
            for (int y = y0 + t; y <= y1 - t; y++) {
                setPixel(x0 + t, y, r, g, b);
                setPixel(x1 - t, y, r, g, b);
            }
        }
    }

    void drawRoundedRect(int x0, int y0, int x1, int y1,
                         uint8_t r, uint8_t g, uint8_t b, int thickness = 4, int radius = 30,
                         int gapX0 = -1, int gapX1 = -1) {
        for (int t = 0; t < thickness; t++) {
            int ax0 = x0 + t, ay0 = y0 + t;
            int ax1 = x1 - t, ay1 = y1 - t;
            int rad = radius - t;
            if (rad < 1) rad = 1;

            // Straight edges (skip the corner arc regions)
            for (int x = ax0 + rad; x <= ax1 - rad; x++) {
                setPixel(x, ay0, r, g, b);
                if (gapX0 < 0 || x < gapX0 || x > gapX1)
                    setPixel(x, ay1, r, g, b);
            }
            for (int y = ay0 + rad; y <= ay1 - rad; y++) {
                setPixel(ax0, y, r, g, b);
                setPixel(ax1, y, r, g, b);
            }

            // Quarter-circle arcs at each corner via midpoint circle algorithm
            auto arc = [&](int ccx, int ccy, int quad) {
                int cx = 0, cy = rad, d = 1 - rad;
                while (cx <= cy) {
                    switch (quad) {
                        case 0: setPixel(ccx-cy, ccy-cx,r,g,b); setPixel(ccx-cx, ccy-cy,r,g,b); break; // top-left
                        case 1: setPixel(ccx+cy, ccy-cx,r,g,b); setPixel(ccx+cx, ccy-cy,r,g,b); break; // top-right
                        case 2: setPixel(ccx+cy, ccy+cx,r,g,b); setPixel(ccx+cx, ccy+cy,r,g,b); break; // bottom-right
                        case 3: setPixel(ccx-cy, ccy+cx,r,g,b); setPixel(ccx-cx, ccy+cy,r,g,b); break; // bottom-left
                    }
                    if (d < 0) d += 2*cx + 3;
                    else { d += 2*(cx-cy) + 5; cy--; }
                    cx++;
                }
            };

            arc(ax0+rad, ay0+rad, 0); // top-left
            arc(ax1-rad, ay0+rad, 1); // top-right
            arc(ax1-rad, ay1-rad, 2); // bottom-right
            arc(ax0+rad, ay1-rad, 3); // bottom-left
        }
    }

    bool writeJpeg(const string& filename, int quality = 90) const {
        return stbi_write_jpg(filename.c_str(), w, h, 3, pixels.data(), quality) != 0;
    }

    // Load a JPEG/PNG and scale it to fill this image's dimensions (nearest-neighbor)
    bool loadBackground(const string& path) {
        int srcW, srcH, channels;
        uint8_t* data = stbi_load(path.c_str(), &srcW, &srcH, &channels, 3);
        if (!data) {
            cerr << "Error: could not load background image: " << path << "\n";
            return false;
        }
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int sx = x * srcW / w;
                int sy = y * srcH / h;
                uint8_t* src = data + (sy * srcW + sx) * 3;
                uint8_t* dst = pixels.data() + (y * w + x) * 3;
                dst[0] = src[0];
                dst[1] = src[1];
                dst[2] = src[2];
            }
        }
        stbi_image_free(data);
        return true;
    }

    // Load a PNG with alpha and alpha-blend it on top of the current pixels (scaled to canvas)
    bool loadForeground(const string& path) {
        int srcW, srcH, channels;
        uint8_t* data = stbi_load(path.c_str(), &srcW, &srcH, &channels, 4); // force RGBA
        if (!data) {
            cerr << "Error: could not load foreground image: " << path << "\n";
            return false;
        }
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int sx = x * srcW / w;
                int sy = y * srcH / h;
                uint8_t* src = data + (sy * srcW + sx) * 4;
                float a = src[3] / 255.0f;
                if (a == 0.0f) continue;
                uint8_t* dst = pixels.data() + (y * w + x) * 3;
                dst[0] = (uint8_t)(src[0] * a + dst[0] * (1.0f - a));
                dst[1] = (uint8_t)(src[1] * a + dst[1] * (1.0f - a));
                dst[2] = (uint8_t)(src[2] * a + dst[2] * (1.0f - a));
            }
        }
        stbi_image_free(data);
        return true;
    }

    // Darken the entire image by a factor in [0,1] to improve text legibility over a background
    void applyDimOverlay(float factor = 0.45f) {
        for (auto& p : pixels)
            p = (uint8_t)(p * (1.0f - factor));
    }
};

// ----------------------------------------------------------------------------
// Font
// ----------------------------------------------------------------------------

// Decode one UTF-8 codepoint from str at byte position i; advances i past it.
// Returns the codepoint, or '?' for invalid sequences.
static int nextCodepoint(const string& str, int& i) {
    unsigned char b = str[i];
    int cp;
    int extra;
    if      (b < 0x80) { cp = b;         extra = 0; }
    else if (b < 0xE0) { cp = b & 0x1F;  extra = 1; }
    else if (b < 0xF0) { cp = b & 0x0F;  extra = 2; }
    else               { cp = b & 0x07;  extra = 3; }
    i++;
    for (int e = 0; e < extra; e++) {
        if (i >= (int)str.size()) return '?';
        cp = (cp << 6) | (str[i++] & 0x3F);
    }
    return cp;
}

struct Font {
    stbtt_fontinfo info;
    vector<uint8_t> buffer;
    float scale = 1.0f;

    bool load(const string& path, float pixelHeight) {
        ifstream f(path, ios::binary | ios::ate);
        if (!f) return false;
        buffer.resize(f.tellg());
        f.seekg(0);
        f.read((char*)buffer.data(), buffer.size());
        int offset = stbtt_GetFontOffsetForIndex(buffer.data(), 0);
        if (offset < 0) return false;
        if (!stbtt_InitFont(&info, buffer.data(), offset)) return false;
        scale = stbtt_ScaleForPixelHeight(&info, pixelHeight);
        return true;
    }

    int textWidth(const string& text) const {
        int width = 0;
        for (int i = 0; i < (int)text.size(); ) {
            int cp = nextCodepoint(text, i);
            int advance, lsb;
            stbtt_GetCodepointHMetrics(&info, cp, &advance, &lsb);
            width += (int)(advance * scale);
        }
        return width;
    }

    void drawText(Image& img, const string& text, int x, int y,
                  uint8_t r, uint8_t g, uint8_t b, bool bold = false) const {
        int cursor = x;
        int prev = 0;
        for (int i = 0; i < (int)text.size(); ) {
            int cp = nextCodepoint(text, i);
            if (prev)
                cursor += (int)(stbtt_GetCodepointKernAdvance(&info, prev, cp) * scale);

            int advance, lsb;
            stbtt_GetCodepointHMetrics(&info, cp, &advance, &lsb);

            int x0, y0, x1, y1;
            stbtt_GetCodepointBitmapBox(&info, cp, scale, scale, &x0, &y0, &x1, &y1);

            int gw = x1 - x0, gh = y1 - y0;
            if (gw > 0 && gh > 0) {
                vector<uint8_t> bitmap(gw * gh);
                stbtt_MakeCodepointBitmap(&info, bitmap.data(), gw, gh, gw, scale, scale, cp);
                img.blendGlyph(bitmap.data(), gw, gh, cursor + x0, y + y0, r, g, b);
                if (bold)
                    img.blendGlyph(bitmap.data(), gw, gh, cursor + x0 + 1, y + y0, r, g, b);
            }
            cursor += (int)(advance * scale);
            prev = cp;
        }
    }

    void drawTextCentered(Image& img, const string& text, int imgWidth, int y,
                          uint8_t r, uint8_t g, uint8_t b, bool bold = false) const {
        int x = (imgWidth - textWidth(text)) / 2;
        drawText(img, text, x, y, r, g, b, bold);
    }

    // Word-wrap a single segment (no newlines) into lines that fit within maxWidth
    vector<string> wrapSegment(const string& text, int maxWidth) const {
        vector<string> lines;
        string current;
        size_t i = 0;
        while (i <= text.size()) {
            size_t space = text.find(' ', i);
            if (space == string::npos) space = text.size();
            string word = text.substr(i, space - i);
            string candidate = current.empty() ? word : current + " " + word;
            if (!current.empty() && textWidth(candidate) > maxWidth) {
                lines.push_back(current);
                current = word;
            } else {
                current = candidate;
            }
            i = space + 1;
        }
        if (!current.empty()) lines.push_back(current);
        return lines;
    }

    // Word-wrap text into lines; \n forces a line break before wrapping resumes
    vector<string> wrapText(const string& text, int maxWidth) const {
        vector<string> lines;
        size_t start = 0;
        while (true) {
            size_t nl = text.find("\\n", start);
            string segment = text.substr(start, nl == string::npos ? string::npos : nl - start);
            auto wrapped = wrapSegment(segment, maxWidth);
            lines.insert(lines.end(), wrapped.begin(), wrapped.end());
            if (nl == string::npos) break;
            start = nl + 2; // skip past the two-char \n
        }
        return lines;
    }
};

// ----------------------------------------------------------------------------
// Joke image renderer
// ----------------------------------------------------------------------------

bool jokeToJpeg(const Joke& joke, int jokeNumber, const string& filename,
                const string& fontPath, const Theme& theme, bool noborder = false,
                const string& bgPath = "", bool nodim = false, bool roundedcorners = false,
                bool nofooter = false, const string& labelOverride = "",
                bool hasLabelOverride = false, float textScale = 1.0f,
                const string& fgPath = "", const string& halign = "center",
                const string& valign = "middle",
                const string& setupFontPath = "",
                const string& punchlineFontPath = "",
                const string& footerFontPath = "",
                bool nopunchline = false,
                int imgW = 800, int imgH = 500) {
    const int W = imgW, H = imgH;
    // Scale layout constants proportionally to width (base: 800px wide)
    const float S        = imgW / 800.0f;
    const int MARGIN    = (int)(50 * S);
    const int FRAME     = max(4, (int)(8  * S));
    const int THICKNESS = max(2, (int)(5  * S));
    const int LINE_H    = (int)(54 * textScale * S);
    const float FONT_SIZE       = 36.0f * textScale * S;
    const float FONT_SIZE_SMALL = 18.0f * textScale * S;

    Image img(W, H, theme.bg.r, theme.bg.g, theme.bg.b);

    if (!bgPath.empty()) {
        if (!img.loadBackground(bgPath)) return false;
        if (!nodim) img.applyDimOverlay();
    }

    if (!fgPath.empty()) {
        if (!img.loadForeground(fgPath)) return false;
    }

    // Resolve per-role font paths, falling back to -f for any not specified
    const string& setupFP    = setupFontPath.empty()     ? fontPath : setupFontPath;
    const string& punchFP    = punchlineFontPath.empty() ? fontPath : punchlineFontPath;
    const string& footerFP   = footerFontPath.empty()    ? fontPath : footerFontPath;

    Font setupFont;
    if (!setupFont.load(setupFP, FONT_SIZE)) {
        cerr << "Error: could not load setup font: " << setupFP << "\n";
        return false;
    }
    Font punchFont;
    if (!punchFont.load(punchFP, FONT_SIZE)) {
        cerr << "Error: could not load punchline font: " << punchFP << "\n";
        return false;
    }

    // Resolve label text; empty string (explicit or nofooter) suppresses the footer
    Font smallFont;
    string label = hasLabelOverride ? labelOverride
                                    : "Groaners & Grins   #" + to_string(jokeNumber) + "  [" + joke.type + "]";
    // Substitute [type] and #N tokens in custom labels
    if (hasLabelOverride && !label.empty()) {
        string type_val = joke.type.empty() ? "" : joke.type;
        string num_val  = jokeNumber > 0 ? to_string(jokeNumber) : "";
        size_t pos;
        while ((pos = label.find("[type]")) != string::npos)
            label.replace(pos, 6, type_val);
        while ((pos = label.find("#N")) != string::npos)
            label.replace(pos, 2, num_val);
    }
    bool showFooter = !nofooter && !label.empty();
    int gapX0 = -1, gapX1 = -1;
    int labelY = H - FRAME - THICKNESS / 2 + 6;
    if (showFooter && smallFont.load(footerFP, FONT_SIZE_SMALL)) {
        const int GAP_PAD = 12;
        int lw = smallFont.textWidth(label);
        gapX0 = (W - lw) / 2 - GAP_PAD;
        gapX1 = (W + lw) / 2 + GAP_PAD;
    }

    // Outer frame (gap only if footer is shown)
    if (!noborder) {
        if (roundedcorners)
            img.drawRoundedRect(FRAME, FRAME, W - FRAME, H - FRAME,
                                theme.border.r, theme.border.g, theme.border.b, THICKNESS,
                                30, gapX0, gapX1);
        else
            img.drawRect(FRAME, FRAME, W - FRAME, H - FRAME,
                         theme.border.r, theme.border.g, theme.border.b, THICKNESS,
                         gapX0, gapX1);
    }

    int textMaxWidth = W - MARGIN * 2;
    const int TEXT_GAP = 20; // gap between setup and punchline blocks

    // Pre-wrap both blocks so we know total height before drawing
    auto setupLines = setupFont.wrapText(
        joke.type == "knock-knock"
            ? "Knock knock! Who's there? " + joke.setup + "  " + joke.setup + " who?"
            : joke.setup,
        textMaxWidth);
    // Always wrap punchline for height calculation so setup stays at the same Y
    // in both the setup-only frame and the full frame (used by --video).
    auto punchLines = punchFont.wrapText(joke.punchline, textMaxWidth);

    int totalTextH = (int)setupLines.size() * LINE_H
                   + (punchLines.empty() ? 0 : (int)punchLines.size() * LINE_H + TEXT_GAP);

    // Vertical start position
    const int topBound    = FRAME + THICKNESS + MARGIN;
    const int bottomBound = H - FRAME - THICKNESS - MARGIN - (showFooter ? (int)FONT_SIZE_SMALL : 0);
    int y;
    if (valign == "middle")
        y = topBound + (bottomBound - topBound - totalTextH) / 2 + (int)FONT_SIZE;
    else if (valign == "bottom")
        y = bottomBound - totalTextH + (int)FONT_SIZE;
    else // top
        y = topBound + (int)FONT_SIZE;

    // Helper: draw one line with the given font and halign
    auto drawLine = [&](const Font& f, const string& text, int lineY,
                        uint8_t r, uint8_t g, uint8_t b) {
        if (halign == "left")
            f.drawText(img, text, MARGIN, lineY, r, g, b, true);
        else if (halign == "right")
            f.drawText(img, text, W - MARGIN - f.textWidth(text), lineY, r, g, b, true);
        else
            f.drawTextCentered(img, text, W, lineY, r, g, b, true);
    };

    // Draw setup
    for (auto& line : setupLines) {
        drawLine(setupFont, line, y, theme.setup.r, theme.setup.g, theme.setup.b);
        y += LINE_H;
    }

    // Draw punchline (suppressed in setup-only frame for --video)
    if (!nopunchline) {
        y += TEXT_GAP;
        for (auto& line : punchLines) {
            drawLine(punchFont, line, y, theme.punchline.r, theme.punchline.g, theme.punchline.b);
            y += LINE_H;
        }
    }

    // Label centered in the gap on the bottom border
    if (showFooter && smallFont.load(footerFP, FONT_SIZE_SMALL)) {
        smallFont.drawTextCentered(img, label, W, labelY, theme.label.r, theme.label.g, theme.label.b);
    }

    if (!img.writeJpeg(filename)) {
        cerr << "Error: could not write image: " << filename << "\n";
        return false;
    }

    cout << "Saved: " << filename << "\n";
    return true;
}

// ----------------------------------------------------------------------------
// Font path detection (platform defaults)
// ----------------------------------------------------------------------------

string defaultFontPath() {
#if defined(_WIN32)
    return "C:/Windows/Fonts/arial.ttf";
#elif defined(__APPLE__)
    return "/System/Library/Fonts/Helvetica.ttc";
#else
    // Linux
    return "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
#endif
}

// ----------------------------------------------------------------------------
// Color parser  (named color, #rrggbb, or rrggbb)
// ----------------------------------------------------------------------------

static bool parseColor(const string& s, Color& out) {
    // Named colors
    static const map<string, Color> NAMED = {
        { "black",   {  0,   0,   0} },
        { "white",   {255, 255, 255} },
        { "red",     {255,   0,   0} },
        { "green",   {  0, 200,   0} },
        { "blue",    {  0,   0, 255} },
        { "yellow",  {255, 255,   0} },
        { "orange",  {255, 165,   0} },
        { "purple",  {128,   0, 128} },
        { "pink",    {255, 105, 180} },
        { "cyan",    {  0, 255, 255} },
        { "magenta", {255,   0, 255} },
        { "gray",    {128, 128, 128} },
        { "grey",    {128, 128, 128} },
        { "silver",  {192, 192, 192} },
        { "brown",   {139,  69,  19} },
        { "gold",    {255, 215,   0} },
        { "lime",    {  0, 255,   0} },
        { "teal",    {  0, 128, 128} },
        { "navy",    {  0,   0, 128} },
        { "maroon",  {128,   0,   0} },
        { "olive",   {128, 128,   0} },
        { "coral",   {255,  80,  60} },
        { "salmon",  {250, 128, 114} },
        { "violet",  {238, 130, 238} },
        { "indigo",  { 75,   0, 130} },
    };
    // Case-insensitive lookup
    string lower = s;
    for (auto& c : lower) c = (char)tolower((unsigned char)c);
    auto it = NAMED.find(lower);
    if (it != NAMED.end()) { out = it->second; return true; }
    // Hex fallback
    string h = s;
    if (!h.empty() && h[0] == '#') h = h.substr(1);
    if (h.size() != 6) return false;
    try {
        out.r = (uint8_t)stoul(h.substr(0, 2), nullptr, 16);
        out.g = (uint8_t)stoul(h.substr(2, 2), nullptr, 16);
        out.b = (uint8_t)stoul(h.substr(4, 2), nullptr, 16);
    } catch (...) { return false; }
    return true;
}

// ----------------------------------------------------------------------------
// Help
// ----------------------------------------------------------------------------

void displayHelp() {
    cout << "Jokes Image - Generates a .jpg image of a joke\n\n";
    cout << "Usage: ./jokes_image -p <number> [OPTIONS]\n\n";
    cout << "Options:\n";
    cout << "  -p, --pick <number>   Joke number to render (required)\n";
    cout << "  -o, --output <file>   Output filename (default: joke_<number>.jpg)\n";
    cout << "  -f, --font <path>     Path to a .ttf or .ttc font file (fallback for all roles)\n";
    cout << "  --setupfont <path>    Font for setup text (overrides -f for setup)\n";
    cout << "  --punchlinefont <path> Font for punchline text (overrides -f for punchline)\n";
    cout << "  --footerfont <path>   Font for footer label (overrides -f for footer)\n";
    cout << "  -t, --theme <name>    Color theme (default: classic)\n";
    cout << "  -bg, --background <file>  Use a .jpg/.png image as the background layer\n";
    cout << "  -fg, --foreground <file>  Overlay a .png (with transparency) above the background, below text\n";
    cout << "  --nodim               Do not dim the background image (default: dims 45%)\n";
    cout << "  --textsize <percent>  Scale text size (default: 100); e.g. 150 = 150%\n";
    cout << "  --aligntext h,v      Text alignment: h = left|center|right, v = top|middle|bottom\n";
    cout << "                       Default: center,middle  e.g. --aligntext left,top\n";
    cout << "  --nofooter            Omit the footer and leave the border intact\n";
    cout << "  --footer \"text\"       Override the footer text; use \"\" to suppress it entirely\n";
    cout << "                        Tokens: [type] = joke type, #N = joke number\n";
    cout << "  --setup \"text\"        Use custom setup text instead of a joke from the data\n";
    cout << "  --punchline \"text\"    Use custom punchline text instead of a joke from the data\n";
    cout << "  --setupcolor <color>      Override the setup text color (name or hex, e.g. red, \"#ff0000\")\n";
    cout << "  --punchlinecolor <color>  Override the punchline text color (name or hex)\n";
    cout << "  --bordercolor <color>     Override the border color (name or hex)\n";
    cout << "                            Named colors: black, white, red, green, blue, yellow, orange,\n";
    cout << "                              purple, pink, cyan, magenta, gray, silver, brown, gold,\n";
    cout << "                              lime, teal, navy, maroon, olive, coral, salmon, violet, indigo\n";
    cout << "  -rc, --roundedcorners Use rounded corners on the border frame\n";
    cout << "  -nb, --noborder       Omit the decorative border frame\n";
    cout << "  --video <seconds>     Generate an MP4: setup shown for N seconds, then punchline for N more\n";
    cout << "                        Requires ffmpeg to be installed and in PATH\n";
    cout << "  --fade <seconds>      Used with --video: add a transition when punchline appears (e.g. 0.8)\n";
    cout << "  --fadehalf            Used with --fade: confine the transition to the bottom half only\n";
    cout << "                        Top half stays completely static; works with any --fadetype\n";
    cout << "  --fadetype <name>     Transition style (default: fade). Options:\n";
    cout << "                          wipe       - reveal from right\n";
    cout << "                          wiperight  - reveal from left\n";
    cout << "                          wipeup     - reveal from bottom\n";
    cout << "                          wipedown   - reveal from top\n";
    cout << "                          fade       - opacity crossfade (default)\n";
    cout << "                          fadeblack  - fade through black\n";
    cout << "                          fadewhite  - fade through white\n";
    cout << "                          slide      - slide in from right\n";
    cout << "                          slideup    - slide in from bottom\n";
    cout << "                          radial     - radial clock-wipe\n";
    cout << "                          circle     - circle opens outward\n";
    cout << "                          pixelize   - pixelate dissolve\n";
    cout << "                          dissolve   - random pixel dissolve\n";
    cout << "                          blur       - horizontal blur\n";
    cout << "                          squeeze    - horizontal squeeze\n";
    cout << "                          zoom       - zoom in reveal\n";
    cout << "                          diagtl/tr  - diagonal from corner\n";
    cout << "  --size WxH            Image dimensions in pixels (default: 800x500)\n";
    cout << "                        e.g. --size 1080x1920 (portrait), --size 1920x1080 (widescreen)\n";
    cout << "                        All layout constants (margins, font, border) scale with width\n";
    cout << "  --saveconfig          Save current settings to .jokes_image as new defaults\n";
    cout << "  --showconfig          Print current effective settings and exit\n";
    cout << "  -v,  --version        Print version and exit\n";
    cout << "  -h, --help            Display this help message\n\n";
    cout << "Config file (.jokes_image in current directory):\n";
    cout << "  Supported keys: font  theme  textsize  size  noborder  nodim  roundedcorners\n";
    cout << "                  nofooter  footer  setupcolor  punchlinecolor  bordercolor\n";
    cout << "                  aligntext  setupfont  punchlinefont  footerfont  background  foreground\n\n";
    cout << "Themes: classic, dark, sunset, ocean, retro, night, white, forest, candy, neon, parchment, ice, dusk\n";
    cout << "        Dark backgrounds: ember, midnight, galaxy, coffee, slate\n";
    cout << "        Use 'all' to render every theme at once\n\n";
    cout << "Examples:\n";
    cout << "  ./jokes_image -p 5                     - Save joke_5.jpg\n";
    cout << "  ./jokes_image -p 5 -o funny.jpg        - Save funny.jpg\n";
    cout << "  ./jokes_image -p 5 -t dark             - Dark theme\n";
    cout << "  ./jokes_image -p 5 --noborder          - No border frame (-nb)\n";
    cout << "  ./jokes_image -p 5 -f /path/to/font.ttf\n";
    cout << "  ./jokes_image -p 5 -bg photo.jpg          - Use photo.jpg as background\n";
    cout << "  ./jokes_image -p 5 -bg photo.jpg -nb      - Background with no border\n";
    cout << "  ./jokes_image -p 5 --nofooter             - No footer\n";
    cout << "  ./jokes_image -p 5 --footer \"My Text\"     - Custom footer text\n";
    cout << "  ./jokes_image -p 5 --footer \"\"            - Suppress footer\n";
    cout << "  ./jokes_image --setup \"Why?\" --punchline \"Because!\"  - Custom text\n";
    cout << "  ./jokes_image -t dark --roundedcorners --saveconfig  - Save dark+rounded as defaults\n";
    cout << "  ./jokes_image --showconfig                           - Show active settings\n";
}

// ----------------------------------------------------------------------------
// main
// ----------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    map<string,string> cfg = loadConfig();

    int    jokeNumber  = -1;
    string outputFile;
    string fontPath    = cfgGet(cfg, "font", defaultFontPath());
    string themeName   = cfgGet(cfg, "theme", "classic");
    bool   noborder       = cfgGet(cfg, "noborder",       "no") == "yes";
    bool   nodim          = cfgGet(cfg, "nodim",          "no") == "yes";
    bool   roundedcorners = cfgGet(cfg, "roundedcorners", "no") == "yes";
    bool   nofooter       = cfgGet(cfg, "nofooter",       "no") == "yes";
    float  textScale      = stof(cfgGet(cfg, "textsize",  "100")) / 100.0f;
    string halign;
    string valign;
    {
        string at = cfgGet(cfg, "aligntext", "center,middle");
        size_t comma = at.find(',');
        halign = (comma != string::npos) ? at.substr(0, comma) : "center";
        valign = (comma != string::npos) ? at.substr(comma + 1) : "middle";
    }
    string setupFontPath     = cfgGet(cfg, "setupfont",     "");
    string punchlineFontPath = cfgGet(cfg, "punchlinefont", "");
    string footerFontPath    = cfgGet(cfg, "footerfont",    "");
    string bgPath            = cfgGet(cfg, "background",    "");
    string fgPath            = cfgGet(cfg, "foreground",    "");
    string customSetup;
    string customPunchline;
    string labelOverride;
    bool   hasLabelOverride  = false;
    if (cfg.count("footer")) {
        labelOverride    = cfg.at("footer");
        hasLabelOverride = true;
    }
    string setupColorHex     = cfgGet(cfg, "setupcolor",     "");
    string punchlineColorHex = cfgGet(cfg, "punchlinecolor", "");
    string borderColorHex    = cfgGet(cfg, "bordercolor",    "");
    int    videoSeconds = 0;
    float  fadeDuration = 0.0f;
    string fadeType     = "fade";
    bool   fadeHalf     = false;
    int    imgW = 800, imgH = 500;
    {
        string sz = cfgGet(cfg, "size", "");
        if (!sz.empty()) {
            size_t x = sz.find('x');
            if (x == string::npos) x = sz.find('X');
            if (x != string::npos) {
                try {
                    int w = stoi(sz.substr(0, x));
                    int h = stoi(sz.substr(x + 1));
                    if (w >= 100 && h >= 100) { imgW = w; imgH = h; }
                } catch (...) {}
            }
        }
    }
    bool   saveConfig  = false;
    bool   showConfig  = false;

    try {
        for (int i = 1; i < argc; i++) {
            string arg = argv[i];
            if (arg == "-p" || arg == "--pick") {
                if (i + 1 < argc) {
                    jokeNumber = stoi(argv[++i]);
                } else {
                    cerr << "Error: --pick requires a number.\n";
                    return 1;
                }
            } else if (arg == "-o" || arg == "--output") {
                if (i + 1 < argc) {
                    outputFile = argv[++i];
                } else {
                    cerr << "Error: --output requires a filename.\n";
                    return 1;
                }
            } else if (arg == "-f" || arg == "--font") {
                if (i + 1 < argc) {
                    fontPath = argv[++i];
                } else {
                    cerr << "Error: --font requires a path.\n";
                    return 1;
                }
            } else if (arg == "--setupfont") {
                if (i + 1 < argc) {
                    setupFontPath = argv[++i];
                } else {
                    cerr << "Error: --setupfont requires a path.\n";
                    return 1;
                }
            } else if (arg == "--punchlinefont") {
                if (i + 1 < argc) {
                    punchlineFontPath = argv[++i];
                } else {
                    cerr << "Error: --punchlinefont requires a path.\n";
                    return 1;
                }
            } else if (arg == "--footerfont") {
                if (i + 1 < argc) {
                    footerFontPath = argv[++i];
                } else {
                    cerr << "Error: --footerfont requires a path.\n";
                    return 1;
                }
            } else if (arg == "-t" || arg == "--theme") {
                if (i + 1 < argc) {
                    themeName = argv[++i];
                } else {
                    cerr << "Error: --theme requires a name.\n";
                    listThemes();
                    return 1;
                }
            } else if (arg == "--nodim") {
                nodim = true;
            } else if (arg == "--aligntext") {
                if (i + 1 < argc) {
                    string val = argv[++i];
                    size_t comma = val.find(',');
                    if (comma == string::npos) {
                        cerr << "Error: --aligntext requires two values separated by a comma (e.g. center,middle).\n";
                        return 1;
                    }
                    halign = val.substr(0, comma);
                    valign = val.substr(comma + 1);
                    if (halign != "left" && halign != "center" && halign != "right") {
                        cerr << "Error: invalid horizontal alignment '" << halign << "'. Use left, center, or right.\n";
                        return 1;
                    }
                    if (valign != "top" && valign != "middle" && valign != "bottom") {
                        cerr << "Error: invalid vertical alignment '" << valign << "'. Use top, middle, or bottom.\n";
                        return 1;
                    }
                } else {
                    cerr << "Error: --aligntext requires a value (e.g. center,middle).\n";
                    return 1;
                }
            } else if (arg == "--textsize") {
                if (i + 1 < argc) {
                    try {
                        float pct = stof(argv[++i]);
                        if (pct <= 0) throw invalid_argument("");
                        textScale = pct / 100.0f;
                    } catch (...) {
                        cerr << "Error: --textsize requires a positive number (e.g. 150 for 150%).\n";
                        return 1;
                    }
                } else {
                    cerr << "Error: --textsize requires a percentage value.\n";
                    return 1;
                }
            } else if (arg == "--nofooter") {
                nofooter = true;
            } else if (arg == "--footer") {
                if (i + 1 < argc) {
                    labelOverride = argv[++i];
                    hasLabelOverride = true;
                } else {
                    cerr << "Error: --footer requires a text argument (use \"\" for no footer).\n";
                    return 1;
                }
            } else if (arg == "--setup") {
                if (i + 1 < argc) {
                    customSetup = argv[++i];
                } else {
                    cerr << "Error: --setup requires text.\n";
                    return 1;
                }
            } else if (arg == "--punchline") {
                if (i + 1 < argc) {
                    customPunchline = argv[++i];
                } else {
                    cerr << "Error: --punchline requires text.\n";
                    return 1;
                }
            } else if (arg == "--setupcolor") {
                if (i + 1 < argc) {
                    setupColorHex = argv[++i];
                } else {
                    cerr << "Error: --setupcolor requires a hex color (e.g. #ff0000).\n";
                    return 1;
                }
            } else if (arg == "--punchlinecolor") {
                if (i + 1 < argc) {
                    punchlineColorHex = argv[++i];
                } else {
                    cerr << "Error: --punchlinecolor requires a hex color (e.g. #ffffff).\n";
                    return 1;
                }
            } else if (arg == "--bordercolor") {
                if (i + 1 < argc) {
                    borderColorHex = argv[++i];
                } else {
                    cerr << "Error: --bordercolor requires a hex color (e.g. #ffffff).\n";
                    return 1;
                }
            } else if (arg == "--roundedcorners" || arg == "-rc") {
                roundedcorners = true;
            } else if (arg == "-bg" || arg == "--background") {
                if (i + 1 < argc) {
                    bgPath = argv[++i];
                } else {
                    cerr << "Error: --background requires a file path.\n";
                    return 1;
                }
            } else if (arg == "-fg" || arg == "--foreground") {
                if (i + 1 < argc) {
                    fgPath = argv[++i];
                } else {
                    cerr << "Error: --foreground requires a file path.\n";
                    return 1;
                }
            } else if (arg == "--noborder" || arg == "-nb") {
                noborder = true;
            } else if (arg == "--size") {
                if (i + 1 < argc) {
                    string val = argv[++i];
                    size_t x = val.find('x');
                    if (x == string::npos) x = val.find('X');
                    if (x == string::npos) {
                        cerr << "Error: --size requires WxH format (e.g. 1080x1920).\n";
                        return 1;
                    }
                    try {
                        imgW = stoi(val.substr(0, x));
                        imgH = stoi(val.substr(x + 1));
                        if (imgW < 100 || imgH < 100) throw invalid_argument("");
                    } catch (...) {
                        cerr << "Error: --size values must be integers >= 100 (e.g. --size 1080x1920).\n";
                        return 1;
                    }
                } else {
                    cerr << "Error: --size requires a WxH argument.\n";
                    return 1;
                }
            } else if (arg == "--video") {
                if (i + 1 < argc) {
                    try {
                        videoSeconds = stoi(argv[++i]);
                        if (videoSeconds <= 0) throw invalid_argument("");
                    } catch (...) {
                        cerr << "Error: --video requires a positive integer (seconds).\n";
                        return 1;
                    }
                } else {
                    cerr << "Error: --video requires a number of seconds.\n";
                    return 1;
                }
            } else if (arg == "--fade") {
                if (i + 1 < argc) {
                    try {
                        fadeDuration = stof(argv[++i]);
                        if (fadeDuration <= 0) throw invalid_argument("");
                    } catch (...) {
                        cerr << "Error: --fade requires a positive number of seconds (e.g. 0.8).\n";
                        return 1;
                    }
                } else {
                    cerr << "Error: --fade requires a duration in seconds.\n";
                    return 1;
                }
            } else if (arg == "--fadetype") {
                if (i + 1 < argc) {
                    fadeType = argv[++i];
                    // lowercase it
                    for (auto& c : fadeType) c = (char)tolower((unsigned char)c);
                } else {
                    cerr << "Error: --fadetype requires a transition name.\n";
                    return 1;
                }
            } else if (arg == "--fadehalf") {
                fadeHalf = true;
            } else if (arg == "--saveconfig") {
                saveConfig = true;
            } else if (arg == "--showconfig") {
                showConfig = true;
            } else if (arg == "--version" || arg == "-v") {
                cout << "jokes_image version " << VERSION << "\n";
                return 0;
            } else if (arg == "-h" || arg == "--help") {
                displayHelp();
                return 0;
            } else {
                cerr << "Unknown option: " << arg << "\n";
                displayHelp();
                return 1;
            }
        }
    } catch (...) {
        cerr << "Error parsing command line arguments.\n";
        return 1;
    }

    // ── --showconfig: print effective settings and exit ───────────────────
    if (showConfig) {
        cout << "Effective settings (config file + command-line):\n";
        cout << "  font           = " << fontPath                                                         << "\n";
        cout << "  theme          = " << themeName                                                        << "\n";
        cout << "  textsize       = " << (int)(textScale * 100)                                           << "\n";
        cout << "  size           = " << imgW << "x" << imgH                                              << "\n";
        cout << "  noborder       = " << (noborder       ? "yes" : "no")                                  << "\n";
        cout << "  nodim          = " << (nodim          ? "yes" : "no")                                  << "\n";
        cout << "  roundedcorners = " << (roundedcorners ? "yes" : "no")                                  << "\n";
        cout << "  nofooter       = " << (nofooter       ? "yes" : "no")                                  << "\n";
        cout << "  footer         = " << (hasLabelOverride ? labelOverride : "(default)")                 << "\n";
        cout << "  setupcolor     = " << (setupColorHex.empty()     ? "(theme default)" : setupColorHex)  << "\n";
        cout << "  punchlinecolor = " << (punchlineColorHex.empty() ? "(theme default)" : punchlineColorHex) << "\n";
        cout << "  bordercolor    = " << (borderColorHex.empty()    ? "(theme default)" : borderColorHex) << "\n";
        cout << "  aligntext      = " << halign << "," << valign                                          << "\n";
        cout << "  setupfont      = " << (setupFontPath.empty()     ? "(uses -f)" : setupFontPath)        << "\n";
        cout << "  punchlinefont  = " << (punchlineFontPath.empty() ? "(uses -f)" : punchlineFontPath)    << "\n";
        cout << "  footerfont     = " << (footerFontPath.empty()    ? "(uses -f)" : footerFontPath)       << "\n";
        cout << "  background     = " << (bgPath.empty() ? "(none)" : bgPath)                            << "\n";
        cout << "  foreground     = " << (fgPath.empty() ? "(none)" : fgPath)                            << "\n";
        ifstream check(CONFIG_FILE);
        if (check.good())
            cout << "\nConfig file: ./" << CONFIG_FILE << " (loaded)\n";
        else
            cout << "\nConfig file: ./" << CONFIG_FILE << " (not found — using defaults)\n";
        return 0;
    }

    // ── --saveconfig: write current settings to .jokes_image ─────────────
    if (saveConfig) {
        ofstream f(CONFIG_FILE);
        if (!f) {
            cerr << "Error: could not write '" << CONFIG_FILE << "'.\n";
            return 1;
        }
        f << "# jokes_image configuration — generated by jokes_image --saveconfig\n";
        f << "font           = " << fontPath                    << "\n";
        f << "theme          = " << themeName                   << "\n";
        f << "textsize       = " << (int)(textScale * 100)      << "\n";
        f << "size           = " << imgW << "x" << imgH         << "\n";
        f << "noborder       = " << (noborder       ? "yes" : "no") << "\n";
        f << "nodim          = " << (nodim          ? "yes" : "no") << "\n";
        f << "roundedcorners = " << (roundedcorners ? "yes" : "no") << "\n";
        f << "nofooter       = " << (nofooter       ? "yes" : "no") << "\n";
        if (hasLabelOverride)
            f << "footer         = " << labelOverride           << "\n";
        f << "setupcolor     = " << setupColorHex               << "\n";
        f << "punchlinecolor = " << punchlineColorHex           << "\n";
        f << "bordercolor    = " << borderColorHex              << "\n";
        f << "aligntext      = " << halign << "," << valign     << "\n";
        f << "setupfont      = " << setupFontPath               << "\n";
        f << "punchlinefont  = " << punchlineFontPath           << "\n";
        f << "footerfont     = " << footerFontPath              << "\n";
        f << "background     = " << bgPath                      << "\n";
        f << "foreground     = " << fgPath                      << "\n";
        cerr << "Saved defaults to ./" << CONFIG_FILE << "\n";
        return 0;
    }

    if (themeName != "all" && THEMES.find(themeName) == THEMES.end()) {
        cerr << "Error: unknown theme '" << themeName << "'.\n";
        listThemes();
        return 1;
    }

    // Parse and validate any color overrides
    Color overrideSetup     = {0,0,0};
    Color overridePunchline = {0,0,0};
    Color overrideBorder    = {0,0,0};
    bool hasSetupColor      = false;
    bool hasPunchlineColor  = false;
    bool hasBorderColor     = false;
    if (!setupColorHex.empty()) {
        if (!parseColor(setupColorHex, overrideSetup)) {
            cerr << "Error: invalid hex color for --setupcolor: " << setupColorHex << "\n";
            return 1;
        }
        hasSetupColor = true;
    }
    if (!punchlineColorHex.empty()) {
        if (!parseColor(punchlineColorHex, overridePunchline)) {
            cerr << "Error: invalid hex color for --punchlinecolor: " << punchlineColorHex << "\n";
            return 1;
        }
        hasPunchlineColor = true;
    }
    if (!borderColorHex.empty()) {
        if (!parseColor(borderColorHex, overrideBorder)) {
            cerr << "Error: invalid hex color for --bordercolor: " << borderColorHex << "\n";
            return 1;
        }
        hasBorderColor = true;
    }

    // Helper to apply color overrides to a theme copy
    auto resolveTheme = [&](const Theme& base) {
        Theme t = base;
        if (hasSetupColor)     t.setup     = overrideSetup;
        if (hasPunchlineColor) t.punchline = overridePunchline;
        if (hasBorderColor)    t.border    = overrideBorder;
        return t;
    };

    bool usingCustomText = !customSetup.empty() || !customPunchline.empty();

    if (!usingCustomText && jokeNumber < 1) {
        cerr << "Error: joke number required. Use -p <number>, or supply --setup/--punchline.\n\n";
        displayHelp();
        return 1;
    }

    Joke joke;
    if (usingCustomText) {
        joke.setup     = customSetup;
        joke.punchline = customPunchline;
        joke.type      = "custom";
        if (jokeNumber < 1) jokeNumber = 0; // no number to display in footer
    } else {
        if (jokeNumber > (int)jokes.size()) {
            cerr << "Error: joke #" << jokeNumber << " does not exist. (Range: 1-" << jokes.size() << ")\n";
            return 1;
        }
        joke = jokes[jokeNumber - 1];
    }

    if (themeName == "all") {
        bool ok = true;
        for (auto& kv : THEMES) {
            string file = outputFile.empty()
                ? (usingCustomText ? "joke_custom_" + kv.first + ".jpg"
                                   : "joke_" + to_string(jokeNumber) + "_" + kv.first + ".jpg")
                : outputFile;
            ok &= jokeToJpeg(joke, jokeNumber, file, fontPath, resolveTheme(kv.second), noborder, bgPath, nodim, roundedcorners, nofooter, labelOverride, hasLabelOverride, textScale, fgPath, halign, valign, setupFontPath, punchlineFontPath, footerFontPath, false, imgW, imgH);
        }
        return ok ? 0 : 1;
    }

    Theme resolvedTheme = resolveTheme(THEMES.at(themeName));

    // --video: generate setup-only frame + full frame, then call FFmpeg to concat
    if (videoSeconds > 0) {
        string base = usingCustomText ? "joke_custom_" + themeName
                                      : "joke_" + to_string(jokeNumber) + "_" + themeName;
        string setupTmp = base + "_setup_tmp.jpg";
        string fullTmp  = base + "_full_tmp.jpg";

        string videoFile;
        if (!outputFile.empty()) {
            videoFile = outputFile;
        } else {
            videoFile = base + ".mp4";
        }

        // Frame 1: setup only (no punchline)
        if (!jokeToJpeg(joke, jokeNumber, setupTmp, fontPath, resolvedTheme, noborder, bgPath, nodim,
                        roundedcorners, nofooter, labelOverride, hasLabelOverride, textScale, fgPath,
                        halign, valign, setupFontPath, punchlineFontPath, footerFontPath, /*nopunchline=*/true, imgW, imgH)) {
            return 1;
        }

        // Frame 2: full joke (setup + punchline)
        if (!jokeToJpeg(joke, jokeNumber, fullTmp, fontPath, resolvedTheme, noborder, bgPath, nodim,
                        roundedcorners, nofooter, labelOverride, hasLabelOverride, textScale, fgPath,
                        halign, valign, setupFontPath, punchlineFontPath, footerFontPath, /*nopunchline=*/false, imgW, imgH)) {
            remove(setupTmp.c_str());
            return 1;
        }

        // Build and run FFmpeg command
        // Setup frame holds for videoSeconds, punchline frame holds for videoSeconds more
        string cmd;
        if (fadeDuration > 0.0f) {
            // Map friendly --fadetype names to FFmpeg xfade transition names
            static const map<string,string> FADE_TYPES = {
                { "wipe",       "wipeleft"   }, // reveal from right (default)
                { "wiperight",  "wiperight"  }, // reveal from left
                { "wipeup",     "wipeup"     }, // reveal from bottom
                { "wipedown",   "wipedown"   }, // reveal from top
                { "fade",       "fade"        }, // opacity crossfade
                { "fadeblack",  "fadeblack"  }, // fade through black
                { "fadewhite",  "fadewhite"  }, // fade through white
                { "slide",      "slideleft"  }, // slide in from right
                { "slideup",    "slideup"    }, // slide in from bottom
                { "radial",     "radial"     }, // radial clock-wipe
                { "circle",     "circleopen" }, // circle opens outward
                { "pixelize",   "pixelize"   }, // pixelate dissolve
                { "dissolve",   "dissolve"   }, // random pixel dissolve
                { "blur",       "hblur"      }, // horizontal blur
                { "squeeze",    "squeezeh"   }, // horizontal squeeze
                { "zoom",       "zoomin"     }, // zoom in reveal
                { "diagtl",     "diagtl"     }, // diagonal from top-left
                { "diagtr",     "diagtr"     }, // diagonal from top-right
            };
            auto it = FADE_TYPES.find(fadeType);
            if (it == FADE_TYPES.end()) {
                cerr << "Error: unknown --fadetype '" << fadeType << "'.\n";
                cerr << "Valid types: wipe, wiperight, wipeup, wipedown, fade, fadeblack, fadewhite,\n"
                        "             slide, slideup, radial, circle, pixelize, dissolve, blur,\n"
                        "             squeeze, zoom, diagtl, diagtr\n";
                remove(setupTmp.c_str());
                remove(fullTmp.c_str());
                return 1;
            }
            string xfadeName = it->second;

            // xfade timing: setup plays N seconds, transition for D seconds, punchline plays N seconds
            // offset = N - D, input1 duration = N + D, total output = 2*N
            float N = (float)videoSeconds;
            float D = fadeDuration;
            float offset = N - D;
            if (offset < 0.0f) offset = 0.0f;
            char buf[256];
            snprintf(buf, sizeof(buf), "%.3f", D);      string dStr(buf);
            snprintf(buf, sizeof(buf), "%.3f", offset);  string oStr(buf);
            snprintf(buf, sizeof(buf), "%.3f", N);        string nStr(buf);
            snprintf(buf, sizeof(buf), "%.3f", N + D);   string n2Str(buf);

            if (fadeHalf) {
                // Crop-xfade-overlay approach:
                //   Input 0: setup frame, N sec           (bottom-half crop source)
                //   Input 1: full frame,  N+D sec         (bottom-half crop source)
                //   Input 2: full frame,  2N sec           (overlay base — full frame throughout)
                //
                // The bottom halves of inputs 0 and 1 are xfaded with the chosen transition,
                // then overlaid onto the bottom half of input 2. Input 2's top half always shows
                // the setup text, completely static. Total output = 2N seconds.
                char buf2[32];
                snprintf(buf2, sizeof(buf2), "%.3f", N * 2.0f);
                string twoNStr(buf2);
                string halfH = to_string(imgH / 2);
                string fc = "[0:v]crop=iw:ih/2:0:ih/2[b0];"
                            "[1:v]crop=iw:ih/2:0:ih/2[b1];"
                            "[b0][b1]xfade=transition=" + xfadeName + ":duration=" + dStr + ":offset=" + oStr + "[bfade];"
                            "[2:v][bfade]overlay=0:" + halfH + "[v]";
                cmd = "ffmpeg -y"
                      " -loop 1 -t " + nStr    + " -i \"" + setupTmp + "\""
                      " -loop 1 -t " + n2Str   + " -i \"" + fullTmp  + "\""
                      " -loop 1 -t " + twoNStr + " -i \"" + fullTmp  + "\""
                      " -filter_complex \"" + fc + "\""
                      " -map \"[v]\" -pix_fmt yuv420p"
                      " \"" + videoFile + "\"";
            } else {
                cmd = "ffmpeg -y"
                      " -loop 1 -t " + nStr  + " -i \"" + setupTmp + "\""
                      " -loop 1 -t " + n2Str + " -i \"" + fullTmp  + "\""
                      " -filter_complex \"[0:v][1:v]xfade=transition=" + xfadeName + ":duration=" + dStr + ":offset=" + oStr + "[v]\""
                      " -map \"[v]\" -pix_fmt yuv420p"
                      " \"" + videoFile + "\"";
            }
        } else {
            cmd = "ffmpeg -y"
                  " -loop 1 -t " + to_string(videoSeconds) + " -i \"" + setupTmp + "\""
                  " -loop 1 -t " + to_string(videoSeconds) + " -i \"" + fullTmp  + "\""
                  " -filter_complex \"[0:v][1:v]concat=n=2:v=1:a=0\""
                  " -pix_fmt yuv420p"
                  " \"" + videoFile + "\"";
        }

        cout << "Running: " << cmd << "\n";
        int ret = system(cmd.c_str());

        remove(setupTmp.c_str());
        remove(fullTmp.c_str());

        if (ret != 0) {
            cerr << "Error: FFmpeg failed (exit code " << ret << "). Is ffmpeg installed?\n";
            return 1;
        }
        cout << "Video saved: " << videoFile << "\n";
        return 0;
    }

    if (outputFile.empty())
        outputFile = usingCustomText ? "joke_custom_" + themeName + ".jpg"
                                     : "joke_" + to_string(jokeNumber) + "_" + themeName + ".jpg";

    return jokeToJpeg(joke, jokeNumber, outputFile, fontPath, resolvedTheme, noborder, bgPath, nodim, roundedcorners, nofooter, labelOverride, hasLabelOverride, textScale, fgPath, halign, valign, setupFontPath, punchlineFontPath, footerFontPath, false, imgW, imgH) ? 0 : 1;
}
