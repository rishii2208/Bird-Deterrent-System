#include "audio_deterrent.h"
#include <math.h>

AudioDeterrent::AudioDeterrent()
{
    currentMode = AUDIO_DISABLED;
    currentPattern = AUDIO_OFF;
    patternStartTime = 0;
    patternCycle = 0;
    currentFrequencyIndex = 0;
    systemEnabled = true;
    environmentNoise = 0.0;
    volumeLimiting = false;
    lastPatternRotation = 0;
    patternRotationIndex = 0;
    bufferIndex = 0;
    sampleRate = 8000;

    audioChannel.pwmPin = -1;
    audioChannel.enablePin = -1;
    audioChannel.currentVolume = 0.0;
    audioChannel.targetVolume = 0.0;
    audioChannel.isActive = false;
    audioChannel.lastUpdate = 0;
    audioChannel.temperature = 25.0;

    for (int i = 0; i < AUDIO_BUFFER_SIZE; i++)
    {
        audioBuffer[i] = 0.0;
    }
}

bool AudioDeterrent::begin(int pwmPin, int enablePin)
{
    Serial.println("Initializing Audio Deterrent System...");

    audioChannel.pwmPin = pwmPin;
    audioChannel.enablePin = enablePin;

    pinMode(pwmPin, OUTPUT);
    pinMode(enablePin, OUTPUT);

    analogWrite(pwmPin, 0);
    digitalWrite(enablePin, LOW);

    initializeAudioPatterns();

    calibrateEnvironmentNoise();

    performAudioTest();

    Serial.println("Audio Deterrent System initialized successfully");
    return true;
}

void AudioDeterrent::initializeAudioPatterns()
{
    Serial.println("Initializing audio patterns...");

    // Pattern 0: Off
    patterns[0] = {AUDIO_OFF, "Off", {}, 0, 0, 0, 0.0, false};

    patterns[1] = {CROW_DISTRESS, "Crow Distress", {{800, 0.8, 0, 500}, {1200, 0.6, 0, 300}, {600, 0.9, 0, 400}}, 3, 3, 1000, 0.7, false};

    patterns[2] = {EAGLE_DISTRESS, "Eagle Distress", {{1800, 0.9, 0, 800}, {1200, 0.7, 0, 600}, {2200, 0.8, 0, 500}}, 3, 2, 2000, 0.8, false};

    patterns[3] = {HAWK_SCREECH, "Hawk Screech", {{2500, 1.0, 0, 1200}, {1800, 0.6, 0, 800}}, 2, 4, 1500, 0.75, false};

    patterns[4] = {GENERAL_ALARM, "General Alarm", {{1000, 0.8, 0, 200}, {1500, 0.8, 0, 200}, {2000, 0.8, 0, 200}}, 3, 10, 500, 0.6, false};

    patterns[5] = {ULTRASONIC_SWEEP, "Ultrasonic Sweep", {{17000, 0.5, 0, 1000}, {20000, 0.5, 0, 1000}, {24000, 0.5, 0, 1000}}, 3, 5, 200, 0.4, true};

    patterns[6] = {PREDATOR_GROWL, "Predator Growl", {{150, 0.9, 0, 2000}, {80, 0.7, 0, 1500}, {200, 0.8, 0, 1000}}, 3, 2, 3000, 0.8, false};

    patterns[7] = {EMERGENCY_SIREN, "Emergency Siren", {{800, 1.0, 0, 300}, {1200, 1.0, 0, 300}}, 2, 999, 0, 0.9, false};

    Serial.println("Audio patterns initialized successfully");
}

void AudioDeterrent::update()
{
    if (!systemEnabled)
        return;

    unsigned long currentTime = millis();

    updateAudioOutput();

    if (currentMode != AUDIO_DISABLED && currentPattern != AUDIO_OFF)
    {
        generateAudioWaveform();
    }

    applyVolumeControl();

    if (currentTime - lastPatternRotation > PATTERN_ROTATION_TIME)
    {
        rotatePatterns();
        lastPatternRotation = currentTime;
    }

    if (currentTime % 5000 == 0)
    {
        adaptToEnvironment();
    }
}

