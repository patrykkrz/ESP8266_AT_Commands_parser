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

uint8_t ESP_LL_Init(ESP_LL_t* LL) {
    /* Init USART */
    TM_USART_Init(USART1, TM_USART_PinsPack_1, LL->Baudrate);
    
    /* Initialize reset pin */
    TM_GPIO_Init(GPIOA, GPIO_PIN_0, TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_UP, TM_GPIO_Speed_Low);
        
    /* We were successful */
    return 0;
}

uint8_t ESP_LL_SendData(ESP_LL_t* LL, const uint8_t* data, uint16_t count) {
    /* Send data */
    TM_USART_Send(USART1, (uint8_t *)data, count);
    
    /* We were successful */
    return 0;
}

uint8_t ESP_LL_SetReset(ESP_LL_t* LL, uint8_t state) {
    /* Set pin according to status */
    if (state == ESP_RESET_SET) {
        TM_GPIO_SetPinLow(GPIOA, GPIO_PIN_0);
    } else {
        TM_GPIO_SetPinHigh(GPIOA, GPIO_PIN_0);
    }
    
    /* We are OK */
    return 0;
}

uint8_t ESP_LL_SetRTS(ESP_LL_t* LL, uint8_t state) {
    /* We are OK */
    return 0;
}

/* USART receive interrupt handler */
void TM_USART1_ReceiveHandler(uint8_t ch) {
	/* Send received character to ESP stack */
	ESP_DataReceived(&ch, 1);
}
