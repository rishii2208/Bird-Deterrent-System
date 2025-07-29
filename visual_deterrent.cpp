
#include "visual_deterrent.h"

VisualDeterrent::VisualDeterrent()
{
    currentMode = MODE_DISABLED;
    currentPattern = PATTERN_OFF;
    patternStartTime = 0;
    patternCycle = 0;
    systemEnabled = true;
    ambientLight = 0.0;
    thermalProtection = false;
    lastThermalCheck = 0;

    for (int i = 0; i < 2; i++)
    {
        ledChannels[i].pin = -1;
        ledChannels[i].currentBrightness = 0;
        ledChannels[i].targetBrightness = 0;
        ledChannels[i].lastUpdate = 0;
        ledChannels[i].isActive = false;
        ledChannels[i].onTime = 0;
        ledChannels[i].temperature = 25.0;
    }
}

bool VisualDeterrent::begin(int led1Pin, int led2Pin)
{
    Serial.println("Initializing Visual Deterrent System...");

    ledChannels[0].pin = led1Pin;
    ledChannels[1].pin = led2Pin;

    pinMode(led1Pin, OUTPUT);
    pinMode(led2Pin, OUTPUT);

    analogWrite(led1Pin, 0);
    analogWrite(led2Pin, 0);

    initializePatterns();

    performLEDTest();

    Serial.println("Visual Deterrent System initialized successfully");
    return true;
}

void VisualDeterrent::initializePatterns()
{
    // Pattern 0: Off
    patterns[0] = {0, 1000, 0, 1, "Off"};

    // Pattern 1: Slow Blink (Alert mode)
    patterns[1] = {500, 1500, 128, 999, "Slow Blink"};

    // Pattern 2: Fast Blink (Active deterrent)
    patterns[2] = {200, 200, 255, 999, "Fast Blink"};

    patterns[3] = {100, 100, 255, 2, "Double Flash"};

    patterns[4] = {0, 0, 255, 999, "Random"};

    Serial.println("Strobe patterns initialized");
}

void VisualDeterrent::update()
{
    if (!systemEnabled)
        return;

    unsigned long currentTime = millis();

    if (currentTime - lastThermalCheck > 1000)
    {
        checkThermalProtection();
        lastThermalCheck = currentTime;
    }

    for (int i = 0; i < 2; i++)
    {
        updateLEDBrightness(i);
    }

    if (currentMode != MODE_DISABLED && !thermalProtection)
    {
        executeStrobePattern();
    }
}

void VisualDeterrent::updateLEDBrightness(int channel)
{
    if (channel < 0 || channel >= 2)
        return;

    LEDChannel *led = &ledChannels[channel];
    unsigned long currentTime = millis();

    if (led->currentBrightness != led->targetBrightness)
    {
        if (currentTime - led->lastUpdate > 10)
        {
            int step = (led->targetBrightness > led->currentBrightness) ? 5 : -5;

            if (abs(led->targetBrightness - led->currentBrightness) < abs(step))
            {
                led->currentBrightness = led->targetBrightness;
            }
            else
            {
                led->currentBrightness += step;
            }

            int adaptiveBrightness = calculateAdaptiveBrightness(led->currentBrightness);
            analogWrite(led->pin, adaptiveBrightness);

            led->lastUpdate = currentTime;
        }
    }

    if (led->currentBrightness > 50)
    {
        if (!led->isActive)
        {
            led->isActive = true;
            led->onTime = currentTime;
        }

        unsigned long onDuration = currentTime - led->onTime;
        float heatFactor = (led->currentBrightness / 255.0) * (onDuration / 1000.0);
        led->temperature = 25.0 + (heatFactor * 0.5);
    }
    else
    {
        led->isActive = false;

        led->temperature = max(25.0, led->temperature - 0.1);
    }
}

