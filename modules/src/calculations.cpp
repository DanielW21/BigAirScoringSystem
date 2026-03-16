#include "calculations.h"
#include <cmath>
#include <algorithm>

static void normalizeQuat(float &w, float &x, float &y, float &z) {
    const float n = std::sqrt(w * w + x * x + y * y + z * z);
    if (n <= 1e-6f) {
        w = 1.0f;
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
        return;
    }
    w /= n;
    x /= n;
    y /= n;
    z /= n;
}

void updateRelativeTracking(RelativeTracker &tracker,
                           float currentY,
                           float currentP,
                           float currentR,
                           float quatW,
                           float quatX,
                           float quatY,
                           float quatZ) {
    normalizeQuat(quatW, quatX, quatY, quatZ);

    if (tracker.isFirstFrame) {
        tracker.lastYaw = currentY;
        tracker.lastPitch = currentP;
        tracker.lastRoll = currentR;
        tracker.lastQuatW = quatW;
        tracker.lastQuatX = quatX;
        tracker.lastQuatY = quatY;
        tracker.lastQuatZ = quatZ;
        tracker.isFirstFrame = false;
        return;
    }

    // Delta quaternion in body frame: dq = conj(q_last) * q_current
    const float lw = tracker.lastQuatW;
    const float lx = tracker.lastQuatX;
    const float ly = tracker.lastQuatY;
    const float lz = tracker.lastQuatZ;

    float dw = lw * quatW + lx * quatX + ly * quatY + lz * quatZ;
    float dx = lw * quatX - lx * quatW - ly * quatZ + lz * quatY;
    float dy = lw * quatY + lx * quatZ - ly * quatW - lz * quatX;
    float dz = lw * quatZ - lx * quatY + ly * quatX - lz * quatW;

    normalizeQuat(dw, dx, dy, dz);

    // Keep shortest-path sign for stable incremental integration.
    if (dw < 0.0f) {
        dw = -dw;
        dx = -dx;
        dy = -dy;
        dz = -dz;
    }

    const float vnorm = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (vnorm > 1e-6f) {
        const float angle = 2.0f * std::atan2(vnorm, dw);
        const float degPerUnit = (180.0f / 3.14159265f) * (angle / vnorm);

        // Body-axis incremental angles from rotation vector components.
        tracker.totalRoll += dx * degPerUnit;
        tracker.totalPitch += dy * degPerUnit;
        tracker.totalYaw += dz * degPerUnit;
    }

    // Keep Euler snapshots for display/debug continuity.
    tracker.lastYaw = currentY;
    tracker.lastPitch = currentP;
    tracker.lastRoll = currentR;
    tracker.lastQuatW = quatW;
    tracker.lastQuatX = quatX;
    tracker.lastQuatY = quatY;
    tracker.lastQuatZ = quatZ;
}

float calculateScore(int mode, RelativeTracker tracker, float airtime, 
                     float &outTrick, float &outRot, float &outLand, float &outAir) {
    
    float primaryRotation = 0;

    // 1. Base points and Primary axis
    switch (mode) {
        case 0: // Flat Spin
            outTrick = 15; 
            primaryRotation = std::abs(tracker.totalYaw);   
            break;

        case 1: // Backflip
            outTrick = 20; 
            primaryRotation = std::abs(tracker.totalPitch); 
            break;

        case 2: // Frontflip
            outTrick = 25; 
            primaryRotation = std::abs(tracker.totalPitch); 
            break;

        case 3: // Side Flip
            outTrick = 30; 
            primaryRotation = std::abs(tracker.totalRoll);  
            break;

        default: 
            outTrick = 0; 
            primaryRotation = 0;
            break;
    }

    // 2. Determine landing "bucket" (multiples of 180)
    int closestMark = (int)((primaryRotation + 90.0f) / 180.0f) * 180;
    int diff = std::abs((int)primaryRotation - closestMark);
    
    // Valid landing is within 20 degrees of a 180n mark (min 180)
    bool landed = (diff <= 20 && closestMark >= 180);

    // Initialize scoring components
    outRot = 0;
    outLand = 0;
    outAir = (airtime > 20) ? 20.0f : (float)airtime;

    // 3. Final scoring logic
    if (landed) {
        // Calculate rotation bonus (10 for 360, +5 per 180 extra)
        if (closestMark >= 360) {
            outRot = 10.0f + ((closestMark - 360) / 180) * 5;
        }
        
        // Cap rotation bonus at 30
        if (outRot > 30.0f) {
            outRot = 30.0f;
        }

        // Score landing accuracy (0-20 pts)
        outLand = 20.0f - (float)diff;

        return outTrick + outRot + outLand + outAir;
    } 
    else {
        // Fall logic: only return the base points
        outRot = 0; 
        outLand = 0; 
        outAir = 0;
        return outTrick;
    }
}