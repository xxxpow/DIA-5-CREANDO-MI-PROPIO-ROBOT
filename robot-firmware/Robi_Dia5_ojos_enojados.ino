/*
  ROBI - DIA 5
  ESP32 + TFT ST7789 + MP3-TF-18P + LED de boca

  RobiVozz envia un ID de dialogo:
  http://192.168.4.1/pista?p=1   -> pistas 1, 2 y 3
  ...
  http://192.168.4.1/pista?p=12  -> pistas 33 a 37

  PAUSA_ENTRE_PISTAS_MS permite ajustar las pausas entre fragmentos.
*/

#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <math.h>

// ---------------- WIFI ROBI ----------------
const char* WIFI_SSID = "ROBI";
const char* WIFI_PASSWORD = "12345678";
WebServer server(80);

// ---------------- TFT ST7789 ----------------
#define TFT_CS   -1
#define TFT_DC   27
#define TFT_RST  4
#define TFT_SCLK 18
#define TFT_MOSI 23

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// ---------------- COLORES / EXPRESIONES ----------------
#define COLOR_NORMAL    0x067F
#define COLOR_ALEGRE    0xFFE0
#define COLOR_PENSATIVO 0x067F
#define COLOR_ENOJO     0xF800

enum Expresion {
  EXPRESION_NORMAL,
  EXPRESION_PENSATIVA,
  EXPRESION_ALEGRE,
  EXPRESION_SORPRENDIDO,
  EXPRESION_TRISTE,
  EXPRESION_ENOJADO
};

Expresion expresionActual = EXPRESION_NORMAL;
void mostrarExpresion(Expresion expresion);

// ---------------- MP3 TF-18P ----------------
HardwareSerial mp3Serial(2);
#define MP3_RX 16
#define MP3_TX 17
#define CMD_PLAY_INDEX 0x03
#define CMD_SET_VOLUME 0x06
#define CMD_SEL_DEV 0x09
#define DEV_TF 0x02

// ---------------- BUSY Y LED DE BOCA ----------------
#define MP3_BUSY_PIN 19
#define LED_VOZ_PIN 5
#define MP3_REPRODUCIENDO LOW
#define INTERVALO_LED_VOZ 180

bool mp3Reproduciendo = false;
bool respuestaEnCurso = false;
int estadoBusyAnterior = HIGH;
unsigned long ultimoParpadeoLED = 0;
bool estadoLEDVoz = false;

// ---------------- CONFIGURACION DE PAUSAS ----------------
const unsigned long PAUSA_ENTRE_PISTAS_MS = 500;

// ---------------- PISTAS DE AUDIO ----------------
const uint16_t PISTA_01 = 1;
const uint16_t PISTA_02 = 2;
const uint16_t PISTA_03 = 3;
const uint16_t PISTA_04 = 4;
const uint16_t PISTA_05 = 5;
const uint16_t PISTA_06 = 6;
const uint16_t PISTA_07 = 7;
const uint16_t PISTA_08 = 8;
const uint16_t PISTA_09 = 9;
const uint16_t PISTA_10 = 10;
const uint16_t PISTA_11 = 11;
const uint16_t PISTA_12 = 12;
const uint16_t PISTA_13 = 13;
const uint16_t PISTA_14 = 14;
const uint16_t PISTA_15 = 15;
const uint16_t PISTA_16 = 16;
const uint16_t PISTA_17 = 17;
const uint16_t PISTA_18 = 18;
const uint16_t PISTA_19 = 19;
const uint16_t PISTA_20 = 20;
const uint16_t PISTA_21 = 21;
const uint16_t PISTA_22 = 22;
const uint16_t PISTA_23 = 23;
const uint16_t PISTA_24 = 24;
const uint16_t PISTA_25 = 25;
const uint16_t PISTA_26 = 26;
const uint16_t PISTA_27 = 27;
const uint16_t PISTA_28 = 28;
const uint16_t PISTA_29 = 29;
const uint16_t PISTA_30 = 30;
const uint16_t PISTA_31 = 31;
const uint16_t PISTA_32 = 32;
const uint16_t PISTA_33 = 33;
const uint16_t PISTA_34 = 34;
const uint16_t PISTA_35 = 35;
const uint16_t PISTA_36 = 36;
const uint16_t PISTA_37 = 37;

