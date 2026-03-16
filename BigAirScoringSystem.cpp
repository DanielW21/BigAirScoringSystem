#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "bno08x.h"
#include "displays.h"
#include "calculations.h"
#include "textRenderer/TextRenderer.h"

// --- CONFIGURATION ---
#define I2C_PORT i2c0

#define SDA_PIN 20
#define SCL_PIN 21

#define BUTTON_WHITE 3   
#define BUTTON_RED   7   
#define BUTTON_BLUE  11  
#define BUTTON_GREEN 14

#define INTERRUPT_PIN 13 
#define ALARM_NUM 0  // "Timer0"

using namespace pico_ssd1306;

// --- GLOBAL VOLATILE VARIABLES ---
volatile uint32_t my_tick_count = 0;
volatile bool event_triggered = false;
volatile int mode_val = 0;   
volatile int record_val = 0; 
volatile int last_btn_id = 0; 
volatile bool mode_locked = false;

// --- 1. MANUAL TIMER & INTERRUPT LOGIC ---

void alarm_irq_handler() {
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
    my_tick_count++;
    timer_hw->alarm[ALARM_NUM] += 1000;
}

uint32_t my_millis() {
    return my_tick_count;
}

void my_delay_ms(uint32_t ms) {
    uint32_t start = my_millis();
    while (my_millis() - start < ms) {
        tight_loop_contents(); // Low Power Wait
    }
}

void initManualTimer() {
    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
    irq_set_exclusive_handler(TIMER_IRQ_0, alarm_irq_handler);
    irq_set_enabled(TIMER_IRQ_0, true);
    timer_hw->alarm[ALARM_NUM] = timer_hw->timerawl + 1000;
}

// --- 2. GPIO STATE-BASED ISR ---
void gpio_callback(uint gpio, uint32_t events) {
    bool current_state = gpio_get(INTERRUPT_PIN);
    static uint32_t last_edge_time = 0;
    static bool last_state = false;
    static int pending_btn_id = 0; 
    
    uint32_t now = my_millis();
    uint32_t delta = now - last_edge_time;

    if (current_state && !last_state) {
        // RISING EDGE: Capture WHICH button is currently high
        if (gpio_get(BUTTON_WHITE))      pending_btn_id = BUTTON_WHITE;
        else if (gpio_get(BUTTON_BLUE))  pending_btn_id = BUTTON_BLUE;
        else if (gpio_get(BUTTON_RED))   pending_btn_id = BUTTON_RED;
        else if (gpio_get(BUTTON_GREEN)) pending_btn_id = BUTTON_GREEN;
        else                             pending_btn_id = 0;

        last_edge_time = now;
        last_state = true;
    } 
    else if (!current_state && last_state) {
        // FALLING EDGE: Verify the duration
        last_state = false;
        last_edge_time = now;

        if (delta >= 50 && pending_btn_id != 0) {
            printf("   [✓] Valid %d ms press confirmed for Pin %d\n", delta, pending_btn_id);
            
            last_btn_id = pending_btn_id; // Pass the remembered ID to the main loop
            
            // Apply logic based on the remembered button
            if (last_btn_id == BUTTON_WHITE) {
                mode_val = 0; record_val = 0; mode_locked = false;
            } 
            else if (last_btn_id == BUTTON_BLUE) {
                if (record_val == 0 && !mode_locked) {
                    mode_val = (mode_val + 1) % 4;
                }
            }
            else if (last_btn_id == BUTTON_RED) {
                record_val ^= 1;
                if (record_val == 1) mode_locked = true;
            }
            
            event_triggered = true; // Tell main loop to update the screen
        } 
        else if (delta < 50) {
            printf("   [!] Noise Filtered: %u ms\n", delta);
        }
        
        pending_btn_id = 0; // Reset for next press
    }
}
// --- 3. HARDWARE INITIALIZATION ---

void initHardwareBase() {
    stdio_init_all();
    initManualTimer();
    my_delay_ms(2000);
    printf("\n--- SYSTEM START ---\n");
    printf("Timer0 Interrupt Active.\n");

    i2c_init(I2C_PORT, 100 * 1000); // 100kHz for stability
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN); 
    gpio_pull_up(SCL_PIN);
    printf("I2C Ready.\n");


    uint pins[] = {BUTTON_WHITE, BUTTON_RED, BUTTON_BLUE, BUTTON_GREEN, INTERRUPT_PIN};

    for (int i = 0; i < 5; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_pull_down(pins[i]); 
    }
    
    gpio_set_irq_enabled_with_callback(INTERRUPT_PIN, 
                                       GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 
                                       true, &gpio_callback);
}

