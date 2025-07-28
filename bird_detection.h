

#ifndef BIRD_DETECTION_H
#define BIRD_DETECTION_H

#include <Arduino.h>

#define MAX_BIRDS 10
#define SENSOR_COUNT 3
#define DETECTION_HISTORY_SIZE 10
#define MIN_BIRD_SIZE_CM 15
#define MAX_BIRD_SIZE_CM 200
#define BIRD_SPEED_THRESHOLD_MPS 2.0
#define NOISE_FILTER_SAMPLES 5

struct BirdObject
{
    float distance;
    float azimuth;
    float lastDistance;
    unsigned long lastSeen;
    float velocity;
    bool confirmed;
    int confidenceLevel;
};

struct SensorData
{
    int trigPin;
    int echoPin;
    float lastDistance;
    float distanceHistory[DETECTION_HISTORY_SIZE];
    int historyIndex;
    unsigned long lastReading;
    bool sensorActive;
};

class BirdDetection
{
private:
    SensorData sensors[SENSOR_COUNT];
    BirdObject detectedBirds[MAX_BIRDS];
    int activeBirdCount;
    float closestBirdDistance;
    bool systemEnabled;
    unsigned long lastUpdate;

    float readUltrasonicDistance(int sensorIndex);
    bool isValidBirdSignature(float distance, float previousDistance);
    void updateBirdTracking();
    void filterNoise(int sensorIndex, float rawDistance);
    float calculateAzimuth(int sensorIndex);
    bool detectBirdMovement(int birdIndex);
    void removeStaleDetections();
    int findClosestFreeBirdSlot();

public:
    BirdDetection();
    bool begin(int trig1, int echo1, int trig2, int echo2, int trig3, int echo3);
    void update();
    bool isBirdDetected(float maxRange);
    int getBirdCount();
    float getClosestDistance();
    BirdObject *getBirdData(int index);
    bool selfTest();
    void calibrateSensors();
    void setEnabled(bool enabled);
    bool isEnabled();
    void resetDetection();
    float getBirdVelocity(int birdIndex);
    String getDetectionReport();
};

#endif