/*
 * 01-ESP8266_ATMEGA_2560.c
 *
 * Created: 30.1.2016 13:49:40
 * Author : MajerleT
 *
 * Example, done for Atmel AVR with 8k of RAM memory.
 * After initialization process and when IP is received, you can establish new connection with external server,
 * if you put "button" pin low. See description below for more info about pinout
 *
 * Used board is Arduino MEGA with ATMEGA2560 microcontroller.
 *
 * Note: ESP8266 is 3.3V, Arduino MEGA is 5V on outputs. This can blow up ESP8266, so you need level translators!
 *
 * Pinout (See Google how to setup your ESP8266 device):
 *
 * ESP8266        ARDUINO MEGA        DESCRIPTION
 * 
 * RX             TX1, pin 18         Use level translator from 5 to 3.3V, you can use simple resistor divider. I used 220 and 330 ohm resistors
 * TX             RX1, pin 19         No need for level translators
 * VCC            3.3V                Use external 3.3V regulator
 * GND            GND
 * RST            PB7, pin 13         Use level translator from 5 to 3.3V, you can use simple resistor divider
 *
 *                PB6                 Button. Use wire to set pin low. After you remove wire from GND, connection should start if everything is OK
 *
 * Debug UART is mapped to normal Arduino pins for Serial, RX0 and RX1, which gives you normal USB access, 115200 baud is used
 * I used Atmel MKII to download software to microcontroller.
 */ 

/**
 * F_CPU macro is defined in global compiler symbols section. 
 */

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "stdio.h"
#include "esp8266.h"

/* Debug output stream */
int Debug_Putc (char c, FILE *stream);

/* Init debug stream */
FILE DebugStream = FDEV_SETUP_STREAM(Debug_Putc, NULL, _FDEV_SETUP_WRITE);

/* ESP8266 working structure */
ESP8266_t ESP8266;

/* UART for debug purpose */
void DebugUARTInit(uint32_t baudrate);
void TimerInit(void);

int main(void) {
	/* Set output stream */
	stdout = &DebugStream;
	
	/* Init UART for debug */
	DebugUARTInit(115200);
	
	/* Initialize 1ms timer */
	TimerInit();
	
	/* Enable global interrupts */
	sei();
	
	/* Output debug message */
	printf("Program started! ATMEGA 2560 on Arduino MEGA board\r\n");
	
	/* Initialize ESP8266 */
	while (ESP8266_Init(&ESP8266, 115200) != ESP_OK) {
		printf("Error trying to initialize ESP8266 module\r\n");
	}
	
	/* Set mode to STA+AP */
	while (ESP8266_SetMode(&ESP8266, ESP8266_Mode_STA_AP) != ESP_OK);
	
	/* Enable server on port 80 */
	while (ESP8266_ServerEnable(&ESP8266, 80) != ESP_OK);
	
	/* Module is connected OK */
	printf("Initialization finished!\r\n");
	
	/* Disconnect from wifi if connected */
	ESP8266_WifiDisconnect(&ESP8266);
	
	/* Wait till finishes */
	ESP8266_WaitReady(&ESP8266);
	
#if ESP8266_USE_APSEARCH
	/* Get a list of all stations */
	ESP8266_ListWifiStations(&ESP8266);
	
	/* Wait till finishes */
	ESP8266_WaitReady(&ESP8266);
#endif
	
	/* Connect to wifi and save settings */
	ESP8266_WifiConnect(&ESP8266, "YOUR SSID", "YOUR PASSWORD");
	
	/* Wait till finish */
	ESP8266_WaitReady(&ESP8266);
	
	/* Get connected devices */
	ESP8266_WifiGetConnected(&ESP8266);
	
	/* Init GPIO pin for making connections */
	/* Make pin input with pullup */
	DDRB &= ~(1 << PINB6);
	PORTB |= 1 << PINB6;
	
    /* Replace with your application code */
    while (1) {
		/* Update ESP stack periodically */
		ESP8266_Update(&ESP8266);
		
		/* Check if pin is low, make a new connection */
		if ((PINB & (1 << PINB6)) == 0) {
			/* Wait till pin is low */
			while ((PINB & (1 << PINB6)) == 0);
			
			/* Make a new connection */
			while (ESP8266_StartClientConnection(&ESP8266, "stm32f4disco", "stm32f4-discovery.com", 80, NULL) != ESP_OK);
		}
	}
}

/* We will use TIMER0 for basic 1ms interrupts */
void TimerInit(void) {
	/* Set timer to count up to OCR0A */
	TCCR0A |= 1 << WGM01;
	
	/* Clock = 16MHz */
	/* 16.000.000 / 64 = 250k ticks per second = 250 ticks per millisecond */
	
	/* Set prescaler to 64 */
	TCCR0B |= (1 << CS01) | (1 << CS00);
	
	/* Set counter to count up to 249 */
	OCR0A = 249;
	
	/* Set interrupt */
	TIMSK0 |= 1 << OCIE0A;
}

/* 1ms timer interrupt handler */
ISR(TIMER0_COMPA_vect) {
	/* Notify stack about new time */
	ESP8266_TimeUpdate(&ESP8266, 1);
}

/* UART interrupt handler, DEBUG uart */
ISR(USART0_RX_vect) {
	/* Get character */
	uint8_t ch = UDR0;
	
	/* Send it back */
	UDR0 = ch;
	while ((UCSR0A & (1 << UDRE0)) == 0);
}

