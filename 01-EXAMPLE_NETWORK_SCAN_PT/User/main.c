/**
 * Keil project example for ESP8266 SERVER mode and without RTOS support
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
 * This examples shows how you can use ESP for basic server
 *
 * - Library if initialized using ESP_Init
 * - Device must connect to network. Check WIFINAME and WIFIPASS defines for proper settings for your wifi network
 * - On debug port, IP address will be written to you where you can connect with browser
 * - Magic will begin, you should see something on your screen on PC
 * - On debug output (PA2 pin) is printf targeted via UART at 921600 bauds
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

#define DEBUG_USART         USART2
#define DEBUG_USART_PP      TM_USART_PinsPack_1

/* Wifi network settings, replace with your settings */
#define WIFINAME            "Majerle WiFiii"
#define WIFIPASS            "majerle_internet"
uint8_t networkMAC[] = {0xA4, 0x2B, 0xB0, 0xC2, 0xB7, 0xBE};

/* Search variables */
static uint8_t network_searching = 0;
struct pt PT;

/* ESP working structure and result enumeration */
evol ESP_t ESP;
ESP_Result_t espRes;

/* Array for access points search */
ESP_AP_t aps[10];
uint16_t ar;

/* ESP callback declaration */
int ESP_Callback(ESP_Event_t evt, ESP_EventParams_t* params);

static
PT_THREAD(NETWORK_SCAN_THREAD(struct pt* pt)) {
    PT_BEGIN(pt);
    
    PT_WAIT_UNTIL(pt, ESP_IsReady(&ESP) == espOK);          /* Wait to be ready first */
    
    /* Scan network for AP */
    if ((espRes = ESP_STA_ListAccessPoints(&ESP, aps, sizeof(aps) / sizeof(aps[0]), &ar, 0)) == espOK) {
        printf("Network scan has started successfully!\r\n");
        
        PT_WAIT_UNTIL(pt, ESP_IsReady(&ESP) == espOK);      /* Wait ESP to be ready */
        espRes = ESP_GetLastReturnStatus(&ESP);             /* Get actual result status from ESP device */
        
        if (espRes == espOK) {                              /* ESP device returned OK? */
            uint8_t i = 0;
            printf("Network scan for access points was successful and found %d access point(s).\r\n", ar);
            
            for (i = 0; i < ar; i++) {              /* Go through all access points found */
                printf("AP %d: Name: %s, RSSI: %d, MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", i, aps[i].SSID, aps[i].RSSI,
                    aps[i].MAC[0], aps[i].MAC[1], aps[i].MAC[2], aps[i].MAC[3], aps[i].MAC[4], aps[i].MAC[5]
                );
            }
        } else {
            printf("Problems with scanning network: %d\r\n", espRes);
        }
    } else {
        printf("Problems with starting network scan: %d\r\n", espRes);
    }
    
    network_searching = 0;                                  /* Finished with network search */
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
        
        if (network_searching) {                            /* When active, process thread */
            NETWORK_SCAN_THREAD(&PT);                       /* Call function */
        }
        
        if (TM_DISCO_ButtonOnPressed()) {                   /* On button press */
            network_searching = 1;                          /* Set variable for searching */
        }
	}
}

/* 1ms handler function, called from SysTick interrupt */
void TM_DELAY_1msHandler(void) {
    ESP_UpdateTime(&ESP, 1);                /* Update ESP library time for 1 ms */
}

/**
 * \brief  Application thread to work with ESP module only
 */
void ESP_Main_Thread(void const* params) {
    /* Init ESP library with 115200 bauds */
    if ((espRes = ESP_Init(&ESP, 115200, ESP_Callback)) == espOK) {
        printf("ESP module init successfully!\r\n");
    } else {
        printf("ESP Init error. Status: %d\r\n", espRes);
    }
    
    /* Scan network for AP */
    if ((espRes = ESP_STA_ListAccessPoints(&ESP, aps, sizeof(aps) / sizeof(aps[0]), &ar, 1)) == espOK) {
        uint8_t i = 0;
        printf("Network scan for access points was successful and found %d access point(s).\r\n", ar);
        
        for (i = 0; i < ar; i++) {              /* Go through all access points found */
            printf("AP %d: Name: %s, RSSI: %d, MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", i, aps[i].SSID, aps[i].RSSI,
                aps[i].MAC[0], aps[i].MAC[1], aps[i].MAC[2], aps[i].MAC[3], aps[i].MAC[4], aps[i].MAC[5]
            );
        }
    }
    
    /* Try to connect to wifi network in blocking mode */
    if ((espRes = ESP_STA_Connect(&ESP, WIFINAME, WIFIPASS, networkMAC, 0, 1)) == espOK) {
        printf("Connected to network\r\n");
    } else {
        printf("Problems trying to connect to network: %d\r\n", espRes);
    }
    
    while (1) {
        ESP_ProcessCallbacks(&ESP);             /* Process all callbacks */
    }
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
