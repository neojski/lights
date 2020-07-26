#include <FastLED.h>

#define LED_PIN     5
#define NUM_LEDS    46
#define BRIGHTNESS  64
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

// This example shows several ways to set up and use 'palettes' of colors
// with FastLED.
//
// These compact palettes provide an easy way to re-colorize your
// animation on the fly, quickly, easily, and with low overhead.
//
// USING palettes is MUCH simpler in practice than in theory, so first just
// run this sketch, and watch the pretty lights as you then read through
// the code.  Although this sketch has eight (or more) different color schemes,
// the entire sketch compiles down to about 6.5K on AVR.
//
// FastLED provides a few pre-configured color palettes, and makes it
// extremely easy to make up your own color schemes with palettes.
//
// Some notes on the more abstract 'theory and practice' of
// FastLED compact palettes are at the bottom of this file.



CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;


void setup() {
    Serial.begin(9600); // open the serial port at 9600 bps:
    delay( 3000 ); // power-up safety delay
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(  BRIGHTNESS );
    
    currentPalette = RainbowColors_p;
    currentBlending = LINEARBLEND;
}


void loop()
{
    static int time = 0;
    time = time + 16; /* motion speed */
    
    FillLEDsFromPaletteColors(time);
    
    FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
}

char buffer[1000];
void printColor (CRGB c) {
  sprintf(buffer, "(%d, %d, %d)", c[0], c[1], c[2]);
  Serial.print(buffer);
}

void FillLEDsFromPaletteColors(int time)
{
    uint8_t brightness = 255;

    for( int i = 0; i < NUM_LEDS; i++) {
        uint8_t paletteIndex = (time / 256) % 256;
        uint8_t subPaletteIndex = time % 256; 

        leds[i] =
          blend(
            ColorFromPalette( currentPalette, paletteIndex, brightness, currentBlending),
            ColorFromPalette( currentPalette, (paletteIndex+1) % 256, brightness, currentBlending),
            subPaletteIndex);
        time += 256;

//        if (i == 0) {
//          sprintf(buffer, "time: %d, int: %d, main: %d, sub: %d\n", time, i, paletteIndex, subPaletteIndex);
//          Serial.println(buffer);
//          printColor(ColorFromPalette( currentPalette, paletteIndex, brightness, currentBlending));
//          printColor(ColorFromPalette( currentPalette, paletteIndex+1, brightness, currentBlending));
//          printColor(leds[i]);
//          Serial.print(" ");
//          printColor(ColorFromPalette( currentPalette, paletteIndex, brightness, currentBlending));
//          Serial.print(" ");
//          printColor(ColorFromPalette( currentPalette, (paletteIndex+1)%256, brightness, currentBlending));
//          Serial.println();
//        }
    }
    
}
