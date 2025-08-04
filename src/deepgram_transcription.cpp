#include <Arduino.h>
#include <WiFiClientSecure.h>   
#include <SD_MMC.h>
#include <ArduinoJson.h>

// --- defines & macros --------

#ifndef DEBUG
#  define DEBUG true            // <- define your preference here  
#  define DebugPrint(x);        if(DEBUG){Serial.print(x);}   
#  define DebugPrintln(x);      if(DEBUG){Serial.println(x);} 
#endif

// --- PRIVATE credentials & user favorites -----  

const char* deepgramApiKey = "85e33bfe537cc2e30ac77a649142dc60e7b3b89a";   // Deepgram API key

#define STT_LANGUAGE      "en"     // forcing single language: e.g. "de" (German), "en" (English)
                                   // keep EMPTY ("") if you want Deepgram to detect language automatically
                                   
#define TIMEOUT_DEEPGRAM   10      // Reduced timeout [sec] for faster responses when no speech detected     

#define STT_KEYWORDS            "&keywords=hello&keywords=world&keywords=ESP32"  // optional, forcing STT to listen exactly 

// --- global vars -------------
WiFiClientSecure client;       

// ----------------------------------------------------------------------------------------------------------------------------

String SpeechToText_Deepgram( String audio_filename )
{ 
  uint32_t t_start = millis(); 
  
  // ---------- Connect to Deepgram Server (always establish fresh connection for reliability)

  // Always establish a fresh connection to prevent socket reuse issues
  if (client.connected()) {
    client.stop();
  }
  
  client.setInsecure();
  client.setTimeout(5000);  // 5 second connection timeout for faster failure detection
  if (!client.connect("api.deepgram.com", 443)) {
    Serial.println("\nERROR - WiFiClientSecure connection to Deepgram Server failed!");
    client.stop();
    return ("");  
  }
  uint32_t t_connected = millis();  

  // ---------- Check if AUDIO file exists, check file size 
  
  File audioFile = SD_MMC.open( audio_filename );    
  if (!audioFile) {
    Serial.println("ERROR - Failed to open file for reading");
    return ("");
  }
  size_t audio_size = audioFile.size();
  audioFile.close();
  
  // ---------- flush (remove) potential inbound streaming data before we start with any new user request
  // reason: increasing reliability, also needed in case we have pending data from earlier Deepgram_KeepAlive()

  String socketcontent = "";
  while (client.available()) {
    char c = client.read(); 
    socketcontent += String(c);
  } 
  int RX_flush_len = socketcontent.length();
 
  // ---------- Sending HTTPS request header to Deepgram Server

  String optional_param;                          // see: https://developers.deepgram.com/docs/stt-streaming-feature-overview
  optional_param =  "?model=nova-2-general";      // Deepgram recommended model (high readability, lowest word error rates)
  optional_param += (STT_LANGUAGE != "") ? ("&language="+(String)STT_LANGUAGE) : ("&detect_language=true");  // see #defines  
  optional_param += "&smart_format=true";         // applies formatting (Punctuation, Paragraphs, upper/lower etc ..) 
  // Remove heavy processing options for speed
  
  client.println("POST /v1/listen" + optional_param + " HTTP/1.1"); 
  client.println("Host: api.deepgram.com");
  client.println("Authorization: Token " + String(deepgramApiKey));
  client.println("Content-Type: audio/wav");
  client.println("Content-Length: " + String(audio_size));
  client.println();   // header complete, now sending binary body (wav bytes) .. 
  
  uint32_t t_headersent = millis();     

  // ---------- Reading the AUDIO wav file, sending in CHUNKS (closing file after done)
  // idea found here (WiFiClientSecure.h issue): https://www.esp32.com/viewtopic.php?t=4675
  
  File file = SD_MMC.open( audio_filename, FILE_READ );
  const size_t bufferSize = 1024;      // Reduced buffer size to prevent memory issues
  uint8_t buffer[bufferSize];
  size_t bytesRead;
  size_t totalSent = 0;
  
  while (file.available()) {
    bytesRead = file.read(buffer, sizeof(buffer));
    if (bytesRead > 0) {
      size_t written = client.write(buffer, bytesRead);   // sending WAV AUDIO data
      if (written != bytesRead) {
        Serial.println("❌ ERROR: Upload write failed");
        file.close();
        return "";
      }
      
      totalSent += written;
      
      // No delay - upload as fast as possible for speed
    }
  }
  file.close();
  uint32_t t_wavbodysent = millis();  

  // ---------- Waiting (!) for Deepgram Server response (stop waiting latest after TIMEOUT_DEEPGRAM [secs])
 
  String response = "";   // waiting until available() true and all data completely received
  while ( response == "" && millis() < (t_wavbodysent + TIMEOUT_DEEPGRAM*1000) ) {   
    while (client.available()) {                         
      char c = client.read();
      response += String(c);      
    }
    // Faster polling - check every 10ms instead of 100ms for quicker response
    delay(10);           
  } 
  if (millis() >= (t_wavbodysent + TIMEOUT_DEEPGRAM*1000)) {
    Serial.println("*** TIMEOUT ERROR - forced TIMEOUT after " + (String) TIMEOUT_DEEPGRAM + " seconds ***");
  } 
  uint32_t t_response = millis();  

  // ---------- closing connection to Deepgram 
  // Force proper socket cleanup to prevent "Bad file number" errors
  if (client.connected()) {
    client.flush();     // Ensure all data is sent
    delay(10);          // Small delay for proper cleanup
    client.stop();      // Close the connection
  }
  
  // Clear the client completely to prevent socket reuse issues
  client = WiFiClientSecure();  // Reset the client object
                     
  // ---------- Parsing json response, extracting transcription etc.
  
  int response_len = response.length();
  
  // Find the start of JSON response (skip HTTP headers)
  int jsonStart = response.indexOf("{");
  if (jsonStart < 0) {
    Serial.println("ERROR - No JSON found in response");
    return "";
  }
  
  String jsonResponse = response.substring(jsonStart);
  
  // Use fast string parsing instead of heavy JSON parsing for better performance
  // First check for errors
  if (jsonResponse.indexOf("\"err_code\"") != -1) {
    if (jsonResponse.indexOf("SLOW_UPLOAD") != -1) {
      return "Upload too slow - try shorter recording";
    }
    return "Deepgram error occurred";
  }
  
  // Extract transcript directly using string parsing - much faster!
  int transcriptStart = jsonResponse.indexOf("\"transcript\":\"");
  if (transcriptStart != -1) {
    transcriptStart += 14; // Move past "transcript":"
    int transcriptEnd = jsonResponse.indexOf("\"", transcriptStart);
    if (transcriptEnd != -1) {
      String transcript = jsonResponse.substring(transcriptStart, transcriptEnd);
      if (transcript.length() > 0) {
        return transcript;
      }
    }
  }
  
  return "No speech detected";
}

// ----------------------------------------------------------------------------------------------------------------------------

void Deepgram_KeepAlive()
{
  // This function maintains connection to Deepgram (called periodically)
  // Ensure proper cleanup to prevent socket errors
  if (client.connected()) {
    // Check if there's any stray data and flush it
    while (client.available()) {
      client.read();
    }
    // Properly close any lingering connections
    client.stop();
  }
}
