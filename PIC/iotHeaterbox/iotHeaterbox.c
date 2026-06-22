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
#define SELECT_PIN PORTBbits.RB3
#define DISP_TX LATBbits.LATB4

#define LCD_CLR      12
#define LCD_L1       128
#define LCD_L2       148
#define LCD_L3       168
#define LCD_L4       188
#define LCD_CUR_ON   13
#define LCD_CUR_OFF  14

#define ADDR_INIT  0xFF
#define ADDR_SP    0x00
#define ADDR_FAN   0x10

volatile float box_setpoint = 25.0;
volatile float heater_target = 0.0;
volatile uint16_t adc_val[2];
volatile uint8_t channel = 0;
volatile uint8_t flag_10hz = 0;
volatile uint8_t wifi_ticks = 0;
volatile uint8_t menu_state = 0;
volatile uint8_t edit_mode = 0;

volatile uint8_t btn_menu = 0;
volatile uint8_t btn_up = 0;
volatile uint8_t btn_down = 0;
volatile uint8_t btn_select = 0;

float Kp = 10.0, Ki = 0.5, Kd = 0.1;
float inner_integral = 0, inner_last_error = 0;
uint8_t duty_cycle = 0;
uint8_t fan_mode = 0;

float trend_buffer[60];
uint8_t trend_idx = 0;
float current_trend = 0.0;

char vram[4][20];
char physical_screen[4][20];
uint8_t disp_row = 0;
uint8_t disp_col = 0;
uint8_t prev_edit_mode = 0xFF;

void soft_putch(char data) {
    uint8_t gie_backup = INTCONbits.GIE;
    INTCONbits.GIE = 0;
    DISP_TX = 0; __delay_us(100);
    for(int i=0; i<8; i++) { DISP_TX = (data >> i) & 0x01; __delay_us(100); }
    DISP_TX = 1; __delay_us(100);
    INTCONbits.GIE = gie_backup;
}

