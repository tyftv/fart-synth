#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "ButtArt.h"
#include "PluginProcessor.h"

// The butt: baked 3D-lit texture + animated cheeks, sphincter, gas, flies and a scope ring.
class ButtView : public juce::Component
{
public:
    explicit ButtView (FartSynthProcessor& p) : proc (p) { setOpaque (true); }

    void setTexture (juce::Image img) { tex = std::move (img); }
    void setSkin (int i) { skin = juce::jlimit (0, 4, i); }
    int getSkin() const { return skin; }
    void kick();
    void tick (double dt);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { hintGone = true; if (onPress) onPress(); }
    void mouseUp (const juce::MouseEvent&) override { if (onRelease) onRelease(); }

    std::function<void()> onPress, onRelease;

private:
    struct Half { float s, ox = 0, vx = 0, oy = 0, vy = 0, sq = 0, vsq = 0; };
    struct Part { float x, y, vx, vy, r, g, life, max, h, l; int type; };
    struct Ring { float r, a; };
    struct Stink { float x, y, life, max, ph; };
    struct Fly { float a, sp, rx, ry, ph, life, max; };

    float prm (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }
    void drawBG (juce::Graphics&);
    void drawCrack (juce::Graphics&);
    void drawSphincter (juce::Graphics&);
    void drawHalf (juce::Graphics&, const Half&);

    FartSynthProcessor& proc;
    juce::Image tex;
    int skin = 1;
    Half halves[2] { { -1.f }, { 1.f } };
    std::vector<Part> parts; std::vector<Ring> rings; std::vector<Stink> stinks; std::vector<Fly> flies;
    float openV = 0, openAvg = 0, rms = 0, shX = 0, shY = 0, spawn = 0, ringT = 0, stinkT = 0, flap = 0, wetGate = 1;
    double t = 0, pressed = 0;
    bool hintGone = false;
    float buf[1024] {};
    juce::Random rng;
    float rnd() { return rng.nextFloat(); }
};
