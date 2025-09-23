
#define MPU6050_ADDR      0x68 << 1   // 7-bit address shifted for STM32
#define WHO_AM_I_REG      0x75
#define PWR_MGMT_1        0x6B
#define ACCEL_XOUT_H      0x3B
#define RAD_TO_DEG 57.2957795f

float roll_deg = 0, pitch_deg = 0;

void I2C1_Init(void);
void I2C1_Write(uint8_t addr, uint8_t reg, uint8_t data);
uint8_t I2C1_Read(uint8_t addr, uint8_t reg);
void I2C1_ReadMulti(uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t length);

int16_t Accel_X, Accel_Y, Accel_Z;

volatile uint32_t msTicks = 0;   // millisecond counter

// TIM2 interrupt handler
void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_UIF) {   // check update interrupt flag
        TIM2->SR &= ~TIM_SR_UIF;   // clear flag
        msTicks++;                 // increment every 1 ms
    }
}

uint32_t millis(void) {
    return msTicks;
}

void Timer2_Init(void) {
    // 1. Enable TIM2 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    // 2. Set prescaler & ARR for 1ms
    // TIM2 counter clock = APB1 * 2 (if prescaler != 1)
    // Assume APB1 = 42 MHz, so TIM2 clk = 84 MHz
    TIM2->PSC = 8400 - 1;    // divide 84 MHz by 8400 = 10 kHz
    TIM2->ARR = 10 - 1;      // count 10 ticks = 1 ms

    // 3. Enable update interrupt
    TIM2->DIER |= TIM_DIER_UIE;

    // 4. Enable counter
    TIM2->CR1 |= TIM_CR1_CEN;

    // 5. Enable TIM2 interrupt in NVIC
    NVIC_EnableIRQ(TIM2_IRQn);
}

void GPIO_Init(void)
{
    // Enable clock for GPIOA
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // Set PA15 as input (00)
    GPIOA->MODER &= ~(3UL << (15 * 2));

    // Enable pull-up (01)
    GPIOA->PUPDR &= ~(3UL << (15 * 2));
    GPIOA->PUPDR |= (1UL << (15 * 2));
}

void I2C1_Init(void)
{
    // Enable clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;  // Enable GPIOB clock
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;   // Enable I2C1 clock

    // --- Configure PB6 = SCL ---
    GPIOB->MODER &= ~(3 << (6*2));       // Clear mode
    GPIOB->MODER |=  (2 << (6*2));       // Alternate function mode
    GPIOB->OTYPER |= (1 << 6);           // Open-drain
    GPIOB->OSPEEDR |= (3 << (6*2));      // High speed
    GPIOB->PUPDR &= ~(3 << (6*2));       // Clear pull
    GPIOB->PUPDR |=  (1 << (6*2));       // Pull-up
    GPIOB->AFR[0] &= ~(0xF << (6*4));    // Clear AF
    GPIOB->AFR[0] |=  (4 << (6*4));      // AF4 (I2C1_SCL)

    // --- Configure PB9 = SDA ---
    GPIOB->MODER &= ~(3 << (9*2));       // Clear mode
    GPIOB->MODER |=  (2 << (9*2));       // Alternate function mode
    GPIOB->OTYPER |= (1 << 9);           // Open-drain
    GPIOB->OSPEEDR |= (3 << (9*2));      // High speed
    GPIOB->PUPDR &= ~(3 << (9*2));       // Clear pull
    GPIOB->PUPDR |=  (1 << (9*2));       // Pull-up
    GPIOB->AFR[1] &= ~(0xF << ((9-8)*4)); // Clear AF (AFR[1] because PB9 > 7)
    GPIOB->AFR[1] |=  (4 << ((9-8)*4));   // AF4 (I2C1_SDA)

    // Reset I2C1
    I2C1->CR1 |= I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    // Configure I2C1 timing for 100kHz (Standard Mode)
    I2C1->CR2 = 42;       // PCLK1 = 42 MHz
    I2C1->CCR = 210;      // 100kHz
    I2C1->TRISE = 43;     // Max rise time

    // Enable I2C1
    I2C1->CR1 |= I2C_CR1_PE;
}


