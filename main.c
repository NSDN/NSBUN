#include "sys.h"

#define LED P47

#define RECV_EEPROM 1
#define RECV_FRAM   2

void __tim2_interrupt() __interrupt (INT_NO_TMR2) __using (1);
void __uart0_interrupt() __interrupt (INT_NO_UART0) __using (1);

uint8_t mode = RECV_EEPROM;
uint16_t addr, chksum, param;

#define ROM_BUFFER_SIZ 1024
uint8_t rom_buf[ROM_BUFFER_SIZ] = { 0 };

__xdata uint8_t __at (XDATA_XBUS_CS1) ROM_CHIP[XDATA_XCS1_SIZE];

void romWrite(uint16_t addr, uint8_t* data, uint16_t len) {
    for (uint16_t i = 0; i < len; i++) {
        ROM_CHIP[addr + i] = data[i];
        if (mode == RECV_EEPROM) {
            if (((addr + i) & 0x3F) == 0x3F) 
                delay(10);
        }
    }
}

uint16_t romCheck(uint16_t addr, uint16_t len) {
    uint16_t sum = 0;
    for (uint16_t i = 0; i < len; i++) {
        sum += ROM_CHIP[addr + i];
    }
    return sum;
}

void romErase() {
    for (uint16_t i = 0; i < XDATA_XCS1_SIZE; i++) {
        ROM_CHIP[i] = 0x00;
        if (mode == RECV_EEPROM) {
            if (((addr + i) & 0x3F) == 0x3F) 
                delay(10);
        }
    }
}

void main() {
    sysClockConfig();
    delay(10);

    P4_DIR = 0xBF; // A0~A5, LED

    PIN_FUNC |= (bXBUS_EN | bXBUS_CS_OE | bXBUS_AH_OE | bXBUS_AL_OE);
    XBUS_AUX |= bALE_CLK_EN;
    XBUS_SPEED &= ~(bXBUS1_WIDTH1 | bXBUS1_WIDTH0);
    XBUS_SPEED |= bXBUS1_WIDTH1;
    
    uart0Config(500000);
    EA = 1;
    delay(50);

    LED = 1;

    mset(rom_buf, 0, ROM_BUFFER_SIZ);

    uint8_t recv = 0;
    while (1) {
        if (recv == mode) {
            param = uart0BlockRecv(NULL, ROM_BUFFER_SIZ);
            if (param == ROM_BUFFER_SIZ) {
                param = 0;
                for (uint16_t i = 0; i < ROM_BUFFER_SIZ; i++)
                    param += rom_buf[i];

                if (param == chksum) {
                    romWrite(addr, rom_buf, ROM_BUFFER_SIZ);
                    param = romCheck(addr, ROM_BUFFER_SIZ);
                    if (param == chksum)
                        uart0Write("OK!\n");
                    else
                        uart0Write("ERR\n");
                } else
                    uart0Write("ERR\n");

                recv = 0;
                LED = !LED;
            }
        } else switch (param = uart0Get()) {
            case 0x55:
                if (uart0Gets((uint8_t*) &param, 2)) {
                    if (param == 0x3232) {
                        romErase();
                        uart0Write("OK!\n");
                    }
                }
                break;
            case 0x59:
                if (uart0Gets((uint8_t*) &param, 2)) {
                    if (mode == RECV_EEPROM) {
                        if (param == 0x5555) {
                            // Disable SDP
                            ROM_CHIP[0x5555] = 0xAA;
                            ROM_CHIP[0x2AAA] = 0x55;
                            ROM_CHIP[0x5555] = 0x80;
                            ROM_CHIP[0x5555] = 0xAA;
                            ROM_CHIP[0x2AAA] = 0x55;
                            ROM_CHIP[0x5555] = 0x20;
                        } else {
                            // Enable SDP
                            ROM_CHIP[0x5555] = 0xAA;
                            ROM_CHIP[0x2AAA] = 0x55;
                            ROM_CHIP[0x5555] = 0xA0;
                        }
                        delay(50);
                        uart0Write("OK!\n");
                    }
                }
                break;
            case 0x99:
                if (uart0Gets((uint8_t*) &param, 2)) {
                    if (param == 0x6666) {
                        mode = RECV_FRAM;
                    } else {
                        mode = RECV_EEPROM;
                    }
                }
                break;
            case 0xA5:
                if (uart0Gets((uint8_t*) &param, 2)) {
                    addr = param;
                }
                break;
            case 0xA9:
                if (uart0Gets((uint8_t*) &param, 2)) {
                    chksum = param;
                }
                break;
            case 0xAA:
                if (uart0Gets((uint8_t*) &param, 2)) {
                    if (param == 0xAA55) {
                        recv = mode;
                        uart0BlockRecv(rom_buf, ROM_BUFFER_SIZ);
                        uart0Write("GO!\n");
                    }
                }
                break;
            case 0xAB:
                if (uart0Gets((uint8_t*) &param, 2)) {
                    if (param == 0x55AA) {
                        uart0Write("GO!\n");
                        param = 0;
                        for (uint16_t i = 0; i < ROM_BUFFER_SIZ; i++) {
                            if (addr + i < XDATA_XCS1_SIZE) {
                                uart0Send(ROM_CHIP[addr + i]);
                                param += ROM_CHIP[addr + i];
                            } else {
                                uart0Send(0x00);
                                param += 0x00;
                            }
                        }
                        uart0Send(param & 0xFF);
                        uart0Send(param >> 8);
                    }
                }
                break;
            default:
                delay(10);
                break;
        }
    }
}
