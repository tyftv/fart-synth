#include "ButtView.h"

using namespace buttart;
using juce::AffineTransform;
using juce::Colour;
using juce::ColourGradient;

namespace
{
constexpr float sphY = 394.f, ZOOM = 1.45f, ZANCH = 395.f, ZPIV = 372.f;
constexpr float TAU = 6.2832f;
inline float smooth (float a, float b, float x) { x = (x - a) / (b - a); x = x < 0 ? 0 : x > 1 ? 1 : x; return x * x * (3 - 2 * x); }
Colour rgba (float r, float g, float b, float a) { return Colour::fromFloatRGBA (juce::jlimit (0.f, 1.f, r / 255.f), juce::jlimit (0.f, 1.f, g / 255.f), juce::jlimit (0.f, 1.f, b / 255.f), juce::jlimit (0.f, 1.f, a)); }
ColourGradient radial (Colour inner, float cx, float cy, float radius, Colour outer)
{
    return ColourGradient (inner, cx, cy, outer, cx + radius, cy, true);
}
}

void ButtView::kick()
{
    for (auto& h : halves) { h.vy += (rnd() - 0.3f) * 160.f; h.vx += h.s * (60.f + rnd() * 60.f); h.vsq += 0.6f; }
}

void ButtView::tick (double dtd)
{
    const float dt = (float) dtd;
    t += dtd;
    if (hintGone) pressed += dtd * 2.5;
    proc.copyScope (buf, 1024);
    const float e = proc.voice.uiEnv.load();
    const float pressure = prm ("pressure"), rumbleP = prm ("rumble"), flutterP = prm ("flutter"), gasP = prm ("gas"), wetP = prm ("wet");
    flap = juce::jmin (1.f, proc.voice.uiFlap.load() * 5.f);
    wetGate = proc.voice.uiWetGate.load();

    const float target = juce::jmin (1.f, e * pressure * 1.15f);
    openV += (target - openV) * juce::jmin (1.f, dt * 18.f);
    openAvg += (openV - openAvg) * juce::jmin (1.f, dt * 0.5f);
    const float open = openV;
    float a = 0; for (float v : buf) a += v * v;
    const float r = std::sqrt (a / 1024.f);
    rms += (r - rms) * juce::jmin (1.f, dt * 20.f);
    const float shake = juce::jmin (1.f, open * 1.2f);

    for (auto& h : halves)
    {
        const float rum = std::sin ((float) t * TAU * (3.5f + h.s * 0.4f)) * rumbleP * shake * 6.f;
        const float tx = (rnd() - 0.5f) * shake * 14.f * (0.4f + flutterP) + rum * h.s;
        const float ty = (rnd() - 0.5f) * shake * 12.f - shake * 3.f + rum * 0.4f;
        const float tq = shake * (0.02f + flap * 0.03f) * (0.5f + rnd() * 0.5f);
        const float k = 420.f, c = 17.f;
        h.vx += (k * (tx - h.ox) - c * h.vx) * dt; h.ox += h.vx * dt;
        h.vy += (k * (ty - h.oy) - c * h.vy) * dt; h.oy += h.vy * dt;
        h.vsq += (k * 0.5f * (tq - h.sq) - c * 0.6f * h.vsq) * dt; h.sq += h.vsq * dt;
        h.ox = juce::jlimit (-26.f, 26.f, h.ox); h.oy = juce::jlimit (-26.f, 26.f, h.oy); h.sq = juce::jlimit (-0.1f, 0.12f, h.sq);
    }
    shX = (rnd() - 0.5f) * shake * 4.f; shY = (rnd() - 0.5f) * shake * 4.f;

    for (int i = (int) rings.size() - 1; i >= 0; --i)
    {
        auto& rg = rings[(size_t) i]; rg.r += dt * 300.f; rg.a -= dt * 1.1f;
        if (rg.a <= 0) rings.erase (rings.begin() + i);
    }

    if (open > 0.02f)
    {
        spawn += dt * open * (30.f + gasP * 110.f);
        while (spawn >= 1.f && parts.size() < 300)
        {
            spawn -= 1.f;
            const float ang = juce::MathConstants<float>::halfPi + (rnd() - 0.5f) * 1.9f, sp = 60.f + open * 230.f * (rnd() + 0.3f);
            const bool dark = rnd() < wetP * 0.35f;
            parts.push_back ({ CX + (rnd() - 0.5f) * 10.f, sphY + 6.f, std::cos (ang) * sp, std::sin (ang) * sp, 7.f + rnd() * 9.f, 26.f + rnd() * 30.f, 0.f, 0.7f + rnd() * 0.9f,
                               dark ? 38.f : 68.f + rnd() * 34.f, dark ? 30.f : 46.f, 0 });
        }
        spawn = juce::jmin (spawn, 2.f);
        const float wd = 1.f - wetGate;
        if (rnd() < wetP * open * wd * 0.9f)
            for (int k = 0; k < 3; ++k)
                parts.push_back ({ (float) CX, sphY + 4.f, (rnd() - 0.5f) * 520.f, 80.f + rnd() * 300.f, 2.f + rnd() * 3.f, 0.f, 0.f, 0.6f, 25.f, 30.f, 1 });
        ringT -= dt; if (ringT <= 0 && open > 0.12f) { ringT = 0.12f; rings.push_back ({ 30.f, 0.9f }); }
        stinkT -= dt;
        if (stinkT <= 0 && gasP * open > 0.03f)
        {
            stinkT = 0.5f - gasP * 0.4f;
            if (stinks.size() < 14) stinks.push_back ({ CX + (rnd() - 0.5f) * 460.f, 320.f + rnd() * 190.f, 0.f, 1.6f + rnd(), rnd() * 6.f });
        }
    }
    if (gasP > 0.3f && openAvg > 0.12f && (int) flies.size() < juce::roundToInt (gasP * 6.f) && rnd() < dt * 1.2f)
        flies.push_back ({ rnd() * 6.28f, (1.5f + rnd() * 2.5f) * (rnd() < .5f ? -1.f : 1.f), 70.f + rnd() * 120.f, 50.f + rnd() * 90.f, rnd() * 6.f, 0.f, 4.f + rnd() * 5.f });

    for (int i = (int) parts.size() - 1; i >= 0; --i)
    {
        auto& p = parts[(size_t) i]; p.life += dt;
        if (p.life >= p.max) { parts.erase (parts.begin() + i); continue; }
        p.x += p.vx * dt; p.y += p.vy * dt;
        if (p.type == 0) { p.vx *= 1 - 1.8f * dt; p.vy *= 1 - 1.3f * dt; p.vy -= 34.f * dt; p.r += p.g * dt; }
        else p.vy += 700.f * dt;
    }
    for (int i = (int) stinks.size() - 1; i >= 0; --i) { stinks[(size_t) i].life += dt; if (stinks[(size_t) i].life >= stinks[(size_t) i].max) stinks.erase (stinks.begin() + i); }
    for (int i = (int) flies.size() - 1; i >= 0; --i)
    {
        auto& f = flies[(size_t) i]; f.life += dt; f.a += f.sp * dt;
        if (f.life >= f.max) flies.erase (flies.begin() + i);
    }
}

