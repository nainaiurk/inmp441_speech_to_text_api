// Configuration file for Deepgram Speech-to-Text
// Copy this file and update with your actual credentials

#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
// Replace with your WiFi network credentials
#define WIFI_SSID "Your_WiFi_Network_Name"
#define WIFI_PASSWORD "Your_WiFi_Password"

// Deepgram API Configuration
// Get your API key from: https://console.deepgram.com/
#define DEEPGRAM_API_KEY "Your_Deepgram_API_Key_Here"

// Optional: Deepgram model configuration
// Available models: nova-2, nova, whisper, etc.
#define DEEPGRAM_MODEL "nova-2"
#define DEEPGRAM_LANGUAGE "en"

// Recording Configuration
#define RECORDING_DURATION_MS 3000  // 3 seconds
#define AUTO_RECORDING false        // Set to true for automatic recording

#endif
