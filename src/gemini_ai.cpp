#include <Arduino.h>
#include <HTTPClient.h>   
#include <ArduinoJson.h>

// --- PRIVATE credentials & user preferences -----  
const char* geminiApiKey = "AIzaSyDGLIPiuzORNrIi-qyuW5N26ri9gUYo0DI";   // Replace with your Gemini API key
const char* geminiMaxTokens = "200";       

// ----------------------------------------------------------------------------------------------------------------------------

String Gemini_ProcessText(String input_text)
{ 
  if (input_text.length() == 0) {
    return "No text to process";
  }
  
  Serial.println("🤖 Asking Gemini AI...");
  
  HTTPClient https;
  
  // Begin HTTPS connection
  if (https.begin("https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key=" + String(geminiApiKey))) {
    
    https.addHeader("Content-Type", "application/json");
    
    // Prepare simple JSON payload
    String payload = String("{\"contents\": [{\"parts\":[{\"text\":\"" + input_text + "\"}]}],\"generationConfig\": {\"maxOutputTokens\": " + String(geminiMaxTokens) + "}}");
    
    // Send POST request
    int httpCode = https.POST(payload);
    
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      String response = https.getString();
      
      // Parse JSON response using ArduinoJson
      JsonDocument doc;
      deserializeJson(doc, response);
      
      String aiResponse = doc["candidates"][0]["content"]["parts"][0]["text"];
      
      // Clean up response - remove special characters but keep basic punctuation
      aiResponse.trim();
      String cleanResponse = "";
      for (size_t i = 0; i < aiResponse.length(); i++) {
        char c = aiResponse[i];
        if (isalnum(c) || isspace(c) || c == '.' || c == ',' || c == '!' || c == '?') {
          cleanResponse += c;
        } else {
          cleanResponse += ' ';
        }
      }
      
      https.end();
      return cleanResponse.length() > 0 ? cleanResponse : "No response from AI";
      
    } else {
      Serial.printf("❌ Gemini API failed, error: %s\n", https.errorToString(httpCode).c_str());
      https.end();
      return "API request failed";
    }
    
  } else {
    Serial.println("❌ Unable to connect to Gemini API");
    return "Connection failed";
  }
}
