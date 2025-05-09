#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <AsyncWebSocket.h>
#include <DallasTemperature.h>
#include <ESPAsyncWebServer.h>
#include <OneWire.h>

const uint8_t READER_R_PIN = 34;
const uint8_t READER_Y_PIN = 35;
const uint8_t READER_B_PIN = 32;
const uint8_t READER_G_PIN = 33;

const uint8_t RELAY_R_PIN = 16;
const uint8_t RELAY_Y_PIN = 17;
const uint8_t RELAY_B_PIN = 5;
const uint8_t RELAY_G_PIN = 18;
const uint8_t RELAY_PINS[] = {RELAY_R_PIN, RELAY_Y_PIN, RELAY_B_PIN, RELAY_G_PIN};

const int ONE_WIRE_BUS_PIN = 4;
const int TEMP_SENSOR_PRECISION = 12;

const int BUZZER_PIN = 27;
const int BUZZER_DURATION_MS = 500;

const unsigned long UPDATE_INTERVAL_MS = 500;

const char *WIFI_SSID = "GlobeAtHome_2G";
const char *WIFI_PASSWORD = "pepot1232g";

const float VOLTAGE_THRESHOLD = 0.05;
const float TEMP_THRESHOLD = 40.0;

Adafruit_SSD1306 display(128, 64, &Wire);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

OneWire oneWire(ONE_WIRE_BUS_PIN);
DallasTemperature tempSensors(&oneWire);
DeviceAddress tempSensor1, tempSensor2, tempSensor3, tempSensor4;

String fault = "";
float faultDistance = 0.0;
float rVoltage = 0.0;
float yVoltage = 0.0;
float bVoltage = 0.0;
float gVoltage = 0.0;
float temp1 = 0.0;
float temp2 = 0.0;
float temp3 = 0.0;
float temp4 = 0.0;
String overheatFault = "";

float baselineRVoltage = 2.5;
float baselineYVoltage = 2.5;
float baselineBVoltage = 2.5;
float baselineGVoltage = 3.3;

bool displayNeedsUpdate = true;

void setupTempSensors()
{
    tempSensors.begin();

    if (!tempSensors.getAddress(tempSensor1, 0))
    {
        Serial.println("Failed to find tempSensor1");
    }
    if (!tempSensors.getAddress(tempSensor2, 1))
    {
        Serial.println("Failed to find tempSensor2");
    }
    if (!tempSensors.getAddress(tempSensor3, 2))
    {
        Serial.println("Failed to find tempSensor3");
    }
    if (!tempSensors.getAddress(tempSensor4, 3))
    {
        Serial.println("Failed to find tempSensor4");
    }

    tempSensors.setResolution(tempSensor1, TEMP_SENSOR_PRECISION);
    tempSensors.setResolution(tempSensor2, TEMP_SENSOR_PRECISION);
    tempSensors.setResolution(tempSensor3, TEMP_SENSOR_PRECISION);
    tempSensors.setResolution(tempSensor4, TEMP_SENSOR_PRECISION);
}

void setupDisplay()
{
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println(F("SSD1306 allocation failed"));
    }

    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextWrap(false);

    Serial.println(F("Display initialized"));
}

void onServerGetFault(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", fault);
}

void onServerGetFaultDistance(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", String(faultDistance));
}

void onServerGetRVoltage(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", String(rVoltage));
}

void onServerGetYVoltage(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", String(yVoltage));
}

void onServerGetBVoltage(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", String(bVoltage));
}

void onServerGetGVoltage(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", String(gVoltage));
}

void onServerGetTemp1(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", String(temp1));
}

void onServerGetTemp2(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", String(temp2));
}

void onServerGetTemp3(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", String(temp3));
}

void onServerGetTemp4(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", String(temp4));
}

void onServerGetOverheatFault(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", String(overheatFault));
}

void onWsEvent(AsyncWebSocket *ws, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}

