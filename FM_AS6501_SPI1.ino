/* 

2023-11-19
Michael Nowak
http://www.mino-elektronik.de
Alle Angaben ohne Gewaehr !
*/

#define FM_AS6501
#define nCAP2_AKTIV                // use delayed 2. channel

#include "System-Pico-FM-AS65.h"

// RP204 GPIOs
#define SPI1_SCLK       10
#define SPI1_TX         11
#define SPI1_RX         12
#define SPI1_SSN        13

// Adressen vom AS6501
#define REF_A_ADR        8        // Ergebnisse ab hier
#define STOP_A_ADR      11
#define REF_B_ADR       20
#define STOP_B_ADR      23

// Befehle fuer AS6501
#define OPC_POWER_ON    0x30      // power-on reset
#define OPC_INIT        0x18      // init und start
#define OPC_WRITE_CFG   0x80      // Konfiguration schreiben
#define OPC_READ_CFG    0x40      // Konfiguration lesen
#define OPC_READ_DATA   0x60      // Ergebnisse lesen ab inkl. Adresse

// AS6501 Init-Sequenz
#define MAX_INIT_TAB    sizeof(AS6501_init_tab)

const uint8_t AS6501_init_tab[] = {
#ifdef CAP2_AKTIV  
      0x15,           // stop-A, stop-B und ext. REFCLK einschalten
      0x85,           // stop-A und stop-B auswerten, 1x Auswertung
#else      
      0x11,           // stop-A und ext. REFCLK einschalten
      0x81,           // stop-A auswerten, 1x Auswertung
#endif      
      0x00,           // LVDS nicht aktiv
      (uint8_t)(REFCLK_AUFLOESUNG),       // LSB  
      (uint8_t)(REFCLK_AUFLOESUNG >> 8),  // MSB-1
      (uint8_t)(REFCLK_AUFLOESUNG >> 16), // MSB
      0x00,           // LVDS nicht aktiv
      0x00,           // dto.
      0xa1,           // interne Konstanten
      0x13,
      0x00,
      0x0a,
      0xcc,
      0xcc,
      0xf1,
      0x7d,           // bis hierhin
      0x04            // CMOS-Pegel Eing�nge
    };

void SPI1_stop()
{
  while((SPI1->SSPSR & SPI0_SSPSR_BSY_Msk));    // Ausgabe abwarten  
  SIO->GPIO_OUT_SET = BIT(SPI1_SSN);            // Select auf '1' passiv
}

// Befehl und Startadresse ausgeben
void TDC_write(uint8_t cmd)
{
  while(!(SPI1->SSPSR & SPI0_SSPSR_TNF_Msk));   // freies FIFO abwarten
  SPI1->SSPDR = (uint8_t)(cmd);
  while((SPI1->SSPSR & SPI0_SSPSR_BSY_Msk));    // Ausgabe abwarten  
}

// RX-FIFO leeren
void flush_SPI1(void)
{
  while((SPI1->SSPSR & SPI0_SSPSR_RNE_Msk))
     SPI1->SSPDR;
}

// WERT holen
uint8_t TDC_read(void)
{
  uint8_t data;
  SPI1->SSPDR = 0;
  while(!(SPI1->SSPSR & SPI0_SSPSR_RNE_Msk));
  data = SPI1->SSPDR;
  return(data);
}


int init_AS6501(void)
{
int n;
int as6501 = 1;                               // default AS6501 vorhanden
  RESETS_SET->RESET = RESETS_RESET_spi1_Msk;
  RESETS_SET->RESET = RESETS_RESET_spi1_Msk;
  RESETS_CLR->RESET = RESETS_RESET_spi1_Msk;
  while (!(RESETS->RESET_DONE & RESETS_RESET_spi1_Msk));

  IO_BANK0->GPIO10_CTRL = IO_BANK0_GPIO10_CTRL_FUNCSEL_spi1_sclk;
  IO_BANK0->GPIO11_CTRL = IO_BANK0_GPIO11_CTRL_FUNCSEL_spi1_tx;
  IO_BANK0->GPIO12_CTRL = IO_BANK0_GPIO12_CTRL_FUNCSEL_spi1_rx;
  IO_BANK0->GPIO13_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;

  SPI1->SSPCR0 = SPI0_SSPCR0_SPH_Msk | 7;     // SPH = 1, SPO = 0; 8 bit
  SPI1->SSPCPSR = (uint16_t)(SystemCoreClock / 4e7 + 1)& 0xfe; // moeglichst 40 MHz
  SPI1_SET->SSPCR1 = SPI0_SSPCR1_SSE_Msk;     // SPI1 aktiv
	
  PADS_BANK0->GPIO14 = //PADS_BANK0_GPIO14_OD_Msk |
                       PADS_BANK0_GPIO14_IE_Msk |             // pullup + schmitttrigger
                      PADS_BANK0_GPIO14_PUE_Msk | 
                      PADS_BANK0_GPIO14_SCHMITT_Msk;



// poer-on reset  
  SIO->GPIO_OUT_CLR = BIT(SPI1_SSN);      // Select auf '0' aktiv
  TDC_write(OPC_POWER_ON);                // Power-on reset
  SPI1_stop();                            // Select auf '1' passiv

// Konfiguration schreiben  
  SIO->GPIO_OUT_CLR = BIT(SPI1_SSN);      // Select auf '0' aktiv
  TDC_write(OPC_WRITE_CFG);               // Konfiguration schreiben
  for(n = 0; n < MAX_INIT_TAB; n++) {
    TDC_write(AS6501_init_tab[n]);
    flush_SPI1();
  }  
  SPI1_stop();                            // Select auf '1' passiv

// Konfiguration pr�fen  
  SIO->GPIO_OUT_CLR = BIT(SPI1_SSN);      // Select auf '0' aktiv
  TDC_write(OPC_READ_CFG);                // Konfiguration zur�cklesen
  flush_SPI1();
  for(n = 0; n < MAX_INIT_TAB; n++) {
    if(TDC_read() != AS6501_init_tab[n])
      as6501 = 0;                         // fehler aufgetreten oder AS6501 fehlt
  }  
  SPI1_stop();                            // Select auf '1' passiv
  
// AS6501 starten
  if(as6501) {
  SIO->GPIO_OUT_CLR = BIT(SPI1_SSN);      // Select auf '0' aktiv
    TDC_write(OPC_INIT);                  // Power-on reset
    SPI1_stop();                          // Select auf '1' passiv
  }
  flush_SPI1();                           // muss leer sein
  return(as6501);
}

