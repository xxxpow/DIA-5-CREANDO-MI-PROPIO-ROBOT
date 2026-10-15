# 🤖 Robi - Día 5: Creando Mi Propio Robot Desde Cero

¡Bienvenido al repositorio oficial del **Día 5** de la serie de YouTube: *Creando mi propio robot desde cero*! En este capítulo avanzamos con la conversación inteligente y la interacción entre el robot **Robi** y nuestra aplicación móvil desarrollada en **Android Studio**.

---

## 📁 Contenido del Repositorio

* **[`robot-firmware/`](./robot-firmware/)**:
  * `Robi_Dia5_ojos_enojados.ino`: Código fuente para Arduino / ESP32 que controla las expresiones faciales (ojos enojados), módulos de audio, sensores y comunicación.
* **[`android-app/`](./android-app/)**:
  * Proyecto completo de Android Studio (`RobiVozz`) desarrollado en Kotlin para la interacción y envío de comandos por voz hacia Robi.
* **[`guion/`](./guion/)**:
  * `dia 5.docx`: Guion completo utilizado para la grabación del video del Día 5.
* **[`pistas-audio/`](./pistas-audio/)**:
  * Pistas de voz y efectos de sonido en formato `.mp3` (pistas 01 a 37) utilizadas para las respuestas y reacciones de Robi.

---

## 🚀 Cómo Usar Este Proyecto

### 1. Robot Firmware (Arduino / ESP32)
1. Abre el archivo `robot-firmware/Robi_Dia5_ojos_enojados.ino` en el Arduino IDE.
2. Instala las librerías necesarias especificadas en el código (pantalla, audio, etc.).
3. Selecciona tu placa y puerto COM correspondiente y compila/sube el código.

### 2. Aplicación Android (RobiVozz)
1. Abre **Android Studio**.
2. Selecciona **Open** y navega hasta la carpeta `android-app/`.
3. Espera a que Gradle sincronice las dependencias del proyecto.
4. Conecta tu teléfono Android o inicia un emulador y presiona **Run** (`Shift + F10`).

---

## 📺 Serie en YouTube
Sigue el paso a paso de la construcción de Robi, la explicación del circuito y las mejoras en cada video de la serie en YouTube.

¡Gracias por apoyar el proyecto! No olvides dejar tu estrella ⭐ en este repositorio.