uint16_t obtenerNumeroPista(uint8_t indice) {
  switch (indice) {
    case 1: return PISTA_01;
    case 2: return PISTA_02;
    case 3: return PISTA_03;
    case 4: return PISTA_04;
    case 5: return PISTA_05;
    case 6: return PISTA_06;
    case 7: return PISTA_07;
    case 8: return PISTA_08;
    case 9: return PISTA_09;
    case 10: return PISTA_10;
    case 11: return PISTA_11;
    case 12: return PISTA_12;
    case 13: return PISTA_13;
    case 14: return PISTA_14;
    case 15: return PISTA_15;
    case 16: return PISTA_16;
    case 17: return PISTA_17;
    case 18: return PISTA_18;
    case 19: return PISTA_19;
    case 20: return PISTA_20;
    case 21: return PISTA_21;
    case 22: return PISTA_22;
    case 23: return PISTA_23;
    case 24: return PISTA_24;
    case 25: return PISTA_25;
    case 26: return PISTA_26;
    case 27: return PISTA_27;
    case 28: return PISTA_28;
    case 29: return PISTA_29;
    case 30: return PISTA_30;
    case 31: return PISTA_31;
    case 32: return PISTA_32;
    case 33: return PISTA_33;
    case 34: return PISTA_34;
    case 35: return PISTA_35;
    case 36: return PISTA_36;
    case 37: return PISTA_37;
    default: return 0;
  }
}

// ---------------- DIALOGOS: GRUPOS DE PISTAS ----------------
const uint8_t TOTAL_DIALOGOS = 12;

const uint8_t inicioDialogo[TOTAL_DIALOGOS + 1] = {
  0,
  1,   // Dialogo 1: pistas 1-3
  4,   // Dialogo 2: pistas 4-6
  7,   // Dialogo 3: pistas 7-8
  9,   // Dialogo 4: pistas 9-11
  12,  // Dialogo 5: pistas 12-14
  15,  // Dialogo 6: pistas 15-16
  17,  // Dialogo 7: pistas 17-22
  23,  // Dialogo 8: pistas 23-24
  25,  // Dialogo 9: pistas 25-27
  28,  // Dialogo 10: pista 28
  29,  // Dialogo 11: pistas 29-32
  33   // Dialogo 12: pistas 33-37
};

const uint8_t finDialogo[TOTAL_DIALOGOS + 1] = {
  0,
  3, 6, 8, 11, 14, 16, 22, 24, 27, 28, 32, 37
};

Expresion obtenerExpresionDialogo(uint8_t dialogo) {
  switch (dialogo) {
    case 1: return EXPRESION_SORPRENDIDO;
    case 2: return EXPRESION_ALEGRE;
    case 3: return EXPRESION_PENSATIVA;
    case 4: return EXPRESION_PENSATIVA;
    case 5: return EXPRESION_SORPRENDIDO;
    case 6: return EXPRESION_PENSATIVA;
    case 7: return EXPRESION_TRISTE;
    case 8: return EXPRESION_ALEGRE;
    case 9: return EXPRESION_ENOJADO;
    case 10: return EXPRESION_ALEGRE;
    case 11: return EXPRESION_ALEGRE;
    case 12: return EXPRESION_PENSATIVA;
    default: return EXPRESION_NORMAL;
  }
}

