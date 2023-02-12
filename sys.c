#include "sys.h"

size_t mc;

void mset(uint8_t* ptr, uint8_t val, size_t siz) {
    for (mc = 0; mc < siz; mc++)
        ptr[mc] = val;
}

void mmov(uint8_t* dst, uint8_t* src, size_t siz) {
    if (dst < src) {
        mc = 0;
        while (1) {
            dst[mc] = src[mc];
            mc += 1;
            if (mc == siz)
                break;
        }
    } else {
        mc = siz - 1;
        while (1) {
            dst[mc] = src[mc];
            if (mc == 0)
                break;
            mc -= 1;
        }
    }
}

void sysClockConfig() {
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    CLOCK_CFG &= ~MASK_SYS_CK_DIV;
    CLOCK_CFG |= (336000000 / FREQ_SYS);
    PLL_CFG = 0xFC; // fPll = 336MHz;
    SAFE_MOD = 0xFF;
#ifdef USE_EXT_OSC
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    CLOCK_CFG |= bOSC_EN_XT;
    delay(10);
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    CLOCK_CFG &= ~bOSC_EN_INT;
    SAFE_MOD = 0xFF;
#endif
}

void delay(uint16_t t) {
    while (t) {
        delay_us(1000);
        -- t;
    }
}

void delay_us(uint16_t t) {
    while (t) {
		++ SAFE_MOD;
#ifdef	FREQ_SYS
#if		FREQ_SYS >= 14000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 16000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 18000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 20000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 22000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 24000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 26000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 28000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 30000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 32000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 34000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 36000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 38000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 40000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 42000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 44000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 46000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 48000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 50000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 52000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 54000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 56000000
		++ SAFE_MOD;
#endif
#endif
		-- t;
	}
}

void sysTickConfig() {
    T2MOD &= ~bT2_CLK;
    C_T2 = 0;
    RCAP2 = T2COUNT = 0xFFFF - (uint16_t) (FREQ_SYS / 12 / 1000);   // 1000Hz
    TR2 = 1;
    ET2 = 1;
}

volatile uint32_t sysTickCount = 0;

void __tim2_interrupt() __interrupt (INT_NO_TMR2) __using (1) {
    if (TF2) {
        TF2 = 0;
        sysTickCount += 1;
    }
}

uint32_t sysGetTickCount() {
    return sysTickCount;
}

uint32_t x; uint8_t x2; uint8_t dat;

void uart0Config(uint32_t baud) {
    SM0 = 0; SM1 = 1; SM2 = 0;
    TCLK = 0; RCLK = 0;
    PCON |= SMOD;
    x = 10 * FREQ_SYS / baud / 16;

    x2 = x % 10;
    x /= 10;
    if (x2 >= 5) x++;

    TMOD = TMOD & ~bT1_GATE & ~bT1_CT & ~MASK_T1_MOD | bT1_M1;
    T2MOD |= (bTMR_CLK | bT1_CLK);
    TH1 = 0 - x;
    TR1 = 1;
    TI = 1;
    REN = 1;
    ES = 1;
}

volatile BOOL busy = 0;
static uint8_t recBuf0[UART_BUF_SIZE] = { 0 };
volatile uint8_t recPtr0 = 0;

BOOL block_recv = 0;
uint8_t* block_recv_ptr = NULL;
uint16_t block_recv_pos = 0;
uint16_t block_recv_siz = 0;

void __uart0_interrupt() __interrupt (INT_NO_UART0) __using (1) {
    if (TI) {
        TI = 0;
        busy = 0;
    }
    if (RI) {
        RI = 0;
        if (block_recv) {
            block_recv_ptr[block_recv_pos] = SBUF;
            block_recv_pos++;
            if (block_recv_pos >= block_recv_siz)
                block_recv = 0;
        } else if (recPtr0 < UART_BUF_SIZE) {
            recBuf0[recPtr0] = SBUF;
            recPtr0 += 1;
        }
    }
}

void uart0Send(uint8_t data) {
    while (busy);
    busy = 1;
    SBUF = data;
}

void uart0Write(char* s) {
    while (*s) {
        uart0Send(*s++);
    }
}

uint8_t uart0Get() {
    if (recPtr0 > 0) {
        dat = recBuf0[0];
        recPtr0 -= 1;
        if (recPtr0 > 0)
            mmov(recBuf0, recBuf0 + 1, recPtr0);
        mset(recBuf0 + recPtr0, 0, 1);
        return dat;
    } else
        return 0;
}

uint8_t uart0Gets(uint8_t* dat, uint8_t len) {
    if (recPtr0 >= len) {
        mmov(dat, recBuf0, len);
        recPtr0 -= len;
        if (recPtr0 > 0)
            mmov(recBuf0, recBuf0 + len, recPtr0);
        mset(recBuf0 + recPtr0, 0, len);
        return 1;
    } else
        return 0;
}

uint16_t uart0BlockRecv(uint8_t* ptr, uint16_t siz) {
    if (ptr != NULL) {
        block_recv_ptr = ptr;
        block_recv_pos = 0;
        block_recv_siz = siz;
        block_recv = 1;
    } else if (siz != 0) {
        return block_recv_pos;
    } else {
        block_recv = 0;
        block_recv_ptr = NULL;
        block_recv_pos = 0;
        block_recv_siz = 0;
    }
    return 0;
}

void uart1Config(uint32_t baud) {
    const uint8_t DIV = 1;

    SER1_LCR |= bLCR_DLAB;
    SER1_DIV = DIV;
    x = 10 * FREQ_SYS * 2 / DIV / 16 / baud;                             
    x2 = x % 10;
    x /= 10;
    if (x2 >= 5) x++;
    SER1_DLM = x >> 8;
    SER1_DLL = x & 0xFF;
    SER1_LCR &= ~bLCR_DLAB;
    
    XBUS_AUX |= bALE_CLK_EN; // RS485_EN = ~(bXBUS_CS_OE & ~bXBUS_AL_OE | bALE_CLK_EN)

    SER1_LCR |= MASK_U1_WORD_SZ;
    SER1_LCR &= ~(bLCR_PAR_EN | bLCR_STOP_BIT);

    SER1_IER |= ((3 << 4) & MASK_U1_PIN_MOD);
    //SER1_IER |= bIER_RECV_RDY;

    SER1_MCR |= (bMCR_OUT2 | bMCR_HALF);
    x2 = SER1_IIR;

    SER1_ADDR |= 0xFF;

    IE_UART1 = 1;

    sysTickConfig();
}

void uart1Send(uint8_t data) {
    while ((SER1_LSR & bLSR_T_FIFO_EMP) == 0);
    SER1_THR = data;
}

void uart1Write(char* s) {
    while (*s) {
        uart1Send(*s++);
    }
}

uint32_t u1t;
uint8_t uart1Get() {
    u1t = sysGetTickCount();
    while ((SER1_LSR & bLSR_DATA_RDY) == 0) {
        if (sysGetTickCount() - u1t > UART1_TIMEOUT) {
            u1t = 0;
            break;
        }
    }
    return SER1_RBR;
}

uint8_t u1c;
uint8_t uart1Gets(uint8_t* dat, uint8_t len) {
    for (u1c = 0; u1c < len; u1c++) {
        dat[u1c] = uart1Get();
        if (u1t == 0) return 0;
    }
    return 1;
}