void ButtView::drawBG (juce::Graphics& g)
{
    const float open = openV;
    g.setGradientFill (ColourGradient (Colour (0xff2a1a3c), 0, 0, Colour (0xff110a1a), 0, (float) LH, false));
    g.fillRect (0, 0, LW, LH);

    // rotating sunburst
    {
        juce::Path rays;
        const int N = 22;
        for (int k = 0; k < N; ++k)
        {
            const float a0 = k * TAU / N;
            rays.addPieSegment (-760.f, -760.f, 1520.f, 1520.f, a0, a0 + 3.1416f / N, 0.f);
        }
        const float al = 0.05f + rms * 0.5f + open * 0.05f;
        g.setColour (rgba (255, 110, 190, al));
        g.fillPath (rays, AffineTransform::rotation ((float) t * 0.035f).translated (300.f, 300.f));
    }
    {
        auto gr = radial (rgba (255, 236, 190, .20f), (float) CX, 60.f, 560.f, rgba (255, 236, 190, 0));
        g.setGradientFill (gr); g.fillRect (0, 0, LW, LH);
    }
    {
        auto gr = radial (rgba (182, 255, 60, 0.03f + open * 0.2f), (float) CX, 420.f, 340.f, rgba (182, 255, 60, 0));
        g.setGradientFill (gr); g.fillRect (0, 0, LW, LH);
    }
    {
        auto gr = radial (rgba (0, 0, 0, 0), (float) CX, 320.f, 470.f, rgba (0, 0, 0, .55f));
        gr.addColour (200.0 / 470.0, rgba (0, 0, 0, 0));
        g.setGradientFill (gr); g.fillRect (0, 0, LW, LH);
    }
}