// ---------------- DIBUJO DE OJOS ----------------
void dibujarOjosNormales(int desplazamientoX = 0, int desplazamientoY = 0) {
  tft.fillScreen(ST77XX_BLACK);
  int izquierdaX = 20 + desplazamientoX;
  int derechaX = 137 + desplazamientoX;
  int ojosY = 80 + desplazamientoY;

  tft.fillRoundRect(izquierdaX, ojosY, 100, 120, 17, COLOR_NORMAL);
  tft.fillRoundRect(derechaX, ojosY, 100, 120, 17, COLOR_NORMAL);
  tft.fillCircle(izquierdaX + 50, ojosY + 60, 20, ST77XX_BLACK);
  tft.fillCircle(derechaX + 50, ojosY + 60, 20, ST77XX_BLACK);
}

void dibujarOjosPensativos() {
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRoundRect(20, 80, 100, 80, 17, COLOR_PENSATIVO);
  tft.fillCircle(80, 115, 30, ST77XX_BLACK);
  tft.fillRoundRect(137, 80, 100, 80, 17, COLOR_PENSATIVO);
  tft.fillCircle(197, 115, 30, ST77XX_BLACK);
}

void dibujarUnArco(int centroX, int centroY) {
  for (int angulo = 200; angulo <= 340; angulo += 5) {
    float rad = angulo * PI / 180.0;
    int x = centroX + (int)(cos(rad) * 44);
    int y = centroY + (int)(sin(rad) * 28);
    tft.fillCircle(x, y, 5, COLOR_ALEGRE);
  }
}

void dibujarOjosAlegres() {
  tft.fillScreen(ST77XX_BLACK);
  dibujarUnArco(82, 140);
  dibujarUnArco(178, 140);
}

void dibujarOjosSorprendidos() {
  tft.fillScreen(ST77XX_BLACK);
  tft.fillCircle(60, 140, 70, ST77XX_WHITE);
  tft.fillCircle(60, 140, 10, ST77XX_BLACK);
  tft.fillCircle(178, 140, 70, ST77XX_WHITE);
  tft.fillCircle(178, 140, 10, ST77XX_BLACK);
}

void dibujarOjosTristes() {
  tft.fillScreen(ST77XX_BLACK);
  tft.fillCircle(60, 140, 70, COLOR_NORMAL);
  tft.fillCircle(60, 126, 50, ST77XX_BLACK);
  tft.fillCircle(48, 112, 10, ST77XX_WHITE);
  tft.fillCircle(70, 125, 4, ST77XX_WHITE);
  tft.fillCircle(178, 140, 70, COLOR_NORMAL);
  tft.fillCircle(178, 126, 50, ST77XX_BLACK);
  tft.fillCircle(166, 112, 12, ST77XX_WHITE);
  tft.fillCircle(185, 122, 5, ST77XX_WHITE);
  tft.fillCircle(178, 138, 3, ST77XX_WHITE);
}

// ---------------- OJOS ENOJADOS ANIMADOS ----------------
#define ENOJO_POSICION_X 127
#define ENOJO_POSICION_Y 120
#define ENOJO_SEPARACION 12
#define ENOJO_FIG1_ANCHO 90
#define ENOJO_FIG1_ALTO  70
#define ENOJO_FIG2_ANCHO 90
#define ENOJO_FIG2_ALTO  24
#define INTERVALO_FIGURA_ENOJO 300
typedef struct {
  int x;
  int y;
} Punto;
bool figuraEnojoGrande = true;
unsigned long ultimoCambioFiguraEnojo = 0;

