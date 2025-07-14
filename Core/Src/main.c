#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx.h"
#include "lcd.h"
#include "cmn.h"
#include "Uson.h"
#include "Vib.h"
#include "Therm.h"

#define THERM_TASKDELAY 1000
#define USON_TASKDELAY 1200
#define VIB_TASKDELAY 1000

void GPIO_Init_Power(void)
{
	RCC->AHB1ENR |= 1<<0; // Enable clock to GPIOA

    GPIOA->MODER &= ~((3 << (2 * 3)) | (3 << (2 * 4))); // Clear mode PA 3, PA 4
    GPIOA->MODER |=  ((1 << (2 * 3)) | (1 << (2 * 4))); // Set output mode

    GPIOA->ODR |= (1<<3);
}


void Therm(void *pvParameters)
{
	GPIO_Init_MotorRelay();
	while(1)
	{
	 ADCModuleInittemp();
	 vTaskDelay(THERM_TASKDELAY); // Prevent task from hogging CPU
	}
}

void Vib(void *pvParameters)
{
    GPIO_Motordriver();
    pwm_gpio_init();

	while(1)
	{
	 ADCModuleInitVib();
	 vTaskDelay(VIB_TASKDELAY); // Prevent task from hogging CPU
	}
}

void Uson(void *pvParameters)
{
  TIM2_us_init();
  TIM4_us_init();
    GPIO_Init_Ultrasonic();
  void GPIO_Init_Buzzer();

  while (1)
  {
	//  display_distance();
	  char str[20];
	  	  uint32_t echo_time;
	  	  float distance_cm;

	  	  send_trigger();
	  	  echo_time = read_echo_time();// time in µs
	  	  if(echo_time==0)
	  	    {
	  		  lprint(0x89, "D: FAR ");
	  	    }
	  	  else
	  	 	{
	  	 	  distance_cm = echo_time / 58.0f;
	  		  sprintf(str, "D: %.2f", distance_cm);

	  	 	  if(distance_cm < DIST_THRESHOLD)
	  	 	  {
	  	 		  GPIOC->ODR |=(1<<9);
	  	 	  }
	  	 	  if(distance_cm > DIST_THRESHOLD)
	  	 	  {
	  	 		  GPIOC->ODR &=~(1<<9);
	  	 	  }
	  	       lprint(0x89, str);
	  	    }
      vTaskDelay(USON_TASKDELAY);
  }
}
/*
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
*/
int main(void)
{
 LcdInit();
 GPIO_Init_Power();

 xTaskCreate(Therm, "Thermal", 512, NULL, 1, NULL);
 xTaskCreate(Uson, "Distance", 512, NULL, 3, NULL);
 xTaskCreate(Vib, "Vibration", 512, NULL, 2, NULL);

 vTaskStartScheduler(); // Start the scheduler
 while (1);

 return 0;

}
