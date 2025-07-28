
#include <WiFiNINA.h>
#include <ArduinoJson.h>
#include "bird_detection.h"
#include "visual_deterrent.h"
#include "audio_deterrent.h"
#include "power_management.h"
#include "weather_protection.h"
#include "emergency_system.h"

#define SYSTEM_VERSION "1.0.0"
#define DEBUG_MODE true
#define BIRD_DETECTION_RADIUS_M 100
#define EMERGENCY_DISTANCE_M 20
#define MAX_ACTIVATION_TIME_MS 30000

#define LED_STROBE_PIN_1 2
#define LED_STROBE_PIN_2 3
#define AUDIO_PWM_PIN 4
#define AUDIO_ENABLE_PIN 5
#define EMERGENCY_SERVO_PIN 6
#define STATUS_LED_PIN 13

#define TRIG_PIN_1 7
#define ECHO_PIN_1 8
#define TRIG_PIN_2 9
#define ECHO_PIN_2 10
#define TRIG_PIN_3 11
#define ECHO_PIN_3 12

enum SystemState
{
    STANDBY,
    ALERT,
    ACTIVE_DETERRENT,
    EMERGENCY,
    MAINTENANCE
};

SystemState currentState = STANDBY;
unsigned long lastBirdDetection = 0;
unsigned long activationStartTime = 0;
bool systemEnabled = true;
int birdCount = 0;
float batteryVoltage = 0.0;
float systemTemperature = 0.0;

BirdDetection birdDetector;
VisualDeterrent visualSystem;
AudioDeterrent audioSystem;
PowerManagement powerManager;
WeatherProtection weatherSystem;
EmergencySystem emergencyHandler;

char ssid[] = "DRONE_NETWORK";
char pass[] = "DroneNet2024";
WiFiClient client;

void setup()
{
    Serial.begin(115200);
    while (!Serial && millis() < 5000)
        ;
    Serial.println("=== Drone Bird Deterrent System ===");
    Serial.println("Version: " + String(SYSTEM_VERSION));
    Serial.println("Initializing...");

    pinMode(STATUS_LED_PIN, OUTPUT);
    pinMode(LED_STROBE_PIN_1, OUTPUT);
    pinMode(LED_STROBE_PIN_2, OUTPUT);
    pinMode(AUDIO_PWM_PIN, OUTPUT);
    pinMode(AUDIO_ENABLE_PIN, OUTPUT);

    if (!initializeComponents())
    {
        Serial.println("CRITICAL: Component initialization failed!");
        emergencyHandler.activateEmergencyMode("INIT_FAILURE");
        return;
    }

    initializeWiFi();

    Serial.println("System initialized successfully");
    Serial.println("Entering STANDBY mode");
    digitalWrite(STATUS_LED_PIN, HIGH);

    // Initial system status
    printSystemStatus();
}

void loop()
{
    // Update system sensors
    updateSensorReadings();

    // Main state machine
    switch (currentState)
    {
    case STANDBY:
        handleStandbyMode();
        break;

    case ALERT:
        handleAlertMode();
        break;

    case ACTIVE_DETERRENT:
        handleActiveDeterrentMode();
        break;

    case EMERGENCY:
        handleEmergencyMode();
        break;

    case MAINTENANCE:
        handleMaintenanceMode();
        break;
    }

    // System health monitoring
    monitorSystemHealth();

    // Telemetry transmission
    sendTelemetryData();

    // Status LED heartbeat
    updateStatusLED();

    delay(50); // 20Hz main loop
}

bool initializeComponents()
{
    Serial.println("Initializing components...");

    if (!birdDetector.begin(TRIG_PIN_1, ECHO_PIN_1, TRIG_PIN_2, ECHO_PIN_2, TRIG_PIN_3, ECHO_PIN_3))
    {
        Serial.println("ERROR: Bird detection system failed to initialize");
        return false;
    }
    Serial.println("✓ Bird detection system initialized");

    if (!visualSystem.begin(LED_STROBE_PIN_1, LED_STROBE_PIN_2))
    {
        Serial.println("ERROR: Visual deterrent system failed to initialize");
        return false;
    }
    Serial.println("✓ Visual deterrent system initialized");

    if (!audioSystem.begin(AUDIO_PWM_PIN, AUDIO_ENABLE_PIN))
    {
        Serial.println("ERROR: Audio deterrent system failed to initialize");
        return false;
    }
    Serial.println("✓ Audio deterrent system initialized");

    // Initialize power management
    if (!powerManager.begin())
    {
        Serial.println("ERROR: Power management system failed to initialize");
        return false;
    }
    Serial.println("✓ Power management system initialized");

    // Initialize weather protection
    if (!weatherSystem.begin())
    {
        Serial.println("ERROR: Weather protection system failed to initialize");
        return false;
    }
    Serial.println("✓ Weather protection system initialized");

    // Initialize emergency system
    if (!emergencyHandler.begin(EMERGENCY_SERVO_PIN))
    {
        Serial.println("ERROR: Emergency system failed to initialize");
        return false;
    }
    Serial.println("✓ Emergency system initialized");

    return true;
}

