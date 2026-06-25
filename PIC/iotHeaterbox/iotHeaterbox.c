#pragma config OSC = HS
#pragma config WDT = OFF
#pragma config PWRT = ON
#pragma config BOREN = BOHW
#pragma config LVP = OFF
#pragma config PBADEN = OFF

#include <xc.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "iotHeaterbox.h"

#define _XTAL_FREQ 20000000

#define HEATER LATCbits.LATC3
#define FAN    LATCbits.LATC2
#define LIGHT  LATCbits.LATC1
#define DISP_TX LATBbits.LATB4

#define LCD_ON_NO_CURSOR    22
#define LCD_BACKLIGHT_OFF   18
#define LCD_BACKLIGHT_ON    17
#define LCD_CLR             12
#define LCD_L1              128
#define LCD_L2              148
#define LCD_L3              168
#define LCD_L4              188
uint8_t lcd_line_addrs[] = {LCD_L1, LCD_L2, LCD_L3, LCD_L4};


#define ADDR_INIT  0xFF
#define ADDR_SP    0x00
#define ADDR_FAN   0x10
#define ADDR_KP    0x20
#define ADDR_KI    0x24
#define AD

volatile float t_h = 0.0;
volatile float t_b = 0.0;
volatile uint8_t adc_ready = 0;
volatile uint8_t isr_channel = 0;
volatile uint8_t box_setpoint = 45.0;
volatile float heater_target = 0.0;
volatile uint16_t adc_val[2];
volatile uint8_t flag_10hz = 0;
volatile uint8_t wifi_ticks = 0;

volatile uint8_t menu_state = 0; 
volatile uint8_t sub_menu_idx = 0; 
uint8_t last_menu_state = 0xFF;

volatile uint8_t menu_press = 0, up_press = 0, down_press = 0, select_press = 0;
volatile uint8_t btn_menu = 0, btn_up = 0, btn_down = 0, btn_select = 0;
volatile uint8_t btn_menu_state = 0, btn_up_state = 0, btn_down_state = 0, btn_select_state = 0;

#define not_pressed 0
#define was_pressed 1
#define being_held  3

#define main_menu 0
#define setpoint_menu 2
#define wifi_menu 3
#define manual_menu 1
#define pid_menu 4

volatile uint8_t *btn_pins[] = {&btn_menu, &btn_up, &btn_down, &btn_select};
volatile uint8_t *btn_states[] = {&btn_menu_state, &btn_up_state, &btn_down_state, &btn_select_state};
volatile uint8_t *btn_flags[] = {&menu_press, &up_press, &down_press, &select_press};
volatile uint8_t time_to_send = 0;

uint8_t lcd_lines[] = {LCD_L1, LCD_L2, LCD_L3, LCD_L4};
uint8_t current_line_idx = 0;
char display_buffer[4][21]; // 4 lines, 20 chars + null terminator

float Kp = 10.0, Ki = 0.5, Kd = 0.1;
uint8_t fan_mode = 0;
uint8_t control_active = 1;


void soft_putch(char data) {
    uint8_t gie_backup = INTCONbits.GIE;
    INTCONbits.GIE = 0;
    DISP_TX = 0; __delay_us(100);
    for(int i=0; i<8; i++) { DISP_TX = (data >> i) & 0x01; __delay_us(100); }
    DISP_TX = 1; __delay_us(100);
    INTCONbits.GIE = gie_backup;
}

void lcd_cmd_direct(uint8_t cmd) {
    soft_putch(cmd);
    __delay_ms(2);
}

void lcd_cmd_with_prefix(uint8_t cmd) {
    soft_putch(255);
    soft_putch(cmd);
    __delay_ms(2);
}

void lcd_move_cursor(uint8_t address) {
    // Just send the address command directly to the display
    soft_putch(address); 
    __delay_ms(2);
}

void lcd_write(const char *str) { while(*str) soft_putch(*str++); }

void poll_buttons(void) {
    btn_menu = (PORTBbits.RB3 == 0);
    btn_up = (PORTBbits.RB1 == 0);
    btn_down = (PORTBbits.RB4 == 0);
    btn_select = (PORTBbits.RB0 == 0);
    
    // Loop through all 4 buttons
    for (uint8_t i = 0; i < 4; i++) {
        if (*btn_pins[i]) {
            switch (*btn_states[i]) {
            case not_pressed: // button was not pressed previously
                *btn_states[i] = was_pressed;
                break;
            case was_pressed: // button was pressed previously
                *btn_states[i] = being_held;
                break;
            case being_held:
                break;
            default:
                break;
            }
        } else {
            switch (*btn_states[i]) {
            case not_pressed: // button was not pressed previously
                break;
            case being_held:
                *btn_flags[i] = 1; // flag to deal with button pressed
                *btn_states[i] = not_pressed;
                break;
            }
        }
    }
}

