/**	
 * |----------------------------------------------------------------------
 * | Copyright (C) Tilen Majerle, 2015
 * | 
 * | This program is free software: you can redistribute it and/or modify
 * | it under the terms of the GNU General Public License as published by
 * | the Free Software Foundation, either version 3 of the License, or
 * | any later version.
 * |  
 * | This program is distributed in the hope that it will be useful,
 * | but WITHOUT ANY WARRANTY; without even the implied warranty of
 * | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * | GNU General Public License for more details.
 * | 
 * | You should have received a copy of the GNU General Public License
 * | along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * |----------------------------------------------------------------------
 */
#include "esp8266_ll.h"

uint8_t ESP8266_LL_USARTInit(uint32_t baudrate) {
	uint16_t prescaler;
	
	/* Calculate prescaler for baudrate */
	prescaler =  (F_CPU / 4 / baudrate - 1) / 2; 
	
	/* Check valid input */
	if (((F_CPU == 16000000UL) && (baudrate == 57600)) || (prescaler > 4095)) {
		UCSR0A = 0;
		
		/* Calculate new value */
		prescaler = (F_CPU / 8 / baudrate - 1) / 2;
	}
	
	/* Set UART prescaler for baudrate */
	UBRR1H = (prescaler >> 8) & 0xFF;
	UBRR1L = prescaler & 0xFF;
	
	/* Set double sampling */
	UCSR1A |= 1 << U2X1;
	
	/* Data size 8 bits */
	UCSR1C |= (1 << UCSZ10) | (1 << UCSZ11);
	
	/* Enable receiver and transmitter */
	UCSR1B |= (1 << RXEN1) | (1 << TXEN1);
	
	/* Enable RX Not empty interrupt */
	UCSR1B |= (1 << RXCIE1);
	
	/* Enable interrupts if not already */
	sei();
	
	/* Make a little delay */
	_delay_ms(10);
	
	/* Return 0 = Successful */
	return 0;
}

uint8_t ESP8266_LL_USARTSend(uint8_t* data, uint16_t count) {
	/* Wait till end */
	while ((UCSR1A & (1 << UDRE1)) == 0);
	
	/* Send data via USART */
	while (count--) {
		/* Send character */
		UDR1 = *data++;

		/* Wait till done */
		while ((UCSR1A & (1 << UDRE1)) == 0);
	}
	
	/* Return 0 = Successful */
	return 0;
}

/* USART receive interrupt handler */
ISR(USART1_RX_vect) {
	uint8_t ch = UDR1;
	
	/* Send received character to ESP stack */
	ESP8266_DataReceived(&ch, 1);
}
