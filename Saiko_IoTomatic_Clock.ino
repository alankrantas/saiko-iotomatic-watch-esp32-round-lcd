#include "secrets.h"  // imports SECRET_SSID and SECRET_PASS

/*
Create a <secrets.h> in the same project:

#define SECRET_SSID "your_WiFi_ssid"
#define SECRET_PASS "your_WiFi_password"
*/

// user configuration

#define DEMO_MODE false     // demo display mode (fixed time, will not connect wifi)
#define BENCHMARK false     // print dial drawing time (ms) into serial port
#define BAUD_RATE 115200    // serial port baud rate
#define CONNECT_TIMEOUT 30  // WiFi connection timeout (seconds)

#define NTP_SERVER "pool.ntp.org"  // NTP server, for example, "pool.ntp.org"
#define NTP_HOUR_OFFSET 0          // timezone offset (hours; 1 = +1, -1 = -1)

#define TFT_CS 5        // GC9A01A CS pin
#define TFT_DC 21       // GC9A01A DC pin
#define TFT_RST 22      // GC9A01A reset pin
#define TFT_WIDTH 240   // GC9A01A width
#define TFT_HEIGHT 240  // GC9A01A height
#define TFT_ROTATION 0  // GC9A01A rotation (0~3)

// import dependencies

#include <math.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSerifItalic9pt7b.h>
#include <Fonts/TomThumb.h>

// configuration for the watch face

// dial specs

#define DIAL_WIDTH 192
#define DIAL_HEIGHT 192
#define BEZEL_TICK_W 6
#define BEZEL_TICK_PAD 5
#define BEZEL_NOON_PAD 5
#define BEZEL_DOT_SELF_R 2
#define BEZEL_DOT_POS_R_P 0.3
#define DIAL_RING_W 10
#define INNER_TICK_W 6
#define INNER_TICK_NOON_W 6
#define INNER_TICK_PAD 3
#define ROUND_DOT_POS_R_P 0.84
#define ROUND_DOT_SELF_R 6
#define ROUND_DOT_BORDER_W 2
#define LOGO_NAME "SAIKO"
#define LOGO_POS_R_P 0.35
#define DESCRIPTION "IoTomatic"
#define DESCRIPTION_POS_R_P 0.38
#define DATE_W 18
#define DATE_H 14
#define WEEK_W 28
#define WEEK_H 14
#define HANDS_AXIS_R 1
#define HOUR_MIN_HAND_BORDER 2
#define HOUR_MIN_HAND_POINT_W 3
#define HOUR_MIN_HAND_POINT_H 4
#define HOUR_HAND_A_W 16
#define HOUR_HAND_A_H 6
#define HOUR_HAND_B_ALT 8
#define HOUR_HAND_C_W 24
#define HOUR_HAND_C_H 12
#define HOUR_HAND_D_BASE 16
#define HOUR_HAND_D_ALT 10
#define MIN_HAND_A_ALT 6
#define MIN_HAND_B_W 18
#define MIN_HAND_B_H 18
#define MIN_HAND_B_OPPS_R 8
#define MIN_HAND_C_W 14
#define MIN_HAND_C_H 14
#define MIN_HAND_D_W 30
#define MIN_HAND_D_H 8
#define MIN_HAND_E_BASE 24
#define MIN_HAND_E_ALT 22
#define MIN_HAND_F_ALT 8
#define SECOND_HAND_VIBRATION 6  // 3 Hz for second hand
#define SECOND_HAND_R_P 0.92
#define SECOND_HAND_BALANCE_R_P 0.42
#define SECOND_HAND_BALANCE_DOT_SELF_R 5
#define SECOND_HAND_BALANCE_DOT_BORDER_W 1
#define SECOND_HAND_BASE_R 5

// dial colors

#define BEZEL_COLOR 0x4E4B47
#define DIAL_COLOR 0x0ABAB5
#define DIAL_RING_COLOR 0x089E9A
#define BEZEL_TICK_COLOR 0xD3D3D3
#define INNER_TICK_COLOR 0x4E4B47
#define ROUND_DOT_COLOR 0xFAEED8
#define ROUND_DOT_BORDER 0x6A6B6E
#define LOGO_COLOR 0x56534E
#define DESCRIPTION_COLOR 0x675b5A
#define DATE_COLOR 0x4E4B47
#define DATE_SUN_COLOR 0xD90000
#define HOUR_MIN_HAND_BORDER_COLOR 0xBE8A0B
#define HOUR_MIN_HAND_FRONT_COLOR 0xFAEED8
#define SECOND_HAND_COLOR 0x373130
#define SECOND_HAND_BALANCE_COLOR 0xFAEED8
#define SECOND_HAND_AXIS_COLOR 0xB5B6B8

// calculated specs

const uint16_t TFT_CENTER_X = round(TFT_WIDTH / 2);
const uint16_t TFT_CENTER_Y = round(TFT_HEIGHT / 2);
const uint16_t TFT_R = TFT_CENTER_X < TFT_CENTER_Y ? TFT_CENTER_X : TFT_CENTER_Y;
const uint16_t DIAL_CENTER_X = round(DIAL_WIDTH / 2);
const uint16_t DIAL_CENTER_Y = round(DIAL_HEIGHT / 2);
const uint16_t DIAL_RING_R = DIAL_CENTER_X < DIAL_CENTER_Y ? DIAL_CENTER_X : DIAL_CENTER_Y;
const uint16_t BEZEL_R = TFT_R;
const uint16_t BEZEL_W = TFT_R - DIAL_RING_R;
const uint16_t DIAL_INNER_R = DIAL_RING_R - DIAL_RING_W;
const uint16_t ROUND_DOT_R = round(DIAL_INNER_R * ROUND_DOT_POS_R_P);

// '10' on bezel ring, 23x26px
const unsigned char bitmap_bezel_10[] PROGMEM = {
  0xff, 0x7f, 0xfe, 0xfe, 0x07, 0xfe, 0xfe, 0x03, 0xfe, 0xfe, 0x01, 0xfe, 0xff, 0x81, 0xfe, 0xfe,
  0x01, 0xfe, 0xf8, 0x07, 0xfe, 0xe0, 0x1f, 0xfe, 0xc0, 0x7f, 0xfe, 0x80, 0xff, 0xfe, 0x83, 0xff,
  0xfe, 0xcf, 0xfe, 0x1e, 0xff, 0xf8, 0x0e, 0xff, 0xe0, 0x06, 0xff, 0xc0, 0x02, 0xff, 0x01, 0xc2,
  0xfe, 0x03, 0xe0, 0xfe, 0x0f, 0xe0, 0xfe, 0x3f, 0xe0, 0xfe, 0x3f, 0x80, 0xfe, 0x1f, 0x02, 0xfe,
  0x0c, 0x06, 0xff, 0x00, 0x0e, 0xff, 0x80, 0x3e, 0xff, 0x80, 0xfe, 0xff, 0xe1, 0xfe
};

// '20' on bezel ring, 25x31px
const unsigned char bitmap_bezel_20[] PROGMEM = {
  0xff, 0xff, 0xff, 0x80, 0xff, 0xf3, 0xff, 0x80, 0xff, 0xe0, 0xff, 0x80, 0xff, 0xe0, 0x7f, 0x80,
  0xff, 0xc0, 0x3f, 0x80, 0xff, 0xc0, 0x37, 0x80, 0xff, 0x84, 0x31, 0x80, 0xff, 0x84, 0x20, 0x80,
  0xff, 0x0c, 0x20, 0x80, 0xfe, 0x0c, 0x38, 0x00, 0xfe, 0x18, 0x78, 0x80, 0xff, 0x38, 0x70, 0x80,
  0xff, 0xf8, 0x60, 0x80, 0xff, 0xf8, 0x01, 0x80, 0xff, 0xfc, 0x03, 0x80, 0xf8, 0xfe, 0x03, 0x80,
  0xe0, 0x3f, 0x87, 0x80, 0xe0, 0x0f, 0xff, 0x80, 0xc0, 0x03, 0xff, 0x80, 0x83, 0x01, 0xff, 0x80,
  0x87, 0x80, 0xff, 0x80, 0x8f, 0xe0, 0x7f, 0x80, 0x8f, 0xf8, 0x7f, 0x80, 0x83, 0xf8, 0x7f, 0x80,
  0x81, 0xf8, 0x7f, 0x80, 0xc0, 0x70, 0xff, 0x80, 0xe0, 0x00, 0xff, 0x80, 0xf8, 0x01, 0xff, 0x80,
  0xfe, 0x03, 0xff, 0x80, 0xff, 0x07, 0xff, 0x80, 0xff, 0xff, 0xff, 0x80
};

