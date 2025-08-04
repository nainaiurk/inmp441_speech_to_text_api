// ------------------------------------------------------------------------------------------------------------------------------
// ----------------           Voice Assistant Display Library - TFT GUI Implementation                         ---------------- 
// ----------------                     Cool animated interface for ESP32-S3 Voice Assistant                   ----------------
// ------------------------------------------------------------------------------------------------------------------------------

#include "voice_assistant_display.h"

// TFT pin definitions (using the pins you specified)
#define TFT_CS   10  // Chip Select
#define TFT_RST  9   // Reset
#define TFT_DC   3   // Data/Command
#define TFT_MOSI 11  // DIN (Data In)
#define TFT_SCLK 12  // CLK (Clock)

// Create display object
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Animation variables
int animationFrame = 0;
unsigned long lastAnimationUpdate = 0;
DisplayState currentState = STATE_INIT;
String currentTranscription = "";
String currentAIResponse = "";
int scrollOffset = 0;
int typewriterIndex = 0;
unsigned long lastScrollUpdate = 0;
unsigned long lastTypewriterUpdate = 0;

// Display dimensions
#define DISPLAY_WIDTH  128
#define DISPLAY_HEIGHT 160

// Modern layout sections (Gemini-style) - Increased heights for better text display
#define AI_SECTION_Y      5
#define AI_SECTION_HEIGHT 70
#define TRANSCRIPTION_Y   80
#define TRANSCRIPTION_HEIGHT 60
#define STATUS_Y          145
#define STATUS_HEIGHT     15

void Display_Init() {
  Serial.println("🖥️ Initializing TFT Display...");
  
  // Initialize TFT
  tft.initR(INITR_BLACKTAB);  // ST7735 Black Tab variant
  tft.setRotation(2);         // Rotate 180 degrees (was 0)
  tft.fillScreen(COLOR_BLACK);
  
  // Set text properties
  tft.setTextWrap(true);
  tft.setTextSize(1);
  
  Serial.println("✅ TFT Display initialized successfully");
  
  // Show welcome screen
  Display_ShowWelcome();
}

void Display_ShowWelcome() {
  currentState = STATE_INIT;
  
  // Dynamic gradient background
  for(int y = 0; y < DISPLAY_HEIGHT; y++) {
    uint16_t color = tft.color565(10 + y/12, 5 + y/20, 30 + y/6);
    tft.drawFastHLine(0, y, DISPLAY_WIDTH, color);
  }
  
  // Stylish title
  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(2);
  tft.setCursor(30, 25);
  tft.println("VOICE");
  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(15, 50);
  tft.println("ASSISTANT");
  
  // Cool AI brain icon
  Display_DrawStylishAIIcon(DISPLAY_WIDTH/2 - 15, 80, COLOR_CYAN);
  
  // Version with style
  tft.setTextSize(1);
  tft.setTextColor(COLOR_LIGHT_GRAY);
  tft.setCursor(30, 120);
  tft.println("Powered by Gemini");
  
  // Animated loading bar
  tft.drawRoundRect(15, 140, DISPLAY_WIDTH - 30, 10, 5, COLOR_WHITE);
  for(int i = 0; i < DISPLAY_WIDTH - 34; i += 2) {
    tft.fillRoundRect(17, 142, i, 6, 3, COLOR_PURPLE);
    delay(20);
  }
  
  delay(1000);
  Display_ShowMainInterface();
}

