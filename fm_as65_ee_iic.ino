#define FM_AS65_EE_IIC
/*
2023-04-07
Michael Nowak
http://www.mino-elektronik.de
Alle Angaben ohne Gewaehr !
*/
#include "System-Pico-FM-AS65.h"


#define WR_ADR_ACK          0x1     // status bei gueltiger adressierung
#define DEF_EE_BASIS_ADR    0xa0    // hardware Einstellung
#define DEF_MULTI_BYTE      8       // mehrere Bytes in einem Zyklus schreiben

#define SDA         26   // GPIO26
#define SCL         27   // GPIO27

#define SDA_PIN   GPIO26
#define SCL_PIN   GPIO27

static uint8_t ee_base_adr = DEF_EE_BASIS_ADR,
               multi_byte = DEF_MULTI_BYTE;
         
static void delay_iic(uint32_t us)
{
  RESETS_CLR->RESET = RESETS_RESET_pwm_Msk;
  while (!(RESETS->RESET_DONE & RESETS_RESET_pwm_Msk));
  PWM->CH1_TOP = 70;                // 1. Zeit
  PWM->CH1_CSR =  1;                // enable
  do {
    PWM->INTR =  2;
    while(!(PWM->INTR & 2));
    PWM->CH1_TOP = SystemCoreClock/MHZ_FAKTOR;    // 
  } while(us--);
}

void IIC_warten(void)
{
  delay_iic(2);      // fuer ca. 100kHz IIC-Takt   
}


uint8_t read_scl(void)
{
  IIC_warten();
  if(SIO->GPIO_IN & (1<< SCL)) return(1);
  return(0);
}

uint8_t read_sda(void)
{
  IIC_warten();
  if(SIO->GPIO_IN & (1<< SDA)) return(1);
  return(0);
}


void scl_0(void)                    // subroutinen fuer langsame Ausfuehrung
{
  SIO->GPIO_OE_SET = (1<< SCL);  // loeschen
  IIC_warten();
}

void scl_1(void)
{
  SIO->GPIO_OE_CLR = (1<< SCL); // setzen
  while(!read_scl());          // warten bis scl = 1
  IIC_warten();
}

void sda_0(void)
{
  SIO->GPIO_OE_SET = (1<< SDA);  // loeschen
  IIC_warten();
}

void sda_1(void)
{
  SIO->GPIO_OE_CLR = (1<< SDA);;  // setzen
  IIC_warten();
}

void iic_start(void)
{
static uint8_t init = 0;
  if(!init) {
// Enable GPIOs
    RESETS_CLR->RESET = RESETS_RESET_io_bank0_Msk | 
                        RESETS_RESET_pads_bank0_Msk;
    while((RESETS->RESET_DONE & (RESETS_RESET_io_bank0_Msk | RESETS_RESET_pads_bank0_Msk)) !=
          (RESETS_RESET_io_bank0_Msk | RESETS_RESET_pads_bank0_Msk));
    init = 1;
// Ausg�nge f�r IIC_EE: Open Drain mit PullUp 
    IO_BANK0->GPIO26_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0; //SDA
    IO_BANK0->GPIO27_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0; //SCL
    PADS_BANK0->SDA_PIN = PADS_BANK0_GPIO26_IE_Msk |             // pullup + schmitttrigger
                      PADS_BANK0_GPIO26_PUE_Msk | 
                      PADS_BANK0_GPIO26_SCHMITT_Msk;
    PADS_BANK0->SCL_PIN = PADS_BANK0_GPIO27_IE_Msk |             // pullup + schmitttrigger
                      PADS_BANK0_GPIO27_PUE_Msk | 
                      PADS_BANK0_GPIO27_SCHMITT_Msk;
  }
  scl_0();              // zunaechst Clock auf 0
  sda_1();              // damit keine Stopkondition entstehen kann
  scl_1();              // sda und scl sind passiv
  sda_0();              // Startbedingung
}

void iic_stop(void)
{
  sda_0();              // muss unten sein
  scl_1();              // Clock zuerst auf 1
  sda_1();              // Stopbedingung
}

void set_ack(void)
{
  scl_0();              // auf 0, damit keine Startbedingung
  sda_0();              // Datenbit auf 0 -> ack
  scl_1();              // impuls dazu
  scl_0();              // ack. ist vorbei
  sda_1();              // wieder auf eingang schalten
}

void set_nack(void)
{
  scl_0();              // auf 0, damit keine Startbedingung
  sda_1();              // Datenbit auf 1 -> ack
  scl_1();              // impuls dazu
  scl_0();              // ack. ist vorbei
}

uint8_t get_ack(void)       // ack-impuls ist activ low
{
uint8_t temp;
  scl_0();              // auf 0, damit keine Startbedingung
  sda_1();              // Datenbit auf 1 -> ack
  scl_1();              // impuls dazu
  temp = read_sda();    // falls benoetigt
  scl_0();              // ack. ist vorbei
  return(temp ^ 1);     // 1 = ack-signal erhalten
}

uint8_t iic_write(uint8_t wert)
{
uint8_t n;
  for(n=0; n<8; n++) {
    scl_0();            // Aenderungen nur bei clock=0
    if(wert & 0x80) sda_1();
    else sda_0();
    scl_1();            // clock dazu
    wert <<= 1;         // von msb -> lsb
  }
  return(get_ack());
}

