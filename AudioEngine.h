#ifndef PEDALERA_AUDIOENGINE_H
#define PEDALERA_AUDIOENGINE_H

#include <oboe/Oboe.h>
#include <memory>
#include <vector>
#include <mutex>

#include "IEffect.h"

/**
 * AudioEngine
 * -----------
 * Clase principal que administra el stream de audio de baja latencia
 * usando Oboe y ejecuta la cadena de efectos (pedales) en serie sobre
 * cada bloque de audio entrante.
 *
 * Flujo general:
 *   micrófono / entrada de línea -> Oboe (input stream)
 *        -> onAudioReady() (callback tiempo real)
 *             -> recorre la cadena de IEffect en orden
 *        -> Oboe (output stream) -> parlante / salida de línea
 *
 * AudioEngine hereda de oboe::AudioStreamCallback para recibir los
 * buffers de audio directamente desde Oboe sin pasar por Java/Kotlin,
 * lo cual es clave para mantener la latencia baja.
 */
class AudioEngine : public oboe::AudioStreamCallback {
public:
    AudioEngine() = default;
    ~AudioEngine() override;

    // Evitamos copias: un AudioEngine representa un recurso de audio
    // único (streams abiertos), copiarlo no tendría sentido.
    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    /**
     * Abre y arranca los streams de entrada y salida de Oboe.
     * @return true si el motor quedó corriendo correctamente.
     */
    bool start();

    /**
     * Detiene y libera los streams de audio.
     * Seguro de llamar aunque el motor no haya arrancado.
     */
    void stop();

    /**
     * Agrega un efecto al final de la cadena de señal.
     * Thread-safe: puede llamarse desde el hilo de UI mientras el
     * audio sigue corriendo, gracias al mutex interno.
     */
    void addEffect(const std::shared_ptr<IEffect>& effect);

    /**
     * Elimina todos los efectos de la cadena.
     */
    void clearEffects();

    /**
     * Devuelve el efecto en la posición `index` de la cadena, o
     * nullptr si el índice no es válido. Útil para que la capa JNI
     * ajuste parámetros (drive, tono, nivel) de un pedal específico
     * sin tener que reconstruir toda la cadena.
     */
    std::shared_ptr<IEffect> getEffect(size_t index);

    // --- oboe::AudioStreamCallback ---

    /**
     * Callback invocado por Oboe en el hilo de audio en tiempo real
     * cada vez que hay un nuevo bloque de muestras para procesar.
     * Aquí es donde se recorre la cadena de efectos.
     */
    oboe::DataCallbackResult onAudioReady(
            oboe::AudioStream* audioStream,
            void* audioData,
            int32_t numFrames) override;

    /**
     * Se invoca si el stream se cierra inesperadamente (por ejemplo,
     * se desconectan los audífonos). Intentamos reabrir el motor.
     */
    void onErrorAfterClose(oboe::AudioStream* audioStream, oboe::Result error) override;

private:
    /**
     * Abre un stream (input u output) con una configuración común:
     * LowLatency + Exclusive, con fallback automático a Shared si
     * el dispositivo no soporta modo exclusivo.
     */
    oboe::Result openStream(oboe::Direction direction,
                             std::shared_ptr<oboe::AudioStream>& streamOut);

    std::shared_ptr<oboe::AudioStream> mInputStream;
    std::shared_ptr<oboe::AudioStream> mOutputStream;

    // Cadena de efectos aplicados en serie. Se protege con mutex
    // porque se modifica desde el hilo de UI (JNI) y se lee desde
    // el hilo de audio en onAudioReady().
    std::vector<std::shared_ptr<IEffect>> mEffectChain;
    std::mutex mEffectChainMutex;

    static constexpr int32_t kSampleRate = 48000;
    static constexpr int32_t kChannelCount = 1; // mono: entrada de guitarra
};

#endif // PEDALERA_AUDIOENGINE_H