void ButtView::drawHalf (juce::Graphics& g, const Half& h)
{
    const float s = h.s, pv = CX + s * 110.f, ox = h.ox + s * openV * 5.f, c = ox / 320.f, a = 1.f + h.sq, d = 1.f + h.sq * 0.6f;
    g.saveState();
    g.addTransform (AffineTransform (a, c, pv - pv * a - c * 120.f, 0.f, d, 250.f - 250.f * d + h.oy));
    g.setOpacity (1.0f);
    g.reduceClipRegion (s < 0 ? juce::Rectangle<int> (-100, -100, CX + 100, LH + 200) : juce::Rectangle<int> (CX, -100, LW - CX + 100, LH + 200));
    g.drawImage (tex, CROP_X0, CROP_Y0, CROP_X1 - CROP_X0, CROP_Y1 - CROP_Y0, 0, 0, tex.getWidth(), tex.getHeight());
    g.restoreState();
}

void ButtView::drawCrack (juce::Graphics& g)
{
    const float open = openV;
    const float sk0 = (float) kSkins[skin][0], sk1 = (float) kSkins[skin][1], sk2 = (float) kSkins[skin][2];
    auto dk = [&] (float m, float al) { return rgba (std::floor (sk0 * m), std::floor (sk1 * m * 0.93f), std::floor (sk2 * m * 0.9f), al); };

    // soft shade that deepens as the cheeks spread
    g.saveState();
    g.addTransform (AffineTransform::scale ((16.f + open * 22.f) / 110.f, 1.f).translated ((float) CX, sphY - 20.f));
    g.setGradientFill (radial (dk (0.3f, 0.15f + open * 0.45f), 0, 0, 125.f, dk (0.5f, 0)));
    g.fillEllipse (-125.f, -125.f, 250.f, 250.f);
    g.restoreState();

    // butthole: pink, soft edges, pucker lines
    const float rx = 10.f + open * 4.f, ry = 13.f + open * 5.f, hole = juce::jmax (0.f, (open * 0.5f + flap * 0.5f) * 8.f);
    g.saveState();
    g.addTransform (AffineTransform::scale (rx / ry, 1.f).translated ((float) CX, sphY + 1.f));
    {
        auto gr = radial (rgba (190, 84, 110, 1), 0, 0, ry * 1.35f, rgba (214, 126, 146, 0));
        gr.addColour (0.5, rgba (206, 108, 132, .85f));
        g.setGradientFill (gr);
        g.fillEllipse (-ry * 1.35f, -ry * 1.35f, ry * 2.7f, ry * 2.7f);
    }
    for (int k = 0; k < 14; ++k)
    {
        const float an = k / 14.f * TAU, c = std::cos (an), sn = std::sin (an);
        const float x0 = c * (hole + 1.5f), y0 = sn * (hole + 1.5f) * 1.1f, x1 = c * ry * 0.85f, y1 = sn * ry * 0.9f;
        g.setColour (rgba (120, 36, 66, .55f)); g.drawLine (x0, y0, x1, y1, 1.5f);
        g.setColour (rgba (255, 200, 215, .16f)); g.drawLine (x0 + 1.1f, y0 + 1.f, x1 + 1.1f, y1 + 1.f, 1.f);
    }
    if (hole > 0.4f)
    {
        auto gr = radial (rgba (58, 10, 26, 1), 0, 0, hole * 1.3f, rgba (150, 50, 80, 0));
        gr.addColour (0.7, rgba (96, 24, 48, .95f));
        g.setGradientFill (gr);
        g.fillEllipse (-hole * 1.1f, -hole * 1.3f, hole * 2.2f, hole * 2.6f);
        g.setColour (rgba (182, 255, 60, juce::jmin (0.5f, open * 0.5f)));
        g.fillEllipse (-hole * 0.45f, -hole * 0.5f, hole * 0.9f, hole);
    }
    g.restoreState();
}

