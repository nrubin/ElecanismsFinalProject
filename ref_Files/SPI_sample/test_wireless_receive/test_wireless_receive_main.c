/*
	Wireless communication using a PIC18F4550 (master) and nRF24L01 (slave) - Receiver
	
	Shivam Desai
	July 15th, 2013
	
	Questions? 						[shivamxdesai@gmail.com]
*/

/***************************************************** 
		Includes
**************************************************** */

// Include files
#include <p18cxxx.h>						// Finds correct header for any PIC18
#include <delays.h>							// Enables you to add varying amount of NOPs (Delays)
#include <stdlib.h>
#include <spi.h>							// Just in case we want to cheat with SPI

// Define names for LED pins
#define LED1 		LATAbits.LATA0			// blinky light (code is running)

// Define names for SPI connection pins
#define CE 			LATAbits.LATA2			// Chip Enable (CE)			-- Pin that activates RX or TX mode
#define IRQ 		PORTBbits.RB2			// Interrupt Request (IRQ)	-- Pin that signals reception
#define SDO 		TRISCbits.TRISC7		// Serial Data Out (SDO)	-- Pin that data goes out from
#define SDI 		TRISBbits.TRISB0		// Serial Data In (SDI)		-- Pin that data comes in to
#define SCK 		TRISBbits.TRISB1		// Serial Clock (SCK)		-- Pin that sends clock signal to sync all  SPI devices

// Define names for "Slave Select" pins
#define SS  		LATAbits.LATA4			// Slave Select (SS)		-- Pin that selects "slave" module to communicate with

#define select 		0						// Device is "active low," define "select" as 0
#define deselect	1

// Define names for SPI configuration pins
#define SMP			SSPSTATbits.SMP			// Sample Bit (SMP)			-- Pin that sets where to sample data
#define CKP			SSPCON1bits.CKP			// Clock Polarity (CKP)		-- Pin that sets where SCK will idle
#define CKE			SSPSTATbits.CKE			// Clock Edge Select (CKE)	-- Pin that sets which transition transmission takes place
#define SSPEN		SSPCON1bits.SSPEN		// Serial Prt Enbl (SSPEN)	-- Pin that will enable serial port and configure SCK, SDO, SDI and SS as serial port pins

#define WCOL		SSPCON1bits.WCOL		// Write Collision (WCOL)	-- Detect bit
#define WCOL_LED	LATCbits.LATC0			// Write Collision (WCOL)	-- Pin for collision visual
#define RX_LED		LATCbits.LATC2			// RX_DR					-- Pin for data reception successful visual

// Define commands for Nordic via SPI
#define R_REGISTER		0x00
#define W_REGISTER		0x20
#define R_RX_PAYLOAD	0x61
#define W_TX_PAYLOAD	0xA0
#define FLUSH_TX		0xE1
#define FLUSH_RX		0xE2
#define REUSE_TX_PL		0xE3
#define NOP				0xFF

// Define Nordic registers
#define CONFIG			0x00
#define EN_AA			0x01
#define EN_RXADDR		0x02
#define SETUP_AW		0x03
#define SETUP_RETR		0x04
#define RF_CH			0x05
#define RF_SETUP		0x06
#define STATUS			0x07
#define OBSERVE_TX		0x08
#define CD				0x09
#define RX_ADDR_P0		0x0A
#define RX_ADDR_P1		0x0B
#define RX_ADDR_P2		0x0C
#define RX_ADDR_P3		0x0D
#define RX_ADDR_P4		0x0E
#define RX_ADDR_P5		0x0F
#define TX_ADDR			0x10
#define RX_PW_P0		0x11
#define RX_PW_P1		0x12
#define RX_PW_P2		0x13
#define RX_PW_P3		0x14
#define RX_PW_P4		0x15
#define RX_PW_P5		0x16
#define FIFO_STATUS		0x17

/***************************************************** 
		Function Prototypes & Variables
**************************************************** */ 