void I2C1_Write(uint8_t addr, uint8_t reg, uint8_t data)
{
    // Start
    I2C1->CR1 |= I2C_CR1_START;
    while(!(I2C1->SR1 & I2C_SR1_SB));

    // Send address
    I2C1->DR = addr;
    while(!(I2C1->SR1 & I2C_SR1_ADDR));
    (void)I2C1->SR2;

    // Send register
    while(!(I2C1->SR1 & I2C_SR1_TXE));
    I2C1->DR = reg;
    while(!(I2C1->SR1 & I2C_SR1_TXE));

    // Send data
    I2C1->DR = data;
    while(!(I2C1->SR1 & I2C_SR1_BTF));

    // Stop
    I2C1->CR1 |= I2C_CR1_STOP;
}

uint8_t I2C1_Read(uint8_t addr, uint8_t reg)
{
    uint8_t data;

    // Send start
    I2C1->CR1 |= I2C_CR1_START;
    while(!(I2C1->SR1 & I2C_SR1_SB));

    // Send address (write mode)
    I2C1->DR = addr;
    while(!(I2C1->SR1 & I2C_SR1_ADDR));
    (void)I2C1->SR2;

    // Send register
    while(!(I2C1->SR1 & I2C_SR1_TXE));
    I2C1->DR = reg;
    while(!(I2C1->SR1 & I2C_SR1_TXE));

    // Restart
    I2C1->CR1 |= I2C_CR1_START;
    while(!(I2C1->SR1 & I2C_SR1_SB));

    // Send address (read mode)
    I2C1->DR = addr | 0x01;
    while(!(I2C1->SR1 & I2C_SR1_ADDR));
    I2C1->CR1 &= ~I2C_CR1_ACK;
    (void)I2C1->SR2;

    // Stop
    I2C1->CR1 |= I2C_CR1_STOP;

    // Read data
    while(!(I2C1->SR1 & I2C_SR1_RXNE));
    data = I2C1->DR;

    return data;
}

void I2C1_ReadMulti(uint8_t addr, uint8_t reg, uint8_t *buffer, uint8_t length)
{
    // Write register address
    I2C1->CR1 |= I2C_CR1_START;
    while(!(I2C1->SR1 & I2C_SR1_SB));
    I2C1->DR = addr;
    while(!(I2C1->SR1 & I2C_SR1_ADDR));
    (void)I2C1->SR2;

    while(!(I2C1->SR1 & I2C_SR1_TXE));
    I2C1->DR = reg;
    while(!(I2C1->SR1 & I2C_SR1_TXE));

    // Restart for read
    I2C1->CR1 |= I2C_CR1_START;
    while(!(I2C1->SR1 & I2C_SR1_SB));
    I2C1->DR = addr | 0x01;
    while(!(I2C1->SR1 & I2C_SR1_ADDR));
    (void)I2C1->SR2;

    for(uint8_t i = 0; i < length; i++)
    {
        if(i == (length-1))
        {
            I2C1->CR1 &= ~I2C_CR1_ACK;
            I2C1->CR1 |= I2C_CR1_STOP;
        }
        else
        {
            I2C1->CR1 |= I2C_CR1_ACK;
        }
        while(!(I2C1->SR1 & I2C_SR1_RXNE));
        buffer[i] = I2C1->DR;
    }
}

void Tilt()
{
	char str[20];
	 int step_count = 0;
	 float threshold = 0.1;  // tune this value
     GPIO_Init();
	 SystemCoreClockUpdate();
	 Timer2_Init();
	 I2C1_Init();

	 // Wake up MPU6050
	 I2C1_Write(MPU6050_ADDR, PWR_MGMT_1, 0x00);

	 // Verify connection
	 uint8_t check = I2C1_Read(MPU6050_ADDR, WHO_AM_I_REG);

	 if(check == 0x70)
	 {
	    uint8_t Rec_Data[6];
        I2C1_ReadMulti(MPU6050_ADDR, ACCEL_XOUT_H, Rec_Data, 6);
        Accel_X = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]);
        Accel_Y = (int16_t)(Rec_Data[2] << 8 | Rec_Data[3]);
        Accel_Z = (int16_t)(Rec_Data[4] << 8 | Rec_Data[5]);

        // Example conversion
	    float Ax = Accel_X / 16384.0;  // g
        float Ay = Accel_Y / 16384.0;
        float Az = Accel_Z / 16384.0;

        // Roll in degrees
	    float roll = atan2f(Ax, sqrtf(Ay*Ay + Az*Az)) * 180.0f / M_PI;
	    sprintf(str, "D:%.2f \r\n", roll);
	    
	    aprint(0x89,roll);
	    for (volatile uint32_t d = 0; d < 1000000; d++); // small delay
}
}

