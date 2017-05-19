/**
 * Keil project example for ESP8266 ACCESS POINT wit hRTOS
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
 * This examples shows how you can use ESP for listing access points using RTOS
 *
 * - Library is initialized using ESP_Init
 * - Device must connect to network. Check WIFINAME and WIFIPASS defines for proper settings for your wifi network
 * - Software Access point is enabled.
 * - After successful setup, wifi network should be shown in your phone/PC/..
 * - Connect to that AP and later press button on board to check ESP for stations if any connected.
 * - On debug output (PA2 pin) is printf targeted via UART at 921600 bauds
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
#define WIFINAME            "wifi_network_SSID"
#define WIFIPASS            "wifi_password"

/* ESP working structure and result enumeration */
evol ESP_t ESP;
ESP_Result_t espRes;

/* Max connected stations */
#define MAX_STATIONS        3
ESP_ConnectedStation_t stations[MAX_STATIONS];
uint16_t sr;

/* Thread prototypes */
void ESP_Update_Thread(void const* params);
void ESP_Main_Thread(void const* params);

/* Thread definitions */
osThreadDef(ESP_Update, ESP_Update_Thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);
osThreadDef(ESP_Main, ESP_Main_Thread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE);

osThreadId ESP_Update_ThreadId, ESP_Main_ThreadId;

/* ESP callback declaration */
int ESP_Callback(ESP_Event_t evt, ESP_EventParams_t* params);

int main(void) {
    TM_RCC_InitSystem();                                    /* Init system */
    HAL_Init();                                             /* Init HAL layer */
    TM_DISCO_LedInit();                                     /* Init leds */
    TM_DISCO_ButtonInit();                                  /* Init button */
    TM_DELAY_Init();                                        /* Init delay */
    TM_USART_Init(DEBUG_USART, DEBUG_USART_PP, 921600);     /* Init USART for debug purpose */

    /* Print first screen message */
    printf("ESP8266 commands parser; Compiled: %s %s\r\n", __DATE__, __TIME__);

    /* Initialize threads */
    ESP_Update_ThreadId = osThreadCreate(osThread(ESP_Update), NULL);
    ESP_Main_ThreadId = osThreadCreate(osThread(ESP_Main), NULL);

    /* Start kernel */
    osKernelStart();
    
	while (1) {

	}
}

/* 1ms handler function, called from SysTick interrupt */
void TM_DELAY_1msHandler() {
    ESP_UpdateTime(&ESP, 1);                /* Update ESP library time for 1 ms */
    osSystickHandler();                     /* Kernel systick handler processing */
}

/***********************************************/
/**            Thread implementations         **/
/***********************************************/

/**
 * \brief  Update ESP received data thread
 */
void ESP_Update_Thread(void const* params) {
    while (1) {
        /* Process ESP update */
        ESP_Update(&ESP);
    }
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
    
    /* Set access point */
    ESP.APConf.Hidden = 0;                  /* Allow AP to be seen in network */
    ESP.APConf.MaxConnections = MAX_STATIONS;   /* Allow up to 3 devices connected to AP at a time */
    strcpy((char *)ESP.APConf.SSID, "ESP_AP_RTOS"); /* Set AP name */
    strcpy((char *)ESP.APConf.Pass, "mypassword");  /* Set AP password */
    ESP.APConf.Ecn = ESP_Ecn_WPA2_PSK;      /* Set security level */
    
    /* Set config */
    if ((espRes = ESP_AP_SetConfig(&ESP, (ESP_APConfig_t *)&ESP.APConf, 0, 1)) == espOK) {
        printf("Access point settings are set. You may connect to AP now\r\n");
    } else {
        printf("Problems trying to set access point settings: %d\r\n", espRes);
    }
        
    while (1) {
        ESP_ProcessCallbacks(&ESP);         /* Process all callbacks */
        
        if (TM_DISCO_ButtonOnPressed()) {   /* When button is pressed */
            /* Try to read stations connected to soft Access Point */
            if ((espRes = ESP_AP_ListConnectedStations(&ESP, stations, MAX_STATIONS, &sr, 1)) == espOK) {
                if (sr) {                   /* Check if any station connected */
                    uint8_t i = 0;
                    printf("%d station(s) found on soft Access Point\r\n", sr);
                    for (i = 0; i < sr; i++) {  /* Print IP addresses of every device */
                        printf("Device %d: %d.%d.%d.%d\r\n", i, stations[i].IP[0], stations[i].IP[1], stations[i].IP[2], stations[i].IP[3]);
                    }
                } else {
                    printf("No stations connected to our Access Point\r\n");
                }
            } else {
            
            }
        }
    }
}

/***********************************************/
/**               Library callback            **/
/***********************************************/
int ESP_Callback(ESP_Event_t evt, ESP_EventParams_t* params) {    
    switch (evt) {                              /* Check events */
        case espEventIdle:
            printf("Stack is IDLE!\r\n");
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