void initChip(void);
void init_spi_master(void);
void init_nordic(void);
void InterruptHandler_SPI(void);
void InterruptHandler_Nordic(void);
void DelayMS(unsigned long milliseconds);
void irq_pin_set();
int bittesting(int val, int shift);

void spi_write(int data);
int spi_read(void);
void receive_data(void);

unsigned char data;
unsigned char input;
unsigned char able_to_read;

unsigned char data_received;

/*************************************************
  RESET VECTORS: REMOVE IF NOT USING BOOTLOADER!!!
**************************************************/

extern void _startup (void);        
#pragma code _RESET_INTERRUPT_VECTOR = 0x000800
void _reset (void)
{
    _asm goto _startup _endasm
}
#pragma code

/*************************************************
  Interrupt Service Routines
**************************************************/

#pragma code _HIGH_INTERRUPT_VECTOR = 0x000808
void _high_ISR (void)
{
	InterruptHandler_SPI();
}
#pragma code

void DelayMS(unsigned long milliseconds)
{
	unsigned long count;
	
	for(count = 0; count < milliseconds; count++)
		Delay10KTCYx(1);
}

/********************************************************* 
	Interrupt Handlers
**********************************************************/

#pragma interrupt InterruptHandler_SPI

void InterruptHandler_SPI(void) {
	if(PIR1bits.SSPIF == 1) // SPI Interrupt
	{
		PIR1bits.SSPIF = 0; // clear interrupt
		able_to_read = 1; 
	}	
}

/*************************************************
			Initialize the PIC
**************************************************/

void initChip(){
	PORTA = 0x00;			// reset all PORT registers
	PORTB = 0x00;
	PORTC = 0x00;
	PORTD = 0x00;
	PORTE = 0x00;

	TRISA = 0x00;			// set all PORTA to be OUTPUT
	TRISB = 0b00000101;		// set RB0 and RB2 as INPUT
							// and the rest of PORTB as OUTPUT
	TRISC = 0x00;			// set all PORTC to be OUTPUT
	TRISD = 0x00;			// set all PORTD to be OUTPUT
	TRISE = 0x00;			// set up PORTE to be OUTPUT
	
	ADCON1 = 0x7; //disable AD converter functionality on PORTA

	input = 0b00000000;
	data = 0b00000000;
	able_to_read = 0; // initilize variables
	data_received = 0;
}

/*************************************************
			Initialize the SPI Master
**************************************************/

void init_spi_master() {

	SDI = 1;	// Serial Data In 			- enable master mode
	SCK = 0;	// Serial Clock				- clear TRISB1 for master
	SDO = 0;	// Serial Data Out 			- clear TRISC7

	SMP = 0;	// Sample bit 				- sample in middle
	CKP = 0;	// Clock Polarity			- low when idle		**MUST BE SAME ON MASTER AND SLAVE**
	CKE = 1;	// Clock Edge Select		- transmit on transition from active to idle
	
	SSPCON1bits.SSPM3 = 0;	// Set the clock rate in master mode
	SSPCON1bits.SSPM2 = 0;	// to be FOSC/64 (Frequency of Oscillation)
	SSPCON1bits.SSPM1 = 1;
	SSPCON1bits.SSPM0 = 0;
	
	SSPEN = 0;	// Wait to enable serial port and configure SCK, SDO, SDI and SS as serial port pins
	
	PIE1bits.SSPIE = 1;	// enable SPI interrupt
}

/*************************************************
			Initialize the Nordic
**************************************************/

