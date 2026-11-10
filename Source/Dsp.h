#pragma once
// Aether - motor DSP (solo libreria estandar, sin dependencias de JUCE)
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace aether
{
constexpr float kPi     = 3.14159265358979323846f;
constexpr float kTwoPi  = 6.28318530717958647692f;
constexpr float kHalfPi = 1.57079632679489661923f;

inline int nextPow2 (int v)
{
    int p = 1;
    while (p < v) p <<= 1;
    return p;
}

// Limitador suave: transparente hasta 0.9, luego curva tanh.
inline float softLimit (float x)
{
    const float a = std::fabs (x);
    if (a <= 0.9f) return x;
    const float o = 0.9f + 0.1f * std::tanh ((a - 0.9f) * 10.0f);
    return x < 0.0f ? -o : o;
}

//==============================================================================
struct Smooth
{
    float v = 0.0f, t = 0.0f, c = 1.0f;
    void setTime (float sr, float seconds) { c = 1.0f - std::exp (-1.0f / (std::max (1.0e-4f, seconds) * sr)); }
    void snap() { v = t; }
    inline float next() { v += (t - v) * c; return v; }
};

//==============================================================================
class DelayLine
{
public:
    void prepare (int maxDelaySamples)
    {
        size = nextPow2 (maxDelaySamples + 8);
        mask = size - 1;
        buf.assign ((size_t) size, 0.0f);
        w = 0;
    }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }

    inline void push (float x) { w = (w + 1) & mask; buf[(size_t) w] = x; }

    // d = 0 -> ultima muestra empujada
    inline float read (float d) const
    {
        d = std::max (0.0f, std::min (d, (float) (size - 4)));
        const int   di = (int) d;
        const float fr = d - (float) di;
        const float a  = buf[(size_t) ((w - di) & mask)];
        const float b  = buf[(size_t) ((w - di - 1) & mask)];
        return a + (b - a) * fr;
    }

    // Interpolacion cubica (Hermite): menos perdida de agudos en lazos de feedback largos.
    inline float readCubic (float d) const
    {
        d = std::max (1.0f, std::min (d, (float) (size - 4)));
        const int   di = (int) d;
        const float fr = d - (float) di;
        const float ym1 = buf[(size_t) ((w - di + 1) & mask)];
        const float y0  = buf[(size_t) ((w - di) & mask)];
        const float y1  = buf[(size_t) ((w - di - 1) & mask)];
        const float y2  = buf[(size_t) ((w - di - 2) & mask)];
        const float c1 = 0.5f * (y1 - ym1);
        const float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
        const float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
        return ((c3 * fr + c2) * fr + c1) * fr + y0;
    }

private:
    std::vector<float> buf;
    int size = 0, mask = 0, w = 0;
};

//==============================================================================
class Allpass
{
public:
    void prepare (int len, float gain)
    {
        n = std::max (1, len);
        g = gain;
        buf.assign ((size_t) n, 0.0f);
        idx = 0;
    }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); idx = 0; }

    inline float process (float x)
    {
        const float d = buf[(size_t) idx];
        const float v = x - g * d;
        buf[(size_t) idx] = v;
        if (++idx >= n) idx = 0;
        return g * v + d;
    }

private:
    std::vector<float> buf;
    int n = 1, idx = 0;
    float g = 0.6f;
};

//==============================================================================
// Filtro de estado variable (TPT) de 12 dB/oct, Butterworth.
class SVF
{
public:
    enum Type { Lowpass, Highpass };
    explicit SVF (Type t = Lowpass) : type (t) {}

    void setCutoff (float fc, float sr)
    {
        fc = std::clamp (fc, 8.0f, 0.46f * sr);
        g  = std::tan (kPi * fc / sr);
        a1 = 1.0f / (1.0f + k * g + g * g);
    }
    void reset() { s1 = s2 = 0.0f; }

    inline float process (float x)
    {
        const float hp = (x - (k + g) * s1 - s2) * a1;
        const float bp = g * hp + s1;
        const float lp = g * bp + s2;
        s1 = g * hp + bp;
        s2 = g * bp + lp;
        return type == Highpass ? hp : lp;
    }

private:
    Type  type;
    float g = 0.1f, a1 = 1.0f, k = 1.41421356f, s1 = 0.0f, s2 = 0.0f;
};

