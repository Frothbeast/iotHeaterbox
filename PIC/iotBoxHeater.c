#include <xc.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <iotBoxHeater.h>

#define _XTAL_FREQ 20000000 

// Pin Definitions
#define HEATER LATCbits.LATC3
#define FAN    LATCbits.LATC2
#define LIGHT  LATCbits.LATC1
#define SELECT_PIN PORTBbits.RB3
#define DISP_TX LATBbits.LATB4

// Parallax 27979 Control
#define LCD_CLR      12
#define LCD_L1       128
#define LCD_L2       148
#define LCD_L3       168
#define LCD_L4       188
#define LCD_CUR_ON   13
#define LCD_CUR_OFF  14

// EEPROM Addresses
#define ADDR_INIT  0xFF
#define ADDR_SP    0x00
#define ADDR_FAN   0x10

// Logic Globals
volatile float box_setpoint = 25.0;
volatile float heater_target = 0.0;
volatile uint16_t adc_val[2];
volatile uint8_t channel = 0;
volatile uint8_t flag_10hz = 0;
volatile uint8_t timer_ticks = 0;
volatile uint8_t wifi_ticks = 0;
volatile uint8_t menu_state = 0; 
volatile uint8_t edit_mode = 0;

// Button Flags
volatile uint8_t btn_menu = 0;
volatile uint8_t btn_up = 0;
volatile uint8_t btn_down = 0;
volatile uint8_t btn_select = 0;

// PID
float Kp = 10.0, Ki = 0.5, Kd = 0.1;
float inner_integral = 0, inner_last_error = 0;
uint8_t duty_cycle = 0;
uint8_t fan_mode = 0; 

// Trend
float trend_buffer[60];
uint8_t trend_idx = 0;
float current_trend = 0.0;

// Wifi Buffers
char wifi_tx_buf[24]; // Increased for hex payload
char wifi_rx_buf[21];
uint8_t current_rssi = 0; // New global for RSSI storage

// Software Serial for Display on B4
void soft_putch(char data) {
    DISP_TX = 0; 
    __delay_us(104); 
    for(int i=0; i<8; i++) {
        DISP_TX = (data >> i) & 0x01;
        __delay_us(104);
    }
    DISP_TX = 1; 
    __delay_us(104);
}

void lcd_cmd(uint8_t cmd) { soft_putch(cmd); __delay_ms(2); }

void lcd_print(const char* str) {
    while(*str) soft_putch(*str++);
}

