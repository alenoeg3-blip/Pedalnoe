package com.example.pedalera

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.core.content.ContextCompat

/**
 * MainActivity
 * ------------
 * Pantalla principal de la pedalera: un botón de encendido/apagado
 * del motor de audio y tres sliders (Drive, Tono, Level) que controlan
 * el efecto de overdrive en tiempo real a través de JNI.
 *
 * IMPORTANTE sobre los sliders: cada vez que el usuario mueve un
 * slider llamamos directamente a la función nativa correspondiente
 * (setOverdriveDrive, etc.). Esas funciones solo escriben un
 * std::atomic<float> en C++ — son extremadamente rápidas y NO
 * bloquean ni tocan el hilo de audio en tiempo real, así que es
 * seguro llamarlas en cada evento de arrastre del slider sin
 * necesidad de debounce.
 */
class MainActivity : ComponentActivity() {

    companion object {
        init {
            // Debe coincidir con el nombre de la librería definida en
            // CMakeLists.txt: add_library(pedalera_audio SHARED ...)
            System.loadLibrary("pedalera_audio")
        }
    }

    // --- Funciones nativas implementadas en native-lib.cpp ---
    external fun startEngine(): Boolean
    external fun stopEngine()
    external fun setOverdriveEnabled(enabled: Boolean)
    external fun setOverdriveDrive(drive: Float)
    external fun setOverdriveLevel(level: Float)
    external fun setOverdriveTone(tone: Float)

    private var engineRunning by mutableStateOf(false)
    private var micPermissionGranted by mutableStateOf(false)

    private val requestMicPermission = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { granted ->
        micPermissionGranted = granted
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        micPermissionGranted = ContextCompat.checkSelfPermission(
            this, Manifest.permission.RECORD_AUDIO
        ) == PackageManager.PERMISSION_GRANTED

        if (!micPermissionGranted) {
            requestMicPermission.launch(Manifest.permission.RECORD_AUDIO)
        }

        setContent {
            MaterialTheme {
                Surface(modifier = Modifier.fillMaxSize()) {
                    PedalScreen()
                }
            }
        }
    }

    override fun onStop() {
        super.onStop()
        // Si la app pasa a segundo plano, detenemos el motor para no
        // seguir consumiendo el micrófono ni la batería innecesariamente.
        if (engineRunning) {
            stopEngine()
            engineRunning = false
        }
    }

    @Composable
    fun PedalScreen() {
        var drive by remember { mutableFloatStateOf(0.3f) }
        var tone by remember { mutableFloatStateOf(0.5f) }
        var level by remember { mutableFloatStateOf(0.7f) }
        var overdriveOn by remember { mutableStateOf(true) }

        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(24.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            Text(text = "Pedalera de Guitarra", style = MaterialTheme.typography.headlineSmall)

            if (!micPermissionGranted) {
                Text(text = "Se necesita permiso de micrófono para procesar audio.")
            }

            Button(
                onClick = {
                    if (!micPermissionGranted) {
                        requestMicPermission.launch(Manifest.permission.RECORD_AUDIO)
                        return@Button
                    }
                    if (engineRunning) {
                        stopEngine()
                    } else {
                        startEngine()
                    }
                    engineRunning = !engineRunning
                },
                enabled = micPermissionGranted
            ) {
                Text(if (engineRunning) "Apagar motor" else "Encender motor")
            }

            HorizontalDivider()

            Text(text = "Overdrive")
            Switch(
                checked = overdriveOn,
                onCheckedChange = {
                    overdriveOn = it
                    setOverdriveEnabled(it)
                }
            )

            Text(text = "Drive: ${"%.2f".format(drive)}")
            Slider(
                value = drive,
                onValueChange = {
                    drive = it
                    setOverdriveDrive(it)
                },
                valueRange = 0f..1f
            )

            Text(text = "Tono: ${"%.2f".format(tone)}")
            Slider(
                value = tone,
                onValueChange = {
                    tone = it
                    setOverdriveTone(it)
                },
                valueRange = 0f..1f
            )

            Text(text = "Level: ${"%.2f".format(level)}")
            Slider(
                value = level,
                onValueChange = {
                    level = it
                    setOverdriveLevel(it)
                },
                valueRange = 0f..1f
            )
        }
    }
}