void AudioDeterrent::generateAudioWaveform()
{
    if (currentPattern == AUDIO_OFF)
        return;

    AudioPatternConfig *pattern = &patterns[currentPattern];
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - patternStartTime;

    if (currentFrequencyIndex < pattern->frequencyCount)
    {
        FrequencyComponent *freq = &pattern->frequencies[currentFrequencyIndex];

        if (elapsed < freq->duration)
        {

            synthesizeFrequency(freq->frequency, freq->amplitude, freq->duration);
        }
        else
        {

            currentFrequencyIndex++;
            if (currentFrequencyIndex >= pattern->frequencyCount)
            {

                patternCycle++;
                currentFrequencyIndex = 0;
                patternStartTime = currentTime;

                if (patternCycle >= pattern->repeatCount && pattern->repeatCount != 999)
                {

                    delay(pattern->pauseDuration);
                    currentPattern = AUDIO_OFF;
                    currentMode = AUDIO_STANDBY;
                }
            }
        }
    }
}

void AudioDeterrent::synthesizeFrequency(float frequency, float amplitude, float duration)
{
    static unsigned long lastSampleTime = 0;
    unsigned long currentTime = micros();

    if (currentTime - lastSampleTime >= (1000000 / sampleRate))
    {

        float phase = (2.0 * PI * frequency * (currentTime / 1000000.0));
        float sample = amplitude * sin(phase);

        audioBuffer[bufferIndex] = sample;
        bufferIndex = (bufferIndex + 1) % AUDIO_BUFFER_SIZE;

        int pwmValue = (int)((sample + 1.0) * 127.5);
        pwmValue = constrain(pwmValue, 0, 255);

        if (audioChannel.isActive)
        {
            analogWrite(audioChannel.pwmPin, pwmValue);
        }

        lastSampleTime = currentTime;
    }
}

void AudioDeterrent::updateAudioOutput()
{

    if (audioChannel.currentVolume != audioChannel.targetVolume)
    {
        float step = 0.05;
        if (abs(audioChannel.targetVolume - audioChannel.currentVolume) < step)
        {
            audioChannel.currentVolume = audioChannel.targetVolume;
        }
        else
        {
            audioChannel.currentVolume += (audioChannel.targetVolume > audioChannel.currentVolume) ? step : -step;
        }
    }

    if (audioChannel.currentVolume > 0.01 && !audioChannel.isActive)
    {
        digitalWrite(audioChannel.enablePin, HIGH);
        audioChannel.isActive = true;
        Serial.println("Audio amplifier enabled");
    }
    else if (audioChannel.currentVolume <= 0.01 && audioChannel.isActive)
    {
        digitalWrite(audioChannel.enablePin, LOW);
        analogWrite(audioChannel.pwmPin, 0);
        audioChannel.isActive = false;
        Serial.println("Audio amplifier disabled");
    }

    if (audioChannel.isActive)
    {
        audioChannel.temperature += 0.1;
    }
    else
    {
        audioChannel.temperature = max(25.0, audioChannel.temperature - 0.05); // Cool down
    }
}

void AudioDeterrent::playDistressCalls()
{
    if (!systemEnabled || volumeLimiting)
        return;

    Serial.println("Audio Deterrent: Playing distress calls");
    currentMode = AUDIO_ACTIVE;

    if (environmentNoise < 0.3)
    {
        setPattern(CROW_DISTRESS);
    }
    else if (environmentNoise < 0.6)
    {
        setPattern(EAGLE_DISTRESS);
    }
    else
    {
        setPattern(HAWK_SCREECH);
    }

    audioChannel.targetVolume = patterns[currentPattern].baseVolume;
}

