void GPIO_Init_Vib(void)
{
	RCC->AHB1ENR |= 1<<0; // Enable clock to GPIOA

    GPIOA->MODER &= ~((3 << (2 * 3)) | (3 << (2 * 4))); // Clear mode PA 3, PA 4
    GPIOA->MODER |=  ((1 << (2 * 3)) | (1 << (2 * 4))); // Set output mode

    GPIOA->ODR |= (1<<3);
}
void GPIO_Motordriver(void)
{
	RCC->AHB1ENR |= 1<<1; // Enable clock to GPIOB

    GPIOB->MODER &= ~((3 << (2 * 8)) | (3 << (2 * 9))); // Clear mode PB 8, PB 9
    GPIOB->MODER |=  ((1 << (2 * 8)) | (1 << (2 * 9))); // Set output mode

    GPIOB->ODR |= (1<<8);
}

void pwm_enable(uint32_t val2)
{
TIM3->CR1 &= ~(1<<0); //timer disable//
TIM3->CCR1 = val2; //Duty Cycle at 10%
TIM3->CR1 |= (1<<0); //timer enable//
}

void pwm_gpio_init()
{
	RCC->AHB1ENR |= (1<<2); /*Enable GPIOC Clock*/
	RCC->APB1ENR |= (1<<1); /*Enable Timer 1 Clock*/
	GPIOC->MODER |= (2<<12);
	GPIOC->AFR[0] |= (2<<24);

	TIM3->PSC = 0; /*Timer 1 Prescaler as 0*/
	TIM3->ARR=1600-1; /*Value for 10KHz PWM Frequency*/
	TIM3->CCMR1 |= (6<<4);
	TIM3->CCMR1 |= (1<<3);
	TIM3->CCER |= (1<<0); /*Compare enable*/
	TIM3->CR1 |= (1<<0); /*Auto Reload Preload enable*/
}

void ADCModuleInitVib() //Pc3 ADC IN
{
	RCC->AHB1ENR |= 1<<2; //Enable GPIOC

	RCC->APB2ENR |= (1<<9); //Enables ADC2 clock
	GPIOC->MODER &= ~(3<<(2*3));
	GPIOC->MODER |= (3<<(2*3)); //Makes PC3 Analog mode

	ADC2->CR1 &= ~(3 << 24); //Makes ADC 12 bit resolution
	ADC2->CR2 |= (1<<1); //ADC2 Continuous Conversion mode
	ADC2->CR2 |= (1<<0); //ADC2 ON
	ADC2->SQR3 = 13; //Channel 13 is the 1st in sequence of conversion

	ADC2->CR2 |= (1<<30); //Start conversion of ADC2
	while (!(ADC2->SR & (1 << 1))); //Wait till EOC
	uint32_t result = ADC2->DR; //Result of Conversion is stored on result

	for(int i=0;i<100000;i++);
	if (result < 3000)
    {
	   pwm_enable(300);
	   lprint(0xC0, "Vib:Yes");
    }

	else
	{
	    pwm_enable(1400);
	    lprint(0xC0, "Vib:No ");
    }

	for (volatile int i = 0; i < 100000; i++);  // delay to see result
}
