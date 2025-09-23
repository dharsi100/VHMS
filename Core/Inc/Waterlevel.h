
// ===== ADC2: Water Level (PC0, Channel 10) =====
void ADC2_Init_WaterLevel()
{
    char str[20];
    float level;
    uint32_t result;

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


        while (!(ADC2->SR & (1<<1))); // Wait EOC
        result = ADC2->DR;

        level = (float)result / 4095.0f; // Normalize 0–1
        sprintf(str, "L: %.2f  ", level);
        
        aprint(0xC0, result);

        for(int i=0;i<80000;i++); // Delay

}