void lcd_cmd(uint8_t cmd) { soft_putch(cmd); __delay_ms(2); }
void lcd_write(const char *str) { while(*str) soft_putch(*str++); }

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
        adc_val[channel] = (uint16_t)((ADRESH << 8) | ADRESL);
        PIR1bits.ADIF = 0;
        if (channel == 0) { channel = 1; ADCON0bits.CHS = 2; ADCON0bits.GO = 1; }
        else { channel = 0; ADCON0bits.CHS = 0; }
    }
    if (INTCONbits.TMR0IF) {
        TMR0H = 0x3C; TMR0L = 0xAF;
        flag_10hz = 1; wifi_ticks++;
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
float calc_celsius(uint16_t adc, float fixed_res, uint8_t type) {
    if (adc <= 5) adc = 5;
    if (adc >= 1020) adc = 1020;
    
    float resistance;
    if (type == 0) {
        // Box: Pull-up to VCC, NTC to GND
        // If this moves the wrong way, we invert the ADC ratio
        resistance = fixed_res * ((float)adc / (1023.0 - (float)adc));
    } else {
        // Heater: Pull-down to GND, NTC to VCC
        resistance = fixed_res * ((1023.0 - (float)adc) / (float)adc);
    }
    
    float temp = 1.0 / (1.0 / 298.15 + (1.0 / 4350.0) * log(resistance / 100000.0));
    return temp - 273.15;
}

void main(void) {
    LATC = 0x00; TRISC = 0x00; TRISB = 0x0F; TRISA = 0x05;
    ADCON1 = 0x0D; ADCON2 = 0x92; ADCON0bits.ADON = 1;
    TRISBbits.TRISB4 = 0; TRISCbits.TRISC3 = 0; 
    __delay_ms(1500);
    lcd_cmd(LCD_CLR);
    lcd_write("IOT HEATER SYSTEM");
    lcd_cmd(LCD_L2);
    lcd_write("V1.0 INITIALIZING");
    __delay_ms(2000);
    lcd_cmd(LCD_CLR); 
    __delay_ms(1500);
    lcd_cmd(17); lcd_cmd(LCD_CLR);
    
   
    if(DATA_EE_Read(ADDR_INIT) == 0xA5) {
        box_setpoint = eeprom_read_f(ADDR_SP);
        fan_mode = DATA_EE_Read(ADDR_FAN);
    } else {
        eeprom_write_f(ADDR_SP, box_setpoint);
        DATA_EE_Write(ADDR_FAN, fan_mode);
        DATA_EE_Write(ADDR_INIT, 0xA5);
    }
    
    memset(vram, ' ', sizeof(vram));
    memset(physical_screen, 0x00, sizeof(physical_screen));
    
    T0CON = 0x84; INTCONbits.TMR0IE = 1; PIE1bits.ADIE = 1;
    INTCONbits.INT0IE = 1; INTCON3bits.INT1IE = 1; INTCON3bits.INT2IE = 1;
    INTCONbits.RBIE = 1; INTCONbits.GIE = 1; INTCONbits.PEIE = 1;

    while(1) {
        if (btn_menu) {
            eeprom_write_f(ADDR_SP, box_setpoint);
            DATA_EE_Write(ADDR_FAN, fan_mode);
            menu_state = (menu_state + 1) % 4;
            btn_menu = 0;
        }

if (flag_10hz) {
            ADCON0bits.GO = 1;
            if (wifi_ticks >= 10) { 
                wifi_ticks = 0; 
                mock_rssi++;
                if(mock_rssi > -30) mock_rssi = -90;
            }

            float t_h = calc_celsius(adc_val[0], 698.0, 1);
            float t_b = calc_celsius(adc_val[1], 50300.0, 0); 

            if (btn_up) {
                if (menu_state == 0) { control_active = !control_active; }
                else if (menu_state == 1) { box_setpoint += 0.5; }
                else if (menu_state == 2) {
                    if (sub_menu_idx == 0) Kp += 0.1;
                    else if (sub_menu_idx == 1) Ki += 0.01;
                    else if (sub_menu_idx == 2) Kd += 0.01;
                }
                btn_up = 0;
            }

            if (btn_down) {
                if (menu_state == 0) { LIGHT = !LIGHT; }
                else if (menu_state == 1) { box_setpoint -= 0.5; }
                else if (menu_state == 2) {
                    if (sub_menu_idx == 0) Kp -= 0.1;
                    else if (sub_menu_idx == 1) Ki -= 0.01;
                    else if (sub_menu_idx == 2) Kd -= 0.01;
                }
                btn_down = 0;
            }

            if (btn_select) {
                if (menu_state == 0) { FAN = !FAN; }
                else if (menu_state == 1) { 
                    eeprom_write_f(ADDR_SP, box_setpoint); 
                }
                else if (menu_state == 2) {
                    sub_menu_idx = (sub_menu_idx + 1) % 4;
                    if (sub_menu_idx == 3) {
                        eeprom_write_f(ADDR_KP, Kp);
                        eeprom_write_f(ADDR_KI, Ki);
                        eeprom_write_f(ADDR_KD, Kd);
                        sub_menu_idx = 0;
                    }
                }
                btn_select = 0;
            }

            char s[21];
            memset(vram, ' ', sizeof(vram));

            if (menu_state == 0) {
                sprintf(s, "SYS: %s  Htr:%s", control_active ? "ON " : "OFF", HEATER ? "ON " : "OFF"); memcpy(vram[0], s, strlen(s));
                sprintf(s, "Fan: %s  Lgt:%s", FAN ? "ON " : "OFF", LIGHT ? "ON " : "OFF"); memcpy(vram[1], s, strlen(s));
                sprintf(s, "Set: %4.1f Box:%4.1f", box_setpoint, t_b); memcpy(vram[2], s, strlen(s));
                sprintf(s, "Heater Temp: %4.1f", t_h); memcpy(vram[3], s, strlen(s));
            } 
            else if (menu_state == 1) {
                sprintf(s, "--- SETPOINT MENU ---"); memcpy(vram[0], s, strlen(s));
                sprintf(s, "Set: %4.1f C", box_setpoint); memcpy(vram[1], s, strlen(s));
                sprintf(s, "UP/DN to change"); memcpy(vram[2], s, strlen(s));
                sprintf(s, "SELECT to save"); memcpy(vram[3], s, strlen(s));
            } 
            else if (menu_state == 2) {
                sprintf(s, "--- PID CONFIG ---"); memcpy(vram[0], s, strlen(s));
                sprintf(s, "%cKp: %4.1f", (sub_menu_idx == 0) ? '>' : ' ', Kp); memcpy(vram[1], s, strlen(s));
                sprintf(s, "%cKi: %4.2f", (sub_menu_idx == 1) ? '>' : ' ', Ki); memcpy(vram[2], s, strlen(s));
                sprintf(s, "%cKd: %4.2f [SEL->SAVE]", (sub_menu_idx == 2) ? '>' : ' ', Kd); memcpy(vram[3], s, strlen(s));
            } 
            else if (menu_state == 3) {
                sprintf(s, "--- WIFI TUNING ---"); memcpy(vram[0], s, strlen(s));
                sprintf(s, "RSSI: %d dBm", mock_rssi); memcpy(vram[1], s, strlen(s));
                if (mock_rssi > -60) sprintf(s, "Signal: EXCELLENT");
                else if (mock_rssi > -75) sprintf(s, "Signal: GOOD");
                else sprintf(s, "Signal: POOR/ALIGN");
                memcpy(vram[2], s, strlen(s));
                sprintf(s, "Antenna Tuning Mode"); memcpy(vram[3], s, strlen(s));
            }

            flag_10hz = 0;
        }

        if (vram[disp_row][disp_col] != physical_screen[disp_row][disp_col]) {
            soft_putch(128 + (disp_row * 20) + disp_col);
            soft_putch(vram[disp_row][disp_col]);
            physical_screen[disp_row][disp_col] = vram[disp_row][disp_col];
            __delay_us(100);
        }
        disp_col++;
        if (disp_col >= 20) { disp_col = 0; disp_row++; if (disp_row >= 4) disp_row = 0; }
    }
}