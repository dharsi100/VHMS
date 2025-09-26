#ifndef __THERMISTOR_H
#define __THERMISTOR_H

#include "stm32f4xx.h"
#include "cmsis_os.h"
#include <stdio.h>

// ===== Globals =====
float g_temperature = 0.0f;

// ===== Power GPIO (if used to power sensor) =====
void GPIO_Init_Power(void)
{
    RCC->AHB1ENR |= (1<<0); // Enable GPIOA

    GPIOA->MODER &= ~((3 << (2*3)) | (3 << (2*4))); 
    GPIOA->MODER |=  ((1 << (2*3)) | (1 << (2*4))); // PA3, PA4 as output

    GPIOA->ODR |= (1<<3); // turn on PA3 (power pin if required)
}

// ===== ADC1: Thermistor on PA6 (Channel 6) =====
void ADC1_Init_Thermistor(void)
{
    // Enable clocks
    RCC->AHB1ENR |= (1<<0);    // GPIOA
    RCC->APB2ENR |= (1<<8);    // ADC1

    // Configure PA6 as Analog
    GPIOA->MODER |= (3 << (2*6));

    // ADC1 config
    ADC1->CR1 &= ~(3 << 24);   // 12-bit
    ADC1->CR2 |= (1<<1);       // Continuous mode
    ADC1->CR2 |= (1<<0);       // ADC ON

    ADC1->SQR1 &= ~(0xF << 20); 
    ADC1->SQR3 &= ~(0x1F << 0);
    ADC1->SQR3 |= 6;           // Channel 6

    ADC1->CR2 |= (1<<30);      // Start conversion
}

// ===== Single Read + Convert to Temperature =====
float Thermistor_ReadTemperature(void)
{
    uint32_t result;
    float Temp;

    while (!(ADC1->SR & (1<<1))); // wait for EOC
    result = ADC1->DR;

    // Example formula (adjust to your thermistor)
    Temp = 125.0f - (result / 22.7f) - 66.0f;
    return Temp;
}

// ===== RTOS Task =====
void vTaskThermistor(void *pvParameters)
{
    (void) pvParameters;
    ADC1_Init_Thermistor();

    for (;;)
    {
        g_temperature = Thermistor_ReadTemperature();

        // TODO: send g_temperature to LCD / UART / queue
        vTaskDelay(pdMS_TO_TICKS(1000)); // sample every 1s
    }
}

#endif