void DATA_EE_Write(uint8_t addr, uint8_t data) {
    EEADR = addr; EEDATA = data;
    EECON1bits.EEPGD = 0; EECON1bits.CFGS = 0; EECON1bits.WREN = 1;
    INTCONbits.GIE = 0; EECON2 = 0x55; EECON2 = 0xAA; EECON1bits.WR = 1;
    INTCONbits.GIE = 1; while(EECON1bits.WR) { ; } EECON1bits.WREN = 0;
}

uint8_t DATA_EE_Read(uint8_t addr) {
    EEADR = addr; EECON1bits.EEPGD = 0; EECON1bits.CFGS = 0;
    EECON1bits.RD = 1; return EEDATA;
}

void eeprom_write_f(uint8_t addr, float val) {
    uint8_t *p = (uint8_t *)&val;
    for(uint8_t i=0; i<4; i++) DATA_EE_Write(addr+i, p[i]);
}

float eeprom_read_f(uint8_t addr) {
    float val; uint8_t *p = (uint8_t *)&val;
    for(uint8_t i=0; i<4; i++) p[i] = DATA_EE_Read(addr+i);
    return val;
}

void __interrupt() ISR(void) { 
    if (PIR1bits.ADIF) {
        adc_val[isr_channel] = (uint16_t)((uint16_t)ADRESH << 8) | ADRESL;
        PIR1bits.ADIF = 0;
        
        adc_ready = 1; 

        // Switch channel and restart
        isr_channel = (isr_channel == 0) ? 1 : 0;
        
        ADCON0bits.CHS = (isr_channel == 0) ? 0 : 2;
        
        ADCON0bits.GO = 1;
    }
    if (INTCONbits.TMR0IF) {
        TMR0H = 0x3C; TMR0L = 0xAF;
        flag_10hz = 1;
        INTCONbits.TMR0IF = 0;
    }
    if (PIR1bits.TMR1IF) {
        static uint16_t timer1_counter = 0;
        timer1_counter++;
        if (timer1_counter >= 572) {
            time_to_send = 1;
            timer1_counter = 0;
        }
    
        PIR1bits.TMR1IF = 0;
    }
}

float calc_celsius(uint16_t adc, float fixed_res, uint8_t type) {
    if (adc <= 5) adc = 5; if (adc >= 1020) adc = 1020;
    float resistance = (type == 0) ? (fixed_res * ((float)adc / (1023.0 - (float)adc))) : (fixed_res * ((1023.0 - (float)adc) / (float)adc));
    return (1.0 / (1.0 / 298.15 + (1.0 / 4350.0) * log(resistance / 100000.0))) - 273.15;
}

void handle_buttons(void){
    if (menu_press){
        menu_press=0;
        menu_state++;
        if (menu_state > pid_menu) {
            menu_state = main_menu;
        }
        lcd_cmd_direct(LCD_CLR);
        
        switch (menu_state){
            case main_menu:
                sprintf(display_buffer[0], "Main Menu");
                sprintf(display_buffer[1], "H:%5.1f B:%5.1f", t_h, t_b);
                sprintf(display_buffer[2], "F:%d L:%d H:%d", FAN, LIGHT, HEATER);

                break;
            case setpoint_menu:
                sprintf(display_buffer[0], "Setpoint Menu");
                sprintf(display_buffer[1], "H:%5.1f B:%5.1f", t_h, t_b);
                sprintf(display_buffer[2], "F:%d L:%d H:%d", FAN, LIGHT, HEATER);
                break;
            case wifi_menu:
                sprintf(display_buffer[0], "WIFI Menu");
                sprintf(display_buffer[1], "H:%5.1f B:%5.1f", t_h, t_b);
                sprintf(display_buffer[2], "F:%d L:%d H:%d", FAN, LIGHT, HEATER);
                break;
            case manual_menu:
                sprintf(display_buffer[0], "Manual Menu");
                sprintf(display_buffer[1], "H:%5.1f B:%5.1f", t_h, t_b);
                sprintf(display_buffer[2], "F:%d L:%d H:%d", FAN, LIGHT, HEATER);
                break;
            case pid_menu:
                sprintf(display_buffer[0], "PID Menu");
                sprintf(display_buffer[1], "H:%5.1f B:%5.1f", t_h, t_b);
                sprintf(display_buffer[2], "F:%d L:%d H:%d", FAN, LIGHT, HEATER);
                break;
            default:
                sprintf(display_buffer[0], "default Menu");
                sprintf(display_buffer[1], "H:%5.1f B:%5.1f", t_h, t_b);
                sprintf(display_buffer[2], "F:%d L:%d H:%d", FAN, LIGHT, HEATER);
                break;
        }
    }
    
    if (up_press){
        up_press=0;

    }
    if (down_press){
        down_press=0;

    }
    if (select_press){
        select_press=0;

}
}

