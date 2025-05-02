#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Arduino.h"

Adafruit_SSD1306 display(128, 64, &Wire, -1);

void setup()
{
    Serial.begin(9600);

    setupDisplay();
}

void setupDisplay()
{
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }

    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextWrap(false);

    display.setCursor(0, 0);
    display.printf("Hi, %.1f", 20.0);
    display.display();
}
void loop()
{
}
