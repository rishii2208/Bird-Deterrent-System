

#include "bird_detection.h"

BirdDetection::BirdDetection()
{
    activeBirdCount = 0;
    closestBirdDistance = 9999.0;
    systemEnabled = true;
    lastUpdate = 0;

    for (int i = 0; i < MAX_BIRDS; i++)
    {
        detectedBirds[i].distance = 9999.0;
        detectedBirds[i].azimuth = 0.0;
        detectedBirds[i].lastDistance = 9999.0;
        detectedBirds[i].lastSeen = 0;
        detectedBirds[i].velocity = 0.0;
        detectedBirds[i].confirmed = false;
        detectedBirds[i].confidenceLevel = 0;
    }
}

bool BirdDetection::begin(int trig1, int echo1, int trig2, int echo2, int trig3, int echo3)
{
    Serial.println("Initializing Bird Detection System...");

    sensors[0].trigPin = trig1;
    sensors[0].echoPin = echo1;
    sensors[0].sensorActive = true;
    pinMode(trig1, OUTPUT);
    pinMode(echo1, INPUT);

    sensors[1].trigPin = trig2;
    sensors[1].echoPin = echo2;
    sensors[1].sensorActive = true;
    pinMode(trig2, OUTPUT);
    pinMode(echo2, INPUT);

    sensors[2].trigPin = trig3;
    sensors[2].echoPin = echo3;
    sensors[2].sensorActive = true;
    pinMode(trig3, OUTPUT);
    pinMode(echo3, INPUT);

    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        sensors[i].lastDistance = 9999.0;
        sensors[i].historyIndex = 0;
        sensors[i].lastReading = 0;
        for (int j = 0; j < DETECTION_HISTORY_SIZE; j++)
        {
            sensors[i].distanceHistory[j] = 9999.0;
        }
    }

    calibrateSensors();

    Serial.println("Bird Detection System initialized successfully");
    return true;
}

void BirdDetection::update()
{
    if (!systemEnabled)
        return;

    unsigned long currentTime = millis();

    for (int i = 0; i < SENSOR_COUNT; i++)
    {
        if (currentTime - sensors[i].lastReading > (50 + i * 20))
        {
            float distance = readUltrasonicDistance(i);

            if (distance > 0)
            {
                filterNoise(i, distance);
                sensors[i].lastReading = currentTime;
            }
        }
    }

    if (currentTime - lastUpdate > 100)
    {
        updateBirdTracking();
        removeStaleDetections();
        lastUpdate = currentTime;
    }
}

float BirdDetection::readUltrasonicDistance(int sensorIndex)
{
    if (!sensors[sensorIndex].sensorActive)
        return -1;

    int trigPin = sensors[sensorIndex].trigPin;
    int echoPin = sensors[sensorIndex].echoPin;

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    unsigned long duration = pulseIn(echoPin, HIGH, 30000);

    if (duration == 0)
        return -1;

    float distance = duration * 0.034 / 2.0;

    if (distance < 2 || distance > 400)
        return -1;

    return distance;
}

void BirdDetection::filterNoise(int sensorIndex, float rawDistance)
{

    sensors[sensorIndex].distanceHistory[sensors[sensorIndex].historyIndex] = rawDistance;
    sensors[sensorIndex].historyIndex = (sensors[sensorIndex].historyIndex + 1) % DETECTION_HISTORY_SIZE;

    float sortedDistances[NOISE_FILTER_SAMPLES];
    int startIndex = (sensors[sensorIndex].historyIndex + DETECTION_HISTORY_SIZE - NOISE_FILTER_SAMPLES) % DETECTION_HISTORY_SIZE;

    for (int i = 0; i < NOISE_FILTER_SAMPLES; i++)
    {
        int index = (startIndex + i) % DETECTION_HISTORY_SIZE;
        sortedDistances[i] = sensors[sensorIndex].distanceHistory[index];
    }

    for (int i = 0; i < NOISE_FILTER_SAMPLES - 1; i++)
    {
        for (int j = 0; j < NOISE_FILTER_SAMPLES - i - 1; j++)
        {
            if (sortedDistances[j] > sortedDistances[j + 1])
            {
                float temp = sortedDistances[j];
                sortedDistances[j] = sortedDistances[j + 1];
                sortedDistances[j + 1] = temp;
            }
        }
    }

    float filteredDistance = sortedDistances[NOISE_FILTER_SAMPLES / 2];
    sensors[sensorIndex].lastDistance = filteredDistance;
}

