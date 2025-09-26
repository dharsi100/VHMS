#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx.h"
#include "lcd.h"
#include "cmn.h"
#include "rfid.h"
#include "thermistor.h"
#include "waterlevel.h"
#include "battery.h"
#include "mpu.h"

// === Task to handle LCD + RFID scan messages ===
void vTaskUI(void *pvParameters) {
    (void) pvParameters;
    char buf[32];

    for (;;) {
        lprint(0x80, "Scanning RFID...");
        if (g_rfidUID[0] != 0) {
            lprint(0xC0, "Card Found      ");
            sprintf(buf, "UID:%02X%02X%02X%02X",
                    g_rfidUID[0], g_rfidUID[1], g_rfidUID[2], g_rfidUID[3]);
            lprint(0x94, buf);
        } else {
            lprint(0xC0, "No Card         ");
        }

        // Show other sensor readings
        sprintf(buf, "T:%.1fC V:%.2fV", g_temperature, g_batteryVoltage);
        lprint(0xD4, buf);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void) {
    // Basic hardware init
    SystemInit();
    USART3_Init();
    LcdInit();
    SPI1_Init();

    // Create tasks
    xTaskCreate(vTaskRFID,       "RFID",       256, NULL, 2, NULL);
    xTaskCreate(vTaskThermistor, "Thermistor", 256, NULL, 2, NULL);
    xTaskCreate(vTaskBattery,    "Battery",    256, NULL, 2, NULL);
    xTaskCreate(vTaskWaterLevel, "WaterLevel", 256, NULL, 2, NULL);
    xTaskCreate(vTaskMPU,        "MPU",        256, NULL, 2, NULL);
    xTaskCreate(vTaskUI,         "UI",         256, NULL, 1, NULL);

    // Start scheduler
    vTaskStartScheduler();

    while (1) {
        // should never reach here
    }
}
