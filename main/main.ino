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

const int BUZZER = 6;
const int BUZZER_DURATION_MS = 3000;

const int oneWireBus = 4;
const int oneWireBus2 = 0;
const int oneWireBus3 = 2;
const int oneWireBus4 = 15;

const unsigned long UPDATE_INTERVAL_MS = 500;

const char *WIFI_SSID = "GlobeAtHome_2G";
const char *WIFI_PASSWORD = "pepot1232g";

const float TEMP_THRESHOLD = 45.0;

Adafruit_SSD1306 display(128, 64, &Wire);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

OneWire oneWire(oneWireBus);
OneWire oneWire2(oneWireBus2);
OneWire oneWire3(oneWireBus3);
OneWire oneWire4(oneWireBus4);

DallasTemperature tempSensor1(&oneWire);
DallasTemperature tempSensor2(&oneWire2);
DallasTemperature tempSensor3(&oneWire3);
DallasTemperature tempSensor4(&oneWire4);

String fault = "";
float faultDistance = 0.0;
float rVoltage = 0.0;
float yVoltage = 0.0;
float bVoltage = 0.0;
float gVoltage = 0.0;
float temp1;
float temp2;
float temp3;
float temp4;
float overheatDistance = 0.0;

float baselineRVoltage = 2.5;
float baselineYVoltage = 2.5;
float baselineBVoltage = 2.5;
float baselineGVoltage = 3.3;

bool displayNeedsUpdate = true;