void AudioDeterrent::playEmergencySignals()
{
    Serial.println("Audio Deterrent: Playing emergency signals");
    currentMode = AUDIO_EMERGENCY;
    setPattern(EMERGENCY_SIREN);
    audioChannel.targetVolume = 0.9;

    void AudioDeterrent::playUltrasonicDeterrent()
    {
        if (!systemEnabled)
            return;

        Serial.println("Audio Deterrent: Playing ultrasonic deterrent");
        currentMode = AUDIO_ACTIVE;
        setPattern(ULTRASONIC_SWEEP);
        audioChannel.targetVolume = patterns[currentPattern].baseVolume;
    }

    void AudioDeterrent::stop()
    {
        Serial.println("Audio Deterrent: Stopping");
        currentMode = AUDIO_STANDBY;
        currentPattern = AUDIO_OFF;
        audioChannel.targetVolume = 0.0;
        patternCycle = 0;
        currentFrequencyIndex = 0;
    }

    void AudioDeterrent::setVolume(float volume)
    {
        float constrainedVolume = constrain(volume, 0.0, 1.0);

        if (isVolumeWithinLimits())
        {
            audioChannel.targetVolume = constrainedVolume;
        }
        else
        {
            Serial.println("WARNING: Volume limited for safety");
            volumeLimiting = true;
            audioChannel.targetVolume = 0.6;
        }
    }

    void AudioDeterrent::setPattern(AudioPattern pattern)
    {
        if (pattern >= 0 && pattern < MAX_AUDIO_PATTERNS)
        {
            currentPattern = pattern;
            patternStartTime = millis();
            patternCycle = 0;
            currentFrequencyIndex = 0;

            Serial.println("Audio pattern changed to: " + patterns[pattern].name);
        }
    }

    void AudioDeterrent::applyVolumeControl()
    {

        float noiseCompensation = environmentNoise * 0.3;
        float adjustedVolume = audioChannel.targetVolume + noiseCompensation;

        if (adjustedVolume > 0.85)
        {
            volumeLimiting = true;
        }
        else
        {
            volumeLimiting = false;
        }

        if (audioChannel.temperature > 60.0)
        {
            adjustedVolume *= 0.7;
        }

        audioChannel.currentVolume = adjustedVolume;
    }

    void AudioDeterrent::rotatePatterns()
    {
        if (currentMode != AUDIO_ACTIVE)
            return;

        AudioPattern rotationPatterns[] = {CROW_DISTRESS, EAGLE_DISTRESS, HAWK_SCREECH, GENERAL_ALARM};
        int rotationCount = sizeof(rotationPatterns) / sizeof(AudioPattern);

        patternRotationIndex = (patternRotationIndex + 1) % rotationCount;
        AudioPattern newPattern = rotationPatterns[patternRotationIndex];

        if (isPatternEffective(newPattern))
        {
            setPattern(newPattern);
            Serial.println("Pattern rotated to prevent habituation: " + patterns[newPattern].name);
        }
    }

    void AudioDeterrent::calibrateEnvironmentNoise()
    {
        Serial.println("Calibrating environment noise baseline...");

        float totalNoise = 0;
        int samples = 50;

        for (int i = 0; i < samples; i++)
        {
            float noise = readEnvironmentNoise();
            totalNoise += noise;
            delay(100);
        }

        environmentNoise = totalNoise / samples;
        Serial.println("Environment noise baseline: " + String(environmentNoise * 100) + "%");
    }

    float AudioDeterrent::readEnvironmentNoise()
    {

        int noiseReading = analogRead(A1);
        return noiseReading / 1023.0;
    }

    void AudioDeterrent::adaptToEnvironment()
    {
        float currentNoise = readEnvironmentNoise();

        environmentNoise = (environmentNoise * 0.9) + (currentNoise * 0.1);

        if (currentMode == AUDIO_ACTIVE)
        {
            if (environmentNoise > 0.7)
            {

                if (currentPattern != HAWK_SCREECH && currentPattern != EMERGENCY_SIREN)
                {
                    setPattern(HAWK_SCREECH);
                }
            }
            else if (environmentNoise < 0.3)
            {

                if (currentPattern != CROW_DISTRESS)
                {
                    setPattern(CROW_DISTRESS);
                }
            }
        }
    }

    bool AudioDeterrent::isVolumeWithinLimits()
    {

        float estimatedDB = audioChannel.targetVolume * MAX_VOLUME_DB;
        return estimatedDB <= MAX_VOLUME_DB;
    }

    bool AudioDeterrent::isPatternEffective(AudioPattern pattern)
    {

        if (patterns[pattern].isUltrasonic && environmentNoise > 0.8)
        {
            return false;
        }

        return true;
    }

    void AudioDeterrent::setEnabled(bool enabled)
    {
        systemEnabled = enabled;
        if (!enabled)
        {
            stop();
        }
    }

    bool AudioDeterrent::isEnabled()
    {
        return systemEnabled;
    }

    bool AudioDeterrent::selfTest()
    {
        Serial.println("Performing audio deterrent self-test...");

        bool testPassed = true;

        Serial.print("Testing amplifier enable... ");
        digitalWrite(audioChannel.enablePin, HIGH);
        delay(100);
        if (digitalRead(audioChannel.enablePin) == HIGH)
        {
            Serial.println("PASS");
        }
        else
        {
            Serial.println("FAIL");
            testPassed = false;
        }

        Serial.print("Testing PWM output... ");
        analogWrite(audioChannel.pwmPin, 128);
        delay(500);
        analogWrite(audioChannel.pwmPin, 0);
        Serial.println("COMPLETE");

        Serial.print("Testing audio patterns... ");
        for (int i = 1; i < 4; i++)
        {
            setPattern((AudioPattern)i);
            audioChannel.targetVolume = 0.3;
            delay(1000);
            stop();
            delay(200);
        }
        Serial.println("COMPLETE");

        digitalWrite(audioChannel.enablePin, LOW);

        if (testPassed)
        {
            Serial.println("Audio deterrent self-test PASSED");
        }
        else
        {
            Serial.println("Audio deterrent self-test FAILED");
        }

        return testPassed;
    }

    void AudioDeterrent::performAudioTest()
    {
        Serial.println("Performing audio initialization test...");

        // Brief test tone
        digitalWrite(audioChannel.enablePin, HIGH);

        for (int freq = 800; freq <= 1200; freq += 200)
        {
            for (int i = 0; i < 20; i++)
            {
                float sample = sin(2.0 * PI * freq * i / 20.0);
                int pwmValue = (int)((sample + 1.0) * 127.5);
                analogWrite(audioChannel.pwmPin, pwmValue);
                delay(10);
            }
        }

        analogWrite(audioChannel.pwmPin, 0);
        digitalWrite(audioChannel.enablePin, LOW);

        Serial.println("Audio test completed");
    }

    AudioMode AudioDeterrent::getCurrentMode()
    {
        return currentMode;
    }

    String AudioDeterrent::getModeString()
    {
        switch (currentMode)
        {
        case AUDIO_DISABLED:
            return "DISABLED";
        case AUDIO_STANDBY:
            return "STANDBY";
        case AUDIO_ACTIVE:
            return "ACTIVE";
        case AUDIO_EMERGENCY:
            return "EMERGENCY";
        default:
            return "UNKNOWN";
        }
    }

    float AudioDeterrent::getCurrentVolume()
    {
        return audioChannel.currentVolume;
    }

    float AudioDeterrent::getAmplifierTemperature()
    {
        return audioChannel.temperature;
    }

    bool AudioDeterrent::isVolumeLimited()
    {
        return volumeLimiting;
    }

    void AudioDeterrent::emergencyStop()
    {
        Serial.println("Audio Deterrent: EMERGENCY STOP");
        stop();
        systemEnabled = false;
        digitalWrite(audioChannel.enablePin, LOW);
        analogWrite(audioChannel.pwmPin, 0);
    }

    String AudioDeterrent::getStatusReport()
    {
        String report = "=== AUDIO DETERRENT STATUS ===\n";
        report += "Mode: " + getModeString() + "\n";
        report += "Pattern: " + patterns[currentPattern].name + "\n";
        report += "Volume: " + String(audioChannel.currentVolume * 100) + "%\n";
        report += "System Enabled: " + String(systemEnabled ? "YES" : "NO") + "\n";
        report += "Volume Limited: " + String(volumeLimiting ? "YES" : "NO") + "\n";
        report += "Amplifier Active: " + String(audioChannel.isActive ? "YES" : "NO") + "\n";
        report += "Amplifier Temp: " + String(audioChannel.temperature) + "°C\n";
        report += "Environment Noise: " + String(environmentNoise * 100) + "%\n";
        report += "Pattern Cycle: " + String(patternCycle) + "\n";
        report += "Frequency Index: " + String(currentFrequencyIndex) + "\n";
        report += "===============================\n";
        return report;
    }