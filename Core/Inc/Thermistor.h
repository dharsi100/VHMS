#ifndef __THERMISTOR_H
#define __THERMISTOR_H

#include "stm32f4xx.h"
#include "cmsis_os.h"
#include "usart.h"

void GPIO_Init_Power(void)
{
	RCC->AHB1ENR |= 1<<0; // Enable clock to GPIOA

    GPIOA->MODER &= ~((3 << (2 * 3)) | (3 << (2 * 4))); // Clear mode PA 3, PA 4
    GPIOA->MODER |=  ((1 << (2 * 3)) | (1 << (2 * 4))); // Set output mode

    GPIOA->ODR |= (1<<3);
}

// ===== ADC1: Thermistor (PA6, Channel 6) =====
void ADC1_Init_Thermistor()
{
    char str[20];
    float Temp;
    uint32_t result;

    // Enable Clocks
    RCC->AHB1ENR |= (1<<0);    // GPIOA
    RCC->APB2ENR |= (1<<8);    // ADC1

    // Configure PA6 as Analog
    GPIOA->MODER |= (3 << (2*6));

    // ADC1 Configuration
    ADC1->CR1 &= ~(3 << 24);   // 12-bit resolution
    ADC1->CR2 |= (1<<1);       // Continuous conversion
    ADC1->CR2 |= (1<<0);       // ADC ON

    ADC1->SQR1 &= ~(0xF << 20); // 1 conversion
    ADC1->SQR3 &= ~(0x1F << 0);
    ADC1->SQR3 |= 6;           // Channel 6

    ADC1->CR2 |= (1<<30);      // Start conversion


        while (!(ADC1->SR & (1<<1))); // Wait EOC
        result = ADC1->DR;

        Temp = 125 - (result / 22.7) - 66; // Thermistor formula
        sprintf(str, "T: %.2f ", Temp);
        lprint(0x80, str);
       

        for(int i=0;i<80000;i++); // Delay

}

float Thermistor_ReadTemperature(void) {
    // Replace with your ADC read + conversion
    return 25.0f; 
}

// ===== RTOS Task =====
float g_temperature = 0.0f;

void vTaskThermistor(void *pvParameters) {
    (void) pvParameters;
    for (;;) {
        g_temperature = Thermistor_ReadTemperature();
        // TODO: send g_temperature to queue or UART
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1s delay
    }
}

#endif



