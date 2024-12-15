/*
 * Wordclock Bärndütsch - Arduino Nano
 * 
 * This sketch is ment to be flashed on an Arduino Nano in conjunction with:
 * - An RTC Module (DS3231) connected via I2C 
 * - A phototransistor connected to A6 (pulled low with a 750k resistor) and powered though Pin A2
 * - A NeuPixel LedStrip (or WS2812 compatiple LedStrip) connected via Pin 9 in an 11x11 Grid
 *
 *    1 2 3 4 5 6 7 8 9 0 1
 * A  E S   I S C H   F Ü F   ->  0-10
 * B    Z Ä     V I E R T U   <- 21-11
 * C    Z W Ä N Z G   V O R   -> 22-32
 * D    A B     H A U B I     <- 43-33 
 * E    E I S     Z W Ö I     -> 44-54
 * F    D R Ü     V I E R I   <- 65-55
 * G  F Ü F I   S Ä C H S I   -> 66-76
 * H  S I B N I   A C H T I   <- 87-77
 * I    N Ü N I   Z Ä N I     -> 88-98
 * J  E U F I   Z W Ö U F I   <- 109-99
 * K  A M   M O R G E A B E   -> 110-120
 *
 * Row indexes start with 0,11,22,33,...,110
 *  ZigZag is used, every second row is backwards (index)
 */

#include <RTClib.h>
#include <DST_RTC.h>
#include <Smooth.h>
#include <Adafruit_NeoPixel.h>

#define PIN_NEO_PIXEL 9      // Arduino pin that connects to NeoPixel
#define NUM_PIXELS 121       // The number of LEDs (pixels)
#define PIN_TRANSISTOR A6    // Arduino pin for the phototransistor
#define POWER_TRANSISTOR A2  // Arduino power pin for the phototransistor

// sigmoid constants
const float s_max = 240.0;     // 254.0
const float s_slope = -0.085;  //  -0.04
const float s_attack = 3.9;    //   3.6

const char rulesDST[] = "EU";

// Word positions defined by      {index, length}
const byte w_es[2] PROGMEM = { 0, 2 };
const byte w_isch[2] PROGMEM = { 3, 4 };
const byte w_fuef[2] PROGMEM = { 8, 3 };
const byte w_zae[2] PROGMEM = { 19, 2 };
const byte w_viertu[2] PROGMEM = { 11, 6 };
const byte w_zwaenzg[2] PROGMEM = { 23, 6 };
const byte w_vor[2] PROGMEM = { 30, 3 };
const byte w_ab[2] PROGMEM = { 41, 2 };
const byte w_haubi[2] PROGMEM = { 34, 5 };
const byte w_am[2] PROGMEM = { 110, 2 };
const byte w_morge[2] PROGMEM = { 113, 5 };
const byte w_abe[2] PROGMEM = { 118, 3 };

// All hours in a 2D Array            0/12      ,  eis     , zwoei    , drue     ,  4       ,  5       ,  6       ,  7       ,  8       ,  9       ,  10      ,  11
const byte w_stunde[12][2] PROGMEM = { { 99, 6 }, { 45, 3 }, { 50, 4 }, { 62, 3 }, { 55, 5 }, { 66, 4 }, { 71, 6 }, { 83, 5 }, { 77, 5 }, { 89, 4 }, { 94, 4 }, { 106, 4 } };

Adafruit_NeoPixel NeoPixel(NUM_PIXELS, PIN_NEO_PIXEL, NEO_GRB + NEO_KHZ800);
RTC_DS3231 rtc;
DST_RTC dst_rtc;
Smooth brightness(41);

DateTime now;

long lastMillis = 0;

void setup() {
  delay(2000);

  Serial.begin(9600);

  pinMode(PIN_TRANSISTOR, INPUT);
  pinMode(POWER_TRANSISTOR, OUTPUT);
  digitalWrite(POWER_TRANSISTOR, HIGH);

  Serial.print(F("Starting NeoPixel... millis: "));
  Serial.println(millis());
  NeoPixel.begin();
  Serial.print(F("Starting RTC... millis: "));
  Serial.println(millis());
  rtc.begin();

  Serial.print(F("Get initial time... millis: "));
  Serial.println(millis());
  now = dst_rtc.calculateTime(rtc.now());
}

void loop() {
  if (Serial.available() > 0) {
    requestDateBySerialAndSet();
    delay(100);
    serialFlush();
  }

  adjustBrightness(readBrightness());
  updateNeoPixel();
  
  if (millis() > lastMillis + 1000) {    
    now = dst_rtc.calculateTime(rtc.now());
    printLogs();
    lastMillis = millis();
  }
}

