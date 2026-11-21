#include "PluginEditor.h"
#include "BinaryData.h"

using juce::Colour;

namespace
{
const Colour BG (0xff120d18), PANEL (0xff231a2e), EDGE (0xff3d2c4d), CREAM (0xfff3e6c4), LIME (0xffb6ff3c), PINK (0xffff5fa2), DIM (0xff8f7fa3), INKC (0xff1b0e0a);
constexpr int W = 1100, H = 700;
juce::Font black (float h) { return juce::Font (juce::FontOptions ("Arial Black", h, juce::Font::plain)); }
juce::Font bold (float h) { return juce::Font (juce::FontOptions (h, juce::Font::bold)); }
}

void FartLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&)
{
    const float d = (float) juce::jmin (w, h) - 6.f;
    const float cx = x + w * 0.5f, cy = y + h * 0.5f - 2.f, r = d * 0.5f;
    g.setColour (Colour (0xff0c0710));
    g.fillEllipse (cx - r, cy - r + 4.f, d, d);
    g.setGradientFill (juce::ColourGradient (Colour (0xfffff4d6), cx - r * 0.3f, cy - r * 0.4f, Colour (0xffa8935a), cx + r, cy + r, true));
    g.fillEllipse (cx - r, cy - r, d, d);
    g.setColour (INKC);
    g.drawEllipse (cx - r, cy - r, d, d, 3.f);
    const float a = start + pos * (end - start);
    const float s = std::sin (a), c = -std::cos (a);
    g.setColour (PINK);
    g.drawLine (cx + s * r * 0.2f, cy + c * r * 0.2f, cx + s * r * 0.78f, cy + c * r * 0.78f, 4.f);
}

void FartLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (2.f);
    const bool on = b.getToggleState();
    Colour fill = b.findColour (juce::TextButton::buttonColourId);
    if (on) fill = LIME;
    if (over && ! on) fill = fill.brighter (0.12f);
    const float dy = down ? 2.f : 0.f;
    g.setColour (Colour (0xff0c0710));
    g.fillRoundedRectangle (r.translated (0, 3.f), 10.f);
    g.setColour (fill);
    g.fillRoundedRectangle (r.translated (0, dy), 10.f);
    g.setColour (INKC);
    g.drawRoundedRectangle (r.translated (0, dy), 10.f, 3.f);
    b.setColour (juce::TextButton::textColourOffId, on ? INKC : (b.findColour (juce::TextButton::buttonColourId) == PINK ? INKC : CREAM));
    b.setColour (juce::TextButton::textColourOnId, INKC);
}

void SkinButton::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (4.f);
    g.setColour (Colour ((juce::uint8) buttart::kSkins[idx][0], (juce::uint8) buttart::kSkins[idx][1], (juce::uint8) buttart::kSkins[idx][2]));
    g.fillEllipse (r);
    g.setColour (INKC);
    g.drawEllipse (r, 3.f);
    if (selected) { g.setColour (LIME); g.drawEllipse (r.expanded (3.f), 2.5f); }
}

FartSynthEditor::FartSynthEditor (FartSynthProcessor& p) : AudioProcessorEditor (&p), proc (p), butt (p)
{
    setLookAndFeel (&lnf);
    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);

    for (int i = 0; i < fart::kNumDefs; ++i)
    {
        auto k = std::make_unique<Knob>();
        k->def = i;
        k->slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        k->slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        k->slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, fart::kDefs[i].id, k->slider);
        k->slider.setDoubleClickReturnValue (true, fart::kDefs[i].def);
        k->slider.setPopupDisplayEnabled (false, false, nullptr);
        addAndMakeVisible (k->slider);
        knobs.push_back (std::move (k));
    }

    for (const auto& pr : fart::presets())
    {
        auto b = std::make_unique<juce::TextButton> (pr.name);
        b->setColour (juce::TextButton::buttonColourId, Colour (0xff2d2140));
        b->onClick = [this, &pr] { applyPreset (pr); };
        addAndMakeVisible (*b);
        presetBtns.push_back (std::move (b));
    }

    holdBtn.setColour (juce::TextButton::buttonColourId, Colour (0xff2d2140));
    holdBtn.setClickingTogglesState (true);
    holdBtn.setToggleState (true, juce::dontSendNotification);
    holdBtn.onClick = [this]
    {
        holdMode = holdBtn.getToggleState();
        holdBtn.setButtonText (holdMode ? "Hold" : "One-shot");
        if (! holdMode) holdBtn.setToggleState (false, juce::dontSendNotification);
        else holdBtn.setToggleState (true, juce::dontSendNotification);
    };
    addAndMakeVisible (holdBtn);
    randBtn.setColour (juce::TextButton::buttonColourId, PINK);
    randBtn.onClick = [this] { surprise(); };
    addAndMakeVisible (randBtn);

    for (int i = 0; i < 5; ++i)
    {
        skinBtns[i] = std::make_unique<SkinButton> (i);
        skinBtns[i]->onClick = [this] (int idx) { setSkin (idx); };
        addAndMakeVisible (*skinBtns[i]);
    }

    addAndMakeVisible (butt);
    butt.onPress = [this] { press(); };
    butt.onRelease = [this] { release(); };

    skinIdx = juce::jlimit (0, 4, (int) proc.apvts.state.getProperty ("skin", 1));
    butt.setSkin (skinIdx);
    for (int i = 0; i < 5; ++i) skinBtns[i]->selected = (i == skinIdx);
    startBake();

    setSize (W, H);   // last: resized() needs every child to exist
    lastTick = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    startTimerHz (30);
}