void VisualDeterrent::executeStrobePattern()
{
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - patternStartTime;

    StrobeConfig *pattern = &patterns[currentPattern];

    switch (currentPattern)
    {
    case PATTERN_OFF:
        setLEDBrightness(0, 0);
        setLEDBrightness(1, 0);
        break;

    case PATTERN_SLOW_BLINK:
    case PATTERN_FAST_BLINK:
    {
        int cycleDuration = pattern->onDuration + pattern->offDuration;
        int cyclePosition = elapsed % cycleDuration;

        if (cyclePosition < pattern->onDuration)
        {

            setLEDBrightness(0, pattern->brightness);
            setLEDBrightness(1, pattern->brightness);
        }
        else
        {

            setLEDBrightness(0, 0);
            setLEDBrightness(1, 0);
        }
    }
    break;

    case PATTERN_DOUBLE_FLASH:
    {
        int totalCycleDuration = (pattern->onDuration + pattern->offDuration) * pattern->repetitions + 1000;
        int cyclePosition = elapsed % totalCycleDuration;

        if (cyclePosition < (pattern->onDuration + pattern->offDuration) * pattern->repetitions)
        {
            int flashCycle = cyclePosition % (pattern->onDuration + pattern->offDuration);
            if (flashCycle < pattern->onDuration)
            {
                setLEDBrightness(0, pattern->brightness);
                setLEDBrightness(1, pattern->brightness);
            }
            else
            {
                setLEDBrightness(0, 0);
                setLEDBrightness(1, 0);
            }
        }
        else
        {

            setLEDBrightness(0, 0);
            setLEDBrightness(1, 0);
        }
    }
    break;

    case PATTERN_RANDOM:
    {

        if (elapsed % 300 == 0)
        {
            int randomBrightness = random(0, 2) * pattern->brightness;
            int randomChannel = random(0, 2);

            setLEDBrightness(randomChannel, randomBrightness);
            setLEDBrightness(1 - randomChannel, random(0, 2) * pattern->brightness);
        }
    }
    break;

    case PATTERN_EMERGENCY:
    {
        +int fastCycle = elapsed % 100;
        if (fastCycle < 50)
        {
            setLEDBrightness(0, 255);
            setLEDBrightness(1, 0);
        }
        else
        {
            setLEDBrightness(0, 0);
            setLEDBrightness(1, 255);
        }
    }
    break;
    }
}

void VisualDeterrent::checkThermalProtection()
{
    float maxTemp = 0;

    for (int i = 0; i < 2; i++)
    {
        if (ledChannels[i].temperature > maxTemp)
        {
            maxTemp = ledChannels[i].temperature;
        }
    }

    if (maxTemp > THERMAL_SHUTDOWN_TEMP)
    {
        if (!thermalProtection)
        {
            Serial.println("WARNING: Thermal protection activated - LED temperature: " + String(maxTemp) + "°C");
            thermalProtection = true;

            setLEDBrightness(0, 0);
            setLEDBrightness(1, 0);
        }
    }
    else if (maxTemp < THERMAL_SHUTDOWN_TEMP - 10)
    {
        if (thermalProtection)
        {
            Serial.println("INFO: Thermal protection deactivated - LED temperature: " + String(maxTemp) + "°C");
            thermalProtection = false;
        }
    }
}

int VisualDeterrent::calculateAdaptiveBrightness(int baseBrightness)
{

    float adaptiveFactor = 0.5 + ambientLight * 0.5;

    int adaptedBrightness = (int)(baseBrightness * adaptiveFactor);
    return constrain(adaptedBrightness, 0, 255);
}

void VisualDeterrent::activateAlertMode()
{
    if (!systemEnabled || thermalProtection)
        return;

    Serial.println("Visual Deterrent: Activating ALERT mode");
    currentMode = MODE_ALERT;
    currentPattern = PATTERN_SLOW_BLINK;
    patternStartTime = millis();
    patternCycle = 0;
}

void VisualDeterrent::activateStrobeMode()
{
    if (!systemEnabled || thermalProtection)
        return;

    Serial.println("Visual Deterrent: Activating STROBE mode");
    currentMode = MODE_STROBE;
    currentPattern = PATTERN_FAST_BLINK;
    patternStartTime = millis();
    patternCycle = 0;
}

void VisualDeterrent::activateEmergencyMode()
{
    Serial.println("Visual Deterrent: Activating EMERGENCY mode");
    currentMode = MODE_EMERGENCY;
    currentPattern = PATTERN_EMERGENCY;
    patternStartTime = millis();
    patternCycle = 0;

    thermalProtection = false;
}

void VisualDeterrent::deactivate()
{
    Serial.println("Visual Deterrent: Deactivating");
    currentMode = MODE_DISABLED;
    currentPattern = PATTERN_OFF;

    setLEDBrightness(0, 0);
    setLEDBrightness(1, 0);
}

void VisualDeterrent::setLEDBrightness(int channel, int brightness)
{
    if (channel >= 0 && channel < 2)
    {
        ledChannels[channel].targetBrightness = constrain(brightness, 0, 255);
    }
}

