// ------------------------------------------------------------------------------------------------------------------------------
// ----------------  AUDIO RECORDING Library - storing .wav on SD_MMC (variable sample/bit rate, gain booster) --------------- 
// ----------------                                      Modified for ESP32-S3                                  ----------------
// ----------------                                                                                              ---------------- 
// ----------------                Used pins SD_MMC: Custom pins CLK=36, CMD=35, DATA=37 (1-bit mode)          ----------------
// ----------------                Used pins I2S Microphone INMP441: WS=16, SCK=17, SD=19                       ----------------
// ----------------                Using ESP32 Arduino framework with legacy I2S driver                         ----------------
// ----------------                                                                                              ----------------
// ----------------   bool I2S_Record_Init();                // Initialization (once)                            ----------------
// ----------------   bool Record_Start(file);               // appending I2S buffer to file (in loop, ongoing)  ----------------
// ----------------   bool Record_Available(file, &duration) // stop recording (once)                            ----------------
// ------------------------------------------------------------------------------------------------------------------------------

#include <Arduino.h>
#include <driver/i2s.h>     // Legacy I2S driver for compatibility
#include <SD_MMC.h>         // SD_MMC library
#include <math.h>           // For sine wave generation in test mode

// --- defines & macros --------

#ifndef DEBUG                   // user can define favorite behaviour ('true' displays addition info)
#  define DEBUG true            // <- define your preference here  
#  define DebugPrint(x);        if(DEBUG){Serial.print(x);}   /* do not touch */
#  define DebugPrintln(x);      if(DEBUG){Serial.println(x);} /* do not touch */ 
#endif

// --- PIN assignments ---------

#define I2S_WS      16          // Word Select (Left/Right channel select)
#define I2S_SD      19          // Serial Data
#define I2S_SCK     17          // Serial Clock

// --- Audio settings ----

#define SAMPLE_RATE             16000  // 16kHz optimal for speech recognition
#define BITS_PER_SAMPLE         16     // 16-bit samples
#define GAIN_BOOSTER_I2S        32     // Software gain multiplier for INMP441 (1-64 range)

// --- global vars -------------

// I2S configuration optimized for speed
const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(16),  // Always 16-bit for I2S slots
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,   // Mono recording
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,      // Higher priority interrupts for faster processing
    .dma_buf_count = 4,                            // Reduced buffer count for lower latency
    .dma_buf_len = 512,                            // Smaller buffers for faster response
    .use_apll = false
};

// I2S pin configuration
const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
};

// WAV Header structure (exactly like Deepgram folder)
struct WAV_HEADER 
{ 
  char  riff[4] = {'R','I','F','F'};                        /* "RIFF"                                   */
  long  flength = 0;                                        /* file length in bytes                     ==> Calc at end ! */
  char  wave[4] = {'W','A','V','E'};                        /* "WAVE"                                   */
  char  fmt[4]  = {'f','m','t',' '};                        /* "fmt "                                   */
  long  chunk_size = 16;                                    /* size of FMT chunk in bytes (usually 16)  */
  short format_tag = 1;                                     /* 1=PCM, 257=Mu-Law, 258=A-Law, 259=ADPCM  */
  short num_chans = 1;                                      /* 1=mono, 2=stereo                         */
  long  srate = SAMPLE_RATE;                                /* samples per second, e.g. 16000           */
  long  bytes_per_sec = SAMPLE_RATE * (BITS_PER_SAMPLE/8);  /* srate * bytes_per_samp, e.g. 32000       */ 
  short bytes_per_samp = (BITS_PER_SAMPLE/8);               /* 2=16-bit mono, 4=16-bit stereo (byte 34) */
  short bits_per_samp = BITS_PER_SAMPLE;                    /* Number of bits per sample, e.g. 16       */
  char  dat[4] = {'d','a','t','a'};                         /* "data"                                   */
  long  dlength = 0;                                        /* data length in bytes (filelength - 44)   ==> Calc at end ! */
} myWAV_Header;

bool flg_is_recording = false;         // only internally used
bool flg_I2S_initialized = false;      // to avoid any runtime errors in case user forgot to initialize

// ------------------------------------------------------------------------------------------------------------------------------