void Display_ShowMainInterface() {
  // Cool dark background with subtle pattern
  tft.fillScreen(COLOR_BLACK);
  
  // Add some style dots in corners
  for(int i = 0; i < 8; i++) {
    uint16_t dotColor = tft.color565(20, 20, 40);
    tft.fillCircle(10 + i*15, 5, 1, dotColor);
    tft.fillCircle(10 + i*15, DISPLAY_HEIGHT-5, 1, dotColor);
  }
  
  // Main title with glow effect
  tft.setTextColor(COLOR_PURPLE);
  tft.setTextSize(2);
  tft.setCursor(27, 30);
  tft.println("ASK ME");
  tft.setTextColor(COLOR_CYAN);
  tft.setCursor(15, 55);
  tft.println("ANYTHING");
  
  // Cool microphone with pulsing ring
  Display_DrawStylishMicrophone(DISPLAY_WIDTH/2 - 15, 85, COLOR_WHITE);
  
  // Animated instruction text
  tft.setTextSize(1);
  tft.setTextColor(COLOR_LIGHT_GRAY);
  tft.setCursor(25, 125);
  tft.println("Hold to speak");
  
  // Add some decorative elements
  tft.drawCircle(20, 20, 3, COLOR_DARK_GRAY);
  tft.drawCircle(DISPLAY_WIDTH-20, 20, 3, COLOR_DARK_GRAY);
  tft.drawCircle(20, DISPLAY_HEIGHT-20, 3, COLOR_DARK_GRAY);
  tft.drawCircle(DISPLAY_WIDTH-20, DISPLAY_HEIGHT-20, 3, COLOR_DARK_GRAY);
  
  Display_ShowWaiting();
}

void Display_ShowWaiting() {
  if(currentState != STATE_WAITING) {
    currentState = STATE_WAITING;
  }
  
  // Animated pulsing effects
  if(millis() - lastAnimationUpdate > 80) {
    // Pulsing microphone ring
    int pulse = 128 + 127 * sin(animationFrame * 0.1);
    uint16_t ringColor = tft.color565(pulse/4, pulse/4, pulse/2);
    
    // Clear previous ring
    tft.drawCircle(DISPLAY_WIDTH/2, 100, 25, COLOR_BLACK);
    tft.drawCircle(DISPLAY_WIDTH/2, 100, 26, COLOR_BLACK);
    
    // Draw new pulsing ring
    tft.drawCircle(DISPLAY_WIDTH/2, 100, 25, ringColor);
    
    // Floating particles animation
    for(int i = 0; i < 6; i++) {
      int x = DISPLAY_WIDTH/2 + 35 * cos(animationFrame * 0.05 + i * PI/3);
      int y = 100 + 35 * sin(animationFrame * 0.05 + i * PI/3);
      if(x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT) {
        tft.fillCircle(x, y, 1, COLOR_CYAN);
      }
    }
    
    animationFrame++;
    lastAnimationUpdate = millis();
  }
}

void Display_ShowListening() {
  if(currentState != STATE_LISTENING) {
    currentState = STATE_LISTENING;
    animationFrame = 0;
    
    // Create listening page
    tft.fillScreen(COLOR_BLACK);
    
    // Header
    tft.setTextColor(COLOR_RED);
    tft.setTextSize(1);
    tft.setCursor(5, 5);
    tft.println("LISTENING...");
    
    // Decorative line
    tft.drawFastHLine(5, 18, DISPLAY_WIDTH-10, COLOR_RED);
    
    // Large microphone in center
    Display_DrawStylishMicrophone(DISPLAY_WIDTH/2 - 15, 40, COLOR_RED);
  }
  
  // Dynamic waveform animation
  if(millis() - lastAnimationUpdate > 50) {
    // Clear waveform area
    tft.fillRect(0, 90, DISPLAY_WIDTH, 50, COLOR_BLACK);
    
    // Draw animated sound waveform
    for(int x = 5; x < DISPLAY_WIDTH-5; x += 3) {
      int height = 5 + 15 * sin((x + animationFrame * 4) * 0.2) * cos((x + animationFrame * 2) * 0.1);
      int y = 115 - height/2;
      uint16_t waveColor = tft.color565(255, 100 - height*3, 100 - height*3);
      tft.drawLine(x, y, x, y + height, waveColor);
    }
    
    // Recording indicator with pulse
    int pulse = 128 + 127 * sin(animationFrame * 0.2);
    tft.fillCircle(15, 25, 3, tft.color565(pulse, 0, 0));
    
    animationFrame++;
    lastAnimationUpdate = millis();
  }
}

