/** 
   ----------------------------------------------------------------------
    Copyright (c) 2016 Tilen Majerle

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge,
    publish, distribute, sublicense, and/or sell copies of the Software, 
    and to permit persons to whom the Software is furnished to do so, 
    subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
    AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
   ----------------------------------------------------------------------
 */
#include "esp8266_ll.h"

/* Include platform dependant libraries */
#include "stm32fxxx_hal.h"
#include "tm_stm32_usart.h"
#include "tm_stm32_usart_dma.h"
#include "tm_stm32_delay.h"
#include "tm_stm32_gpio.h"

/* UART and pin configuration */
#if defined(STM32F769_DISCOVERY)
#define LL_UART             UART5
#define LL_RESET_PORT       GPIOJ
#define LL_RESET_PIN        GPIO_PIN_14
#define LL_UART_TX_PORT     GPIOC
#define LL_UART_TX_PIN      GPIO_PIN_12
#define LL_UART_RX_PORT     GPIOD
#define LL_UART_RX_PIN      GPIO_PIN_2

#define LL_CH_PD_PORT       GPIOH
#define LL_CH_PD_PIN        GPIO_PIN_7
#define LL_GPIO2_PORT       GPIOG
#define LL_GPIO2_PIN        GPIO_PIN_3

#else

#define LL_UART             USART1
#define LL_RESET_PORT       GPIOA
#define LL_RESET_PIN        GPIO_PIN_0
#define LL_UART_TX_PORT     GPIOA
#define LL_UART_TX_PIN      GPIO_PIN_9
#define LL_UART_RX_PORT     GPIOA
#define LL_UART_RX_PIN      GPIO_PIN_10
#endif /* defined(STM32F769_DISCOVERY) */

uint8_t ESP_LL_Init(ESP_LL_t* LL) {
    static uint8_t first = 1;
    
    /* Init USART */
    TM_USART_Init(LL_UART, TM_USART_PinsPack_Custom, LL->Baudrate);
    
    if (first) {
        /* Initialize reset pin */
        TM_GPIO_Init(LL_RESET_PORT, LL_RESET_PIN, TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_UP, TM_GPIO_Speed_Low);
        TM_GPIO_SetPinHigh(LL_RESET_PORT, LL_RESET_PIN);
        
#if defined(STM32F769_DISCOVERY)
        /* Setup control pins for ESP8266 */
        TM_GPIO_Init(LL_CH_PD_PORT, LL_CH_PD_PIN, TM_GPIO_Mode_IN, TM_GPIO_OType_PP, TM_GPIO_PuPd_UP, TM_GPIO_Speed_Low);
        TM_GPIO_Init(LL_GPIO2_PORT, LL_GPIO2_PIN, TM_GPIO_Mode_IN, TM_GPIO_OType_PP, TM_GPIO_PuPd_UP, TM_GPIO_Speed_Low);
#endif
            
        /* First time reset */
        first = 0;
    }
        
    /* We were successful */
    return 0;
}

uint8_t ESP_LL_SendData(ESP_LL_t* LL, const uint8_t* data, uint16_t count) {
    /* Send data */
    TM_USART_Send(LL_UART, (uint8_t *)data, count);
    
    /* We were successful */
    return 0;
}

uint8_t ESP_LL_SetReset(ESP_LL_t* LL, uint8_t state) {
    /* Set pin according to status */
    if (state == ESP_RESET_SET) {
        TM_GPIO_SetPinLow(LL_RESET_PORT, LL_RESET_PIN);
    } else {
        TM_GPIO_SetPinHigh(LL_RESET_PORT, LL_RESET_PIN);
    }
    
    /* We are OK */
    return 0;
}

uint8_t ESP_LL_SetRTS(ESP_LL_t* LL, uint8_t state) {
    /* We are OK */
    return 0;
}

/* Callback for pin initialization */
void TM_USART_InitCustomPinsCallback(USART_TypeDef* USARTx, uint16_t AlternateFunction) {
    /* Set up pins for UART */
	if (USARTx == LL_UART) {
        TM_GPIO_InitAlternate(LL_UART_TX_PORT, LL_UART_TX_PIN, TM_GPIO_OType_PP, TM_GPIO_PuPd_UP, TM_GPIO_Speed_Fast, AlternateFunction);
        TM_GPIO_InitAlternate(LL_UART_RX_PORT, LL_UART_RX_PIN, TM_GPIO_OType_PP, TM_GPIO_PuPd_UP, TM_GPIO_Speed_Fast, AlternateFunction);
    }
}

/* Handle receive */
#if defined(STM32F769_DISCOVERY)
void TM_UART5_ReceiveHandler(uint8_t ch) {
    /* Send received character to ESP stack */
    ESP_DataReceived(&ch, 1);
}
#else
/* USART receive interrupt handler */
void TM_USART1_ReceiveHandler(uint8_t ch) {
    /* Send received character to ESP stack */
    ESP_DataReceived(&ch, 1);
}
#endif /* STM32F769_DISCOVERY */
