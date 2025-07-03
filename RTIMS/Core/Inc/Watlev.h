void GPIO_Init_WatLev(void)
{
	RCC->AHB1ENR |= 1<<0; // Enable clock to GPIOA
   // RCC->AHB1ENR |= 1<<1; // Enable clock to GPIOB
  //  RCC->AHB1ENR |= 1<<2; // Enable clock to GPIOC

    GPIOA->MODER &= ~((3 << (2 * 3)) | (3 << (2 * 4))); // Clear mode PA 3, PA 4
    GPIOA->MODER |=  ((1 << (2 * 3)) | (1 << (2 * 4))); // Set output mode

    GPIOA->ODR |= (1<<3);

}
void ADCModuleInitWatlev() //PC0 ADC IN
{
	RCC->APB2ENR |= (1<<10); //Enables ADC2 clock
	GPIOA->MODER &= ~(3<<(2*2));
	GPIOA->MODER |= (3<<(2*2)); //Makes PC3 Analog mode

	ADC3->CR1 &= ~(3 << 24); //Makes ADC 12 bit resolution
	ADC3->CR2 |= (1<<1); //ADC1 Continuous Conversion mode
	ADC3->CR2 |= (1<<0); //ADC1 ON
	ADC3->SQR3 = 2<<0; //Channel 2  is the 1st in sequence of conversion


	ADC3->CR2 |= (1<<30); //Start conversion of ADC1
	while (!(ADC3->SR & (1 << 1))); //Wait till EOC
	uint32_t result = ADC3->DR; //Result of Conversion is stored on result

	float waterlevel = (result -1600)/ 300;
	char str[20];
	sprintf(str, "Lev: %d", result);
	lprint(0xC0,str);
	//for(int i=0;i<80000;i++);
}
