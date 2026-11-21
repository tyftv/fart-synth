#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace fart
{
struct VoiceParams
{
    double pressure = 0.8, attack = 15, decay = 300, sustain = 0.6, release = 250, length = 1.2;
    double pitch = 0, tight = 0.4, flutter = 0.3, slide = 0.5, cavity = 275, bright = 2400;
    double wet = 0.3, sputter = 0.2, rumble = 0.2, gas = 0.2, body = 0.9, drive = 0.2, volume = 0.7;
};

// Physically-inspired fart voice: a self-oscillating mass-spring sphincter valve driven by
// pressure, coupled to a resonant tract and buffeted by turbulence noise.
class Voice
{
public:
    std::atomic<float> uiEnv { 0.f }, uiFlap { 0.f }, uiWetGate { 1.f };

    void prepare (double sampleRate) { sr = sampleRate; fullReset(); }

    void on (int n, bool retrig)
    {
        note = n;
        if (retrig || stage == 0 || stage == 4)
        {
            if (stage == 0) reset();
            stage = 1;
        }
    }
    void off() { if (stage != 0) stage = 4; }
    void setNote (int n) { note = n; }
    bool isActive() const { return stage != 0; }

    void render (float* out, int n, const VoiceParams& P)
    {
        const double dt = 1.0 / sr;
        const double att = 1.0 / (std::max (P.attack, 1.0) * 0.001 * sr);
        const double dec = (1.0 - P.sustain) / (std::max (P.decay, 1.0) * 0.001 * sr);
        const double relC = std::exp (-4.6 / (std::max (P.release, 5.0) * 0.001 * sr));
        auto lpc = [this] (double hz) { return 1.0 - std::exp (-6.2832 * hz / sr); };
        const double wetA = lpc (12 + P.wet * 90), wetNorm = 1.0 / std::sqrt (wetA / 6);
        const double pnA = lpc (28), pnNorm = 1.0 / std::sqrt (pnA / 6);
        const double flA = lpc (14), flNorm = 1.0 / std::sqrt (flA / 6);
        const double drA = lpc (3), drNorm = 1.0 / std::sqrt (drA / 6);
        const double tnA = lpc (2500);
        const double gasA = lpc (std::min (P.bright * 1.3, 9000.0));
        const double dGain = 1 + P.drive * 7;
        const double f0 = 26 * std::pow (2.0, (P.pitch + note) / 12.0) * (1 + P.tight * 0.6) * (0.85 + 0.3 * P.pressure);
        const double x0 = -0.12 - P.tight * 0.3;
        const double Q = 1.3 + P.tight * 2.5;
        const double maxFc = sr / 6.5;
        const double fcb = P.cavity;

        for (int i = 0; i < n; ++i)
        {
            if (stage == 0) { out[i] = 0.f; flap *= 0.999; continue; }
            if (stage == 1) { env += att; if (env >= 1) { env = 1; stage = 2; } }
            else if (stage == 2) { env -= dec; if (env <= P.sustain) { env = P.sustain; stage = 3; } }
            else if (stage == 3) { env += (P.sustain - env) * 0.002; }
            else if (stage == 4)
            {
                env *= relC;
                if (env < 0.0008) { env = 0; stage = 0; out[i] = 0.f; continue; }
            }
            const double e = env;
            const double nz = rnd();
            rumPh2 += 6.2832 * 0.37 / sr;
            rumPh += (4 + 5 * (0.5 + 0.5 * std::sin (rumPh2))) / sr;
            if (rumPh > 1) rumPh -= 1;
            const double rum = std::sin (6.2832 * rumPh);
            wetN += (nz - wetN) * wetA;
            const double wn = wetN * wetNorm;
            const double wg = ss (-0.4, 0.5, wn);
            wetGate = wg;
            pn += (rnd() - pn) * pnA;  const double pnN = pn * pnNorm;
            fn += (rnd() - fn) * flA;  const double flN = fn * flNorm;
            dr += (rnd() - dr) * drA;  const double drN = dr * drNorm;

            double pm = P.pressure * (0.2 + 0.8 * e) * (1 + 0.12 * pnN + P.sputter * 0.9 * pnN)
                        * (1 - P.rumble * 0.45 * (0.5 + 0.5 * rum)) * (1 - P.wet * 0.8 * (1 - wg));
            if (pm < 0) pm = 0;

            double w = 6.2832 * f0 * (1 + P.slide * (e - 0.5) * 1.4) * (1 + (0.04 + P.flutter * 0.3) * flN * 0.5 + 0.05 * drN)
                       * (1 + P.rumble * 0.1 * rum) * (1 + P.wet * 0.25 * wn);
            if (w < 94) w = 94; else if (w > 3770) w = 3770;

            const double dp = pm - pt;
            const double open1 = 1 - x / 0.9, pf = dp * (open1 > 0 ? open1 : 0);
            const double acc = -(w / Q) * v - w * w * (x - x0) + w * w * 1.5 * pf * (1 + ASYM * std::tanh (v / (0.25 * w)));
            v += acc * dt; x += v * dt;
            if (x < 0) { x = 0; if (v < 0) v *= -0.25; }
            else if (x > 1.4) { x = 1.4; if (v > 0) v = 0; }
            const double U = dp > 0 ? x * std::sqrt (dp) : 0.0;

            const double ff = 2 * std::sin (3.14159 * std::min (fcb, maxFc) / sr);
            fl1 += ff * fb1; const double fh = U - fl1 - 0.6 * fb1; fb1 += ff * fh;
            pt = 0.35 * fb1;

            tn += (rnd() - tn) * tnA;
            gl += (rnd() - gl) * gasA;
            const double ud = (U - Up) * sr * 0.0004; Up = U;
            const double uh = U - dcU; dcU += uh * 0.002;
            const double src = ((ud * 0.5 + uh * MIXL) * P.body) + tn * U * (0.9 + 1.4 * P.wet) + gl * P.gas * (0.35 + 0.65 * e) * 2.2;

            const double fc1 = std::min (P.cavity * (0.9 + 0.2 * e), maxFc);
            const double fc2 = std::min (fc1 * 3, maxFc), fc3 = std::min (fc1 * 5, maxFc);
            const double f1 = 2 * std::sin (3.14159 * fc1 / sr), f2 = 2 * std::sin (3.14159 * fc2 / sr), f3 = 2 * std::sin (3.14159 * fc3 / sr);
            lp1 += f1 * bp1; const double h1 = src - lp1 - 0.6 * bp1; bp1 += f1 * h1;
            lp2 += f2 * bp2; const double h2 = src - lp2 - 0.5 * bp2; bp2 += f2 * h2;
            lp3 += f3 * bp3; const double h3 = src - lp3 - 0.5 * bp3; bp3 += f3 * h3;
            double m = src * 0.8 + bp1 * 0.7 + bp2 * 0.3 + bp3 * 0.15;
            const double brA = lpc (P.bright * (0.2 + 0.8 * e));
            lpb += brA * (m - lpb);
            m = std::tanh (lpb * dGain) * 0.8;
            const double o = m * std::pow (e, 0.7) * P.volume * (0.4 + 0.6 * P.pressure) * 2.0;
            out[i] = (float) o;
            flap += (std::fabs (o) - flap) * 0.002;
        }
        uiEnv.store ((float) env);
        uiFlap.store ((float) flap);
        uiWetGate.store ((float) wetGate);
    }

private:
    static constexpr double ASYM = 0.9, MIXL = 0.5;
    static double ss (double a, double b, double xx) { xx = (xx - a) / (b - a); xx = xx < 0 ? 0 : xx > 1 ? 1 : xx; return xx * xx * (3 - 2 * xx); }
    double rnd() { rng = rng * 1664525u + 1013904223u; return (double) (rng >> 8) / 8388608.0 - 1.0; }

    void fullReset()
    {
        reset(); stage = 0; env = 0; note = 0; flap = 0; wetGate = 1; rumPh = 0; rumPh2 = 0;
    }
    void reset()
    {
        x = v = pt = wetN = pn = fn = dr = tn = gl = 0;
        fl1 = fb1 = lp1 = bp1 = lp2 = bp2 = lp3 = bp3 = lpb = Up = dcU = 0;
    }

    double sr = 44100;
    int stage = 0, note = 0;
    double env = 0, flap = 0, wetGate = 1, rumPh = 0, rumPh2 = 0;
    double x = 0, v = 0, pt = 0, wetN = 0, pn = 0, fn = 0, dr = 0, tn = 0, gl = 0;
    double fl1 = 0, fb1 = 0, lp1 = 0, bp1 = 0, lp2 = 0, bp2 = 0, lp3 = 0, bp3 = 0, lpb = 0, Up = 0, dcU = 0;
    uint32_t rng = 12345u;
};
} // namespace fart
