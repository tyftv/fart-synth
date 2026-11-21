#include "ButtArt.h"
#include <cmath>

using namespace buttart;

namespace
{
inline double sm (double a, double b, double x) { x = (x - a) / (b - a); x = x < 0 ? 0 : x > 1 ? 1 : x; return x * x * (3 - 2 * x); }
inline double hnoise (double x, double y) { double v = std::sin (x * 12.9898 + y * 78.233) * 43758.5453; return v - std::floor (v) - 0.5; }

void boxH (const float* src, float* dst, int w, int h, int r)
{
    const float inv = 1.f / (float) (2 * r + 1);
    for (int y = 0; y < h; ++y)
    {
        const int o = y * w; float acc = 0;
        for (int x = -r; x <= r; ++x) if (x >= 0 && x < w) acc += src[o + x];
        for (int x = 0; x < w; ++x)
        {
            dst[o + x] = acc * inv;
            const int ad = x + r + 1, sb = x - r;
            if (ad < w) acc += src[o + ad];
            if (sb >= 0) acc -= src[o + sb];
        }
    }
}
void boxV (const float* src, float* dst, int w, int h, int r)
{
    const float inv = 1.f / (float) (2 * r + 1);
    for (int x = 0; x < w; ++x)
    {
        float acc = 0;
        for (int y = -r; y <= r; ++y) if (y >= 0 && y < h) acc += src[y * w + x];
        for (int y = 0; y < h; ++y)
        {
            dst[y * w + x] = acc * inv;
            const int ad = y + r + 1, sb = y - r;
            if (ad < h) acc += src[ad * w + x];
            if (sb >= 0) acc -= src[sb * w + x];
        }
    }
}
std::vector<float> blur (const std::vector<float>& src, double rr)
{
    const int r = juce::jmax (1, (int) std::lround (rr));
    std::vector<float> a (src), b (src.size());
    for (int p = 0; p < 3; ++p) { boxH (a.data(), b.data(), TW, TH, r); boxV (b.data(), a.data(), TW, TH, r); }
    return a;
}

juce::Path torsoPath()
{
    juce::Path p;
    const float c = (float) CX;
    p.startNewSubPath (c, -40); p.lineTo (c + 110, -40);
    p.cubicTo (c + 110, 60, c + 215, 140, c + 210, 270);
    p.cubicTo (c + 205, 380, c + 170, 440, c + 110, 445);
    p.cubicTo (c + 60, 448, c + 20, 430, c, 415);
    p.cubicTo (c - 20, 430, c - 60, 448, c - 110, 445);
    p.cubicTo (c - 170, 440, c - 205, 380, c - 210, 270);
    p.cubicTo (c - 215, 140, c - 110, 60, c - 110, -40);
    p.closeSubPath();
    return p;
}
}

void Baker::bakeGeometry()
{
    std::lock_guard<std::mutex> lk (geoLock);
    if (geoReady) return;

    juce::Image img (juce::Image::SingleChannel, TW, TH, true);
    {
        juce::Graphics g (img);
        g.addTransform (juce::AffineTransform::translation (0.f, (float) PADT).scaled (BK));
        g.setColour (juce::Colours::white);
        g.fillPath (torsoPath());
    }
    const size_t n = (size_t) TW * (size_t) TH;
    tm.assign (n, 0.f);
    {
        juce::Image::BitmapData bd (img, juce::Image::BitmapData::readOnly);
        for (int y = 0; y < TH; ++y)
        {
            const uint8_t* line = bd.getLinePointer (y);
            for (int x = 0; x < TW; ++x) tm[(size_t) y * TW + x] = line[x] / 255.f;
        }
    }
    auto big = blur (tm, 58 * BK), med = blur (tm, 20 * BK);
    tiny = blur (tm, 2.4 * BK);
    th.assign (n, 0.f);
    for (int y = 0; y < TH; ++y)
        for (int x = 0; x < TW; ++x)
        {
            const size_t i = (size_t) y * TW + x;
            if (tm[i] < 0.004f) continue;
            const double X = x / BK, Y = y / BK - PADT;
            const double bl = std::exp (-(std::pow (X - (CX - 105), 2) / 9025 + std::pow (Y - 255, 2) / 22500));
            const double br = std::exp (-(std::pow (X - (CX + 105), 2) / 9025 + std::pow (Y - 255, 2) / 22500));
            th[i] = (float) (0.6 * big[i] + 0.4 * med[i] + 0.24 * (bl + br)
                    - 0.2 * std::exp (-std::pow (X - CX, 2) / (2 * 26 * 26)) * sm (190, 360, Y)
                    - 0.19 * std::exp (-std::pow (X - CX, 2) / (2 * 19 * 19)) * sm (205, 330, Y) * (1 - sm (395, 428, Y)));
        }
    geoReady = true;
}

