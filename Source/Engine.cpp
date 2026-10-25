#include "Engine.h"
#include "Theory.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace vs
{
namespace
{
const char* kNote[12] = { "C", "Db", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };

using Voicing = std::vector<int>;   // ascending pitches of the upper structure (bass is separate)

struct RealChord { int rootPc = 0; int q = 0; int bassPc = 0; };

std::string chordName (const RealChord& c)
{
    std::string s = kNote[c.rootPc];
    s += qualityTable()[(size_t) c.q].suffix;
    if (c.bassPc != c.rootPc) { s += "/"; s += kNote[c.bassPc]; }
    return s;
}

int bassMidi (int pc) { return 36 + (((pc - 36) % 12) + 12) % 12; }   // E1..D#2 register -> 36..47

// Distance between two voicings: how much the voices move, reward common tones,
// punish big leaps in the top voice (the "melody" of the progression).
double dist (const Voicing& a, const Voicing& b)
{
    double s = 0.0;
    int common = 0;
    for (int x : b) { int m = 99; for (int y : a) m = std::min (m, std::abs (x - y)); s += m; if (m == 0) ++common; }
    for (int y : a) { int m = 99; for (int x : b) m = std::min (m, std::abs (x - y)); s += m; }
    s *= 0.5;
    s -= 1.5 * common;
    const int leap = std::abs (a.back() - b.back());
    if (leap > 4) s += 2.0 * (leap - 4);
    return s;
}

double baseCost (const Voicing& v, double space)
{
    const double lo = v.front(), hi = v.back();
    const double span = hi - lo, target = 9.0 + 14.0 * space;
    double c = 0.25 * std::abs (span - target);
    double mean = 0.0;
    for (int p : v) mean += p;
    mean /= (double) v.size();
    c += 0.15 * std::abs (mean - (64.0 + 4.0 * space));
    if (hi > 79) c += 2.0 * (hi - 79);
    if (lo < 52) c += 2.0 * (52 - lo);
    return c;
}

std::vector<Voicing> candidates (int rootPc, int qid, double space)
{
    const QDef& q = qualityTable()[(size_t) qid];
    std::vector<int> tones;
    for (size_t i = 1; i < q.iv.size(); ++i) tones.push_back (q.iv[i]);
    if (! q.keepFifth && tones.size() >= 4)
        tones.erase (std::remove (tones.begin(), tones.end(), 7), tones.end());

    std::vector<Voicing> out;
    const int anchors[] = { 53, 58, 63 };
    const size_t n = tones.size();

    for (size_t r = 0; r < n; ++r)                 // inversions
        for (int anchor : anchors)                 // registers
        {
            Voicing v;
            int prev = -1;
            for (size_t j = 0; j < n; ++j)
            {
                const int pc = (rootPc + tones[(r + j) % n]) % 12;
                int p = (j == 0) ? anchor : prev + 1;
                while (p % 12 != pc) ++p;
                v.push_back (p);
                prev = p;
            }
            if (v.back() > 88 || v.front() < 45) continue;
            out.push_back (v);

            if (space > 0.45 && v.size() >= 4)     // drop-2 voicing for open sound
            {
                Voicing d = v;
                d[d.size() - 2] -= 12;
                std::sort (d.begin(), d.end());
                if (d.front() >= 45) out.push_back (d);
            }
        }
    return out;
}

// Viterbi over the 4 chords, with loop closure (last -> first) so the loop has no seam.
std::array<Voicing, 4> solve (const std::array<RealChord, 4>& ch, double space, std::mt19937& rng)
{
    std::array<std::vector<Voicing>, 4> cand;
    std::array<std::vector<double>, 4> base;
    std::uniform_real_distribution<double> u (0.0, 0.8);

    for (size_t i = 0; i < 4; ++i)
    {
        cand[i] = candidates (ch[i].rootPc, ch[i].q, space);
        for (auto& v : cand[i]) base[i].push_back (baseCost (v, space) + u (rng));
    }

    double best = 1e18;
    std::array<size_t, 4> bestIdx { 0, 0, 0, 0 };

    for (size_t s = 0; s < cand[0].size(); ++s)
    {
        std::array<std::vector<double>, 4> dp;
        std::array<std::vector<int>, 4> bp;
        dp[0].assign (cand[0].size(), 1e18);
        dp[0][s] = base[0][s];

        for (size_t i = 1; i < 4; ++i)
        {
            dp[i].assign (cand[i].size(), 1e18);
            bp[i].assign (cand[i].size(), -1);
            for (size_t j = 0; j < cand[i].size(); ++j)
                for (size_t k = 0; k < cand[i - 1].size(); ++k)
                    if (dp[i - 1][k] < 1e17)
                    {
                        const double c = dp[i - 1][k] + base[i][j] + dist (cand[i - 1][k], cand[i][j]);
                        if (c < dp[i][j]) { dp[i][j] = c; bp[i][j] = (int) k; }
                    }
        }
        for (size_t j = 0; j < cand[3].size(); ++j)
        {
            if (dp[3][j] > 1e17) continue;
            const double total = dp[3][j] + dist (cand[3][j], cand[0][s]);
            if (total < best)
            {
                best = total;
                bestIdx[3] = j;
                bestIdx[2] = (size_t) bp[3][j];
                bestIdx[1] = (size_t) bp[2][bestIdx[2]];
                bestIdx[0] = s;
            }
        }
    }
    std::array<Voicing, 4> out;
    for (size_t i = 0; i < 4; ++i) out[i] = cand[i][bestIdx[i]];
    return out;
}
} // namespace

const std::vector<std::string>& styleNames()
{
    static const std::vector<std::string> n = [] {
        std::vector<std::string> v;
        for (auto& s : styleTable()) v.emplace_back (s.name);
        return v;
    }();
    return n;
}

Loop generate (const GenParams& gp, uint32_t seed)
{
    std::mt19937 rng (seed);
    std::uniform_real_distribution<double> u01 (0.0, 1.0);
    auto gauss = [&] (double sd) { std::normal_distribution<double> d (0.0, sd); return d (rng); };

    const int styleIdx = std::clamp (gp.style, 0, (int) styleTable().size() - 1);
    const StyleDef& st = styleTable()[(size_t) styleIdx];
    const auto& bank = progBank();
    const uint32_t bit = 1u << styleIdx;

    // ---- 1. choose a progression: weighted by how well its warmth matches the Vibe dial
    std::vector<double> w (bank.size(), 0.0);
    for (size_t i = 0; i < bank.size(); ++i)
        if (bank[i].mask & bit)
        {
            const double d = gp.vibe - (double) bank[i].warmth;
            w[i] = std::exp (-d * d / 0.09) + 0.12;
            if ((int) i == gp.avoidProg) w[i] *= 0.1;
        }
    std::discrete_distribution<int> pick (w.begin(), w.end());
    const int progIdx = pick (rng);
    const Prog& prog = bank[(size_t) progIdx];
    const int tonic = gp.keyPc >= 0 ? gp.keyPc % 12 : (int) (u01 (rng) * 12.0) % 12;

    // ---- 2. chord colour: Vibe picks the variant (cold -> warm), with a little noise
    std::array<int, 4> baseVar {};
    for (size_t c = 0; c < 4; ++c)
    {
        const auto& var = variants (prog.c[c].kind);
        const double x = gp.vibe * (double) (var.size() - 1) + gauss (0.55);
        baseVar[c] = std::clamp ((int) std::lround (x), 0, (int) var.size() - 1);
    }

    // ---- 3. rhythm DNA: weights react to tempo (slow -> sustained, fast -> busier)
    auto pickPattern = [&] () -> int
    {
        std::array<double, 5> pw = st.patW;
        if (gp.bpm < 70.0)       { pw[3] *= 0.4; pw[4] *= 0.4; pw[0] *= 1.3; }
        else if (gp.bpm > 88.0)  { pw[3] *= 1.8; pw[4] *= 1.8; pw[0] *= 0.5; }
        std::discrete_distribution<int> d (pw.begin(), pw.end());
        return d (rng);
    };
    std::array<int, 4> dna {};
    for (auto& d : dna) d = pickPattern();

    Loop loop;
    loop.keyName   = kNote[tonic];
    loop.progName  = prog.name;
    loop.progIndex = progIdx;

    const auto& pt = patterns();
    auto hasAnt = [&] (int pi) { for (auto& h : pt[(size_t) pi]) if (h.anticip) return true; return false; };

    for (int pass = 0; pass < 4; ++pass)
    {
        // ---- 4. evolution: each pass nudges 1-2 chords one colour step and re-rolls a bar's rhythm
        auto vp = baseVar;
        auto pats = dna;
        if (pass > 0)
        {
            const int changes = (pass == 2) ? 2 : 1;
            for (int m = 0; m < changes; ++m)
            {
                const size_t c = (size_t) ((int) (u01 (rng) * 4.0) % 4);
                const auto& var = variants (prog.c[c].kind);
                if (var.size() > 1) vp[c] += (vp[c] < (int) var.size() - 1) ? 1 : -1;
                pats[(size_t) ((int) (u01 (rng) * 4.0) % 4)] = pickPattern();
            }
        }

        std::array<RealChord, 4> ch;
        for (size_t c = 0; c < 4; ++c)
        {
            const ProgChord& pc = prog.c[c];
            RealChord r;
            r.rootPc = (tonic + pc.semis) % 12;
            r.q      = variants (pc.kind)[(size_t) vp[c]];
            r.bassPc = prog.pedal ? tonic : (pc.bass >= 0 ? (tonic + pc.bass) % 12 : r.rootPc);
            ch[c] = r;
            if (pass == 0) loop.chordNames[c] = chordName (r);
        }

        // ---- 5. voice leading (seam-free loop)
        const auto V = solve (ch, gp.space, rng);

        // chromatic approach chord (dominant a semitone above chord 1) in pass 3 - classic R&B turnaround
        const bool approach = (pass == 2) && (styleIdx == 0 || styleIdx == 1 || styleIdx == 3 || styleIdx == 5)
                              && u01 (rng) < 0.7;

        std::vector<Note>& out = loop.passes[(size_t) pass];

        auto addChord = [&] (const Voicing& up, int bass, bool withBass, double startBeat,
                             double len, double velScale, double offMs)
        {
            const double hu = gp.human;
            const int top = (int) up.size() - 1;
            for (int j = 0; j <= top; ++j)
            {
                Note n;
                n.pitch = up[(size_t) j];
                n.start = startBeat;
                n.len   = len;
                const double strum = st.bassLateMs > 0.0 ? j * st.strumMs : (j + 1) * st.strumMs;
                n.offMs = offMs + strum * hu * (0.7 + 0.6 * u01 (rng)) + gauss (2.5 * hu + 0.01);
                const double v = st.vel * velScale + gauss (st.velSpread * hu + 0.01) + (j == top ? 5.0 : -3.0);
                n.vel = std::clamp ((int) std::lround (v), 1, 127);
                out.push_back (n);
            }
            if (withBass)
            {
                Note n;
                n.pitch = bass;
                n.start = startBeat;
                n.len   = len;
                n.offMs = offMs + st.bassLateMs * hu + gauss (2.0 * hu + 0.01);
                n.vel   = std::clamp ((int) std::lround (st.vel * velScale + 9.0 + gauss (st.velSpread * hu + 0.01)), 1, 127);
                out.push_back (n);
            }
        };

        // ---- 6. comping: place chords on the grid with swing, lazy backbeat, anticipations, ghost re-voicings
        for (int b = 0; b < 4; ++b)
        {
            const auto& hits = pt[(size_t) pats[(size_t) b]];
            const int prevBar = (b + 3) % 4;
            const bool prevAnt = hasAnt (pats[(size_t) prevBar]) && ! (approach && prevBar == 3);

            for (size_t hi = 0; hi < hits.size(); ++hi)
            {
                Hit h = hits[hi];
                if (approach && b == 3)
                {
                    if (h.beat >= 2.5) continue;
                    h.len = std::min (h.len, 2.5 - h.beat - 0.05);
                }
                if (hi == 0 && prevAnt) continue;   // previous bar already played this chord early

                const size_t ci = (size_t) (h.anticip ? (b + 1) % 4 : b);
                Voicing up = V[ci];
                if (h.ghost) { up.front() += 12; std::sort (up.begin(), up.end()); }   // re-voiced, never a duplicate

                double len = h.len;
                if (h.anticip)
                {
                    const auto& nx = pt[(size_t) pats[(size_t) ((b + 1) % 4)]];
                    len = (4.0 - h.beat) + (nx.size() > 1 ? nx[1].beat : 3.9) - 0.05;
                }

                double start = b * 4.0 + h.beat;
                if (std::fabs ((h.beat - std::floor (h.beat)) - 0.5) < 1e-6) start += st.swing - 0.5;

                const double off = h.anticip ? -20.0 * gp.human
                                             : (h.beat > 0.01 ? st.lazyMs * gp.human : 0.0);

                addChord (up, bassMidi (ch[ci].bassPc), ! h.ghost, start, len - 0.03, h.vel, off);
            }
        }

        if (approach)
        {
            const int rootPc = (ch[0].rootPc + 1) % 12;
            double best = 1e18;
            Voicing bv;
            for (auto& c : candidates (rootPc, Dom9, gp.space))
            {
                const double cost = dist (V[3], c) + 0.7 * dist (c, V[0]) + baseCost (c, gp.space);
                if (cost < best) { best = cost; bv = c; }
            }
            if (! bv.empty())
                addChord (bv, bassMidi (rootPc), true, 3 * 4.0 + 2.5 + (st.swing - 0.5), 1.40, 0.85,
                          st.lazyMs * gp.human);
        }
    }
    return loop;
}
} // namespace vs