void VisualDeterrent::setStrobePattern(StrobePattern pattern)
{
    if (pattern >= 0 && pattern < MAX_STROBE_PATTERNS)
    {
        currentPattern = pattern;
        patternStartTime = millis();
        Serial.println("Strobe pattern changed to: " + patterns[pattern].name);
    }
}

void VisualDeterrent::setBrightness(int brightness)
{
    int constrainedBrightness = constrain(brightness, 0, 255);

    if (currentPattern >= 0 && currentPattern < MAX_STROBE_PATTERNS)
    {
        patterns[currentPattern].brightness = constrainedBrightness;
    }
}

void VisualDeterrent::setEnabled(bool enabled)
{
    systemEnabled = enabled;
    if (!enabled)
    {
        deactivate();
    }
}

bool VisualDeterrent::isEnabled()
{
    return systemEnabled;
}

bool VisualDeterrent::selfTest()
{
    Serial.println("Performing visual deterrent self-test...");

    bool testPassed = true;

    for (int channel = 0; channel < 2; channel++)
    {
        Serial.print("Testing LED channel " + String(channel + 1) + "... ");

        setLEDBrightness(channel, 64);
        delay(500);

        if (ledChannels[channel].currentBrightness > 50)
        {
            Serial.print("LOW OK, ");
        }
        else
        {
            Serial.print("LOW FAIL, ");
            testPassed = false;
        }

        setLEDBrightness(channel, 255);
        delay(500);

        if (ledChannels[channel].currentBrightness > 200)
        {
            Serial.println("HIGH OK");
        }
        else
        {
            Serial.println("HIGH FAIL");
            testPassed = false;
        }

        setLEDBrightness(channel, 0);
        delay(200);
    }

    Serial.print("Testing strobe patterns... ");
    for (int pattern = 1; pattern < MAX_STROBE_PATTERNS; pattern++)
    {
        setStrobePattern((StrobePattern)pattern);
        delay(1000);
    }
    Serial.println("COMPLETE");

    deactivate();

    if (testPassed)
    {
        Serial.println("Visual deterrent self-test PASSED");
    }
    else
    {
        Serial.println("Visual deterrent self-test FAILED");
    }

    return testPassed;
}

void VisualDeterrent::performLEDTest()
{
    Serial.println("Performing LED initialization test...");

    for (int i = 0; i < 2; i++)
    {
        analogWrite(ledChannels[i].pin, 128);
        delay(200);
        analogWrite(ledChannels[i].pin, 0);
        delay(200);
    }

    Serial.println("LED test completed");
}

VisualMode VisualDeterrent::getCurrentMode()
{
    return currentMode;
}

String VisualDeterrent::getModeString()
{
    switch (currentMode)
    {
    case MODE_DISABLED:
        return "DISABLED";
    case MODE_ALERT:
        return "ALERT";
    case MODE_STROBE:
        return "STROBE";
    case MODE_EMERGENCY:
        return "EMERGENCY";
    default:
        return "UNKNOWN";
    }
}

float VisualDeterrent::getLEDTemperature(int channel)
{
    if (channel >= 0 && channel < 2)
    {
        return ledChannels[channel].temperature;
    }
    return 0.0;
}

bool VisualDeterrent::isThermalProtectionActive()
{
    return thermalProtection;
}

void VisualDeterrent::forceShutdown()
{
    Serial.println("Visual Deterrent: FORCE SHUTDOWN");
    deactivate();
    systemEnabled = false;
}

String VisualDeterrent::getStatusReport()
{
    String report = "=== VISUAL DETERRENT STATUS ===\n";
    report += "Mode: " + getModeString() + "\n";
    report += "Pattern: " + patterns[currentPattern].name + "\n";
    report += "System Enabled: " + String(systemEnabled ? "YES" : "NO") + "\n";
    report += "Thermal Protection: " + String(thermalProtection ? "ACTIVE" : "INACTIVE") + "\n";
    report += "Ambient Light: " + String(ambientLight * 100) + "%\n";

    report += "\nLED Channel Status:\n";
    for (int i = 0; i < 2; i++)
    {
        report += "Channel " + String(i + 1) + ": ";
        report += String(ledChannels[i].currentBrightness) + "/" + String(ledChannels[i].targetBrightness) + " ";
        report += "(" + String(ledChannels[i].temperature) + "°C) ";
        report += String(ledChannels[i].isActive ? "ACTIVE" : "INACTIVE") + "\n";
    }

    report += "===============================\n";
    return report;
}