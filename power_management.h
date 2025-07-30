

#ifndef POWER_MANAGEMENT_H
#define POWER_MANAGEMENT_H

#include <Arduino.h>

#define VOLTAGE_RAILS 3
#define POWER_SAMPLES 10
#define BATTERY_MIN_VOLTAGE 10.5
#define BATTERY_MAX_VOLTAGE 16.8
#define THERMAL_SHUTDOWN_TEMP 75.0
#define LOW_POWER_THRESHOLD 11.0
#define EFFICIENCY_TARGET 0.95

enum PowerRail
{
    RAIL_12V = 0,
    RAIL_5V = 1,
    RAIL_3V3 = 2
};

enum PowerMode
{
    POWER_NORMAL = 0,
    POWER_LOW = 1,
    POWER_CRITICAL = 2,
    POWER_EMERGENCY = 3
};

struct VoltageRail
{
    PowerRail railId;
    String name;
    float targetVoltage;
    float currentVoltage;
    float currentDraw;
    float maxCurrent;
    bool enabled;
    bool overcurrent;
    float efficiency;
    int enablePin;
    int voltagePin;
    int currentPin;
};

struct PowerMetrics
{
    float batteryVoltage;
    float batteryCurrent;
    float totalPowerConsumption;
    float systemEfficiency;
    float temperature;
    unsigned long uptimeHours;
    float energyConsumed;
    PowerMode currentMode;
};

class PowerManagement
{
private:
    VoltageRail rails[VOLTAGE_RAILS];
    PowerMetrics metrics;
    PowerMode currentMode;
    bool lowPowerMode;
    bool emergencyShutdown;
    float voltageHistory[POWER_SAMPLES];
    int voltageHistoryIndex;
    unsigned long lastMetricsUpdate;
    unsigned long systemStartTime;
    float temperatureSensors[3];

    void initializeRails();
    void updateVoltageReadings();
    void updateCurrentReadings();
    void calculateEfficiency();
    void checkOvercurrentConditions();
    void checkThermalConditions();
    void adjustRailVoltages();
    void updatePowerMetrics();
    float readBatteryVoltage();
    float readBatteryCurrent();
    float readSystemTemperature();
    void enableRail(PowerRail rail, bool enable);
    void setRailVoltage(PowerRail rail, float voltage);
    bool isRailHealthy(PowerRail rail);
    void performLoadBalancing();

public:
    PowerManagement();
    bool begin();
    void update();
    float getBatteryVoltage();
    float getBatteryCurrent();
    float getTotalPowerConsumption();
    float getSystemEfficiency();
    float getTemperature();
    PowerMode getCurrentMode();
    void setLowPowerMode(bool enabled);
    bool isLowPowerMode();
    void emergencyPowerShutdown();
    bool isEmergencyShutdown();
    VoltageRail *getRailInfo(PowerRail rail);
    PowerMetrics getMetrics();
    bool selfTest();
    String getPowerReport();
    void resetEnergyCounters();
    float getRailVoltage(PowerRail rail);
    float getRailCurrent(PowerRail rail);
    bool isRailEnabled(PowerRail rail);
    void calibratePowerReadings();
    String getModeString();
    bool isBatteryHealthy();
    float getEstimatedRuntime();
};

#endif