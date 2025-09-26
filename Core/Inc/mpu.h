#ifndef __MPU_H
#define __MPU_H

#include "stm32f4xx.h"
#include "cmsis_os.h"
#include <math.h>
#include <stdio.h>

#define MPU6050_ADDR      (0x68 << 1)   // 7-bit address shifted
#define WHO_AM_I_REG      0x75
#define PWR_MGMT_1        0x6B
#define ACCEL_XOUT_H      0x3B

#define RAD_TO_DEG 57.2957795f

// ===== Globals =====
int16_t g_ax = 0, g_ay = 0, g_az = 0;
float g_roll = 0.0f, g_pitch = 0.0f;

// ===== Low-level drivers =====
void I2C1_Init(void);
void I2C1_Write(uint8_t addr, uint8_t reg, uint8_t data);
uint8_t I2C1_Read(uint8_t addr, uint8_t reg);
void I2C1_ReadMulti(uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t length);

void I2C1_Init(void)
{
    // Enable clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;  
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;   

    // PB6 = SCL, PB9 = SDA
    // Configure as AF, Open-drain, Pull-up
    GPIOB->MODER &= ~((3<<(6*2)) | (3<<(9*2)));
    GPIOB->MODER |=  (2<<(6*2)) | (2<<(9*2));
    GPIOB->OTYPER |= (1<<6) | (1<<9);
    GPIOB->OSPEEDR |= (3<<(6*2)) | (3<<(9*2));
    GPIOB->PUPDR &= ~((3<<(6*2)) | (3<<(9*2)));
    GPIOB->PUPDR |=  (1<<(6*2)) | (1<<(9*2));
    GPIOB->AFR[0] |=  (4<<(6*4));
    GPIOB->AFR[1] |=  (4<<((9-8)*4));

    // Reset I2C1
    I2C1->CR1 |= I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    I2C1->CR2 = 42;       
    I2C1->CCR = 210;      
    I2C1->TRISE = 43;     

    I2C1->CR1 |= I2C_CR1_PE;
}

void I2C1_Write(uint8_t addr, uint8_t reg, uint8_t data)
{
    I2C1->CR1 |= I2C_CR1_START;
    while(!(I2C1->SR1 & I2C_SR1_SB));

    I2C1->DR = addr;
    while(!(I2C1->SR1 & I2C_SR1_ADDR)); (void)I2C1->SR2;

    while(!(I2C1->SR1 & I2C_SR1_TXE)); I2C1->DR = reg;
    while(!(I2C1->SR1 & I2C_SR1_TXE)); I2C1->DR = data;

    while(!(I2C1->SR1 & I2C_SR1_BTF));
    I2C1->CR1 |= I2C_CR1_STOP;
}

uint8_t I2C1_Read(uint8_t addr, uint8_t reg)
{
    uint8_t data;

    I2C1->CR1 |= I2C_CR1_START;
    while(!(I2C1->SR1 & I2C_SR1_SB));
    I2C1->DR = addr;
    while(!(I2C1->SR1 & I2C_SR1_ADDR)); (void)I2C1->SR2;

    while(!(I2C1->SR1 & I2C_SR1_TXE)); I2C1->DR = reg;
    while(!(I2C1->SR1 & I2C_SR1_TXE));

    I2C1->CR1 |= I2C_CR1_START;
    while(!(I2C1->SR1 & I2C_SR1_SB));
    I2C1->DR = addr | 0x01;
    while(!(I2C1->SR1 & I2C_SR1_ADDR)); (void)I2C1->SR2;
    I2C1->CR1 &= ~I2C_CR1_ACK; I2C1->CR1 |= I2C_CR1_STOP;

    while(!(I2C1->SR1 & I2C_SR1_RXNE)); data = I2C1->DR;
    return data;
}

void I2C1_ReadMulti(uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t length)
{
    I2C1->CR1 |= I2C_CR1_START;
    while(!(I2C1->SR1 & I2C_SR1_SB));
    I2C1->DR = addr;
    while(!(I2C1->SR1 & I2C_SR1_ADDR)); (void)I2C1->SR2;
    while(!(I2C1->SR1 & I2C_SR1_TXE)); I2C1->DR = reg;

    while(!(I2C1->SR1 & I2C_SR1_TXE));
    I2C1->CR1 |= I2C_CR1_START;
    while(!(I2C1->SR1 & I2C_SR1_SB));
    I2C1->DR = addr | 0x01;
    while(!(I2C1->SR1 & I2C_SR1_ADDR)); (void)I2C1->SR2;

    for(uint8_t i=0;i<length;i++) {
        if(i == (length-1)) {
            I2C1->CR1 &= ~I2C_CR1_ACK;
            I2C1->CR1 |= I2C_CR1_STOP;
        } else {
            I2C1->CR1 |= I2C_CR1_ACK;
        }
        while(!(I2C1->SR1 & I2C_SR1_RXNE));
        buffer[i] = I2C1->DR;
    }
}

// ===== MPU Driver =====
void MPU_Init(void) {
    I2C1_Init();
    I2C1_Write(MPU6050_ADDR, PWR_MGMT_1, 0x00); // wake up
}

void MPU_ReadRaw(int16_t *ax, int16_t *ay, int16_t *az) {
    uint8_t Rec_Data[6];
    I2C1_ReadMulti(MPU6050_ADDR, ACCEL_XOUT_H, Rec_Data, 6);
    *ax = (int16_t)(Rec_Data[0]<<8 | Rec_Data[1]);
    *ay = (int16_t)(Rec_Data[2]<<8 | Rec_Data[3]);
    *az = (int16_t)(Rec_Data[4]<<8 | Rec_Data[5]);
}

// ===== RTOS Task =====
void vTaskMPU(void *pvParameters) {
    (void) pvParameters;
    MPU_Init();
    for (;;) {
        MPU_ReadRaw(&g_ax, &g_ay, &g_az);

        float Ax = g_ax / 16384.0f;
        float Ay = g_ay / 16384.0f;
        float Az = g_az / 16384.0f;

        g_roll  = atan2f(Ay, Az) * RAD_TO_DEG;
        g_pitch = atan2f(-Ax, sqrtf(Ay*Ay + Az*Az)) * RAD_TO_DEG;

        // TODO: Send to LCD, UART, or queue
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

#endif

