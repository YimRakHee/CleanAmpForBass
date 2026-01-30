#pragma once
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Biquad {
public:
    Biquad() { reset(); }

    void reset() {
        z1 = z2 = 0.0f;
    }

    // --- 필터 계수 설정 함수들 (기존과 동일) ---
    void setHighPass(float freq, float sr, float q = 0.707f) {
        float w0 = 2.0f * M_PI * freq / sr;
        float alpha = std::sin(w0) / (2.0f * q);
        float cosw0 = std::cos(w0);
        b0 = (1.0f + cosw0) / 2.0f; b1 = -(1.0f + cosw0); b2 = (1.0f + cosw0) / 2.0f;
        a0 = 1.0f + alpha; a1 = -2.0f * cosw0; a2 = 1.0f - alpha;
        normalize();
    }

    void setLowShelf(float freq, float sr, float gain_db, float q = 0.707f) {
        float A = std::pow(10.0f, gain_db / 40.0f);
        float w0 = 2.0f * M_PI * freq / sr;
        float alpha = std::sin(w0) / 2.0f * std::sqrt((A + 1.0f / A) * (1.0f / q - 1.0f) + 2.0f);
        float cosw0 = std::cos(w0);
        b0 = A * ((A+1.0f) - (A-1.0f)*cosw0 + 2.0f*std::sqrt(A)*alpha);
        b1 = 2.0f*A * ((A-1.0f) - (A+1.0f)*cosw0);
        b2 = A * ((A+1.0f) - (A-1.0f)*cosw0 - 2.0f*std::sqrt(A)*alpha);
        a0 = (A+1.0f) + (A-1.0f)*cosw0 + 2.0f*std::sqrt(A)*alpha;
        a1 = -2.0f * ((A-1.0f) + (A+1.0f)*cosw0);
        a2 = (A+1.0f) + (A-1.0f)*cosw0 - 2.0f*std::sqrt(A)*alpha;
        normalize();
    }

    void setPeak(float freq, float sr, float gain_db, float q = 0.707f) {
        float A = std::pow(10.0f, gain_db / 40.0f);
        float w0 = 2.0f * M_PI * freq / sr;
        float alpha = std::sin(w0) / (2.0f * q);
        float cosw0 = std::cos(w0);
        b0 = 1.0f + alpha * A; b1 = -2.0f * cosw0; b2 = 1.0f - alpha * A;
        a0 = 1.0f + alpha / A; a1 = -2.0f * cosw0; a2 = 1.0f - alpha / A;
        normalize();
    }

    void setHighShelf(float freq, float sr, float gain_db, float q = 0.707f) {
        float A = std::pow(10.0f, gain_db / 40.0f);
        float w0 = 2.0f * M_PI * freq / sr;
        float alpha = std::sin(w0) / 2.0f * std::sqrt((A + 1.0f / A) * (1.0f / q - 1.0f) + 2.0f);
        float cosw0 = std::cos(w0);
        b0 = A * ((A+1.0f) + (A-1.0f)*cosw0 + 2.0f*std::sqrt(A)*alpha);
        b1 = -2.0f*A * ((A-1.0f) + (A+1.0f)*cosw0);
        b2 = A * ((A+1.0f) + (A-1.0f)*cosw0 - 2.0f*std::sqrt(A)*alpha);
        a0 = (A+1.0f) - (A-1.0f)*cosw0 + 2.0f*std::sqrt(A)*alpha;
        a1 = 2.0f * ((A-1.0f) - (A+1.0f)*cosw0);
        a2 = (A+1.0f) - (A-1.0f)*cosw0 - 2.0f*std::sqrt(A)*alpha;
        normalize();
    }

    void setLowPass(float freq, float sr, float q = 0.707f) {
        float w0 = 2.0f * M_PI * freq / sr;
        float alpha = std::sin(w0) / (2.0f * q);
        float cosw0 = std::cos(w0);
        b0 = (1.0f - cosw0) / 2.0f; b1 = 1.0f - cosw0; b2 = (1.0f - cosw0) / 2.0f;
        a0 = 1.0f + alpha; a1 = -2.0f * cosw0; a2 = 1.0f - alpha;
        normalize();
    }

    inline float process(float in) {
        // Denormal prevention: 0에 무한히 가까운 값을 방지
        static const float antidenormal = 1e-18f;
        float out = b0 * in + z1 + antidenormal;
        z1 = b1 * in - a1 * out + z2;
        z2 = b2 * in - a2 * out;
        return out;
    }

private:
    float b0, b1, b2, a0, a1, a2;
    float z1, z2;
    void normalize() {
        b0 /= a0; b1 /= a0; b2 /= a0;
        a1 /= a0; a2 /= a0;
    }
};
