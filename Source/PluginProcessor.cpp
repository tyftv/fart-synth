#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace fart;

juce::AudioProcessorValueTreeState::ParameterLayout FartSynthProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    for (const auto& d : kDefs)
    {
        juce::NormalisableRange<float> range (d.mn, d.mx);
        if (d.expo)
        {
            const float mn = d.mn, mx = d.mx;
            range = juce::NormalisableRange<float> (
                mn, mx,
                [mn, mx] (float, float, float n) { return mn * std::pow (mx / mn, juce::jlimit (0.f, 1.f, n)); },
                [mn, mx] (float, float, float v) { return std::log (juce::jlimit (mn, mx, v) / mn) / std::log (mx / mn); });
        }
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { d.id, 1 }, d.name, range, d.def,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction ([d] (float v, int) { return formatValue (d, v); })));
    }
    return { params.begin(), params.end() };
}

FartSynthProcessor::FartSynthProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "FartSynth", createLayout())
{
    for (int i = 0; i < kNumDefs; ++i)
        raw[i] = apvts.getRawParameterValue (kDefs[i].id);
    held.reserve (32);
}

bool FartSynthProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || l.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void FartSynthProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    voice.prepare (sampleRate);
    mono.setSize (1, juce::jmax (1, samplesPerBlock));
    held.clear();
}

void FartSynthProcessor::loadParams (VoiceParams& p) const
{
    auto g = [this] (const char* id) { return (double) raw[indexOf (id)]->load(); };
    p.pressure = g ("pressure"); p.attack = g ("attack"); p.decay = g ("decay"); p.sustain = g ("sustain");
    p.release = g ("release"); p.length = g ("length"); p.pitch = g ("pitch"); p.tight = g ("tight");
    p.flutter = g ("flutter"); p.slide = g ("slide"); p.cavity = g ("cavity"); p.bright = g ("bright");
    p.wet = g ("wet"); p.sputter = g ("sputter"); p.rumble = g ("rumble"); p.gas = g ("gas");
    p.body = g ("body"); p.drive = g ("drive"); p.volume = g ("volume");
}

// MIDI note 48 (C3) = the base pitch of the voice
void FartSynthProcessor::handleNoteOn (int midiNote)
{
    const int n = midiNote - 48;
    const bool was = ! held.empty();
    held.push_back (n);
    voice.on (n, ! was);
}

void FartSynthProcessor::handleNoteOff (int midiNote)
{
    const int n = midiNote - 48;
    for (size_t i = 0; i < held.size(); ++i)
        if (held[i] == n) { held.erase (held.begin() + (long) i); break; }
    if (held.empty()) voice.off();
    else voice.setNote (held.back());
}

void FartSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    if (mono.getNumSamples() < numSamples) mono.setSize (1, numSamples, false, false, true);

    VoiceParams P;
    loadParams (P);

    // click-on-butt from the editor
    const int cmd = uiCmd.exchange (0);
    if (cmd == 1) { held.clear(); voice.on (0, true); }
    else if (cmd == 2) { held.clear(); voice.off(); }

    float* m = mono.getWritePointer (0);
    int pos = 0;
    auto renderTo = [&] (int end)
    {
        if (end > pos) { voice.render (m + pos, end - pos, P); pos = end; }
    };

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        renderTo (juce::jlimit (0, numSamples, meta.samplePosition));
        if (msg.isNoteOn()) handleNoteOn (msg.getNoteNumber());
        else if (msg.isNoteOff()) handleNoteOff (msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) { held.clear(); voice.off(); }
    }
    renderTo (numSamples);

    int sp = scopePos.load();
    for (int i = 0; i < numSamples; ++i)
    {
        const float s = std::tanh (m[i]);
        m[i] = s;
        scopeRing[sp] = s;
        sp = (sp + 1) % kScope;
    }
    scopePos.store (sp);

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, m, numSamples);
}

void FartSynthProcessor::copyScope (float* dest, int n) const
{
    n = juce::jmin (n, kScope);
    int p = scopePos.load() - n;
    if (p < 0) p += kScope;
    for (int i = 0; i < n; ++i) { dest[i] = scopeRing[p]; p = (p + 1) % kScope; }
}

void FartSynthProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void FartSynthProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* FartSynthProcessor::createEditor() { return new FartSynthEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new FartSynthProcessor(); }
