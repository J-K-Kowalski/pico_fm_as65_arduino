/*
2024-02-29
Michael Nowak
http://www.mino-elektronik.de
Alle Angaben ohne Gewaehr !
*/

#define FM_AS65_UART1_PICO
#include "System-Pico-FM-AS65.h"

// Einstellen der Baudrate �ber baud_tab[baudraten_index]
uint32_t baud_tab[] = {9600, 19200, 38400, 57600, 115200, 230400, 256000, 500000};
const char *baud_string[] = {"9600 ","19200","38400","57600","115k2","230k4","256k0","500k0"};

uint8_t  baudraten_index = DEF_BAUD_INDEX;        // default 4: 115200 Bd

void set_baudrate1(uint32_t baud)
{
  baud /= 4;
  baud = SystemCoreClock / (baud );
  UART1->UARTIBRD = baud / 64;
  UART1->UARTFBRD = baud % 64;
  UART1->UARTLCR_H = 3 << UART0_UARTLCR_H_WLEN_Pos | 
                     UART0_UARTLCR_H_FEN_Msk;    // RxD+TxD fifo
}

// TX = GPIO4
// RX = GPIO5
void init_uart1(void)
{
  RESETS_CLR->RESET = RESETS_RESET_uart1_Msk;
  while (!(RESETS->RESET_DONE & RESETS_RESET_uart1_Msk));

  IO_BANK0->GPIO4_CTRL = IO_BANK0_GPIO4_CTRL_FUNCSEL_uart1_tx;
  IO_BANK0->GPIO5_CTRL = IO_BANK0_GPIO5_CTRL_FUNCSEL_uart1_rx;
  set_baudrate1(DEF_BAUDRATE);                      // Baudrate einstellen
  UART1_SET->UARTCR = UART0_UARTCR_UARTEN_Msk;      // und UART0 aktivieren
}	

void uart1_putc(int zeichen)
{
  while (UART1->UARTFR & UART0_UARTFR_TXFF_Msk);
  UART1->UARTDR = zeichen;
  USB_putc(zeichen);
}

int putchar_rp(int x)                 // 
{
  if((uint8_t)x == 10) uart1_putc(13); 
  uart1_putc(x);
  return(x);
}

int getchar_rp(void)
{
  if(UART1->UARTFR & UART0_UARTFR_RXFE_Msk) {
    return(EOF);                      // bei leerem Fifo -1
  } else {
    return(UART1->UARTDR & 0xff);     // 8 bit
  }
}  

void sende_str1(char *s)              // ser. Ausgabe
{
  while(*s) uart1_putc(*(s++));
  uart1_putc(13);                     // CR
  uart1_putc(10);                     // LF
}

void sende_strx(char *s, int cr_lf)              // ser. Ausgabe
{
  while(*s) uart1_putc(*(s++));
  if(cr_lf) {
    uart1_putc(13);                   // CR
    uart1_putc(10);                   // LF
  } else
    uart1_putc(' ');
} 
