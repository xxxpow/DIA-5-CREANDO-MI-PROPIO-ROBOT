
package com.inventosyproyectos.robivozz

import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Bundle
import android.os.Looper
import android.speech.RecognitionListener
import android.speech.RecognizerIntent
import android.speech.SpeechRecognizer
import android.widget.Button
import android.widget.TextView
import java.net.HttpURLConnection
import java.net.URL
import java.text.Normalizer
import java.util.Locale

class MainActivity : Activity() {

    private lateinit var btnHablar: Button
    private lateinit var txtEstado: TextView
    private lateinit var txtReconocido: TextView
    private lateinit var txtDialogo: TextView

    private var reconocedor: SpeechRecognizer? = null

    companion object {
        const val PERMISO_MICROFONO = 100
        const val IP_ROBI = "192.168.4.1"
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        btnHablar = findViewById(R.id.btnHablar)
        txtEstado = findViewById(R.id.txtEstado)
        txtReconocido = findViewById(R.id.txtReconocido)
        txtDialogo = findViewById(R.id.txtDialogo)

        if (SpeechRecognizer.isRecognitionAvailable(this)) {
            reconocedor = SpeechRecognizer.createSpeechRecognizer(this).apply {
                setRecognitionListener(object : RecognitionListener {

                    override fun onReadyForSpeech(params: Bundle?) {
                        txtEstado.text = "ESCUCHANDO..."
                    }

                    override fun onBeginningOfSpeech() {}

                    override fun onRmsChanged(rmsdB: Float) {}

                    override fun onBufferReceived(buffer: ByteArray?) {}

                    override fun onEndOfSpeech() {
                        txtEstado.text = "PROCESANDO VOZ..."
                    }

                    override fun onError(error: Int) {
                        txtEstado.text = when (error) {
                            SpeechRecognizer.ERROR_NETWORK ->
                                "ERROR: SERVICIO DE VOZ SIN RED"
                            SpeechRecognizer.ERROR_NETWORK_TIMEOUT ->
                                "TIEMPO DE ESPERA AGOTADO"
                            SpeechRecognizer.ERROR_NO_MATCH ->
                                "NO SE RECONOCIÓ LA FRASE"
                            SpeechRecognizer.ERROR_SPEECH_TIMEOUT ->
                                "NO SE DETECTÓ VOZ"
                            SpeechRecognizer.ERROR_AUDIO ->
                                "ERROR DE MICRÓFONO"
                            SpeechRecognizer.ERROR_INSUFFICIENT_PERMISSIONS ->
                                "FALTA PERMISO DE MICRÓFONO"
                            SpeechRecognizer.ERROR_RECOGNIZER_BUSY ->
                                "RECONOCEDOR OCUPADO; INTENTA OTRA VEZ"
                            SpeechRecognizer.ERROR_CLIENT ->
                                "ERROR DEL SERVICIO DE RECONOCIMIENTO"
                            else ->
                                "ERROR DE RECONOCIMIENTO: $error"
                        }
                    }

                    override fun onResults(results: Bundle?) {
                        val frases = results?.getStringArrayList(
                            SpeechRecognizer.RESULTS_RECOGNITION
                        )

                        if (frases.isNullOrEmpty()) {
                            txtEstado.text = "NO SE RECIBIÓ NINGUNA FRASE"
                            return
                        }

                        val texto = frases[0]
                        txtReconocido.text = "Tú dijiste: $texto"
                        procesarPregunta(texto)
                    }

                    override fun onPartialResults(partialResults: Bundle?) {}

                    override fun onEvent(eventType: Int, params: Bundle?) {}
                })
            }
        } else {
            txtEstado.text = "NO HAY SERVICIO DE RECONOCIMIENTO"
        }

        btnHablar.setOnClickListener {
            if (checkSelfPermission(Manifest.permission.RECORD_AUDIO) !=
                PackageManager.PERMISSION_GRANTED
            ) {
                requestPermissions(
                    arrayOf(Manifest.permission.RECORD_AUDIO),
                    PERMISO_MICROFONO
                )
            } else {
                escuchar()
            }
        }
    }

    private fun escuchar() {
        val servicio = reconocedor

        if (servicio == null) {
            txtEstado.text = "NO HAY SERVICIO DE RECONOCIMIENTO"
            return
        }

        txtEstado.text = "PREPARANDO RECONOCIMIENTO..."
        txtReconocido.text = ""
        txtDialogo.text = ""

        val intent = Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH).apply {
            putExtra(
                RecognizerIntent.EXTRA_LANGUAGE_MODEL,
                RecognizerIntent.LANGUAGE_MODEL_FREE_FORM
            )
            putExtra(RecognizerIntent.EXTRA_LANGUAGE, "es-US")
            putExtra(RecognizerIntent.EXTRA_LANGUAGE_PREFERENCE, "es-US")
            putExtra(RecognizerIntent.EXTRA_MAX_RESULTS, 5)
            putExtra(RecognizerIntent.EXTRA_PREFER_OFFLINE, true)
        }

