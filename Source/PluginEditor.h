#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <future>
#include "PluginProcessor.h"
#include "ButtView.h"

class FartLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return juce::Font (juce::FontOptions (12.f, juce::Font::bold)); }
};

class SkinButton : public juce::Component
{
public:
    SkinButton (int i) : idx (i) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick (idx); }
    bool selected = false;
    std::function<void (int)> onClick;
private:
    int idx;
};

class FartSynthEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit FartSynthEditor (FartSynthProcessor&);
    ~FartSynthEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        int def = 0;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    void timerCallback() override;
    void applyPreset (const fart::Preset&);
    void surprise();
    void setSkin (int i);
    void startBake();
    void press();
    void release();
    float prm (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }
    void panel (juce::Graphics&, juce::Rectangle<int>, const juce::String& title);
    void drawEnv (juce::Graphics&, juce::Rectangle<int>);

    FartSynthProcessor& proc;
    FartLookAndFeel lnf;
    ButtView butt;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<juce::TextButton>> presetBtns;
    juce::TextButton holdBtn { "Hold" }, randBtn { "Surprise me" };
    std::unique_ptr<SkinButton> skinBtns[5];
    juce::Image logo;
    juce::Rectangle<int> envRect, scopeRect, psiRect, pnlEnv, pnlTone, pnlTex, pnlOut, pnlPre;
    float scope[512] {};
    bool holdMode = true;
    double oneShotEnd = 0, lastTick = 0;

    buttart::Baker baker;
    std::future<juce::Image> bakeFut;
    bool bakeQueued = false;
    int skinIdx = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FartSynthEditor)
};
