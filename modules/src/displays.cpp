#include "displays.h"
#include "textRenderer/TextRenderer.h"
#include <stdio.h>
#include <vector>

using namespace pico_ssd1306;

void showWelcomeScreen(SSD1306 *display, int mode) {
    display->clear();
    drawText(display, font_8x8, "WELCOME", 35, 0);
    
    char modeBuf[16];
    snprintf(modeBuf, sizeof(modeBuf), "MODE: %d", mode);
    drawText(display, font_8x8, modeBuf, 35, 25);
    
    drawText(display, font_8x8, "Press RED to REC", 0, 50);
    display->sendBuffer();
}

void showRecordScreen(SSD1306 *display, float rY, float rP, float rR) {
    display->clear();
    drawText(display, font_8x8, "RECORDING...", 20, 0);
    
    char yBuf[32], pBuf[32], rBuf[32];
    snprintf(yBuf, sizeof(yBuf), "Rel Y: %.1f", rY);
    snprintf(pBuf, sizeof(pBuf), "Rel P: %.1f", rP);
    snprintf(rBuf, sizeof(rBuf), "Rel R: %.1f", rR);
    
    drawText(display, font_8x8, yBuf, 0, 20);
    drawText(display, font_8x8, pBuf, 0, 30);
    drawText(display, font_8x8, rBuf, 0, 40);
    
    drawText(display, font_8x8, "Press RED to STOP", 0, 56);
    display->sendBuffer();
}

void showResultScreen(SSD1306 *display, int mode, float total, float t, float r, float l, float a) {
    display->clear();
    
    // Header
    drawText(display, font_8x8, "RESULTS", 35, 0);
    
    // Trick Name (Small font to handle long names)
    const char* trickNames[] = {"FLAT SPIN", "BACKFLIP", "FRONTFLIP", "SIDE FLIP"};
    const char* currentTrick = (mode >= 0 && mode < 4) ? trickNames[mode] : "UNKNOWN";
    char nameBuf[32];
    snprintf(nameBuf, sizeof(nameBuf), "TRICK: %s", currentTrick);
    drawText(display, font_5x8, nameBuf, 0, 12);

    // Total Score (Big font - the main event)
    char totalBuf[32];
    snprintf(totalBuf, sizeof(totalBuf), "TOTAL: %.1f", total);
    drawText(display, font_8x8, totalBuf, 0, 22);

    // Sub-score Breakdown (Small font to fit all 4)
    char row1[64], row2[64];
    // Line 1: Trick and Rotation
    snprintf(row1, sizeof(row1), "Trk:%.0f  Rot:%.0f", t, r);
    // Line 2: Landing and Airtime
    snprintf(row2, sizeof(row2), "Lnd:%.1f Air:%.1f", l, a);

    drawText(display, font_5x8, row1, 0, 36);
    drawText(display, font_5x8, row2, 0, 46);

    // Footer
    drawText(display, font_5x8, "Press RST to EXIT", 15, 56);
    
    display->sendBuffer();
}

void showResetMsg(SSD1306 *display) {
    display->clear();
    drawText(display, font_5x8, "SYSTEM RESET...", 26, 28);
    display->sendBuffer();
}

void showScoresScreen(SSD1306 *display, float pastScores[], int count) {
    display->clear();
    drawText(display, font_8x8, "TOP 3 SCORES", 15, 0);

    if (count == 0) {
        drawText(display, font_5x8, "No scores yet!", 25, 30);
    } else {
        std::vector<float> temp(count);
        for (int i = 0; i < count; i++) temp[i] = pastScores[i];

        for (int i = 0; i < 3 && i < count; i++) {
            int maxIdx = i;
            for (int j = i + 1; j < count; j++) {
                if (temp[j] > temp[maxIdx]) maxIdx = j;
            }

            float val = temp[maxIdx];
            temp[maxIdx] = temp[i];
            temp[i] = val;

            char buf[32];
            snprintf(buf, sizeof(buf), "#%d: %.1f pts", i + 1, temp[i]);
            drawText(display, font_8x8, buf, 10, 20 + (i * 12));
        }
    }

    display->sendBuffer();
}