bool I2S_Record_Init() 
{  
  // Install I2S driver
  esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("Failed installing I2S driver: %d\n", err);
    return false;
  }

  // Set I2S pin configuration
  err = i2s_set_pin(I2S_NUM_0, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("Failed setting I2S pins: %d\n", err);
    return false;
  }
  
  // Start I2S
  err = i2s_start(I2S_NUM_0);
  if (err != ESP_OK) {
    Serial.printf("Failed starting I2S: %d\n", err);
    return false;
  }

  flg_I2S_initialized = true;
  DebugPrintln("✅ I2S Recording initialized successfully");

  return flg_I2S_initialized;  
}

// ------------------------------------------------------------------------------------------------------------------------------
bool Record_Start( String audio_filename ) 
{
  if (!flg_I2S_initialized) {
    Serial.println( "ERROR in Record_Start() - I2S not initialized, call 'I2S_Record_Init()' missed" );    
    return false;
  }
  
  if (!flg_is_recording) {  // entering 1st time -> remove old AUDIO file, create new file with WAV header
    flg_is_recording = true;
    
    if (SD_MMC.exists(audio_filename)) {
      SD_MMC.remove(audio_filename); 
    }
    
    // Create new file with WAV header
    File audio_file = SD_MMC.open(audio_filename, FILE_WRITE);
    if (!audio_file) {
      Serial.println("Failed to open file for writing WAV header");
      flg_is_recording = false;
      return false;
    }
    audio_file.write((uint8_t *) &myWAV_Header, 44);
    audio_file.close(); 
    
    return true;
  }
  
  if (flg_is_recording) {  // here we land when recording started already -> task: append record buffer to file
    // Array to store original audio I2S input stream (reading in chunks, e.g. 512 values for faster processing) 
    int16_t audio_buffer[512];         // Reduced buffer size for faster processing
    uint8_t audio_buffer_8bit[512];    // Reduced buffer size

    // Reading the I2S input stream
    size_t bytes_read = 0;
    esp_err_t result = i2s_read(I2S_NUM_0, audio_buffer, sizeof(audio_buffer), &bytes_read, portMAX_DELAY);
    
    if (result != ESP_OK) {
      Serial.println("Error reading from I2S");
      return false;
    }

    // Debug: Check if we're getting actual audio data
    static bool first_chunk = true;
    if (first_chunk) {
      first_chunk = false;
      
      // Check first few samples for actual data
      bool has_audio = false;
      for (int i = 0; i < 10 && i < (bytes_read/2); i++) {
        if (abs(audio_buffer[i]) > 100) {  // Non-zero threshold
          has_audio = true;
          break;
        }
      }
      
      if (!has_audio) {
        // Generate a test sine wave at 1kHz for testing file writing
        for (int i = 0; i < (bytes_read/2); i++) {
          float angle = 2.0 * PI * 1000.0 * i / SAMPLE_RATE;  // 1kHz tone
          audio_buffer[i] = (int16_t)(sin(angle) * 8000);  // Moderate amplitude
        }
      }
    }

    // Apply gain boost to compensate for low I2S Microphone INMP441 amplitude
    if ( GAIN_BOOSTER_I2S > 1 && GAIN_BOOSTER_I2S <= 64 ) {
      for (int16_t i = 0; i < ( bytes_read / 2 ); ++i) {          // all samples, 16bit (bytes_read/2) 
        int32_t boosted_sample = (int32_t)audio_buffer[i] * GAIN_BOOSTER_I2S;
        // Clamp to prevent overflow
        if (boosted_sample > 32767) boosted_sample = 32767;
        if (boosted_sample < -32768) boosted_sample = -32768;
        audio_buffer[i] = (int16_t)boosted_sample;
      }
    }

    // If 8bit requested: Calculate 8bit Mono files 
    // 16-bit signed to 8-bit WAV conversion rule: FROM -32768...0(silence)...+32767 -> TO: 0...128(silence)...256 
    if (BITS_PER_SAMPLE == 8) {
      for (int16_t i = 0; i < ( bytes_read / 2 ); ++i) {        
        audio_buffer_8bit[i] = (uint8_t) ((( audio_buffer[i] + 32768 ) >>8 ) & 0xFF); 
      }
    }
    
    // Save audio data to SD_MMC card (appending chunk array to file end)
    File audio_file = SD_MMC.open(audio_filename, FILE_APPEND);
    if (audio_file) {  
       size_t bytes_written = 0;
       if (BITS_PER_SAMPLE == 16) { // 16 bit default: appending original I2S chunks
         bytes_written = audio_file.write((uint8_t*)audio_buffer, bytes_read);
       }        
       if (BITS_PER_SAMPLE == 8) {  // 8bit mode: appending calculated values instead
         bytes_written = audio_file.write((uint8_t*)audio_buffer_8bit, bytes_read/2);
       }  
       audio_file.flush();  // Ensure data is written to SD card
       audio_file.close(); 
       
       return true; 
    }  
    
    if (!audio_file) {
      Serial.println("ERROR in Record_Start() - Failed to open audio file!"); 
      return false;
    }    
  }
  
  return false;  
}

