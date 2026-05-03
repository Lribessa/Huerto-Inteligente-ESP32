#include <Wire.h>
#include <RTClib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "LittleFS.h"

// --- CONFIGURACIÓN ---
const char* ssid = "NOMBRE SSID";         
const char* password = "CONTRASEÑA";        
String urlScript = "https://script.google.com/macros/exec"; 

const int VALOR_SECO = 2550; // CALIBRA TUS SENSORES A TU SUELO. VALOR MAXIMO
const int VALOR_MOJADO = 1100; // CALIBRA TUS SENSORES A TU SUELO. VALOR MINIMO

RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);
  delay(1000);

  if (!rtc.begin()) {
    Serial.println("No se encuentra el RTC");
    while (1);
  }

  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Descomenta solo para poner en hora

  if (!LittleFS.begin(true)) {
    Serial.println("Error al montar LittleFS");
    return;
  }

  DateTime ahora = rtc.now();
  
  // --- 1. LÓGICA DE GRABACIÓN CON HORA EXACTA ---
  // ID_Turno usa solo el día y la hora para no repetir en la misma ventana de 60min
  String ID_Turno = String(ahora.day()) + "_" + String(ahora.hour());
  bool yaGrabado = false;
  if (LittleFS.exists("/check.txt")) {
    File f = LittleFS.open("/check.txt", FILE_READ);
    if (f.readString() == ID_Turno) yaGrabado = true;
    f.close();
  }

  if ((ahora.hour() == 7 || ahora.hour() == 19) && !yaGrabado) {
    File file = LittleFS.open("/pendientes.txt", FILE_APPEND);
    if (file) {
      char fechaF[11], horaF[6];
      sprintf(fechaF, "%02d/%02d/%04d", ahora.day(), ahora.month(), ahora.year());
      // CAMBIO: Ahora guarda hora Y minutos exactos (ej: 07:02)
      sprintf(horaF, "%02d:%02d", ahora.hour(), ahora.minute()); 
      
      int s1 = map(constrain(analogRead(36), VALOR_MOJADO, VALOR_SECO), VALOR_SECO, VALOR_MOJADO, 0, 100);
      int s2 = map(constrain(analogRead(39), VALOR_MOJADO, VALOR_SECO), VALOR_SECO, VALOR_MOJADO, 0, 100);
      int s3 = map(constrain(analogRead(34), VALOR_MOJADO, VALOR_SECO), VALOR_SECO, VALOR_MOJADO, 0, 100);
      
      file.printf("%s,%s,%d,%d,%d\n", fechaF, horaF, s1, s2, s3);
      file.close();
      
      File fStatus = LittleFS.open("/check.txt", FILE_WRITE);
      fStatus.print(ID_Turno);
      fStatus.close();
      Serial.println("Lectura guardada a las " + String(horaF));
    }
  }

  // --- 2. ENVÍO (SOLO POR BOTÓN EN) ---
  esp_sleep_wakeup_cause_t razon = esp_sleep_get_wakeup_cause();
  
  if (razon != ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println("Botón EN detectado. Conectando...");
    WiFi.begin(ssid, password);
    
    unsigned long start = millis();
    // Espera máximo 15 segundos a conectar
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      File file = LittleFS.open("/pendientes.txt", FILE_READ);
      if (file) {
        while (file.available()) {
          String linea = file.readStringUntil('\n');
          linea.trim();
          if (linea.length() > 0) {
            HTTPClient http;
            http.begin(urlScript + "?datos=" + linea);
            http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
            http.GET();
            http.end();
          }
        }
        file.close();
        LittleFS.remove("/pendientes.txt");
        Serial.println("\n¡Datos enviados!");
      }
    }
    // Forzamos un pequeño delay para asegurar que el paquete salió
    delay(2000); 
  }

  // --- 3. AUTO-DESCONEXIÓN Y SUEÑO (1 HORA) ---
  Serial.println("Desconectando y durmiendo...");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF); // Apaga el radio Wi-Fi por completo
  
  esp_sleep_enable_timer_wakeup(3600ULL * 1000000ULL); 
  esp_deep_sleep_start();
}

void loop() {}
