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

#define GOTO_POS    0   // Vendor request that receives 2 unenesigned integer values
#define SET_0    1   // Vendor request that returns 2 unsigned integer values
#define GET_POS 2 //Vender request that clears the rev counter

#define HB_TIMER &timer3
#define CLK_TIMER &timer2
#define STARTUP_TIMER &timer1
#define ENABLE &D[1]
#define CLK &D[0]
#define RESET &D[2] 
#define CW &D[3]

uint16_t desired_pos, val2, startup_counter,clk_out, current_pos;


void VendorRequests(void) {
    WORD temp;

    switch (USB_setup.bRequest) {
        case GOTO_POS:
            desired_pos = USB_setup.wValue.w;
            BD[EP0IN].bytecount = 0;    // set EP0 IN byte count to 0 
            BD[EP0IN].status = 0xC8;    // send packet as DATA1, set UOWN bit
            break;
        case SET_0:
            current_pos = 0;
            // DUTY = USB_setup.wValue.w;
            // REQUESTED_DIRECTION = USB_setup.wIndex.w;
            BD[EP0IN].bytecount = 0;    // set EP0 IN byte count to 0 
            BD[EP0IN].status = 0xC8;    // send packet as DATA1, set UOWN bit
            break;
        case GET_POS:
            temp.w = current_pos;
            BD[EP0IN].address[0] = temp.b[0];
            BD[EP0IN].address[1] = temp.b[1];
            temp.w = desired_pos;
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

void init(void){
    init_pin();
    init_clock();
    init_uart();
    init_ui();
    init_timer();
    init_oc();
    InitUSB(); 

    // oc_pwm(&oc1,CLK,CLK_TIMER,10e3,10000); //10kHZ at 50% Duty

    timer_setPeriod(HB_TIMER, 0.3);
    timer_start(HB_TIMER);

    // max CLK freq is 15 kHz, four clock cycles 
    timer_setPeriod(STARTUP_TIMER, 0.001); 
    timer_start(STARTUP_TIMER);

    timer_setPeriod(CLK_TIMER, 0.0022); 
    timer_start(CLK_TIMER);

    pin_digitalOut(CLK);
    pin_digitalOut(ENABLE);
    pin_digitalOut(RESET);
    pin_digitalOut(CW);

    pin_write(CLK,0);
    pin_write(ENABLE,0);
    pin_write(RESET,0);
    pin_write(CW,0);
    clk_out = 0;
    current_pos = 0;
}

void startup(void){
    //make sure we follow the correct startup routine
    // if (startup_counter < 4)
    // {
    //     pin_write(ENABLE,0);
    //     pin_write(RESET,0);
    // }
    // else if (startup_counter >= 4 && startup_counter <= 8)
    // {
    //     pin_write(ENABLE,0);
    //     pin_write(RESET,1);
    // } else {
        pin_write(ENABLE,1);
        pin_write(RESET,1);
    // }
}


int16_t main(void) {
    init();
    while (USB_USWSTAT!=CONFIG_STATE) {     // while the peripheral is not configured...
        ServiceUSB();                       // ...service USB requests
    }
    while (1) {
        ServiceUSB();
         if (timer_flag(HB_TIMER)) {
            timer_lower(HB_TIMER);
            led_toggle(&led1);
        }
        // if (timer_flag(STARTUP_TIMER)) {
        //     timer_lower(STARTUP_TIMER);
        //     ++startup_counter;
        // }
        if (timer_flag(CLK_TIMER)) {
            timer_lower(CLK_TIMER);
            if (desired_pos != current_pos)
            {
                if (desired_pos > current_pos)
                {
                    pin_write(CW,1); //forwards
                    clk_out = !clk_out;
                    pin_write(CLK,clk_out);
                    ++current_pos;
                } else {
                    pin_write(CW,0); //backwards
                    clk_out = !clk_out;
                    pin_write(CLK,clk_out);
                    --current_pos;
                }

            }
            
            if (clk_out)
            {
                led_on(&led2);
            }
            else{
                led_off(&led2);   
            }
        }
        startup();
    }
}