FartSynthEditor::~FartSynthEditor()
{
    stopTimer();
    proc.uiNoteOff();
    if (bakeFut.valid()) bakeFut.wait();
    setLookAndFeel (nullptr);
}

void FartSynthEditor::startBake()
{
    if (bakeFut.valid()) { bakeQueued = true; return; }
    const auto* s = buttart::kSkins[skinIdx];
    const int r = s[0], g = s[1], b = s[2];
    bakeFut = std::async (std::launch::async, [this, r, g, b] { return baker.bake (r, g, b); });
}

void FartSynthEditor::setSkin (int i)
{
    skinIdx = i;
    proc.apvts.state.setProperty ("skin", i, nullptr);
    butt.setSkin (i);
    for (int k = 0; k < 5; ++k) { skinBtns[k]->selected = (k == i); skinBtns[k]->repaint(); }
    startBake();
}

void FartSynthEditor::press()
{
    butt.kick();
    proc.uiNoteOn();
    if (! holdMode) oneShotEnd = juce::Time::getMillisecondCounterHiRes() / 1000.0 + prm ("length");
}

void FartSynthEditor::release()
{
    if (holdMode) proc.uiNoteOff();
}

void FartSynthEditor::applyPreset (const fart::Preset& pr)
{
    for (int i = 0; i < fart::kNumDefs; ++i)
    {
        float v = fart::kDefs[i].def;
        for (auto& kv : pr.v) if (juce::String (kv.first) == fart::kDefs[i].id) v = kv.second;
        if (auto* prmP = proc.apvts.getParameter (fart::kDefs[i].id))
        {
            prmP->beginChangeGesture();
            prmP->setValueNotifyingHost (prmP->convertTo0to1 (v));
            prmP->endChangeGesture();
        }
    }
}

void FartSynthEditor::surprise()
{
    juce::Random r;
    for (int i = 0; i < fart::kNumDefs; ++i)
    {
        const juce::String id (fart::kDefs[i].id);
        if (id == "volume" || id == "pressure") continue;
        float n = r.nextFloat();
        if (id == "body") n = 0.6f + n * 0.4f;
        if (id == "length") n = 0.3f + n * 0.5f;
        if (auto* prmP = proc.apvts.getParameter (id))
        {
            prmP->beginChangeGesture();
            prmP->setValueNotifyingHost (n);
            prmP->endChangeGesture();
        }
    }
}

void FartSynthEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    const double dt = juce::jmin (0.05, now - lastTick);
    lastTick = now;

    if (bakeFut.valid() && bakeFut.wait_for (std::chrono::milliseconds (0)) == std::future_status::ready)
    {
        butt.setTexture (bakeFut.get());
        if (bakeQueued) { bakeQueued = false; startBake(); }
    }
    if (oneShotEnd > 0 && now >= oneShotEnd) { oneShotEnd = 0; proc.uiNoteOff(); }

    butt.tick (dt);
    butt.repaint();
    proc.copyScope (scope, 512);
    repaint (scopeRect.expanded (4));
    repaint (psiRect.expanded (4));
    repaint (envRect.expanded (4));
    for (auto& k : knobs) repaint (k->slider.getBounds().translated (0, 54).withHeight (30).expanded (14, 0));
}