// EEPROM Functions
void DATA_EE_Write(uint8_t addr, uint8_t data) {
    EEADR = addr; EEDATA = data;
    EECON1bits.EEPGD = 0; EECON1bits.CFGS = 0; EECON1bits.WREN = 1;
    INTCONbits.GIE = 0; EECON2 = 0x55; EECON2 = 0xAA; EECON1bits.WR = 1;
    INTCONbits.GIE = 1; while(EECON1bits.WR); EECON1bits.WREN = 0;
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

void esp_send_cmd(const char* cmd) {
    while(*cmd) {
        while(!TXSTAbits.TRMT);
        TXREG = *cmd++;
    }
    while(!TXSTAbits.TRMT);
    TXREG = '\r';
    while(!TXSTAbits.TRMT);
    TXREG = '\n';
}

void __interrupt() ISR(void) {
    if (PIR1bits.ADIF) {
        adc_val[channel] = (uint16_t)((ADRESH << 8) | ADRESL);
        PIR1bits.ADIF = 0;
        if (channel == 0) { channel = 1; ADCON0bits.CHS = 2; ADCON0bits.GO = 1; }
        else { channel = 0; ADCON0bits.CHS = 0; }
    }
    if (INTCONbits.TMR0IF) {
        TMR0H = 0x3C; TMR0L = 0xAF; // 20MHz / 10Hz
        flag_10hz = 1; timer_ticks++; wifi_ticks++;
        INTCONbits.TMR0IF = 0;
    }
    if (INTCONbits.INT0IF) { btn_menu = 1; INTCONbits.INT0IF = 0; }
    if (INTCON3bits.INT1IF) { btn_up = 1; INTCON3bits.INT1IF = 0; }
    if (INTCON3bits.INT2IF) { btn_down = 1; INTCON3bits.INT2IF = 0; }
    if (INTCONbits.RBIF) {
        uint8_t dummy = PORTB;
        if (SELECT_PIN == 0) { btn_select = 1; } 
        INTCONbits.RBIF = 0;
    }
}

float calc_celsius(uint16_t adc, float pull_down_res) {
    if (adc == 0 || adc >= 1023) return 0;
    float resistance = pull_down_res * ((1023.0 / (float)adc) - 1.0);
    float temp = 1.0 / (1.0 / 298.15 + (1.0 / 4350.0) * log(resistance / 100000.0));
    return temp - 273.15;
}

void main(void) {
    TRISA = 0x05; TRISB = 0x0F; TRISC = 0x80;
    LATBbits.LATB4 = 1; TRISBbits.TRISB4 = 0; 
    ADCON1 = 0x0C; ADCON2 = 0x92; PIE1bits.ADIE = 1; ADCON0bits.ADON = 1;
    INTCON2bits.RBPU = 0; INTCONbits.INT0IE = 1; INTCON3bits.INT1IE = 1; 
    INTCON3bits.INT2IE = 1; INTCONbits.RBIE = 1;
    
    T0CON = 0x83; INTCONbits.TMR0IE = 1;
    SPBRG = 129; // 20MHz / 9600 Baud
    TXSTA = 0x24; RCSTA = 0x90; 
    
    if(DATA_EE_Read(ADDR_INIT) == 0xA5) {
        box_setpoint = eeprom_read_f(ADDR_SP);
        fan_mode = DATA_EE_Read(ADDR_FAN);
    } else {
        eeprom_write_f(ADDR_SP, box_setpoint);
        DATA_EE_Write(ADDR_FAN, fan_mode);
        DATA_EE_Write(ADDR_INIT, 0xA5);
    }

    INTCONbits.GIE = 1; INTCONbits.PEIE = 1;
    __delay_ms(1000); lcd_cmd(LCD_CLR);

    uint8_t soft_pwm_cnt = 0;

    while(1) {
        if (btn_menu) {
            eeprom_write_f(ADDR_SP, box_setpoint);
            DATA_EE_Write(ADDR_FAN, fan_mode);
            menu_state = (menu_state + 1) % 4; 
            edit_mode = 0;
            lcd_cmd(LCD_CLR); lcd_cmd(LCD_L1);
            if (menu_state == 0) lcd_print("Main Menu");
            else if (menu_state == 1) lcd_print("Setpoint Menu");
            else if (menu_state == 2) lcd_print("Manual Override");
            else if (menu_state == 3) lcd_print("Wifi Test Menu");
            __delay_ms(1000); lcd_cmd(LCD_CLR);
            btn_menu = 0;
        }

        if (flag_10hz) {
            ADCON0bits.GO = 1;
            float t_heater = calc_celsius(adc_val[0], 698.0);
            float t_box    = calc_celsius(adc_val[1], 50300.0);

            if (btn_select) { edit_mode = !edit_mode; btn_select = 0; }
            if (btn_up) {
                if (edit_mode) {
                    if (menu_state == 1) box_setpoint += 1.0;
                    if (menu_state == 2) fan_mode = (fan_mode + 1) % 3;
                }
                btn_up = 0;
            }
            if (btn_down) {
                if (edit_mode) {
                    if (menu_state == 1) box_setpoint -= 1.0;
                    if (menu_state == 2) fan_mode = (fan_mode == 0) ? 2 : fan_mode - 1;
                }
                btn_down = 0;
            }

            // PID Logic
            float box_error = box_setpoint - t_box;
            heater_target = box_error * 5.0;
            if (heater_target > 150.0) heater_target = 150.0;
            if (heater_target < 0.0) heater_target = 0.0;

            float inner_error = heater_target - t_heater;
            inner_integral += inner_error;
            float output = (Kp * inner_error) + (Ki * inner_integral) + (Kd * (inner_error - inner_last_error));
            if (output > 100) output = 100; if (output < 0) output = 0;
            duty_cycle = (uint8_t)output;
            inner_last_error = inner_error;

            if (fan_mode == 0) FAN = (duty_cycle > 0);
            else if (fan_mode == 1) FAN = 1;
            else FAN = 0;

            if (timer_ticks >= 10) {
                current_trend = (t_box - trend_buffer[trend_idx]) * 60.0;
                trend_buffer[trend_idx] = t_box;
                trend_idx = (trend_idx + 1) % 60;
                timer_ticks = 0;
            }

            // NEW: Send 12-character hex payload every 10 seconds
            if (wifi_ticks >= 100) {
                uint8_t status_mask = (HEATER) | (FAN << 1) | (LIGHT << 2);
                // Payload: BoxTemp(4 hex) + HeatTemp(4 hex) + Status(2 hex) + RSSI(2 hex) = 12 total
                sprintf(wifi_tx_buf, "%04X%04X%02X%02X", 
                        (int)(t_box * 10), 
                        (int)(t_heater * 10), 
                        status_mask, 
                        current_rssi);
                esp_send_cmd(wifi_tx_buf);
                wifi_ticks = 0;
            }

            if (PIR1bits.RCIF) {
                uint8_t r_idx = 0;
                while(PIR1bits.RCIF && r_idx < 20) {
                    char c = RCREG;
                    wifi_rx_buf[r_idx++] = c;
                    // Logic to extract RSSI from ESP response would go here
                }
                wifi_rx_buf[r_idx] = '\0';
            }

            if (!edit_mode) lcd_cmd(LCD_CUR_OFF); else lcd_cmd(LCD_CUR_ON);

            if (menu_state == 3) {
                lcd_cmd(LCD_L1);
                char line1[21]; sprintf(line1, "TX: %-16s", wifi_tx_buf); lcd_print(line1);
                lcd_cmd(LCD_L4);
                char line4[21]; sprintf(line4, "RSSI: -%u dBm   ", current_rssi); lcd_print(line4);
            } else {
                lcd_cmd(LCD_L1);
                if (menu_state == 0) lcd_print("Temp Control    ");
                else if (menu_state == 1) lcd_print("Setpoint Menu   ");
                else lcd_print("Manual Override ");

                lcd_cmd(LCD_L2); 
                char line2[21]; sprintf(line2, "Heater: %3.0f C", t_heater); lcd_print(line2);

                lcd_cmd(LCD_L3);
                if (menu_state == 1) {
                    char line3[21]; sprintf(line3, "Set Box: %3.0f C", box_setpoint); lcd_print(line3);
                } else {
                    char line3[21]; sprintf(line3, "Box:     %3.0f C", t_box); lcd_print(line3);
                }

                lcd_cmd(LCD_L4);
                if (menu_state == 2) {
                    char line4[21]; sprintf(line4, "Fan: %-10s", (fan_mode==0?"AUTO":(fan_mode==1?"ON":"OFF"))); lcd_print(line4);
                } else {
                    char line4[21]; sprintf(line4, "Trend: %c%4.1f C/m", (current_trend>=0.1?'/':'\\'), fabs(current_trend)); lcd_print(line4);
                }
            }
            flag_10hz = 0;
        }

        if (soft_pwm_cnt < duty_cycle) HEATER = 1; else HEATER = 0;
        soft_pwm_cnt++; if (soft_pwm_cnt >= 100) soft_pwm_cnt = 0;
        __delay_us(10); 
    }
}
