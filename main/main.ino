const uint8_t READER_R_PIN = 27;
const uint8_t READER_Y_PIN = 35;
const uint8_t READER_B_PIN = 32;
const uint8_t READER_G_PIN = 34;

const uint8_t RELAY_R_PIN = 16;
const uint8_t RELAY_Y_PIN = 17;
const uint8_t RELAY_B_PIN = 5;
const uint8_t RELAY_G_PIN = 18;
const uint8_t RELAY_PINS[] = {RELAY_R_PIN, RELAY_Y_PIN, RELAY_B_PIN, RELAY_G_PIN};

float baselineRVoltage = 2.5;
float baselineYVoltage = 2.5;
float baselineBVoltage = 2.5;
float baselineGVoltage = 3.3;

void setup()
{
    Serial.begin(115200);

    pinMode(READER_R_PIN, INPUT_PULLUP);
    pinMode(READER_Y_PIN, INPUT_PULLUP);
    pinMode(READER_B_PIN, INPUT_PULLUP);
    pinMode(READER_G_PIN, INPUT_PULLUP);

    for (int i = 0; i < 4; i++)
    {
        pinMode(RELAY_PINS[i], OUTPUT);
    }

    baselineBVoltage = readVoltage(READER_B_PIN, RELAY_B_PIN);
    baselineRVoltage = readVoltage(READER_R_PIN, RELAY_R_PIN);
    baselineYVoltage = readVoltage(READER_Y_PIN, RELAY_Y_PIN);
    baselineGVoltage = readVoltage(READER_G_PIN, RELAY_G_PIN);

    Serial.printf("Baseline: %.2f, %.2f, %.2f, %.2f\n", baselineRVoltage, baselineYVoltage, baselineBVoltage, baselineGVoltage);
}

void loop()
{
    // detect faults
    float rVoltage = readVoltage(READER_R_PIN, RELAY_R_PIN);
    float yVoltage = readVoltage(READER_Y_PIN, RELAY_Y_PIN);
    float bVoltage = readVoltage(READER_B_PIN, RELAY_B_PIN);
    float gVoltage = readVoltage(READER_G_PIN, RELAY_G_PIN);

    Serial.printf("%.2f, %.2f, %.2f, %.2f\n", rVoltage, yVoltage, bVoltage, gVoltage);

    bool rFault = !isAboutEqual(rVoltage, baselineRVoltage);
    bool yFault = !isAboutEqual(yVoltage, baselineYVoltage);
    bool bFault = !isAboutEqual(bVoltage, baselineBVoltage);
    bool gFault = !isAboutEqual(gVoltage, baselineGVoltage);

    if (rFault && yFault && bFault)
    {
        if (gFault)
        {
            Serial.println("R-Y-B-G fault detected");
        }
        else
        {
            Serial.println("R-Y-B fault detected");
        }
    }
    else if ((rFault && yFault) || (rFault && bFault) || (yFault && bFault))
    {
        if (gFault)
        {
            if (rFault && yFault)
            {
                Serial.println("R-Y to G fault detected");
            }
            else if (rFault && bFault)
            {
                Serial.println("R-B to G fault detected");
            }
            else if (yFault && bFault)
            {
                Serial.println("Y-B to G fault detected");
            }
        }
        else
        {
            if (rFault && yFault)
            {
                Serial.println("R-Y fault detected");
            }
            else if (rFault && bFault)
            {
                Serial.println("R-B fault detected");
            }
            else if (yFault && bFault)
            {
                Serial.println("Y-B fault detected");
            }
        }
    }
    else if ((rFault && gFault) || (yFault && gFault) || (bFault && gFault))
    {
        if (rFault && gFault)
        {
            Serial.println("R to G fault detected");
        }
        else if (yFault && gFault)
        {
            Serial.println("Y to G fault detected");
        }
        else if (bFault && gFault)
        {
            Serial.println("B to G fault detected");
        }
    }
    else if (rFault || yFault || bFault || gFault)
    {
        if (rFault)
        {
            Serial.println("R fault detected");
        }
        if (yFault)
        {
            Serial.println("Y fault detected");
        }
        if (bFault)
        {
            Serial.println("B fault detected");
        }
        if (gFault)
        {
            Serial.println("G fault detected");
        }
    }
    else
    {
        Serial.println("No faults detected");
    }

    if (rFault || yFault || bFault || gFault)
    {
        float faultDistance;

        if (rFault)
        {
            faultDistance = computeFaultDistance(rVoltage, baselineRVoltage);
        }
        else if (yFault)
        {
            faultDistance = computeFaultDistance(yVoltage, baselineYVoltage);
        }
        else if (bFault)
        {
            faultDistance = computeFaultDistance(bVoltage, baselineBVoltage);
        }
        else if (gFault)
        {
            faultDistance = computeFaultDistance(gVoltage, baselineGVoltage);
        }

        Serial.printf("Fault distance: %.2f\n", faultDistance);
    }

    delay(1000);
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