void FartSynthEditor::panel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    g.setColour (PANEL);
    g.fillRoundedRectangle (r.toFloat(), 18.f);
    g.setColour (EDGE);
    g.drawRoundedRectangle (r.toFloat().reduced (1.5f), 18.f, 3.f);
    g.setColour (PINK);
    g.setFont (black (15.f));
    g.drawText (title, r.getX() + 14, r.getY() + 10, r.getWidth() - 28, 20, juce::Justification::left);
}

void FartSynthEditor::drawEnv (juce::Graphics& g, juce::Rectangle<int> r)
{
    g.setColour (Colour (0xff150e1d)); g.fillRoundedRectangle (r.toFloat(), 10.f);
    g.setColour (EDGE); g.drawRoundedRectangle (r.toFloat().reduced (1.f), 10.f, 2.f);
    const float Wd = (float) r.getWidth(), Hd = (float) r.getHeight();
    const float atk = prm ("attack"), dec = prm ("decay"), rel = prm ("release"), sus = prm ("sustain");
    const float wA = 24 + 110 * std::sqrt (atk / 800.f), wD = 24 + 130 * std::sqrt (dec / 2500.f), wS = 90, wR = 24 + 130 * std::sqrt (rel / 3000.f);
    const float sc = (Wd - 24) / (wA + wD + wS + wR), pad = 12;
    auto y = [&] (float l) { return r.getY() + Hd - 10 - l * (Hd - 26); };
    float x0 = r.getX() + pad;
    juce::Point<float> pts[5];
    pts[0] = { x0, y (0) };
    x0 += wA * sc; pts[1] = { x0, y (1) };
    x0 += wD * sc; pts[2] = { x0, y (sus) };
    x0 += wS * sc; pts[3] = { x0, y (sus) };
    x0 += wR * sc; pts[4] = { x0, y (0) };
    juce::Path line;
    line.startNewSubPath (pts[0]);
    for (int i = 1; i < 5; ++i) line.lineTo (pts[i]);
    juce::Path fill (line);
    fill.lineTo (pts[4].x, (float) r.getBottom() - 2); fill.lineTo (pts[0].x, (float) r.getBottom() - 2); fill.closeSubPath();
    g.setColour (LIME.withAlpha (0.18f)); g.fillPath (fill);
    g.setColour (LIME); g.strokePath (line, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void FartSynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (BG);

    // header: logo + name
    if (logo.isValid())
    {
        g.setColour (PINK);
        g.drawImage (logo, 26, 18, 46, 46, 0, 0, logo.getWidth(), logo.getHeight(), true);
        g.setColour (PINK);
        g.drawImage (logo, 24, 16, 46, 46, 0, 0, logo.getWidth(), logo.getHeight(), false);
    }
    g.setFont (black (42.f));
    g.setColour (PINK);  g.drawText ("Fart Synth", 83, 15, 400, 52, juce::Justification::left);
    g.setColour (LIME);  g.drawText ("Fart Synth", 80, 12, 400, 52, juce::Justification::left);
    g.setColour (DIM);
    g.setFont (bold (11.f));
    g.drawText ("SKIN", skinBtns[0]->getX() - 52, skinBtns[0]->getY(), 46, skinBtns[0]->getHeight(), juce::Justification::centredRight);

    panel (g, pnlEnv, "PRESSURE");
    panel (g, pnlTone, "TONE");
    panel (g, pnlTex, "TEXTURE");
    panel (g, pnlOut, "OUTPUT");
    panel (g, pnlPre, "PRESETS");
    drawEnv (g, envRect);

    // knob labels + values
    for (auto& k : knobs)
    {
        const auto& d = fart::kDefs[k->def];
        auto b = k->slider.getBounds();
        g.setColour (CREAM); g.setFont (bold (10.f));
        g.drawText (juce::String (d.name).toUpperCase(), b.getX() - 14, b.getBottom() + 1, b.getWidth() + 28, 13, juce::Justification::centred);
        g.setColour (LIME); g.setFont (bold (10.f));
        g.drawText (fart::formatValue (d, (float) k->slider.getValue()), b.getX() - 14, b.getBottom() + 13, b.getWidth() + 28, 13, juce::Justification::centred);
    }

    // scope
    g.setColour (Colour (0xff150e1d)); g.fillRoundedRectangle (scopeRect.toFloat(), 14.f);
    g.setColour (EDGE); g.drawRoundedRectangle (scopeRect.toFloat().reduced (1.5f), 14.f, 3.f);
    {
        auto sr = scopeRect.toFloat().reduced (3.f);
        g.setColour (DIM.withAlpha (0.25f));
        g.drawLine (sr.getX(), sr.getCentreY(), sr.getRight(), sr.getCentreY(), 1.f);
        juce::Path p;
        for (int i = 0; i < 512; ++i)
        {
            const float x = sr.getX() + (float) i / 511.f * sr.getWidth(), yy = sr.getCentreY() - scope[i] * sr.getHeight() * 0.45f;
            if (i) p.lineTo (x, yy); else p.startNewSubPath (x, yy);
        }
        g.setColour (LIME.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (5.f));
        g.setColour (LIME); g.strokePath (p, juce::PathStrokeType (2.f));
    }

    // pressure gauge
    g.setColour (Colour (0xff150e1d)); g.fillRoundedRectangle (psiRect.toFloat(), 8.f);
    {
        const float lvl = juce::jmin (1.f, proc.voice.uiEnv.load() * prm ("pressure"));
        auto inner = psiRect.toFloat().reduced (2.f);
        auto bar = inner.withTop (inner.getBottom() - inner.getHeight() * lvl);
        g.setGradientFill (juce::ColourGradient (PINK, 0, inner.getY(), LIME, 0, inner.getBottom(), false));
        g.fillRect (bar);
    }
    g.setColour (EDGE); g.drawRoundedRectangle (psiRect.toFloat().reduced (1.f), 8.f, 2.f);
}

void FartSynthEditor::resized()
{
    const int top = 80;
    // columns
    const int lx = 16, lw = 270, cx = 300, cw = 500, rx = 814, rw = 270;

    // left
    pnlEnv = { lx, top, lw, 296 };
    envRect = { lx + 16, top + 36, lw - 32, 70 };
    pnlTone = { lx, pnlEnv.getBottom() + 12, lw, 210 };
    // right
    pnlTex = { rx, top, rw, 210 };
    pnlOut = { rx, pnlTex.getBottom() + 12, rw, 130 };
    pnlPre = { rx, pnlOut.getBottom() + 12, rw, H - 16 - (pnlOut.getBottom() + 12) };

    auto place = [this] (const char* group, juce::Rectangle<int> area, int cols, int rowH)
    {
        int n = 0;
        for (auto& k : knobs)
            if (juce::String (fart::kDefs[k->def].group) == group)
            {
                const int cellW = area.getWidth() / cols;
                const int col = n % cols, row = n / cols;
                const int cxk = area.getX() + col * cellW + cellW / 2;
                k->slider.setBounds (cxk - 27, area.getY() + row * rowH, 54, 54);
                ++n;
            }
    };
    place ("env",  { pnlEnv.getX() + 8, envRect.getBottom() + 12, pnlEnv.getWidth() - 16, 0 }, 3, 88);
    place ("tone", { pnlTone.getX() + 8, pnlTone.getY() + 36, pnlTone.getWidth() - 16, 0 }, 3, 88);
    place ("tex",  { pnlTex.getX() + 8, pnlTex.getY() + 36, pnlTex.getWidth() - 16, 0 }, 3, 88);
    psiRect = { pnlOut.getX() + 18, pnlOut.getY() + 14 + 22, 22, 84 };
    place ("out",  { pnlOut.getX() + 52, pnlOut.getY() + 36, pnlOut.getWidth() - 60, 0 }, 2, 88);

    // presets grid
    {
        auto r = pnlPre.reduced (12, 0).withTrimmedTop (38).withTrimmedBottom (12);
        const int bw = (r.getWidth() - 8) / 2, bh = 32;
        int n = 0;
        for (auto& b : presetBtns)
        {
            b->setBounds (r.getX() + (n % 2) * (bw + 8), r.getY() + (n / 2) * (bh + 4), bw, bh);
            ++n;
        }
        const int y2 = r.getY() + ((n + 1) / 2) * (bh + 4) + 6;
        holdBtn.setBounds (r.getX(), y2, bw, bh);
        randBtn.setBounds (r.getX() + bw + 8, y2, bw, bh);
    }

    butt.setBounds (cx, top, cw, (int) (cw * 640.0 / 600.0));
    scopeRect = { cx, butt.getBottom() + 10, cw, H - 16 - (butt.getBottom() + 10) };

    for (int i = 0; i < 5; ++i) skinBtns[i]->setBounds (W - 16 - (5 - i) * 32, 22, 32, 32);
}
