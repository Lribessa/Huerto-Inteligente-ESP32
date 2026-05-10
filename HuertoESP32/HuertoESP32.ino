#include <Wire.h>
#include <RTClib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "LittleFS.h"

// --- CONFIGURACIÓN ---
const char* ssid = "nombre wifi";         
const char* password = "contraseña";        
String urlScript = "url script"; 

const int VALOR_SECO = 3300;
const int VALOR_MOJADO = 1500;

RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);
  if (!rtc.begin() || !LittleFS.begin(true)) esp_deep_sleep_start();

  DateTime ahora = rtc.now();
  esp_sleep_wakeup_cause_t razon = esp_sleep_get_wakeup_cause();

  // --- 1. ENVÍO MANUAL (BOTÓN EN) ---
  if (razon != ESP_SLEEP_WAKEUP_TIMER) {
    WiFi.begin(ssid, password);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) { delay(100); }

    if (WiFi.status() == WL_CONNECTED) {
      if (LittleFS.exists("/pendientes.txt")) {
        File file = LittleFS.open("/pendientes.txt", FILE_READ);
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
      }
    }
    esp_sleep_enable_timer_wakeup(10 * 1000000ULL); 
    esp_deep_sleep_start();
  }

  // --- 2. LÓGICA DE GRABACIÓN (FRANJAS REDUCIDAS) ---
  int franja = 0; 
  // Mañana: 06:00 a 10:00 | Tarde: 18:00 a 22:00
  if (ahora.hour() >= 6 && ahora.hour() < 10) franja = 1;
  else if (ahora.hour() >= 18 && ahora.hour() < 22) franja = 2;

  if (franja > 0) {
    String ID_Turno = String(ahora.day()) + "_" + String(franja);
    bool yaGrabado = false;
    
    if (LittleFS.exists("/check.txt")) {
      File f = LittleFS.open("/check.txt", FILE_READ);
      if (f.readString() == ID_Turno) yaGrabado = true;
      f.close();
    }

    if (!yaGrabado) {
      File file = LittleFS.open("/pendientes.txt", FILE_APPEND);
      if (file) {
        char fechaF[11], horaF[6];
        sprintf(fechaF, "%02d/%02d/%04d", ahora.day(), ahora.month(), ahora.year());
        sprintf(horaF, "%02d:%02d", ahora.hour(), ahora.minute()); 
        
        int s1 = map(constrain(analogRead(36), VALOR_MOJADO, VALOR_SECO), VALOR_SECO, VALOR_MOJADO, 0, 100);
        int s2 = map(constrain(analogRead(39), VALOR_MOJADO, VALOR_SECO), VALOR_SECO, VALOR_MOJADO, 0, 100);
        int s3 = map(constrain(analogRead(34), VALOR_MOJADO, VALOR_SECO), VALOR_SECO, VALOR_MOJADO, 0, 100);
        
        file.printf("%s,%s,%d,%d,%d\n", fechaF, horaF, s1, s2, s3);
        file.close();
        
        File fStatus = LittleFS.open("/check.txt", FILE_WRITE);
        fStatus.print(ID_Turno);
        fStatus.close();
      }
    }
  }

  // --- 3. DORMIR (3 HORAS) ---
  // Despertando cada 3 horas garantizamos pillar ambas franjas de 4 horas
  // y solo hacemos 8 comprobaciones al día.
  esp_sleep_enable_timer_wakeup(10800ULL * 1000000ULL); 
  esp_deep_sleep_start();
}

void loop() {}