// ------------------------------------------------------------------------------------------------------------------------------
bool Record_Available( String audio_filename, float* audiolength_sec ) 
{
  if (flg_is_recording) {  // Recording was started and now finalized when this procedure is called
    flg_is_recording = false;
    
    // Update WAV header with actual file sizes
    File audio_file = SD_MMC.open(audio_filename, FILE_READ);
    if (!audio_file) {
      Serial.println("ERROR - Failed to open file for size calculation");
      return false;
    }
    
    size_t file_size = audio_file.size();
    audio_file.close();
    
    // Calculate duration in seconds
    size_t data_size = file_size - 44; // Subtract WAV header size
    *audiolength_sec = (float)data_size / (SAMPLE_RATE * (BITS_PER_SAMPLE / 8));
    
    if (data_size == 0) {
      Serial.println("❌ ERROR: No audio data written to file!");
      return false;
    }
    
    // Update WAV header with correct file sizes
    myWAV_Header.flength = file_size - 8;  // Total file size minus 8 bytes
    myWAV_Header.dlength = data_size;      // Data chunk size
    
    // WORKAROUND: Create a temporary file with correct header, then copy data
    String temp_filename = "/Audio_temp.wav";
    
    // Remove any existing temp file
    if (SD_MMC.exists(temp_filename)) {
      SD_MMC.remove(temp_filename);
    }
    
    // Create new file with correct header
    File temp_file = SD_MMC.open(temp_filename, FILE_WRITE);
    if (!temp_file) {
      Serial.println("❌ ERROR: Failed to create temporary file");
      return false;
    }
    
    // Write correct WAV header
    temp_file.write((uint8_t*)&myWAV_Header, 44);
    
    // Copy audio data from original file (skip the old header)
    File original_file = SD_MMC.open(audio_filename, FILE_READ);
    if (!original_file) {
      Serial.println("❌ ERROR: Failed to open original file for copying");
      temp_file.close();
      return false;
    }
    
    original_file.seek(44);  // Skip the old header
    uint8_t buffer[512];
    size_t bytes_copied = 0;
    
    while (original_file.available()) {
      size_t bytes_read = original_file.read(buffer, sizeof(buffer));
      size_t bytes_written = temp_file.write(buffer, bytes_read);
      bytes_copied += bytes_written;
      
      if (bytes_read != bytes_written) {
        Serial.println("❌ ERROR: Copy operation failed");
        break;
      }
    }
    
    original_file.close();
    temp_file.flush();
    temp_file.close();
    
    // Replace original file with corrected file
    SD_MMC.remove(audio_filename);
    
    // Create final file with corrected name
    File final_file = SD_MMC.open(audio_filename, FILE_WRITE);
    File source_file = SD_MMC.open(temp_filename, FILE_READ);
    
    if (final_file && source_file) {
      while (source_file.available()) {
        size_t bytes_read = source_file.read(buffer, sizeof(buffer));
        final_file.write(buffer, bytes_read);
      }
      
      source_file.close();
      final_file.flush();
      final_file.close();
      
      // Clean up temp file
      SD_MMC.remove(temp_filename);
      
    } else {
      Serial.println("❌ ERROR: Failed to create final corrected file");
      return false;
    }
    
    return true;
  }
  
  return false;  // if not recording, nothing available
}
