#include <FastLED.h>
#include <ESP8266WiFi.h>
#include "EC11.hpp"

#define LED_PIN     2
#define NUM_LEDS    46
#define NUM_ROWS    7
#define BRIGHTNESS  128
#define LED_TYPE    WS2811
#define COLOR_ORDER RGB
#define UPDATES_PER_SECOND 100
CRGB leds[NUM_LEDS];
WiFiServer server(80);

const int d1 = 5;
const int d2 = 4;
const int d3 = 0;
const int d4 = 2;
const int d5 = 14;
const int d6 = 12;

char buffer[1000];
void printColor (CRGB c) {
  sprintf(buffer, "(%d, %d, %d)", c[0], c[1], c[2]);
  Serial.print(buffer);
}

int clamp (int v, int m, int M) {
  if (v < m) {
    return m;
  }
  if (v > M) {
    return M;
  }
  return v;
}

int dir (bool clockwise) {
  return clockwise ? 1 : -1;
}

struct Program {
  public:
    virtual void loop ();
    virtual void knob (int knob, bool clockwise);
    virtual int knobs ();
};

struct White : public Program {
  private:
    int howManyRowsOn;
    int warmth; // 0 to 9

    const int minWarmth = 0;
    const int maxWarmth = 9;

  public:
    White() : howManyRowsOn(NUM_ROWS) {}
    
    int rowToCount (int rows) {
      int count = 0;
      for (int i = 0; i < rows; i++) {
        if (i % 2 == 0) {
          count += 7;
        } else {
          count += 6;
        }
      }
      return count;
    }

    virtual void loop () {
      int howManyOn = rowToCount(howManyRowsOn);
      for (int i = 0; i < NUM_LEDS; i++) {
        if (i < howManyOn) {
          const CHSV warmWhite = CHSV(26, 210, 180);
          const CHSV coldWhite = CHSV(0, 0, 180);
          const CHSV color = blend(warmWhite, coldWhite, map(warmth, minWarmth, maxWarmth, 0, 255));
          setLed(i, color);
        } else {
          setLed(i, CRGB::Black);
        }
      }
    }


    virtual void knob (int knob, bool clockwise) {
      switch (knob) {
        case 0:
          howManyRowsOn = clamp(howManyRowsOn + dir(clockwise), 0, NUM_ROWS);
          break;
        case 1:
          warmth = clamp (warmth + dir(clockwise), minWarmth, maxWarmth);
          break;
      }
    }

    virtual int knobs() {
      return 2;
    }
};

struct Rainbow : public Program {
  private:
    int prevMillis;
    int time = 0;
    int speed = 3;

  public:
    Rainbow () {}

    virtual void loop () {
      int time0 = time;
      for ( int i = 0; i < NUM_LEDS; i++) {
        const uint8_t paletteIndex = (time0 / 256) % 256;
        const uint8_t subPaletteIndex = time0 % 256;

        setLed(i,
               blend(
                 ColorFromPalette( RainbowColors_p, paletteIndex, BRIGHTNESS, LINEARBLEND),
                 ColorFromPalette( RainbowColors_p, (paletteIndex + 1) % 256, BRIGHTNESS, LINEARBLEND),
                 subPaletteIndex));
        time0 += 256;
      }

      int currentMillis = millis();
      time += speed * (currentMillis - prevMillis);
      prevMillis = currentMillis;
    }

    virtual void knob (int knob, bool clockwise) {
      speed += dir(clockwise);

      sprintf(buffer, "rainbow: speed %d", speed);
      Serial.println(buffer);
    }

    virtual int knobs () {
      return 1;
    }
};

struct Hue : public Program {
  private:
    int hue;

  public:
    Hue() : hue(0) {}

    virtual void loop () {
      CHSV color = CHSV(hue, 255, BRIGHTNESS);
      for (int i = 0; i < NUM_LEDS; i++) {
        setLed(i, color);
      }
    }

    virtual void knob (int knob, bool clockwise) {
      sprintf(buffer, "knob: %d, hue: %d", clockwise, hue);
      Serial.println(buffer);
      hue += 10 * dir(clockwise);
    }

    virtual int knobs () {
      return 1;
    }
};

// smooth transition between the current color and the target color
void setLed(int i, CRGB color) {
  //  leds[i] = blend(leds[i], color, 10);
  leds[i] = color;
}

