#ifndef CONFIG_H
#define CONFIG_H

/* Program 35 Configuration */

/* Hardware */
#define HSE_VALUE       8000000U
#define SYSCLK_FREQ     72000000U

/* UART */
#define UART_BAUD       115200
#define UART_TX_PIN     GPIO_PIN_9
#define UART_RX_PIN     GPIO_PIN_10

/* LED Pins (STM32) */
#define LED_RED         GPIO_PIN_1
#define LED_YEL         GPIO_PIN_2
#define LED_GRN         GPIO_PIN_3

/* ESP32 Pins */
#define LED_GPIO_RED    GPIO_NUM_2
#define LED_GPIO_YEL    GPIO_NUM_4
#define LED_GPIO_GRN    GPIO_NUM_5

/* FreeRTOS Stack Sizes */
#define TASK_STACK_SIZE 512
#define QUEUE_LENGTH    10

/* Chip name */
#define CHIP_NAME "ESP32"

#endif // CONFIG_H
