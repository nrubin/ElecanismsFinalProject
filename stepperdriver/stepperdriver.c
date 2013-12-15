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
#define POT &A[0]
#define M1_CW &D[0]
#define M1_RESET &D[1] 
#define M1_ENABLE &D[2]
#define M1_CLK &D[3]
#define M2_CLK &D[4]
#define M2_ENABLE &D[5]
#define M2_RESET &D[6] 
#define M2_CW &D[7]
#define LIMIT1 &D[8]
#define LIMIT2 &D[9]
#define DEBOUNCE 200


uint16_t m1_dpos, m2_dpos, m1_cpos, m2_cpos, val2, clk_out, m1_dir, m2_dir, m1_clk, m2_clk, limit1_counter,limit2_counter, limit1_prev, limit2_prev, pot_val;


void VendorRequests(void) {
    WORD temp;

    switch (USB_setup.bRequest) {
        case GOTO_POS:
            m1_dpos = USB_setup.wValue.w;
            BD[EP0IN].bytecount = 0;    // set EP0 IN byte count to 0 
            BD[EP0IN].status = 0xC8;    // send packet as DATA1, set UOWN bit
            break;
        case SET_0:
            m1_cpos = 0;
            // DUTY = USB_setup.wValue.w;
            // REQUESTED_DIRECTION = USB_setup.wIndex.w;
            BD[EP0IN].bytecount = 0;    // set EP0 IN byte count to 0 
            BD[EP0IN].status = 0xC8;    // send packet as DATA1, set UOWN bit
            break;
        case GET_POS:
            temp.w = m1_cpos;
            BD[EP0IN].address[0] = temp.b[0];
            BD[EP0IN].address[1] = temp.b[1];
            temp.w = m1_dpos;
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

    pin_digitalOut(M1_ENABLE);
    pin_digitalOut(M1_CLK);
    pin_digitalOut(M1_RESET);
    pin_digitalOut(M1_CW);
    pin_digitalOut(M2_ENABLE);
    pin_digitalOut(M2_CLK);
    pin_digitalOut(M2_RESET);
    pin_digitalOut(M2_CW);
    
    pin_analogIn(POT);

    pin_write(M1_ENABLE,0);
    pin_write(M1_CLK,0);
    pin_write(M1_RESET,0);
    pin_write(M1_CW,0);
    pin_write(M2_ENABLE,0);
    pin_write(M2_CLK,0);
    pin_write(M2_RESET,0);
    pin_write(M2_CW,0);

    m1_clk= 0;
    m2_clk= 0;
    m1_cpos = 0;
    m1_dpos = 0;
}

void startup(void){
        pin_write(M1_ENABLE,1);
        pin_write(M1_RESET,1);
        pin_write(M2_ENABLE,1);
        pin_write(M2_RESET,1);
}

uint16_t within_interval(uint16_t actual, uint16_t desired, uint16_t interval){
    if ((actual > desired - interval) && (actual < desired + interval))
    {
        return 1;
    } else {
        return 0;
    }
}

void move_m1(void){
    // if (within_interval(m1_cpos, m1_dpos, 5))
    if (m1_cpos != m1_dpos)
    {
        if (m1_dpos > m1_cpos)
        {
            m1_dir = 1; //forward
            ++m1_cpos;
        } else {
            m1_dir = 0; //backward
            --m1_cpos;
        }
        m1_clk = !m1_clk;
    }
    pin_write(M1_CW, m1_dir);
    pin_write(M1_CLK,m1_clk);
    if (m1_clk)
    {
        led_on(&led2);
    } else {
        led_off(&led2);
    }
}

void move_m2(void){
    /* We want to be within some interval of the desired position,
    in order to avoid jitter. */
    // if (within_interval(m2_cpos, m2_dpos, 5))
    if (m2_cpos != m2_dpos)
    {
        if (m2_dpos > m2_cpos)
        {
            m2_dir = 1; //forward
            ++m2_cpos;
        } else {
            m2_dir = 0; //backward
            --m2_cpos;
        }
        m2_clk = !m2_clk;
    }
    pin_write(M2_CW, m2_dir);
    pin_write(M2_CLK,m2_clk);
    if (m2_clk)
    {
        led_on(&led3);
    } else {
        led_off(&led3);
    }
}


int16_t main(void) {
    init();
    printf("Hello World!\n");
    m1_dpos = 65000;
    m2_dpos = 65000;
    m1_cpos = 30000;
    m2_cpos = 30000;
    // while (USB_USWSTAT!=CONFIG_STATE) {     // while the peripheral is not configured...
    //     ServiceUSB();                       // ...service USB requests
    // }
    while (1) {
        m1_dpos = pin_read(POT);
        m2_dpos = pin_read(POT);

        // ServiceUSB();
         if (timer_flag(HB_TIMER)) {
            timer_lower(HB_TIMER);
            led_toggle(&led1);
        }

        if (pin_read(LIMIT1))
        {
            if (limit1_prev)
            {
                ++limit1_counter;
            }
            limit1_prev = 1;
        } else {
            limit1_counter = 0;
            limit1_prev = 0;
        }

        if (pin_read(LIMIT2))
        {
            if (limit2_prev)
            {
                ++limit2_counter;
            }
            limit2_prev = 1;
        } else {
            limit2_counter = 0;
            limit2_prev = 0;
        }

        if (limit1_counter > DEBOUNCE)
        {
            m1_dpos = m1_cpos;
            limit1_counter = 0;
        }

        if (limit2_counter > DEBOUNCE)
        {
            m2_dpos = m2_cpos;
            limit2_counter = 0;
        }
        if (timer_flag(CLK_TIMER)) {
            timer_lower(CLK_TIMER);
            move_m1();
            move_m2();
            printf("%u\n", m1_dpos);
        }
        startup();
    }
}