void dibujarPoligono(void* datosPuntos, int cantidad, uint16_t color) {
  Punto* puntos = (Punto*)datosPuntos;
  int minY = puntos[0].y;
  int maxY = puntos[0].y;

  for (int i = 1; i < cantidad; i++) {
    if (puntos[i].y < minY) minY = puntos[i].y;
    if (puntos[i].y > maxY) maxY = puntos[i].y;
  }

  for (int y = minY; y <= maxY; y++) {
    int nodos[20];
    int cantidadNodos = 0;

    for (int i = 0; i < cantidad; i++) {
      int j = (i + 1) % cantidad;
      int yi = puntos[i].y;
      int yj = puntos[j].y;

      if ((yi < y && yj >= y) || (yj < y && yi >= y)) {
        int xi = puntos[i].x;
        int xj = puntos[j].x;
        int x = xi + (long)(y - yi) * (xj - xi) / (yj - yi);
        if (cantidadNodos < 20) nodos[cantidadNodos++] = x;
      }
    }

    for (int i = 0; i < cantidadNodos - 1; i++) {
      for (int j = i + 1; j < cantidadNodos; j++) {
        if (nodos[i] > nodos[j]) {
          int temporal = nodos[i];
          nodos[i] = nodos[j];
          nodos[j] = temporal;
        }
      }
    }

    for (int i = 0; i + 1 < cantidadNodos; i += 2) {
      tft.drawFastHLine(nodos[i], y, nodos[i + 1] - nodos[i] + 1, color);
    }
  }
}

void dibujarOjoEnojo(int centroX, int centroY, int ancho, int alto,
                     bool izquierdo, bool figuraGrande) {
  int mitadAncho = ancho / 2;
  int mitadAlto = alto / 2;
  Punto puntos[7];

  if (figuraGrande) {
    if (izquierdo) {
      puntos[0] = {centroX + mitadAncho, centroY - (mitadAlto * 45) / 100};
      puntos[1] = {centroX - (mitadAncho * 65) / 100, centroY - mitadAlto};
      puntos[2] = {centroX - (mitadAncho * 95) / 100, centroY - (mitadAlto * 55) / 100};
      puntos[3] = {centroX - mitadAncho, centroY};
      puntos[4] = {centroX - (mitadAncho * 95) / 100, centroY + (mitadAlto * 55) / 100};
      puntos[5] = {centroX - (mitadAncho * 65) / 100, centroY + mitadAlto};
      puntos[6] = {centroX + mitadAncho, centroY + (mitadAlto * 45) / 100};
    } else {
      puntos[0] = {centroX - mitadAncho, centroY - (mitadAlto * 45) / 100};
      puntos[1] = {centroX + (mitadAncho * 65) / 100, centroY - mitadAlto};
      puntos[2] = {centroX + (mitadAncho * 95) / 100, centroY - (mitadAlto * 55) / 100};
      puntos[3] = {centroX + mitadAncho, centroY};
      puntos[4] = {centroX + (mitadAncho * 95) / 100, centroY + (mitadAlto * 55) / 100};
      puntos[5] = {centroX + (mitadAncho * 65) / 100, centroY + mitadAlto};
      puntos[6] = {centroX - mitadAncho, centroY + (mitadAlto * 45) / 100};
    }
  } else {
    if (izquierdo) {
      puntos[0] = {centroX + mitadAncho, centroY - (mitadAlto * 25) / 100};
      puntos[1] = {centroX - (mitadAncho * 70) / 100, centroY - mitadAlto};
      puntos[2] = {centroX - (mitadAncho * 95) / 100, centroY - (mitadAlto * 40) / 100};
      puntos[3] = {centroX - mitadAncho, centroY};
      puntos[4] = {centroX - (mitadAncho * 95) / 100, centroY + (mitadAlto * 40) / 100};
      puntos[5] = {centroX - (mitadAncho * 70) / 100, centroY + mitadAlto};
      puntos[6] = {centroX + mitadAncho, centroY + (mitadAlto * 25) / 100};
    } else {
      puntos[0] = {centroX - mitadAncho, centroY - (mitadAlto * 25) / 100};
      puntos[1] = {centroX + (mitadAncho * 70) / 100, centroY - mitadAlto};
      puntos[2] = {centroX + (mitadAncho * 95) / 100, centroY - (mitadAlto * 40) / 100};
      puntos[3] = {centroX + mitadAncho, centroY};
      puntos[4] = {centroX + (mitadAncho * 95) / 100, centroY + (mitadAlto * 40) / 100};
      puntos[5] = {centroX + (mitadAncho * 70) / 100, centroY + mitadAlto};
      puntos[6] = {centroX - mitadAncho, centroY + (mitadAlto * 25) / 100};
    }
  }

  dibujarPoligono(puntos, 7, COLOR_ENOJO);
}

