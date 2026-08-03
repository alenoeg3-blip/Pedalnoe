#include "OverdriveEffect.h"
#include <cmath>
#include <algorithm>

void OverdriveEffect::setDrive(float drive) {
    mDrive.store(std::clamp(drive, 0.0f, 1.0f));
}

void OverdriveEffect::setTone(float tone) {
    mTone.store(std::clamp(tone, 0.0f, 1.0f));
}

void OverdriveEffect::setLevel(float level) {
    mLevel.store(std::clamp(level, 0.0f, 1.0f));
}

void OverdriveEffect::process(float* audioData, int32_t numFrames, int32_t channelCount) {
    // Leemos los parámetros una sola vez por buffer (no por muestra)
    // para evitar overhead de accesos atómicos repetidos.
    const float driveParam = mDrive.load();
    const float toneParam = mTone.load();
    const float levelParam = mLevel.load();

    // Mapeo de drive (0-1) a ganancia de entrada real (1x a 20x).
    const float inputGain = kMinGain + driveParam * (kMaxGain - kMinGain);

    // Coeficiente del filtro de tono: un simple low-pass de un polo.
    // toneParam alto -> coeficiente bajo -> pasa más agudos (más brillo).
    const float toneCoeff = 1.0f - (toneParam * 0.7f);

    channelCount = std::min(channelCount, kMaxChannels);

    for (int32_t frame = 0; frame < numFrames; ++frame) {
        for (int32_t ch = 0; ch < channelCount; ++ch) {
            int32_t idx = frame * channelCount + ch;
            float sample = audioData[idx];

            // 1. Ganancia de entrada: empujamos la señal antes de saturar.
            sample *= inputGain;

            // 2. Soft-clipping con tanh(): comprime suavemente los picos
            //    en vez de cortarlos de forma abrupta, simulando la
            //    saturación de un amplificador analógico.
            sample = std::tanh(sample);

            // 3. Filtro de tono (low-pass de un polo, IIR):
            //    y[n] = y[n-1] + toneCoeff * (x[n] - y[n-1])
            float& filterState = mToneFilterState[ch];
            filterState = filterState + toneCoeff * (sample - filterState);
            sample = filterState;

            // 4. Ganancia de salida para ajustar el nivel final.
            sample *= levelParam;

            audioData[idx] = sample;
        }
    }
}