void ButtView::drawSphincter (juce::Graphics& g)
{
    const float open = openV;
    if (open <= 0.04f) return;
    const float r = 10.f + open * 6.f;
    juce::Path p;
    const int N = 96;
    for (int i = 0; i <= N; ++i)
    {
        const float a = (float) i / N * TAU, w = buf[(i % N) * 4 % 1024], rad = r + 16.f + w * 30.f;
        const float px = std::cos (a) * rad, py = std::sin (a) * rad * 1.15f;
        if (i) p.lineTo (px, py); else p.startNewSubPath (px, py);
    }
    p.closeSubPath();
    const auto tr = AffineTransform::translation ((float) CX, sphY);
    g.setColour (rgba (182, 255, 60, juce::jmin (0.8f, open * 1.1f) * 0.25f));
    g.strokePath (p, juce::PathStrokeType (6.f), tr);
    g.setColour (rgba (182, 255, 60, juce::jmin (0.8f, open * 1.1f)));
    g.strokePath (p, juce::PathStrokeType (2.f), tr);
}

void ButtView::paint (juce::Graphics& g)
{
    const float viewScale = (float) getWidth() / (float) LW;
    g.fillAll (Colour (0xff120d18));
    g.saveState();
    {
        juce::Path clip; clip.addRoundedRectangle (getLocalBounds().toFloat(), 14.f);
        g.reduceClipRegion (clip);
    }
    g.addTransform (AffineTransform::scale (viewScale));
    g.reduceClipRegion (0, 0, LW, LH);
    drawBG (g);

    if (! tex.isValid())
    {
        g.setColour (Colour (0xffb6ff3c));
        g.setFont (juce::Font (juce::FontOptions (20.f)));
        g.drawText ("warming up...", 0, 0, LW, LH, juce::Justification::centred);
        g.restoreState();
        return;
    }

    const float open = openV;
    g.saveState();
    g.addTransform (AffineTransform::translation (-(float) CX, -ZPIV).scaled (ZOOM).translated ((float) CX, ZANCH));

    for (auto& r : rings)
    {
        g.setColour (rgba (182, 255, 60, r.a * 0.5f));
        g.drawEllipse (CX - r.r, sphY - r.r, r.r * 2, r.r * 2, 3.f + r.a * 4.f);
    }

    g.saveState();
    g.addTransform (AffineTransform::translation (shX, shY));
    g.setOpacity (1.0f);
    g.setImageResamplingQuality (juce::Graphics::mediumResamplingQuality);
    g.drawImage (tex, CROP_X0, CROP_Y0, CROP_X1 - CROP_X0, CROP_Y1 - CROP_Y0, 0, 0, tex.getWidth(), tex.getHeight());
    for (auto& h : halves) drawHalf (g, h);
    drawCrack (g);
    drawSphincter (g);
    g.setGradientFill (radial (rgba (150, 255, 60, open * 0.2f), (float) CX, sphY + 14.f, 150.f, rgba (150, 255, 60, 0)));
    g.fillRect (CX - 160, (int) sphY - 140, 320, 300);
    g.restoreState();

    for (auto& p : parts)
    {
        const float k = 1.f - p.life / p.max;
        if (p.type == 0)
        {
            auto gr = radial (Colour::fromHSL (p.h / 360.f, .70f, juce::jmin (1.f, (p.l + 8.f) / 100.f), k * 0.6f), p.x, p.y, p.r,
                              Colour::fromHSL (p.h / 360.f, .60f, p.l / 100.f, 0.f));
            gr.addColour (0.6, Colour::fromHSL (p.h / 360.f, .65f, p.l / 100.f, k * 0.3f));
            g.setGradientFill (gr);
            g.fillEllipse (p.x - p.r, p.y - p.r, p.r * 2, p.r * 2);
        }
        else
        {
            g.setColour (Colour::fromHSL (p.h / 360.f, .60f, p.l / 100.f, juce::jlimit (0.f, 1.f, k)));
            g.fillEllipse (p.x - p.r, p.y - p.r, p.r * 2, p.r * 2);
        }
    }
    for (auto& s : stinks)
    {
        const float k = std::sin (s.life / s.max * juce::MathConstants<float>::pi);
        juce::Path p;
        for (int j = 0; j <= 18; ++j)
        {
            const float px = s.x + std::sin (j * 0.7f + (float) t * 5.f + s.ph) * 9.f, py = s.y - j * 8.f - s.life * 70.f;
            if (j) p.lineTo (px, py); else p.startNewSubPath (px, py);
        }
        g.setColour (Colour::fromHSL (85.f / 360.f, .70f, .52f, juce::jlimit (0.f, 1.f, k * 0.7f)));
        g.strokePath (p, juce::PathStrokeType (4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    for (auto& f : flies)
    {
        const float al = std::sin (f.life / f.max * juce::MathConstants<float>::pi);
        const float x = CX + std::cos (f.a) * f.rx + std::sin ((float) t * 7.f + f.ph) * 5.f;
        const float y = 345.f + std::sin (f.a * 1.7f + f.ph) * f.ry + std::cos ((float) t * 9.f + f.ph) * 5.f;
        const float wf = std::sin ((float) t * 70.f + f.ph) * 0.5f + 0.5f;
        g.setColour (rgba (255, 255, 255, 0.5f * al));
        const float ry = 1.6f + wf * 1.4f;
        juce::Path w; w.addEllipse (-4.f, -ry, 8.f, ry * 2);
        g.fillPath (w, AffineTransform::rotation (-0.6f).translated (x - 3.5f, y - 3.5f));
        g.fillPath (w, AffineTransform::rotation (0.6f).translated (x + 3.5f, y - 3.5f));
        g.setColour (rgba (8, 4, 6, al));
        g.fillEllipse (x - 3.4f, y - 3.4f, 6.8f, 6.8f);
    }
    g.restoreState();   // zoom
    g.restoreState();   // view scale

    // hint (fades after the first press)
    if (pressed < 1.0)
    {
        g.setColour (Colour (0xffb6ff3c).withAlpha (1.f - (float) pressed));
        g.setFont (juce::Font (juce::FontOptions ("Arial Black", 17.f, juce::Font::plain)));
        g.drawText ("HOLD THE BUTT", 0, 12, getWidth(), 24, juce::Justification::centred);
    }
    g.setColour (Colour (0xff3d2c4d).interpolatedWith (Colour (0xffb6ff3c), juce::jlimit (0.f, 1.f, openV * 0.8f)));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.5f), 14.f, 3.f);
}
