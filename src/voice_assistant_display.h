// ------------------------------------------------------------------------------------------------------------------------------
// ----------------           Voice Assistant Display Library - TFT GUI Interface                              ---------------- 
// ----------------                     Cool animated interface for ESP32-S3 Voice Assistant                   ----------------
// ------------------------------------------------------------------------------------------------------------------------------

#ifndef VOICE_ASSISTANT_DISPLAY_H
#define VOICE_ASSISTANT_DISPLAY_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// Display states
enum DisplayState {
  STATE_INIT,
  STATE_WAITING,
  STATE_LISTENING,
  STATE_PROCESSING,
  STATE_DISPLAYING_TEXT,
  STATE_AI_THINKING,
  STATE_AI_RESPONSE,
  STATE_ERROR
};

// Colors (RGB565 format)
#define COLOR_BLACK       0x0000
#define COLOR_WHITE       0xFFFF
#define COLOR_RED         0xF800
#define COLOR_GREEN       0x07E0
#define COLOR_BLUE        0x001F
#define COLOR_YELLOW      0xFFE0
#define COLOR_CYAN        0x07FF
#define COLOR_MAGENTA     0xF81F
#define COLOR_ORANGE      0xFD20
#define COLOR_PURPLE      0x8010
#define COLOR_DARK_BLUE   0x0008
#define COLOR_LIGHT_GRAY  0xC618
#define COLOR_DARK_GRAY   0x7BEF

// Display functions
void Display_Init();
void Display_ShowWelcome();
void Display_ShowMainInterface();
void Display_ShowWaiting();
void Display_ShowListening();
void Display_ShowProcessing();
void Display_ShowTranscription(String text);
void Display_ShowAIThinking();
void Display_ShowAIResponse(String response);
void Display_ShowError(String error);
void Display_Update();
void Display_DrawMicrophone(int x, int y, uint16_t color);
void Display_DrawWaveform(int frame);
void Display_DrawSpinner(int frame);
void Display_DrawAIIcon(int x, int y, uint16_t color);
void Display_ScrollText(String text, int y, uint16_t color, int maxWidth);
void Display_DrawTextInBox(String text, int startY, int maxHeight, uint16_t color);
void Display_DrawModernText(String text, int startY, int maxHeight, uint16_t color);
void Display_DrawModernMicrophone(int x, int y, uint16_t color);
void Display_DrawModernAIIcon(int x, int y, uint16_t color);
void Display_DrawStylishAIIcon(int x, int y, uint16_t color);
void Display_DrawStylishMicrophone(int x, int y, uint16_t color);
void Display_DrawSpeechIcon(int x, int y, uint16_t color);
void Display_DrawSmartAIIcon(int x, int y, uint16_t color);
void Display_DrawFullPageText(String text, int startY, int maxHeight, uint16_t color);

// Animation variables
extern int animationFrame;
extern unsigned long lastAnimationUpdate;
extern DisplayState currentState;

#endif
