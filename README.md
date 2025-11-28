# ESP32-S3 High-Speed Traditional Chinese LED Matrix

![IMG_led-matrix-dmeo_20251128](https://github.com/user-attachments/assets/7cdd2f57-c0b2-4cc5-a48e-2ace2171bacd)

https://github.com/user-attachments/assets/081b6dae-b0a2-4cd4-9848-4d3ffebd0bb0

## 1\. Project Overview

This project drives a 64x64 RGB LED Matrix to display scrolling Traditional Chinese text at high frame rates (120FPS+). It uses **Pre-Rendering** (drawing text to memory first) to eliminate flickering and lag.

## 2\. Hardware Bill of Materials

  * **Microcontroller:** Espressif **ESP32-S3-DevKitC-1U-N8R8**
      * **Flash:** 8MB (Quad SPI)
      * **PSRAM:** 8MB (Octal SPI) - *REQUIRED for this code.*
      * *Note:* The "1U" model requires an external WiFi antenna.
  * **Display:** Waveshare **RGB-Matrix-P3-64x64**
      * **Driver IC:** SM16208 or SM5166 (High Refresh Rate).
      * **Interface:** HUB75E.
        * HUB75: For panels 32 pixels high (Pin 8 is Ground).
        * HUB75E: For panels 64 pixels high (Pin 8 is Address E).
  * **Power Supply:** 5V 4A (or higher) DC Adapter.
      * *Connect 5V directly to the panel's power header, not through the ESP32.*

## 3\. Wiring Guide (ESP32-S3 to HUB75)

Direct connection (no level shifter needed for this specific board).

| Matrix Pin | ESP32 Pin | Wire Color | Function |
| :--- | :--- | :--- | :--- |
| **R1** | 4 | Red | Top Red Data |
| **G1** | 41 | Green | Top Green Data |
| **B1** | 5 | Blue | Top Blue Data |
| **R2** | 6 | Red | Bottom Red Data |
| **G2** | 40 | Green | Bottom Green Data |
| **B2** | 7 | Blue | Bottom Blue Data |
| **A** | 15 | Yellow | Row Address A |
| **B** | 48 | Orange | Row Address B |
| **C** | 16 | Brown | Row Address C |
| **D** | 47 | White | Row Address D |
| **E** | 39 | Black | Row Address E (GND on some panels) |
| **LAT** | 21 | Purple | Latch Data |
| **OE** | 18 | Gray | Output Enable (Brightness) |
| **CLK** | 17 | Black | Clock Signal |
| **GND** | GND | Black | **Common Ground (CRITICAL)** |

## 4\. Software Setup (PlatformIO)

### A. `platformio.ini` (Project Settings)

You MUST use these settings to enable the 8MB Octal PSRAM.

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200

; Memory & Partition Settings
board_build.arduino.memory_type = qio_opi 
board_build.partitions = partitions_custom.csv
board_build.filesystem = littlefs
board_upload.flash_size = 8MB

; Libraries
lib_deps = 
    ; 1. The Matrix Driver
    mrfaptastic/ESP32 HUB75 LED MATRIX PANEL DMA Display @ ^3.0.12
    
    ; 2. Adafruit GFX (Required by the Matrix Driver)
    adafruit/Adafruit GFX Library @ ^1.11.9
    
    ; 3. OpenFontRender -> DOWNLOAD DIRECTLY FROM GITHUB
    https://github.com/takkaO/OpenFontRender.git

build_flags = 
    -D BOARD_HAS_PSRAM
```

### B. `partitions_custom.csv` (Memory Map)

Create this file in the project root. It gives 5MB of storage for the Font File.

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     ,        0x4000,
otadata,  data, ota,     ,        0x2000,
phy_init, data, phy,     ,        0x1000,
factory,  app,  factory, ,        0x200000,
spiffs,   data, spiffs,  ,        0x500000,
```

## 5\. Font System (Choosing & Flashing)

The ESP32 reads `/font.ttf` from its internal storage. You have two choices:

### Option A: High-Quality Vector Font (Recommended)

  * **File:** `TaipeiSansTC-Subset.ttf` (\~3MB).
      * (免費可商用字體 台北黑體 Taipei Sans TC [https://github.com/VdustR/taipei-sans-tc](https://github.com/VdustR/taipei-sans-tc))
      * (Adobe 及 Google 推出思源黑體（Source Han Sans 或 Noto Sans CJK）後，思源黑體頓時佔據繁體中文世界的許多平面設計。然而許多人指出該製品 TC/TW 版的設計問題，甚至不適用於印刷。因此，翰字鑄造投入開源字型的改作，以思源黑體為基礎，讓繁體中文的使用者也能有適用於不同情境的印刷風格黑體。)
  * **Pros:** Smooth curves, looks professional at large sizes (40px+).
  * **Cons:** Requires "Subsetting" (removing unused characters) to fit in memory.
  * **How to Generate:**
    1.  Go to folder `1_subsetting fonts_ttf_to_ttf`.
    2.  Run `generate_font.bat` (Requires Python & fonttools).
    3.  It generates the subset based on `tw_edu_common_4808_chars.txt` and `basic_latin`.

### Option B: Pixel Font (Alternative)

  * **File:** `Cubic_11.ttf` (\~2.7MB).
      * (免費開源的 11×11 中文點陣體 [https://github.com/ACh-K/Cubic-11](https://github.com/ACh-K/Cubic-11))
  * **Pros:** No subsetting needed (file is naturally small). Sharp edges.
  * **Cons:** Looks "blocky" or low-res.
  * **How to use:** Just rename `Cubic_11.ttf` to `font.ttf`.

### How to Flash the Font to ESP32

1.  Rename your chosen `.ttf` file to `font.ttf`.
2.  Place it inside the `data/` folder in your project.
3.  In PlatformIO, click the **Alien Icon -\> Project Tasks -\> Platform -\> Upload Filesystem Image**.

## 6\. User Guide

  * **Change Text:** Edit `userText` in `src/main.cpp` and upload.
  * **Change Text via USB:** Open Serial Monitor, type text, press Enter.
  * **Change Color:** Type `COLOR 255 0 0` in Serial Monitor.

## 7\. Troubleshooting

  * **Text split in middle:** Adjust `bottomHalfOffset = 1` or `-1` in `main.cpp`.
  * **Red Screen:** Font load failed. Re-run "Upload Filesystem Image".
  * **Ghosting:** The code uses `HZ_20M` speed. If flickering occurs, lower to `HZ_10M`.

## 8\. The Final Code (`src/main.cpp`)

  * **Text Split in Middle:** Change `bottomHalfOffset` in code (1 or -1).
  * **Ghosting:** Ensure `mxconfig.i2sspeed` is `HZ_20M`.
  * **Red Screen:** Font file missing. Upload Filesystem Image.

<!-- end list -->

```cpp
/**
 * ----------------------------------------------------------------------------
 * PROJECT: ESP32-S3 High-Speed Chinese LED Matrix
 * HARDWARE: ESP32-S3-DevKitC-1U-N8R8 + Waveshare 64x64 HUB75 Panel
 * ----------------------------------------------------------------------------
 * This code loads a TrueType font from internal memory, renders a text string
 * into a virtual image in RAM (PSRAM), and scrolls it smoothly across the display.
 */

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <FS.h>
#include <LittleFS.h>
#include <OpenFontRender.h>

/* =======================================================================
   SECTION 1: USER SETTINGS (EDIT HERE)
   ======================================================================= */
// Default Text (Can be changed via Serial Monitor later)
String currentText = "注意! 車禍. Slow Down!"; 

// Font Size
// Tip: 48px is safe. 55px is max but might cut off 'g' or 'y' tails.
int fontSize       = 48;      
int brightness     = 60;      // 0 (Off) to 255 (Max)

// Default Color (Cyan)
uint8_t colorR     = 0;
uint8_t colorG     = 255;
uint8_t colorB     = 255;

// Rainbow Effect
bool useRainbow    = true;    // Set 'true' for rainbow, 'false' for solid color
float rainbowScale = 0.5;     // 0.2 = Wide Rainbow, 1.0 = Tight Rainbow

// Scroll Physics
int scrollDelay    = 20;      // Speed: Lower number = Faster
int scrollStep     = 2;       // Jump: Pixels to move per frame (1=Smooth, 3=Fast)

// *** HARDWARE CALIBRATION ***
// Waveshare panels often have a manufacturing quirk where the bottom half
// (Rows 32-63) is shifted by 1 pixel.
int bottomHalfOffset = 1;     // Try 1, -1, or 0 to align the image.
int verticalOffset   = -4;    // Moves text UP to fit tails of letters.

/* =======================================================================
   SECTION 2: HARDWARE PIN MAPPING
   ======================================================================= */
#define R1 4
#define G1 41
#define B1 5
#define R2 6
#define G2 40
#define B2 7
#define A_PIN 15
#define B_PIN 48
#define C_PIN 16
#define D_PIN 47
#define E_PIN 39 
#define LAT 21
#define OE 18
#define CLK 17

#define RES_X 64      
#define RES_Y 64      

// Global Objects
MatrixPanel_I2S_DMA *dma_display = nullptr;
OpenFontRender render;
uint8_t *fontBuffer = nullptr;

// Animation State
float currentScrollX = -RES_X; 
unsigned long lastDrawTime = 0;
int canvasWidth = 0;

/* ----------------------------------------------------------------------
   CLASS: PSRAMCanvas
   Purpose: A "Virtual Screen" stored in the ESP32's 8MB RAM.
   We draw the text here ONCE, then copy it to the LED Matrix.
   ---------------------------------------------------------------------- */
class PSRAMCanvas : public Adafruit_GFX {
public:
    uint16_t* buffer;

    // Constructor: Allocates massive memory buffer
    PSRAMCanvas(int16_t w, int16_t h) : Adafruit_GFX(w, h) {
        buffer = (uint16_t*) ps_calloc(w * h, sizeof(uint16_t));
    }
    
    // Destructor: Frees memory when text changes
    ~PSRAMCanvas() {
        if(buffer) free(buffer);
    }

    // Custom Draw: Applies the "Hardware Fix" automatically
    void drawPixel(int16_t x, int16_t y, uint16_t color) override {
        // Shift bottom half of screen if calibration is set
        if (y >= 32) x = x + bottomHalfOffset; 

        // Safety: Only draw if inside memory bounds
        if (x >= 0 && x < _width && y >= 0 && y < _height) {
            buffer[y * _width + x] = color;
        }
    }
    
    void fillScreen(uint16_t color) override { 
        if(buffer) memset(buffer, color, _width * _height * sizeof(uint16_t));
    }
};

PSRAMCanvas *bigCanvas = nullptr; // The main image buffer

/* ----------------------------------------------------------------------
   HELPER: Rainbow Color Generator
   Converts Hue (0-255) to RGB565 format
   ---------------------------------------------------------------------- */
uint16_t colorHSV(long hue, uint8_t sat, uint8_t val) {
    uint8_t r, g, b;
    unsigned char region, remainder, p, q, t;
    if (sat == 0) { r = val; g = val; b = val; }
    else {
        region = hue / 43; remainder = (hue - (region * 43)) * 6;
        p = (val * (255 - sat)) >> 8;
        q = (val * (255 - ((sat * remainder) >> 8))) >> 8;
        t = (val * (255 - ((sat * (255 - remainder)) >> 8))) >> 8;
        switch (region) {
            case 0: r = val; g = t; b = p; break;
            case 1: r = q; g = val; b = p; break;
            case 2: r = p; g = val; b = t; break;
            case 3: r = p; g = q; b = val; break;
            case 4: r = t; g = p; b = val; break;
            default: r = val; g = p; b = q; break;
        }
    }
    return dma_display->color565(r, g, b);
}

/* ----------------------------------------------------------------------
   HELPER: Load Resources
   Reads font.ttf from Flash into RAM.
   ---------------------------------------------------------------------- */
void loadResources() {
    Serial.println("--- Loading Resources ---");
    
    // 1. Check Hardware RAM
    if (!psramInit()) { Serial.println("ERROR: No PSRAM found. Check platformio.ini"); while(1); }
    
    // 2. Mount File System
    if (!LittleFS.begin(false)) { Serial.println("ERROR: Filesystem Failed"); while(1); }

    // 3. Open Font File
    File fontFile = LittleFS.open("/font.ttf", "r");
    if (!fontFile) { Serial.println("ERROR: 'font.ttf' missing in /data folder"); while(1); }
    
    // 4. Allocate RAM for Font
    size_t size = fontFile.size();
    fontBuffer = (uint8_t*) ps_calloc(size, sizeof(uint8_t));
    
    // 5. Robust Chunk Reading (Prevents crashing on large files)
    size_t bytesRead = 0;
    while (fontFile.available()) {
        size_t toRead = std::min((size_t)4096, size - bytesRead);
        int res = fontFile.read(fontBuffer + bytesRead, toRead);
        if (res <= 0) break;
        bytesRead += res;
    }
    fontFile.close();
    
    // 6. Initialize Font Engine
    render.loadFont(fontBuffer, size);
    Serial.println("Resources Loaded.");
}

/* ----------------------------------------------------------------------
   HELPER: Pre-Render Text
   Draws the text string into the 'bigCanvas' memory buffer.
   ---------------------------------------------------------------------- */
void preRenderText() {
    // Clear old memory
    if (bigCanvas) { delete bigCanvas; bigCanvas = nullptr; }

    // 1. Measure Text Width
    render.setFontSize(fontSize);
    int textW = render.getTextWidth(currentText.c_str());
    canvasWidth = textW + 20; // Add padding

    // 2. Create Virtual Screen
    bigCanvas = new PSRAMCanvas(canvasWidth, RES_Y);
    if (!bigCanvas->buffer) { Serial.println("Error: Out of RAM!"); return; }

    render.setDrawer(*bigCanvas);
    
    // 3. Draw Text Base (White)
    render.setFontColor(0xFFFF); 
    int yPos = ((RES_Y - fontSize) / 2) + verticalOffset;
    render.setCursor(0, yPos); 
    render.printf(currentText.c_str());
    
    // 4. Apply Rainbow Effect (Optional)
    if (useRainbow) {
        for (int y = 0; y < RES_Y; y++) {
            for (int x = 0; x < canvasWidth; x++) {
                // If this pixel is lit (part of a letter)
                if (bigCanvas->buffer[y * canvasWidth + x] != 0) {
                    // Color it based on X position
                    int hue = (int)(x * rainbowScale) % 255;
                    bigCanvas->buffer[y * canvasWidth + x] = colorHSV(hue, 255, 255);
                }
            }
        }
    } else {
        // Apply Solid User Color
        uint16_t solidColor = dma_display->color565(colorR, colorG, colorB);
        for (int i = 0; i < canvasWidth * RES_Y; i++) {
             if (bigCanvas->buffer[i] != 0) bigCanvas->buffer[i] = solidColor;
        }
    }
    
    // Reset Scroll to Start
    currentScrollX = -RES_X;
    Serial.println("Canvas Updated.");
}

// *** SERIAL COMMAND HANDLER ***
// Allows changing text/color via USB without re-uploading
void checkSerialInput() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim(); 

        if (input.length() > 0) {
            String cmdCheck = input;
            cmdCheck.toUpperCase();

            // Check for "COLOR 255 0 0" command
            if (cmdCheck.startsWith("COLOR")) {
                int separatorIndex = -1;
                for(int i=0; i<input.length(); i++) {
                    if(input[i] == ' ' || input[i] == ':') { separatorIndex = i; break; }
                }
                if(separatorIndex != -1) {
                    String params = input.substring(separatorIndex + 1);
                    int r, g, b;
                    if(sscanf(params.c_str(), "%d %d %d", &r, &g, &b) >= 3 || 
                       sscanf(params.c_str(), "%d,%d,%d", &r, &g, &b) >= 3) {
                        colorR = r; colorG = g; colorB = b;
                        Serial.printf("Color updated: %d %d %d\n", r, g, b);
                        preRenderText(); 
                        return;
                    }
                }
            } 
            
            // Otherwise treat as Text
            currentText = input;
            Serial.println("Text Updated: " + currentText);
            preRenderText();
        }
    }
}

/* =======================================================================
   SECTION 3: SETUP
   ======================================================================= */
void setup() {
    Serial.begin(115200);
    delay(2000); 
    loadResources();

    // 1. Configure Matrix Pins
    HUB75_I2S_CFG mxconfig(RES_X, RES_Y, 1);
    mxconfig.gpio.r1 = R1; mxconfig.gpio.g1 = G1; mxconfig.gpio.b1 = B1;
    mxconfig.gpio.r2 = R2; mxconfig.gpio.g2 = G2; mxconfig.gpio.b2 = B2;
    mxconfig.gpio.a = A_PIN; mxconfig.gpio.b = B_PIN; mxconfig.gpio.c = C_PIN;
    mxconfig.gpio.d = D_PIN; mxconfig.gpio.e = E_PIN;
    mxconfig.gpio.lat = LAT; mxconfig.gpio.oe = OE; mxconfig.gpio.clk = CLK;
    
    // 2. Configure Driver Speed & Stability
    mxconfig.double_buff = true;               // Essential for no flicker
    mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_20M; // High Speed
    mxconfig.latch_blanking = 1; 
    mxconfig.clkphase = false; 

    dma_display = new MatrixPanel_I2S_DMA(mxconfig);
    dma_display->begin();
    dma_display->setBrightness8(brightness); 

    preRenderText();
    Serial.println("--- READY ---");
    Serial.println("Type text to change display.");
    Serial.println("Type 'color 255 0 0' to change color.");
}

/* =======================================================================
   SECTION 4: LOOP
   ======================================================================= */
void loop() {
    checkSerialInput();

    if (millis() - lastDrawTime > scrollDelay) {
        
        // A. Clear Screen
        dma_display->fillScreen(0); 
        
        // B. Copy from Virtual Canvas to Screen
        // We only loop through the visible 64x64 area
        for (int y = 0; y < RES_Y; y++) {
            // Optimization: Pointer arithmetic for speed
            uint16_t* rowPtr = bigCanvas->buffer + (y * canvasWidth);
            
            for (int x = 0; x < RES_X; x++) {
                int srcX = (int)currentScrollX + x;
                // Boundary Check
                if (srcX >= 0 && srcX < canvasWidth) {
                    uint16_t c = rowPtr[srcX]; 
                    if (c > 0) dma_display->drawPixel(x, y, c);
                }
            }
        }
        
        // C. Swap Buffers (Show the new frame)
        dma_display->flipDMABuffer(); 

        // D. Move Scroll Position
        currentScrollX += scrollStep; 
        
        // E. Reset if end reached
        if (currentScrollX > canvasWidth) currentScrollX = -RES_X;
        
        lastDrawTime = millis();
    }
}
```

## Project Structure

```
MY_LED_PROJECT/
├── .pio/
├── 1_subsetting fonts_ttf_to_ttf/   <-- YOUR FONT TOOLS FOLDER
│   ├── basic_latin.txt
│   ├── Cubic_11.ttf
│   ├── README.txt
│   ├── TaipeiSansTC-Subset.ttf      <-- The result file
│   ├── TaipeiSansTCBeta-Regular.ttf <-- The 20MB source
│   └── tw_edu_common_4808_chars.txt <-- The Chinese list
├── data/
│   └── font.ttf                     <-- COPY 'TaipeiSansTC-Subset.ttf' HERE and rename
├── src/
│   └── main.cpp                     <-- The Code
├── include/
├── platformio.ini
├── partitions_custom.csv
└── README.md                        <-- The Master Manual (Paste code below)
```