void init_nordic() {

	CE = 1; 					    // hold CE high to receive
	
	SS = select;
	spi_write(W_REGISTER+CONFIG);   // Write to the CONFIG register
	spi_write(0b00111111);		  	  // [7] Reserved as 0
									  // [6] RX_DR Interrupt 0 - unmask
								  	  // [5] TX_DS Interrupt 1 - mask
									  // [4] MAX_RT Interrupt 1 - mask
									  // [3] Enable CRC
									  // [2] 2 byte CRC
									  // [1] PWR_UP
									  // [0] RX 1
	SS = deselect;

	DelayMS(2);

	SS = select;
	spi_write(W_REGISTER+CONFIG);   // Write to the CONFIG register
	spi_write(0b00111111);		  	  // [7] Reserved as 0
									  // [6] RX_DR Interrupt 0 - unmask
								  	  // [5] TX_DS Interrupt 1 - mask
									  // [4] MAX_RT Interrupt 1 - mask
									  // [3] Enable CRC
									  // [2] 2 byte CRC
									  // [1] PWR_UP
									  // [0] RX 1
	SS = deselect;
	
	SS = select;
	spi_write(W_REGISTER+SETUP_AW); // Write to the SETUP_AW register
	spi_write(0b00000011);		  	  // Set the width to 5 bytes
	SS = deselect;

	SS = select;
	spi_write(W_REGISTER+EN_RXADDR); // Write to the EN_RXADDR register
	spi_write(0b00000001);		  	  // Enable data pipe 0
	SS = deselect;

	SS = select;
	spi_write(W_REGISTER+RX_PW_P0); // Write to the RX_PW_P0 register
	spi_write(0b00000001);		  	  // set the number of bytes in RX payload in data pipe 0 to 1 byte
	SS = deselect;

//	SS = select;
//	spi_write(R_REGISTER+CONFIG);   // Read the CONFIG register
//	spi_write(0b00000000);   // Read the CONFIG register
//	SS = deselect;
	
}

/*************************************************
			Functions for Writing Data
**************************************************/

void spi_write(int data) { 
	
	SSPEN = 1;			   // Enable serial port and configure SCK, SDO, SDI and SS as serial port pins
	
	if(WCOL == 0) { 	   // collision testing
		SSPBUF = data;
	}
	else {				   // there is a collision
		WCOL = 0;
		WCOL_LED = !WCOL_LED; //visual to see a collision
	}
	
	while(!DataRdySPI());
	
	input = SSPBUF;
}

/*************************************************
			Functions for Reading Data
**************************************************/

int spi_read() {
	if(able_to_read == 1) {
		able_to_read = 0;
		return SSPBUF;
	}
	else {
		return 0b11111111; // not able to read so turn on all LEDS
	}
}

void receive_data(void) {
	CE = 0; 					// bring CE low to disable receiver
	SS = select;
	spi_write(R_RX_PAYLOAD);	// send command for reception
	spi_write(0b00000000);		// send dummy data
	SS = deselect;
	CE = 1;
}

/*************************************************
			NORDIC INTERRUPT REQUEST
**************************************************/

int bittesting(int val, int shift){
	if(((1 << shift) & val) != 0){
		return 1;
	}
	else{
		return 0;
	}
}

void irq_pin_set(void){

	SS = select;
	spi_write(R_REGISTER+STATUS);   // Read the STATUS register
	spi_write(0b00000000);   		// Read the STATUS register
	SS = deselect;

	if(bittesting(input,6)==1){				// if the value of RX_DR is 1 then receive succeed
		SS = select;
		spi_write(W_REGISTER+STATUS);
		spi_write(0b01111111); // clear
		SS = deselect;
		data_received = 1;
		RX_LED = !RX_LED;
	}
	if(bittesting(input,6)==0){ 			// if the value of RX_DR is 0 then receive failed
		receive_data(); 					// failed so retry reception
	}
}

/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/

void main() {

	PORTD = 0b00000000;
	initChip();
	init_spi_master();
	init_nordic();
	INTCONbits.GIEH = 1;    // enables all high priority interrupts
	INTCONbits.GIEL = 1;    // enables all low priority interrupts

	while (1) {
		
//		SS = select;
//		spi_write(R_REGISTER+STATUS);   // Read the STATUS register
//		spi_write(0b00000000);   // Read the STATUS register
//		SS = deselect;

//		PORTA = input; // visualize the STATUS register

		if(IRQ == 0){
				irq_pin_set();
		}
		
		if (data_received == 1) {
				LED1 = !LED1; 		// toggle LED - data is received
				receive_data();
				PORTD = input;
		}
		DelayMS(50); // make the damn thing blink
	}
}















