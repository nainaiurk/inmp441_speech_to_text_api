#ifndef DEEPGRAM_TRANSCRIPTION_H
#define DEEPGRAM_TRANSCRIPTION_H

#include <Arduino.h>

// Function declarations
String SpeechToText_Deepgram(String audio_filename);
void Deepgram_KeepAlive();

#endif