// '30' on bezel ring, 29x16px
const unsigned char bitmap_bezel_30[] PROGMEM = {
  0xe0, 0x1f, 0xc0, 0x38, 0xc0, 0x0f, 0x80, 0x18, 0x80, 0x0f, 0x00, 0x08, 0x80, 0x07, 0x00, 0x08,
  0x87, 0x87, 0x0f, 0x08, 0x87, 0x87, 0x0f, 0x08, 0x87, 0x87, 0x00, 0x88, 0x87, 0x87, 0x80, 0xf8,
  0x87, 0x87, 0x80, 0xf8, 0x87, 0x87, 0x00, 0x18, 0x87, 0x87, 0x0f, 0x08, 0x87, 0x87, 0x0f, 0x08,
  0x80, 0x07, 0x00, 0x08, 0x80, 0x0f, 0x80, 0x18, 0xc0, 0x0f, 0x80, 0x18, 0xf0, 0x7f, 0xe0, 0xf8
};

// '40' on bezel ring, 23x27px
const unsigned char bitmap_bezel_40[] PROGMEM = {
  0xff, 0xff, 0xfe, 0xff, 0x07, 0xfe, 0xfc, 0x03, 0xfe, 0xf0, 0x01, 0xfe, 0xe0, 0x21, 0xfe, 0x80,
  0x70, 0xfe, 0x81, 0xf0, 0xfe, 0x07, 0xf8, 0x7e, 0x0f, 0xf0, 0x7e, 0x0f, 0xc0, 0xfe, 0x87, 0x80,
  0xfe, 0x86, 0x01, 0xfe, 0xc0, 0x07, 0xfe, 0xe0, 0x1f, 0xfe, 0xe0, 0x3e, 0x66, 0xf9, 0xfc, 0x02,
  0xff, 0xfc, 0x02, 0xff, 0xfc, 0x06, 0xff, 0xf0, 0x0e, 0xff, 0xc0, 0x0e, 0xff, 0x80, 0x86, 0xff,
  0x01, 0x86, 0xff, 0x00, 0x02, 0xff, 0x80, 0x00, 0xff, 0x80, 0x00, 0xff, 0xff, 0x06, 0xff, 0xff,
  0xfe
};

// '50' on bezel ring, 26x30px
const unsigned char bitmap_bezel_50[] PROGMEM = {
  0xff, 0xff, 0xff, 0xc0, 0xff, 0xf8, 0x7f, 0xc0, 0xff, 0xf0, 0x1f, 0xc0, 0xff, 0xe0, 0x07, 0xc0,
  0xff, 0xc0, 0x03, 0xc0, 0xff, 0xc3, 0x80, 0xc0, 0xff, 0x87, 0xc0, 0x40, 0xff, 0x87, 0xf0, 0x40,
  0xff, 0x87, 0xf8, 0x40, 0xff, 0x81, 0xf8, 0x40, 0xff, 0xc0, 0xf8, 0x40, 0xff, 0xe0, 0x30, 0xc0,
  0xff, 0xf0, 0x00, 0xc0, 0xff, 0xfc, 0x01, 0xc0, 0xf9, 0xff, 0x03, 0xc0, 0xf0, 0xff, 0x87, 0xc0,
  0xf0, 0xdf, 0xff, 0xc0, 0xe1, 0x03, 0xff, 0xc0, 0xc0, 0x01, 0xff, 0xc0, 0xc2, 0x00, 0x7f, 0xc0,
  0x84, 0x20, 0x7f, 0xc0, 0x80, 0x38, 0x7f, 0xc0, 0x00, 0x7c, 0x7f, 0xc0, 0x80, 0x78, 0x7f, 0xc0,
  0xe0, 0x78, 0x7f, 0xc0, 0xf8, 0x00, 0xff, 0xc0, 0xfc, 0x00, 0xff, 0xc0, 0xff, 0x01, 0xff, 0xc0,
  0xff, 0x83, 0xff, 0xc0, 0xff, 0xff, 0xff, 0xc0
};

const uint8_t BEAT_DELAY = round(1000 / SECOND_HAND_VIBRATION);
const uint8_t SECOND_DELTA_DEGREE = round(6 / SECOND_HAND_VIBRATION);

const uint8_t npt_update_interval = 20;  // NTP client will update itself every 20 minutes
const String weekDays[7] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
const char ssid[] = SECRET_SSID;
const char pass[] = SECRET_PASS;

// internal variables

String weekday = weekDays[0];
uint8_t day = 31;
uint8_t hour = 10;
uint8_t minute = 9;
uint8_t second = 42;
uint8_t second_prev = 42;
uint8_t second_vibrate_count = 0;
unsigned long t;
int16_t text_x, text_y;
uint16_t text_w, text_h;
static int prev_NTP_res = 0;
bool firstDialDrawn = false;
bool timeGet = false;

// functionalities and devices

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 canvas(DIAL_WIDTH, DIAL_HEIGHT);  // buffer
WiFiUDP ntpUDP;
NTPClient timeClient(
  ntpUDP,
  NTP_SERVER,
  NTP_HOUR_OFFSET * 60 * 60,
  npt_update_interval * 60 * 1000);

// setup
void setup() {
  // set ESP32 freq to save power
  setCpuFrequencyMhz(80);

  // initialize
  initializeDevice();
  if (!DEMO_MODE) initializeClock();
  drawInitialDial();

  if (DEMO_MODE) Serial.println("[demo mode]");
  else t = millis();
}

// main loop
void loop() {
  if (!DEMO_MODE) {

    // read system time
    timeGet = getTime();

    // check and force update time
    if (timeGet) updateTime();
  }

  if ((!DEMO_MODE || !firstDialDrawn) && millis() - t >= BEAT_DELAY) {

    // draw dial with updated time
    t = millis();
    drawDial();
    if (BENCHMARK) Serial.printf("[benchmark] dial drawing: %d ms\n", millis() - t);

    if (!firstDialDrawn) firstDialDrawn = true;

    if (!timeGet && second_vibrate_count < SECOND_HAND_VIBRATION - 1)
      second_vibrate_count += SECOND_DELTA_DEGREE;
  }
}

// initialize device
void initializeDevice() {
  // serial
  Serial.begin(BAUD_RATE);

  // initialize screen and buffer
  tft.begin();
  tft.setRotation(TFT_ROTATION);
  tft.fillScreen(GC9A01A_BLACK);
  tft.fillScreen(GC9A01A_BLACK);
  tft.setTextWrap(false);
  tft.cp437(true);
  canvas.fillScreen(GC9A01A_BLACK);
  canvas.setTextWrap(false);
  canvas.cp437(true);
}

