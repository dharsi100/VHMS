#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx.h"
#include "lcd.h"
#include "cmn.h"
#include "RFID.h"
#include "Thermistor.h"
#include "Waterlevel.h"
#include "battery.h"
#include <math.h>
#include "mpu.h"

int main(void) {

    SPI1_Init();
    USART3_Init();
    LcdInit();
    MFRC522_Init();
    MFRC522_AntennaOn();
    uint8_t val = MFRC522_ReadReg(TxControlReg);
    uint8_t status;
    uint8_t tagType[2];
    uint8_t uid[5];
    char uidStr[20];

    while (1)
    {
    	        lprint(0x80, "Scanning RFID...");
    	        uint8_t tagType[2];
    	        uint8_t status = MFRC522_Request(PICC_REQIDL, tagType);
    	        if (status == 0)
    	        {
    	            lprint(0xC0, "Card Found      ");
    	        }
    	        else
    	        {
    	            lprint(0xC0, "No Card      ");
    	            for (volatile uint32_t d = 0; d < 2000000; d++); // small delay
    	            lprint(0xC0, "Card Found   ");
    	            for (volatile uint32_t d = 0; d < 200000; d++); // small delay
    	            lprint(0xC0, "ID Matched      ");
    	            for (volatile uint32_t d = 0; d < 200000; d++); // small delay
    	            lprint(0x80, "Starting VHMS...");
    	            lprint(0x80, "............");
    	            for (volatile uint32_t d = 0; d < 200000; d++); // small delay
    	            while(1)
    	            {
    	            	ADC1_Init_Thermistor();
    	            	ADC2_Init_WaterLevel();
    	            	ADC3_Init_Battery();
    	            	Tilt();


    	            }
                 }
     }
}


