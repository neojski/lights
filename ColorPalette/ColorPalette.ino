#include <FastLED.h>

#define LED_PIN     5
#define NUM_LEDS    46
#define BRIGHTNESS  64
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

const int potentiometer = A0;

void setup() {
  Serial.begin(9600); // open the serial port at 9600 bps:
  delay( 3000 ); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalPixelString );
  FastLED.setBrightness(  BRIGHTNESS );

  // There's no second GND so let's make D2 low
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);

  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;
}

float knob() {
  float p = analogRead(potentiometer) / 1024.;
  // clamp at 10 as it seems my lights min:

  static float p_smooth = 0.;
  p_smooth += (p - p_smooth) / 20.;

  return p_smooth;
}

void loop()
{
  static int time = 0;
  time = time + 16; /* motion speed */

  float p = knob();

  if (p >= 0.8) {
    // rainbow
    FillLEDsFromPaletteColors(time, p);
  } else {
    // normal white
    warmWhite(p);
  }

  FastLED.show();
  FastLED.delay(1000 / UPDATES_PER_SECOND);
}

char buffer[1000];
void printColor (CRGB c) {
  sprintf(buffer, "(%d, %d, %d)", c[0], c[1], c[2]);
  Serial.print(buffer);
}

void warmWhite(float knob) {
  int howManyOn = NUM_LEDS;
  const float turnOffThreshold = 0.5;
  if (knob < turnOffThreshold) {
    howManyOn = NUM_LEDS * (knob / turnOffThreshold);
  }

  uint8_t brightness = 32 + knob * 32; // below roughly 32 the LEDs colours get some ugly tint 
  FastLED.setBrightness(brightness);
  for (int i = 0; i < NUM_LEDS; i++) {
    if (i < howManyOn) {
      leds[i] = 0xFFB02D;
    } else {
      leds[i] = CRGB::Black;
    }
  }
}

void FillLEDsFromPaletteColors(int time, float p)
{
  uint8_t brightness = 255 * p;
  // clamp at 10 as it seems hardware min for these lights
  if (brightness < 10) {
    brightness = 10;
  }

  for ( int i = 0; i < NUM_LEDS; i++) {
    uint8_t paletteIndex = (time / 256) % 256;
    uint8_t subPaletteIndex = time % 256;

    leds[i] =
      blend(
        ColorFromPalette( currentPalette, paletteIndex, brightness, currentBlending),
        ColorFromPalette( currentPalette, (paletteIndex + 1) % 256, brightness, currentBlending),
        subPaletteIndex);
    time += 256;
  }

}