// initialize clock
void initializeClock() {
  // connect to WiFi and initialize NTP client
  delay(500);
  Serial.printf("Connecting to WiFi '%s'...\n", SECRET_SSID);

  tft.setFont(&FreeMonoBold9pt7b);
  tft.setTextSize(1);
  tft.getTextBounds("Connecting WiFi...",
                    0,
                    0,
                    &text_x,
                    &text_y,
                    &text_w,
                    &text_h);

  uint8_t count = 0;
  bool startup_switch = true;
  WiFi.disconnect();
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED * 2) {
    if (count >= CONNECT_TIMEOUT) ESP.restart();

    count++;
    Serial.print(".");

    if (startup_switch)
      tft.setTextColor(startup_switch ? GC9A01A_WHITE : GC9A01A_BLACK);

    tft.setCursor(round(TFT_CENTER_X - text_w / 2),
                  round(TFT_CENTER_Y - text_h / 2));
    tft.print("Connecting WiFi...");

    startup_switch = !startup_switch;
    delay(500);
  }

  Serial.printf("\nConnected.\n[IP] %s\n", WiFi.localIP().toString());

  timeClient.begin();
}

// read date and time from NTP client
bool getTime() {
  bool timeGet = false;

  second = timeClient.getSeconds();
  if (second >= 60) second = 0;

  // update time every second
  if (second != second_prev) {
    timeGet = true;
    second_vibrate_count = 0;

    time_t rawtime = timeClient.getEpochTime();
    struct tm *ti = localtime(&rawtime);

    int year = ti->tm_year + 1900;
    int month = ti->tm_mon + 1;
    day = ti->tm_mday;
    hour = ti->tm_hour;
    minute = ti->tm_min;
    weekday = weekDays[ti->tm_wday];
    Serial.printf("[time] %4d-%02d-%02d (%s) %02d:%02d:%02d\n", year, month, day, weekday, hour, minute, second);
  }

  second_prev = second;
  return timeGet;
}

// check and force update time (do so when time is not set or per hour)
void updateTime() {
  if (!timeClient.isTimeSet() || (minute == 0 && second == 0)) {
    if (!timeClient.update())
      timeClient.forceUpdate();
  }
}

// draw the initial parts that do not require redrawing
void drawInitialDial() {
  // diver bezel ring
  tft.fillCircle(TFT_CENTER_X,
                 TFT_CENTER_Y,
                 BEZEL_R,
                 RGB565(BEZEL_COLOR));
  canvas.fillCircle(DIAL_CENTER_X,
                    DIAL_CENTER_Y,
                    BEZEL_R,
                    RGB565(BEZEL_COLOR));

  // ticks on diver bezel ring
  for (uint8_t tick_pos = 5; tick_pos <= 55; tick_pos += 10) {
    drawAngledBox(tft,
                  TFT_CENTER_X,
                  TFT_CENTER_Y,
                  DIAL_RING_R + BEZEL_TICK_PAD,
                  BEZEL_TICK_W, BEZEL_W - BEZEL_TICK_PAD * 2,
                  handToDegree(tick_pos, false),
                  RGB565(BEZEL_TICK_COLOR));
    drawAngledBox(canvas,
                  DIAL_CENTER_X,
                  DIAL_CENTER_Y,
                  DIAL_RING_R + BEZEL_TICK_PAD,
                  BEZEL_TICK_W, BEZEL_W - BEZEL_TICK_PAD * 2,
                  handToDegree(tick_pos, false),
                  RGB565(BEZEL_TICK_COLOR));
  }

  // bezel noon (triangle on 12 o'clock)
  const uint16_t x_noon = TFT_CENTER_X;
  const uint16_t y_noon = TFT_CENTER_Y - DIAL_RING_R - BEZEL_TICK_PAD;
  const float x_delta = (BEZEL_W - BEZEL_TICK_PAD) * cos(50 * M_PI / 180);
  const float y_delta = (BEZEL_W - BEZEL_TICK_PAD) * sin(50 * M_PI / 180);
  tft.fillTriangle(x_noon,
                   y_noon,
                   round(x_noon + x_delta),
                   round(y_noon - y_delta),
                   round(x_noon - x_delta),
                   round(y_noon - y_delta),
                   RGB565(BEZEL_TICK_COLOR));

  // numbers on bezel ring
  for (uint8_t tick_pos = 10; tick_pos <= 50; tick_pos += 10) {
    const float x_delta = cos(handToDegree(tick_pos, false) * M_PI / 180);
    const float y_delta = sin(handToDegree(tick_pos, false) * M_PI / 180);
    uint8_t w = 0;
    uint8_t h = 0;
    switch (tick_pos) {
      case 10:
        w = 23;
        h = 26;
        tft.drawBitmap(round(TFT_CENTER_X + (DIAL_RING_R + BEZEL_W / 2) * x_delta - w / 2),
                       round(TFT_CENTER_Y + (DIAL_RING_R + BEZEL_W / 2) * y_delta - h / 2),
                       bitmap_bezel_10,
                       w,
                       h,
                       RGB565(BEZEL_COLOR),
                       RGB565(BEZEL_TICK_COLOR));
        canvas.drawBitmap(round(DIAL_CENTER_X + (DIAL_RING_R + BEZEL_W / 2) * x_delta - w / 2),
                          round(DIAL_CENTER_Y + (DIAL_RING_R + BEZEL_W / 2) * y_delta - h / 2),
                          bitmap_bezel_10,
                          w,
                          h,
                          RGB565(BEZEL_COLOR),
                          RGB565(BEZEL_TICK_COLOR));
        break;
      case 20:
        w = 25;
        h = 31;
        tft.drawBitmap(round(TFT_CENTER_X + (DIAL_RING_R + BEZEL_W / 2) * x_delta - w / 2),
                       round(TFT_CENTER_Y + (DIAL_RING_R + BEZEL_W / 2) * y_delta - h / 2),
                       bitmap_bezel_20,
                       w,
                       h,
                       RGB565(BEZEL_COLOR),
                       RGB565(BEZEL_TICK_COLOR));
        canvas.drawBitmap(round(DIAL_CENTER_X + (DIAL_RING_R + BEZEL_W / 2) * x_delta - w / 2),
                          round(DIAL_CENTER_Y + (DIAL_RING_R + BEZEL_W / 2) * y_delta - h / 2),
                          bitmap_bezel_20,
                          w,
                          h,
                          RGB565(BEZEL_COLOR),
                          RGB565(BEZEL_TICK_COLOR));
        break;
      case 30:
        w = 29;
        h = 16;
        tft.drawBitmap(round(TFT_CENTER_X + (DIAL_RING_R + BEZEL_W / 2) * x_delta - w / 2),
                       round(TFT_CENTER_Y + (DIAL_RING_R + BEZEL_W / 2) * y_delta - h / 2),
                       bitmap_bezel_30,
                       w,
                       h,
                       RGB565(BEZEL_COLOR),
                       RGB565(BEZEL_TICK_COLOR));
        canvas.drawBitmap(round(DIAL_CENTER_X + (DIAL_RING_R + BEZEL_W / 2) * x_delta - w / 2),
                          round(DIAL_CENTER_Y + (DIAL_RING_R + BEZEL_W / 2) * y_delta - h / 2),
                          bitmap_bezel_30,
                          w,
                          h,
                          RGB565(BEZEL_COLOR),
                          RGB565(BEZEL_TICK_COLOR));
        break;
      case 40:
        w = 23;
        h = 27;
        tft.drawBitmap(round(TFT_CENTER_X + (DIAL_RING_R + BEZEL_W / 2) * x_delta - w / 2),
                       round(TFT_CENTER_Y + (DIAL_RING_R + BEZEL_W / 2) * y_delta - h / 2),
                       bitmap_bezel_40,
                       w,
                       h,
                       RGB565(BEZEL_COLOR),
                       RGB565(BEZEL_TICK_COLOR));
        canvas.drawBitmap(round(DIAL_CENTER_X + (DIAL_RING_R + BEZEL_W / 2) * x_delta - w / 2), round(DIAL_CENTER_Y + (DIAL_RING_R + BEZEL_W / 2) * y_delta - h / 2), bitmap_bezel_40, w, h, RGB565(BEZEL_COLOR), RGB565(BEZEL_TICK_COLOR));
        break;
      case 50:
        w = 26;
        h = 30;
        tft.drawBitmap(round(TFT_CENTER_X + (DIAL_RING_R + BEZEL_W / 2) * x_delta - w / 2),
                       round(TFT_CENTER_Y + (DIAL_RING_R + BEZEL_W / 2) * y_delta - h / 2),
                       bitmap_bezel_50,
                       w,
                       h,
                       RGB565(BEZEL_COLOR),
                       RGB565(BEZEL_TICK_COLOR));
        canvas.drawBitmap(round(DIAL_CENTER_X + (DIAL_RING_R + BEZEL_W / 2) * x_delta - w / 2),
                          round(DIAL_CENTER_Y + (DIAL_RING_R + BEZEL_W / 2) * y_delta - h / 2), bitmap_bezel_50,
                          w,
                          h,
                          RGB565(BEZEL_COLOR),
                          RGB565(BEZEL_TICK_COLOR));
        break;
    }
  }

  // small dots on bezel ring
  for (uint8_t tick_pos = 1; tick_pos <= 18; tick_pos++) {
    switch (tick_pos) {
      case 5:
      case 9:
      case 10:
      case 11:
      case 15:
        continue;
    }
    const float x_delta = cos(handToDegree(tick_pos, false) * M_PI / 180);
    const float y_delta = sin(handToDegree(tick_pos, false) * M_PI / 180);
    tft.fillCircle(round(TFT_CENTER_X + (DIAL_RING_R + BEZEL_W * BEZEL_DOT_POS_R_P) * x_delta),
                   round(TFT_CENTER_Y + (DIAL_RING_R + BEZEL_W * BEZEL_DOT_POS_R_P) * y_delta),
                   BEZEL_DOT_SELF_R,
                   RGB565(BEZEL_TICK_COLOR));
    canvas.fillCircle(round(DIAL_CENTER_X + (DIAL_RING_R + BEZEL_W * BEZEL_DOT_POS_R_P) * x_delta),
                      round(DIAL_CENTER_Y + (DIAL_RING_R + BEZEL_W * BEZEL_DOT_POS_R_P) * y_delta),
                      BEZEL_DOT_SELF_R,
                      RGB565(BEZEL_TICK_COLOR));
  }

  // dial ring
  canvas.fillCircle(DIAL_CENTER_X,
                    DIAL_CENTER_Y,
                    DIAL_RING_R,
                    RGB565(DIAL_RING_COLOR));

  // second ticks on dial ring
  for (uint8_t tick_pos = 0; tick_pos < 60; tick_pos++) {
    drawLine(canvas,
             DIAL_CENTER_X,
             DIAL_CENTER_Y,
             DIAL_INNER_R + INNER_TICK_PAD,
             DIAL_RING_W - INNER_TICK_PAD * 2,
             handToDegree(tick_pos, false),
             RGB565(INNER_TICK_COLOR));
  }

  // 5-minute ticks on dial ring
  for (uint8_t tick_pos = 5; tick_pos <= 55; tick_pos += 5) {
    drawAngledBox(canvas,
                  DIAL_CENTER_X,
                  DIAL_CENTER_Y,
                  DIAL_INNER_R + INNER_TICK_PAD,
                  INNER_TICK_W,
                  DIAL_RING_W - INNER_TICK_PAD * 2,
                  handToDegree(tick_pos, false),
                  RGB565(INNER_TICK_COLOR));
  }

  // the thicker tick on dial ring's 12 o'clock
  drawAngledBox(canvas,
                DIAL_CENTER_X,
                DIAL_CENTER_Y,
                DIAL_INNER_R + INNER_TICK_PAD,
                INNER_TICK_NOON_W,
                DIAL_RING_W - INNER_TICK_PAD * 2,
                handToDegree(0, false),
                RGB565(INNER_TICK_COLOR));

  // dial face
  canvas.fillCircle(DIAL_CENTER_X,
                    DIAL_CENTER_Y,
                    DIAL_INNER_R,
                    RGB565(DIAL_COLOR));
}