void mostrarFiguraEnojo(bool figuraGrande) {
  tft.fillScreen(ST77XX_BLACK);

  int ancho = figuraGrande ? ENOJO_FIG1_ANCHO : ENOJO_FIG2_ANCHO;
  int alto  = figuraGrande ? ENOJO_FIG1_ALTO : ENOJO_FIG2_ALTO;
  int distancia = (ancho / 2) + (ENOJO_SEPARACION / 2);

  dibujarOjoEnojo(ENOJO_POSICION_X - distancia, ENOJO_POSICION_Y,
                  ancho, alto, true, figuraGrande);
  dibujarOjoEnojo(ENOJO_POSICION_X + distancia, ENOJO_POSICION_Y,
                  ancho, alto, false, figuraGrande);
}

void dibujarOjosEnojados() {
  figuraEnojoGrande = true;
  ultimoCambioFiguraEnojo = millis();
  mostrarFiguraEnojo(true);
}

void actualizarAnimacionEnojo() {
  if (expresionActual != EXPRESION_ENOJADO) return;

  unsigned long ahora = millis();
  if (ahora - ultimoCambioFiguraEnojo >= INTERVALO_FIGURA_ENOJO) {
    ultimoCambioFiguraEnojo = ahora;
    figuraEnojoGrande = !figuraEnojoGrande;
    mostrarFiguraEnojo(figuraEnojoGrande);
  }
}

// ---------------- SELECCION DE EXPRESIONES ----------------
void mostrarExpresion(Expresion expresion) {
  // No redibujar si la expresion ya esta activa.
  if (expresionActual == expresion) {
    return;
  }

  expresionActual = expresion;

  switch (expresion) {
    case EXPRESION_NORMAL:
      dibujarOjosNormales();
      break;

    case EXPRESION_PENSATIVA:
      dibujarOjosPensativos();
      break;

    case EXPRESION_ALEGRE:
      dibujarOjosAlegres();
      break;

    case EXPRESION_SORPRENDIDO:
      dibujarOjosSorprendidos();
      break;

    case EXPRESION_TRISTE:
      dibujarOjosTristes();
      break;

    case EXPRESION_ENOJADO:
      dibujarOjosEnojados();
      break;
  }
}

// ---------------- PARPADEO NORMAL ----------------
unsigned long ultimoParpadeoOjos = 0;
enum EstadoParpadeo {
  PARPADEO_NINGUNO,
  PARPADEO_PRIMERO_CERRADO,
  PARPADEO_ESPERA_SEGUNDO,
  PARPADEO_SEGUNDO_CERRADO
};

EstadoParpadeo estadoParpadeo = PARPADEO_NINGUNO;
bool siguienteParpadeoDoble = false;
#define INTERVALO_PARPADEO_OJOS 2500
#define DURACION_PARPADEO_OJOS 250
#define INTERVALO_SEGUNDO_PARPADEO 150

void cerrarOjosNormal() {
  tft.fillRect(20, 137, 100, 6, COLOR_NORMAL);
  tft.fillRect(137, 137, 100, 6, COLOR_NORMAL);
}