//==============================================================================
// Pitch shifter granular (dos cabezales con ventana seno^2). Mono.
class PitchShifter
{
public:
    void prepare (double sampleRate)
    {
        win = (float) (0.08 * sampleRate);
        buf.prepare ((int) win + 16);
        phase = 0.0f;
    }
    void clear() { buf.clear(); phase = 0.0f; }

    inline float process (float x, float ratio)
    {
        buf.push (x);
        phase += (1.0f - ratio) / win;
        phase -= std::floor (phase);

        float p2 = phase + 0.5f;
        if (p2 >= 1.0f) p2 -= 1.0f;

        const float s1 = std::sin (kPi * phase);
        const float s2 = std::sin (kPi * p2);
        const float g1 = s1 * s1;
        const float g2 = s2 * s2;

        return buf.read (phase * win) * g1 + buf.read (p2 * win) * g2;
    }

private:
    DelayLine buf;
    float win = 4000.0f, phase = 0.0f;
};

//==============================================================================
// Reverb FDN de 8 lineas con matriz Hadamard, lineas moduladas y damping.
class Tank
{
public:
    static constexpr int N = 8;

    void prepare (double sampleRate)
    {
        sr     = (float) sampleRate;
        maxLen = 0.36f * sr;
        for (auto& l : lines) l.prepare ((int) maxLen + 64);
        lenCoef      = 1.0f - std::exp (-1.0f / (0.25f * sr));
        firstControl = true;
        reset();
    }

    void reset()
    {
        for (auto& l : lines) l.clear();
        for (int i = 0; i < N; ++i)
        {
            lp[i] = 0.0f;
            ph[i] = 0.8f * (float) i;
        }
    }

    void setControl (float sizeScale, float rt60, bool frozen, float dampHz, float modDepth01, float modRateHz)
    {
        float gSum = 0.0f;
        for (int i = 0; i < N; ++i)
        {
            targetLen[i] = std::min (baseMs[i] * 0.001f * sr * sizeScale, maxLen);
            g[i]         = std::pow (10.0f, -3.0f * targetLen[i] / (std::max (0.05f, rt60) * sr));
            gSum += g[i] * g[i];
        }
        inNorm   = std::sqrt (std::max (0.0f, 1.0f - gSum / (float) N));
        dampCoef = frozen ? 1.0f : std::min (1.0f, 1.0f - std::exp (-kTwoPi * dampHz / sr));
        modDepthSamples = modDepth01 * 14.0f * sr / 48000.0f;
        modInc          = kTwoPi * modRateHz / sr;

        if (firstControl)
        {
            for (int i = 0; i < N; ++i) curLen[i] = targetLen[i];
            firstControl = false;
        }
    }

    inline void process (float inL, float inR, float shimIn, float& outL, float& outR)
    {
        float y[N];
        for (int i = 0; i < N; ++i)
        {
            curLen[i] += (targetLen[i] - curLen[i]) * lenCoef;
            const float lfo = std::sin (ph[i]);
            ph[i] += modInc * (1.0f + 0.137f * (float) i);
            if (ph[i] > kTwoPi) ph[i] -= kTwoPi;

            const float v = lines[(size_t) i].readCubic (curLen[i] + modDepthSamples * lfo - 1.0f);
            lp[i] += dampCoef * (v - lp[i]);
            y[i] = lp[i] * g[i];
        }

        outL = (y[0] - y[2] + y[4] - y[6]);
        outR = (y[1] - y[3] + y[5] - y[7]);

        // Transformada de Walsh-Hadamard (matriz de feedback ortogonal)
        for (int len = 1; len < N; len <<= 1)
            for (int i = 0; i < N; i += len << 1)
                for (int j = i; j < i + len; ++j)
                {
                    const float u = y[j], v = y[j + len];
                    y[j]       = u + v;
                    y[j + len] = u - v;
                }

        constexpr float k = 0.35355339f; // 1/sqrt(8)
        for (int i = 0; i < N; ++i)
        {
            const float in = ((i & 1) ? inR : inL) + shimIn * shimSign[i];
            lines[(size_t) i].push (y[i] * k + in * inNorm);
        }
    }

private:
    static constexpr float baseMs[N]   = { 31.3f, 37.7f, 43.9f, 51.7f, 59.3f, 67.9f, 77.3f, 89.1f };
    static constexpr float shimSign[N] = { 1.f, -1.f, 1.f, 1.f, -1.f, 1.f, -1.f, -1.f };

