#include <lv2/core/lv2.h>
#include <cmath>
#include <algorithm>
#include "Biquad.hpp"

enum {
    PORT_IN          = 0,
    PORT_OUT         = 1,
    PORT_HPF_FREQ    = 2,
    PORT_BASS_GAIN   = 3,
    PORT_MID_FREQ    = 4,
    PORT_MID_GAIN    = 5,
    PORT_TREBLE_GAIN = 6,
    PORT_LPF_FREQ    = 7,
    PORT_MASTER_GAIN = 8
};

class CleanAmpForBass {
public:
    const float* ports[9];
    float sample_rate;

    Biquad hpf, bass_shelf, mid_peak, treble_shelf, lpf;

    float last_hpf, last_bass, last_mid_f, last_mid_g, last_treble, last_lpf, last_master;
    float master_gain_linear;

    CleanAmpForBass(double rate) : sample_rate(static_cast<float>(rate)) {

        hpf.reset(); bass_shelf.reset(); mid_peak.reset(); treble_shelf.reset(); lpf.reset();

        last_hpf = 30.0f;
        last_bass = 0.0f;
        last_mid_f = 500.0f;
        last_mid_g = 0.0f;
        last_treble = 0.0f;
        last_lpf = 5000.0f;
        last_master = 0.0f;
        master_gain_linear = 1.0f;

        hpf.setHighPass(last_hpf, sample_rate);
        bass_shelf.setLowShelf(80.0f, sample_rate, last_bass);
        mid_peak.setPeak(last_mid_f, sample_rate, last_mid_g);
        treble_shelf.setHighShelf(3000.0f, sample_rate, last_treble);
        lpf.setLowPass(last_lpf, sample_rate);
    }

    inline void update_params() {
        if (*ports[PORT_MASTER_GAIN] != last_master) {
            last_master = *ports[PORT_MASTER_GAIN];
            master_gain_linear = std::pow(10.0f, last_master / 20.0f);
        }
        if (*ports[PORT_HPF_FREQ] != last_hpf) {
            last_hpf = *ports[PORT_HPF_FREQ];
            hpf.setHighPass(last_hpf, sample_rate);
        }
        if (*ports[PORT_BASS_GAIN] != last_bass) {
            last_bass = *ports[PORT_BASS_GAIN];
            bass_shelf.setLowShelf(80.0f, sample_rate, last_bass);
        }
        if (*ports[PORT_MID_FREQ] != last_mid_f || *ports[PORT_MID_GAIN] != last_mid_g) {
            last_mid_f = *ports[PORT_MID_FREQ];
            last_mid_g = *ports[PORT_MID_GAIN];
            mid_peak.setPeak(last_mid_f, sample_rate, last_mid_g);
        }
        if (*ports[PORT_TREBLE_GAIN] != last_treble) {
            last_treble = *ports[PORT_TREBLE_GAIN];
            treble_shelf.setHighShelf(3000.0f, sample_rate, last_treble);
        }
        if (*ports[PORT_LPF_FREQ] != last_lpf) {
            last_lpf = *ports[PORT_LPF_FREQ];
            lpf.setLowPass(last_lpf, sample_rate);
        }
    }

    static LV2_Handle instantiate(const LV2_Descriptor*, double rate, const char*, const LV2_Feature* const*) {
        return new CleanAmpForBass(rate);
    }

    static void connect_port(LV2_Handle instance, uint32_t port, void* data) {
        static_cast<CleanAmpForBass*>(instance)->ports[port] = static_cast<const float*>(data);
    }

    static void run(LV2_Handle instance, uint32_t n_samples) {
        auto* self = static_cast<CleanAmpForBass*>(instance);

        self->update_params();

        const float* in = self->ports[PORT_IN];
        float* out = const_cast<float*>(self->ports[PORT_OUT]);
        const float gain = self->master_gain_linear;

        #pragma GCC ivdep
        for (uint32_t i = 0; i < n_samples; ++i) {
            float s = in[i];

            s = self->hpf.process(s);
            s = self->bass_shelf.process(s);
            s = self->mid_peak.process(s);
            s = self->treble_shelf.process(s);
            s = self->lpf.process(s);

            out[i] = std::clamp(s * gain, -1.0f, 1.0f);
        }
    }

    static void cleanup(LV2_Handle instance) {
        delete static_cast<CleanAmpForBass*>(instance);
    }
};

static const LV2_Descriptor descriptor = {
    "https://github.com/YimRakHee/CleanAmpForBass",
    CleanAmpForBass::instantiate,
    CleanAmpForBass::connect_port,
    nullptr,
    CleanAmpForBass::run,
    nullptr,
    CleanAmpForBass::cleanup,
    nullptr
};

extern "C" LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index) {
    return (index == 0) ? &descriptor : nullptr;
}