        try {
            servicio.startListening(intent)
        } catch (e: Exception) {
            txtEstado.text = "NO SE PUDO INICIAR EL RECONOCIMIENTO"
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)

        if (requestCode == PERMISO_MICROFONO) {
            if (grantResults.isNotEmpty() &&
                grantResults[0] == PackageManager.PERMISSION_GRANTED
            ) {
                escuchar()
            } else {
                txtEstado.text = "PERMISO DE MICRÓFONO DENEGADO"
            }
        }
    }

    override fun onDestroy() {
        reconocedor?.destroy()
        reconocedor = null
        super.onDestroy()
    }

    private fun procesarPregunta(textoOriginal: String) {
        val texto = normalizar(textoOriginal)
        val comando = identificarDialogo(texto)

        if (comando == 0) {
            txtEstado.text = "NO RECONOCÍ LA FRASE DE ACTIVACIÓN"
            txtDialogo.text = ""
        } else {
            txtEstado.text = "COMANDO RECONOCIDO"
            txtDialogo.text = "Comando: $comando"
            enviarDialogoAlRobot(comando)
        }
    }

    private fun normalizar(texto: String): String {
        return Normalizer.normalize(
            texto.lowercase(Locale.ROOT),
            Normalizer.Form.NFD
        )
            .replace(Regex("\\p{Mn}+"), "")
            .replace(Regex("[^a-z0-9\\s]"), " ")
            .replace(Regex("\\s+"), " ")
            .trim()
    }

    private fun identificarDialogo(texto: String): Int {
        return when {
            texto.contains("nuestro vinculo es la electronica y la programacion") ||
                    (texto.contains("vinculo") && texto.contains("electronica") &&
                            texto.contains("programacion")) -> 1

            texto.contains("si claro tu eres mi hijo") ||
                    (texto.contains("tu eres mi hijo") && texto.contains("claro")) -> 2

            texto.contains("que cosa quieres saber hijo") ||
                    (texto.contains("que cosa quieres saber")||
                            (texto.contains("que cosa quiere saber") &&
                            texto.contains("hijo"))) -> 3

            texto.contains("pero casi todos me conocen como moño") ||
                    (texto.contains("me conocen como mono") ||
                            (texto.contains("me conocen como moña") &&
                            texto.contains("casi todos"))) -> 4

            (
                    texto.contains("mis padres") &&
                            texto.contains("familiares") &&
                            texto.contains("me llaman") &&
                            texto.contains("naci")
                    ) -> 5

            texto.contains("podrias llamarte robotin") ||
                    (texto.contains("llamarte robotin") &&
                            texto.contains("podrias")) -> 6

            texto.contains("suena bastante tecnologico") ||
                    (texto.contains("bastante tecnologico") &&
                            texto.contains("suena")) -> 7

            texto.contains("y que nombre encontraste") ||
                    texto.contains("que nombre encontraste") -> 8

            (
                    (texto.contains("roberto es un nombre de persona") ||
                            texto.contains("roberto es un hombre de persona") ||
                            texto.contains("roberto es nombre de persona") ||
                            texto.contains("roberto es un nombre"))
                    ) -> 9

            texto.contains("a partir de ahora te llamare roberto") ||
                    (texto.contains("te llamare roberto") &&
                            texto.contains("a partir de ahora")) -> 10
            (
                    (texto.contains("creo que voy a llamarte robi") ||
                            texto.contains("creo que voy a llamarte robbie") ||
                            texto.contains("voy a llamarte robi") ||
                            texto.contains("voy a llamarte robbie")) &&
                            (texto.contains("corto") || texto.contains("mas corto"))
                    ) -> 11

            (texto.contains("tu y yo") && texto.contains("todavia tenemos") &&
                    texto.contains("muchas cosas por construir")) ||
                    texto.contains("muchas cosas por construir") -> 12

            else -> 0
        }
    }

    private fun enviarDialogoAlRobot(comando: Int) {
        Thread {
            var conexion: HttpURLConnection? = null

            try {
                val direccion = "http://$IP_ROBI/pista?p=$comando"
                conexion = URL(direccion).openConnection() as HttpURLConnection
                conexion.requestMethod = "GET"
                conexion.connectTimeout = 3000
                conexion.readTimeout = 3000

                val codigo = conexion.responseCode

                runOnUiThread {
                    txtEstado.text = if (codigo in 200..299) {
                        "COMANDO ENVIADO A ROBI"
                    } else {
                        "ROBI NO RESPONDIÓ"
                    }
                }
            } catch (e: Exception) {
                runOnUiThread {
                    txtEstado.text = "ERROR DE CONEXIÓN CON ROBI"
                }
            } finally {
                conexion?.disconnect()
            }
        }.start()
    }
}
