/**
 * Keil project example for ESP8266 CLIENT mode and RTOS support
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
 * This examples shows how you can use ESP for basic client connection
 *
 * - Library is initialized using ESP_Init
 * - Device must connect to network. Check WIFINAME and WIFIPASS defines for proper settings for your wifi network
 * - Press on button to connect to example.com website with custom HTTP GET request method.
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

/* Client connection pointer */
ESP_CONN_t* conn;

/* Connection manupulation */
uint32_t bw;
const uint8_t requestData[] = ""
"GET / HTTP/1.1\r\n"
"Host: example.com\r\n"
"Connection: close\r\n"
"\r\n";

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
    
    /* Try to connect to wifi network in blocking mode */
    if ((espRes = ESP_STA_Connect(&ESP, WIFINAME, WIFIPASS, NULL, 0, 1)) == espOK) {
        printf("Connected to network\r\n");
    } else {
        printf("Problems trying to connect to network: %d\r\n", espRes);
    }
    
    while (1) {
        ESP_ProcessCallbacks(&ESP);         /* Process all callbacks */
        
        if (TM_DISCO_ButtonOnPressed()) {   /* Handle button press */
            /* Try to connect to server as client, connect to example.com domain */
            if ((espRes = ESP_CONN_Start(&ESP, &conn, ESP_CONN_Type_TCP, "example.com", 80, 1)) == espOK) {
                printf("Connected to example.com!\r\n");
                
                /* Send HTTP request for example */
                if ((espRes = ESP_CONN_Send(&ESP, conn, requestData, sizeof(requestData), &bw, 1)) == espOK) {
                    printf("Data sent! Number of bytes sent: %d. We expect connection will be closed by remote server\r\n", bw);
                } else {
                    printf("Problems trying to send data!\r\n");
                }
            } else {
                printf("Problems trying to connect to server as client: %d\r\n", espRes);
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
        case espEventConnActive: {
            ESP_CONN_t* conn = (ESP_CONN_t *)params->CP1;   /* Get connection for event */
            printf("Connection %d just became active!\r\n", conn->Number);
            
            break;
        }
        case espEventConnClosed: {
            ESP_CONN_t* conn = (ESP_CONN_t *)params->CP1;   /* Get connection for event */
            printf("Connection %d was just closed!\r\n", conn->Number);
            
            break;
        }
        case espEventDataReceived: {
            ESP_CONN_t* conn = (ESP_CONN_t *)params->CP1;   /* Get connection for event */
            uint8_t* data = (uint8_t *)params->CP2;         /* Get actual received data */
            uint16_t datalen = params->UI;                  /* Print length of received data in current part packet */
            
            printf("%d bytes of data received on connection %d\r\n", datalen, conn->Number);
            (void)(data);                                   /* Prevent compiler warnings */
            
            break;
        }
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