void actualizarParpadeoOjos() {
  if (expresionActual != EXPRESION_NORMAL) {
    estadoParpadeo = PARPADEO_NINGUNO;
    return;
  }

  unsigned long ahora = millis();
  if (estadoParpadeo == PARPADEO_NINGUNO) {
    if (ahora - ultimoParpadeoOjos >= INTERVALO_PARPADEO_OJOS) {
      cerrarOjosNormal();
      estadoParpadeo = PARPADEO_PRIMERO_CERRADO;
      ultimoParpadeoOjos = ahora;
    }
    return;
  }

  if (estadoParpadeo == PARPADEO_PRIMERO_CERRADO &&
      ahora - ultimoParpadeoOjos >= DURACION_PARPADEO_OJOS) {
    dibujarOjosNormales();
    if (siguienteParpadeoDoble) {
      estadoParpadeo = PARPADEO_ESPERA_SEGUNDO;
      ultimoParpadeoOjos = ahora;
    } else {
      estadoParpadeo = PARPADEO_NINGUNO;
      ultimoParpadeoOjos = ahora;
    }
    return;
  }

  if (estadoParpadeo == PARPADEO_ESPERA_SEGUNDO &&
      ahora - ultimoParpadeoOjos >= INTERVALO_SEGUNDO_PARPADEO) {
    cerrarOjosNormal();
    estadoParpadeo = PARPADEO_SEGUNDO_CERRADO;
    ultimoParpadeoOjos = ahora;
    return;
  }

  if (estadoParpadeo == PARPADEO_SEGUNDO_CERRADO &&
      ahora - ultimoParpadeoOjos >= DURACION_PARPADEO_OJOS) {
    dibujarOjosNormales();
    estadoParpadeo = PARPADEO_NINGUNO;
    ultimoParpadeoOjos = ahora;
  }
}

// ---------------- COMUNICACION CON MP3 ----------------
void enviarComandoMP3(byte comando, uint16_t parametro) {
  byte buffer[10];
  buffer[0] = 0x7E;
  buffer[1] = 0xFF;
  buffer[2] = 0x06;
  buffer[3] = comando;
  buffer[4] = 0x00;
  buffer[5] = parametro >> 8;
  buffer[6] = parametro & 0xFF;
  uint16_t checksum = 0 - (0xFF + 0x06 + comando + 0x00 +
                           (parametro >> 8) + (parametro & 0xFF));
  buffer[7] = checksum >> 8;
  buffer[8] = checksum & 0xFF;
  buffer[9] = 0xEF;
  mp3Serial.write(buffer, 10);
}

void configurarMP3() {
  mp3Serial.begin(9600, SERIAL_8N1, MP3_RX, MP3_TX);
  delay(500);
  enviarComandoMP3(CMD_SEL_DEV, DEV_TF);
  delay(500);
  enviarComandoMP3(CMD_SET_VOLUME, 25);
  delay(500);
}

// ---------------- ESTADO DE REPRODUCCION ----------------
uint8_t dialogoActual = 0;
uint8_t pistaActualDelDialogo = 0;
uint8_t ultimaPistaDelDialogo = 0;
bool pausaEntrePistas = false;
unsigned long inicioPausaEntrePistas = 0;
bool ultimoDialogoFueFinal = false;

