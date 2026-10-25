#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include <memory>
#include <vector>
#include "Engine.h"

class VibeSparkProcessor : public juce::AudioProcessor
{
public:
    VibeSparkProcessor();
    ~VibeSparkProcessor() override = default;

    void prepareToPlay (double, int) override { pending.reserve (512); }
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override
    {
        return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VibeSpark"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }   // Ableton refuses to load "MIDI effect" VST3s
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // --- called from the UI
    void spark();                                   // generate a brand-new 4-bar loop
    std::shared_ptr<const vs::Loop> getLoop() const;
    juce::File exportMidiFile();                    // writes a .mid for drag & drop

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<double> hostBpm { 90.0 };
    std::atomic<int> currentBar { -1 };

private:
    void regenerate (uint32_t seed, double bpm, bool avoidLast);
    void flushPending (juce::MidiBuffer& midi, int samplePos);

    struct Pending { int pitch; double offPpq; };

    mutable juce::SpinLock loopLock;
    std::shared_ptr<const vs::Loop> loop;           // guarded by loopLock
    std::shared_ptr<const vs::Loop> audioLoop;      // audio thread copy
    std::atomic<int> loopVersion { 0 };
    int seenVersion = 0;

    std::vector<Pending> pending;                   // audio thread only
    double expectedPpq = -1.0;

    uint32_t currentSeed = 1;
    double currentBpm = 90.0;
    int lastProg = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibeSparkProcessor)
};