// 24 bit Ergebnisse holen
uint32_t lese_AS6501_24(uint8_t adr)
{
static uint32_t temp=0, n;  
  SIO->GPIO_OUT_CLR = BIT(SPI1_SSN);          // Select auf '0' aktiv
  while(!(SPI1->SSPSR & SPI0_SSPSR_TFE_Msk)); // freies FIFO abwarten
  SPI1->SSPDR = (OPC_READ_DATA + adr);        // vier bytes ausgeben
  SPI1->SSPDR = 0;
  SPI1->SSPDR = 0;
  SPI1->SSPDR = 0;
  for(n = 0; n < 4; n++) {
    while(!(SPI1->SSPSR & SPI0_SSPSR_RNE_Msk));
    temp <<= 8;
    temp |= SPI1->SSPDR;  
  }  
  SPI1_stop();                                // Select auf '1' passiv
  return(temp & 0xffffff);                    // 24 Bit Werte
}

uint32_t lese_ref_A(void)
{
  return(lese_AS6501_24(REF_A_ADR));
}
         
uint32_t lese_stop_A(void)
{
  return(lese_AS6501_24(STOP_A_ADR));
} 
         
uint32_t lese_ref_B(void)
{
  return(lese_AS6501_24(REF_B_ADR));
}
         
uint32_t lese_stop_B(void)
{
  return(lese_AS6501_24(STOP_B_ADR));
} 
   
// 64 bit Ergebnisse ins SPI1-FIFO holen
void trigger_AS6501_64(int adr)
{
  SIO->GPIO_OUT_CLR = BIT(SPI1_SSN);          // Select auf '0' aktiv
  SPI1->SSPDR = (OPC_READ_DATA + adr);        // 1+6 bytes ausgeben
  SPI1->SSPDR = 0;
  SPI1->SSPDR = 0;
  SPI1->SSPDR = 0;
  SPI1->SSPDR = 0;
  SPI1->SSPDR = 0;
  SPI1->SSPDR = 0;
} 

// SPI1-FIFO auslesen und Endwert bilden
uint64_t read_AS6501_64(void)
{
  uint32_t temp1, temp2;                      // 2 x 32 Bit wegen Laufzeit
  while(!(SPI1->SSPSR & SPI0_SSPSR_RNE_Msk));
  SPI1->SSPDR;                                // ignore CMD
  
  while(!(SPI1->SSPSR & SPI0_SSPSR_RNE_Msk));
  temp1 = SPI1->SSPDR;                        // Index-MSB 
  temp1 <<= 8;           
  while(!(SPI1->SSPSR & SPI0_SSPSR_RNE_Msk));
  temp1 += SPI1->SSPDR;                       // Index+1
  temp1 <<= 8;           
  while(!(SPI1->SSPSR & SPI0_SSPSR_RNE_Msk));
  temp1 += SPI1->SSPDR;                       // Index-LSB
           
  while(!(SPI1->SSPSR & SPI0_SSPSR_RNE_Msk));
  SPI1->SSPDR;                                // ignore Stop-MSB
  while(!(SPI1->SSPSR & SPI0_SSPSR_RNE_Msk));
  temp2 = SPI1->SSPDR;                        // Stop+1
  temp2 <<= 8;
  while(!(SPI1->SSPSR & SPI0_SSPSR_RNE_Msk));
  temp2 += SPI1->SSPDR;                       // Stop-LSB 
  SIO->GPIO_OUT_SET = BIT(SPI1_SSN);          // Select auf '1' passiv
  return(((uint64_t)temp1 << 16) + temp2);    // 64 Bit Werte
}

// Anfordern und Auslesen des Zeitpunktes
uint64_t lese_AS6501(void)
{
static uint64_t result;  

  trigger_AS6501_64(REF_A_ADR);
  result = read_AS6501_64();
#ifdef CAP2_AKTIV   
  trigger_AS6501_64(REF_B_ADR);               // read cap2_time
  result += read_AS6501_64();                 // add to result
  result /= 2;                                // mean value
#endif  
 
  return(result);
}  