uint8_t iic_read_ack(uint8_t *wert)
{
uint8_t temp,n;
  scl_0();              // clock muss 0 sein
  sda_1();              // sda als input
  for(n=0; n<8; n++) {
    temp <<= 1;         // von msb -> lsb
    scl_1();            // daten einlesen
    temp += read_sda();
    scl_0();            // clock wegnehmen
  }
  *wert = temp;         // gelesenes byte ablegen
  set_ack();            // weitere bytes werden erwartet
  return(0);            // dummy status
}

uint8_t iic_read_nack(uint8_t *wert)
{
uint8_t temp,n;
  scl_0();              // clock muss 0 sein
  sda_1();              // sda als input
  for(n=0; n<8; n++) {
    temp <<= 1;         // von msb -> lsb
    scl_1();            // daten einlesen
    temp += read_sda();
    scl_0();            // clock wegnehemen
  }
  *wert = temp;         // gelesenes byte ablegen
  set_nack();           // transfer beenden
  return(0);            // dummy status
}

uint8_t iic_adr_write(uint8_t adr)  // baustein zum schreiben adressieren
{
  iic_start();
  return(iic_write(adr & 0xfe));    // return(1), wenn baustein vorhanden
}

uint8_t iic_adr_read(uint8_t adr)   // baustein zum lesen adressieren
{
  iic_start();
  return(iic_write(adr | 1));       // return(1), wenn baustein vorhanden
}

void init_ee_base(uint8_t basis, uint8_t multi)
{
  ee_base_adr = basis;
  multi_byte = multi;
}

static void form_page_adr(uint16_t adr, uint8_t *page, uint8_t *rest)
{
  *page = ee_base_adr + ((adr/256) << 1);   // IIC-Adresse bilden
  *rest = adr & 0xff;                       // rest der gesamtadresse
}

uint8_t iic_test(uint8_t iic_adr)       // Baustein testen
{
uint8_t status;
int timeout=500;                        // wartet ca. 10ms maximal
  while(timeout--) {
    status = iic_adr_write(iic_adr);    // nur die Basisadresse testen
    iic_stop();
    if(status == WR_ADR_ACK) return(0); // baustein laesst sich ansprechen
  }
  return(1);                            // laesst sich nicht ansprechen
}

uint8_t ee_write_n(uint16_t adr, void *ptr, uint16_t anzahl)    // array schreiben
{
uint8_t page, rest, status=0, n;
uint8_t *p = (uint8_t *)ptr;
  while(anzahl) {
    n = multi_byte - (adr & (multi_byte-1));    // adr ausrichten
    form_page_adr(adr, &page, &rest);
    if(n > anzahl) n = anzahl;          // begrenzen
    anzahl -= n;
    adr += n;                           // adresse fuer naechsten zugriff
    status = iic_test(page);
    if(status)           // test, ob eeprom bereit
      return(status);                   // eeprom nicht vorhanden
    iic_adr_write(page);                // eeprom adressieren
    iic_write(rest);                    // zieladresse uebergeben
    for(;n;n--)
      iic_write(*(p++));                // bytes schreiben
    iic_stop();                         // Programmierung starten
  }
  return(status);
}

uint8_t ee_read_n(uint16_t adr, void *ptr, uint16_t anzahl) // array lesen
{
uint8_t page, rest, status=0;
uint8_t *p = (uint8_t *)ptr;
  if(!anzahl--) return(1);              // anzahl = 0
  form_page_adr(adr, &page, &rest);
  status = iic_test(page);
  if(status)           // test, ob eeprom bereit
    return(status);                     // eeprom nicht vorhanden
  iic_adr_write(page);                  // adressieren
  iic_write(rest);                      // leseadresse vorgeben
  iic_adr_read(page);                   // und read-modus einstellen
  while(anzahl--) {
    iic_read_ack(p);                    // anzahl-1 lesen
    p++;        
  }
  iic_read_nack(p);                     // letztes byte ohne Quittung
  iic_stop();
  return(status);
}


uint8_t ee_write_uint8(uint16_t adr, uint8_t wert)      // ein byte schreiben
{
  return(ee_write_n(adr, &wert, sizeof(char)));
}

uint8_t ee_write_uint16(uint16_t adr, uint16_t wert)    // zwei byte schreiben
{
  return(ee_write_n(adr, &wert, sizeof(uint16_t)));
}

uint8_t ee_write_uint32(uint16_t adr, uint32_t wert)  // vier byte schreiben
{
  return(ee_write_n(adr, &wert, sizeof(uint32_t)));
}

uint8_t ee_write_double(uint16_t adr, double wert)  // acht byte schreiben
{
  return(ee_write_n(adr, &wert, sizeof(double)));
}


uint8_t ee_read_uint8(uint16_t adr, uint8_t *wert)      // ein byte lesen
{
  return(ee_read_n(adr, wert, sizeof(uint8_t)));
}

uint8_t ee_read_uint16(uint16_t adr, uint16_t *wert)    // zwei byte lesen
{
  return(ee_read_n(adr, wert, sizeof(uint16_t)));
}

uint8_t ee_read_uint32(uint16_t adr, uint32_t *wert)    // vier byte lesen
{
  return(ee_read_n(adr, wert, sizeof(uint32_t)));
}

uint8_t ee_read_double(uint16_t adr, double *wert)    // acht byte lesen
{
  return(ee_read_n(adr, wert, sizeof(double)));
}