juce::Image Baker::bake (int sr, int sg, int sb)
{
    bakeGeometry();
    const double sk[3] = { sr / 255.0, sg / 255.0, sb / 255.0 };

    double LV[3] = { -0.5, -0.62, 0.6 };
    { double l = std::sqrt (LV[0] * LV[0] + LV[1] * LV[1] + LV[2] * LV[2]); for (auto& a : LV) a /= l; }
    double HV[3] = { LV[0], LV[1], LV[2] + 1 };
    { double l = std::sqrt (HV[0] * HV[0] + HV[1] * HV[1] + HV[2] * HV[2]); for (auto& a : HV) a /= l; }

    const double S = 230.0, wrap = 0.55, spec = 0.34, shin = 34, rim = 0.3, sss = 0.38;

    // render only the region that can ever be visible
    const int x0 = (int) (CROP_X0 * BK), x1 = (int) (CROP_X1 * BK);
    const int y0 = (int) ((CROP_Y0 + PADT) * BK), y1 = (int) ((CROP_Y1 + PADT) * BK);
    juce::Image out (juce::Image::ARGB, x1 - x0, y1 - y0, true);
    juce::Image::BitmapData bd (out, juce::Image::BitmapData::writeOnly);

    for (int y = juce::jmax (1, y0); y < juce::jmin (TH - 1, y1); ++y)
        for (int x = juce::jmax (1, x0); x < juce::jmin (TW - 1, x1); ++x)
        {
            const size_t i = (size_t) y * TW + x;
            const double a = tm[i];
            if (a < 0.004) continue;
            double nx = -(th[i + 1] - th[i - 1]) * S, ny = -(th[i + TW] - th[i - TW]) * S;
            const double il = 1.0 / std::sqrt (nx * nx + ny * ny + 1);
            nx *= il; ny *= il; const double nz = il;
            const double nl = nx * LV[0] + ny * LV[1] + nz * LV[2];
            const double diff = std::max (0.0, (nl + wrap) / (1 + wrap));
            const double nh = std::max (0.0, nx * HV[0] + ny * HV[1] + nz * HV[2]);
            const double sp = std::pow (nh, shin) * spec + std::pow (nh, shin * 0.22) * spec * 0.18;
            const double fres = std::pow (1 - nz, 2.4) * rim * (0.55 + 0.45 * nx + 0.2 * ny);
            const double ao = 0.5 + 0.5 * sm (0.04, 0.6, th[i]);

            const double X = x / BK, Y = y / BK - PADT;
            const double nn = hnoise (x, y) * 0.05 + hnoise (x * 0.31, y * 0.31) * 0.04;
            const double blm = sm (290, 440, Y) * std::exp (-std::pow (X - CX, 2) / (2 * 95 * 95)) * 0.5;
            const double hem = 1.0;
            const double cr = 1 - 0.5 * std::exp (-std::pow (X - CX, 2) / (2 * 9 * 9)) * sm (210, 300, Y) * (1 - sm (398, 428, Y))
                                - 0.25 * std::exp (-std::pow (X - CX, 2) / (2 * 26 * 26)) * sm (200, 320, Y) * (1 - sm (398, 428, Y));
            const double al0 = sk[0] * (1 + nn) * hem * cr;
            const double al1 = sk[1] * (1 + nn) * (1 - 0.24 * blm) * hem * cr;
            const double al2 = sk[2] * (1 + nn) * (1 - 0.3 * blm) * hem * cr;

            const double ss = std::pow (1 - diff, 2.2) * sss;
            double r = al0 * (0.2 + diff * 1.0) + al0 * ss * 0.9 + 0.5 * fres + sp;
            double g = al1 * (0.17 + diff * 0.93) + al1 * ss * 0.25 + 0.28 * fres + sp * 0.95;
            double b = al2 * (0.26 + diff * 0.82) + al2 * ss * 0.2 + 0.95 * fres + sp * 0.9;
            const double edge = 0.2 + 0.8 * sm (0.6, 0.985, tiny[i]);
            const double k = (0.55 + 0.45 * ao) * edge;
            r *= k; g *= k; b *= k;
            auto c8 = [] (double v) { return (juce::uint8) juce::jlimit (0, 255, (int) (v * 255)); };
            bd.setPixelColour (x - x0, y - y0, juce::Colour (c8 (r), c8 (g), c8 (b), c8 (a)));
        }
    return out;
}
