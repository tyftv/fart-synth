// Offscreen test: renders audio from the engine to a WAV-ish stats print and paints the butt view to PNG.
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/ButtView.h"
#include "../Source/PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    FartSynthProcessor proc;
    proc.prepareToPlay (44100, 512);
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 48, (juce::uint8) 100), 0);
    double peak = 0, sum = 0; int cnt = 0;
    juce::AudioBuffer<float> all (1, 44100 * 2);
    for (int b = 0; b < 44100 * 2 / 512; ++b)
    {
        if (b == 60) midi.addEvent (juce::MidiMessage::noteOff (1, 48), 0);
        proc.processBlock (buf, midi); midi.clear();
        all.copyFrom (0, b * 512, buf, 0, 0, 512);
        for (int i = 0; i < 512; ++i) { peak = std::max (peak, (double) std::abs (buf.getSample (0, i))); sum += buf.getSample (0, i) * buf.getSample (0, i); ++cnt; }
    }
    std::printf ("peak %.3f rms %.3f\n", peak, std::sqrt (sum / cnt));
    { juce::File f ("/tmp/fart.wav"); f.deleteFile(); juce::WavAudioFormat w; std::unique_ptr<juce::AudioFormatWriter> wr (w.createWriterFor (new juce::FileOutputStream (f), 44100, 1, 16, {}, 0)); if (wr) wr->writeFromAudioSampleBuffer (all, 0, all.getNumSamples()); }

    // bake + render the view
    buttart::Baker baker;
    auto t0 = juce::Time::getMillisecondCounterHiRes();
    auto tex = baker.bake (224, 163, 122);
    std::printf ("bake %.0f ms\n", juce::Time::getMillisecondCounterHiRes() - t0);
    { juce::PNGImageFormat pp; juce::File tf ("/tmp/tex.png"); tf.deleteFile(); juce::FileOutputStream ts (tf); pp.writeImageToStream (tex, ts); }
    ButtView view (proc);
    view.setBounds (0, 0, 500, 533);
    view.setTexture (tex);
    proc.voice.on (0, true);
    juce::AudioBuffer<float> b2 (2, 512); juce::MidiBuffer m2;
    for (int f = 0; f < 40; ++f) { proc.processBlock (b2, m2); proc.processBlock (b2, m2); view.tick (1.0 / 30); }
    t0 = juce::Time::getMillisecondCounterHiRes();
    juce::Image img (juce::Image::ARGB, 500, 533, true);
    { juce::Graphics g (img); view.paint (g); }
    t0 = juce::Time::getMillisecondCounterHiRes();
    for (int k = 0; k < 10; ++k) { view.tick (1.0 / 30); juce::Graphics g (img); view.paint (g); }
    std::printf ("paint avg %.1f ms\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 10);
    juce::PNGImageFormat png;
    { juce::File out ("/tmp/butt.png"); out.deleteFile(); juce::FileOutputStream os (out); png.writeImageToStream (img, os); }
    std::fflush (stdout);

    std::printf ("view ok\n"); std::fflush (stdout);
    // full editor
    FartSynthEditor ed (proc);
    ed.setBounds (0, 0, 1100, 700);
    juce::MessageManager::getInstance();
    for (int i = 0; i < 60; ++i) { juce::Thread::sleep (40); }
    ed.repaint();
    auto shot = ed.createComponentSnapshot (ed.getLocalBounds());
    { juce::File o2 ("/tmp/editor.png"); o2.deleteFile(); juce::FileOutputStream os2 (o2); png.writeImageToStream (shot, os2); }
    return 0;
}