void setupServer()
{
    WiFi.mode(WIFI_AP);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to WiFi ..");

    unsigned long prevMs = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        unsigned long currMs = millis();
        if (currMs - prevMs >= 1000)
        {
            Serial.print('.');
            prevMs = currMs;
        }
    }

    Serial.println(WiFi.localIP());

    server.on("/getFault", HTTP_GET, onServerGetFault);
    server.on("/getFaultDistance", HTTP_GET, onServerGetFaultDistance);
    server.on("/getRVoltage", HTTP_GET, onServerGetRVoltage);
    server.on("/getYVoltage", HTTP_GET, onServerGetYVoltage);
    server.on("/getBVoltage", HTTP_GET, onServerGetBVoltage);
    server.on("/getGVoltage", HTTP_GET, onServerGetGVoltage);
    server.on("/getTemp1", HTTP_GET, onServerGetTemp1);
    server.on("/getTemp2", HTTP_GET, onServerGetTemp2);
    server.on("/getTemp3", HTTP_GET, onServerGetTemp3);
    server.on("/getTemp4", HTTP_GET, onServerGetTemp4);
    server.on("/getOverheatFault", HTTP_GET, onServerGetOverheatFault);

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    server.begin();
}

void setup()
{
    Serial.begin(115200);

    pinMode(READER_R_PIN, INPUT_PULLUP);
    pinMode(READER_Y_PIN, INPUT_PULLUP);
    pinMode(READER_B_PIN, INPUT_PULLUP);
    pinMode(READER_G_PIN, INPUT_PULLUP);

    pinMode(BUZZER_PIN, OUTPUT);

    for (int i = 0; i < 4; i++)
    {
        pinMode(RELAY_PINS[i], OUTPUT);
    }

    setupTempSensors();

    setupDisplay();

    setupServer();

    displayDrawIpAddress();
    delay(500);

    baselineBVoltage = readVoltage(READER_B_PIN, RELAY_B_PIN);
    baselineRVoltage = readVoltage(READER_R_PIN, RELAY_R_PIN);
    baselineYVoltage = readVoltage(READER_Y_PIN, RELAY_Y_PIN);
    baselineGVoltage = readVoltage(READER_G_PIN, RELAY_G_PIN);

    Serial.printf("Baseline: %.2f, %.2f, %.2f, %.2f\n", baselineRVoltage, baselineYVoltage, baselineBVoltage, baselineGVoltage);
}

unsigned long prevTimestamp = millis();
unsigned long buzzerStartTimestamp = 0;

