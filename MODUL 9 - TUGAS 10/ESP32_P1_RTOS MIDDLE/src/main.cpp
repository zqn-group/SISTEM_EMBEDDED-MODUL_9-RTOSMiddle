#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

// Definisi Pin Hardware
#define DHTPIN 4
#define DHTTYPE DHT22
#define SD_CS 5

// Inisialisasi Perangkat
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// Struktur Data untuk Antrean (Queue) FreeRTOS
struct DataSensor {
  float suhu;
  float kelembaban;
};

// Handle Queue
QueueHandle_t dataQueue;
bool sdCardReady = false; 

// Prototipe Task FreeRTOS
void taskLCD(void *pvParameters);
void taskDHT(void *pvParameters);
void taskSDAndSerial(void *pvParameters);

// Fungsi untuk membaca dan menampilkan spesifikasi SD Card
void tampilkanSpesifikasiSD() {
  uint8_t cardType = SD.cardType();

  Serial.println("\n========== SPESIFIKASI SD CARD ==========");
  
  // 1. Cek Tipe Kartu
  Serial.print("Tipe SD Card: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC (Standard Capacity)");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC (High Capacity)");
  } else {
    Serial.println("UNKNOWN (Tidak Dikenali)");
  }

  // 2. Cek Ukuran/Kapasitas Total & Sisa Memori
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  uint64_t totalBytes = SD.totalBytes() / (1024 * 1024);
  uint64_t usedBytes = SD.usedBytes() / (1024 * 1024);

  Serial.printf("Kapasitas Card : %llu MB\n", cardSize);
  Serial.printf("Total Partisi  : %llu MB\n", totalBytes);
  Serial.printf("Memori Terpakai: %llu MB\n", usedBytes);
  Serial.printf("Sisa Memori    : %llu MB\n", (totalBytes - usedBytes));
  Serial.println("=========================================\n");
}

void setup() {
  Serial.begin(115200);
  
  // 1. Inisialisasi Sensor DHT22
  dht.begin();
  
  // 2. Inisialisasi LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Mulai Sistem...");
  lcd.setCursor(0, 1);
  lcd.print("Membaca Sensor..");

  // 3. Inisialisasi SD Card
  if(!SD.begin(SD_CS)){
    Serial.println("\n[HARDWARE ERROR] Gagal menginisialisasi SD Card!");
    Serial.println("[SOLUSI] Pastikan MicroSD sudah diformat ke FAT32 & kabel VCC terhubung ke 5V.\n");
    sdCardReady = false; 
  } else {
    Serial.println("[SUCCESS] SD Card Berhasil diinisialisasi.");
    sdCardReady = true;
    
    // Panggil fungsi untuk menampilkan spesifikasi kartu ke Serial Monitor
    tampilkanSpesifikasiSD();
    
    // Buat file baru dengan header CSV jika belum ada
    File file = SD.open("/data.csv", FILE_APPEND);
    if(file) {
      if(file.size() == 0) {
        file.println("Timestamp(ms),Suhu(C),Kelembaban(%)");
      }
      file.close();
    }
  }

  // Membuat Queue dengan kapasitas penyimpanan 5 elemen data
  dataQueue = xQueueCreate(5, sizeof(DataSensor));

  if (dataQueue != NULL) {
    // Task LCD - Prioritas Tinggi (3) - Berjalan di Core 1
    xTaskCreatePinnedToCore(taskLCD, "Task LCD", 2048, NULL, 3, NULL, 1);
    
    // Task DHT - Prioritas Sedang (2) - Berjalan di Core 1
    xTaskCreatePinnedToCore(taskDHT, "Task DHT", 2048, NULL, 2, NULL, 1);
    
    // Task SD & Serial - Prioritas Rendah (1) - Berjalan di Core 0
    xTaskCreatePinnedToCore(taskSDAndSerial, "Task SD Serial", 3072, NULL, 1, NULL, 0);
    
    Serial.println("Sistem FreeRTOS Siap.");
  } else {
    Serial.println("Gagal membuat Queue!");
  }
}

void loop() {
  vTaskDelete(NULL); 
}

/* ========================================================================
   1. TASK LCD (PRIORITAS TINGGI)
   Murni hanya menampilkan data real-time dari sensor DHT22 ke layar LCD
   ======================================================================== */
void taskLCD(void *pvParameters) {
  DataSensor dataLCD;
  for (;;) {
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
    vTaskDelay(pdMS_TO_TICKS(250)); 
  }
}

/* ========================================================================
   2. TASK DHT (PRIORITAS SEDANG)
   Membaca sensor DHT22 dan mengirimkan datanya ke dalam Queue
   ======================================================================== */
void taskDHT(void *pvParameters) {
  DataSensor dataBaru;
  for (;;) {
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
    } else {
      Serial.println("[ERROR] Gagal membaca sensor DHT22!");
    }
    vTaskDelay(pdMS_TO_TICKS(2000)); 
  }
}

/* ========================================================================
   3. TASK SD CARD & SERIAL MONITOR (PRIORITAS RENDAH)
   Menampilkan data ke Serial Monitor dan menyimpannya ke format data.csv
   ======================================================================== */
void taskSDAndSerial(void *pvParameters) {
  DataSensor dataLog;
  for (;;) {
    if (xQueueReceive(dataQueue, &dataLog, pdMS_TO_TICKS(200)) == pdPASS) {
      
      unsigned long waktuSekarang = millis();

      // 1. Tampilkan Log Data ke Serial Monitor
      Serial.print("[LOG Serial] Waktu: ");
      Serial.print(waktuSekarang);
      Serial.print(" ms | Suhu: ");
      Serial.print(dataLog.suhu, 1);
      Serial.print(" C | Kelembaban: ");
      Serial.print(dataLog.kelembaban, 1);
      Serial.println(" %");

      // 2. Tulis data ke SD Card
      if (sdCardReady) {
        File file = SD.open("/data.csv", FILE_APPEND);
        if (file) {
          file.print(waktuSekarang);
          file.print(",");
          file.print(dataLog.suhu, 1);
          file.print(",");
          file.println(dataLog.kelembaban, 1);
          file.close(); 
        } else {
          Serial.println("[WARNING] Gagal membuka file data.csv untuk menulis data!");
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(2500)); 
  }
}