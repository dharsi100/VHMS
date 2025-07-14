#define TRIG_PIN 15   // PA6
#define ECHO_PIN 7   // PA7

#define TIMEOUT_US 25000  // 25 milliseconds timeout
#define DIST_THRESHOLD 4

void GPIO_Init_Buzzer(void)
{
	RCC->AHB1ENR |= 1<<2; //Enable GPIOC
	GPIOC->MODER &= ~((3 << (2 * 9))); // Clear mode PC9
	GPIOC->MODER |=  ((1 << (2 * 9))); // Set output mode
}

void delay_us(uint32_t us)
{
    TIM2->CNT = 0;
    while (TIM2->CNT < us);
}

void TIM2_us_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = (SystemCoreClock / 1000000) - 1;  // 1 MHz timer = 1 µs per count
    TIM2->ARR = 0xFFFF;
    TIM2->CR1 |= TIM_CR1_CEN;
}

void delay_us_exc(uint32_t us)
{
    TIM4->CNT = 0;
    while (TIM4->CNT < us);
}

void TIM4_us_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    TIM4->PSC = (SystemCoreClock / 1000000) - 1;  // 1 MHz timer = 1 µs per count
    TIM4->ARR = 0xFFFF;
    TIM4->CR1 |= TIM_CR1_CEN;
}

void GPIO_Init_Ultrasonic(void)
{
    // TRIG = PA6 as output
    GPIOA->MODER &= ~(3 << (2 * TRIG_PIN));
    GPIOA->MODER |= (1 << (2 * TRIG_PIN)); // Output mode
    GPIOA->OTYPER &= ~(1 << TRIG_PIN);
    GPIOA->OSPEEDR |= (3 << (2 * TRIG_PIN));

    // ECHO = PA7 as input
    GPIOA->MODER &= ~(3 << (2 * ECHO_PIN));
}

void send_trigger(void)
{
    GPIOA->BSRR = (1 << TRIG_PIN);           // HIGH
    delay_us(10);                            // 10 µs
    GPIOA->BSRR = (1 << (TRIG_PIN + 16));    // LOW
}


uint32_t read_echo_time(void)
{
    TIM4->CNT = 0; // Start timeout timer

    while (!(GPIOA->IDR & (1 << ECHO_PIN)))  // Wait for ECHO pin to go HIGH (start of pulse)
    {
        if (TIM4->CNT > TIMEOUT_US) {
            return 0;  // Timeout occurred: no echo
        }
    }

    TIM2->CNT = 0;  // Start echo timer
    TIM4->CNT = 0;  // Reset timeout timer

    while ((GPIOA->IDR & (1 << ECHO_PIN))) // Wait for ECHO pin to go LOW (end of pulse)
    {
        if (TIM4->CNT > TIMEOUT_US) {
            return 0;  // Timeout: echo pulse too long
        }
    }

    return TIM2->CNT;  // Pulse width in microseconds
}

/*uint32_t display_distance(void)
{

}*/


