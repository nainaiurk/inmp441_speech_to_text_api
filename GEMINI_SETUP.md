# Gemini AI Integration Setup

## Getting Your Gemini API Key

1. Go to [Google AI Studio](https://makersuite.google.com/app/apikey)
2. Sign in with your Google account
3. Click "Create API Key"
4. Copy the generated API key

## Configuration

1. Open `src/gemini_ai.cpp`
2. Find the line: `const char* geminiApiKey = "YOUR_GEMINI_API_KEY_HERE";`
3. Replace `YOUR_GEMINI_API_KEY_HERE` with your actual API key
4. Example: `const char* geminiApiKey = "AIzaSyC1234567890abcdefghijklmnopqrstuvwxyz";`

## Features

- Processes speech-to-text transcriptions through Gemini AI
- Gets intelligent AI responses to user voice input
- Shows processing times for both speech recognition and AI processing
- Handles errors and connection issues gracefully

## Output Format

```
🎉 SUCCESS: Hello, how are you today?
⏱️ Processing time: 4200 ms (4.2s)
🤖 Asking Gemini AI...
🧠 AI RESPONSE: Hello! I'm doing well, thank you for asking. How are you doing today?
🤖 AI processing time: 2100 ms (2.1s)
```

## Troubleshooting

- Make sure your WiFi connection is stable
- Verify your API key is correct and has proper permissions
- Check that you haven't exceeded your API quota
- Gemini AI responses are limited to 200 tokens for optimal performance
