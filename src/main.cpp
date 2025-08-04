#define VERSION "\n=== ESP32-S3 Voice Assistant with SD_MMC (Modified from KALO) ======================"

#include <Arduino.h>
#include <WiFi.h>
#include <SD_MMC.h>
#include "audio_recording.h"
#include "deepgram_transcription.h"

// Custom SD_MMC pin definitions
#define SD_MMC_CMD  35   // io35
#define SD_MMC_CLK  36   // io36  
#define SD_MMC_DATA 37   // io37

#define AUDIO_FILE        "/Audio.wav"    // mandatory, filename for the AUDIO recording

// --- PRIVATE credentials -----
const char* ssid = "Explore";         // ## INSERT your wlan ssid
const char* password = "Explore.us";  // ## INSERT your password

// --- PIN assignments ---------
#define pin_RECORD_BTN 4  // GPIO4 for record button with pullup

// ------------------------------------------------------------------------------------------------------------------------------
void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.setTimeout(100);  // 10 times faster reaction after CR entered (default is 1000ms)

  // Pin assignments:
  pinMode(pin_RECORD_BTN, INPUT_PULLUP);  // GPIO4 button with built-in pullup

  // Hello World
  Serial.println(VERSION);

  // Connecting to WLAN
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting WLAN ");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println(". Done, device connected.");

  // Initialize SD_MMC card with custom pins
  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_DATA);
  if (!SD_MMC.begin("/sdcard", true)) {  // true = 1-bit mode
    Serial.println("ERROR - SD_MMC Card initialization failed!");
    Serial.println("Make sure SD card is inserted and wired correctly");
    Serial.printf("SD_MMC Pins: CLK=%d, CMD=%d, DATA=%d\n", SD_MMC_CLK, SD_MMC_CMD, SD_MMC_DATA);
    while(1) {
      delay(1000);  // Stay in error loop
    }
  }

  Serial.println("✅ SD_MMC initialized successfully");

  // initialize I2S Recording Services (don't forget!)
  I2S_Record_Init();

  // INIT done, starting user interaction
  Serial.println("> HOLD button for recording AUDIO .. RELEASE button for Deepgram transcription");
  Serial.println("🚀 Ready to record!");
}

// ------------------------------------------------------------------------------------------------------------------------------
void loop() {
  static bool was_recording = false;  // Track recording state
  
  if (digitalRead(pin_RECORD_BTN) == LOW)  // Recording started (ongoing)
  {
    if (!was_recording) {
      Serial.println("🎙️ Recording...");
      was_recording = true;
    }
    
    delay(30);  // unbouncing & suppressing button 'click' noise in begin of audio recording

    //Start Recording
    Record_Start(AUDIO_FILE);
  }

  if (digitalRead(pin_RECORD_BTN) == HIGH)  // Recording not started yet .. OR stopped now (on release button)
  {
    if (was_recording) {
      Serial.println("🛑 Recording stopped");
      was_recording = false;
    }
    
    float recorded_seconds;
    if (Record_Available(AUDIO_FILE, &recorded_seconds))  //  true once when recording finalized (.wav file available)
    {
      if (recorded_seconds > 0.4)  // ignore short btn TOUCH (e.g. <0.4 secs)
      {
        Serial.println("🔄 Processing...");
        
        // Start timing the processing
        unsigned long start_time = millis();
        
        // [SpeechToText] - Transcript the Audio (waiting here until done)
        String transcription = SpeechToText_Deepgram(AUDIO_FILE);
        
        // Calculate processing time
        unsigned long processing_time = millis() - start_time;
        
        if (transcription.length() > 0) {
          Serial.println("🎉 SUCCESS: " + transcription);
          Serial.printf("⏱️ Processing time: %lu ms (%.1fs)\n", processing_time, processing_time / 1000.0);
        } else {
          Serial.println("⚠️ No transcript received");
          Serial.printf("⏱️ Processing time: %lu ms (%.1fs)\n", processing_time, processing_time / 1000.0);
        }
      } else {
        Serial.println("⚠️ Recording too short (< 0.4s)");
      }
    }

    // Deepgram Keep Alive (re-connecting when needed) 
    Deepgram_KeepAlive();
  }

  delay(10);
}