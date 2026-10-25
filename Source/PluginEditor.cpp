#include "PluginEditor.h"

namespace
{
const juce::Rectangle<int> kTiles { 20, 330, 480, 62 };
}

VibeSparkEditor::VibeSparkEditor (VibeSparkProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p), pad (p)
{
    setSize (520, 460);

    vibe.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    vibe.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    vibe.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xFFA78BFA));
    addAndMakeVisible (vibe);

    for (auto* s : { &space, &human })
    {
        s->setSliderStyle (juce::Slider::LinearHorizontal);
        s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 20);
        s->setColour (juce::Slider::trackColourId, juce::Colour (0xFFA78BFA));
        addAndMakeVisible (*s);
    }

    const auto& names = vs::styleNames();
    for (size_t i = 0; i < names.size(); ++i) style.addItem (names[i], (int) i + 1);
    key.addItem ("Auto", 1);
    const char* kn[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int i = 0; i < 12; ++i) key.addItem (kn[i], i + 2);
    addAndMakeVisible (style);
    addAndMakeVisible (key);
    addAndMakeVisible (evolve);

    spark.setColour (juce::TextButton::buttonColourId, juce::Colour (0xFF7C3AED));
    spark.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    spark.onClick = [this] { proc.spark(); repaint(); };
    addAndMakeVisible (spark);
    addAndMakeVisible (pad);

    vibeA   = std::make_unique<SA> (proc.apvts, "vibe", vibe);
    spaceA  = std::make_unique<SA> (proc.apvts, "space", space);
    humanA  = std::make_unique<SA> (proc.apvts, "human", human);
    styleA  = std::make_unique<CA> (proc.apvts, "style", style);
    keyA    = std::make_unique<CA> (proc.apvts, "key", key);
    evolveA = std::make_unique<BA> (proc.apvts, "evolve", evolve);

    startTimerHz (15);
}

VibeSparkEditor::~VibeSparkEditor() { stopTimer(); }

void VibeSparkEditor::resized()
{
    vibe.setBounds (20, 78, 170, 170);
    space.setBounds (215, 82, 285, 24);
    human.setBounds (215, 132, 285, 24);
    style.setBounds (215, 182, 140, 28);
    key.setBounds (365, 182, 135, 28);
    evolve.setBounds (215, 220, 200, 24);
    spark.setBounds (20, 262, 480, 56);
    pad.setBounds (20, 412, 300, 38);
}

void VibeSparkEditor::timerCallback()
{
    const int bar = proc.currentBar.load();
    if (bar != lastBar) { lastBar = bar; repaint(); }
}

void VibeSparkEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xFF0E0B1A), 0.0f, 0.0f,
                                             juce::Colour (0xFF2A1450), 0.0f, (float) getHeight(), false));
    g.fillAll();

    g.setColour (juce::Colours::white);
    g.setFont (28.0f);
    g.drawText ("VibeSpark", 20, 14, 300, 34, juce::Justification::centredLeft);
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.setFont (13.0f);
    g.drawText ("R&B chord generator", 20, 46, 300, 18, juce::Justification::centredLeft);

    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.setFont (12.0f);
    g.drawText ("VIBE   cold  <->  warm", 20, 248, 170, 14, juce::Justification::centred);
    g.drawText ("SPACE  tight  <->  wide", 215, 64, 285, 16, juce::Justification::centredLeft);
    g.drawText ("HUMAN  robot  <->  live", 215, 114, 285, 16, juce::Justification::centredLeft);
    g.drawText ("STYLE", 215, 166, 140, 14, juce::Justification::centredLeft);
    g.drawText ("KEY", 365, 166, 135, 14, juce::Justification::centredLeft);

    // chord tiles
    auto loop = proc.getLoop();
    const int bar = proc.currentBar.load();
    const int w = (kTiles.getWidth() - 3 * 8) / 4;
    for (int i = 0; i < 4; ++i)
    {
        juce::Rectangle<int> r (kTiles.getX() + i * (w + 8), kTiles.getY(), w, kTiles.getHeight());
        g.setColour (i == bar ? juce::Colour (0xFF7C3AED) : juce::Colour (0xFF1E1B2E));
        g.fillRoundedRectangle (r.toFloat(), 10.0f);
        g.setColour (juce::Colour (0xFF8B7CF6).withAlpha (0.6f));
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 10.0f, 1.0f);
        g.setColour (juce::Colours::white);
        g.setFont (17.0f);
        g.drawText (loop ? juce::String (loop->chordNames[(size_t) i]) : juce::String ("-"), r,
                    juce::Justification::centred);
    }

    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.setFont (12.0f);
    if (loop)
        g.drawText ("Key " + juce::String (loop->keyName) + "  |  " + juce::String (loop->progName),
                    20, 392, 480, 14, juce::Justification::centredLeft);
    g.drawText (juce::String (juce::roundToInt (proc.hostBpm.load())) + " BPM  (host sync)",
                330, 421, 170, 20, juce::Justification::centredRight);
}
