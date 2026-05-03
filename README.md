# Smart Irrigation Monitor (ESP32 + Google Sheets)

Este proyecto es un sistema de monitoreo de humedad de suelo diseñado para operar de forma autónoma en entornos con conectividad limitada y restricciones de energía (batería).

## 2. Arquitectura y Funcionamiento
A diferencia de los sistemas IoT convencionales que mantienen el Wi-Fi activo, este dispositivo prioriza la **autonomía de la batería** mediante una estrategia de "Sincronización Bajo Demanda".

*   **Ciclo de Trabajo:** El ESP32 despierta cada 60 minutos mediante *Timer Wakeup*.
*   **Captura de Datos:** Si el RTC indica que es la ventana de lectura (07:00 o 19:00), los datos de los sensores se almacenan en el sistema de archivos **LittleFS**.
*   **Transmisión:** El radio Wi-Fi solo se energiza si el dispositivo detecta un reinicio manual (Botón EN), procediendo a volcar los datos acumulados a un **Google App Script**.

## Stack Tecnológico
*   **Core:** ESP32 DevKit V1.
*   **Storage:** LittleFS (Flash interna del ESP32).
*   **Timekeeping:** RTC DS3231 via I2C.
*   **Cloud:** Google Apps Script / Google Sheets.

## Aprendizajes e Ingeniería de Soluciones

Durante el desarrollo se resolvieron varios retos técnicos que mejoraron la robustez del sistema:

### 1. Gestión de la "Deriva Térmica" del Temporizador
**Problema:** El despertador interno del ESP32 no es 100% preciso y podía despertar segundos después de la hora exacta, perdiendo la lectura.
**Solución:** Se implementó una **ventana de captura de 60 minutos**. El sistema comprueba si ya existe una lectura para esa hora específica usando un archivo de control (`check.txt`), permitiendo que el sensor grabe el dato en cualquier momento dentro de la hora programada sin duplicar entradas.

### 2. Optimización Energética Crítica
**Problema:** El consumo del Wi-Fi agotaba la batería rápidamente en intentos de conexión fallidos.
**Solución:** Se implementó lógica de discriminación de la causa de despertado (`esp_sleep_get_wakeup_cause`). El Wi-Fi permanece apagado durante los ciclos automáticos y solo se activa por intervención humana, reduciendo el consumo medio diario en un 95%.

### 3. Integridad de Datos en Formato 24h
Se estandarizó el uso de `sprintf` para formatear cadenas de texto, garantizando que el archivo CSV generado localmente mantenga siempre la misma estructura, facilitando el parseo posterior en Google Sheets.

## Instalación
1. Configurar `SSID` y `PASS` en el sketch de Arduino.
2. Desplegar el script de Google Apps Script incluido en la carpeta `/scripts`.
3. Calibrar valores de ADC para los sensores (actualmente: Seco 2550 / Mojado 1100).

## Galería del Proyecto

| Instalación en el Huerto | Detalle de Sensores |
| :---: | :---: |
| ![Huerto](img/huerto-final.jpg) | ![Sensores](img/sensores-suelo.jpg) |

### Datos Recibidos
![Google Sheets](img/hoja-calculo.png)
*Captura de los datos sincronizados tras pulsar el botón EN.*