void loop()
{
    ws.cleanupClients();

    if (millis() - prevTimestamp > UPDATE_INTERVAL_MS)
    {
        prevTimestamp = millis();

        tempSensors.requestTemperatures();
        float t1 = tempSensors.getTempC(tempSensor1);
        float t2 = tempSensors.getTempC(tempSensor2);
        float t3 = tempSensors.getTempC(tempSensor3);
        float t4 = tempSensors.getTempC(tempSensor4);
        updateTemps(t1, t2, t3, t4);

        String overheatF = "";
        if (t1 > TEMP_THRESHOLD)
        {
            overheatF += "T1 ";
        }
        if (t2 > TEMP_THRESHOLD)
        {
            overheatF += "T2 ";
        }
        if (t3 > TEMP_THRESHOLD)
        {
            overheatF += "T3 ";
        }
        if (t4 > TEMP_THRESHOLD)
        {
            overheatF += "T4 ";
        }
        updateOverheatFault(overheatF);

        Serial.printf("Temp: %.2f, %.2f, %.2f, %.2f\n", t1, t2, t3, t4);

        if (overheatF.isEmpty())
        {
            float rV = readVoltage(READER_R_PIN, RELAY_R_PIN);
            float yV = readVoltage(READER_Y_PIN, RELAY_Y_PIN);
            float bV = readVoltage(READER_B_PIN, RELAY_B_PIN);
            float gV = readVoltage(READER_G_PIN, RELAY_G_PIN);
            updateVoltages(rV, yV, bV, gV);

            Serial.printf("Volts: %.2f, %.2f, %.2f, %.2f\n", rVoltage, yVoltage, bVoltage, gVoltage);

            bool rOpen = isAboutEqual(rVoltage, 0.0, 0.5);
            bool yOpen = isAboutEqual(yVoltage, 0.0, 0.5);
            bool bOpen = isAboutEqual(bVoltage, 0.0, 0.5);

            bool rGShorted = isAboutEqual(rVoltage, gVoltage, VOLTAGE_THRESHOLD);
            bool yGShorted = isAboutEqual(yVoltage, gVoltage, VOLTAGE_THRESHOLD);
            bool bGShorted = isAboutEqual(bVoltage, gVoltage, VOLTAGE_THRESHOLD);

            bool rYShorted = isAboutEqual(rVoltage, yVoltage, 0.2) && rVoltage + VOLTAGE_THRESHOLD < baselineRVoltage && yVoltage + VOLTAGE_THRESHOLD < baselineYVoltage;
            bool yBShorted = isAboutEqual(yVoltage, bVoltage, 0.2) && yVoltage + VOLTAGE_THRESHOLD < baselineYVoltage && bVoltage + VOLTAGE_THRESHOLD < baselineBVoltage;
            bool bRShorted = isAboutEqual(rVoltage, bVoltage, 0.2) && rVoltage + VOLTAGE_THRESHOLD < baselineRVoltage && bVoltage + VOLTAGE_THRESHOLD < baselineBVoltage;

            float dist = 0.0;
            String f = "";
            if (rOpen || yOpen || bOpen)
            {
                f = "Open: ";
                if (rOpen)
                {
                    f += "R ";
                }
                if (yOpen)
                {
                    f += "Y ";
                }
                if (bOpen)
                {
                    f += "B ";
                }
            }
            else if (rGShorted && yGShorted && bGShorted)
            {
                f = "Sym: R-Y-B to G";
                dist = computeFaultDistance(rVoltage, baselineRVoltage);
            }
            else if (rGShorted && yGShorted)
            {
                f = "Sym: R-Y to G";
                dist = computeFaultDistance(rVoltage, baselineRVoltage);
            }
            else if (yGShorted && bGShorted)
            {
                f = "Sym: Y-B to G";
                dist = computeFaultDistance(yVoltage, baselineYVoltage);
            }
            else if (rGShorted && bGShorted)
            {
                f = "Sym: R-B to G";
                dist = computeFaultDistance(bVoltage, baselineBVoltage);
            }
            else if (rGShorted)
            {
                f = "Sym: R to G";
                dist = computeFaultDistance(rVoltage, baselineRVoltage);
            }
            else if (yGShorted)
            {
                f = "Sym: Y to G";
                dist = computeFaultDistance(yVoltage, baselineYVoltage);
            }
            else if (bGShorted)
            {
                f = "Sym: B to G";
                dist = computeFaultDistance(bVoltage, baselineBVoltage);
            }
            else if ((rYShorted && bRShorted) || (yBShorted && bRShorted) || (rYShorted && yBShorted))
            {
                f = "Sym: R-Y-B";
                dist = computeFaultDistance(rVoltage, baselineRVoltage);
            }
            else if (rYShorted)
            {
                f = "Sym: R-Y";
                dist = computeFaultDistance(rVoltage, baselineRVoltage);
            }
            else if (bRShorted)
            {
                f = "Sym: R-B";
                dist = computeFaultDistance(bVoltage, baselineBVoltage);
            }
            else if (yBShorted)
            {
                f = "Sym: Y-B";
                dist = computeFaultDistance(yVoltage, baselineYVoltage);
            }
            updateFault(f);
            updateFaultDistance(dist);

            Serial.printf("Fault: %s, Distance: %.2f\n", fault.c_str(), faultDistance);
        }
        else
        {
            for (int i = 0; i < 4; i++)
            {
                digitalWrite(RELAY_PINS[i], HIGH);
            }
        }
    }

    if (!fault.isEmpty() || !overheatFault.isEmpty())
    {
        buzzerStartTimestamp = millis();
        tone(BUZZER_PIN, 1000);
    }

    if (buzzerStartTimestamp > 0 && millis() - buzzerStartTimestamp >= BUZZER_DURATION_MS)
    {
        noTone(BUZZER_PIN);
        buzzerStartTimestamp = 0;
    }

    if (displayNeedsUpdate)
    {
        displayDrawMainScreen();
        displayNeedsUpdate = false;
    }
}

