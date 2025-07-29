
#ifndef VISUAL_DETERRENT_H
#define VISUAL_DETERRENT_H

#include <Arduino.h>

#define MAX_STROBE_PATTERNS 5
#define PWM_RESOLUTION 255
#define THERMAL_SHUTDOWN_TEMP 70.0
#define MAX_CONTINUOUS_ON_TIME 5000
#define LED_WARMUP_TIME 100

enum StrobePattern
{
    PATTERN_OFF = 0,
    PATTERN_SLOW_BLINK = 1,
    PATTERN_FAST_BLINK = 2,
    PATTERN_DOUBLE_FLASH = 3,
    PATTERN_RANDOM = 4,
    PATTERN_EMERGENCY = 5
};

enum VisualMode
{
    MODE_DISABLED = 0,
    MODE_ALERT = 1,
    MODE_STROBE = 2,
    MODE_EMERGENCY = 3
};

struct LEDChannel
{
    int pin;
    int currentBrightness;
    int targetBrightness;
    unsigned long lastUpdate;
    bool isActive;
    unsigned long onTime;
    float temperature;
};

struct StrobeConfig
{
    int onDuration;
    int offDuration;
    int brightness;
    int repetitions;
    String name;
};

class VisualDeterrent
{
private:
    LEDChannel ledChannels[2];
    VisualMode currentMode;
    StrobePattern currentPattern;
    StrobeConfig patterns[MAX_STROBE_PATTERNS];
    unsigned long patternStartTime;
    int patternCycle;
    bool systemEnabled;
    float ambientLight;
    bool thermalProtection;
    unsigned long lastThermalCheck;

    void updateLEDBrightness(int channel);
    void setLEDBrightness(int channel, int brightness);
    void executeStrobePattern();
    void checkThermalProtection();
    float readAmbientLight();
    void initializePatterns();
    int calculateAdaptiveBrightness(int baseBrightness);
    void performLEDTest();

public:
    VisualDeterrent();
    bool begin(int led1Pin, int led2Pin);
    void update();
    void activateAlertMode();
    void activateStrobeMode();
    void activateEmergencyMode();
    void deactivate();
    void setStrobePattern(StrobePattern pattern);
    void setBrightness(int brightness);
    void setEnabled(bool enabled);
    bool isEnabled();
    bool selfTest();
    VisualMode getCurrentMode();
    String getModeString();
    float getLEDTemperature(int channel);
    bool isThermalProtectionActive();
    void forceShutdown();
    String getStatusReport();
};

#endif