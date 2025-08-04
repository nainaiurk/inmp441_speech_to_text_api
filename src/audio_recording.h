#ifndef AUDIO_RECORDING_H
#define AUDIO_RECORDING_H

#include <Arduino.h>

// Function declarations
bool I2S_Record_Init();
bool Record_Start(String filename);
bool Record_Available(String filename, float* audiolength_sec);

#endif