void drawDial() {
  float x_delta;
  float y_delta;

  // dial face
  canvas.fillCircle(DIAL_CENTER_X,
                    DIAL_CENTER_Y,
                    DIAL_INNER_R,
                    RGB565(DIAL_COLOR));

  for (uint8_t tick_pos = 0; tick_pos <= 55; tick_pos += 5) {
    switch (tick_pos) {
      case 0:  // face noon (triangle) at 12 o'clock
        canvas.fillTriangle(DIAL_CENTER_X,
                            round(DIAL_CENTER_Y - ROUND_DOT_R + ROUND_DOT_SELF_R * 2.5 + ROUND_DOT_BORDER_W * 2),
                            round(DIAL_CENTER_X - ROUND_DOT_SELF_R * 2 - ROUND_DOT_BORDER_W),
                            DIAL_CENTER_Y - ROUND_DOT_R - ROUND_DOT_SELF_R - ROUND_DOT_BORDER_W,
                            round(DIAL_CENTER_X + ROUND_DOT_SELF_R * 2 + ROUND_DOT_BORDER_W),
                            DIAL_CENTER_Y - ROUND_DOT_R - ROUND_DOT_SELF_R - ROUND_DOT_BORDER_W,
                            RGB565(ROUND_DOT_BORDER));
        canvas.drawLine(DIAL_CENTER_X,
                        round(DIAL_CENTER_Y - ROUND_DOT_R + ROUND_DOT_SELF_R * 2.5 + ROUND_DOT_BORDER_W * 2),
                        DIAL_CENTER_X,
                        round(DIAL_CENTER_Y - ROUND_DOT_R + ROUND_DOT_SELF_R * (2.5 + 2)),
                        RGB565(ROUND_DOT_BORDER));
        canvas.fillTriangle(DIAL_CENTER_X,
                            round(DIAL_CENTER_Y - ROUND_DOT_R + ROUND_DOT_SELF_R * 2.5),
                            round(DIAL_CENTER_X - ROUND_DOT_SELF_R * 2),
                            DIAL_CENTER_Y - ROUND_DOT_R - ROUND_DOT_SELF_R,
                            round(DIAL_CENTER_X + ROUND_DOT_SELF_R * 2), DIAL_CENTER_Y - ROUND_DOT_R - ROUND_DOT_SELF_R,
                            RGB565(ROUND_DOT_COLOR));
        continue;
      case 15:  // date and week at 3 o'clock
        canvas.fillRect(round(DIAL_CENTER_X + ROUND_DOT_R - DATE_W / 2 - 1),
                        round(DIAL_CENTER_Y - DATE_H / 2 - 1),
                        DATE_W + 2,
                        DATE_H + 2,
                        RGB565(ROUND_DOT_BORDER));
        canvas.fillRect(round(DIAL_CENTER_X + ROUND_DOT_R - DATE_W / 2 - WEEK_W - 2),
                        round(DIAL_CENTER_Y - WEEK_H / 2 - 1),
                        WEEK_W + 2,
                        WEEK_H + 2,
                        RGB565(ROUND_DOT_BORDER));
        canvas.fillRect(round(DIAL_CENTER_X + ROUND_DOT_R - DATE_W / 2),
                        round(DIAL_CENTER_Y - DATE_H / 2),
                        DATE_W,
                        DATE_H,
                        RGB565(ROUND_DOT_COLOR));
        canvas.fillRect(round(DIAL_CENTER_X + ROUND_DOT_R - DATE_W / 2 - WEEK_W - 1),
                        round(DIAL_CENTER_Y - WEEK_H / 2),
                        WEEK_W,
                        WEEK_H,
                        RGB565(ROUND_DOT_COLOR));
        canvas.setFont(&TomThumb);
        canvas.setTextSize(2);
        canvas.setTextColor(RGB565(DATE_COLOR));
        canvas.getTextBounds(String(day),
                             0,
                             0,
                             &text_x,
                             &text_y,
                             &text_w,
                             &text_h);
        canvas.setCursor(round(DIAL_CENTER_X + ROUND_DOT_R - text_w / 2),
                         round(DIAL_CENTER_Y + text_h / 2));
        canvas.print(String(day));
        if (weekday == weekDays[0]) canvas.setTextColor(RGB565(DATE_SUN_COLOR));
        canvas.getTextBounds(weekday,
                             0,
                             0,
                             &text_x,
                             &text_y,
                             &text_w,
                             &text_h);
        canvas.setCursor(round(DIAL_CENTER_X + ROUND_DOT_R - DATE_W / 2 - WEEK_W - 1 + WEEK_W / 2 - text_w / 2),
                         round(DIAL_CENTER_Y + text_h / 2));
        canvas.print(weekday);
        continue;
      case 30:  // longer round dot at 6 o'clock
        canvas.fillRoundRect(DIAL_CENTER_X - ROUND_DOT_SELF_R - ROUND_DOT_BORDER_W,
                             DIAL_CENTER_Y + ROUND_DOT_R - ROUND_DOT_SELF_R * 2 - ROUND_DOT_BORDER_W,
                             ROUND_DOT_SELF_R * 2 + ROUND_DOT_BORDER_W * 2, ROUND_DOT_SELF_R * 4 + ROUND_DOT_BORDER_W * 2,
                             ROUND_DOT_SELF_R + ROUND_DOT_BORDER_W,
                             RGB565(ROUND_DOT_BORDER));
        canvas.drawLine(DIAL_CENTER_X,
                        DIAL_CENTER_Y + ROUND_DOT_R - ROUND_DOT_SELF_R * 2,
                        DIAL_CENTER_X,
                        DIAL_CENTER_Y + ROUND_DOT_R - ROUND_DOT_SELF_R * 4,
                        RGB565(ROUND_DOT_BORDER));
        canvas.fillRoundRect(DIAL_CENTER_X - ROUND_DOT_SELF_R,
                             DIAL_CENTER_Y + ROUND_DOT_R - ROUND_DOT_SELF_R * 2,
                             ROUND_DOT_SELF_R * 2,
                             ROUND_DOT_SELF_R * 4,
                             ROUND_DOT_SELF_R,
                             RGB565(ROUND_DOT_COLOR));
        continue;
      case 45:  // longer round dot at 9 o'clock
        canvas.fillRoundRect(DIAL_CENTER_X - ROUND_DOT_R - ROUND_DOT_SELF_R * 2 - ROUND_DOT_BORDER_W,
                             DIAL_CENTER_Y - ROUND_DOT_SELF_R - ROUND_DOT_BORDER_W,
                             ROUND_DOT_SELF_R * 4 + ROUND_DOT_BORDER_W * 2,
                             ROUND_DOT_SELF_R * 2 + ROUND_DOT_BORDER_W * 2,
                             ROUND_DOT_SELF_R + ROUND_DOT_BORDER_W,
                             RGB565(ROUND_DOT_BORDER));
        canvas.drawLine(DIAL_CENTER_X - ROUND_DOT_R + ROUND_DOT_SELF_R * 2 + ROUND_DOT_BORDER_W,
                        DIAL_CENTER_Y,
                        DIAL_CENTER_X - ROUND_DOT_R + ROUND_DOT_SELF_R * 4,
                        DIAL_CENTER_Y,
                        RGB565(ROUND_DOT_BORDER));
        canvas.fillRoundRect(DIAL_CENTER_X - ROUND_DOT_R - ROUND_DOT_SELF_R * 2,
                             DIAL_CENTER_Y - ROUND_DOT_SELF_R,
                             ROUND_DOT_SELF_R * 4,
                             ROUND_DOT_SELF_R * 2,
                             ROUND_DOT_SELF_R,
                             RGB565(ROUND_DOT_COLOR));
        continue;
      default:  // the rest of the round dots on the face
        const float x_delta = cos(handToDegree(tick_pos, false) * M_PI / 180);
        const float y_delta = sin(handToDegree(tick_pos, false) * M_PI / 180);
        const uint16_t dot_x = round(ROUND_DOT_R * x_delta);
        const uint16_t dot_y = round(ROUND_DOT_R * y_delta);
        canvas.fillCircle(DIAL_CENTER_X + dot_x,
                          DIAL_CENTER_Y + dot_y,
                          ROUND_DOT_SELF_R + ROUND_DOT_BORDER_W,
                          RGB565(ROUND_DOT_BORDER));
        canvas.fillCircle(DIAL_CENTER_X + dot_x,
                          DIAL_CENTER_Y + dot_y,
                          ROUND_DOT_SELF_R,
                          RGB565(ROUND_DOT_COLOR));
    }
  }

  // logo
  canvas.setFont(&FreeMonoBold9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(RGB565(LOGO_COLOR));
  canvas.getTextBounds(LOGO_NAME,
                       0,
                       0,
                       &text_x,
                       &text_y,
                       &text_w,
                       &text_h);
  canvas.setCursor(round(DIAL_CENTER_X - text_w / 2),
                   round(DIAL_CENTER_Y - DIAL_INNER_R * LOGO_POS_R_P + text_h / 2));
  canvas.print(LOGO_NAME);

  // description
  canvas.setFont(&FreeSerifItalic9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(RGB565(DESCRIPTION_COLOR));
  canvas.getTextBounds(DESCRIPTION,
                       0,
                       0,
                       &text_x,
                       &text_y,
                       &text_w,
                       &text_h);
  canvas.setCursor(round(DIAL_CENTER_X - text_w / 2),
                   round(DIAL_CENTER_Y + DIAL_INNER_R * DESCRIPTION_POS_R_P + text_h / 2));
  canvas.print(DESCRIPTION);

  // hour hand
  x_delta = cos((handToDegree(hour, true) + 30 * minute / 60) * M_PI / 180);
  y_delta = sin((handToDegree(hour, true) + 30 * minute / 60) * M_PI / 180);
  float x_delta_90 = cos((handToDegree(hour, true) + 30 * minute / 60 + 90) * M_PI / 180);
  float y_delta_90 = sin((handToDegree(hour, true) + 30 * minute / 60 + 90) * M_PI / 180);
  drawAngledBox(canvas,
                DIAL_CENTER_X,
                DIAL_CENTER_Y,
                0,
                HOUR_HAND_A_H,
                HOUR_HAND_A_W,
                handToDegree(hour, true) + 30 * minute / 60,
                RGB565(HOUR_MIN_HAND_BORDER_COLOR));
  canvas.fillTriangle(round(DIAL_CENTER_X + (HOUR_HAND_A_W - HOUR_HAND_B_ALT) * x_delta),
                      round(DIAL_CENTER_Y + (HOUR_HAND_A_W - HOUR_HAND_B_ALT) * y_delta),
                      round(DIAL_CENTER_X + HOUR_HAND_A_W * x_delta - HOUR_HAND_C_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + HOUR_HAND_A_W * y_delta - HOUR_HAND_C_H / 2 * y_delta_90),
                      round(DIAL_CENTER_X + HOUR_HAND_A_W * x_delta + HOUR_HAND_C_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + HOUR_HAND_A_W * y_delta + HOUR_HAND_C_H / 2 * y_delta_90),
                      RGB565(HOUR_MIN_HAND_BORDER_COLOR));
  canvas.fillTriangle(round(DIAL_CENTER_X + (HOUR_HAND_A_W + HOUR_HAND_C_W) * x_delta - HOUR_HAND_D_BASE / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + (HOUR_HAND_A_W + HOUR_HAND_C_W) * y_delta - HOUR_HAND_D_BASE / 2 * y_delta_90),
                      round(DIAL_CENTER_X + HOUR_HAND_A_W * x_delta - HOUR_HAND_C_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + HOUR_HAND_A_W * y_delta - HOUR_HAND_C_H / 2 * y_delta_90),
                      round(DIAL_CENTER_X + HOUR_HAND_A_W * x_delta + HOUR_HAND_C_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + HOUR_HAND_A_W * y_delta + HOUR_HAND_C_H / 2 * y_delta_90),
                      RGB565(HOUR_MIN_HAND_BORDER_COLOR));
  canvas.fillTriangle(round(DIAL_CENTER_X + (HOUR_HAND_A_W + HOUR_HAND_C_W) * x_delta - HOUR_HAND_D_BASE / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + (HOUR_HAND_A_W + HOUR_HAND_C_W) * y_delta - HOUR_HAND_D_BASE / 2 * y_delta_90),
                      round(DIAL_CENTER_X + (HOUR_HAND_A_W + HOUR_HAND_C_W) * x_delta + HOUR_HAND_D_BASE / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + (HOUR_HAND_A_W + HOUR_HAND_C_W) * y_delta + HOUR_HAND_D_BASE / 2 * y_delta_90),
                      round(DIAL_CENTER_X + HOUR_HAND_A_W * x_delta + HOUR_HAND_C_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + HOUR_HAND_A_W * y_delta + HOUR_HAND_C_H / 2 * y_delta_90),
                      RGB565(HOUR_MIN_HAND_BORDER_COLOR));
  canvas.fillTriangle(round(DIAL_CENTER_X + (HOUR_HAND_A_W + HOUR_HAND_C_W + HOUR_HAND_D_ALT) * x_delta),
                      round(DIAL_CENTER_Y + (HOUR_HAND_A_W + HOUR_HAND_C_W + HOUR_HAND_D_ALT) * y_delta),
                      round(DIAL_CENTER_X + (HOUR_HAND_A_W + HOUR_HAND_C_W) * x_delta - HOUR_HAND_D_BASE / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + (HOUR_HAND_A_W + HOUR_HAND_C_W) * y_delta - HOUR_HAND_D_BASE / 2 * y_delta_90),
                      round(DIAL_CENTER_X + (HOUR_HAND_A_W + HOUR_HAND_C_W) * x_delta + HOUR_HAND_D_BASE / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + (HOUR_HAND_A_W + HOUR_HAND_C_W) * y_delta + HOUR_HAND_D_BASE / 2 * y_delta_90),
                      RGB565(HOUR_MIN_HAND_BORDER_COLOR));
  drawAngledBox(canvas,
                DIAL_CENTER_X,
                DIAL_CENTER_Y,
                HOUR_HAND_A_W + HOUR_HAND_C_W + HOUR_HAND_D_ALT,
                HOUR_MIN_HAND_POINT_W,
                HOUR_MIN_HAND_POINT_H,
                handToDegree(hour, true) + 30 * minute / 60,
                RGB565(HOUR_MIN_HAND_BORDER_COLOR));
  canvas.fillTriangle(round(DIAL_CENTER_X + (HOUR_HAND_A_W + HOUR_HAND_C_W) * x_delta - (HOUR_HAND_D_BASE / 2 - HOUR_MIN_HAND_BORDER * 1.5) * x_delta_90),
                      round(DIAL_CENTER_Y + (HOUR_HAND_A_W + HOUR_HAND_C_W) * y_delta - (HOUR_HAND_D_BASE / 2 - HOUR_MIN_HAND_BORDER * 1.5) * y_delta_90),
                      round(DIAL_CENTER_X + (HOUR_HAND_A_W + HOUR_MIN_HAND_BORDER) * x_delta - (HOUR_HAND_C_H / 2 - HOUR_MIN_HAND_BORDER) * x_delta_90),
                      round(DIAL_CENTER_Y + (HOUR_HAND_A_W + HOUR_MIN_HAND_BORDER) * y_delta - (HOUR_HAND_C_H / 2 - HOUR_MIN_HAND_BORDER) * y_delta_90),
                      round(DIAL_CENTER_X + (HOUR_HAND_A_W + HOUR_MIN_HAND_BORDER) * x_delta + (HOUR_HAND_C_H / 2 - HOUR_MIN_HAND_BORDER) * x_delta_90),
                      round(DIAL_CENTER_Y + (HOUR_HAND_A_W + HOUR_MIN_HAND_BORDER) * y_delta + (HOUR_HAND_C_H / 2 - HOUR_MIN_HAND_BORDER) * y_delta_90),
                      RGB565(HOUR_MIN_HAND_FRONT_COLOR));
  canvas.fillTriangle(round(DIAL_CENTER_X + (HOUR_HAND_A_W + HOUR_HAND_C_W) * x_delta - (HOUR_HAND_D_BASE / 2 - HOUR_MIN_HAND_BORDER * 1.5) * x_delta_90),
                      round(DIAL_CENTER_Y + (HOUR_HAND_A_W + HOUR_HAND_C_W) * y_delta - (HOUR_HAND_D_BASE / 2 - HOUR_MIN_HAND_BORDER * 1.5) * y_delta_90),
                      round(DIAL_CENTER_X + (HOUR_HAND_A_W + HOUR_HAND_C_W) * x_delta + (HOUR_HAND_D_BASE / 2 - HOUR_MIN_HAND_BORDER * 1.5) * x_delta_90),
                      round(DIAL_CENTER_Y + (HOUR_HAND_A_W + HOUR_HAND_C_W) * y_delta + (HOUR_HAND_D_BASE / 2 - HOUR_MIN_HAND_BORDER * 1.5) * y_delta_90),
                      round(DIAL_CENTER_X + (HOUR_HAND_A_W + HOUR_MIN_HAND_BORDER) * x_delta + (HOUR_HAND_C_H / 2 - HOUR_MIN_HAND_BORDER) * x_delta_90),
                      round(DIAL_CENTER_Y + (HOUR_HAND_A_W + HOUR_MIN_HAND_BORDER) * y_delta + (HOUR_HAND_C_H / 2 - HOUR_MIN_HAND_BORDER) * y_delta_90),
                      RGB565(HOUR_MIN_HAND_FRONT_COLOR));
  canvas.fillTriangle(round(DIAL_CENTER_X + (HOUR_HAND_A_W + HOUR_HAND_C_W + HOUR_HAND_D_ALT - HOUR_MIN_HAND_BORDER * 2) * x_delta),
                      round(DIAL_CENTER_Y + (HOUR_HAND_A_W + HOUR_HAND_C_W + HOUR_HAND_D_ALT - HOUR_MIN_HAND_BORDER * 2) * y_delta),
                      round(DIAL_CENTER_X + (HOUR_HAND_A_W + HOUR_HAND_C_W) * x_delta - (HOUR_HAND_D_BASE / 2 - HOUR_MIN_HAND_BORDER * 1.5) * x_delta_90),
                      round(DIAL_CENTER_Y + (HOUR_HAND_A_W + HOUR_HAND_C_W) * y_delta - (HOUR_HAND_D_BASE / 2 - HOUR_MIN_HAND_BORDER * 1.5) * y_delta_90),
                      round(DIAL_CENTER_X + (HOUR_HAND_A_W + HOUR_HAND_C_W) * x_delta + (HOUR_HAND_D_BASE / 2 - HOUR_MIN_HAND_BORDER * 1.5) * x_delta_90),
                      round(DIAL_CENTER_Y + (HOUR_HAND_A_W + HOUR_HAND_C_W) * y_delta + (HOUR_HAND_D_BASE / 2 - HOUR_MIN_HAND_BORDER * 1.5) * y_delta_90),
                      RGB565(HOUR_MIN_HAND_FRONT_COLOR));

  // minute hand
  x_delta = cos((handToDegree(minute, false) + 6 * second / 60) * M_PI / 180);
  y_delta = sin((handToDegree(minute, false) + 6 * second / 60) * M_PI / 180);
  x_delta_90 = cos((handToDegree(minute, false) + 6 * second / 60 + 90) * M_PI / 180);
  y_delta_90 = sin((handToDegree(minute, false) + 6 * second / 60 + 90) * M_PI / 180);
  canvas.fillTriangle(round(DIAL_CENTER_X - (MIN_HAND_A_ALT + MIN_HAND_B_OPPS_R) * x_delta),
                      round(DIAL_CENTER_Y - (MIN_HAND_A_ALT + MIN_HAND_B_OPPS_R) * y_delta),
                      round(DIAL_CENTER_X - MIN_HAND_B_OPPS_R * x_delta - MIN_HAND_B_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y - MIN_HAND_B_OPPS_R * y_delta - MIN_HAND_B_H / 2 * y_delta_90),
                      round(DIAL_CENTER_X - MIN_HAND_B_OPPS_R * x_delta + MIN_HAND_B_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y - MIN_HAND_B_OPPS_R * y_delta + MIN_HAND_B_H / 2 * y_delta_90),
                      RGB565(HOUR_MIN_HAND_BORDER_COLOR));
  canvas.fillTriangle(round(DIAL_CENTER_X - MIN_HAND_B_OPPS_R * x_delta - MIN_HAND_B_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y - MIN_HAND_B_OPPS_R * y_delta - MIN_HAND_B_H / 2 * y_delta_90),
                      round(DIAL_CENTER_X - MIN_HAND_B_OPPS_R * x_delta + MIN_HAND_B_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y - MIN_HAND_B_OPPS_R * y_delta + MIN_HAND_B_H / 2 * y_delta_90),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R) * x_delta - MIN_HAND_B_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R) * y_delta - MIN_HAND_B_H / 2 * y_delta_90),
                      RGB565(HOUR_MIN_HAND_BORDER_COLOR));
  canvas.fillTriangle(round(DIAL_CENTER_X - MIN_HAND_B_OPPS_R * x_delta + MIN_HAND_B_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y - MIN_HAND_B_OPPS_R * y_delta + MIN_HAND_B_H / 2 * y_delta_90),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R) * x_delta - MIN_HAND_B_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R) * y_delta - MIN_HAND_B_H / 2 * y_delta_90),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R) * x_delta + MIN_HAND_B_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R) * y_delta + MIN_HAND_B_H / 2 * y_delta_90),
                      RGB565(HOUR_MIN_HAND_BORDER_COLOR));
  canvas.fillTriangle(round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R) * x_delta - MIN_HAND_B_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R) * y_delta - MIN_HAND_B_H / 2 * y_delta_90),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R) * x_delta + MIN_HAND_B_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R) * y_delta + MIN_HAND_B_H / 2 * y_delta_90),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * x_delta - MIN_HAND_C_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * y_delta - MIN_HAND_C_H / 2 * y_delta_90),
                      RGB565(HOUR_MIN_HAND_BORDER_COLOR));
  canvas.fillTriangle(round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R) * x_delta + MIN_HAND_B_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R) * y_delta + MIN_HAND_B_H / 2 * y_delta_90),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * x_delta - MIN_HAND_C_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * y_delta - MIN_HAND_C_H / 2 * y_delta_90),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * x_delta + MIN_HAND_C_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * y_delta + MIN_HAND_C_H / 2 * y_delta_90),
                      RGB565(HOUR_MIN_HAND_BORDER_COLOR));
  drawAngledBox(canvas,
                DIAL_CENTER_X,
                DIAL_CENTER_Y,
                MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W,
                MIN_HAND_D_H,
                MIN_HAND_D_W,
                handToDegree(minute, false) + 6 * second / 60,
                RGB565(HOUR_MIN_HAND_BORDER_COLOR));
  canvas.fillTriangle(round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W + MIN_HAND_F_ALT) * x_delta),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W + MIN_HAND_F_ALT) * y_delta),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * x_delta - MIN_HAND_C_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * y_delta - MIN_HAND_C_H / 2 * y_delta_90),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * x_delta + MIN_HAND_C_H / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * y_delta + MIN_HAND_C_H / 2 * y_delta_90),
                      RGB565(HOUR_MIN_HAND_BORDER_COLOR));
  canvas.fillTriangle(round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W + MIN_HAND_D_W + MIN_HAND_E_ALT) * x_delta),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W + MIN_HAND_D_W + MIN_HAND_E_ALT) * y_delta),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W + MIN_HAND_D_W) * x_delta - MIN_HAND_E_BASE / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W + MIN_HAND_D_W) * y_delta - MIN_HAND_E_BASE / 2 * y_delta_90),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W + MIN_HAND_D_W) * x_delta + MIN_HAND_E_BASE / 2 * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W + MIN_HAND_D_W) * y_delta + MIN_HAND_E_BASE / 2 * y_delta_90),
                      RGB565(HOUR_MIN_HAND_BORDER_COLOR));
  drawAngledBox(canvas,
                DIAL_CENTER_X,
                DIAL_CENTER_Y,
                MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W + MIN_HAND_D_W + MIN_HAND_E_ALT,
                HOUR_MIN_HAND_POINT_W,
                HOUR_MIN_HAND_POINT_H,
                handToDegree(minute, false) + 6 * second / 60,
                RGB565(HOUR_MIN_HAND_BORDER_COLOR));
  canvas.fillTriangle(round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + HOUR_MIN_HAND_BORDER) * x_delta - (MIN_HAND_B_H / 2 - HOUR_MIN_HAND_BORDER) * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + HOUR_MIN_HAND_BORDER) * y_delta - (MIN_HAND_B_H / 2 - HOUR_MIN_HAND_BORDER) * y_delta_90),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + HOUR_MIN_HAND_BORDER) * x_delta + (MIN_HAND_B_H / 2 - HOUR_MIN_HAND_BORDER) * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + HOUR_MIN_HAND_BORDER) * y_delta + (MIN_HAND_B_H / 2 - HOUR_MIN_HAND_BORDER) * y_delta_90),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * x_delta - (MIN_HAND_C_H / 2 - HOUR_MIN_HAND_BORDER) * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * y_delta - (MIN_HAND_C_H / 2 - HOUR_MIN_HAND_BORDER) * y_delta_90),
                      RGB565(HOUR_MIN_HAND_FRONT_COLOR));
  canvas.fillTriangle(round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + HOUR_MIN_HAND_BORDER) * x_delta + (MIN_HAND_B_H / 2 - HOUR_MIN_HAND_BORDER) * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + HOUR_MIN_HAND_BORDER) * y_delta + (MIN_HAND_B_H / 2 - HOUR_MIN_HAND_BORDER) * y_delta_90),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * x_delta - (MIN_HAND_C_H / 2 - HOUR_MIN_HAND_BORDER) * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * y_delta - (MIN_HAND_C_H / 2 - HOUR_MIN_HAND_BORDER) * y_delta_90),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * x_delta + (MIN_HAND_C_H / 2 - HOUR_MIN_HAND_BORDER) * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * y_delta + (MIN_HAND_C_H / 2 - HOUR_MIN_HAND_BORDER) * y_delta_90),
                      RGB565(HOUR_MIN_HAND_FRONT_COLOR));
  canvas.fillTriangle(round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W + MIN_HAND_F_ALT - HOUR_MIN_HAND_BORDER * 2) * x_delta),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W + MIN_HAND_F_ALT - HOUR_MIN_HAND_BORDER * 2) * y_delta),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * x_delta - (MIN_HAND_C_H / 2 - HOUR_MIN_HAND_BORDER) * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * y_delta - (MIN_HAND_C_H / 2 - HOUR_MIN_HAND_BORDER) * y_delta_90),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * x_delta + (MIN_HAND_C_H / 2 - HOUR_MIN_HAND_BORDER) * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W) * y_delta + (MIN_HAND_C_H / 2 - HOUR_MIN_HAND_BORDER) * y_delta_90),
                      RGB565(HOUR_MIN_HAND_FRONT_COLOR));
  canvas.fillTriangle(round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W + MIN_HAND_D_W + MIN_HAND_E_ALT - HOUR_MIN_HAND_BORDER * 2) * x_delta),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W + MIN_HAND_D_W + MIN_HAND_E_ALT - HOUR_MIN_HAND_BORDER * 2) * y_delta),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W + MIN_HAND_D_W + HOUR_MIN_HAND_BORDER) * x_delta - (MIN_HAND_E_BASE / 2 - HOUR_MIN_HAND_BORDER * 2) * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W + MIN_HAND_D_W + HOUR_MIN_HAND_BORDER) * y_delta - (MIN_HAND_E_BASE / 2 - HOUR_MIN_HAND_BORDER * 2) * y_delta_90),
                      round(DIAL_CENTER_X + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W + MIN_HAND_D_W + HOUR_MIN_HAND_BORDER) * x_delta + (MIN_HAND_E_BASE / 2 - HOUR_MIN_HAND_BORDER * 2) * x_delta_90),
                      round(DIAL_CENTER_Y + (MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W + MIN_HAND_D_W + HOUR_MIN_HAND_BORDER) * y_delta + (MIN_HAND_E_BASE / 2 - HOUR_MIN_HAND_BORDER * 2) * y_delta_90),
                      RGB565(HOUR_MIN_HAND_FRONT_COLOR));
  drawAngledBox(canvas,
                DIAL_CENTER_X,
                DIAL_CENTER_Y,
                MIN_HAND_B_W - MIN_HAND_B_OPPS_R + MIN_HAND_C_W,
                MIN_HAND_D_H - HOUR_MIN_HAND_BORDER * 2,
                MIN_HAND_D_W + HOUR_MIN_HAND_BORDER * 2,
                handToDegree(minute, false) + 6 * second / 60,
                RGB565(HOUR_MIN_HAND_FRONT_COLOR));

  // second hand
  x_delta = cos((handToDegree(second, false) + second_vibrate_count * SECOND_HAND_VIBRATION / 6) * M_PI / 180);
  y_delta = sin((handToDegree(second, false) + second_vibrate_count * SECOND_HAND_VIBRATION / 6) * M_PI / 180);
  drawLine(canvas,
           DIAL_CENTER_X,
           DIAL_CENTER_Y,
           0,
           DIAL_INNER_R * SECOND_HAND_R_P,
           handToDegree(second, false) + second_vibrate_count * SECOND_HAND_VIBRATION / 6,
           RGB565(SECOND_HAND_COLOR));
  drawAngledBox(canvas,
                DIAL_CENTER_X,
                DIAL_CENTER_Y,
                0,
                2,
                DIAL_INNER_R * SECOND_HAND_BALANCE_R_P,
                (handToDegree(second, false) + second_vibrate_count * SECOND_HAND_VIBRATION / 6) + 180,
                RGB565(SECOND_HAND_COLOR));
  canvas.fillCircle(round(DIAL_CENTER_X - DIAL_INNER_R * SECOND_HAND_BALANCE_R_P * x_delta),
                    round(DIAL_CENTER_Y - DIAL_INNER_R * SECOND_HAND_BALANCE_R_P * y_delta),
                    SECOND_HAND_BALANCE_DOT_SELF_R + SECOND_HAND_BALANCE_DOT_BORDER_W,
                    RGB565(SECOND_HAND_COLOR));
  canvas.fillCircle(round(DIAL_CENTER_X - DIAL_INNER_R * SECOND_HAND_BALANCE_R_P * x_delta),
                    round(DIAL_CENTER_Y - DIAL_INNER_R * SECOND_HAND_BALANCE_R_P * y_delta),
                    SECOND_HAND_BALANCE_DOT_SELF_R,
                    RGB565(SECOND_HAND_BALANCE_COLOR));
  canvas.fillCircle(DIAL_CENTER_X,
                    DIAL_CENTER_Y,
                    SECOND_HAND_BASE_R,
                    RGB565(SECOND_HAND_COLOR));
  canvas.fillCircle(DIAL_CENTER_X,
                    DIAL_CENTER_Y,
                    HANDS_AXIS_R,
                    RGB565(SECOND_HAND_AXIS_COLOR));

  // overwrite buffer to screen
  tft.drawRGBBitmap(round((TFT_WIDTH - DIAL_WIDTH) / 2),
                    round((TFT_HEIGHT - DIAL_HEIGHT) / 2),
                    canvas.getBuffer(),
                    canvas.width(),
                    canvas.height());
}

