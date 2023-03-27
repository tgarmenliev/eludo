#include <FastLED.h>

#define LED_PIN     7
#define NUM_LEDS    30
#define BRIGHTNESS  24
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

#define UPDATES_PER_SECOND 100

CRGB leds[NUM_LEDS];

CRGB Yellow1 = CRGB(255,105,0);

int leds_cnt[NUM_LEDS];

void setup() {
    delay( 3000 ); // power-up safety delay
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(  BRIGHTNESS );

}

void loop() {
  // put your main code here, to run repeatedly:
  // Turn the LED on, then pause
  leds[0] = Yellow1;
  FastLED.show();
  delay(300);
  leds[0] = CRGB::Black;
  FastLED.show();
  delay(300);
  leds[0] = CRGB::Red;
  FastLED.show();
  delay(300);
  leds[0] = CRGB::Black;
  FastLED.show();
  delay(300);
  leds[0] = CRGB::Green;
  FastLED.show();
  delay(300);
  leds[0] = CRGB::Black;
  FastLED.show();
  delay(300);
  leds[0] = CRGB::Blue;
  FastLED.show();
  delay(300);
  // Now turn the LED off, then pause
  leds[0] = CRGB::Black;
  FastLED.show();
  delay(300);
}
