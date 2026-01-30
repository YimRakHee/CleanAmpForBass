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

    // 파라미터 캐싱용 변수
    float last_hpf, last_bass, last_mid_f, last_mid_g, last_treble, last_lpf, last_master;
    float master_gain_linear;

    CleanAmpForBass(double rate) : sample_rate(static_cast<float>(rate)) {
        // 1. 내부 상태 초기화
        hpf.reset(); bass_shelf.reset(); mid_peak.reset(); treble_shelf.reset(); lpf.reset();

        // 2. 초기값을 .ttl의 default 값과 일치시켜 첫 run에서의 중복 계산 방지
        last_hpf = 30.0f;
        last_bass = 0.0f;
        last_mid_f = 500.0f;
        last_mid_g = 0.0f;
        last_treble = 0.0f;
        last_lpf = 5000.0f;
        last_master = 0.0f;
        master_gain_linear = 1.0f;

        // 3. 인스턴스 생성 시(비실시간 스레드) 미리 계수 계산
        // 이 과정 덕분에 첫 run() 호출 시 삼각함수 연산이 실행되지 않아 Xrun이 방지됩니다.
        hpf.setHighPass(last_hpf, sample_rate);
        bass_shelf.setLowShelf(80.0f, sample_rate, last_bass);
        mid_peak.setPeak(last_mid_f, sample_rate, last_mid_g);
        treble_shelf.setHighShelf(3000.0f, sample_rate, last_treble);
        lpf.setLowPass(last_lpf, sample_rate);
    }

    // 인라인 함수로 성능 최적화
    inline void update_params() {
        // 값이 실제로 변했을 때만 무거운 연산(pow, sin, cos) 수행
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

        // 파라미터 업데이트 (변경 사항이 있을 때만 동작)
        self->update_params();

        const float* in = self->ports[PORT_IN];
        float* out = const_cast<float*>(self->ports[PORT_OUT]);
        const float gain = self->master_gain_linear;

        // 루프 최적화 힌트
        #pragma GCC ivdep
        for (uint32_t i = 0; i < n_samples; ++i) {
            float s = in[i];

            // 필터 체인 (Biquad.hpp의 process 함수는 inline 필수)
            s = self->hpf.process(s);
            s = self->bass_shelf.process(s);
            s = self->mid_peak.process(s);
            s = self->treble_shelf.process(s);
            s = self->lpf.process(s);

            // 최종 출력 및 클리핑 방지
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
