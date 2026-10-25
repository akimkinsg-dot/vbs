#pragma once
// VibeSpark generative engine (no JUCE dependency, unit-testable on its own).
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace vs
{
struct GenParams
{
    double vibe  = 0.5;    // 0 = cold (m11, sus, quartal)  ->  1 = warm (maj9, 6/9, 13)
    double space = 0.5;    // 0 = tight closed voicings     ->  1 = wide open voicings
    double human = 0.7;    // amount of micro-timing / velocity humanisation
    double bpm   = 90.0;   // used to pick rhythm density
    int    style = 0;      // index into styleTable()
    int    keyPc = -1;     // -1 = random key, 0..11 = C..B
    int    avoidProg = -1; // previous progression index (to avoid repeating)
};

struct Note
{
    int    pitch = 60;
    double start = 0.0;    // beats from loop start (16 beats = 4 bars of 4/4)
    double len   = 1.0;    // beats
    double offMs = 0.0;    // micro-timing offset in milliseconds
    int    vel   = 64;
};

struct Loop
{
    std::array<std::vector<Note>, 4> passes;     // A, A', A'', A''' - evolving variations
    std::array<std::string, 4> chordNames;
    std::string keyName;
    std::string progName;
    int progIndex = 0;
};

Loop generate(const GenParams& gp, uint32_t seed);
const std::vector<std::string>& styleNames();
} // namespace vs