// draw an angled rect box on screen
void drawAngledBox(Adafruit_GC9A01A &canvas, uint8_t center_x, uint8_t center_y, uint8_t r, float w, float h, float angle, uint16_t color) {
  const float x_delta = cos(angle * M_PI / 180);
  const float y_delta = sin(angle * M_PI / 180);
  const float x_side_delta = cos((angle + 90) * M_PI / 180);
  const float y_side_delta = sin((angle + 90) * M_PI / 180);
  const uint16_t x1 = round(center_x + r * x_delta - (w / 2) * x_side_delta);
  const uint16_t x2 = round(center_x + r * x_delta + (w / 2) * x_side_delta);
  const uint16_t x3 = round(center_x + (r + h) * x_delta - (w / 2) * x_side_delta);
  const uint16_t x4 = round(center_x + (r + h) * x_delta + (w / 2) * x_side_delta);
  const uint16_t y1 = round(center_y + r * y_delta - (w / 2) * y_side_delta);
  const uint16_t y2 = round(center_y + r * y_delta + (w / 2) * y_side_delta);
  const uint16_t y3 = round(center_y + (r + h) * y_delta - (w / 2) * y_side_delta);
  const uint16_t y4 = round(center_y + (r + h) * y_delta + (w / 2) * y_side_delta);
  canvas.fillTriangle(x1, y1, x2, y2, x4, y4, color);
  canvas.fillTriangle(x1, y1, x3, y3, x4, y4, color);
}

