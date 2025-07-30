
#ifndef WEATHER_PROTECTION_H
#define WEATHER_PROTECTION_H

#include <Arduino.h>

#define MAX_WEATHER_SENSORS 5
#define WEATHER_HISTORY_SIZE 20
#define CRITICAL_TEMP_HIGH 50.0
#define CRITICAL_TEMP_LOW -10.0
#define CRITICAL_HUMIDITY 90.0
#define CRITICAL_WIND_SPEED 30.0
#define CRITICAL_PRESSURE_DROP 20.0

enum WeatherCondition
{
  WEATHER_CLEAR = 0,
  WEATHER_LIGHT_RAIN = 1,
  WEATHER_HEAVY_RAIN = 2,
  WEATHER_SNOW = 3,
  WEATHER_HIGH_WIND = 4,
  WEATHER_STORM = 5,
  WEATHER_EXTREME = 6
};

enum ProtectionMode
{
  PROTECTION_NORMAL = 0,
  PROTECTION_ENHANCED = 1,
  PROTECTION_EMERGENCY = 2,
  PROTECTION_SHUTDOWN = 3
};

struct WeatherSensor
{
  String name;
  int pin;
  float currentReading;
  float history[WEATHER_HISTORY_SIZE];
  int historyIndex;
  bool active;
  float calibrationOffset;
  unsigned long lastReading;
};

struct WeatherData
{
  float temperature;
  float humidity;
  float pressure;
  float windSpeed;
  float precipitation;
  float lightLevel;
  WeatherCondition condition;
  bool criticalWeather;
  unsigned long timestamp;
};

struct EnclosureStatus
{
  float internalTemp;
  float internalHumidity;
  bool sealIntegrity;
  bool ventilationActive;
  bool heaterActive;
  bool desiccantActive;
  float pressureDifferential;
};

class WeatherProtection
{
private:
  WeatherSensor sensors[MAX_WEATHER_SENSORS];
  WeatherData currentWeather;
  EnclosureStatus enclosureStatus;
  ProtectionMode currentMode;
  bool systemEnabled;
  unsigned long lastWeatherUpdate;
  unsigned long lastEnclosureCheck;
  bool weatherCritical;

  int tempSensorIndex;
  int humiditySensorIndex;
  int pressureSensorIndex;
  int windSensorIndex;
  int precipitationSensorIndex;

  int ventilationPin;
  int heaterPin;
  int desiccantPin;
  int sealMonitorPin;

  void initializeSensors();
  void updateWeatherReadings();
  void analyzeWeatherConditions();
  void updateEnclosureStatus();
  void controlEnvironmentalSystems();
  float readTemperature();
  float readHumidity();
  float readPressure();
  float readWindSpeed();
  float readPrecipitation();
  float readLightLevel();
  void activateVentilation(bool enable);
  void activateHeater(bool enable);
  void activateDesiccant(bool enable);
  bool checkSealIntegrity();
  WeatherCondition classifyWeatherCondition();
  void adaptSystemToWeather();
  bool isWeatherSafe();
  void logWeatherEvent(String event);

public:
  WeatherProtection();
  bool begin();
  void update();
  WeatherData getWeatherData();
  EnclosureStatus getEnclosureStatus();
  bool isWeatherCritical();
  WeatherCondition getCurrentCondition();
  String getWeatherStatus();
  ProtectionMode getProtectionMode();
  void setProtectionMode(ProtectionMode mode);
  bool selfTest();
  void calibrateSensors();
  float getSensorReading(int sensorIndex);
  bool isSensorActive(int sensorIndex);
  void enableWeatherProtection(bool enable);
  bool isWeatherProtectionEnabled();
  String getWeatherReport();
  void emergencyWeatherShutdown();
  bool isEnclosureCompromised();
  float getInternalTemperature();
  float getInternalHumidity();
  void forceVentilation();
  void resetWeatherHistory();
};

#endif