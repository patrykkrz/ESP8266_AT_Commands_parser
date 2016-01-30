/**
 * @author  Tilen Majerle
 * @email   tilen@majerle.eu
 * @website http://stm32f4-discovery.com
 * @link    
 * @version v1.0
 * @ide     Keil uVision
 * @license GNU GPL v3
 * @brief   Low level, platform dependant, part for communicate with ESP module and platform.
 *	
\verbatim
   ----------------------------------------------------------------------
    Copyright (C) Tilen Majerle, 2016
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.
     
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
   ----------------------------------------------------------------------
\endverbatim
 */
#ifndef ESP8266_LL_H
#define ESP8266_LL_H 100

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*                                                                        */
/*    Edit file name to esp8266_ll.h and edit values for your platform    */
/*                                                                        */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

/**
 * @defgroup ESP8266_LL
 * @brief    Low level, platform dependent, part for communicate with ESP module and platform.
 * @{
 *
 * This is a low-level part of ESP module library.
 *
 * It provides communication between ESP module and platform. There are some function, which needs to be implemented by user and with care.
 *
 * \par U(S)ART configuration
 *
 * ESP8266 module works with U(S)ART communication with device. For this purpose, library supposes 2, USART based, functions, which are called from ESP module stack when needed:
 *
 * - \ref ESP8266_LL_USARTInit: Function, which is called when USART should be initialized
 * - \ref ESP8266_LL_USARTSend: Function, which is called when data should be sent to ESP8266 device
 *
 * ESP stack module does not check for any incoming data from ESP8266 module to USART of your device.
 *
 * Most microcontrollers have USART RX capability, so when USART RX interrupt happens,
 * you should send this received byte to ESP8266 module using @ref ESP8266_DataReceived function to notify new incoming data.
 * Use interrupt handler routing to notify new data using previous mentioned function.
 *
\code
//Code below show example procedure and does need to be directly for your platform.
//You have to make sure to properly configure UART and RX interrupt for it.

//USART Initialization function, which is called from ESP stack
uint8_t ESP8266_LL_USARTInit(uint32_t baudrate) {
    //Init USART peripheral and GPIO clock for it.
    //Use baudate as passed in function.

    //Enable RX Data Ready interrupt (RXNE = Received Not Empty)
    
    //Return 0 = Successful
    return 0;
}

//USART send function, which is called from ESP stack
uint8_t ESP8266_LL_USARTSend(uint8_t* data, uint16_t count) {
    //Send data via USART
    
    //Return 0 = Successful
    return 0;
}

//USART receive interrupt handler,
//Function names depends on your platform
void USART_RX_INTERRUPT_HANDLER_FUNCTION_NAME(void) {
    uint8_t ch;
    //Get character from USART, example code
    uart_read_character(&ch);
    
    //Send received character to ESP stack
    ESP8266_DataReceived(&ch, 1);
}

\endcode
 * 
 * \par Reset configuration
 *
 * ESP8266 module can be reset using AT commands. However, it may happen that ESP module ignores AT commands for some reasons.
 *
 * You need to implement 3 macros or functions for GPIO capability for reset purpose.
 *
 * Take a look for \ref ESP8266_RESET_INIT, \ref ESP8266_RESET_LOW and \ref ESP8266_RESET_HIGH macros.
 *
\code
//Examples for 3 defines
//Implementation must be made according to your platform
//This is just a technical pseudo code and does not need to work on any platform

//Init reset pin as output
//Use do-while if you must specify more than 1 statement in define
#define ESP8266_RESET_INIT        do {  \
    GPIOA->MODER |= GPIO_Pin_5;         \
    GPIOA->OTYPER |= GPIO_Pin_5;        \
} while (0)

//Set reset pin high
#define ESP8266_RESET_HIGH        GPIOA->ODR |= GPIO_Pin_5

//Set reset pin low
#define ESP8266_RESET_LOW         GPIOA->ODR &= ~GPIO_Pin_5
\endcode
 *
 * \par Time configuration
 *
 * ESP module part needs current time in milliseconds to take care of any possible timeouts on connections and similar things.
 *
 * You need to implement your own time source (systick interrupt, normal timer with interrupt capability, etc) to tell ESP stack current time.
 *
 * Use @ref ESP8266_TimeUpdate to notify ESP stack with new time.
 *
\code
//Example of configuring timer for 1ms interrupts

//Function for timer init
void timer_init() {
    //Enable timer clock if needed

    //Set timer prescaler and period according to selected platform and MCU speed
    //Set values for 1ms overflow

    //Enable timer interrupt overflow
}

int main(void) {
    //Init timer
    timer_init();

    //Do other job
    ...
}

//Handle timer interrupts
void TIMER_INTERRUPT_FUNCTION_NAME(void) {
    //Call stack function and notify time update
    //Notify that time has been updated for 1 ms according to last time
    ESP8266_TimeUpdate(&ESP8266, 1);
}
\endcode
 *
 * \par Delay configuration
 *
 * ESP module part needs milliseconds based delay for some special events, specially on initialization process.
 *
 * You need to implement your own delay function to be used with ESP stack.
 *
 * Use \ref ESP8266_DELAYMS macro to specify delay function.
 *
\code
//Set delay function

//Init delay function
#define ESP8266_DELAYMS(x)      Delayms(x)

//Example for Atmel AVR
#define ESP8266_DELAYMS(x)      do {   \
    uint16_t i;                        \
    for (i = 0; i < x; i++) {          \
        _delay_ms(1);                  \
    }                                  \
} while (0)
\endcode
 *
 * \par Dependencies
 *
\verbatim
 Platform based dependencies
\endverbatim
 */

/**
 * @defgroup ESP8266_LL
 * @brief    Low level, platform dependent, part for communicate with ESP module and platform.
 * @{
 */

/* Include ESP layer */
#include "esp8266.h"

/**
 * @brief   Provides delay for amount of milliseconds
 * @param   x: Number of milliseconds for delay
 * @retval  None
 */
#define ESP8266_DELAYMS(x)         Delayms(x)

/**
 * @brief  Initializes U(S)ART peripheral for ESP8266 communication
 * @note   This function is called from ESP stack
 * @param  baudrate: baudrate for USART you have to use when initializing USART peripheral
 * @retval Initialization status:
 *           - 0: Initialization OK
 *           - > 0: Initialization failed
 */
uint8_t ESP8266_LL_USARTInit(uint32_t baudrate);

/**
 * @brief  Sends data from ESP stack to ESP8266 module using USART
 * @param  *data: Pointer to data array which should be sent
 * @param  count: Number of data bytes to sent to module
 * @retval Sent status:
 *           - 0: Sent OK
 *           - > 0: Sent error
 */
uint8_t ESP8266_LL_USARTSend(uint8_t* data, uint16_t count);

/**
 * @brief  Initializes reset pin on platform
 * @note   Function is called from ESP stack module when needed
 * @note   Declared as macro 
 */
#define ESP8266_RESET_INIT    (void)0
	
/**
 * @brief  Sets reset pin LOW
 * @note   Function is called from ESP stack module when needed
 * @note   Declared as macro 
 */
#define ESP8266_RESET_LOW     (void)0

/**
 * @brief  Sets reset pin HIGH
 * @note   Function is called from ESP stack module when needed
 * @note   Declared as macro 
 */
#define ESP8266_RESET_HIGH    (void)0

/**
 * @}
 */

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif
