#include <Arduino.h>
#include "esp_freertos_hooks.h"

#define PIN_LED     2
#define PIN_BOTON   0
#define PIN_ADC     34
#define LED_RAPIDO_MS   100
#define LED_LENTO_MS    500
#define SENSOR_MS       300
#define MONITOR_MS      20
#define TIMEOUT_LENTO   5000

volatile bool g_ledRapido = true;
volatile bool g_botonPres = false;
volatile bool g_sensorActivo = false;

TaskHandle_t hLedRapido = NULL;
TaskHandle_t hLedLento  = NULL;
TaskHandle_t hSensor    = NULL;
TaskHandle_t hMonitor   = NULL;

unsigned long tiempoInicioLento = 0;

// Idle Hook 
bool idleHookESP32(void) {
  static unsigned long ultimoIdle = 0;

  if (millis() - ultimoIdle >= 2000) {
    ultimoIdle = millis();
    Serial.println("[IDLE] CPU libre — esperando evento de boton");
  }

  return true;
}

// Tarea LED rápido
void vTaskLedRapido(void *pvParameters) {
  while (true) {
    if (g_ledRapido) {
      digitalWrite(PIN_LED, HIGH);
      Serial.printf("[LED_R] ON tick:%lu\n", millis());
      vTaskDelay(pdMS_TO_TICKS(LED_RAPIDO_MS));

      digitalWrite(PIN_LED, LOW);
      Serial.printf("[LED_R] OFF tick:%lu\n", millis());
      vTaskDelay(pdMS_TO_TICKS(LED_RAPIDO_MS));
    }

    if (g_botonPres) {
      Serial.println("[LED_R] Boton detectado -> activando modo LENTO");

      g_botonPres = false;
      g_ledRapido = false;
      g_sensorActivo = true;
      tiempoInicioLento = millis();

      vTaskSuspend(NULL);
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// Tarea LED lento
void vTaskLedLento(void *pvParameters) {
  while (true) {
    if (!g_ledRapido) {
      unsigned long tiempoActual = millis();
      unsigned long transcurrido = tiempoActual - tiempoInicioLento;

      if (transcurrido >= TIMEOUT_LENTO) {
        Serial.println("[LED_L] Timeout 5s -> regresando a modo RAPIDO");

        g_ledRapido = true;
        g_sensorActivo = false;

        if (hLedRapido != NULL) {
          vTaskResume(hLedRapido);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
      } else {
        unsigned long restante = TIMEOUT_LENTO - transcurrido;

        digitalWrite(PIN_LED, HIGH);
        Serial.printf("[LED_L] ON tick:%lu t_restante:%lums\n", millis(), restante);
        vTaskDelay(pdMS_TO_TICKS(LED_LENTO_MS));

        digitalWrite(PIN_LED, LOW);
        Serial.printf("[LED_L] OFF tick:%lu t_restante:%lums\n", millis(), restante);
        vTaskDelay(pdMS_TO_TICKS(LED_LENTO_MS));
      }
    } else {
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

//ADC
void vTaskSensor(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();

  while (true) {
    if (g_sensorActivo) {
      int adcRaw = analogRead(PIN_ADC);
      float voltaje = (adcRaw * 3.3f) / 4095.0f;

      Serial.printf("[SENS] ADC raw:%d %.2fV tick:%lu\n", adcRaw, voltaje, millis());
    }

    vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SENSOR_MS));
  }
}

// Tarea monitor
void vTaskMonitor(void *pvParameters) {
  bool estadoAnterior = HIGH;
  unsigned long ultimoReporte = 0;
  unsigned long ultimoRebote = 0;

  while (true) {
    bool estadoActual = digitalRead(PIN_BOTON);

    if (estadoAnterior == HIGH && estadoActual == LOW) {
      if (millis() - ultimoRebote > 250) {
        ultimoRebote = millis();

        if (g_ledRapido) {
          g_botonPres = true;
          Serial.println("[MON] *** BOTON PRESIONADO ***");
        }
      }
    }

    estadoAnterior = estadoActual;

    if (millis() - ultimoReporte >= 1000) {
      ultimoReporte = millis();

      Serial.printf(
        "[MON] Heap:%u Stack_R:%u Stack_L:%u Stack_S:%u words\n",
        ESP.getFreeHeap(),
        (unsigned int)uxTaskGetStackHighWaterMark(hLedRapido),
        (unsigned int)uxTaskGetStackHighWaterMark(hLedLento),
        (unsigned int)uxTaskGetStackHighWaterMark(hSensor)
      );
    }

    vTaskDelay(pdMS_TO_TICKS(MONITOR_MS));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BOTON, INPUT_PULLUP);
  pinMode(PIN_ADC, INPUT);

  analogReadResolution(12);

  esp_register_freertos_idle_hook(idleHookESP32);

  Serial.println("=== Practica 1 Extendida — FreeRTOS ESP32 ===");
  Serial.println("Presiona el boton BOOT GPIO0 para iniciar modo LENTO");

  xTaskCreate(vTaskLedRapido, "LED_RAPIDO", 2048, NULL, 1, &hLedRapido);
  xTaskCreate(vTaskLedLento,  "LED_LENTO",  2048, NULL, 2, &hLedLento);
  xTaskCreate(vTaskSensor,    "SENSOR_ADC", 2048, NULL, 3, &hSensor);
  xTaskCreate(vTaskMonitor,   "MONITOR",    3072, NULL, 4, &hMonitor);
}

void loop() {
  vTaskDelay(portMAX_DELAY);
}