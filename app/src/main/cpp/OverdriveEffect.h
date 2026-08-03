#ifndef PEDALERA_OVERDRIVEEFFECT_H
#define PEDALERA_OVERDRIVEEFFECT_H

#include <atomic>
#include "IEffect.h"

/**
 * OverdriveEffect
 * ---------------
 * Pedal de distorsión/overdrive clásico basado en "soft-clipping"
 * con tanh(), que simula la saturación progresiva de un amplificador
 * o pedal analógico (a diferencia del "hard-clipping", que corta la
 * señal de forma abrupta y suena más agresivo/digital).
 *
 * Parámetros:
 *   - drive:  cuánto se "empuja" la señal antes de saturarla (ganancia
 *             de entrada). A mayor drive, más distorsión.
 *   - tone:   filtro simple de un polo que oscurece (0.0) o brilla
 *             (1.0) el sonido resultante, simulando el control de
 *             tono de un pedal real.
 *   - level:  ganancia de salida, para compensar el volumen extra
 *             que agrega el drive y dejar la señal a un nivel usable.
 */
class OverdriveEffect : public IEffect {
public:
    OverdriveEffect() = default;

    void process(float* audioData, int32_t numFrames, int32_t channelCount) override;

    // Los setters son thread-safe (std::atomic) porque se llaman desde
    // el hilo de UI (a través de JNI) mientras process() los lee desde
    // el hilo de audio en tiempo real.

    /** @param drive Rango sugerido 0.0 (sin distorsión) a 1.0 (máxima). */
    void setDrive(float drive);

    /** @param tone Rango 0.0 (oscuro) a 1.0 (brillante). */
    void setTone(float tone);

    /** @param level Rango 0.0 (silencio) a 1.0 (nivel de salida pleno). */
    void setLevel(float level);

    float getDrive() const { return mDrive.load(); }
    float getTone() const { return mTone.load(); }
    float getLevel() const { return mLevel.load(); }

private:
    // Mapea el drive (0-1) a un factor de ganancia de entrada real.
    // 1.0 -> prácticamente sin distorsión; valores altos empujan la
    // señal mucho más allá de +-1.0 antes del tanh(), generando más
    // saturación.
    static constexpr float kMinGain = 1.0f;
    static constexpr float kMaxGain = 20.0f;

    std::atomic<float> mDrive{0.3f};
    std::atomic<float> mTone{0.5f};
    std::atomic<float> mLevel{0.7f};

    // Estado del filtro de tono (un polo, IIR de primer orden).
    // Se mantiene por canal para no mezclar historia entre L/R.
    static constexpr int kMaxChannels = 2;
    float mToneFilterState[kMaxChannels] = {0.0f, 0.0f};
};

#endif // PEDALERA_OVERDRIVEEFFECT_H