void initializeWiFi()
{
    Serial.print("Connecting to WiFi network: ");
    Serial.println(ssid);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10)
    {
        WiFi.begin(ssid, pass);
        delay(1000);
        attempts++;
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi connected successfully");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("\nWiFi connection failed - operating in offline mode");
    }
}

void updateSensorReadings()
{

    birdDetector.update();

    // Update power management readings
    batteryVoltage = powerManager.getBatteryVoltage();
    systemTemperature = powerManager.getTemperature();

    weatherSystem.update();
}

void handleStandbyMode()
{
    // Continuous bird monitoring in low-power mode
    if (birdDetector.isBirdDetected(BIRD_DETECTION_RADIUS_M))
    {
        Serial.println("ALERT: Bird detected at " + String(birdDetector.getClosestDistance()) + "m");
        birdCount = birdDetector.getBirdCount();
        lastBirdDetection = millis();

        // Transition to alert mode
        currentState = ALERT;
        visualSystem.activateAlertMode();

        if (DEBUG_MODE)
        {
            Serial.println("State transition: STANDBY -> ALERT");
        }
    }

    powerManager.setLowPowerMode(true);
}

void handleAlertMode()
{
    float closestDistance = birdDetector.getClosestDistance();

    if (closestDistance <= EMERGENCY_DISTANCE_M)
    {
        Serial.println("EMERGENCY: Bird within " + String(EMERGENCY_DISTANCE_M) + "m!");
        currentState = ACTIVE_DETERRENT;
        activationStartTime = millis();

        // Activate full deterrent system
        visualSystem.activateStrobeMode();
        audioSystem.playDistressCalls();
        powerManager.setLowPowerMode(false);

        if (DEBUG_MODE)
        {
            Serial.println("State transition: ALERT -> ACTIVE_DETERRENT");
        }
    }
    // Check if birds moved away
    else if (!birdDetector.isBirdDetected(BIRD_DETECTION_RADIUS_M))
    {
        Serial.println("All clear - birds moved away");
        currentState = STANDBY;
        visualSystem.deactivate();

        if (DEBUG_MODE)
        {
            Serial.println("State transition: ALERT -> STANDBY");
        }
    }
}

void handleActiveDeterrentMode()
{
    unsigned long currentTime = millis();
    float closestDistance = birdDetector.getClosestDistance();

    if (closestDistance <= 5.0 && birdDetector.getBirdCount() > 2)
    {
        Serial.println("CRITICAL: Multiple birds within 5m - Emergency landing!");
        currentState = EMERGENCY;
        emergencyHandler.activateEmergencyMode("BIRD_STRIKE_IMMINENT");
        return;
    }

    if (currentTime - activationStartTime > MAX_ACTIVATION_TIME_MS)
    {
        Serial.println("Maximum deterrent time reached - switching to emergency mode");
        currentState = EMERGENCY;
        emergencyHandler.activateEmergencyMode("DETERRENT_TIMEOUT");
        return;
    }

    if (!birdDetector.isBirdDetected(BIRD_DETECTION_RADIUS_M * 2))
    {
        Serial.println("SUCCESS: Birds successfully deterred");
        currentState = STANDBY;
        visualSystem.deactivate();
        audioSystem.stop();

        if (DEBUG_MODE)
        {
            Serial.println("State transition: ACTIVE_DETERRENT -> STANDBY");
        }
    }

    visualSystem.update();
    audioSystem.update();
}

