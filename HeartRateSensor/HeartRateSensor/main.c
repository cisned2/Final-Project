/*
 * HeartRateSensor.c
 *
 * Created: 5/3/2018 11:50:08 AM
 * Author : Damian
 * Descrition : This program collects and outputs data from the AD8232 to UART
 *				terminal. I used a bluetooth module to send data
 */ 

#define BAUD 9600
#define F_CPU 8000000UL

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <util/delay.h>

void init_USART();
void init_ADC();
void USART_tx_string(char*);
static int put_char(char c, FILE *stream); //required for printf
static FILE mystdout = FDEV_SETUP_STREAM(put_char, NULL, _FDEV_SETUP_WRITE); //required for printf

int main(void)
{
	DDRB = 0; //input for LO- and LO+ from AD8232

	char c[9]; //holds converted value in string
	stdout = &mystdout; //set the output stream

	init_USART();
	init_ADC();

	while(1){
			uint8_t low; //ADCL
			uint16_t all = 0; //ADC 10 bit
			uint16_t avg = 0; //average buffer
		
			for(int i=0;i < 100 ; i++){ //grab 100 data points and sum then average.
				ADCSRA |= (1 << ADSC); //start the conversion. while in free running mode it will
				while((ADCSRA&(1 << ADIF))==0); //check if conversion done
				ADCSRA |= (1 << ADIF); //reset flag
				low = ADCL; //grab lower 8 adc bits
				all = ADCH << 8 | low; //grab all 10 bits of ADC
				avg += all; //add to buffer
				_delay_us(1000);
			}
			
			all = avg/10; //average and scale down so its easier to see
			
			dtostrf(all,3,1,c); //convert double to string
			if(PINB & 0b00000011){ //check if all pads connected
				printf("0"); //pads not connected so output 0
			}
			else 
				USART_tx_string(c); //print value to terminal
			
			printf("\r\n");
	}
}

void init_USART(){
	unsigned int BAUDrate;

	//set BAUD rate: UBRR = [F_CPU/(16*BAUD)]-1
	BAUDrate = ((F_CPU/16)/BAUD) - 1;
	UBRR0H = (unsigned char) (BAUDrate >> 8); //shift top 8 bits into UBRR0H
	UBRR0L = (unsigned char) BAUDrate; //shift rest of 8 bits into UBRR0L
	UCSR0B |= (1 << RXEN0) | (1 << TXEN0); //enable receiver and trasmitter
	UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00); //set data frame: 8 bit, 1 stop
}

void init_ADC(){
	ADMUX = 0; //use ADC0
	ADMUX |= (1 << REFS0); //use AVcc as the reference (5V)
	ADMUX |= (1 << ADLAR); //set to right adjust for 8-bit ADC

	//ADCSRA |= (1 << ADIE); //ADC interrupt enable
	ADCSRA |= (1 << ADEN); //enable ADC
	
	//set pre-scale to 2 for input frequency
	ADCSRA &= ~(1 << ADPS2) | ~(1 << ADPS1) | ~(1 << ADPS0);
	
	ADCSRB = 0; //free running mode
}

static int put_char(char c, FILE *stream) //required for printf
{
	while(!(UCSR0A &(1<<UDRE0))); // wait for UDR to be clear
	UDR0 = c;    //send the character
	return 0;
}

void USART_tx_string(char* data){
	while((*data!='\0')){ //print until null
		while(!(UCSR0A &(1<<UDRE0))); //check if transmit buffer is ready for new data
		UDR0=*data; //print char at current pointer
		data++; //iterate char pointer
	}
}