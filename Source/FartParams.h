#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

namespace fart
{
struct Def
{
    const char* group; const char* id; const char* name;
    float mn, mx, def; bool expo; const char* fmt;
};

static const Def kDefs[] = {
    { "env",  "pressure", "Pressure", 0.f,   1.f,    0.8f,  false, "pct" },
    { "env",  "attack",   "Attack",   3.f,   800.f,  15.f,  true,  "ms"  },
    { "env",  "decay",    "Decay",    10.f,  2500.f, 300.f, true,  "ms"  },
    { "env",  "sustain",  "Sustain",  0.f,   1.f,    0.6f,  false, "pct" },
    { "env",  "release",  "Release",  10.f,  3000.f, 250.f, true,  "ms"  },
    { "env",  "length",   "Length",   0.1f,  5.f,    1.2f,  true,  "s"   },
    { "tone", "pitch",    "Pitch",    -24.f, 24.f,   0.f,   false, "st"  },
    { "tone", "tight",    "Tightness",0.f,   1.f,    0.4f,  false, "pct" },
    { "tone", "flutter",  "Flutter",  0.f,   1.f,    0.3f,  false, "pct" },
    { "tone", "slide",    "Slide",    -1.f,  1.f,    0.5f,  false, "bi"  },
    { "tone", "cavity",   "Cavity",   80.f,  1500.f, 275.f, true,  "hz"  },
    { "tone", "bright",   "Bright",   500.f, 10000.f,2400.f,true,  "hz"  },
    { "tex",  "wet",      "Wetness",  0.f,   1.f,    0.3f,  false, "pct" },
    { "tex",  "sputter",  "Sputter",  0.f,   1.f,    0.2f,  false, "pct" },
    { "tex",  "rumble",   "Rumble",   0.f,   1.f,    0.2f,  false, "pct" },
    { "tex",  "gas",      "Gas",      0.f,   1.f,    0.2f,  false, "pct" },
    { "tex",  "body",     "Body",     0.f,   1.f,    0.9f,  false, "pct" },
    { "out",  "drive",    "Drive",    0.f,   1.f,    0.2f,  false, "pct" },
    { "out",  "volume",   "Volume",   0.f,   1.f,    0.7f,  false, "pct" },
};
static constexpr int kNumDefs = (int) (sizeof (kDefs) / sizeof (kDefs[0]));

inline int indexOf (const juce::String& id)
{
    for (int i = 0; i < kNumDefs; ++i)
        if (id == kDefs[i].id) return i;
    return -1;
}

inline juce::String formatValue (const Def& d, float v)
{
    juce::String f (d.fmt);
    if (f == "pct") return juce::String (juce::roundToInt (v * 100.f)) + "%";
    if (f == "ms")  return v >= 1000.f ? juce::String (v / 1000.f, 2) + "s" : juce::String (juce::roundToInt (v)) + "ms";
    if (f == "s")   return juce::String (v, 2) + "s";
    if (f == "st")  return (v > 0.f ? "+" : "") + juce::String (v, 1) + "st";
    if (f == "hz")  return juce::String (juce::roundToInt (v)) + "Hz";
    if (f == "bi")  return (v > 0.f ? "+" : "") + juce::String (juce::roundToInt (v * 100.f)) + "%";
    return juce::String (v, 2);
}

struct Preset { const char* name; std::vector<std::pair<const char*, float>> v; };

inline const std::vector<Preset>& presets()
{
    static const std::vector<Preset> p = {
        { "Classic", {} },
        { "Squeaker", { {"pitch",14},{"tight",.95f},{"flutter",.15f},{"sputter",.1f},{"wet",0},{"attack",5},{"decay",120},{"sustain",.3f},{"release",80},{"length",.5f},{"cavity",700},{"slide",.2f},{"gas",.1f},{"rumble",0} } },
        { "Wet Squelch", { {"pitch",-3},{"tight",.3f},{"wet",.9f},{"sputter",.5f},{"flutter",.6f},{"cavity",250},{"slide",.7f},{"length",1},{"rumble",.3f},{"drive",.35f} } },
        { "Long Rumbler", { {"pitch",-14},{"tight",.2f},{"flutter",.25f},{"rumble",.8f},{"sustain",.8f},{"attack",120},{"decay",800},{"release",900},{"length",3.5f},{"cavity",170},{"slide",.3f},{"wet",.2f},{"bright",1800} } },
        { "Trumpet", { {"pitch",4},{"tight",.7f},{"flutter",.05f},{"sputter",0},{"wet",0},{"rumble",.05f},{"sustain",.9f},{"attack",30},{"cavity",500},{"bright",6000},{"drive",.5f},{"slide",-.1f},{"length",1.2f} } },
        { "Silent But Deadly", { {"body",.08f},{"gas",.95f},{"pressure",.6f},{"attack",200},{"sustain",.7f},{"release",600},{"length",3},{"bright",2500},{"cavity",900},{"wet",0},{"sputter",0} } },
        { "Machine Gun", { {"sputter",.9f},{"flutter",.5f},{"tight",.7f},{"pitch",4},{"length",1.4f},{"wet",.2f},{"decay",600},{"sustain",.7f},{"rumble",.1f} } },
    };
    return p;
}
} // namespace fart
