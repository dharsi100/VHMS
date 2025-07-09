void GPIO_Init_Therm(void)
{
	RCC->AHB1ENR |= 1<<0; // Enable clock to GPIOA

    GPIOA->MODER &= ~((3 << (2 * 3)) | (3 << (2 * 4))); // Clear mode PA 3, PA 4
    GPIOA->MODER |=  ((1 << (2 * 3)) | (1 << (2 * 4))); // Set output mode

    GPIOA->ODR |= (1<<3);
}

void GPIO_Init_MotorRelay(void)
{
	RCC->AHB1ENR |= 1<<2; // Enable clock to GPIOC

    GPIOC->MODER &= ~((3 << (2 * 10)) | (3 << (2 * 11))); // Clear mode PC 10, PC 11
    GPIOC->MODER |=  ((1 << (2 * 10)) | (1 << (2 * 11))); // Set output mode

    GPIOC->ODR |= (1<<10);
}

void ADC_IRQHandler(void)
{
	if (ADC1->SR & (1 << 1)) // Check EOC flag
	{
		uint32_t result = ADC1->DR; // Read result to clear EOC

		float Temp = 125 - (result / 22.7) - 61.4;
		char str[20];
		sprintf(str, "T:%.2f ", Temp);
		lprint(0x80, str); // Print on LCD

		if (Temp > 20)
		{
			GPIOC->ODR &=~ (1<<10); // Turn ON motor relay (active low)
		}
		else
		{
			GPIOC->ODR |= (1<<10); // Turn OFF motor relay
		}
	}
}


void ADCModuleInittemp() //PA6 ADC IN
{

	RCC->APB2ENR |= (1<<8); //Enables ADC1 clock
	GPIOA->MODER &= ~(3<<(2*2));
	GPIOA->MODER |= (3<<(2*6)); //Makes PA6 Analog mode

	ADC1->CR1 &= ~(3 << 24); //Makes ADC 12 bit resolution
	ADC1->CR2 |= (1<<1); //ADC1 Continuous Conversion mode
	ADC1->CR2 |= (1<<0); //ADC1 ON
	ADC1->SQR3 = 6; //Channel 6 is the 1st in sequence of conversion

	ADC1->CR2 |= (1<<30); //Start conversion of ADC1
	while (!(ADC1->SR & (1 << 1))); //Wait till EOC
	uint32_t result = ADC1->DR; //Result of Conversion is stored on result
// 0- 4092
	float Temp = 125- (result / 22.7 )-61.4;
	char str[20];
	if(Temp>23)
	{
		GPIOC->ODR &=~ (1<<10); //Turns off the motor relay
	}
	else
	{
		GPIOC->ODR |= (1<<10);
	}
	sprintf(str, "T:%.2f ", Temp);
	lprint(0x80,str);
	for(int i=0;i<80000;i++);
}
