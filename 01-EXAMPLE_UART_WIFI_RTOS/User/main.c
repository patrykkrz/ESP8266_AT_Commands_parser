/**
 * Keil project example for ESP8266 CLIENT mode (UART WIFI passthrough) and RTOS support
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
 * This examples shows how you can use ESP for UART to WiFi bridge as client device
 *
 * Requirements:
 * - Create listening socket on your PC (or any other server with this support)
 * - For Windows, a great tool is TCP/IP Builder, where you can create socket and listen to it.
 * - ESP stack must be in single connection mode. Check esp8266_config.h file for mode informations.
 *
 * - Library is initialized using ESP_Init
 * - Device must connect to network. Check WIFINAME and WIFIPASS defines for proper settings for your wifi network
 * - Set IP address for your PC and port where PC listens for your socket
 * - On button press, device will go to UART-WIFI mode, send some data and wait user to press button again to close UART-WIFI mode
 * - Connection will close
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

#define DEBUG_USART         USART2
#define DEBUG_USART_PP      TM_USART_PinsPack_1

/* Wifi network settings, replace with your settings */
#define WIFINAME            "wifi_ssid"
#define WIFIPASS            "wifi_password"

/* Set up your server settings to connect to */
#define IP_ADDR             "192.168.0.106"
#define PORT                100

/* ESP working structure and result enumeration */
evol ESP_t ESP;
ESP_Result_t espRes;

/* Client connection pointer */
ESP_CONN_t* conn;

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
        
        if (TM_DISCO_ButtonPressed()) {     /* Handle button press */
            while (TM_DISCO_ButtonPressed());   /* Wait button release */
            
            /* Set transparent mode, enable UART<->WIFI mode */
            if ((espRes = ESP_TRANSFER_SetMode(&ESP, ESP_TransferMode_Transparent, 1)) == espOK) {
                printf("Transparent mode enabled\r\n");
                
                /* Try to connect to server as client, connect to example.com domain */
                if ((espRes = ESP_CONN_Start(&ESP, &conn, ESP_CONN_Type_TCP, IP_ADDR, PORT, 1)) == espOK) {
                    printf("Connected to %s:%d!\r\n", IP_ADDR, PORT);
                    
                    /* Start UART<->WIFI transparent mode by execution AT+CIPSEND command */
                    if ((espRes = ESP_TRANSFER_Start(&ESP, 1)) == espOK) {
                        uint32_t i;
                        printf("Transfer has started\r\n");
                        
                        /* From now on, everything sent to ESP via UART will be transferred to WIFI directly (passthrough) */
                        
                        /* Send data 100 times */
                        for (i = 0; i < 100; i++) {
                            /* Send those bytes directly, without executing AT+CIPSEND each time */
                            ESP_TRANSFER_Send(&ESP, "1234567890\r\n", 12, 1);
                        }
                        
                        /* Wait for button press, user may send data from PC back to ESP to see what is happening */
                        /* Every received byte from PC to ESP should be displayed as event will trigger */
                        printf("Waiting button press...\r\n");
                        
                        while (!TM_DISCO_ButtonPressed());  /* Wait button press */
                        while (TM_DISCO_ButtonPressed());   /* Wait button release */
                        
                        printf("Stopping transfer mode\r\n");
                        
                        /* Stop UART<->WIFI mode */
                        if ((espRes = ESP_TRANSFER_Stop(&ESP, 1)) == espOK) {
                            printf("Transfer mode disabled!\r\n");
                        } else {
                            printf("Error disabling transfer mode: %d\r\n", espRes);
                        }
                    } else {
                        printf("Failed to start transfer: %d\r\n", espRes);
                    }
                    
                    /* Go back to normal mode */
                    if ((espRes = ESP_TRANSFER_SetMode(&ESP, ESP_TransferMode_Normal, 1)) == espOK) {
                        printf("Mode set back to normal\r\n");
                        
                        /* Close connection */
                        if ((espRes = ESP_CONN_Close(&ESP, conn, 1)) == espOK) {
                            printf("Connection closed\r\n");
                        } else {
                            printf("Failed to close connection: %d\r\n", espRes);
                        }
                    } else {
                        printf("Failed to set to normal mode: %d\r\n", espRes);
                    }
                } else {
                    printf("Failed to connect: %d\r\n", espRes);
                }
            } else {
                printf("Failed to set to transparent mode: %d\r\n", espRes);
            }
        }
    }
}

/***********************************************/
/**               Library callback            **/
/***********************************************/
int ESP_Callback(ESP_Event_t evt, ESP_EventParams_t* params) {
    switch (evt) {                                          /* Check events */
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
        case espEventTransparentReceived: {                 /* This event is always called from thread where ESP_Update function is called */
            uint8_t ch = *(uint8_t *)params->CP1;           /* Get single character from device */
            printf("%c", ch);                               /* Print character to screen */
            
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
