
// ===== ADC3: Battery Voltage Monitoring (PA3, Channel 3) =====
void ADC3_Init_Battery()
{
    char str[20];
    float batteryVoltage;
    uint32_t result;

    // ===== Enable Clocks =====
    RCC->AHB1ENR |= (1<<0);    // GPIOA
    RCC->APB2ENR |= (1<<10);   // ADC3

    // ===== Configure PA3 as Analog =====
    GPIOA->MODER |= (3 << (2*3)); // PA3 analog

    // ===== ADC3 Configuration =====
    ADC3->CR1 &= ~(3 << 24);   // 12-bit resolution
    ADC3->CR2 |= (1<<1);       // Continuous conversion
    ADC3->CR2 |= (1<<0);       // ADC ON

    ADC3->SQR1 &= ~(0xF << 20); // 1 conversion
    ADC3->SQR3 &= ~(0x1F << 0);
    ADC3->SQR3 |= 3;           // Channel 3

    ADC3->CR2 |= (1<<30);      // Start conversion

 while (!(ADC3->SR & (1<<1))); // Wait EOC
        result = ADC3->DR;

        // Assuming voltage divider to scale battery to ADC range
        batteryVoltage = ((float)result / 4095.0f) * 12;
        // Multiply by factor if voltage divider halves the battery voltage

        sprintf(str, "V:%.2f ", batteryVoltage+1.1 );
        
        lprint(0xC6,str );

    
        for(int i=0;i<80000;i++); // Delay

}
