/*
	Elecanisms Final Project using a PIC24F
	
	Shivam Desai and Noam Rubin
	December 09, 2013
	
	Questions? 		   [Shivam@students.olin.edu; Noam.Rubin@students.olin.edu]
*/

/***************************************************** 
		Includes
**************************************************** */

// Include files
#include <p24FJ128GB206.h> // PIC
#include "config.h"
#include "common.h"
#include "oc.h"			   // output compare
#include "pin.h"
#include "timer.h"
#include "uart.h"
#include "ui.h"
#include "usb.h"
#include <stdio.h>

// Define vendor requests
#define SET_VALS            1   // Vendor request that receives 2 unsigned integer values
#define GET_VALS            2   // Vendor request that returns  2 unsigned integer values
#define PRINT_VALS          3   // Vendor request that prints   2 unsigned integer values 
#define PING_TREE     		4   // Vendor request that prints 	1 unsigned integer value

// Define names for pins
#define MOTOR_2_CW_CCW	&D[0]
#define MOTOR_2_RESET	&D[1]
#define MOTOR_2_ENABLE	&D[2]
#define MOTOR_2_CLK		&D[3]
#define MOTOR_1_CW_CCW	&D[4]
#define MOTOR_1_RESET	&D[5]
#define MOTOR_1_ENABLE	&D[6]
#define MOTOR_1_CLK		&D[7]
#define LIMIT_START		&D[8]
#define LIMIT_END		&D[9]

#define POT         	&A[0] // User input

// Define names for timers
#define BLINKY_TIMER	  &timer1 // blinky light (heartbeat)
#define STARTUP_TIMER	  &timer2 //
#define MOTOR_1_CLK_TIMER &timer3
#define MOTOR_2_CLK_TIMER &timer4

// Define track limits
#define LIMIT_BEGIN	  0 	// reset the count to min
#define LIMIT_END	  1000	// 					  max

/***************************************************** 
		Function Prototypes & Variables
**************************************************** */ 

void initChip(void);
void initInt(void);

void __attribute__((interrupt)) _CNInterrupt(void); 

uint16_t LOW  = 0;
uint16_t HIGH = 1;

uint16_t POT_VAL;

/*************************************************
			Initialize the PIC24F
**************************************************/

void initChip(){
	
    init_clock();
    init_uart();
    init_pin();
    init_ui();
    init_timer();
    init_oc(); 		// initialize the output compare module for STEPPERS

	pin_digitalIn(LIMIT_START);		// configure digital inputs
    pin_digitalIn(LIMIT_END);

	pin_digitalOut(MOTOR_1_CLK);    // configure digital outputs
	pin_digitalOut(MOTOR_2_CLK);
    pin_digitalOut(MOTOR_1_CW_CCW);
    pin_digitalOut(MOTOR_2_CW_CCW);
    pin_digitalOut(MOTOR_1_ENABLE);
    pin_digitalOut(MOTOR_2_ENABLE);
    pin_digitalOut(MOTOR_1_RESET);
    pin_digitalOut(MOTOR_2_RESET);
    
    pin_analogIn(POT);      		// configure analog inputs

}

/*************************************************
            Initialize Interrupts 
**************************************************/

void initInt(void) {

	// Enable encoder change interrupt
	CNEN1bits.CN14IE = 1; 	// configure change notification interrupt D[0]
	IFS1bits.CNIF = 0;		// clear change notification flag D[0]	
	IEC1bits.CNIE = 1;		// enable notification interrupt D[0]

}

/*************************************************
            Initialize Motor 
**************************************************/

void initMotor(void) {

    pin_write(MOTOR_1_ENABLE, LOW);  // Disable Motor 1
    pin_write(MOTOR_1_CLK, LOW);
    pin_write(MOTOR_1_CW_CCW, LOW);
    pin_write(MOTOR_1_RESET, LOW);

    pin_write(MOTOR_2_ENABLE, LOW);  // Disable Motor 2
    pin_write(MOTOR_2_CLK, LOW);
    pin_write(MOTOR_2_CW_CCW, LOW);
    pin_write(MOTOR_2_RESET, LOW);
    
}

/*************************************************
            Interrupt Declarations
**************************************************/

void __attribute__((interrupt, auto_psv)) _CNInterrupt(void) {
    limit_serviceInterrupt();
}

/*************************************************
            Interrupt Service Routines
**************************************************/

void limit_serviceInterrupt() {
    IFS1bits.CNIF = 0; // clear change notification flag D[0]	
}

/*************************************************
			Vendor Requests
**************************************************/

void VendorRequests(void) {
    WORD temp;

    switch (USB_setup.bRequest) {
        case SET_VALS:
            VEL_VAL = USB_setup.wValue.w;
            BD[EP0IN].bytecount = 0;    // set EP0 IN byte count to 0 
            BD[EP0IN].status = 0xC8;    // send packet as DATA1, set UOWN bit
            break;      
        default:
            USB_error_flags |= 0x01;    // set Request Error Flag
    }
}

void VendorRequestsIn(void) {
    switch (USB_request.setup.bRequest) {
        default:
            USB_error_flags |= 0x01;                    // set Request Error Flag
    }
}

void VendorRequestsOut(void) {
    switch (USB_request.setup.bRequest) {
        default:
            USB_error_flags |= 0x01;                    // set Request Error Flag
    }
}

/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/

int16_t main(void) {
	
	initChip();						// initialize the PIC pins etc.
    InitUSB();                      // initialize the USB registers and serial interface engine
    initInt();						// initialize the interrupt pins
    initMotor();					// initialize the motor pins

    led_on(&led1);					// initial state for BLINKY LIGHT
    timer_setPeriod(BLINKY_TIMER, 1);	// timer for BLINKY LIGHT
    timer_start(BLINKY_TIMER);
	
    while (USB_USWSTAT!=CONFIG_STATE) {     // while the peripheral is not configured...
        
        ServiceUSB();                       // ...service USB requests
        
    }

    while (1) {

        ServiceUSB(); 

        if (timer_flag(BLINKY_TIMER)) {	// when the timer trips
            timer_lower(BLINKY_TIMER);
            led_toggle(&led1);			// toggle the BLINKY LIGHT
        }
			
        POT_VAL = pin_read(POT);
                
    }
}