struct Button {
  private:
    bool state;
    int pin;

  public:
    // This requires pins with a pull-up resistor
    Button (int pin) : state(1), pin(pin) {
      pinMode(pin, INPUT);
    }

    bool click() {
      bool newState = digitalRead(pin) == LOW ? 0 : 1;
      bool oldState = state;
      state = newState;
      if (oldState == 0 && newState == 1) {
        return true;
      }
      return false;
    }
};


const int pinA = d2;
const int pinB = d1;
EC11 encoder;

void setup() {
  Serial.begin(115200);
  
  delay( 3000 ); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalPixelString );
  FastLED.setBrightness(  BRIGHTNESS );

  // There's no second GND so let's make D2 low
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);

  // pinA & pinB need pull-ups
  pinMode(pinA, INPUT);
  pinMode(pinB, INPUT);

  WiFi.begin("homeomorphism", "satisfy scandal helmet");
  //WiFi.begin("xoranS20", "helloworld");
}

void pinDidChange() {
  encoder.checkPins(digitalRead(pinA), digitalRead(pinB));
}

// starts with 0
void selectionIndicator(int selection) {
  const int row = 7;
  const int start = NUM_LEDS - row;
  for (int i = start; i < NUM_LEDS; i++) {
    setLed(i, CRGB::Black);
  }
  setLed(NUM_LEDS - 1 - selection, CRGB::White);
}

void wifiIndicator() {
  static bool prevConnected = false;
  
  bool connected = WiFi.status() == WL_CONNECTED;
  if(!connected) {
    if (prevConnected == true) {
      Serial.print("WIFI disconnected");
    }
    setLed(NUM_LEDS - 1 - 6, CRGB::Red);
  } else {
    if (prevConnected == false) {
      Serial.print("WIFI connected, IP address: ");
      Serial.println(WiFi.localIP());
      server.begin();
    }
  }
  prevConnected = connected;
}

int selection = 0;
bool active = false;
int deactivateAt;
void deactivate() {
  selection = 0;
  active = false;
}
void activate() {
  active = true;
  deactivateAt = millis() + 5000;
}
void checkActive() {
  if (millis() > deactivateAt) {
    deactivate();
  }
}

void loop () {
  static Hue hue;
  static Rainbow rainbow;
  static White white;
  static int brightness = BRIGHTNESS;
  static int currentProgram = 0;
  static Button button(d3);

  checkActive();

  Program* programs[] = { &rainbow, &hue, &white };
  const int numPrograms = sizeof(programs) / sizeof(*programs);
  Program* program = programs[currentProgram];

  // TODO:
  // I was trying to call it like this:
  // attachInterrupt(digitalPinToInterrupt(pinB), pinDidChange, CHANGE);
  // but the controller was crashing for some reason
  pinDidChange();

  char wifiInput = -1;
  WiFiClient client = server.available();
  if (client) {
    Serial.printf("client\n");
    wifiInput = client.read();
    Serial.printf("input: %c", wifiInput);
  }

  int click = button.click() || wifiInput == 'c';
  if (click) {
    activate();
    selection = (selection + 1) % (2 + program->knobs());

    sprintf(buffer, "button press, selection: %d", selection);
    Serial.println(buffer);
  }

  program->loop();

  int rotate = -1;
  EC11Event e;
  if (encoder.read(&e)) {
    rotate = e.type == EC11Event::StepCW;
  }
  if (wifiInput == 'l' || wifiInput == 'r') {
    rotate = wifiInput == 'r';
  }

  if (rotate == 0 || rotate == 1) {
    bool clockwise = rotate == 1;
    activate();
    switch (selection) {
      case 0: // brightness
        brightness = clamp(brightness + 10 * dir(clockwise), 0, 255);
        FastLED.setBrightness(  brightness );

        sprintf(buffer, "brightness: %d", brightness);
        Serial.println(buffer);

        break;
      case 1: // program
        currentProgram = (currentProgram + dir(clockwise) + numPrograms) % numPrograms;

        sprintf(buffer, "currentProgram: %d", currentProgram);
        Serial.println(buffer);

        break;
      default: // subprogram
        program->knob(selection - 2, clockwise);

        sprintf(buffer, "program->knob, clockwise: %d", clockwise);
        Serial.println(buffer);
    }
  }

  if (active) {
    selectionIndicator(selection);
  }

  wifiIndicator();

  FastLED.show();
}