void handleEmergencyMode()
{

    emergencyHandler.update();

    // All deterrent systems at maximum
    visualSystem.activateEmergencyMode();
    audioSystem.playEmergencySignals();

    // Check if emergency resolved
    if (emergencyHandler.isEmergencyResolved())
    {
        Serial.println("Emergency resolved - returning to standby");
        currentState = STANDBY;
        visualSystem.deactivate();
        audioSystem.stop();
    }
}

void handleMaintenanceMode()
{

    Serial.println("=== MAINTENANCE MODE ===");

    bool allTestsPassed = true;

    allTestsPassed &= birdDetector.selfTest();
    allTestsPassed &= visualSystem.selfTest();
    allTestsPassed &= audioSystem.selfTest();
    allTestsPassed &= powerManager.selfTest();
    allTestsPassed &= weatherSystem.selfTest();
    allTestsPassed &= emergencyHandler.selfTest();

    if (allTestsPassed)
    {
        Serial.println("All systems passed self-test");
        currentState = STANDBY;
    }
    else
    {
        Serial.println("CRITICAL: System failures detected");
    }
}

void monitorSystemHealth()
{
    // Battery voltage monitoring
    if (batteryVoltage < 10.5)
    { // Below safe operating voltage
        Serial.println("WARNING: Low battery voltage: " + String(batteryVoltage) + "V");
        emergencyHandler.reportHealthIssue("LOW_BATTERY");
    }

    // Temperature monitoring
    if (systemTemperature > 60.0)
    { // Overheating threshold
        Serial.println("WARNING: High system temperature: " + String(systemTemperature) + "°C");
        emergencyHandler.reportHealthIssue("OVERHEATING");
    }

    // Weather conditions
    if (weatherSystem.isWeatherCritical())
    {
        Serial.println("WARNING: Critical weather conditions detected");
        emergencyHandler.reportHealthIssue("SEVERE_WEATHER");
    }
}

void sendTelemetryData()
{
    static unsigned long lastTelemetry = 0;

    if (millis() - lastTelemetry > 5000)
    { // Send every 5 seconds
        if (WiFi.status() == WL_CONNECTED)
        {
            DynamicJsonDocument telemetry(1024);

            telemetry["timestamp"] = millis();
            telemetry["state"] = getStateString(currentState);
            telemetry["battery_voltage"] = batteryVoltage;
            telemetry["temperature"] = systemTemperature;
            telemetry["bird_count"] = birdCount;
            telemetry["closest_bird_distance"] = birdDetector.getClosestDistance();
            telemetry["weather_status"] = weatherSystem.getWeatherStatus();
            telemetry["system_health"] = emergencyHandler.getHealthStatus();

            String telemetryString;
            serializeJson(telemetry, telemetryString);

            if (DEBUG_MODE)
            {
                Serial.println("Telemetry: " + telemetryString);
            }
        }

        lastTelemetry = millis();
    }
}

void updateStatusLED()
{
    static unsigned long lastBlink = 0;
    static bool ledState = false;

    unsigned long blinkInterval;

    switch (currentState)
    {
    case STANDBY:
        blinkInterval = 2000; // Slow blink
        break;
    case ALERT:
        blinkInterval = 500; // Fast blink
        break;
    case ACTIVE_DETERRENT:
        blinkInterval = 100; // Very fast blink
        break;
    case EMERGENCY:
        digitalWrite(STATUS_LED_PIN, HIGH); // Solid on
        return;
    case MAINTENANCE:
        blinkInterval = 1000; // Medium blink
        break;
    }

    if (millis() - lastBlink > blinkInterval)
    {
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState);
        lastBlink = millis();
    }
}

String getStateString(SystemState state)
{
    switch (state)
    {
    case STANDBY:
        return "STANDBY";
    case ALERT:
        return "ALERT";
    case ACTIVE_DETERRENT:
        return "ACTIVE_DETERRENT";
    case EMERGENCY:
        return "EMERGENCY";
    case MAINTENANCE:
        return "MAINTENANCE";
    default:
        return "UNKNOWN";
    }
}

void printSystemStatus()
{
    Serial.println("\n=== SYSTEM STATUS ===");
    Serial.println("State: " + getStateString(currentState));
    Serial.println("Battery: " + String(batteryVoltage) + "V");
    Serial.println("Temperature: " + String(systemTemperature) + "°C");
    Serial.println("Birds detected: " + String(birdCount));
    Serial.println("WiFi: " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected"));
    Serial.println("====================\n");
}