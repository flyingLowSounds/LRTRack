#include "DSPMath.hpp"

using namespace lrt;

/**
 * @brief Get PLL increment depending on frequency
 * @param frq Frequency
 * @param sr Samplerate
 * @return  PLL increment
 */
float getPhaseIncrement(float frq, float sr) {
    return TWOPI * frq / sr;
}


/**
 * @brief Actual BLIT core computation
 * @param N Harmonics
 * @param phase Current phase value
 * @return
 */
float BLITcore(float N, float phase) {
    float a = wrapTWOPI((clipl(N - 1.f, 0.f) + 0.5f) * phase);
    float x = fastSin(a) * 1.f / fastSin(0.5f * phase);

    clipl(1, 2);

    return (x - 1.f) * 2.f;
}


/**
 * @brief BLIT generator based on current phase
 * @param N Harmonics
 * @param phase Current phase of PLL
 * @return
 */
float BLIT(float N, float phase) {
    if (phase == 0.f) return 1.f;
    else return BLITcore(N, phase);
}


/**
 * @brief Add value to integrator
 * @param x Input
 * @param Fn
 * @return
 */
float Integrator::add(float x, float Fn) {
    value = (x - value) * (d * Fn) + value;
    return value;
}


/**
 * @brief Filter function for DC block
 * @param x Input sample
 * @return Filtered sample
 */
double DCBlocker::filter(double x) {
    double y = x - xm1 + r * ym1;
    xm1 = x;
    ym1 = y;

    return y;
}


DCBlocker::DCBlocker(double r) : r(r) {}


/**
 * @brief Cheap class to hold two numbers
 */
struct Vec {
    float a, b;


    Vec(float a, float b) : a(a), b(b) {}


    float getA() const {
        return a;
    }


    void setA(float a) {
        Vec::a = a;
    }


    float getB() const {
        return b;
    }


    void setB(float b) {
        Vec::b = b;
    }
};

/**
 * @brief Shaper type 1 (Saturate)
 * @param a Amount from 0 - x
 * @param x Input sample
 * @return
 */
float shape1(float a, float x) {
    float k = 2 * a / (1 - a);
    float b = (1 + k) * (x * 0.5f) / (1 + k * fabsf(x * 0.5f));

    return b * 4;
}


/**
 * @brief Waveshaper as used in ReShaper. Input should be in the range -1..+1
 * @param a Shaping factor
 * @param x Input sample
 * @return
 */
float shape2(float a, float x) {
    return atanf(x * a);//x * (fabs(x) + a) / (x * x + (a - 1) * fabs(x) + 1);
}


/**
 * @brief Soft saturating with a clip of a. Works only with positive values so use 'b' as helper here.
 * @param x Input sample
 * @param a Saturating threshold
 * @return
 */
double saturate(double x, double a) {
    double b = 1;

    /* turn negative values positive and remind in b as coefficient */
    if (x < 0) {
        b = -1;
        x *= -1;
    }

    // nothing to do
    if (x <= a) return x * b;

    double d = (a + (x - a) / (1 + pow((x - a) / (1 - a), 2)));

    if (d > 1) {
        return (a + 1) / 2 * b;
    } else {
        return d * b;
    }
}


/**
 * @brief
 * @param input
 * @return
 */
double overdrive(double input) {
    const double x = input * 0.686306;
    const double a = 1 + exp(sqrt(fabs(x)) * -0.75);
    return (exp(x) - exp(-x * a)) / (exp(x) + exp(-x));
}