void Display_ShowProcessing() {
  if(currentState != STATE_PROCESSING) {
    currentState = STATE_PROCESSING;
    animationFrame = 0;
    
    // Keep the same listening page background but change header
    tft.fillRect(0, 0, DISPLAY_WIDTH, 25, COLOR_BLACK);
    
    // New header for processing
    tft.setTextColor(COLOR_YELLOW);
    tft.setTextSize(1);
    tft.setCursor(5, 5);
    tft.println("PROCESSING...");
    
    // Decorative line
    tft.drawFastHLine(5, 18, DISPLAY_WIDTH-10, COLOR_YELLOW);
    
    // Replace microphone with AI icon
    tft.fillRect(DISPLAY_WIDTH/2 - 20, 35, 40, 40, COLOR_BLACK);
    Display_DrawStylishAIIcon(DISPLAY_WIDTH/2 - 15, 45, COLOR_CYAN);
  }
  
  // Cool buffering animation around AI icon
  if(millis() - lastAnimationUpdate > 80) {
    // Clear animation area around AI icon
    tft.fillRect(0, 90, DISPLAY_WIDTH, 50, COLOR_BLACK);
    
    // Rotating dots around AI icon
    int centerX = DISPLAY_WIDTH/2;
    int centerY = 60;
    int radius = 30;
    
    for(int i = 0; i < 8; i++) {
      float angle = (animationFrame * 0.3 + i * 45) * PI / 180;
      int x = centerX + radius * cos(angle);
      int y = centerY + radius * sin(angle);
      
      int brightness = 255 - i * 25;
      uint16_t dotColor = tft.color565(brightness/2, brightness/3, brightness);
      tft.fillCircle(x, y, 2, dotColor);
    }
    
    // Pulsing progress dots at bottom
    for(int i = 0; i < 5; i++) {
      int brightness = 100 + 155 * sin(animationFrame * 0.25 + i * 1.2);
      uint16_t color = tft.color565(brightness, brightness/2, 0);
      tft.fillCircle(20 + i * 20, 115, 2, color);
    }
    
    // Status text
    tft.setTextColor(COLOR_WHITE);
    tft.setTextSize(1);
    tft.setCursor(25, 130);
    tft.println("Processing...");
    
    animationFrame++;
    lastAnimationUpdate = millis();
  }
}

void Display_ShowTranscription(String text) {
  currentState = STATE_DISPLAYING_TEXT;
  currentTranscription = text;
  
  Serial.println("🗣️ Displaying transcription: " + text);
  
  // Create a dedicated speech page
  tft.fillScreen(COLOR_BLACK);
  
  // Header with icon
  tft.setTextColor(COLOR_GREEN);
  tft.setTextSize(1);
  tft.setCursor(5, 5);
  tft.println("YOUR SPEECH:");
  
  // Draw speech icon
  Display_DrawSpeechIcon(DISPLAY_WIDTH - 20, 5, COLOR_GREEN);
  
  // Decorative line
  tft.drawFastHLine(5, 18, DISPLAY_WIDTH-10, COLOR_GREEN);
  
  // Display the full text immediately
  tft.setTextColor(COLOR_WHITE);
  tft.setTextSize(1);
  
  // Clear text area first
  tft.fillRect(0, 25, DISPLAY_WIDTH, DISPLAY_HEIGHT-30, COLOR_BLACK);
  
  // Display text directly with word wrapping
  int y = 25;
  int x = 5;
  int lineHeight = 9;
  int charWidth = 6;
  int maxChars = (DISPLAY_WIDTH - 10) / charWidth;
  
  Serial.println("📏 Display area: " + String(DISPLAY_WIDTH) + "x" + String(DISPLAY_HEIGHT));
  Serial.println("📏 Max chars per line: " + String(maxChars));
  Serial.println("📏 Text length: " + String(text.length()));
  
  for(int i = 0; i < text.length(); i++) {
    char c = text[i];
    
    if(c == ' ' && (i % maxChars) > (maxChars - 5)) {
      // Start new line for word wrap
      y += lineHeight;
      x = 5;
      if(y > DISPLAY_HEIGHT - 15) break;
      continue;
    }
    
    if((i % maxChars) == 0 && i > 0) {
      // Force new line after max chars
      y += lineHeight;
      x = 5;
      if(y > DISPLAY_HEIGHT - 15) break;
    }
    
    tft.setCursor(x, y);
    tft.print(c);
    x += charWidth;
  }
  
  Serial.println("✅ Transcription displayed at position y=" + String(y));
}

