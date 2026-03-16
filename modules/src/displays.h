#ifndef DISPLAYS_H
#define DISPLAYS_H

namespace pico_ssd1306 {
class SSD1306;
}

enum ScreenState {
    WELCOME,
    RECORDING,
    RESULT,
    SCORES
};

void showWelcomeScreen(pico_ssd1306::SSD1306 *display, int mode);
void showRecordScreen(pico_ssd1306::SSD1306 *display, float rY, float rP, float rR);
void showResultScreen(pico_ssd1306::SSD1306 *display, int mode, float total, float t, float r, float l, float a);
void showResetMsg(pico_ssd1306::SSD1306 *display);
void showScoresScreen(pico_ssd1306::SSD1306 *display, float pastScores[], int count);

#endif