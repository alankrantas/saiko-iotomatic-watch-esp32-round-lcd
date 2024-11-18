# "Saiko" IoTomatic Watch with ESP32 and Round LCD

![saiko](saiko.JPG)

**"Saiko(u)" (さいこう)** is an ESP32-based NTP clock using a GC9A01A round TFT LCD (240x240 pixels).

The analog dial design is drawn mostly in real time by code and is based on/inspired by the **Seiko 5 Sports SRPK33K1** 38mm diver watch, with the second hand moves 6 times a second as in the 4R36 mechanical movement. The dates do not rotate like the real movement though.

The project is not meant for commerical use. You can read more about the more detailed story on [my Hackster.io project page](https://www.hackster.io/alankrantas/saiko-iotomatic-watch-d619e5).

![saiko-dial](saiko-dial.JPG)

## Wiring

![wiring](wiring.png)

| ESP32 | GC9A01A |
| --- | --- |
| GND | GND |
| 3.3V | VCC |
| 18 (SCK) | SCL |
| 23 (MOSI) | SDA |
| 22 | RES (RST) |
| 21 | DC |
| 5 | CS |
| 3.3V | BLK (backlight) |

## Libraries

- [Arduino NTPClient](https://github.com/arduino-libraries/NTPClient)
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit GC9A01A Library](https://github.com/adafruit/Adafruit_GC9A01A)

> Note: by default the NTP client will update time every 20 minutes and will check/force update every hour.
>
> However, the `NTPClient` may cause several-second blocking delays during the updates (which makes the watch temporarily stopped moving) due to internet or NTP server issues. There is currently no good solution to get around this but to change a better NTP server and/or a better WiFi environment depending on where you are.

## Clock Setup

### WiFi, NTP and Hour Offset

In the script

1. Change the `SECRET_SSID` and `SECRET_PASS` to your WiFi ssid/password in `secret.h`.

2. Modify `NTP_SERVER` and `NTP_HOUR_OFFSET` to a preferred setting.

### Demo Mode

If `DEMO_MODE` in the script is set to `true`, the watch will not connect to WiFi/NTP and displays a fixed time ([`10:08:42 SUN 31th`](https://museum.seiko.co.jp/en/knowledge/trivia01/)).

### Benchmark

If `BENCHMARK` in the script is set to `true`, it will calculate and print the time of each cycle for drawing the dial in serial.

## Additional Notes

- Most specs and color on the dial can actually be adjusted, for example:
  - `LOGO_NAME` is the name shown on the top half of watch, and `DESCRIPTION` is the smaller text on the lower half.
  - `SECOND_HAND_VIBRATION` defines the the second hand vibrations (`6` = 3 Hz). The code automatically calculates the drawing cycle time and the second hand moving angle.
- The script utilizes an [offscreen canvas](https://learn.adafruit.com/adafruit-gfx-graphics-library?view=all#overwriting-text-or-graphics-using-an-offscreen-canvas-3132174) (a buffer) to update the screen smoothly. However, it appears that the buffer cannot be as large as the screen itself without causing memory issues. This is why I chose a diver style since the outer bezel ring can be drawn only once, and I only need to update the inner dial afterwards.
- The script uses a few Adafruit GFX fonts. The complete list can be found [here](https://github.com/adafruit/Adafruit-GFX-Library/tree/master/Fonts).
- The parts of the hour and minute hands are labeled by alphabet codes, since I do not know how to name them:

![design](design.png)

> The script utilizes an [offscreen canvas](https://learn.adafruit.com/adafruit-gfx-graphics-library?view=all#overwriting-text-or-graphics-using-an-offscreen-canvas-3132174) (a screen buffer) to update the screen smoothly. However, it appears the buffer cannot be as large as the screen itself without causing memory issues. This is why I chose the diver dial style since the outer bezel ring can be drawn _only_ once.