float readBrightness() {
  float sensorReading = 0;
  for (int i = 0; i < 10; i++) {
    sensorReading += analogRead(PIN_TRANSISTOR);
    delay(1);
  }
  return sensorReading / 10;
}

void adjustBrightness(float sensorReading) {
  // Set brightness based on sigmoid activation function. See sigmoid-light.ggb for visualization
  brightness.add(s_max / (1 + pow(2.718, (s_slope * sensorReading + s_attack))));
}

void updateNeoPixel() {
  byte hour = now.hour();
  byte minute = now.minute();

  NeoPixel.clear();

  if (minute < 5) {
    addWord(w_es);
    addWord(w_isch);
  } else if (minute < 10) {
    addWord(w_fuef);
    addWord(w_ab);
  } else if (minute < 15) {
    addWord(w_zae);
    addWord(w_ab);
  } else if (minute < 20) {
    addWord(w_viertu);
    addWord(w_ab);
  } else if (minute < 25) {
    addWord(w_zwaenzg);
    addWord(w_ab);
  } else if (minute < 30) {
    addWord(w_fuef);
    addWord(w_vor);
    addWord(w_haubi);
  } else if (minute < 35) {
    addWord(w_es);
    addWord(w_isch);
    addWord(w_haubi);
  } else if (minute < 40) {
    addWord(w_fuef);
    addWord(w_ab);
    addWord(w_haubi);
  } else if (minute < 45) {
    addWord(w_zwaenzg);
    addWord(w_vor);
  } else if (minute < 50) {
    addWord(w_viertu);
    addWord(w_vor);
  } else if (minute < 55) {
    addWord(w_zae);
    addWord(w_vor);
  } else {
    addWord(w_fuef);
    addWord(w_vor);
  }

  if (minute >= 25) {
    hour = (hour + 1);
  }

  addWord(w_stunde[hour % 12]);

  if (hour > 5 && hour < 10) {
    addWord(w_am);
    addWord(w_morge);
  } else if (hour > 17 && hour < 22) {
    addWord(w_am);
    addWord(w_abe);
  }

  NeoPixel.show();
}

void addWord(const byte theword[2]) {
  byte start = pgm_read_byte(theword);
  byte len = pgm_read_byte(theword + 1);
  byte bright = (int)ceil(brightness.get_avg());

  for (int i = start; i < start + len; i++) {
    NeoPixel.setPixelColor(i, NeoPixel.Color(bright, (int)floor(bright * 0.666), bright));
  }
}

void printLogs() {
  Serial.print(F("millis: "));
  Serial.print(millis());
  Serial.print(F(" - Timestamp: "));
  Serial.print(now.timestamp());
  Serial.print(" - brightness: ");
  Serial.print((int)ceil(brightness.get_avg()), DEC);
  Serial.print(" light reading: ");
  Serial.println(readBrightness());
}

bool requestDateBySerialAndSet(void) {
  uint8_t tSecond, tMinute, tHour, tDay, tMonth, tYear;
  char tInputBuffer[17];

  serialFlush();
  flashCorners();

  Serial.println(F("Enter the time in format: \"YY MM DD HH MM SS\" - Timeout is 30 seconds"));
  Serial.setTimeout(30000);

  size_t tReadLength = Serial.readBytes(tInputBuffer, 17);  // read exactly 17 characters
  if (tReadLength == 17 && tInputBuffer[14] == ' ') {

    sscanf_P(tInputBuffer, PSTR("%2hhu %2hhu %2hhu %2hhu %2hhu %2hhu"), &tYear, &tMonth, &tDay, &tHour, &tMinute, &tSecond);

    rtc.adjust(DateTime(tYear, tMonth, tDay, tHour, tMinute, tSecond));
    Serial.print(F("Time set to: "));
    Serial.println(rtc.now().timestamp());
    return true;
  }
  Serial.println(F("Clock has not been changed, press reset for next try."));
  return false;
}

void serialFlush() {
  while (Serial.available() > 0) {
    char t = Serial.read();
  }
}

void flashCorners() {
  NeoPixel.clear();
  for (int i = 0; i < 10; i++) {
    int light = i * 10;
    NeoPixel.clear();
    NeoPixel.setPixelColor(0, NeoPixel.Color(light, (int)floor(light * 0.55), light));
    NeoPixel.setPixelColor(10, NeoPixel.Color(light, (int)floor(light * 0.55), light));
    NeoPixel.setPixelColor(110, NeoPixel.Color(light, (int)floor(light * 0.55), light));
    NeoPixel.setPixelColor(120, NeoPixel.Color(light, (int)floor(light * 0.55), light));
    NeoPixel.show();
    delay(200);
  }
}
