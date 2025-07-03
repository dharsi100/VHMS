#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx.h"
#include "lcd.h"
#include "cmn.h"
#include "watlev.h"
#include "Uson.h"
#include "Vib.h"
#include "Therm.h"

void Therm(void *pvParameters)
{
	while(1)
	{
	 GPIO_Init_Therm();
	 GPIO_Init_MotorRelay();
	 ADCModuleInittemp();
	 vTaskDelay(1000); // Prevent task from hogging CPU
	}
}

void Vib(void *pvParameters)
{
	while(1)
	{
	 GPIO_Init_Vib();
	 GPIO_Motordriver();
	 pwm_gpio_init();
	 ADCModuleInitVib();
	 vTaskDelay(1000); // Prevent task from hogging CPU
	}
}


void Uson(void *pvParameters)
{
  char str[20];
  uint32_t echo_time;
  float distance_cm;
  GPIO_Init_Ultrasonic();
  TIM2_us_init();
  TIM4_us_init();

  RCC->AHB1ENR |= 1<<2; //Enable GPIOC
  GPIOC->MODER &= ~((3 << (2 * 9))); // Clear mode PC9
  GPIOC->MODER |=  ((1 << (2 * 9))); // Set output mode

  while (1)
  {
	  send_trigger();
	  echo_time = read_echo_time();// time in µs
	  if(echo_time==0)
	  {
		  lprint(0x89, "D: FAR ");
	  }
	  else{
	  distance_cm = echo_time / 58.0f;
	  sprintf(str, "D: %.2f", distance_cm);
	  if(distance_cm<4)
	  {
		  GPIOC->ODR |=(1<<9);
	  }
	  if(distance_cm>4)
	  {
		  GPIOC->ODR &=~(1<<9);
	  }
      lprint(0x89, str);

	  }
      vTaskDelay(1200);

}
}

int main(void)
{
 LcdInit();


 while(1)
 {
 xTaskCreate(Therm, "Thermal", 512, NULL, 1, NULL);
 xTaskCreate(Uson, "Distance", 512, NULL, 3, NULL);
 xTaskCreate(Vib, "Vibration", 512, NULL, 2, NULL);

 vTaskStartScheduler(); // Start the scheduler
 while (1)
  {


  }

 }
}