void Display_ShowAIThinking() {
  if(currentState != STATE_AI_THINKING) {
    currentState = STATE_AI_THINKING;
    animationFrame = 0;
    
    // Update header smoothly without clearing entire screen
    tft.fillRect(0, 0, DISPLAY_WIDTH, 25, COLOR_BLACK);
    
    // Header
    tft.setTextColor(COLOR_PURPLE);
    tft.setTextSize(1);
    tft.setCursor(5, 5);
    tft.println("AI THINKING...");
    
    // Decorative line
    tft.drawFastHLine(5, 18, DISPLAY_WIDTH-10, COLOR_PURPLE);
  }
  
  // Enhanced thinking animation with neural network
  if(millis() - lastAnimationUpdate > 100) {
    // Clear animation area
    tft.fillRect(0, 90, DISPLAY_WIDTH, 50, COLOR_BLACK);
    
    // Neural network connections around AI brain
    int centerX = DISPLAY_WIDTH/2;
    int centerY = 60;
    
    // Draw pulsing neural connections
    for(int i = 0; i < 6; i++) {
      float angle = (animationFrame * 0.15 + i * 60) * PI / 180;
      int brightness = 100 + 155 * sin(animationFrame * 0.2 + i * 1.0);
      uint16_t color = tft.color565(brightness/3, brightness/5, brightness);
      
      int x1 = centerX + 25 * cos(angle);
      int y1 = centerY + 25 * sin(angle);
      int x2 = centerX + 35 * cos(angle);
      int y2 = centerY + 35 * sin(angle);
      
      tft.drawLine(x1, y1, x2, y2, color);
      tft.fillCircle(x2, y2, 2, color);
    }
    
    // Thinking wave patterns
    for(int x = 5; x < DISPLAY_WIDTH-5; x += 4) {
      int wave = 3 + 8 * sin((x + animationFrame * 2) * 0.15);
      int y = 115 + wave;
      uint16_t waveColor = tft.color565(100, 50, 150 + wave*5);
      tft.fillCircle(x, y, 1, waveColor);
    }
    
    // Status text with pulsing
    int textBrightness = 150 + 105 * sin(animationFrame * 0.25);
    uint16_t textColor = tft.color565(textBrightness/2, textBrightness/3, textBrightness);
    tft.setTextColor(textColor);
    tft.setTextSize(1);
    tft.setCursor(25, 135);
    tft.println("Thinking...");
    
    animationFrame++;
    lastAnimationUpdate = millis();
  }
}

void Display_ShowAIResponse(String response) {
  currentState = STATE_AI_RESPONSE;
  currentAIResponse = response;
  
  Serial.println("🤖 Displaying AI response: " + response);
  
  // Create a dedicated AI response page
  tft.fillScreen(COLOR_BLACK);
  
  // Header with AI icon
  tft.setTextColor(COLOR_CYAN);
  tft.setTextSize(1);
  tft.setCursor(5, 5);
  tft.println("AI RESPONSE:");
  
  // Draw AI brain icon
  Display_DrawSmartAIIcon(DISPLAY_WIDTH - 20, 5, COLOR_CYAN);
  
  // Decorative line
  tft.drawFastHLine(5, 18, DISPLAY_WIDTH-10, COLOR_CYAN);
  
  // Display the full response immediately
  tft.setTextColor(COLOR_CYAN);
  tft.setTextSize(1);
  
  // Clear text area first
  tft.fillRect(0, 25, DISPLAY_WIDTH, DISPLAY_HEIGHT-30, COLOR_BLACK);
  
  // Display text directly with word wrapping
  int y = 25;
  int x = 5;
  int lineHeight = 9;
  int charWidth = 6;
  int maxChars = (DISPLAY_WIDTH - 10) / charWidth;
  
  Serial.println("📏 AI Display area: " + String(DISPLAY_WIDTH) + "x" + String(DISPLAY_HEIGHT));
  Serial.println("📏 AI Max chars per line: " + String(maxChars));
  Serial.println("📏 AI Text length: " + String(response.length()));
  
  for(int i = 0; i < response.length(); i++) {
    char c = response[i];
    
    if(c == ' ' && (i % maxChars) > (maxChars - 5)) {
      // Start new line for word wrap
      y += lineHeight;
      x = 5;
      if(y > DISPLAY_HEIGHT - 15) break;
      continue;
    }
    
    if((i % maxChars) == 0 && i > 0) {
      // Force new line after max chars
      y += lineHeight;
      x = 5;
      if(y > DISPLAY_HEIGHT - 15) break;
    }
    
    tft.setCursor(x, y);
    tft.print(c);
    x += charWidth;
  }
  
  Serial.println("✅ AI response displayed at position y=" + String(y));
}