// draw an angled rect box on buffer
void drawAngledBox(GFXcanvas16 &canvas, uint8_t center_x, uint8_t center_y, uint8_t r, float w, float h, float angle, uint16_t color) {
  const float x_delta = cos(angle * M_PI / 180);
  const float y_delta = sin(angle * M_PI / 180);
  const float x_side_delta = cos((angle + 90) * M_PI / 180);
  const float y_side_delta = sin((angle + 90) * M_PI / 180);
  const uint16_t x1 = round(center_x + r * x_delta - (w / 2) * x_side_delta);
  const uint16_t x2 = round(center_x + r * x_delta + (w / 2) * x_side_delta);
  const uint16_t x3 = round(center_x + (r + h) * x_delta - (w / 2) * x_side_delta);
  const uint16_t x4 = round(center_x + (r + h) * x_delta + (w / 2) * x_side_delta);
  const uint16_t y1 = round(center_y + r * y_delta - (w / 2) * y_side_delta);
  const uint16_t y2 = round(center_y + r * y_delta + (w / 2) * y_side_delta);
  const uint16_t y3 = round(center_y + (r + h) * y_delta - (w / 2) * y_side_delta);
  const uint16_t y4 = round(center_y + (r + h) * y_delta + (w / 2) * y_side_delta);
  canvas.fillTriangle(x1, y1, x2, y2, x4, y4, color);
  canvas.fillTriangle(x1, y1, x3, y3, x4, y4, color);
}

// draw angled line on buffer
void drawLine(GFXcanvas16 &canvas, uint8_t center_x, uint8_t center_y, float r, uint8_t length, int16_t angle, uint16_t color) {
  const float x_delta = cos(angle * M_PI / 180);
  const float y_delta = sin(angle * M_PI / 180);
  canvas.drawLine(round(center_x + r * x_delta),
                  round(center_y + r * y_delta),
                  round(DIAL_CENTER_X + (r + length) * x_delta),
                  round(DIAL_CENTER_Y + (r + length) * y_delta),
                  color);
}

// convert seconds to degree
int16_t handToDegree(int16_t hand, bool hour) {
  if (hour) hand = (hand > 12 ? hand - 12 : hand) * 5;
  return hand * 6 - 90;
}

// convert RGB888 to RGB565
uint16_t RGB565(unsigned long rgb32) {
  return (rgb32 >> 8 & 0xf800) | (rgb32 >> 5 & 0x07e0) | (rgb32 >> 3 & 0x001f);
}