float readVoltage(uint8_t readerPin, uint8_t relayPin)
{
    for (int i = 0; i < 4; i++)
    {
        if (RELAY_PINS[i] == relayPin)
        {
            digitalWrite(RELAY_PINS[i], LOW);
        }
        else
        {
            digitalWrite(RELAY_PINS[i], HIGH);
        }
    }

    delay(50);

    uint16_t val = analogRead(readerPin);
    float voltage = valToVoltage(val);

    delay(50);

    return voltage;
}

void displayDrawMainScreen()
{
    display.clearDisplay();
    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println("Fault:");

    display.setCursor(0, 10);
    if (!fault.isEmpty())
    {
        display.printf("  %s\n", fault.c_str());
    }
    else
    {
        display.println("  No Fault");
    }

    display.setCursor(0, 20);
    display.println("Fault Est. Distance:");

    display.setCursor(0, 30);
    if (faultDistance > 0.0)
    {
        display.printf("  %.1f km\n", faultDistance);
    }
    else
    {
        display.println("  N/A");
    }

    display.setCursor(0, 40);
    display.println("Overheat fault:");
    if (!overheatFault.isEmpty())
    {
        display.printf("  %s\n", overheatFault.c_str());
    }
    else
    {
        display.println("  N/A");
    }

    display.display();
}

void displayDrawIpAddress()
{
    display.clearDisplay();
    display.setTextSize(1);

    display.setCursor(0, 0);
    display.println("IP Address:");

    display.setCursor(0, 10);
    display.printf("  %s\n", WiFi.localIP().toString().c_str());

    display.display();
}

void updateFault(String f)
{
    if (f.compareTo(fault) != 0)
    {
        fault = f;
        wsSend("fault", fault);
        displayNeedsUpdate = true;
    }
}

void updateFaultDistance(float fd)
{
    if (abs(fd - faultDistance) > __FLT_EPSILON__)
    {
        faultDistance = fd;
        wsSend("faultDistance", String(faultDistance));
        displayNeedsUpdate = true;
    }
}

void updateVoltages(float r, float y, float b, float g)
{
    if (abs(r - rVoltage) > __FLT_EPSILON__)
    {
        rVoltage = r;
        wsSend("rVoltage", String(rVoltage));
    }

    if (abs(y - yVoltage) > __FLT_EPSILON__)
    {
        yVoltage = y;
        wsSend("yVoltage", String(yVoltage));
    }

    if (abs(b - bVoltage) > __FLT_EPSILON__)
    {
        bVoltage = b;
        wsSend("bVoltage", String(bVoltage));
    }

    if (abs(g - gVoltage) > __FLT_EPSILON__)
    {
        gVoltage = g;
        wsSend("gVoltage", String(gVoltage));
    }
}

void updateTemps(float t1, float t2, float t3, float t4)
{
    if (abs(t1 - temp1) > __FLT_EPSILON__)
    {
        temp1 = t1;
        wsSend("temp1", String(temp1));
    }

    if (abs(t2 - temp2) > __FLT_EPSILON__)
    {
        temp2 = t2;
        wsSend("temp2", String(temp2));
    }

    if (abs(t3 - temp3) > __FLT_EPSILON__)
    {
        temp3 = t3;
        wsSend("temp3", String(temp3));
    }

    if (abs(t4 - temp4) > __FLT_EPSILON__)
    {
        temp4 = t4;
        wsSend("temp4", String(temp4));
    }
}

void updateOverheatFault(String overheatF)
{
    if (overheatF.compareTo(overheatFault) != 0)
    {
        overheatFault = overheatF;
        wsSend("overheatFault", String(overheatFault));
        displayNeedsUpdate = true;
    }
}

void wsSend(const char *dataType, String data)
{
    ws.printfAll("%s: %s", dataType, data.c_str());
}

float valToVoltage(uint16_t val)
{
    return ((float)val / 4095.0) * 3.3;
}

bool isAboutEqual(float a, float b, float tolerance)
{
    return abs(a - b) < tolerance;
}

float computeFaultDistance(float voltage, float baseline)
{
    float delta = abs(baseline - voltage);

    return mapFloat(delta, 0.0, 1.0, 0.0, 8.0);
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