void Display_ShowError(String error) {
  currentState = STATE_ERROR;
  tft.fillScreen(COLOR_BLACK);
  
  // Header
  tft.setTextColor(COLOR_RED);
  tft.setTextSize(1);
  tft.setCursor(10, 10);
  tft.println("❌ ERROR");
  
  // Error message
  tft.setTextColor(COLOR_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(10, 40);
  
  // Word wrap for error message
  int lineHeight = 10;
  int y = 40;
  String words[20];
  int wordCount = 0;
  
  // Simple word splitting
  int start = 0;
  for(int i = 0; i <= error.length(); i++) {
    if(i == error.length() || error[i] == ' ') {
      words[wordCount++] = error.substring(start, i);
      start = i + 1;
    }
  }
  
  String line = "";
  for(int i = 0; i < wordCount; i++) {
    if((line + words[i]).length() * 6 > DISPLAY_WIDTH - 20) {
      tft.setCursor(10, y);
      tft.println(line);
      y += lineHeight;
      line = words[i] + " ";
    } else {
      line += words[i] + " ";
    }
  }
  if(line.length() > 0) {
    tft.setCursor(10, y);
    tft.println(line);
  }
}

void Display_Update() {
  // This function can be called regularly to update animations
  // Currently animations are handled within each show function
}

void Display_DrawMicrophone(int x, int y, uint16_t color) {
  // Draw microphone icon
  tft.fillRect(x + 5, y, 10, 15, color);        // Main body
  tft.drawRect(x + 3, y + 15, 14, 8, color);    // Base
  tft.drawLine(x + 10, y + 23, x + 10, y + 28, color); // Stand
  tft.drawLine(x + 5, y + 28, x + 15, y + 28, color);  // Base line
  
  // Microphone grille lines
  for(int i = 1; i < 4; i++) {
    tft.drawLine(x + 6, y + i * 3, x + 14, y + i * 3, COLOR_BLACK);
  }
}

void Display_DrawWaveform(int frame) {
  // Draw animated sound waveform
  for(int x = 0; x < DISPLAY_WIDTH; x += 4) {
    int height = 10 + 8 * sin((x + frame * 4) * 0.1) * cos((x + frame * 2) * 0.05);
    int y = 60 - height/2;
    tft.drawLine(x, y, x, y + height, COLOR_GREEN);
  }
}

void Display_DrawSpinner(int frame) {
  // Draw spinning loading animation
  int centerX = DISPLAY_WIDTH/2;
  int centerY = 75;
  int radius = 20;
  
  for(int i = 0; i < 8; i++) {
    float angle = (frame * 0.3 + i * 45) * PI / 180;
    int x1 = centerX + (radius - 5) * cos(angle);
    int y1 = centerY + (radius - 5) * sin(angle);
    int x2 = centerX + radius * cos(angle);
    int y2 = centerY + radius * sin(angle);
    
    uint16_t color = tft.color565(255 - i * 32, 255 - i * 32, 0);
    tft.drawLine(x1, y1, x2, y2, color);
  }
}

void Display_DrawAIIcon(int x, int y, uint16_t color) {
  // Draw simple AI brain icon
  tft.drawCircle(x + 8, y + 8, 8, color);       // Head
  tft.fillCircle(x + 5, y + 6, 1, color);       // Left eye
  tft.fillCircle(x + 11, y + 6, 1, color);      // Right eye
  tft.drawLine(x + 4, y + 10, x + 12, y + 10, color); // Mouth
  
  // Circuit lines for AI effect
  tft.drawLine(x, y + 4, x + 3, y + 4, color);
  tft.drawLine(x + 13, y + 4, x + 16, y + 4, color);
  tft.drawLine(x + 8, y - 2, x + 8, y + 1, color);
}

void Display_ScrollText(String text, int y, uint16_t color, int maxWidth) {
  tft.setTextColor(color);
  tft.setTextSize(1);
  
  // Calculate text width
  int textWidth = text.length() * 6; // 6 pixels per character for size 1
  
  if(textWidth <= maxWidth) {
    // Text fits, just display it
    tft.setCursor(8, y);
    tft.println(text);
  } else {
    // Text needs scrolling
    int maxChars = maxWidth / 6;
    int lines = (textWidth / maxWidth) + 1;
    
    // Display multiple lines
    for(int line = 0; line < lines && line < 6; line++) {
      int startChar = line * maxChars;
      int endChar = min(startChar + maxChars, (int)text.length());
      String lineText = text.substring(startChar, endChar);
      
      tft.setCursor(8, y + line * 10);
      tft.println(lineText);
    }
  }
}

void Display_DrawTextInBox(String text, int startY, int maxHeight, uint16_t color) {
  tft.setTextColor(color);
  tft.setTextSize(1);
  
  int maxCharsPerLine = (DISPLAY_WIDTH - 10) / 6; // Fixed calculation
  int currentY = startY;
  int lineHeight = 9;
  
  // Simple character-based display
  int textIndex = 0;
  while(textIndex < text.length() && (currentY + lineHeight) < (startY + maxHeight)) {
    String line = "";
    
    // Build line character by character
    for(int i = 0; i < maxCharsPerLine && textIndex < text.length(); i++) {
      char c = text[textIndex];
      line += c;
      textIndex++;
      
      if(c == '\n') break;
    }
    
    // Draw the line
    tft.setCursor(5, currentY);
    tft.println(line);
    currentY += lineHeight;
  }
}

// Modern text rendering for Gemini-style interface
void Display_DrawModernText(String text, int startY, int maxHeight, uint16_t color) {
  tft.setTextColor(color);
  tft.setTextSize(1);
  
  int maxCharsPerLine = (DISPLAY_WIDTH - 10) / 6; // Fixed character calculation
  int currentY = startY;
  int lineHeight = 9; // Compact line spacing for more text
  
  // Simple character-based wrapping for reliable display
  int textIndex = 0;
  while(textIndex < text.length() && (currentY + lineHeight) < (startY + maxHeight)) {
    String line = "";
    
    // Build line character by character
    for(int i = 0; i < maxCharsPerLine && textIndex < text.length(); i++) {
      char c = text[textIndex];
      line += c;
      textIndex++;
      
      if(c == '\n') break; // Force new line on newline character
    }
    
    // Draw the line
    tft.setCursor(5, currentY);
    tft.println(line);
    currentY += lineHeight;
  }
}

// Modern microphone icon
void Display_DrawModernMicrophone(int x, int y, uint16_t color) {
  // Sleek rounded microphone
  tft.fillRoundRect(x + 3, y, 10, 12, 3, color);
  tft.drawRoundRect(x + 1, y + 12, 14, 6, 2, color);
  tft.drawLine(x + 8, y + 18, x + 8, y + 22, color);
  tft.drawLine(x + 4, y + 22, x + 12, y + 22, color);
}

// Modern AI icon with glow effect
void Display_DrawModernAIIcon(int x, int y, uint16_t color) {
  // Modern AI brain with circuits
  tft.drawCircle(x + 20, y + 15, 15, color);
  tft.fillCircle(x + 15, y + 12, 2, color);
  tft.fillCircle(x + 25, y + 12, 2, color);
  
  // Neural network lines
  tft.drawLine(x + 5, y + 10, x + 15, y + 15, color);
  tft.drawLine(x + 35, y + 10, x + 25, y + 15, color);
  tft.drawLine(x + 20, y + 5, x + 20, y + 10, color);
  tft.drawLine(x + 20, y + 20, x + 20, y + 25, color);
  
  // Connection nodes
  tft.fillCircle(x + 5, y + 10, 1, color);
  tft.fillCircle(x + 35, y + 10, 1, color);
  tft.fillCircle(x + 20, y + 5, 1, color);
  tft.fillCircle(x + 20, y + 25, 1, color);
}

// Stylish AI brain icon for welcome screen
void Display_DrawStylishAIIcon(int x, int y, uint16_t color) {
  // Main brain outline
  tft.drawCircle(x + 15, y + 12, 12, color);
  
  // Brain hemispheres
  tft.drawLine(x + 15, y + 2, x + 15, y + 22, color);
  
  // Neural pathways
  tft.drawLine(x + 8, y + 8, x + 22, y + 16, color);
  tft.drawLine(x + 8, y + 16, x + 22, y + 8, color);
  
  // Synapses
  tft.fillCircle(x + 8, y + 8, 1, color);
  tft.fillCircle(x + 22, y + 8, 1, color);
  tft.fillCircle(x + 8, y + 16, 1, color);
  tft.fillCircle(x + 22, y + 16, 1, color);
  tft.fillCircle(x + 15, y + 12, 1, color);
}

// Stylish microphone with ring effects
void Display_DrawStylishMicrophone(int x, int y, uint16_t color) {
  // Main microphone body with rounded design
  tft.fillRoundRect(x + 10, y + 5, 10, 15, 5, color);
  
  // Microphone grille
  for(int i = 0; i < 3; i++) {
    tft.drawFastHLine(x + 12, y + 8 + i * 3, 6, COLOR_BLACK);
  }
  
  // Stand and base
  tft.drawRoundRect(x + 5, y + 20, 20, 8, 4, color);
  tft.drawLine(x + 15, y + 28, x + 15, y + 32, color);
  tft.drawFastHLine(x + 10, y + 32, 10, color);
  
  // Signal waves
  tft.drawCircle(x + 15, y + 12, 18, tft.color565(color >> 11, (color >> 5) & 0x3F, color & 0x1F));
  tft.drawCircle(x + 15, y + 12, 22, tft.color565((color >> 11)/2, ((color >> 5) & 0x3F)/2, (color & 0x1F)/2));
}

// Speech bubble icon
void Display_DrawSpeechIcon(int x, int y, uint16_t color) {
  // Speech bubble
  tft.fillRoundRect(x, y, 12, 8, 3, color);
  tft.fillTriangle(x + 2, y + 8, x + 6, y + 8, x + 4, y + 11, color);
  
  // Speech lines
  tft.drawFastHLine(x + 2, y + 2, 8, COLOR_BLACK);
  tft.drawFastHLine(x + 2, y + 4, 6, COLOR_BLACK);
  tft.drawFastHLine(x + 2, y + 6, 7, COLOR_BLACK);
}

// Smart AI icon for response page
void Display_DrawSmartAIIcon(int x, int y, uint16_t color) {
  // AI chip design
  tft.drawRoundRect(x, y, 12, 10, 2, color);
  
  // Circuit patterns
  tft.drawFastHLine(x + 2, y + 2, 8, color);
  tft.drawFastHLine(x + 2, y + 4, 6, color);
  tft.drawFastHLine(x + 2, y + 6, 8, color);
  tft.drawFastHLine(x + 2, y + 8, 4, color);
  
  // Connection pins
  tft.drawFastVLine(x - 1, y + 2, 2, color);
  tft.drawFastVLine(x - 1, y + 6, 2, color);
  tft.drawFastVLine(x + 12, y + 2, 2, color);
  tft.drawFastVLine(x + 12, y + 6, 2, color);
}

// Full page text rendering for dedicated pages
void Display_DrawFullPageText(String text, int startY, int maxHeight, uint16_t color) {
  tft.setTextColor(color);
  tft.setTextSize(1);
  
  int maxCharsPerLine = 20; // Fixed number of characters per line
  int currentY = startY;
  int lineHeight = 8; // Very compact line spacing
  
  // Simple character-based wrapping for full page display
  int textIndex = 0;
  int lineCount = 0;
  int maxLines = maxHeight / lineHeight;
  
  while(textIndex < text.length() && lineCount < maxLines) {
    String line = "";
    
    // Build line character by character
    for(int i = 0; i < maxCharsPerLine && textIndex < text.length(); i++) {
      char c = text[textIndex];
      line += c;
      textIndex++;
      
      if(c == '\n') break; // Handle newlines
    }
    
    // Draw the line
    tft.setCursor(3, currentY);
    tft.print(line); // Use print instead of println
    currentY += lineHeight;
    lineCount++;
  }
}
