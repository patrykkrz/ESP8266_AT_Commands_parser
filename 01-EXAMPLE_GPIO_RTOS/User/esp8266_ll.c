/**	
 * |----------------------------------------------------------------------
 * | Copyright (c) 2016 Tilen Majerle
 * |  
 * | Permission is hereby granted, free of charge, to any person
 * | obtaining a copy of this software and associated documentation
 * | files (the "Software"), to deal in the Software without restriction,
 * | including without limitation the rights to use, copy, modify, merge,
 * | publish, distribute, sublicense, and/or sell copies of the Software, 
 * | and to permit persons to whom the Software is furnished to do so, 
 * | subject to the following conditions:
 * | 
 * | The above copyright notice and this permission notice shall be
 * | included in all copies or substantial portions of the Software.
 * | 
 * | THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * | EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * | OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * | AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * | HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * | WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * | FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * | OTHER DEALINGS IN THE SOFTWARE.
 * |----------------------------------------------------------------------
 */
#include "esp8266_ll.h"
#include "esp8266.h"

/* Include platform dependant libraries */
#include "stm32fxxx_hal.h"
#include "tm_stm32_usart.h"
#include "tm_stm32_usart_dma.h"
#include "tm_stm32_delay.h"
#include "tm_stm32_gpio.h"

#if ESP_RTOS
osMutexId id;
#endif /* ESP_RTOS */

/*****************************************/
/* Set up GPIO and USART pins for boards */
/*****************************************/
#if defined(STM32F769_DISCOVERY)
#define ESP_USART               UART5
#define ESP_USART_TX_PORT       GPIOD
#define ESP_USART_TX_PIN        GPIO_PIN_2
#define ESP_USART_RX_PORT       GPIOC
#define ESP_USART_RX_PIN        GPIO_PIN_12
#define ESP_RESET_PORT          GPIOJ
#define ESP_RESET_PIN           GPIO_PIN_14
#endif /* defined(STM32F769_DISCOVERY) */
#if defined(STM32F7_DISCOVERY)
#define ESP_USART               USART6
#define ESP_USART_TX_PORT       GPIOC
#define ESP_USART_TX_PIN        GPIO_PIN_6
#define ESP_USART_RX_PORT       GPIOC
#define ESP_USART_RX_PIN        GPIO_PIN_7
#define ESP_RESET_PORT          GPIOA
#define ESP_RESET_PIN           GPIO_PIN_0
#endif /* defined(STM32F769_DISCOVERY) */
#if defined(NUCLEO_F401) || defined(NUCLEO_F411)
#define ESP_USART               USART1
#define ESP_USART_TX_PORT       GPIOA
#define ESP_USART_TX_PIN        GPIO_PIN_9
#define ESP_USART_RX_PORT       GPIOA
#define ESP_USART_RX_PIN        GPIO_PIN_10
#define ESP_RESET_PORT          GPIOA
#define ESP_RESET_PIN           GPIO_PIN_0
#endif /* defined(NUCLEO_F401) || defined(NUCLEO_F411) */
#if defined(STM32F429_DISCOVERY)
#define ESP_USART               USART1
#define ESP_USART_TX_PORT       GPIOA
#define ESP_USART_TX_PIN        GPIO_PIN_9
#define ESP_USART_RX_PORT       GPIOA
#define ESP_USART_RX_PIN        GPIO_PIN_10
#define ESP_RESET_PORT          GPIOA
#define ESP_RESET_PIN           GPIO_PIN_1
#endif /* defined(STM32F429_DISCOVERY) */
#if defined(STM32F4_DISCOVERY)
#define ESP_USART               USART1
#define ESP_USART_TX_PORT       GPIOB
#define ESP_USART_TX_PIN        GPIO_PIN_6
#define ESP_USART_RX_PORT       GPIOB
#define ESP_USART_RX_PIN        GPIO_PIN_7
#define ESP_RESET_PORT          GPIOA
#define ESP_RESET_PIN           GPIO_PIN_1
#endif /* defined(STM32F429_DISCOVERY) */

