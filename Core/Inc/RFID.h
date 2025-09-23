void SPI1_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // PA5=SCK, PA6=MISO, PA7=MOSI (AF5)
    GPIOA->MODER &= ~((3<<(5*2))|(3<<(6*2))|(3<<(7*2)));
    GPIOA->MODER |=  (2<<(5*2))|(2<<(6*2))|(2<<(7*2));
    GPIOA->AFR[0] |= (5<<(5*4))|(5<<(6*4))|(5<<(7*4));

    // PA4=CS (manual GPIO)
    GPIOA->MODER &= ~(3<<(4*2));
    GPIOA->MODER |=  (1<<(4*2));
    GPIOA->BSRR = (1<<4); // set high (deselect)

    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_0; // master, mode0
    SPI1->CR1 |= SPI_CR1_SPE;
}

uint8_t SPI1_Transfer(uint8_t data) {
    while(!(SPI1->SR & SPI_SR_TXE));
    *((__IO uint8_t*)&SPI1->DR) = data;
    while(!(SPI1->SR & SPI_SR_RXNE));
    return *((__IO uint8_t*)&SPI1->DR);
}

// ===== MFRC522 helper =====
#define CS_LOW()   (GPIOA->BSRR = (1<<(4+16)))
#define CS_HIGH()  (GPIOA->BSRR = (1<<4))

uint8_t MFRC522_ReadReg(uint8_t reg) {
    CS_LOW();
    SPI1_Transfer(((reg<<1)&0x7E) | 0x80);
    uint8_t val = SPI1_Transfer(0x00);
    CS_HIGH();
    return val;
}

void MFRC522_WriteReg(uint8_t reg, uint8_t val) {
    CS_LOW();
    SPI1_Transfer((reg<<1)&0x7E);
    SPI1_Transfer(val);
    CS_HIGH();
}

// ===== MFRC522 registers =====
#define CommandReg     0x01
#define ComIEnReg      0x02
#define DivIrqReg      0x05
#define FIFODataReg    0x09
#define FIFOLevelReg   0x0A
#define ControlReg     0x0C
#define BitFramingReg  0x0D
#define ErrorReg       0x06
#define ModeReg        0x11
#define TxControlReg   0x14

#define PCD_IDLE       0x00
#define PCD_TRANSCEIVE 0x0C

#define PICC_REQIDL    0x26
#define PICC_ANTICOLL  0x93

// ===== Core functions =====
void MFRC522_AntennaOn(void) {
    uint8_t val = MFRC522_ReadReg(TxControlReg);
    if (!(val & 0x03)) {
        MFRC522_WriteReg(TxControlReg, val | 0x03);
    }
}

void MFRC522_Init(void) {
    // Reset FIFO
    MFRC522_WriteReg(CommandReg, PCD_IDLE);
    MFRC522_WriteReg(FIFOLevelReg, 0x80);

    // Enable antenna
    MFRC522_AntennaOn();
}

uint8_t MFRC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen,
                      uint8_t *backData, uint8_t *backLen)
{
    uint8_t n;
    uint8_t irqEn = 0x77;
    uint8_t waitIRq = 0x30; // RxIRq and IdleIRq
    uint32_t i;

    MFRC522_WriteReg(ComIEnReg, irqEn | 0x80);   // enable interrupts
    MFRC522_WriteReg(CommandReg, PCD_IDLE);

    // Flush FIFO
    MFRC522_WriteReg(FIFOLevelReg, 0x80);

    // Write data to FIFO
    for (uint8_t k = 0; k < sendLen; k++) {
        MFRC522_WriteReg(FIFODataReg, sendData[k]);
    }

    // Execute command
    MFRC522_WriteReg(CommandReg, command);
    if (command == PCD_TRANSCEIVE) {
        MFRC522_WriteReg(BitFramingReg, 0x80); // Start transmission
    }

    // Wait for completion or timeout
    i = 0xFFFF; // larger timeout
    do {
        n = MFRC522_ReadReg(DivIrqReg);
        i--;
    } while ((i != 0) && !(n & waitIRq));

    MFRC522_WriteReg(BitFramingReg, 0x00);

    if (i == 0) return 1; // timeout/error

    // Check error register
    if (MFRC522_ReadReg(ErrorReg) & 0x1B) return 1;

    // Read response if requested
    if (backData && backLen) {
        uint8_t size = MFRC522_ReadReg(FIFOLevelReg);
        uint8_t lastBits = MFRC522_ReadReg(ControlReg) & 0x07;
        if (lastBits) *backLen = (size - 1) * 8 + lastBits;
        else *backLen = size * 8;

        if (size > 16) size = 16; // FIFO limit
        for (uint8_t j = 0; j < size; j++) {
            backData[j] = MFRC522_ReadReg(FIFODataReg);
        }
    }

    return 0; // success
}
uint8_t MFRC522_Request(uint8_t reqMode, uint8_t *TagType)
{
    uint8_t backBits;
    uint8_t buf[1];
    uint8_t status;

    // REQA must be sent as 7 bits (TxLastBits = 7)
    uint8_t originalBitFraming = MFRC522_ReadReg(BitFramingReg);
    MFRC522_WriteReg(BitFramingReg, 0x07); // send only 7 bits

    buf[0] = reqMode;
    status = MFRC522_ToCard(PCD_TRANSCEIVE, buf, 1, TagType, &backBits);

    MFRC522_WriteReg(BitFramingReg, originalBitFraming); // restore

    return status;
}

// ---- Replace MFRC522_Anticoll to properly read 5 bytes (4 uid + BCC) ----
uint8_t MFRC522_Anticoll(uint8_t *serNum)
{
    uint8_t backLen;
    uint8_t status;
    uint8_t buffer[10];  // bigger buffer

    uint8_t sendBuf[2] = {PICC_ANTICOLL, 0x20};
    status = MFRC522_ToCard(PCD_TRANSCEIVE, sendBuf, 2, buffer, &backLen);

    if (status == 0) {
        // Copy first 5 bytes (most cards are 4-byte UID + BCC)
        for (int i = 0; i < 5; i++) {
            serNum[i] = buffer[i];
        }
        return 0; // success (skip BCC check for now)
    }
    return 1; // error
}