void setupTempSensors()
{
    tempSensor1.begin();
    tempSensor2.begin();
    tempSensor3.begin();
    tempSensor4.begin();
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

void onServerGetOverheatDistance(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain", String(overheatDistance));
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
    server.on("/getOverheatDistance", HTTP_GET, onServerGetOverheatDistance);

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

    pinMode(BUZZER, OUTPUT);
    digitalWrite(BUZZER, LOW);

    for (int i = 0; i < 4; i++)
    {
        pinMode(RELAY_PINS[i], OUTPUT);
    }

    baselineBVoltage = readVoltage(READER_B_PIN, RELAY_B_PIN);
    baselineRVoltage = readVoltage(READER_R_PIN, RELAY_R_PIN);
    baselineYVoltage = readVoltage(READER_Y_PIN, RELAY_Y_PIN);
    baselineGVoltage = readVoltage(READER_G_PIN, RELAY_G_PIN);

    setupDisplay();

    setupServer();

    setupTempSensors();

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

        float rV = readVoltage(READER_R_PIN, RELAY_R_PIN);
        float yV = readVoltage(READER_Y_PIN, RELAY_Y_PIN);
        float bV = readVoltage(READER_B_PIN, RELAY_B_PIN);
        float gV = readVoltage(READER_G_PIN, RELAY_G_PIN);
        updateVoltages(rV, yV, bV, gV);

        Serial.printf("%.2f, %.2f, %.2f, %.2f\n", rVoltage, yVoltage, bVoltage, gVoltage);

        bool rFault = !isAboutEqual(rVoltage, baselineRVoltage);
        bool yFault = !isAboutEqual(yVoltage, baselineYVoltage);
        bool bFault = !isAboutEqual(bVoltage, baselineBVoltage);
        bool gFault = !isAboutEqual(gVoltage, baselineGVoltage);

        String f;
        if (rFault && yFault && bFault)
        {
            if (gFault)
            {
                f = "Sym: R-Y-B to G";
            }
            else
            {
                f = "Sym: R-Y-B";
            }
        }
        else if ((rFault && yFault) || (rFault && bFault) || (yFault && bFault))
        {
            if (gFault)
            {
                if (rFault && yFault)
                {
                    f = "Unsym: R-Y to G";
                }
                else if (rFault && bFault)
                {
                    f = "Unsym: R-B to G";
                }
                else if (yFault && bFault)
                {
                    f = "Unsym: Y-B to G";
                }
            }
            else
            {
                if (rFault && yFault)
                {
                    f = "Unsym: R-Y";
                }
                else if (rFault && bFault)
                {
                    f = "Unsym: R-B";
                }
                else if (yFault && bFault)
                {
                    f = "Unsym: Y-B";
                }
            }
        }
        else if ((rFault && gFault) || (yFault && gFault) || (bFault && gFault))
        {
            if (rFault && gFault)
            {
                f = "Unsym: R to G";
            }
            else if (yFault && gFault)
            {
                f = "Unsym: Y to G";
            }
            else if (bFault && gFault)
            {
                f = "Unsym: B to G";
            }
        }
        else if (rFault || yFault || bFault || gFault)
        {
            f = "Open: ";
            if (rFault)
            {
                f += "R ";
            }
            if (yFault)
            {
                f += "Y ";
            }
            if (bFault)
            {
                f += "B ";
            }
            if (gFault)
            {
                f += "G ";
            }
        }
        else
        {
            f = "";
        }
        updateFault(f);

        float dist;
        if (rFault || yFault || bFault || gFault)
        {
            if (rFault)
            {
                dist = computeFaultDistance(rVoltage, baselineRVoltage);
            }
            else if (yFault)
            {
                dist = computeFaultDistance(yVoltage, baselineYVoltage);
            }
            else if (bFault)
            {
                dist = computeFaultDistance(bVoltage, baselineBVoltage);
            }
            else if (gFault)
            {
                dist = computeFaultDistance(gVoltage, baselineGVoltage);
            }
        }
        else
        {
            dist = 0.0;
        }
        updateFaultDistance(dist);

        tempSensor1.requestTemperatures();
        tempSensor2.requestTemperatures();
        tempSensor3.requestTemperatures();
        tempSensor4.requestTemperatures();

        float t1 = tempSensor1.getTempCByIndex(0);
        float t2 = tempSensor2.getTempCByIndex(0);
        float t3 = tempSensor3.getTempCByIndex(0);
        float t4 = tempSensor4.getTempCByIndex(0);

        updateTemps(t1, t2, t3, t4);

        Serial.print("Temp: ");
        Serial.print(t1);
        Serial.println(" ºC");

        Serial.print("Temp2: ");
        Serial.print(t2);
        Serial.println(" ºC");

        Serial.print("Temp 3: ");
        Serial.print(t3);
        Serial.println(" ºC");

        Serial.print("Temp 4: ");
        Serial.print(t4);
        Serial.println(" ºC");

        float overheatDist;
        if (t1 > TEMP_THRESHOLD)

        {
            overheatDist = 2.0;
        }
        else if (t2 > TEMP_THRESHOLD)
        {
            overheatDist = 4.0;
        }
        else if (t3 > TEMP_THRESHOLD)
        {
            overheatDist = 6.0;
        }
        else if (t4 > TEMP_THRESHOLD)
        {
            overheatDist = 8.0;
        }
        else
        {
            overheatDist = 0.0;
        }
        updateOverheatDistance(overheatDist);
    }

    if (faultDistance > 0.0 || overheatDistance > 0.0)
    {
        buzzerStartTimestamp = millis();
        digitalWrite(BUZZER, HIGH);
    }

    if (buzzerStartTimestamp > 0 && millis() - buzzerStartTimestamp >= BUZZER_DURATION_MS)
    {
        digitalWrite(BUZZER, LOW);
        buzzerStartTimestamp = 0;
    }

    if (displayNeedsUpdate)
    {
        drawDisplay();
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

void drawDisplay()
{
    display.clearDisplay();

    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println("Fault");

    display.setCursor(0, 10);
    display.setTextSize(2);
    if (fault != "")
    {
        display.printf(" %s\n", fault.c_str());
    }
    else
    {
        display.println("No Fault");
    }

    display.setTextSize(1);
    display.setCursor(0, 40);
    display.println("Distance");
    display.setCursor(0, 50);
    display.setTextSize(2);

    if (faultDistance > 0.0)
    {
        display.printf(" %.1f m\n", faultDistance);
    }
    else
    {
        display.println("N/A");
    }

    display.display();
}

void updateFault(String f)
{
    if (f != fault)
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

void updateOverheatDistance(float od)
{
    if (abs(od - overheatDistance) > __FLT_EPSILON__)
    {
        overheatDistance = od;
        wsSend("overheatDistance", String(overheatDistance));
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

bool isAboutEqual(float a, float b)
{
    return abs(a - b) < 0.2;
}

// Return 2.0, 4.0, 6.0 or 8.0 only!
float computeFaultDistance(float voltage, float baseline)
{
    if (voltage < baseline - 0.5)
    {
        return 8.0;
    }
    else if (voltage < baseline - 0.4)
    {
        return 6.0;
    }
    else if (voltage < baseline - 0.3)
    {
        return 4.0;
    }
    else if (voltage < baseline - 0.2)
    {
        return 2.0;
    }
    else
    {
        return 0.0;
    }
}
