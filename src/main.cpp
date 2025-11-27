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
bool useRainbow    = false;    // Set 'true' for rainbow, 'false' for solid color
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