    std::array<DelayLine, N> lines;
    float lp[N] {}, ph[N] {}, g[N] {}, curLen[N] {}, targetLen[N] {};
    float sr = 48000.0f, maxLen = 17000.0f, lenCoef = 0.0001f;
    float inNorm = 1.0f, dampCoef = 0.5f, modDepthSamples = 0.0f, modInc = 0.0f;
    bool  firstControl = true;
};

//==============================================================================
struct Params
{
    float mix = 0.35f, size = 0.6f, decay = 6.0f, damp = 6000.0f, predelay = 20.0f;
    float shimmer = 0.0f;
    int   shimInterval = 0; // 0:+12  1:+7  2:-12  3:+19
    float modDepth = 0.3f, modRate = 0.4f, tape = 0.0f;
    float lowCut = 80.0f, highCut = 12000.0f, width = 1.2f, outDb = 0.0f;
    bool  freeze = false;
};

//==============================================================================
class Engine
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        tank.prepare (sampleRate);
        shifter.prepare (sampleRate);

        const int maxPre = (int) (0.26f * sr);
        pdL.prepare (maxPre);
        pdR.prepare (maxPre);
        tapeDL.prepare ((int) (0.03f * sr));
        tapeDR.prepare ((int) (0.03f * sr));

        const float s = sr / 48000.0f;
        static const int dl[4] = { 142, 107, 379, 277 };
        static const int dr[4] = { 149, 113, 389, 283 };
        for (int k = 0; k < 4; ++k)
        {
            apL[k].prepare ((int) ((float) dl[k] * s), 0.6f);
            apR[k].prepare ((int) ((float) dr[k] * s), 0.6f);
        }

        mixS.setTime (sr, 0.03f);   outS.setTime (sr, 0.03f);
        shimS.setTime (sr, 0.05f);  tapeS.setTime (sr, 0.05f);
        widS.setTime (sr, 0.03f);   fzS.setTime (sr, 0.05f);
        preS.setTime (sr, 0.12f);   decayS.setTime (sr, 0.08f);
        dampS.setTime (sr, 0.05f);  lowS.setTime (sr, 0.05f);
        highS.setTime (sr, 0.05f);  ratioS.setTime (sr, 0.05f);

        snapNext = true;
        reset();
    }

    void reset()
    {
        tank.reset();
        shifter.clear();
        pdL.clear(); pdR.clear(); tapeDL.clear(); tapeDR.clear();
        for (auto& a : apL) a.clear();
        for (auto& a : apR) a.clear();
        hpL.reset(); hpR.reset(); lpL.reset(); lpR.reset();
        shimFb = 0.0f; tapeLpL = tapeLpR = 0.0f; wowPh = flutPh = 0.0f;
        ctr = 0;
    }

    void setParams (const Params& p)
    {
        static const float semis[4] = { 12.0f, 7.0f, -12.0f, 19.0f };
        const int   idx = std::clamp (p.shimInterval, 0, 3);

        mixS.t   = p.mix;
        outS.t   = std::pow (10.0f, p.outDb / 20.0f);
        shimS.t  = p.shimmer;
        tapeS.t  = p.tape;
        widS.t   = p.width;
        fzS.t    = p.freeze ? 1.0f : 0.0f;
        preS.t   = p.predelay * 0.001f * sr;
        decayS.t = p.decay;
        dampS.t  = p.damp;
        lowS.t   = p.lowCut;
        highS.t  = p.highCut;
        ratioS.t = std::pow (2.0f, semis[idx] / 12.0f);

        sizeScale = 0.4f + 2.6f * std::clamp (p.size, 0.0f, 1.0f);
        modDepth  = p.modDepth;
        modRate   = p.modRate;

        if (snapNext)
        {
            for (auto* s : { &mixS, &outS, &shimS, &tapeS, &widS, &fzS, &preS, &decayS, &dampS, &lowS, &highS, &ratioS })
                s->snap();
            snapNext = false;
        }
    }

    void process (float* L, float* R, int n)
    {
        const float wowD = 0.0015f * sr, fluD = 0.00015f * sr, tapeBase = 0.003f * sr;
        const float wowInc = kTwoPi * 0.37f / sr, flutInc = kTwoPi * 5.9f / sr;

        for (int i = 0; i < n; ++i)
        {
            if (ctr == 0) updateControl();
            ctr = (ctr + 1) & 31;

            const float mix = mixS.next(), outG = outS.next(), shim = shimS.next();
            const float tape = tapeS.next(), wid = widS.next(), fz = fzS.next(), pre = preS.next();
            decayS.next(); dampS.next(); lowS.next(); highS.next(); ratioS.next();

            const float dL = L[i], dR = R[i];

            // pre-delay + difusion de entrada
            pdL.push (dL);
            pdR.push (dR);
            float xL = pdL.read (pre), xR = pdR.read (pre);
            for (int k = 0; k < 4; ++k) { xL = apL[k].process (xL); xR = apR[k].process (xR); }

            // tanque + shimmer
            const float inG = 1.0f - fz;
            float tL = 0.0f, tR = 0.0f;
            tank.process (xL * inG, xR * inG, shimFb, tL, tR);

            const float sh = shifter.process (0.5f * (tL + tR), ratioS.v);
            shimFb = std::tanh (sh) * shim * 0.5f * (1.0f - fz);

            // cinta: wow/flutter + saturacion + paso bajo
            wowPh += wowInc;   if (wowPh > kTwoPi)  wowPh  -= kTwoPi;
            flutPh += flutInc; if (flutPh > kTwoPi) flutPh -= kTwoPi;

            tapeDL.push (tL);
            tapeDR.push (tR);
            float wL = tapeDL.read (tapeBase + tape * (wowD * std::sin (wowPh) + fluD * std::sin (flutPh)));
            float wR = tapeDR.read (tapeBase + tape * (wowD * std::sin (wowPh + 1.3f) + fluD * std::sin (flutPh + 2.1f)));

            if (tape > 0.001f)
            {
                const float drive = 1.0f + 2.5f * tape;
                wL = std::tanh (wL * drive) / drive;
                wR = std::tanh (wR * drive) / drive;
            }
            tapeLpL += tapeLpA * (wL - tapeLpL); wL = tapeLpL;
            tapeLpR += tapeLpA * (wR - tapeLpR); wR = tapeLpR;

            // tono
            wL = lpL.process (hpL.process (wL));
            wR = lpR.process (hpR.process (wR));

            // ancho estereo (M/S)
            const float m = 0.5f * (wL + wR), s = 0.5f * (wL - wR) * wid;
            wL = m + s;
            wR = m - s;

            // mezcla (potencia constante)
            const float wetG = std::sin (mix * kHalfPi), dryG = std::cos (mix * kHalfPi);
            float oL = (dL * dryG + wL * wetG) * outG;
            float oR = (dR * dryG + wR * wetG) * outG;
            if (! std::isfinite (oL)) oL = 0.0f;
            if (! std::isfinite (oR)) oR = 0.0f;
            L[i] = softLimit (oL);
            R[i] = softLimit (oR);
        }
    }

