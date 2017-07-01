/**
 * Keil project example for ESP8266 SNTP feature using RTOS
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
 * This examples shows how you can use SNTP protocol to get current time from ESP8266
 *
 * - Library is initialized using ESP_Init
 * - Device must connect to network. Check WIFINAME and WIFIPASS defines for proper settings for your wifi network
 * - Press on button to get current time from device
 * - On debug output (PA2 pin) is printf targeted via UART at 921600 bauds
 *
 * \note  Example uses single buffer for all connections.
 *        Since only one connection is active at a time in this example, there is no problems with that.
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
#define WIFINAME            "wifi_ssid"
#define WIFIPASS            "wifi_password"

/* ESP working structure and result enumeration */
evol ESP_t ESP;
ESP_Result_t espRes;

/* SNTP structure */
ESP_SNTP_t sntp;
ESP_DateTime_t datetime;
char sntp_server[3][50];

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
    uint8_t i = 0;
    /* Init ESP library with 115200 bauds */
    if ((espRes = ESP_Init(&ESP, 115200, ESP_Callback)) == espOK) {
        printf("ESP module init successfully!\r\n");
    } else {
        printf("ESP Init error. Status: %d\r\n", espRes);
    }
    
    /* Try to connect to wifi network in blocking mode */
    if ((espRes = ESP_STA_Connect(&ESP, WIFINAME, WIFIPASS, NULL, 0, 1)) == espOK) {
        printf("Connected to network\r\n");
    } else {
        printf("Problems trying to connect to network: %d\r\n", espRes);
    }
    
    /* Get SNTP config */
    for (i = 0; i < 3; i++) {
        sntp.Addr[i] = sntp_server[i];
    }
    if ((espRes = ESP_SNTP_GetConfig(&ESP, &sntp, 1)) == espOK) {
        printf("SNTP config received\r\n");
        printf("SNTP enabled: %d\r\n", sntp.Enable);
        printf("SNTP timezone: %d\r\n", sntp.Timezone);
        printf("SNTP server 1: %s\r\n", sntp.Addr[0]);
        printf("SNTP server 2: %s\r\n", sntp.Addr[1]);
        printf("SNTP server 3: %s\r\n", sntp.Addr[2]);
    } else {
        printf("Problems to get SNTP config: %d\r\n", espRes);
    }
    
    /* Set SNTP config */
    sntp.Enable = 1;                        /* Enable SNTP */
    sntp.Timezone = 2;                      /* Set timezone */
    if ((espRes = ESP_SNTP_SetConfig(&ESP, &sntp, 1)) == espOK) {
        printf("SNTP config enabled\r\n");
    } else {
        printf("Problems to get SNTP config: %d\r\n", espRes);
    }
    
    /* Get config again */
    if ((espRes = ESP_SNTP_GetConfig(&ESP, &sntp, 1)) == espOK) {
        printf("SNTP config received\r\n");
        printf("SNTP enabled: %d\r\n", sntp.Enable);
        printf("SNTP timezone: %d\r\n", sntp.Timezone);
        printf("SNTP server 1: %s\r\n", sntp.Addr[0]);
        printf("SNTP server 2: %s\r\n", sntp.Addr[1]);
        printf("SNTP server 3: %s\r\n", sntp.Addr[2]);
    } else {
        printf("Problems to get SNTP config: %d\r\n", espRes);
    }
    
    while (1) {
        ESP_ProcessCallbacks(&ESP);         /* Process all callbacks */
        
        if (TM_DISCO_ButtonOnPressed()) {   /* Handle button press */
            /* Try to connect to server as client, connect to example.com domain */
            if ((espRes = ESP_SNTP_GetDateTime(&ESP, &datetime, 1)) == espOK) {
                printf("Date time received: %02d.%02d.%4d %02d:%02d:%02d\r\n", 
                    datetime.Date, datetime.Month, datetime.Year,
                    datetime.Hours, datetime.Minutes, datetime.Seconds
                );
            } else {
                printf("Problems trying to get current time: %d\r\n", espRes);
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
