
#ifndef AUDIO_DETERRENT_H
#define AUDIO_DETERRENT_H

#include <Arduino.h>

#define MAX_AUDIO_PATTERNS 8
#define MAX_FREQUENCY_SWEEP 5
#define AUDIO_BUFFER_SIZE 256
#define MAX_VOLUME_DB 85
#define ULTRASONIC_BASE_FREQ 17000
#define AUDIBLE_BASE_FREQ 1000
#define PATTERN_ROTATION_TIME 30000

enum AudioPattern
{
  AUDIO_OFF = 0,
  CROW_DISTRESS = 1,
  EAGLE_DISTRESS = 2,
  HAWK_SCREECH = 3,
  GENERAL_ALARM = 4,
  ULTRASONIC_SWEEP = 5,
  PREDATOR_GROWL = 6,
  EMERGENCY_SIREN = 7
};

enum AudioMode
{
  AUDIO_DISABLED = 0,
  AUDIO_STANDBY = 1,
  AUDIO_ACTIVE = 2,
  AUDIO_EMERGENCY = 3
};

struct FrequencyComponent
{
  float frequency;
  float amplitude;
  float phase;
  unsigned long duration;
};

struct AudioPatternConfig
{
  AudioPattern patternId;
  String name;
  FrequencyComponent frequencies[MAX_FREQUENCY_SWEEP];
  int frequencyCount;
  int repeatCount;
  int pauseDuration;
  float baseVolume;
  bool isUltrasonic;
};

struct AudioChannel
{
  int pwmPin;
  int enablePin;
  float currentVolume;
  float targetVolume;
  bool isActive;
  unsigned long lastUpdate;
  float temperature;
};

class AudioDeterrent
{
private:
  AudioChannel audioChannel;
  AudioMode currentMode;
  AudioPattern currentPattern;
  AudioPatternConfig patterns[MAX_AUDIO_PATTERNS];
  unsigned long patternStartTime;
  int patternCycle;
  int currentFrequencyIndex;
  bool systemEnabled;
  float environmentNoise;
  bool volumeLimiting;
  unsigned long lastPatternRotation;
  int patternRotationIndex;

  float audioBuffer[AUDIO_BUFFER_SIZE];
  int bufferIndex;
  unsigned long sampleRate;

  void initializeAudioPatterns();
  void generateAudioWaveform();
  void updateAudioOutput();
  void synthesizeFrequency(float frequency, float amplitude, float duration);
  void synthesizeDistressCall(AudioPattern pattern);
  void synthesizeUltrasonicSweep();
  void synthesizePredatorSound();
  float readEnvironmentNoise();
  void applyVolumeControl();
  void rotatePatterns();
  bool isVolumeWithinLimits();
  void performAudioTest();

public:
  AudioDeterrent();
  bool begin(int pwmPin, int enablePin);
  void update();
  void playDistressCalls();
  void playEmergencySignals();
  void playUltrasonicDeterrent();
  void stop();
  void setVolume(float volume);
  void setPattern(AudioPattern pattern);
  void setEnabled(bool enabled);
  bool isEnabled();
  bool selfTest();
  AudioMode getCurrentMode();
  String getModeString();
  float getCurrentVolume();
  float getAmplifierTemperature();
  bool isVolumeLimited();
  String getStatusReport();
  void emergencyStop();