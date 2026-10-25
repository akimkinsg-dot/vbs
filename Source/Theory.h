#pragma once
// VibeSpark - music theory data bank (no JUCE dependency).
#include <array>
#include <cstdint>
#include <vector>

namespace vs
{
// ---------------------------------------------------------------- chord qualities
enum QId { M7, M9, M11, Maj7, Maj9, Maj7s11, Six9, Dom7, Dom9, Dom13, Dom13sus,
           Dom7b9, Dom7alt, Dim7, HDim7, Sus2, P5Add9, Quartal4, MMaj7 };

struct QDef { const char* suffix; std::vector<int> iv; bool keepFifth; };

// Order MUST match QId.
inline const std::vector<QDef>& qualityTable()
{
    static const std::vector<QDef> t = {
        { "m7",      { 0, 3, 7, 10 },            false },
        { "m9",      { 0, 3, 7, 10, 14 },        false },
        { "m11",     { 0, 3, 7, 10, 14, 17 },    false },
        { "maj7",    { 0, 4, 7, 11 },            false },
        { "maj9",    { 0, 4, 7, 11, 14 },        false },
        { "maj7#11", { 0, 4, 7, 11, 18 },        false },
        { "6/9",     { 0, 4, 7, 9, 14 },         false },
        { "7",       { 0, 4, 7, 10 },            false },
        { "9",       { 0, 4, 7, 10, 14 },        false },
        { "13",      { 0, 4, 7, 10, 14, 21 },    false },
        { "13sus4",  { 0, 5, 7, 10, 14, 21 },    false },
        { "7b9",     { 0, 4, 7, 10, 13 },        false },
        { "7alt",    { 0, 4, 10, 13, 18 },       false },
        { "dim7",    { 0, 3, 6, 9 },             true  },
        { "m7b5",    { 0, 3, 6, 10 },            true  },
        { "sus2",    { 0, 2, 7, 14 },            true  },
        { "5(add9,11)", { 0, 7, 14, 17 },        true  },
        { "4ths",    { 0, 5, 10, 15 },           true  },
        { "m(maj7)", { 0, 3, 7, 11 },            false },
    };
    return t;
}

// Chord "kinds" are slots in a progression; the Vibe dial picks the variant
// (ordered cold -> warm).
enum class Kind { Min, Maj, Dom, Alt, Dim, HDim, Sus, Quartal, MMaj, NoThird };

inline const std::vector<int>& variants(Kind k)
{
    static const std::vector<int> mn { M11, M9, M7 };
    static const std::vector<int> mj { Maj7s11, Maj7, Maj9, Six9 };
    static const std::vector<int> dm { Dom13sus, Dom9, Dom13 };
    static const std::vector<int> al { Dom7alt, Dom7b9 };
    static const std::vector<int> di { Dim7 };
    static const std::vector<int> hd { HDim7 };
    static const std::vector<int> su { P5Add9, Sus2 };
    static const std::vector<int> qu { Quartal4 };
    static const std::vector<int> mm { MMaj7 };
    switch (k)
    {
        case Kind::Min:     return mn;
        case Kind::Maj:     return mj;
        case Kind::Dom:     return dm;
        case Kind::Alt:     return al;
        case Kind::Dim:     return di;
        case Kind::HDim:    return hd;
        case Kind::Sus:     return su;
        case Kind::Quartal: return qu;
        case Kind::MMaj:    return mm;
        case Kind::NoThird: return su;
    }
    return mn;
}

// ---------------------------------------------------------------- progression bank
struct ProgChord { int semis; Kind kind; int bass; };   // semis from tonic, bass = -1 -> root

constexpr uint32_t sN = 1, sR = 2, sD = 4, sJ = 8, sA = 16, sT = 32;
// style bits: N=Neo-Soul R=90s R&B D=Dark OVO J=Jazz A=Ambient T=Trapsoul

struct Prog
{
    const char* name;
    uint32_t mask;
    float warmth, tension, gravity;
    bool pedal;                       // bass stays on the tonic
    std::array<ProgChord, 4> c;
};

inline const std::vector<Prog>& progBank()
{
    using K = Kind;
    static const std::vector<Prog> b = {
        // ---- Neo-Soul
        { "Dorian vamp  i9 / IV13", sN | sT, .55f, .30f, .35f, false,
          {{ {0,K::Min,-1}, {5,K::Dom,-1}, {0,K::Min,-1}, {5,K::Dom,-1} }} },
        { "ii - V - I - vi (extended)", sN | sJ, .85f, .45f, .80f, false,
          {{ {2,K::Min,-1}, {7,K::Dom,-1}, {0,K::Maj,-1}, {9,K::Min,-1} }} },
        { "Chromatic planing i - bII - i - bVII", sN | sD | sT, .30f, .50f, .40f, false,
          {{ {0,K::Min,-1}, {1,K::Maj,-1}, {0,K::Min,-1}, {10,K::Maj,-1} }} },
        { "Backdoor  I - iv - bVII13 - iii", sN | sR, .75f, .40f, .60f, false,
          {{ {0,K::Maj,-1}, {5,K::Min,-1}, {10,K::Dom,-1}, {4,K::Min,-1} }} },
        { "Andalusian  i - bVII - bVI - V7alt", sN | sT | sR, .30f, .65f, .80f, false,
          {{ {0,K::Min,-1}, {10,K::Maj,-1}, {8,K::Maj,-1}, {7,K::Alt,-1} }} },

        // ---- 90s R&B
        { "Turnaround  I - #idim7 - ii - V13sus", sR | sJ, .80f, .50f, .85f, false,
          {{ {0,K::Maj,-1}, {1,K::Dim,-1}, {2,K::Min,-1}, {7,K::Dom,-1} }} },
        { "Ballad  IV - iii - ii - V13sus", sR | sN, .80f, .35f, .75f, false,
          {{ {5,K::Maj,-1}, {4,K::Min,-1}, {2,K::Min,-1}, {7,K::Dom,-1} }} },
        { "Borrowed iv  I - IV - iv - I/3", sR, .80f, .35f, .80f, false,
          {{ {0,K::Maj,-1}, {5,K::Maj,-1}, {5,K::Min,-1}, {0,K::Maj,4} }} },
        { "Tritone sub  vi - ii - bII13 - I", sR | sJ | sN, .75f, .55f, .90f, false,
          {{ {9,K::Min,-1}, {2,K::Min,-1}, {1,K::Dom,-1}, {0,K::Maj,-1} }} },
        { "Minor ballad  i - bVI - bIII - bVII13", sR | sT, .40f, .40f, .70f, false,
          {{ {0,K::Min,-1}, {8,K::Maj,-1}, {3,K::Maj,-1}, {10,K::Dom,-1} }} },

        // ---- Dark OVO
        { "Tonic pedal  i - bVI - bIII - bVIIsus", sD | sT, .20f, .35f, .60f, true,
          {{ {0,K::Min,0}, {8,K::Maj,0}, {3,K::Maj,0}, {10,K::Sus,0} }} },
        { "Phrygian  i - bII - i - bIII", sD | sT, .20f, .50f, .50f, false,
          {{ {0,K::Min,-1}, {1,K::Maj,-1}, {0,K::Min,-1}, {3,K::Maj,-1} }} },
        { "Floating  i - bIII - i - bVI", sD | sT | sA, .20f, .30f, .15f, false,
          {{ {0,K::Min,-1}, {3,K::Maj,-1}, {0,K::Min,-1}, {8,K::Maj,-1} }} },
        { "No-third  power / sus stack", sD | sA | sT, .15f, .20f, .30f, false,
          {{ {0,K::NoThird,-1}, {8,K::NoThird,-1}, {3,K::Sus,-1}, {10,K::Sus,-1} }} },
        { "Chromatic mediant  i - bVI - III - bVII", sD | sT, .30f, .60f, .55f, false,
          {{ {0,K::Min,-1}, {8,K::Maj,-1}, {4,K::Maj,-1}, {10,K::Maj,-1} }} },
        { "Minor plagal  i - bVII - iv - i/5", sD | sT | sN, .25f, .35f, .45f, false,
          {{ {0,K::Min,-1}, {10,K::Maj,-1}, {5,K::Min,-1}, {0,K::Min,7} }} },

        // ---- Jazz
        { "vi - ii - V - I", sJ | sR, .80f, .35f, .95f, false,
          {{ {9,K::Min,-1}, {2,K::Min,-1}, {7,K::Dom,-1}, {0,K::Maj,-1} }} },
        { "ii-V-I - VI7alt", sJ, .75f, .60f, .85f, false,
          {{ {2,K::Min,-1}, {7,K::Dom,-1}, {0,K::Maj,-1}, {9,K::Alt,-1} }} },
        { "Minor ii-V  iim7b5 - V7alt - i - bVI", sJ | sN, .35f, .70f, .85f, false,
          {{ {2,K::HDim,-1}, {7,K::Alt,-1}, {0,K::Min,-1}, {8,K::Maj,-1} }} },
        { "Chromatic bass  I - bIII7 - ii - bII7", sJ | sN, .70f, .65f, .75f, false,
          {{ {0,K::Maj,-1}, {3,K::Dom,-1}, {2,K::Min,-1}, {1,K::Dom,-1} }} },
        { "Major-thirds axis  I - bVI - III - V7", sJ, .85f, .60f, .50f, false,
          {{ {0,K::Maj,-1}, {8,K::Maj,-1}, {4,K::Maj,-1}, {7,K::Dom,-1} }} },

        // ---- Ambient
        { "Drift  I - IV#11 - I - Vsus", sA | sN, .70f, .15f, .40f, false,
          {{ {0,K::Maj,-1}, {5,K::Maj,-1}, {0,K::Maj,-1}, {7,K::Sus,-1} }} },
        { "Dorian haze  i - IV - bIII - Vsus", sA | sN, .50f, .20f, .35f, false,
          {{ {0,K::Min,-1}, {5,K::Maj,-1}, {3,K::Maj,-1}, {7,K::Sus,-1} }} },
        { "Lydian drift  I - II - I - V", sA, .80f, .20f, .35f, false,
          {{ {0,K::Maj,-1}, {2,K::Maj,-1}, {0,K::Maj,-1}, {7,K::Maj,-1} }} },
        { "Quartal clouds", sA | sD, .30f, .15f, .20f, false,
          {{ {0,K::Quartal,-1}, {5,K::Quartal,-1}, {3,K::Quartal,-1}, {10,K::Quartal,-1} }} },
        { "Night shift  i - bVI - i - bVIIsus", sA | sD | sT, .35f, .25f, .30f, false,
          {{ {0,K::Min,-1}, {8,K::Maj,-1}, {0,K::Min,-1}, {10,K::Sus,-1} }} },
    };
    return b;
}

// ---------------------------------------------------------------- styles
struct StyleDef
{
    const char* name;
    double swing;       // 0.5 = straight, 0.62 = heavy 8th swing (position of the off-beat)
    double strumMs;     // delay between voices
    double lazyMs;      // how far behind the beat non-downbeat hits sit
    double bassLateMs;  // >0: bass lands after the upper voices
    int    vel;         // centre velocity
    int    velSpread;
    std::array<double, 5> patW;   // weights: Pad, PadAnt, Lazy, Backbeat, Stab
};

inline const std::array<StyleDef, 6>& styleTable()
{
    static const std::array<StyleDef, 6> s = {{
        { "Neo-Soul", .58, 14, 18,  0, 62, 9, { .5, .9, 1.0, .7, .1 } },
        { "90s R&B",  .56, 18, 12,  0, 66, 8, { .4, 1.0, .7, .9, .0 } },
        { "Dark OVO", .50,  3,  6, 10, 56, 4, { .9, .6, .2, .0, 1.0 } },
        { "Jazz",     .62, 10,  8,  0, 70, 10, { .2, .7, .8, 1.0, .0 } },
        { "Ambient",  .50, 28,  0,  6, 46, 5, { 1.0, .4, .5, .0, .0 } },
        { "Trapsoul", .50,  6, 10,  8, 60, 6, { .5, .5, .6, .2, 1.0 } },
    }};
    return s;
}

// ---------------------------------------------------------------- comping patterns (one bar, 4/4)
struct Hit { double beat; double len; double vel; bool ghost; bool anticip; };

enum PatId { Pad = 0, PadAnt, Lazy, Backbeat, Stab };

inline const std::vector<std::vector<Hit>>& patterns()
{
    static const std::vector<std::vector<Hit>> p = {
        /* Pad      */ { {0.0, 3.9, 1.00, false, false} },
        /* PadAnt   */ { {0.0, 3.0, 1.00, false, false}, {3.5, 0.5, 0.90, false, true} },
        /* Lazy     */ { {0.0, 2.0, 1.00, false, false}, {2.75, 1.0, 0.60, true, false} },
        /* Backbeat */ { {0.0, 1.5, 1.00, false, false}, {1.5, 0.5, 0.60, true, false}, {2.5, 1.25, 0.85, false, false} },
        /* Stab     */ { {0.0, 1.5, 1.00, false, false}, {2.5, 1.25, 0.80, false, false} },
    };
    return p;
}
} // namespace vs
