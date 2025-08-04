#define VERSION "\n=== ESP32-S3 Voice Assistant with SD_MMC (Modified from KALO) ======================"

#include <Arduino.h>
#include <WiFi.h>
#include <SD_MMC.h>
#include "audio_recording.h"
#include "deepgram_transcription.h"
#include "gemini_ai.h"
#include "voice_assistant_display.h"

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

  // Initialize TFT Display
  Display_Init();

  // Connecting to WLAN
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting WLAN ");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    Display_Update(); // Keep display animations running
    delay(500);
  }
  Serial.println(". Done, device connected.");

  // Initialize SD_MMC card with custom pins
  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_DATA);
  if (!SD_MMC.begin("/sdcard", true)) {  // true = 1-bit mode
    Serial.println("ERROR - SD_MMC Card initialization failed!");
    Serial.println("Make sure SD card is inserted and wired correctly");
    Serial.printf("SD_MMC Pins: CLK=%d, CMD=%d, DATA=%d\n", SD_MMC_CLK, SD_MMC_CMD, SD_MMC_DATA);
    Display_ShowError("SD Card failed! Check wiring and card insertion");
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
  
  // Show main interface on display
  Display_ShowMainInterface();
}

// ------------------------------------------------------------------------------------------------------------------------------
void loop() {
  static bool was_recording = false;  // Track recording state
  
  // Update display animations
  Display_Update();
  
  if (digitalRead(pin_RECORD_BTN) == LOW)  // Recording started (ongoing)
  {
    if (!was_recording) {
      Serial.println("🎙️ Recording...");
      Display_ShowListening();  // Show listening animation
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
      Display_ShowProcessing();  // Show processing animation
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
          
          // Show transcription on display with typewriter effect
          Display_ShowTranscription(transcription);
          
          // Process transcription through Gemini AI
          Serial.println("🤖 Asking Gemini AI...");
          Display_ShowAIThinking();  // Show AI thinking animation
          unsigned long ai_start_time = millis();
          
          String ai_response = Gemini_ProcessText(transcription);
          
          unsigned long ai_processing_time = millis() - ai_start_time;
          
          Serial.println("🧠 AI RESPONSE: " + ai_response);
          Serial.printf("🤖 AI processing time: %lu ms (%.1fs)\n", ai_processing_time, ai_processing_time / 1000.0);
          
          // Show AI response on display with typewriter effect
          if (ai_response.indexOf("Error") != -1 || ai_response.indexOf("failed") != -1) {
            Display_ShowError(ai_response);
            delay(3000);
            Display_ShowMainInterface();
          } else {
            Display_ShowAIResponse(ai_response);
            // Keep showing the complete conversation for 8 seconds
            delay(8000);
            Display_ShowMainInterface();
          }
          
        } else {
          Serial.println("⚠️ No transcript received");
          Serial.printf("⏱️ Processing time: %lu ms (%.1fs)\n", processing_time, processing_time / 1000.0);
          Display_ShowError("No speech detected. Try speaking louder or closer to microphone.");
          delay(3000);
          Display_ShowMainInterface();
        }
      } else {
        Serial.println("⚠️ Recording too short (< 0.4s)");
        Display_ShowError("Recording too short. Hold button longer while speaking.");
        delay(2000);
        Display_ShowMainInterface();
      }
    } else {
      // When not recording and no new audio available, maintain current display state
      Display_Update();
    }

    // Deepgram Keep Alive (re-connecting when needed) 
    Deepgram_KeepAlive();
    
    // Gemini AI Keep Alive (re-connecting when needed)
    Gemini_KeepAlive();
  }

  delay(10);
}