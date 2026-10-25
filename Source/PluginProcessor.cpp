#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "vibe", 1 }, "Vibe",
           NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.45f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "space", 1 }, "Space",
           NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "human", 1 }, "Human",
           NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.7f));

    StringArray styles;
    for (auto& s : vs::styleNames()) styles.add (s);
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "style", 1 }, "Style", styles, 0));

    StringArray keys { "Auto", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 }, "Key", keys, 0));

    l.add (std::make_unique<AudioParameterBool> (ParameterID { "evolve", 1 }, "Evolve", true));
    return l;
}
} // namespace

VibeSparkProcessor::VibeSparkProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VIBESPARK", makeLayout())
{
    regenerate ((uint32_t) juce::Random::getSystemRandom().nextInt(), 90.0, false);
}

juce::AudioProcessorEditor* VibeSparkProcessor::createEditor() { return new VibeSparkEditor (*this); }

std::shared_ptr<const vs::Loop> VibeSparkProcessor::getLoop() const
{
    const juce::SpinLock::ScopedLockType l (loopLock);
    return loop;
}

void VibeSparkProcessor::spark()
{
    regenerate ((uint32_t) juce::Random::getSystemRandom().nextInt(), hostBpm.load(), true);
}

void VibeSparkProcessor::regenerate (uint32_t seed, double bpm, bool avoidLast)
{
    vs::GenParams gp;
    gp.vibe  = apvts.getRawParameterValue ("vibe")->load();
    gp.space = apvts.getRawParameterValue ("space")->load();
    gp.human = apvts.getRawParameterValue ("human")->load();
    gp.style = (int) std::lround (apvts.getRawParameterValue ("style")->load());
    gp.keyPc = (int) std::lround (apvts.getRawParameterValue ("key")->load()) - 1;   // 0 = Auto -> -1
    gp.bpm   = bpm;
    gp.avoidProg = avoidLast ? lastProg : -1;

    auto fresh = std::make_shared<const vs::Loop> (vs::generate (gp, seed));
    lastProg = fresh->progIndex;
    currentSeed = seed;
    currentBpm = bpm;
    {
        const juce::SpinLock::ScopedLockType l (loopLock);
        loop = fresh;
    }
    loopVersion.fetch_add (1);
}

void VibeSparkProcessor::flushPending (juce::MidiBuffer& midi, int samplePos)
{
    for (auto& p : pending)
        midi.addEvent (juce::MidiMessage::noteOff (1, p.pitch), samplePos);
    pending.clear();
}

void VibeSparkProcessor::processBlock (juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    audio.clear();
    midi.clear();                                   // we are a generator: incoming MIDI is not forwarded
    const int n = audio.getNumSamples();

    {
        juce::SpinLock::ScopedTryLockType l (loopLock);
        if (l.isLocked()) audioLoop = loop;
    }

    const int ver = loopVersion.load();
    if (ver != seenVersion) { flushPending (midi, 0); seenVersion = ver; }

    double bpm = 120.0, ppq = 0.0;
    bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            playing = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition()) ppq = *p; else playing = false;
        }
    hostBpm.store (bpm);

    if (! playing || audioLoop == nullptr || getSampleRate() <= 0.0)
    {
        flushPending (midi, 0);
        expectedPpq = -1.0;
        return;
    }

    const double bps = bpm / 60.0 / getSampleRate();        // beats per sample
    const double p0 = ppq, p1 = ppq + n * bps;
    if (expectedPpq >= 0.0 && std::abs (p0 - expectedPpq) > 0.05)   // transport jump / loop wrap
        flushPending (midi, 0);
    expectedPpq = p1;
    currentBar.store ((int) std::floor (std::fmod (p0, 16.0) / 4.0));

    const bool evolve = apvts.getRawParameterValue ("evolve")->load() > 0.5f;
    const double msToBeats = bpm / 60000.0;

    auto emitOffs = [&]
    {
        for (size_t i = 0; i < pending.size();)
        {
            if (pending[i].offPpq < p1)
            {
                const int s = juce::jlimit (0, n - 1, (int) ((pending[i].offPpq - p0) / bps));
                midi.addEvent (juce::MidiMessage::noteOff (1, pending[i].pitch), s);
                pending[i] = pending.back();
                pending.pop_back();
            }
            else ++i;
        }
    };

    emitOffs();

    const int k0 = (int) std::floor (p0 / 16.0) - 1;
    const int k1 = (int) std::floor (p1 / 16.0) + 1;
    for (int k = juce::jmax (0, k0); k <= k1; ++k)
    {
        const int pass = evolve ? (((k % 4) + 4) % 4) : 0;
        for (const auto& nt : audioLoop->passes[(size_t) pass])
        {
            const double t = k * 16.0 + nt.start + nt.offMs * msToBeats;
            if (t >= p0 && t < p1)
            {
                const int s = juce::jlimit (0, n - 1, (int) ((t - p0) / bps));
                midi.addEvent (juce::MidiMessage::noteOn (1, nt.pitch, (juce::uint8) nt.vel), s);
                pending.push_back ({ nt.pitch, t + nt.len - 0.01 });
            }
        }
    }

    emitOffs();
}

juce::File VibeSparkProcessor::exportMidiFile()
{
    auto lp = getLoop();
    if (lp == nullptr) return {};

    const double bpm = juce::jlimit (30.0, 300.0, hostBpm.load());
    const bool evolve = apvts.getRawParameterValue ("evolve")->load() > 0.5f;
    const int passes = evolve ? 4 : 1;
    const int ppq = 960;

    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4));
    seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) (60000000.0 / bpm)));

    for (int p = 0; p < passes; ++p)
        for (const auto& nt : lp->passes[(size_t) p])
        {
            double t = (p * 16.0 + nt.start) * ppq + nt.offMs * bpm / 60000.0 * ppq;
            if (t < 0.0) t = 0.0;
            auto on = juce::MidiMessage::noteOn (1, nt.pitch, (juce::uint8) nt.vel);
            on.setTimeStamp (t);
            seq.addEvent (on);
            auto off = juce::MidiMessage::noteOff (1, nt.pitch);
            off.setTimeStamp (t + nt.len * ppq - 8.0);
            seq.addEvent (off);
        }
    seq.updateMatchedPairs();

    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (ppq);
    mf.addTrack (seq);

    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("VibeSpark");
    dir.createDirectory();
    auto file = dir.getChildFile ("VibeSpark_" + juce::String (lp->keyName) + "_" + juce::String (juce::roundToInt (bpm)) + "bpm.mid");
    file.deleteFile();
    if (auto os = file.createOutputStream())
    {
        mf.writeTo (*os);
        os->flush();
    }
    return file;
}

void VibeSparkProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("seed", (int) currentSeed, nullptr);
    state.setProperty ("bpm", currentBpm, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void VibeSparkProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            apvts.replaceState (vt);
            regenerate ((uint32_t) (int) vt.getProperty ("seed", 1),
                        (double) vt.getProperty ("bpm", 90.0), false);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VibeSparkProcessor(); }
