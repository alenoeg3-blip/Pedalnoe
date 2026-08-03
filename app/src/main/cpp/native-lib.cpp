#include <jni.h>
#include <memory>
#include "AudioEngine.h"
#include "OverdriveEffect.h"

/**
 * native-lib.cpp
 * --------------
 * Puente JNI entre Kotlin y el motor de audio en C++.
 *
 * Mantenemos UNA sola instancia global de AudioEngine y del
 * OverdriveEffect durante toda la vida del proceso nativo. Esto evita
 * tener que pasar punteros hacia y desde Kotlin (que sería frágil y
 * propenso a crashes si Kotlin guarda mal un puntero de 64 bits).
 *
 * Convención de nombres JNI:
 *   Java_<paquete_con_guiones_bajos>_<Clase>_<metodo>
 * Ajusta "com_example_pedalera_MainActivity" al paquete real de tu app.
 */

namespace {
    std::unique_ptr<AudioEngine> gAudioEngine;
    std::shared_ptr<OverdriveEffect> gOverdrive;
}

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_example_pedalera_MainActivity_startEngine(JNIEnv* env, jobject /* this */) {
    if (!gAudioEngine) {
        gAudioEngine = std::make_unique<AudioEngine>();
    }

    // Creamos el pedal de overdrive una sola vez y lo conectamos a la
    // cadena de efectos. Si más adelante agregas más pedales (delay,
    // chorus...), solo tienes que llamar addEffect() de nuevo aquí,
    // en el orden en que quieras que suenen en cadena.
    if (!gOverdrive) {
        gOverdrive = std::make_shared<OverdriveEffect>();
        gAudioEngine->addEffect(gOverdrive);
    }

    bool started = gAudioEngine->start();
    return static_cast<jboolean>(started);
}

JNIEXPORT void JNICALL
Java_com_example_pedalera_MainActivity_stopEngine(JNIEnv* env, jobject /* this */) {
    if (gAudioEngine) {
        gAudioEngine->stop();
    }
}

JNIEXPORT void JNICALL
Java_com_example_pedalera_MainActivity_setOverdriveEnabled(JNIEnv* env, jobject /* this */,
                                                             jboolean enabled) {
    if (gOverdrive) {
        gOverdrive->setEnabled(static_cast<bool>(enabled));
    }
}

JNIEXPORT void JNICALL
Java_com_example_pedalera_MainActivity_setOverdriveDrive(JNIEnv* env, jobject /* this */,
                                                           jfloat drive) {
    if (gOverdrive) {
        gOverdrive->setDrive(drive);
    }
}

JNIEXPORT void JNICALL
Java_com_example_pedalera_MainActivity_setOverdriveLevel(JNIEnv* env, jobject /* this */,
                                                           jfloat level) {
    if (gOverdrive) {
        gOverdrive->setLevel(level);
    }
}

JNIEXPORT void JNICALL
Java_com_example_pedalera_MainActivity_setOverdriveTone(JNIEnv* env, jobject /* this */,
                                                          jfloat tone) {
    if (gOverdrive) {
        gOverdrive->setTone(tone);
    }
}

} // extern "C"