void iniciarPistaActual() {
  uint16_t pista = obtenerNumeroPista(pistaActualDelDialogo);
  if (pista == 0) {
    respuestaEnCurso = false;
    return;
  }

  // Dialogo 2: pista 4 alegre; pistas 5 y 6 sorprendidas.
  if (dialogoActual == 2) {
    if (pistaActualDelDialogo == 4) {
      mostrarExpresion(EXPRESION_ALEGRE);
    } else if (pistaActualDelDialogo == 5 ||
               pistaActualDelDialogo == 6) {
      mostrarExpresion(EXPRESION_SORPRENDIDO);
    }
  }

  // Pistas 9 y 10: ojos alegres.
  if (pistaActualDelDialogo == 9 || pistaActualDelDialogo == 10) {
    mostrarExpresion(EXPRESION_ALEGRE);
  }

  // Pistas 17, 18 y 19: ojos enojados rojos.
  if (pistaActualDelDialogo >= 17 && pistaActualDelDialogo <= 19) {
    mostrarExpresion(EXPRESION_ENOJADO);
  }

  // Pistas 20 y 21: ojos pensativos.
  if (pistaActualDelDialogo == 20 || pistaActualDelDialogo == 21) {
    mostrarExpresion(EXPRESION_PENSATIVA);
  }

  // Pista 22: ojos alegres.
  if (pistaActualDelDialogo == 22) {
    mostrarExpresion(EXPRESION_ALEGRE);
  }
  // Pista 32: ojos tristes.
  if (pistaActualDelDialogo == 32) {
    mostrarExpresion(EXPRESION_TRISTE);
  }
    // Pista 33: ojos alegres.
  if (pistaActualDelDialogo == 33) {
    mostrarExpresion(EXPRESION_ALEGRE);
  }

  // Pista 34: ojos tristes.
  if (pistaActualDelDialogo == 34) {
    mostrarExpresion(EXPRESION_TRISTE);
  }

  // Pista 35: ojos alegres.
  if (pistaActualDelDialogo == 35) {
    mostrarExpresion(EXPRESION_ALEGRE);
  }

  // Pista 36: ojos normales.
  if (pistaActualDelDialogo == 36) {
    mostrarExpresion(EXPRESION_NORMAL);
  }

  // Pista 37: ojos alegres.
  if (pistaActualDelDialogo == 37) {
    mostrarExpresion(EXPRESION_ALEGRE);
  }
  respuestaEnCurso = true;
  estadoParpadeo = PARPADEO_NINGUNO;
  ultimoParpadeoOjos = millis();

  enviarComandoMP3(CMD_PLAY_INDEX, pista);

  Serial.print("DIA 5 - Dialogo ");
  Serial.print(dialogoActual);
  Serial.print(", pista de guion ");
  Serial.print(pistaActualDelDialogo);
  Serial.print(" -> MP3 index ");
  Serial.println(pista);
}

void iniciarDialogo(uint8_t dialogo) {
  if (dialogo < 1 || dialogo > TOTAL_DIALOGOS) {
    Serial.println("ROBI: numero de dialogo no valido");
    return;
  }

  if (mp3Reproduciendo || respuestaEnCurso || pausaEntrePistas) {
    Serial.println("ROBI: ocupado; espera a que termine el dialogo actual");
    return;
  }

  dialogoActual = dialogo;
  pistaActualDelDialogo = inicioDialogo[dialogo];
  ultimaPistaDelDialogo = finDialogo[dialogo];
  ultimoDialogoFueFinal = (dialogo == 12);

  if (dialogo == 2) {
    mostrarExpresion(EXPRESION_ALEGRE);
  } else {
    mostrarExpresion(obtenerExpresionDialogo(dialogo));
  }

  iniciarPistaActual();
}

void actualizarBusyMP3() {
  int estadoBusy = digitalRead(MP3_BUSY_PIN);
  bool reproduciendo = (estadoBusy == MP3_REPRODUCIENDO);
  mp3Reproduciendo = reproduciendo;

  if (reproduciendo) {
    if (millis() - ultimoParpadeoLED >= INTERVALO_LED_VOZ) {
      ultimoParpadeoLED = millis();
      estadoLEDVoz = !estadoLEDVoz;
      digitalWrite(LED_VOZ_PIN, estadoLEDVoz ? HIGH : LOW);
    }
  } else {
    estadoLEDVoz = false;
    digitalWrite(LED_VOZ_PIN, LOW);
  }

  if (respuestaEnCurso &&
      estadoBusyAnterior == MP3_REPRODUCIENDO &&
      !reproduciendo) {
    respuestaEnCurso = false;

    if (pistaActualDelDialogo < ultimaPistaDelDialogo) {
      pausaEntrePistas = true;
      inicioPausaEntrePistas = millis();
      Serial.print("Pista ");
      Serial.print(pistaActualDelDialogo);
      Serial.print(" terminada. Pausa de ");
      Serial.print(PAUSA_ENTRE_PISTAS_MS);
      Serial.println(" ms antes de la siguiente pista.");
    } else {
      pausaEntrePistas = false;
      dialogoActual = 0;
      pistaActualDelDialogo = 0;
      ultimaPistaDelDialogo = 0;

      if (ultimoDialogoFueFinal) {
        mostrarExpresion(EXPRESION_ALEGRE);
      } else {
        mostrarExpresion(EXPRESION_NORMAL);
      }

      ultimoDialogoFueFinal = false;
      Serial.println("ROBI: dialogo terminado");
    }
  }

  estadoBusyAnterior = estadoBusy;
}