void DebugUARTInit(uint32_t baudrate) {
	uint16_t prescaler;
	
	/* Calculate prescaler for baudrate */
	prescaler =  (F_CPU / 4 / baudrate - 1) / 2;
	
	/* Set UART prescaler for baudrate */
	UBRR0H = (prescaler >> 8) & 0xFF;
	UBRR0L = prescaler & 0xFF;
	
	UCSR0A |= 1 << U2X0;
	
	/* Data size 8 bits */
	UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01);
	
	/* Enable receiver and transmitter */
	UCSR0B |= (1 << RXEN0) | (1 << TXEN0);
	
	/* Enable RX Not empty interrupt */
	UCSR0B |= (1 << RXCIE0);
}

int Debug_Putc(char ch, FILE *stream) {
	/* Send character */
	UDR0 = ch;

	/* Wait till done */
	while ((UCSR0A & (1 << UDRE0)) == 0);

	/* Return OK */
	return 0;
}

/************************************/
/*           ESP CALLBACKS          */
/************************************/
/* Called when ready string detected */
void ESP8266_Callback_DeviceReady(ESP8266_t* ESP8266) {
	printf("Device is ready\r\n");
}

/* Called when watchdog reset on ESP8266 is detected */
void ESP8266_Callback_WatchdogReset(ESP8266_t* ESP8266) {
	printf("Watchdog reset detected!\r\n");
}

/* Called when we are disconnected from WIFI */
void ESP8266_Callback_WifiDisconnected(ESP8266_t* ESP8266) {
	printf("Wifi is disconnected!\r\n");
}

void ESP8266_Callback_WifiConnected(ESP8266_t* ESP8266) {
	printf("Wifi is connected!\r\n");
}

void ESP8266_Callback_WifiConnectFailed(ESP8266_t* ESP8266) {
	printf("Connection to wifi network has failed. Reason %d\r\n", ESP8266->WifiConnectError);
}

void ESP8266_Callback_WifiGotIP(ESP8266_t* ESP8266) {
	printf("Wifi got an IP address\r\n");
	
	/* Read that IP from module */
	printf("Grabbing IP status: %d\r\n", ESP8266_GetSTAIP(ESP8266));
}

void ESP8266_Callback_WifiIPSet(ESP8266_t* ESP8266) {
	/* We have STA IP set (IP set by router we are connected to) */
	printf("We have valid IP address: %d.%d.%d.%d\r\n", ESP8266->STAIP[0], ESP8266->STAIP[1], ESP8266->STAIP[2], ESP8266->STAIP[3]);
}

void ESP8266_Callback_DHCPTimeout(ESP8266_t* ESP8266) {
	printf("DHCP timeout!\r\n");
}

void ESP8266_Callback_WifiDetected(ESP8266_t* ESP8266, ESP8266_APs_t* ESP8266_AP) {
	uint8_t i = 0;
	
	/* Print number of detected stations */
	printf("We have detected %d AP stations\r\n", ESP8266_AP->Count);
	
	/* Print each AP */
	for (i = 0; i < ESP8266_AP->Count; i++) {
		/* Print SSID for each AP */
		printf("%2d: %s\r\n", i, ESP8266_AP->AP[i].SSID);
	}
}


/************************************/
/*         CLIENT CALLBACKS         */
/************************************/
void ESP8266_Callback_ClientConnectionConnected(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	/* We are connected to external server */
	printf("Client connected to server! Connection number: %s\r\n", Connection->Name);
	
	/* We are connected to server, request to sent header data to server */
	ESP8266_RequestSendData(ESP8266, Connection);
}

/* Called when client connection fails to server */
void ESP8266_Callback_ClientConnectionError(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	/* Fail with connection to server */
	printf("An error occurred when trying to connect on connection: %d\r\n", Connection->Number);
}

/* Called when data are ready to be sent to server */
uint16_t ESP8266_Callback_ClientConnectionSendData(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection, char* Buffer, uint16_t max_buffer_size) {
	/* Format data to sent to server */
	sprintf(Buffer, "GET / HTTP/1.1\r\n");
	strcat(Buffer, "Host: stm32f4-discovery.com\r\n");
	strcat(Buffer, "Connection: close\r\n");
	strcat(Buffer, "\r\n");
	
	/* Return length of buffer */
	return strlen(Buffer);
}

/* Called when data are send successfully */
void ESP8266_Callback_ClientConnectionDataSent(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	printf("Data successfully sent as client!\r\n");
}

/* Called when error returned trying to sent data */
void ESP8266_Callback_ClientConnectionDataSentError(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	printf("Error while sending data on connection %d!\r\n", Connection->Number);
}

void ESP8266_Callback_ClientConnectionDataReceived(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection, char* Buffer) {
	/* Data received from server back to client */
	printf("Data received from server on connection: %s; Number of bytes received: %lu; %lu / %lu;\r\n",
		Connection->Name,
		Connection->BytesReceived,
		Connection->TotalBytesReceived,
		Connection->ContentLength
	);
	
	/* Print message when first packet */
	if (Connection->Flags.F.FirstPacket) {
		
		/* Print first message */
		printf("This is first packet received. Content length on this connection is: %lu\r\n", Connection->ContentLength);
	}
}

/* Called when connection is closed */
void ESP8266_Callback_ClientConnectionClosed(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	printf("Client connection closed, connection: %d; Total bytes received: %lu; Content-Length header: %lu\r\n",
		Connection->Number, Connection->TotalBytesReceived, Connection->ContentLength
	);
}

/* Called when timeout is reached on connection to server */
void ESP8266_Callback_ClientConnectionTimeout(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	printf("Timeout reached on connection: %d\r\n", Connection->Number);
}
