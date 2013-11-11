#include <p24FJ128GB206.h>
#include "config.h"
#include "common.h"
#include "usb.h"
#include "pin.h"
#include "uart.h"
#include "oc.h"
#include "ui.h"
#include "timer.h"
#include <stdio.h>

#define SET_VELOCITY    0   // Vendor request that receives 2 unenesigned integer values
#define GET_VALS    1   // Vendor request that returns 2 unsigned integer values
#define CLEAR_REV 2 //Vender request that clears the rev counter

#define UARTTx &D[0]
#define UARTRx &D[1]
#define HeartBeatTimer &timer1

uint16_t val1, val2, received_char;



void VendorRequests(void) {
    WORD temp;

    switch (USB_setup.bRequest) {
        case SET_VELOCITY:
            BD[EP0IN].bytecount = 0;    // set EP0 IN byte count to 0 
            BD[EP0IN].status = 0xC8;    // send packet as DATA1, set UOWN bit
            break;
        case CLEAR_REV:
            // DUTY = USB_setup.wValue.w;
            // REQUESTED_DIRECTION = USB_setup.wIndex.w;
            BD[EP0IN].bytecount = 0;    // set EP0 IN byte count to 0 
            BD[EP0IN].status = 0xC8;    // send packet as DATA1, set UOWN bit
            break;
        case GET_VALS:
            temp.w = val1;
            BD[EP0IN].address[0] = temp.b[0];
            BD[EP0IN].address[1] = temp.b[1];
            temp.w = val2;
            BD[EP0IN].address[2] = temp.b[0];
            BD[EP0IN].address[3] = temp.b[1];
            BD[EP0IN].bytecount = 4;    // set EP0 IN byte count to 4
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

init_ardcom(void){
    uart_open(&uart2, UARTTx, UARTRx, NULL, NULL, 9600., 'N', 1, 
          0, NULL, 0, NULL, 0);
}

void init(void){
    init_pin();
    init_clock();
    init_uart();
    init_ui();
    init_timer();
    init_oc();
    InitUSB(); // initialize the USB registers and serial interface engine

}

int16_t main(void) {
    init();
    val1 = 0;
    val2 = 0;
    led_on(&led2);
    timer_setPeriod(HeartBeatTimer, 0.5);
    timer_start(HeartBeatTimer);
    printf("Good morning!\n");
    
    while (USB_USWSTAT!=CONFIG_STATE) {     // while the peripheral is not configured...
        ServiceUSB();                       // ...service USB requests
    }
    while (1) {
        val1 = uart_getc(&uart2);
        val2 = uart_getc(&uart2);
        ServiceUSB();
         if (timer_flag(HeartBeatTimer)) {
            timer_lower(HeartBeatTimer);
            led_toggle(&led1);
        }      
    }
}

