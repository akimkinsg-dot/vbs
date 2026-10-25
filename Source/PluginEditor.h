#pragma once
#include "PluginProcessor.h"

// Drag this pad into an Ableton track / clip slot to drop the current loop as a .mid file.
// A plain click reveals the file in Finder as a fallback.
class DragPad : public juce::Component
{
public:
    explicit DragPad (VibeSparkProcessor& p) : proc (p) {}

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (juce::Colour (0xFF241F3A));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (juce::Colour (0xFF8B7CF6));
        g.drawRoundedRectangle (r, 8.0f, 1.2f);
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.setFont (14.0f);
        g.drawText ("DRAG MIDI  >  Ableton track", getLocalBounds(), juce::Justification::centred);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 8) return;
        dragging = true;
        auto f = proc.exportMidiFile();
        if (f.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this,
                                                                         [this] { dragging = false; });
        else
            dragging = false;
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! dragging && e.getDistanceFromDragStart() < 8)
            proc.exportMidiFile().revealToUser();
        dragging = false;
    }

private:
    VibeSparkProcessor& proc;
    bool dragging = false;
};

class VibeSparkEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VibeSparkEditor (VibeSparkProcessor&);
    ~VibeSparkEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    VibeSparkProcessor& proc;

    juce::Slider vibe, space, human;
    juce::ComboBox style, key;
    juce::ToggleButton evolve { "Evolve loops" };
    juce::TextButton spark { "SPARK" };
    DragPad pad;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SA> vibeA, spaceA, humanA;
    std::unique_ptr<CA> styleA, keyA;
    std::unique_ptr<BA> evolveA;

    int lastBar = -2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibeSparkEditor)
};