void control_outputs(void){
    FAN = 1;
    LIGHT = 1;
    if (t_b<= box_setpoint){
        HEATER = 1;
    }
    else {
        HEATER = 0;
    }
}

void send_to_esp(void) {
    
}

void send_to_display(void) {
    
    uint8_t old_gie = INTCONbits.GIE; //save state of interrupts
    INTCONbits.GIE = 0;// stop all interrupts

    lcd_move_cursor(lcd_line_addrs[current_line_idx]);
    lcd_write(display_buffer[current_line_idx]); 

    current_line_idx++;
    if (current_line_idx >= 4) {
        current_line_idx = 0;
    } 

    INTCONbits.GIE = old_gie;// restore interrupts
}

void main(void) {
  // Configuration
    ECANCON = 0x00;
    CANCON = 0x20;
    LATC = 0x00; TRISC = 0x00; TRISA = 0x05; TRISB = 0x0F;
    ADCON1 = 0x0D; ADCON2 = 0x92; ADCON0bits.ADON = 1;
    
    // A/D channel 0 (Heater Temp)
    ADCON0bits.CHS = 0;
    ADCON0bits.GO = 1;
    while(ADCON0bits.GO); // Wait for conversion to complete
    adc_val[0] = (uint16_t)((uint16_t)ADRESH << 8) | ADRESL;
    
    // A/D channel 2 (Box Temp)
    ADCON0bits.CHS = 2;
    ADCON0bits.GO = 1;
    while(ADCON0bits.GO); // Wait for conversion to complete
    adc_val[1] = (uint16_t)((uint16_t)ADRESH << 8) | ADRESL;
    
    // Calculate initial temperatures
    t_h = calc_celsius(adc_val[0], 698.0, 1);
    t_b = calc_celsius(adc_val[1], 50300.0, 0);
    
    lcd_cmd_direct(LCD_ON_NO_CURSOR);
    lcd_cmd_direct(LCD_BACKLIGHT_ON);
    lcd_cmd_direct(LCD_CLR);
    
    // Initial buffer setup
    sprintf(display_buffer[0], "*Heater Box Control*");
    sprintf(display_buffer[1], "   by Dan Jubenville");
    sprintf(display_buffer[2], "June 2026 recovering");
    sprintf(display_buffer[3], "from big toe surgery");
    
    lcd_move_cursor(lcd_line_addrs[0]);
    lcd_write(display_buffer[0]);
    lcd_move_cursor(lcd_line_addrs[1]);
    lcd_write(display_buffer[1]);
    lcd_move_cursor(lcd_line_addrs[2]);
    lcd_write(display_buffer[2]);
    lcd_move_cursor(lcd_line_addrs[3]);
    lcd_write(display_buffer[3]);
    
          // Splash screen (standard delay allowed here as no other tasks are running)
    __delay_ms(2000);
    lcd_cmd_direct(LCD_CLR);

    // Start interrupts for main loop
    T0CON = 0x84; INTCONbits.TMR0IE = 1; PIE1bits.ADIE = 1; INTCONbits.GIE = 1; INTCONbits.PEIE = 1;
    T1CON = 0x30; TMR1H = 0; TMR1L = 0; PIE1bits.TMR1IE = 1; T1CONbits.TMR1ON = 1;
    
    ADCON0bits.CHS = 0; 
    ADCON0bits.GO = 1;
    
    sprintf(display_buffer[0], "Main Menu");
    sprintf(display_buffer[1], "H:%5.1f B:%5.1f", t_h, t_b);
    sprintf(display_buffer[2], "F:%d L:%d H:%d", FAN, LIGHT, HEATER);
    sprintf(display_buffer[3], "");

    while(1) {
        
        if (flag_10hz) {
            flag_10hz = 0;
            
            if (adc_ready) {
                adc_ready = 0;
                if (!isr_channel) {
                    t_h = calc_celsius(adc_val[0], 698.0, 1);    
                }
                else {
                    t_b = calc_celsius(adc_val[1], 50300.0, 0);
                }
            }
            sprintf(display_buffer[1], "H:%5.1f B:%5.1f", t_h, t_b);
            sprintf(display_buffer[2], "F:%d L:%d H:%d SP:%3d", FAN, LIGHT, HEATER, box_setpoint);
            
            send_to_display();
            
            poll_buttons();
            
            handle_buttons();
            
            control_outputs();
            
            if(time_to_send){
                send_to_esp();
            }
            

        }
    }
}