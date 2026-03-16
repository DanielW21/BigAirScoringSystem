#ifndef CALCULATIONS_H
#define CALCULATIONS_H

#include <cstdint>

struct RelativeTracker {
    float totalYaw = 0;
    float totalPitch = 0;
    float totalRoll = 0;
    float lastYaw = 0;
    float lastPitch = 0;
    float lastRoll = 0;
    float lastQuatW = 1.0f;
    float lastQuatX = 0.0f;
    float lastQuatY = 0.0f;
    float lastQuatZ = 0.0f;
    bool isFirstFrame = true;
};

// Updated signature to allow breakdown of scores for the display/debugging
float calculateScore(int mode, RelativeTracker tracker, float airtime, 
                     float &outTrick, float &outRot, float &outLand, float &outAir);

void updateRelativeTracking(RelativeTracker &tracker,
                           float currentY,
                           float currentP,
                           float currentR,
                           float quatW,
                           float quatX,
                           float quatY,
                           float quatZ);

#endif