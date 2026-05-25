#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// Definisi Pin Hardware
#define DHTPIN 4
#define DHTTYPE DHT22
#define BUTTON_PIN 14  // Tombol: Hubungkan langsung ke GPIO 14 dan GND
#define LED_PIN 12     // LED: Hubungkan Kaki Panjang (Anoda) ke GPIO 12, Kaki Pendek (Katoda) ke GND

// Konfigurasi PWM untuk membatasi arus LED (Pengganti Resistor)
#define PWM_CHANNEL 0
#define PWM_FREQ    5000
#define PWM_RES     8
#define LED_SAFE_BRIGHTNESS 20 // Nilai 0-255 (Aman tanpa resistor eksternal)

// Inisialisasi Perangkat
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); 

struct DataSensor {
  float suhu;
  float kelembaban;
};

// Handle FreeRTOS
QueueHandle_t dataQueue;
TaskHandle_t xTaskLCDHandle = NULL; 

volatile bool isInterrupted = false;

// Prototipe Task & ISR
void taskLCD(void *pvParameters);
void taskDHT(void *pvParameters);
void taskSerial(void *pvParameters);
void IRAM_ATTR buttonISR(); 

void setup() {
  Serial.begin(115200);
  
  // Konfigurasi Button dengan Pull-Up Internal (Tanpa Resistor Eksternal)
  pinMode(BUTTON_PIN, INPUT_PULLUP); 
  
  // Konfigurasi LED menggunakan PWM/LEDC ESP32 (Pembatas Arus Software)
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RES);
  ledcAttachPin(LED_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0); // Pastikan LED mati di awal

  // Menggunakan fungsi standar digitalPinToInterrupt()
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  // Inisialisasi Sensor & LCD
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Mulai Sistem...");

  dataQueue = xQueueCreate(5, sizeof(DataSensor));

  if (dataQueue != NULL) {
    xTaskCreatePinnedToCore(taskLCD, "Task LCD", 2048, NULL, 3, &xTaskLCDHandle, 1);
    xTaskCreatePinnedToCore(taskDHT, "Task DHT", 2048, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(taskSerial, "Task Serial", 2048, NULL, 1, NULL, 0);
    
    Serial.println("Sistem FreeRTOS Siap.");
  } else {
    Serial.println("Gagal membuat Queue!");
  }
}

void loop() {
  vTaskDelete(NULL); 
}

/* ========================================================================
   FUNGSI INTERRUPT (ISR)
   ======================================================================== */
void IRAM_ATTR buttonISR() {
  isInterrupted = !isInterrupted; 
  
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(xTaskLCDHandle, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

/* ========================================================================
   1. TASK LCD (PRIORITAS TINGGI)
   ======================================================================== */
void taskLCD(void *pvParameters) {
  DataSensor dataLCD;
  for (;;) {
    if (isInterrupted) {
      // Kondisi Interrupt: Nyalakan LED dengan arus dibatasi (Aman)
      ledcWrite(PWM_CHANNEL, LED_SAFE_BRIGHTNESS);
      
      lcd.setCursor(0, 0);
      lcd.print("== INTERRUPT == ");
      lcd.setCursor(0, 1);
      lcd.print("DHT22 Dihentikan");
      
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      lcd.clear(); 
    } 
    else {
      // Kondisi Normal: Matikan LED
      ledcWrite(PWM_CHANNEL, 0);

      if (xQueuePeek(dataQueue, &dataLCD, pdMS_TO_TICKS(100)) == pdPASS) {
        lcd.setCursor(0, 0);
        lcd.print("Suhu  : ");
        lcd.print(dataLCD.suhu, 1);
        lcd.print((char)223); 
        lcd.print("C    ");
        
        lcd.setCursor(0, 1);
        lcd.print("Lembab: ");
        lcd.print(dataLCD.kelembaban, 1);
        lcd.print("%    ");
      }
      ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(250));
    }
  }
}

/* ========================================================================
   2. TASK DHT (PRIORITAS SEDANG)
   ======================================================================== */
void taskDHT(void *pvParameters) {
  DataSensor dataBaru;
  for (;;) {
    if (!isInterrupted) {
      float h = dht.readHumidity();
      float t = dht.readTemperature();

      if (!isnan(h) && !isnan(t)) {
        dataBaru.suhu = t;
        dataBaru.kelembaban = h;
        
        if (xQueueSend(dataQueue, &dataBaru, 0) != pdPASS) {
          DataSensor dummy;
          xQueueReceive(dataQueue, &dummy, 0); 
          xQueueSend(dataQueue, &dataBaru, 0);  
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(2000)); 
  }
}

/* ========================================================================
   3. TASK SERIAL MONITOR (PRIORITAS RENDAH)
   ======================================================================== */
void taskSerial(void *pvParameters) {
  DataSensor dataLog;
  for (;;) {
    // Tampilan jika interupsi sedang AKTIF
    if (isInterrupted) {
      Serial.print("[LOG Serial] Waktu: ");
      Serial.print(millis());
      Serial.println(" ms | Interrupt: AKTIF (Pembacaan Sensor Berhenti)");
    } 
    // Tampilan jika interupsi sedang NON-AKTIF (Kondisi Normal)
    else {
      // Mengambil data dari antrean jika tersedia
      if (xQueueReceive(dataQueue, &dataLog, pdMS_TO_TICKS(100)) == pdPASS) {
        Serial.print("[LOG Serial] Suhu: ");
        Serial.print(dataLog.suhu, 1);
        Serial.print(" C | Kelembaban: ");
        Serial.print(dataLog.kelembaban, 1);
        Serial.println(" % | Interrupt: NON-AKTIF");
      } else {
        // Jika antrean kosong tetapi sistem normal (misal saat awal booting)
        Serial.println("[LOG Serial] Menunggu data sensor... | Interrupt: NON-AKTIF");
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(2000)); // Cetak ke monitor setiap 2 detik
  }
}