void procesarPausaEntrePistas() {
  if (!pausaEntrePistas) return;
  if (millis() - inicioPausaEntrePistas < PAUSA_ENTRE_PISTAS_MS) return;
  if (mp3Reproduciendo || respuestaEnCurso) return;

  pausaEntrePistas = false;
  pistaActualDelDialogo++;
  iniciarPistaActual();
}

// ---------------- PAGINA WEB ----------------
void paginaPrincipal() {
  String pagina =
    "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Robi Dia 5</title></head><body>"
    "<h1>ROBI - DIA 5</h1>"
    "<p>ESP32 conectado correctamente.</p>"
    "<p>IP: 192.168.4.1</p>"
    "<p>Envia /pista?p=1 hasta /pista?p=12 para iniciar un dialogo.</p>"
    "</body></html>";
  server.send(200, "text/html", pagina);
}

void manejarPista() {
  if (!server.hasArg("p")) {
    server.send(400, "text/plain", "Falta parametro p");
    return;
  }

  int dialogo = server.arg("p").toInt();
  Serial.print("Comando recibido desde celular: dialogo ");
  Serial.println(dialogo);

  if (dialogo < 1 || dialogo > TOTAL_DIALOGOS) {
    server.send(400, "text/plain", "El dialogo debe estar entre 1 y 12");
    return;
  }

  if (mp3Reproduciendo || respuestaEnCurso || pausaEntrePistas) {
    server.send(409, "text/plain", "ROBI ocupado; espera a que termine");
    return;
  }

  iniciarDialogo((uint8_t)dialogo);
  server.send(200, "text/plain", "OK");
}

// ---------------- WIFI ----------------
void configurarWiFi() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  IPAddress IP = WiFi.softAPIP();

  Serial.println();
  Serial.println("==============================");
  Serial.println(" WIFI ROBI - DIA 5");
  Serial.print(" SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print(" IP: ");
  Serial.println(IP);
  Serial.println("==============================");

  server.on("/", paginaPrincipal);
  server.on("/pista", manejarPista);
  server.begin();
  Serial.println("Servidor HTTP iniciado");
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.init(240, 240, SPI_MODE3);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  pinMode(MP3_BUSY_PIN, INPUT);
  pinMode(LED_VOZ_PIN, OUTPUT);
  digitalWrite(LED_VOZ_PIN, LOW);
  estadoBusyAnterior = digitalRead(MP3_BUSY_PIN);

  configurarMP3();
  mostrarExpresion(EXPRESION_NORMAL);
  configurarWiFi();

  Serial.println();
  Serial.println("==============================");
  Serial.println(" ROBI - DIA 5");
  Serial.println(" SISTEMA INICIADO");
  Serial.println(" 37 PISTAS / 12 COMANDOS DE DIALOGO");
  Serial.print(" PAUSA ENTRE PISTAS: ");
  Serial.print(PAUSA_ENTRE_PISTAS_MS);
  Serial.println(" ms");
  Serial.println(" SIN MICROFONO EN EL ESP32");
  Serial.println("==============================");
}

// ---------------- LOOP ----------------
void loop() {
  server.handleClient();
  actualizarBusyMP3();
  procesarPausaEntrePistas();
  actualizarParpadeoOjos();
  actualizarAnimacionEnojo();
  delay(10);
}
