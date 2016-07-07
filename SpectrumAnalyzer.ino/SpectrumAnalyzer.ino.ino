// LED Audio Spectrum Analyzer Display
// 
// Creates an impressive LED light show to music input
//   using Teensy 3.1 with the OctoWS2811 adaptor board
//   http://www.pjrc.com/store/teensy31.html
//   http://www.pjrc.com/store/octo28_adaptor.html
//
// Line Level Audio Input connects to analog pin A3
//   Recommended input circuit:
//   http://www.pjrc.com/teensy/gui/?info=AudioInputAnalog
//
// This example code is in the public domain.

#include "FastLED.h"

FASTLED_USING_NAMESPACE

// FastLED "100-lines-of-code" demo reel, showing just a few 
// of the kinds of animation patterns you can quickly and easily 
// compose using FastLED.  
//
// This example also shows one easy way to define multiple 
// animations patterns and have them automatically rotate.
//
// -Mark Kriegsman, December 2014

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif
#include <OctoWS2811.h>
#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>

// The display size and color to use
const unsigned int matrix_width = 63;
const unsigned int matrix_height = 6;
const unsigned int myColor = 0x400020;

// These parameters adjust the vertical thresholds
const float maxLevel = 0.5;      // 1.0 = max, lower is more "sensitive"
const float dynamicRange = 40.0; // total range to display, in decibels
const float linearBlend = 0.3;   // useful range is 0 to 0.7

//// OctoWS2811 objects
//const int ledsPerPin = matrix_width * matrix_height / 6;
//DMAMEM int displayMemory[ledsPerPin*6];
//int drawingMemory[ledsPerPin*6];
//const int config = WS2811_GRB | WS2811_800kHz;
//OctoWS2811 leds(ledsPerPin, displayMemory, drawingMemory, config);

#define NUM_STRIPS 6
#define NUM_LEDS_PER_STRIP 63
#define NUM_LEDS NUM_LEDS_PER_STRIP * NUM_STRIPS
//#define CLK_PIN   4
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];
#define BRIGHTNESS          96
#define FRAMES_PER_SECOND  120
// Audio library objects
AudioInputAnalog         adc1(A3);       //xy=99,55
AudioAnalyzeFFT1024      fft;            //xy=265,75
AudioConnection          patchCord1(adc1, fft);


// This array holds the volume level (0 to 1.0) for each
// vertical pixel to turn on.  Computed in setup() using
// the 3 parameters above.
float thresholdVertical[matrix_height];

// This array specifies how many of the FFT frequency bin
// to use for each horizontal pixel.  Because humans hear
// in octaves and FFT bins are linear, the low frequencies
// use a small number of bins, higher frequencies use more.
int frequencyBinsHorizontal[matrix_width] = {
   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
   2,  2,  2,  2,  2,  2,  2,  2,  2,  3,
   3,  3,  3,  3,  4,  4,  4,  4,  4,  5,
   5,  5,  6,  6,  6,  7,  7,  7,  8,  8,
   9,  9, 10, 10, 11, 12, 12, 13, 14, 15,
  15, 16, 17, 18, 19, 20, 22, 23, 24, 25
};



// Run setup once
void setup() {
  // the audio library needs to be given memory to start working
  AudioMemory(12);

  delay(3000); // 3 second delay for recovery
  
  // tell FastLED about the LED strip configuration
  //FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds<NEOPIXEL, 2>(leds, 0, NUM_LEDS_PER_STRIP);

  // tell FastLED there's 60 NEOPIXEL leds on pin 11, starting at index 60 in the led array
  FastLED.addLeds<NEOPIXEL, 14>(leds, NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP);

  // tell FastLED there's 60 NEOPIXEL leds on pin 12, starting at index 120 in the led array
  FastLED.addLeds<NEOPIXEL, 7>(leds, 2 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 8>(leds, 3 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 6>(leds, 4 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 20>(leds, 5 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP);
  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
}

// A simple xy() function to turn display matrix coordinates
// into the index numbers OctoWS2811 requires.  If your LEDs
// are arranged differently, edit this code...
unsigned int xy(unsigned int x, unsigned int y) {
//  if ((y & 1) == 0) {
//    // even numbered rows (0, 2, 4...) are left to right
//    return y * matrix_width + x;
//  } else {
//    // odd numbered rows (1, 3, 5...) are right to left
//    return y * matrix_width + matrix_width - 1 - x;
//  }
  return y * matrix_width + x;
}

// Run repetitively
void loop() {
  unsigned int x, y, freqBin;
  float level;

  if (fft.available()) {
    // freqBin counts which FFT frequency data has been used,
    // starting at low frequency
    freqBin = 0;

    for (x=0; x < matrix_width; x++) {
      // get the volume for each horizontal pixel position
      level = fft.read(freqBin, freqBin + frequencyBinsHorizontal[x] - 1);

      // uncomment to see the spectrum in Arduino's Serial Monitor
      // Serial.print(level);
      // Serial.print("  ");

      for (y=0; y < matrix_height; y++) {
        // for each vertical pixel, check if above the threshold
        // and turn the LED on or off
        if (level >= thresholdVertical[y]) {
          //leds.setPixel(xy(x, y), myColor);
          leds[xy(x, y)] = CRGB::Red;
        } else {
          //leds.setPixel(xy(x, y), 0x000000);
          leds[xy(x, y)] = CRGB::Blue;
        }
      }
      // increment the frequency bin count, so we display
      // low to higher frequency from left to right
      freqBin = freqBin + frequencyBinsHorizontal[x];
    }
    // after all pixels set, show them all at the same instant
    
    FastLED.show();
    // Serial.println();
  }
}


// Run once from setup, the compute the vertical levels
void computeVerticalLevels() {
  unsigned int y;
  float n, logLevel, linearLevel;

  for (y=0; y < matrix_height; y++) {
    n = (float)y / (float)(matrix_height - 1);
    logLevel = pow10f(n * -1.0 * (dynamicRange / 20.0));
    linearLevel = 1.0 - n;
    linearLevel = linearLevel * linearBlend;
    logLevel = logLevel * (1.0 - linearBlend);
    thresholdVertical[y] = (logLevel + linearLevel) * maxLevel;
  }
}



