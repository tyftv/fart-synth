#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "FartParams.h"
#include "FartVoice.h"

class FartSynthProcessor : public juce::AudioProcessor
{
public:
    FartSynthProcessor();
    ~FartSynthProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Fart Synth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // used by the editor (click the butt to play)
    void uiNoteOn()  { uiCmd.store (1); }
    void uiNoteOff() { uiCmd.store (2); }

    // read the last n output samples (for the scope / sphincter ring)
    void copyScope (float* dest, int n) const;

    juce::AudioProcessorValueTreeState apvts;
    fart::Voice voice;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void loadParams (fart::VoiceParams&) const;
    void handleNoteOn (int midiNote);
    void handleNoteOff (int midiNote);

    std::atomic<float>* raw[fart::kNumDefs] {};
    std::atomic<int> uiCmd { 0 };
    std::vector<int> held;

    static constexpr int kScope = 4096;
    float scopeRing[kScope] {};
    std::atomic<int> scopePos { 0 };
    juce::AudioBuffer<float> mono;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FartSynthProcessor)
};
