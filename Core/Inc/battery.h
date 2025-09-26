#ifndef __BATTERY_H
#define __BATTERY_H

#include "stm32f4xx.h"
#include "cmsis_os.h"
#include <stdio.h>

// ===== Globals =====
float g_batteryVoltage = 0.0f;
uint16_t g_batteryRaw = 0;

// ===== ADC3: Battery Voltage Monitoring (PA3, Channel 3) =====
void ADC3_Init_Battery(void) {
    // Enable Clocks
    RCC->AHB1ENR |= (1<<0);    // GPIOA
    RCC->APB2ENR |= (1<<10);   // ADC3

    // Configure PA3 as Analog
    GPIOA->MODER |= (3 << (2*3)); // PA3 analog

    // ADC3 Configuration
    ADC3->CR1 &= ~(3 << 24);   // 12-bit resolution
    ADC3->CR2 |= (1<<1);       // Continuous conversion
    ADC3->CR2 |= (1<<0);       // ADC ON

    ADC3->SQR1 &= ~(0xF << 20); // 1 conversion
    ADC3->SQR3 &= ~(0x1F << 0);
    ADC3->SQR3 |= 3;           // Channel 3

    ADC3->CR2 |= (1<<30);      // Start conversion
}

// ===== Battery monitor driver =====
float Battery_ReadVoltage(void) {
    uint32_t result;

    while (!(ADC3->SR & (1<<1))); // Wait for EOC
    result = ADC3->DR;

    g_batteryRaw = (uint16_t)result;

    // Assuming resistor divider maps battery max (e.g. 12V) to 3.3V ADC
    g_batteryVoltage = ((float)result / 4095.0f) * 12.0f;

    return g_batteryVoltage;
}

// ===== RTOS Task =====
void vTaskBattery(void *pvParameters) {
    (void) pvParameters;

    ADC3_Init_Battery(); // init once

    for (;;) {
        float vbat = Battery_ReadVoltage();

        // Example: print via UART/LCD
        // char str[20];
        // sprintf(str, "V:%.2f ", vbat);
        // lprint(0xC6, str);

        vTaskDelay(pdMS_TO_TICKS(2000)); // check every 2s
    }
}

#endif