uint8_t ESP_LL_Callback(ESP_LL_Control_t ctrl, void* param, void* result) {
    switch (ctrl) {
        case ESP_LL_Control_Init: {                 /* Initialize low-level part of communication */
            ESP_LL_t* LL = (ESP_LL_t *)param;       /* Get low-level value from callback */
            
            /************************************/
            /*  Device specific initialization  */
            /************************************/
            TM_USART_Init(ESP_USART, TM_USART_PinsPack_Custom, LL->Baudrate);
#if defined(ESP_RESET_PORT) && defined(ESP_RESET_PIN)
            TM_GPIO_Init(ESP_RESET_PORT, ESP_RESET_PIN, TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_UP, TM_GPIO_Speed_Low);
            TM_GPIO_SetPinHigh(ESP_RESET_PORT, ESP_RESET_PIN);
#endif /* defined(ESP_RESET_PORT) && defined(ESP_RESET_PIN) */
#if defined(ESP_RTS_PORT) && defined(ESP_RTS_PIN)
            TM_GPIO_Init(ESP_RTS_PORT, ESP_RTS_PIN, TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_UP, TM_GPIO_Speed_Low);
            TM_GPIO_SetPinLow(ESP_RTS_PORT, ESP_RTS_PIN);
#endif /* defined(ESP_RESET_PORT) && defined(ESP_RESET_PIN) */
            
            if (result) {
                *(uint8_t *)result = 0;             /* Successfully initialized */
            }
            return 1;                               /* Return 1 = command was processed */
        }
        case ESP_LL_Control_Send: {
            ESP_LL_Send_t* send = (ESP_LL_Send_t *)param;   /* Get send parameters */
            
            /* Send actual data to UART */
            TM_USART_Send(ESP_USART, (uint8_t *)send->Data, send->Count);   /* Send actual data */
            
            if (result) {
                *(uint8_t *)result = 0;             /* Successfully send */
            }
            return 1;                               /* Command processed */
        }
#if defined(ESP_RESET_PORT) && defined(ESP_RESET_PIN)
        case ESP_LL_Control_SetReset: {             /* Set reset value */
            uint8_t state = *(uint8_t *)param;      /* Get state packed in uint8_t variable */
            if (state == ESP_RESET_SET) {           /* Check state value */
                TM_GPIO_SetPinLow(ESP_RESET_PORT, ESP_RESET_PIN);
            } else {
                TM_GPIO_SetPinHigh(ESP_RESET_PORT, ESP_RESET_PIN);
            }
            return 1;                               /* Command has been processed */
        }
#endif /* defined(ESP_RESET_PORT) && defined(ESP_RESET_PIN) */
#if defined(ESP_RTS_PORT) && defined(ESP_RTS_PIN)
        case ESP_LL_Control_SetRTS: {               /* Set RTS value */
            uint8_t state = *(uint8_t *)param;      /* Get state packed in uint8_t variable */
            if (state == ESP_RTS_SET) {             /* Check state value */
                TM_GPIO_SetPinHigh(ESP_RTS_PORT, ESP_RTS_PIN);
            } else {
                TM_GPIO_SetPinHigh(ESP_RTS_PORT, ESP_RTS_PIN);
            }
            return 1;                               /* Command has been processed */
        }
#endif /* defined(ESP_RTS_PORT) && defined(ESP_RTS_PIN) */
#if ESP_RTOS
        case ESP_LL_Control_SYS_Create: {           /* Create system synchronization object */
            ESP_RTOS_SYNC_t* Sync = (ESP_RTOS_SYNC_t *)param;   /* Get pointer to sync object */
            id = osMutexCreate(Sync);               /* Create mutex */
            
            if (result) {
                *(uint8_t *)result = id == NULL;    /*!< Set result value */
            }
            return 1;                               /* Command processed */
        }
        case ESP_LL_Control_SYS_Delete: {           /* Delete system synchronization object */
            ESP_RTOS_SYNC_t* Sync = (ESP_RTOS_SYNC_t *)param;   /* Get pointer to sync object */
            osMutexDelete(Sync);                    /* Delete mutex object */
            
            if (result) {
                *(uint8_t *)result = id == NULL;    /*!< Set result value */
            }
            return 1;                               /* Command processed */
        }
        case ESP_LL_Control_SYS_Request: {          /* Request system synchronization object */
            ESP_RTOS_SYNC_t* Sync = (ESP_RTOS_SYNC_t *)param;   /* Get pointer to sync object */
            (void)Sync;                             /* Prevent compiler warnings */
            
            *(uint8_t *)result = osMutexWait(id, 1000) == osOK ? 0 : 1; /* Set result according to response */
            return 1;                               /* Command processed */
        }
        case ESP_LL_Control_SYS_Release: {          /* Release system synchronization object */
            ESP_RTOS_SYNC_t* Sync = (ESP_RTOS_SYNC_t *)param;   /* Get pointer to sync object */
            (void)Sync;                             /* Prevent compiler warnings */
            
            *(uint8_t *)result = osMutexRelease(id) == osOK ? 0 : 1;    /* Set result according to response */
            return 1;                               /* Command processed */
        }
#endif /* ESP_RTOS */
        default: 
            return 0;
    }
}

#if defined(STM32F769_DISCOVERY)
/* USART receive interrupt handler */
void TM_UART5_ReceiveHandler(uint8_t ch) {
	/* Send received character to ESP stack */
	ESP_DataReceived(&ch, 1);
}
#endif /* defined(STM32F769_DISCOVERY) */

#if defined(NUCLEO_F401) || defined(NUCLEO_F411) || defined(STM32F4_DISCOVERY) ||defined(STM32F429_DISCOVERY)
/* USART receive interrupt handler */
void TM_USART1_ReceiveHandler(uint8_t ch) {
	/* Send received character to ESP stack */
	ESP_DataReceived(&ch, 1);
}
#endif /* defined(NUCLEO_F411) */

#if defined(STM32F7_DISCOVERY)
/* USART receive interrupt handler */
void TM_USART6_ReceiveHandler(uint8_t ch) {
	/* Send received character to ESP stack */
	ESP_DataReceived(&ch, 1);
}
#endif /* defined(NUCLEO_F411) */

/* Init custom pins */
void TM_USART_InitCustomPinsCallback(USART_TypeDef* USARTx, uint16_t AlternateFunction) { 
	if (USARTx == ESP_USART) {
        TM_GPIO_InitAlternate(ESP_USART_RX_PORT, ESP_USART_RX_PIN, TM_GPIO_OType_PP, TM_GPIO_PuPd_UP, TM_GPIO_Speed_Fast, AlternateFunction);
        TM_GPIO_InitAlternate(ESP_USART_TX_PORT, ESP_USART_TX_PIN, TM_GPIO_OType_PP, TM_GPIO_PuPd_UP, TM_GPIO_Speed_Fast, AlternateFunction);
    }
}
