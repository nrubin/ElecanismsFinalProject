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

// #define HB_TIMER        &timer1
#define CLK_TIMER       &timer2
#define CONTROL_TIMER   &timer3
#define MEASURE_TIMER   &timer4
#define STOP_TIMER      &timer1
#define POT         &A[0]
#define M1_CW       &D[0]
#define M1_RESET    &D[1] 
#define M1_ENABLE   &D[2]
#define M1_CLK      &D[3]
#define M2_CLK      &D[4]
#define M2_ENABLE   &D[5]
#define M2_RESET    &D[6] 
#define M2_CW       &D[7]
#define LIMIT1      &D[8]
#define LIMIT2      &D[9]

#define GOTO_POS    0   // Vendor request that receives 2 unsigned integer values
#define SET_0       1   // Vendor request that returns  2 unsigned integer values
#define GET_POS     2   // Vender request that clears the rev counter

#define DEBOUNCE    1000
#define ARR_SIZE    10
#define BAD_VAL     5000
#define INTERVAL    5000

uint16_t val2, clk_out, m1_dpos, m2_dpos, m1_cpos, m2_cpos, m1_dir, m2_dir, m1_clk, m2_clk, limit1_counter,limit2_counter, limit1_prev, limit2_prev;
uint16_t pot_val, prev_pot_val, new_pot_val;

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

    // timer_setPeriod(HB_TIMER, 0.3);
    // timer_start(HB_TIMER);

    timer_setPeriod(CLK_TIMER, 0.0022); 
    timer_start(CLK_TIMER);

    timer_setPeriod(MEASURE_TIMER, 0.0001); 
    timer_start(MEASURE_TIMER);

    timer_setPeriod(CONTROL_TIMER, 0.001); 
    timer_start(CONTROL_TIMER);

    timer_setPeriod(STOP_TIMER, 0.000001); 
    timer_start(STOP_TIMER);

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

    m1_clk = 0;
    m2_clk = 0;
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
    /*
    n - ( > n) -> 65536
    65536-n + ()
    */
    if ( (actual - desired) <= interval || (desired - actual) <= interval )
    {
        return 1;
    } else {
        return 0;
    }
}

void move_m1(void){
    if (!within_interval(m1_cpos, m1_dpos, INTERVAL))
    {
        pin_write(M1_RESET, 1);
        pin_write(M1_ENABLE, 1);
        if (m1_dpos > m1_cpos)
        {
            m1_dir = 1; //forward
            ++m1_cpos;
        } else {
            m1_dir = 0; //backward
            --m1_cpos;
        }
        pin_write(M1_CW, m1_dir);
    } else {
        m1_dpos = m1_cpos;
        pin_write(M1_ENABLE, 0);
        pin_write(M1_RESET, 0);
        pin_write(M1_CLK, 0);
    }
}

void move_m2(void){
    if (!within_interval(m2_cpos, m2_dpos, INTERVAL))
    {
        pin_write(M2_RESET, 1);
        pin_write(M2_ENABLE, 1);
        if (m2_dpos > m2_cpos)
        {
            m2_dir = 1; //forward
            ++m2_cpos;
        } else {
            m2_dir = 0; //backward
            --m2_cpos;
        }
        pin_write(M2_CW, m2_dir);
    } else {
        m2_dpos = m2_cpos;
        pin_write(M2_ENABLE, 0);
        pin_write(M2_RESET, 0);
        pin_write(M2_CLK, 0);
    }
}

void MEASURE_LOOP(void){
    /*Should read the pot val and update the running average
    (must filter)
    */
    
    pot_val = pin_read(POT);
    
    if ( abs(pot_val - prev_pot_val) > BAD_VAL )
    {
        new_pot_val = prev_pot_val;
    } else {
        new_pot_val = .25*pot_val + .75*prev_pot_val;            
        prev_pot_val = new_pot_val;
    }
    m1_dpos = new_pot_val;
    m2_dpos = new_pot_val;
}

void CONTROL_LOOP(void){
    /*
    Steps in controlling the steppers:
    read the pot value running average
    convert the pot value to a stepper setpoint (0 - 65536)
    check if the stepper is within an acceptable value of the set point
    */
    move_m1();
    move_m2();
}

void STOP_LOOP(void){
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
            pin_write(M1_ENABLE, 0);
            pin_write(M1_RESET, 0);
            pin_write(M1_CLK, 0);
            limit1_counter = 0;
        }

        if (limit2_counter > DEBOUNCE)
        {
            m2_dpos = m2_cpos;
            pin_write(M2_ENABLE, 0);
            pin_write(M2_RESET, 0);
            pin_write(M2_CLK, 0);
             limit2_counter = 0;
        }
}

void SIGNAL_LOOP(void){
    clk_out = !clk_out;
    pin_write(M1_CLK,clk_out);
    pin_write(M2_CLK,clk_out);
}

int16_t main(void) {

    init();
    startup();
    printf("Hello World!\n");

    m1_dpos = 0;
    m2_dpos = 0;
    m1_cpos = 0;
    m2_cpos = 0;

    // while (USB_USWSTAT!=CONFIG_STATE) {     // while the peripheral is not configured...
    //     ServiceUSB();                       // ...service USB requests
    // }

    while (1) {

        // ServiceUSB();
         // if (timer_flag(HB_TIMER)) {
         //    timer_lower(HB_TIMER);
         //    led_toggle(&led1);
         //    // printf("pot = %u, new_pot = %u\n", pot_val, new_pot_val);
         //    // printf("pot = %i, range1 = %i, range2 = %i, prev = %i\n", pot_val, pot_val+BAD_VAL, pot_val-BAD_VAL, prev_pot_val);
         //    printf("pot = %u, new = %u, prev = %u, desired = %u, current = %u, delta = %u\n", pot_val, new_pot_val, prev_pot_val, m1_dpos, m1_cpos, m1_dpos-m1_cpos);
        // }

        if (timer_flag(MEASURE_TIMER)) {
            timer_lower(MEASURE_TIMER);
            MEASURE_LOOP();
        }

        if (timer_flag(CONTROL_TIMER)) {
            timer_lower(CONTROL_TIMER);
            CONTROL_LOOP();
        }

        // if (timer_flag(STOP_TIMER)) {
        //     timer_lower(STOP_TIMER);
        //     STOP_LOOP();
        // }
       
        if (timer_flag(CLK_TIMER)) {
            timer_lower(CLK_TIMER);
            SIGNAL_LOOP();
        }
    }
}

