#include <Wire.h>
#include <RTClib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "LittleFS.h"

// --- CONFIGURACIÓN ---
const char* ssid = "RedmiNote14";         
const char* password = "comemeelwifi";        
String urlScript = "https://script.google.com/macros/s/AKfycbzdsKCNDvW-xu7e_oObm2JNnqCKrZK0XJFU9-_dy5PKfOw6N5st6rxPU3TCVinQafxPqQ/exec"; 

const int VALOR_SECO = 3300;
const int VALOR_MOJADO = 1500;

RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- ESP32 DESPIERTO ---");

  if (!rtc.begin()) {
    Serial.println("ERROR: No se encuentra el RTC");
    while (1);
  }

  if (!LittleFS.begin(true)) {
    Serial.println("ERROR: LittleFS");
    return;
  }

  DateTime ahora = rtc.now();
  Serial.printf("Hora actual: %02d:%02d:%02d\n", ahora.hour(), ahora.minute(), ahora.second());

  // --- 1. LÓGICA DE GRABACIÓN ---
  String ID_Turno = String(ahora.day()) + "_" + String(ahora.hour());
  bool yaGrabado = false;
  
  if (LittleFS.exists("/check.txt")) {
    File f = LittleFS.open("/check.txt", FILE_READ);
    if (f.readString() == ID_Turno) yaGrabado = true;
    f.close();
  }

  if ((ahora.hour() == 7 || ahora.hour() == 19) && !yaGrabado) {
    Serial.println("Dentro de ventana horaria. Guardando lectura...");
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
      Serial.println("Datos guardados en memoria.");
    }
  }

  // --- 2. LÓGICA DE ENVÍO (REFORZADA) ---
  esp_sleep_wakeup_cause_t razon = esp_sleep_get_wakeup_cause();
  Serial.print("Razón del despertar: ");
  Serial.println(razon);

  // Intentamos enviar si:
  // - La razón NO es el temporizador (fue el botón EN)
  // - O si la razón es 0 (que significa reinicio por alimentación o botón en muchos modelos)
  if (razon != ESP_SLEEP_WAKEUP_TIMER) {
    Serial.println("Iniciando conexión Wi-Fi (Acción manual)...");
    WiFi.begin(ssid, password);
    
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) { // 20 seg de margen
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConectado. Enviando archivos...");
      if (LittleFS.exists("/pendientes.txt")) {
        File file = LittleFS.open("/pendientes.txt", FILE_READ);
        while (file.available()) {
          String linea = file.readStringUntil('\n');
          linea.trim();
          if (linea.length() > 0) {
            HTTPClient http;
            http.begin(urlScript + "?datos=" + linea);
            http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
            int httpCode = http.GET();
            Serial.printf("Enviado: %s | Respuesta: %d\n", linea.c_str(), httpCode);
            http.end();
          }
        }
        file.close();
        LittleFS.remove("/pendientes.txt");
        Serial.println("Memoria limpia.");
      } else {
        Serial.println("No hay datos pendientes para enviar.");
      }
    } else {
      Serial.println("\nError: No se pudo conectar al Wi-Fi.");
    }
    delay(2000); 
  } else {
    Serial.println("Despertado por temporizador: Wi-Fi omitido.");
  }

  // --- 3. DORMIR ---
  Serial.println("Entrando en Deep Sleep por 1 hora...");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  esp_sleep_enable_timer_wakeup(1800ULL * 1000000ULL); 
  esp_deep_sleep_start();
}

void loop() {}
