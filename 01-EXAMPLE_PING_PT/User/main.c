/**
 * Keil project example for ESP8266 PING mode and without RTOS support
 *
 * @note      Check defines.h file for configuration settings!
 * @note      When using Nucleo F411 board, example has set 8MHz external HSE clock!
 *
 * Before you start, select your target, on the right of the "Load" button
 *
 * @author    Tilen Majerle
 * @email     tilen@majerle.eu
 * @website   http://stm32f4-discovery.net
 * @ide       Keil uVision 5
 * @conf      PLL parameters are set in "Options for Target" -> "C/C++" -> "Defines"
 * @packs     STM32F4xx/STM32F7xx Keil packs are requred with HAL driver support
 * @stdperiph STM32F4xx/STM32F7xx HAL drivers required
 *
 * \par Description
 *
 * This examples shows how you can use ESP for ping
 *
 * - Library is initialized using ESP_Init
 * - Ping procedurect will start using protothread on button press
 *
 * Protothreads are used with non-blocking API calls to show how ESP stack can easily be used as blocking flow with non-blocking calls.
 * Check Adam Dunkels' protothreads for more informations about flow.
 *
 * \note  Example uses separate buffers for each connection, because multiple connections can be active at a time
 *
 * \par Pinout for example (Nucleo STM32F411)
 *
\verbatim
ESP         STM32F4xx           DESCRIPTION
 
RX          PA9                 TX from STM to RX from ESP
TX          PA10                RX from STM to RX from ESP
VCC         3.3V                Use external 3.3V regulator
GND         GND
RST         PA0                 Reset pin for ESP
CTS         PA3                 RTS from ST to CTS from ESP
            BUTTON(PA0, PC13)   Discovery/Nucleo button, depends on configuration
            
            PA2                 TX for debug purpose (connect to PC) with 921600 bauds
\endverbatim
 */
/* Include core modules */
#include "stm32fxxx_hal.h"
/* Include my libraries here */
#include "defines.h"
#include "tm_stm32_disco.h"
#include "tm_stm32_delay.h"
#include "tm_stm32_usart.h"
#include "esp8266.h"
#include "cmsis_os.h"

/* Set debug port */
#define DEBUG_USART         DISCO_USART
#define DEBUG_USART_PP      DISCO_USART_PP

/* Wifi network settings, replace with your settings */
#define WIFINAME            "Majerle WiFiii"
#define WIFIPASS            "majerle_internet"
uint8_t networkMAC[] = {0xA4, 0x2B, 0xB0, 0xC2, 0xB7, 0xBE};

/* Search variables */
static uint8_t ping = 0;
struct pt PT;

/* ESP working structure and result enumeration */
evol ESP_t ESP;
ESP_Result_t espRes;

/* Array for access points search */
uint32_t time;

/* ESP callback declaration */
int ESP_Callback(ESP_Event_t evt, ESP_EventParams_t* params);

static
PT_THREAD(PING_THREAD(struct pt* pt)) {
    PT_BEGIN(pt);
    
    PT_WAIT_UNTIL(pt, ESP_IsReady(&ESP) == espOK);          /* Wait to be ready first */
    
    /* Start ping procedure in non-blocking mode */
    if ((espRes = ESP_Ping(&ESP, "example.com", &time, 0)) == espOK) {
        printf("Network scan has started successfully!\r\n");
        
        PT_WAIT_UNTIL(pt, ESP_IsReady(&ESP) == espOK);      /* Wait ESP to be ready */
        espRes = ESP_GetLastReturnStatus(&ESP);             /* Get actual result status from ESP device */
        
        if (espRes == espOK) {                              /* ESP device returned OK? */
            printf("Ping successful in %d ms\r\n", time);
        } else {
            printf("Ping failed with error: %d\r\n", espRes);
        }
    } else {
        printf("Problems with starting ping prodecure: %d\r\n", espRes);
    }
    
    ping = 0;                                               /* Finished with pinging */
    PT_END(pt);
}

int main(void) {
    TM_RCC_InitSystem();                                    /* Init system */
    HAL_Init();                                             /* Init HAL layer */
    TM_DISCO_LedInit();                                     /* Init leds */
    TM_DISCO_ButtonInit();                                  /* Init button */
    TM_DELAY_Init();                                        /* Init delay */
    TM_USART_Init(DEBUG_USART, DEBUG_USART_PP, 921600);     /* Init USART for debug purpose */

    /* Print first screen message */
    printf("ESP8266 commands parser; Compiled: %s %s\r\n", __DATE__, __TIME__);
    
    PT_INIT(&PT);                                           /* Init protothread */

    /* Init ESP library with 115200 bauds */
    if ((espRes = ESP_Init(&ESP, 115200, ESP_Callback)) == espOK) {
        printf("ESP module init successfully!\r\n");
    } else {
        printf("ESP Init error. Status: %d\r\n", espRes);
    }
    
	while (1) {
        ESP_Update(&ESP);                                   /* Process ESP update */
        
        if (ping) {                                         /* When active, process thread */
            PING_THREAD(&PT);                               /* Call function */
        }
        
        if (TM_DISCO_ButtonOnPressed()) {                   /* On button press */
            ping = 1;                                       /* Set variable for thread */
        }
	}
}

/* 1ms handler function, called from SysTick interrupt */
void TM_DELAY_1msHandler(void) {
    ESP_UpdateTime(&ESP, 1);                /* Update ESP library time for 1 ms */
}

/***********************************************/
/**               Library callback            **/
/***********************************************/
int ESP_Callback(ESP_Event_t evt, ESP_EventParams_t* params) {
    switch (evt) {                              /* Check events */
        case espEventIdle:
            break;
        default:
            break;
    }
    
    return 0;
}

/* printf handler */
int fputc(int ch, FILE* fil) {
    TM_USART_Putc(DEBUG_USART, ch);         /* Send over debug USART */
    return ch;                              /* Return OK */
}
