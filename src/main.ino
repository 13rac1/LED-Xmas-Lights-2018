#include "FastLED.h"
FASTLED_USING_NAMESPACE;

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

// clang-format off
#define DATA_PIN     3
#define LED_TYPE     WS2811
//#define COLOR_ORDER RGB

// TODO: Fix to work for 100 exactly without a final palette++ pixel.
#define NUM_LEDS    101
CRGB leds[NUM_LEDS];

#define BRIGHTNESS_NIGHT                      96
#define BRIGHTNESS_DAY                         3
#define FRAMES_PER_SECOND                    120
#define MILLIS_PER_FRAME  1000/FRAMES_PER_SECOND
// clang-format on

void setup() {
  delay(1000);

  // Time.zone(-8);  // Set Pacific Time Zone

  FastLED.addLeds<LED_TYPE, DATA_PIN>(leds, NUM_LEDS)
      .setCorrection(Tungsten40W);

  FastLED.setBrightness(BRIGHTNESS_NIGHT);

  // Serial.begin(9600);
}

uint8_t gHue = 0;
void loop() {
  EVERY_N_SECONDS(60) {
    // 4PM to 1 AM
    // if (Time.hour() > 16 || Time.hour() < 1) {
    //   FastLED.setBrightness(BRIGHTNESS_NIGHT);
    // } else {
    //   FastLED.setBrightness(BRIGHTNESS_DAY);
    // }
    gHue++;
  }

  EVERY_N_MILLISECONDS(MILLIS_PER_FRAME) { onEnterFrame(); }
  // onEnterFrame();
}

void onEnterFrame() {
  // xmas_rgbop();
  xmas_rgbop_range();
  addGlitter(80);

  FastLED.show();
}

void xmas_rgbop() {
  // Red, Green, Blue, Orange, and Purple cycling Xmas lights
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    switch ((gHue) % 5) {
      case 0:
        leds[i] = CRGB(0xfff276);
        break;
      case 1:
        leds[i] = CRGB(0x6aff7a);
        break;
      case 2:
        leds[i] = CRGB(0x5c65ff);
        break;
      case 3:
        leds[i] = CRGB(0xffae3f);
        break;
      case 4:
        leds[i] = CRGB(0xc33fff);
        break;
    }
  }
}

void addGlitter(fract8 chanceOfGlitter) {
  if (random8() < chanceOfGlitter) {
    leds[random16(NUM_LEDS)] += CRGB::White;
  }
}

void xmas_rgbop_range() {
  // xmas_rgbop() plus color count cycle, palette index shift, and a sine wave
  // based wipe. The palette count changes every time the wave AKA brightness
  // touches zero.

  // Example cycle (RGB to simplify)
  // # | Count | Palette Offset
  // 0 |     1 |              0
  // 1 |     2 |              0
  // 2 |     3 |              0
  // 3 |     2 |              0
  // 4 |     1 |              1

  // The five color palette to cycle through.
  const uint8_t color_total = 5;
  static CRGB colors[color_total];

  // Store if the pixel is using the current palette or the next palette.
  static bool next[NUM_LEDS];

  static bool start = true;
  if (start) {
    // Run init tasks
    start = false;

    colors[0] = CRGB(0xFF0000);
    colors[1] = CRGB(0x00FF00);
    colors[2] = CRGB(0x0000FF);
    colors[3] = CRGB(0xFF6600);
    // Reduce red for color correction and get a better looking purple.
    colors[4] = CRGB(0x6600FF);

    // Set all next to false at start.
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
      next[i] = false;
    }
  }

  // Current number of colors to display from the palette.
  static uint8_t color_count = 1;
  // The amount to change the color count, positive or negative one only.
  static int8_t sign = 1;
  // The zero color of the palette, increments every cycle.
  static uint8_t palette_offset = 0;

  int8_t brightness   = 0;
  uint8_t color_index = 0;
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    // Find the brightness using a sine wave.
    // WARNING: A max of 254 works, 255 fails to create a 0 on the Arduino Nano
    brightness                 = beatsin8(16, 0, 254, 0, NUM_LEDS * 2 - i * 2);
    uint16_t scaled_brightness = brightness * 8;
    if (scaled_brightness > 255) {
      brightness = 255;
    } else {
      brightness = (uint8_t)scaled_brightness;
    }

    // Switch to the next palette, when pixel is dark
    if (brightness == 0) {
      next[i] = true;
    }

    if (next[i]) {
      // Load the next palette when next==true.
      // Modified copy of the total strip switch.
      // TODO: Refactor to a function call to get the current palette.
      uint8_t local_color_count    = color_count;
      uint8_t local_palette_offset = palette_offset;
      local_color_count += sign;
      // When the count of colors is one, increment the palette offset.
      if (local_color_count == 1) {
        local_palette_offset++;
        if (local_palette_offset == color_total) {
          local_palette_offset = 0;
        }
      }
      color_index = i % local_color_count;
      leds[i]     = colors[(color_index + local_palette_offset) % color_total];
    } else {
      // Find the color in the current palette, limiting by count and total.
      color_index = i % color_count;
      leds[i]     = colors[(color_index + palette_offset) % color_total];
    }
    // Reduce the brightness depending on the sine value.
    leds[i].nscale8_video(brightness);
  }

  bool check = true;
  // check all pixels have changed.
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    if (!next[i]) {
      check = false;
    }
  }

  // Time to change palette?
  if (check && brightness == 0) {
    // This is ugh, we are using the brightness variable far from it's original
    // init to get the brightness used in the last pixel on the strip. Change
    // the entire set when the last pixel has zero brightness and all pixels are
    // palette shifted.
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
      // Set all to false again.
      next[i] = false;
    }

    color_count += sign;
    if (color_count == color_total) {
      // If we've reached the max count, start going down.
      sign = -1;
    } else if (color_count == 1) {
      // If we've reached the minimum count, start going up.
      sign = 1;
      // Next palette, limit to total number of colors
      palette_offset++;
      if (palette_offset == color_total) {
        palette_offset = 0;
      }
    }
  }
}