void initIMUAndDisplay(SSD1306 &display, BNO08x &IMU) {
    display.clear();
    drawText(&display, font_8x8, "INIT IMU...", 20, 28);
    display.sendBuffer();

    printf("Polling IMU at 0x4B...\n");
    while (!IMU.begin(0x4B, I2C_PORT)) {
        printf("IMU NOT READY - check wires\n");
        display.clear();
        drawText(&display, font_8x8, "IMU NOT READY", 10, 20);
        drawText(&display, font_5x8, "Check wiring/power", 8, 36);
        display.sendBuffer();
        my_delay_ms(1000);
    }
    IMU.enableRotationVector();
    printf("IMU Ready and Enabled.\n");
}

// --- 4. MAIN LOOP ---

int main() {
    initHardwareBase();
    float scoreHistory[50];
    int scoresStored = 0;
    SSD1306 display(I2C_PORT, 0x3C, Size::W128xH64);
    BNO08x IMU;
    initIMUAndDisplay(display, IMU);

    ScreenState currentScreen = WELCOME;
    RelativeTracker tracker = {0, 0, 0, 0, 0, 0, true};
    int last_record_val = 0;
    
    uint32_t last_display_time = 0;
    uint32_t jump_start_time = 0;
    float final_score = 0;
    float t_pts = 0, r_pts = 0, l_pts = 0, a_pts = 0;

    printf("Entering Concurrent Loop...\n");

    while (true) {
        uint32_t now = my_millis();

        // 
        
        // EVENT 2: Constant IMU Polling (Independent of Display)
        while (IMU.getSensorEvent()) {
            if (IMU.getSensorEventID() == SENSOR_REPORTID_ROTATION_VECTOR) {
                float y = IMU.getYaw() * 57.3f;
                float p = IMU.getPitch() * 57.3f;
                float r = IMU.getRoll() * 57.3f;
                float qw = IMU.getQuatReal();
                float qx = IMU.getQuatI();
                float qy = IMU.getQuatJ();
                float qz = IMU.getQuatK();
                if (currentScreen == RECORDING) {
                    updateRelativeTracking(tracker, y, p, r, qw, qx, qy, qz);
                }
            }
        }

        // EVENT 3: Logic & UI Update (Every 200ms)
        if (now - last_display_time >= 200) {
            last_display_time = now;

            if (event_triggered) {
                // Reset Function
                if (last_btn_id == BUTTON_WHITE) {
                    currentScreen = WELCOME;
                    scoresStored = 0; 
                    for(int i=0; i<50; i++) scoreHistory[i] = 0.0f;
                    tracker = {0,0,0,0,0,0, true};
                    record_val = 0;
                    mode_locked = false;
                    showResetMsg(&display);
                    my_delay_ms(500);
                }

                // Screen-Specific Context
                else if (currentScreen == WELCOME && last_btn_id == BUTTON_GREEN) {
                    currentScreen = SCORES;
                }
                else if (currentScreen == RECORDING) {
                    if (last_btn_id == BUTTON_RED) record_val = 0;
                }
                else if (currentScreen == SCORES || currentScreen == RESULT) {
                    currentScreen = WELCOME;
                    mode_locked = false;
                }

                // Record Transitions (Start/Stop recording)
                if (record_val == 1 && last_record_val == 0) { 
                    currentScreen = RECORDING;
                    tracker = {0,0,0,0,0,0, true}; 
                    jump_start_time = now;
                } 
                else if (record_val == 0 && last_record_val == 1) { 
                    currentScreen = RESULT;
                    float airtime_sec = (now - jump_start_time) / 1000.0f;
                    final_score = calculateScore(mode_val, tracker, airtime_sec, t_pts, r_pts, l_pts, a_pts);
                    if (scoresStored < 50) scoreHistory[scoresStored++] = final_score;
                }
                last_record_val = record_val;
                event_triggered = false; 
            }

            switch(currentScreen) {
                case WELCOME:   showWelcomeScreen(&display, mode_val); break;
                case RECORDING: showRecordScreen(&display, tracker.totalYaw, tracker.totalPitch, tracker.totalRoll); break;
                case RESULT:    showResultScreen(&display, mode_val, final_score, t_pts, r_pts, l_pts, a_pts); break;
                case SCORES:    showScoresScreen(&display, scoreHistory, scoresStored); break;
            }
        }
    }
}
