#ifndef __WATERLEVEL_H
#define __WATERLEVEL_H

#include "stm32f4xx.h"
#include "cmsis_os.h"
#include <stdio.h>

// ===== Globals =====
uint16_t g_waterLevelRaw = 0;
float    g_waterLevelNorm = 0.0f;

// ===== ADC2 Init: Water Level (PC0, Channel 10) =====
void ADC2_Init_WaterLevel(void) {
    // Enable Clocks
    RCC->AHB1ENR |= (1<<2);    // GPIOC
    RCC->APB2ENR |= (1<<9);    // ADC2

    // Configure PC0 as Analog
    GPIOC->MODER |= (3 << (2*0));

    // ADC2 Configuration
    ADC2->CR1 &= ~(3 << 24);   // 12-bit resolution
    ADC2->CR2 |= (1<<1);       // Continuous conversion
    ADC2->CR2 |= (1<<0);       // ADC ON

    ADC2->SQR1 &= ~(0xF << 20); // 1 conversion
    ADC2->SQR3 &= ~(0x1F << 0);
    ADC2->SQR3 |= 10;          // Channel 10

    ADC2->CR2 |= (1<<30);      // Start conversion
}

// ===== Read water level =====
float WaterLevel_Read(void) {
    uint32_t result;

    while (!(ADC2->SR & (1<<1))); // Wait EOC
    result = ADC2->DR;

    g_waterLevelRaw = (uint16_t)result;
    g_waterLevelNorm = (float)result / 4095.0f; // 0–1 normalized

    return g_waterLevelNorm;
}

// ===== RTOS Task =====
void vTaskWaterlevel(void *pvParameters) {
    (void) pvParameters;

    ADC2_Init_WaterLevel();  // init once

    for (;;) {
        float level = WaterLevel_Read();

        // Example: print normalized value
        // aprint(0xC0, (uint16_t)(level*100)); 

        vTaskDelay(pdMS_TO_TICKS(1000)); // 1s update
    }
}

#endif