private:
    void updateControl()
    {
        const bool frozen = fzS.v > 0.5f;
        tank.setControl (sizeScale, frozen ? 3000.0f : decayS.v, frozen, dampS.v, modDepth, modRate);
        hpL.setCutoff (lowS.v, sr);  hpR.setCutoff (lowS.v, sr);
        lpL.setCutoff (highS.v, sr); lpR.setCutoff (highS.v, sr);
        tapeLpA = std::min (1.0f, 1.0f - std::exp (-kTwoPi * (20000.0f - 14000.0f * tapeS.v) / sr));
    }

    float sr = 48000.0f;
    Tank tank;
    PitchShifter shifter;
    DelayLine pdL, pdR, tapeDL, tapeDR;
    Allpass apL[4], apR[4];
    SVF hpL { SVF::Highpass }, hpR { SVF::Highpass }, lpL { SVF::Lowpass }, lpR { SVF::Lowpass };
    Smooth mixS, outS, shimS, tapeS, widS, fzS, preS, decayS, dampS, lowS, highS, ratioS;

    float sizeScale = 1.0f, modDepth = 0.3f, modRate = 0.4f;
    float shimFb = 0.0f, tapeLpL = 0.0f, tapeLpR = 0.0f, tapeLpA = 0.9f;
    float wowPh = 0.0f, flutPh = 0.0f;
    int   ctr = 0;
    bool  snapNext = true;
};

} // namespace aether