void BirdDetection::updateBirdTracking()
{
    closestBirdDistance = 9999.0;
    activeBirdCount = 0;

    for (int sensorIndex = 0; sensorIndex < SENSOR_COUNT; sensorIndex++)
    {
        float distance = sensors[sensorIndex].lastDistance;

        if (isValidBirdSignature(distance, sensors[sensorIndex].distanceHistory[0]))
        {
            float azimuth = calculateAzimuth(sensorIndex);

            bool matched = false;

            for (int birdIndex = 0; birdIndex < MAX_BIRDS; birdIndex++)
            {
                if (detectedBirds[birdIndex].confirmed)
                {

                    float azimuthDiff = abs(detectedBirds[birdIndex].azimuth - azimuth);
                    float distanceDiff = abs(detectedBirds[birdIndex].distance - distance);

                    if (azimuthDiff < 30.0 && distanceDiff < distance * 0.2)
                    { // 20% distance tolerance

                        detectedBirds[birdIndex].lastDistance = detectedBirds[birdIndex].distance;
                        detectedBirds[birdIndex].distance = distance;
                        detectedBirds[birdIndex].azimuth = azimuth;
                        detectedBirds[birdIndex].lastSeen = millis();

                        unsigned long timeDiff = millis() - detectedBirds[birdIndex].lastSeen;
                        if (timeDiff > 0)
                        {
                            float distanceChange = abs(detectedBirds[birdIndex].distance - detectedBirds[birdIndex].lastDistance);
                            detectedBirds[birdIndex].velocity = (distanceChange / 100.0) / (timeDiff / 1000.0); // m/s
                        }

                        detectedBirds[birdIndex].confidenceLevel = min(100, detectedBirds[birdIndex].confidenceLevel + 10);
                        matched = true;
                        break;
                    }
                }
            }

            if (!matched)
            {
                int newBirdIndex = findClosestFreeBirdSlot();
                if (newBirdIndex != -1)
                {
                    detectedBirds[newBirdIndex].distance = distance;
                    detectedBirds[newBirdIndex].azimuth = azimuth;
                    detectedBirds[newBirdIndex].lastDistance = distance;
                    detectedBirds[newBirdIndex].lastSeen = millis();
                    detectedBirds[newBirdIndex].velocity = 0.0;
                    detectedBirds[newBirdIndex].confirmed = true;
                    detectedBirds[newBirdIndex].confidenceLevel = 20;
                }
            }
        }
    }
    for (int i = 0; i < MAX_BIRDS; i++)
    {
        if (detectedBirds[i].confirmed && detectedBirds[i].confidenceLevel > 30)
        {
            activeBirdCount++;
            if (detectedBirds[i].distance < closestBirdDistance)
            {
                closestBirdDistance = detectedBirds[i].distance;
            }
        }
    }
}

bool BirdDetection::isValidBirdSignature(float distance, float previousDistance)
{

    if (distance < MIN_BIRD_SIZE_CM || distance > 500)
        return false;

    float distanceChange = abs(distance - previousDistance);

    if (distanceChange < 2.0)
        return false;

    if (distanceChange > 100.0)
        return false;

    return true;
}

float BirdDetection::calculateAzimuth(int sensorIndex)
{

    switch (sensorIndex)
    {
    case 0:
        return 0.0; // Front
    case 1:
        return 270.0; // Left
    case 2:
        return 90.0; // Right
    default:
        return 0.0;
    }
}

void BirdDetection::removeStaleDetections()
{
    unsigned long currentTime = millis();

    for (int i = 0; i < MAX_BIRDS; i++)
    {
        if (detectedBirds[i].confirmed)
        {

            if (currentTime - detectedBirds[i].lastSeen > 2000)
            {
                detectedBirds[i].confirmed = false;
                detectedBirds[i].confidenceLevel = 0;
            }

            else if (currentTime - detectedBirds[i].lastSeen > 500)
            {
                detectedBirds[i].confidenceLevel = max(0, detectedBirds[i].confidenceLevel - 5);
                if (detectedBirds[i].confidenceLevel < 10)
                {
                    detectedBirds[i].confirmed = false;
                }
            }
        }
    }
}

int BirdDetection::findClosestFreeBirdSlot()
{
    for (int i = 0; i < MAX_BIRDS; i++)
    {
    if (!detectedB