/*
2024-03-08
Michael Nowak
http://www.mino-elektronik.de
Alle Angaben ohne Gewaehr !
*/

#define PICO_FM_AS65
#include "System-Pico-FM-AS65.h"

#define VERSION_LCD1      "mino PicoFmeter3"
#define VERSION_LCD2      "V2.1  2024-03-08"
#define VERSION_LCD1_AS65 "mino PicoFM-AS65"
#define VERSION_LCD2_AS65 "V2.1  2024-03-08"
#define VERSION_SER       "mino PicoFmeter3 V2.1 2024-03-08"
#define VERSION_SER_AS65  "mino Pico-FM-AS65 V2.1 2024-03-08"

#define MSP               (Reset_IRQn - 1)                    // position of master stackptr.
#define VEC_TAB_ELEMENTE  (RTC_IRQ_IRQn - MSP)
uint32_t vec_tab[VEC_TAB_ELEMENTE] __attribute__((aligned(256)));  // for VTOR
uint32_t *irq_tab;                                            // IRQ vec_tab inside RAM

const char *version;
uint32_t SystemCoreClock  = (DEF_SYSCLOCK);                   // 133 MHz
uint32_t F_xtal = MHZ_12;                                     // onboard default

double f_ref_korrektur = 1.0;                                 // TCXO-Korrektur mit Poti
volatile uint8_t ms_flag, adc_flag;                           // ms-flags
uint8_t ex_ref_aktiv;
uint8_t sprache = DT,
        as6501_aktiv,
        as6501_bestueckt,
        as6501_sperre;
extern void Fgps_messung(void);
extern int  init_AS6501(void);

// USB-output using Arduino-IDE
#ifdef RP2040_ARDUINO                                         // defined in "RP2040_arduino.h"
int USB_getc(void)
{
int temp = EOF;  
  if(Serial.available() > 0)                                  // test
    temp = Serial.read();                                     // and read data
  return(temp);
} 

void USB_putc(int zeichen)                                    // USB-out
{
  if(Serial.availableForWrite() >= 20)                        // test
    Serial.write(zeichen);                                    // and write
}
#else
int USB_getc(void) {return(EOF);}                             // default no USB
void USB_putc(int zeichen) {}
#endif


void short_delay(void)                                        // no timer, no ISR
{
  volatile int i = 1000;
  while(i--);
}  

// setup XOSC, SystemCoreClock and some IO-clock
void init_clock(void)
{
volatile int i;
  VREG_AND_CHIP_RESET_SET-> VREG = 15<<VREG_AND_CHIP_RESET_VREG_VSEL_Pos;
  while(!(VREG_AND_CHIP_RESET->VREG & VREG_AND_CHIP_RESET_VREG_ROK_Msk));
  short_delay();                                              // waiting for stable Vreg 

  XOSC->CTRL     = (XOSC_CTRL_FREQ_RANGE_1_15MHZ << XOSC_CTRL_FREQ_RANGE_Pos);
  XOSC->STARTUP  = 100;                                       // waiting for stable XO 12 MHz
  XOSC_SET->CTRL = (XOSC_CTRL_ENABLE_ENABLE << XOSC_CTRL_ENABLE_Pos);
  while (!(XOSC->STATUS & XOSC_STATUS_STABLE_Msk));
  
  RESETS_CLR->RESET = RESETS_RESET_pll_sys_Msk;
  while (!(RESETS->RESET_DONE & RESETS_RESET_pll_sys_Msk));


  PLL_SYS->CS = (REF_DIV << PLL_SYS_CS_REFDIV_Pos);           // PLL: 6 MHz / step
  PLL_SYS->FBDIV_INT = (SystemCoreClock*PLL_DIV2*REF_DIV)/(F_xtal/PLL_DIV1);  // PLL-VCO typ. 1310 MHz
 
  PLL_SYS->PRIM = (PLL_DIV1 << PLL_SYS_PRIM_POSTDIV1_Pos) |   // setup devider
                  (PLL_DIV2 << PLL_SYS_PRIM_POSTDIV2_Pos);
  SystemCoreClock = PLL_SYS->FBDIV_INT *(F_xtal/REF_DIV) /    // calculate eff. frequency
                    (PLL_DIV1*PLL_DIV2);

  PLL_SYS_CLR->PWR = PLL_SYS_PWR_VCOPD_Msk | PLL_SYS_PWR_PD_Msk;
  while (!(PLL_SYS->CS & PLL_SYS_CS_LOCK_Msk));               // wait until done

  PLL_SYS_CLR->PWR = PLL_SYS_PWR_POSTDIVPD_Msk;

  CLOCKS->CLK_REF_CTRL = (CLOCKS_CLK_REF_CTRL_SRC_xosc_clksrc << CLOCKS_CLK_REF_CTRL_SRC_Pos);
  CLOCKS->CLK_SYS_CTRL = (CLOCKS_CLK_SYS_CTRL_AUXSRC_clksrc_pll_sys << CLOCKS_CLK_SYS_CTRL_AUXSRC_Pos) |
                         (CLOCKS_CLK_SYS_CTRL_SRC_clksrc_clk_sys_aux << CLOCKS_CLK_SYS_CTRL_SRC_Pos);

  CLOCKS->CLK_PERI_CTRL = CLOCKS_CLK_PERI_CTRL_ENABLE_Msk |
      (CLOCKS_CLK_PERI_CTRL_AUXSRC_clk_sys << CLOCKS_CLK_PERI_CTRL_AUXSRC_Pos);

  CLOCKS->CLK_ADC_CTRL = CLOCKS_CLK_ADC_CTRL_ENABLE_Msk |
      (CLOCKS_CLK_ADC_CTRL_AUXSRC_xosc_clksrc << CLOCKS_CLK_ADC_CTRL_AUXSRC_Pos);

  RESETS_CLR->RESET = RESETS_RESET_io_bank0_Msk |             // enable GPIO
                      RESETS_RESET_pads_bank0_Msk;
  while((RESETS->RESET_DONE & (RESETS_RESET_io_bank0_Msk | RESETS_RESET_pads_bank0_Msk)) !=
        (RESETS_RESET_io_bank0_Msk | RESETS_RESET_pads_bank0_Msk));
}


void set_turbo_mode()                                         // POSTDIV2 / 2 => SystemCoreClock *= 2
{
uint32_t div1, div2, ref_div;
  VREG_AND_CHIP_RESET_SET-> VREG = 15<<VREG_AND_CHIP_RESET_VREG_VSEL_Pos;
  while(!(VREG_AND_CHIP_RESET->VREG & VREG_AND_CHIP_RESET_VREG_ROK_Msk));
  short_delay();       
  div1 = (PLL_SYS->PRIM >> PLL_SYS_PRIM_POSTDIV1_Pos) & 0x07;
  div2 = (PLL_SYS->PRIM >> PLL_SYS_PRIM_POSTDIV2_Pos) & 0x07;
  div2 /= 2;
  ref_div = (PLL_SYS->CS >> PLL_SYS_CS_REFDIV_Pos) & 0x3f;    // PLL-Vorteiler

  PLL_SYS->PRIM = (div1 << PLL_SYS_PRIM_POSTDIV1_Pos) | 
                  (div2 << PLL_SYS_PRIM_POSTDIV2_Pos);
  while (!(PLL_SYS->CS & PLL_SYS_CS_LOCK_Msk));
                  
  SystemCoreClock = PLL_SYS->FBDIV_INT * (F_xtal/ref_div) / (div1 * div2);
} 

// looking for 3k3 resistor between LCD_RS - LCD_RW
// double SystemCoreClock if found
int teste_turbo(void)
{
uint8_t turbo = 1;                                            // default  
  RESETS_CLR->RESET = RESETS_RESET_io_bank0_Msk | 
                      RESETS_RESET_pads_bank0_Msk;
  while((RESETS->RESET_DONE & (RESETS_RESET_io_bank0_Msk | RESETS_RESET_pads_bank0_Msk)) !=
        (RESETS_RESET_io_bank0_Msk | RESETS_RESET_pads_bank0_Msk));
  IO_BANK0->GPIO1_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;
  IO_BANK0->GPIO2_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;
// pullup an beiden Eingaengen einschalten
  PADS_BANK0->GPIO1 = PADS_BANK0_GPIO0_IE_Msk | PADS_BANK0_GPIO0_PUE_Msk;
  PADS_BANK0->GPIO2 = PADS_BANK0_GPIO0_IE_Msk | PADS_BANK0_GPIO0_PUE_Msk;
  SIO->GPIO_OE_SET =  BIT(2);                                 // set GPIO2 = 0
  short_delay();
  if((SIO->GPIO_IN & BIT(1))) turbo = 0;                      // GPIO1 remains '1': no 3k3
  SIO->GPIO_OE_CLR =  BIT(2);                                 // set GPIO2 to input again
  SIO->GPIO_OE_SET =  BIT(1);                                 // set GPIO1 = 0
  short_delay();
  if((SIO->GPIO_IN & BIT(2))) turbo = 0;                      // GPIO2 remains '1': no 3k3
  SIO->GPIO_OUT_CLR =  BIT(1);                                // set GPIO1 to input again
  return(turbo);                                              // '1': set turbo-mode
}  


void TIMER_IRQ_0_IRQHandler(void)                             // ms timer
{
  TIMER->INTR = TIMER_INTR_ALARM_0_Msk;                       // clear flag
  TIMER->ALARM0 = TIMER->TIMELR + MS_TEILER;                  // set next event
  ms_flag = 1;
  adc_flag = 1;
}

// Arduino-IDE has initialised TIMER_CH3 for USB
void init_timer(void)
{
  if(!(RESETS->RESET_DONE & RESETS_RESET_timer_Msk)) {        // not yet done
    RESETS_CLR->RESET = RESETS_RESET_timer_Msk;
    while (!(RESETS->RESET_DONE & RESETS_RESET_timer_Msk));
    WATCHDOG->TICK = WATCHDOG_TICK_ENABLE_Msk | (F_xtal / MHZ_FAKTOR);
    TIMER->TIMELW = 0;
    TIMER->TIMEHW = 0;
  }
  irq_tab[TIMER_IRQ_0_IRQn] = (uint32_t)TIMER_IRQ_0_IRQHandler; // set ISR-vector
  NVIC_EnableIRQ(TIMER_IRQ_0_IRQn);
  NVIC_SetPriority(TIMER_IRQ_0_IRQn, PRIO_LOW);               // low priority
  TIMER->ALARM0 = TIMER->TIMELR + MS_TEILER;
  TIMER->INTE |= TIMER_INTE_ALARM_0_Msk;
}

void warten_ms(uint32_t ms)
{
  do {
    while(!ms_flag);
    ms_flag = 0;
    adc_flag = 0;
  } while(--ms);
} 

void init_adc2(void)                                          // ADC-ch2 running continuously
{
  RESETS_CLR->RESET = RESETS_RESET_adc_Msk;
  while (!(RESETS->RESET_DONE & RESETS_RESET_adc_Msk));
  ADC->CS = ADC_CS_START_MANY_Msk                             // auto
          | 2 << ADC_CS_AINSEL_Pos                            // set ch2
          | ADC_CS_EN_Msk;                                    // start
  PADS_BANK0->GPIO28 = PADS_BANK0_GPIO0_OD_Msk;               // output disable
}

uint32_t lese_poti(void)                                      // read
{
  return(ADC->RESULT);                                        // ADC2 at GPIO28
}  

// test external trimmpot and correct frequency if available
void teste_abgleich(void)
{
static int32_t summe = ADC_MAX/2 * ADC_FILTER;                // starting at +/- 0
static uint32_t poti_vorhanden, test_poti;
  if(!test_poti) {
    test_poti = 1;
    IO_BANK0->GPIO28_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;  // GPIO input
// set pullup 
    PADS_BANK0->GPIO28 = PADS_BANK0_GPIO0_IE_Msk | PADS_BANK0_GPIO0_PUE_Msk;
    short_delay();
    if(lese_poti() <= ADC_MAX - 100) poti_vorhanden = 2;      // max. not reached
// set pulldown
    PADS_BANK0->GPIO28 = PADS_BANK0_GPIO0_IE_Msk | PADS_BANK0_GPIO0_PDE_Msk;
    short_delay();
    if(lese_poti() >= 100) poti_vorhanden = 3;                // min. no reached
    PADS_BANK0->GPIO28 = PADS_BANK0_GPIO0_IE_Msk;             // no pullup/pulldown
  }  

  if(adc_flag && poti_vorhanden) {                            // ms-delay and pot found
    adc_flag = 0;
    summe -= summe/ADC_FILTER;
    summe += (lese_poti() - ADC_MAX/2);                       // +/- 0 ppm in mid position
// setup correction: +/- 2047 to +/- 2,047 ppm ofSystemCoreClock
    f_ref_korrektur = (double)SystemCoreClock * (double)summe/ADC_FILTER*1e-9;  // 2e-8 for +/- 20 ppm
  }
}  

void reset_io(void)
{
  RESETS_SET->RESET = RESETS_RESET_io_bank0_Msk |             // setup 'clean' IO
                      RESETS_RESET_pio0_Msk | 
                      RESETS_RESET_pio1_Msk | 
                      RESETS_RESET_dma_Msk | 
                      RESETS_RESET_pwm_Msk | 
                      RESETS_RESET_timer_Msk |                        
                      RESETS_RESET_pads_bank0_Msk;
  SIO->GPIO_OE_CLR = 0xffffffff;                              // all GPIOs are inputs
  short_delay();
}

void setup(void)
{
uint8_t ee_init;
int temp;
#ifndef RP2040_ARDUINO
  irq_tab = (uint32_t *)(PPB->VTOR);                          // org. vectors
  for(temp = 0; temp < VEC_TAB_ELEMENTE; temp++)
      vec_tab[temp] = irq_tab[temp];                          // copy into RAM
  PPB->VTOR = (uint32_t)vec_tab;                              // and set VTOR
  reset_io();                                                 // for debugging
#else                                                         // Arduino uses VTOR @ 0x20000000
  RESETS_CLR->RESET = RESETS_RESET_io_bank0_Msk |             // min. setup of GPIO
                      RESETS_RESET_pads_bank0_Msk;
  while((RESETS->RESET_DONE & (RESETS_RESET_io_bank0_Msk | RESETS_RESET_pads_bank0_Msk)) !=
        (RESETS_RESET_io_bank0_Msk | RESETS_RESET_pads_bank0_Msk));
  Serial.begin(DEF_BAUDRATE);                                 // init Arduino-USB
#endif  

  irq_tab = (uint32_t *)(PPB->VTOR) + (TIMER_IRQ_0_IRQn - MSP); // offset to 1. user ISR 
  init_clock();
  
  ee_read_uint8(EE_ADR_PARA_INIT, &ee_init);                  // test EEPROM data
  if(ee_init == EE_INIT_MUSTER) {                             // use EEPROM data
      lese_ee_F1_parameter();                                 // if EE_INIT_MUSTER ok
      lese_ee_F2_parameter();                                 // load rest
      lese_ee_allgemein_parameter();
  } else {                                                    
      ee_write_uint8(EE_ADR_PARA_INIT, EE_INIT_MUSTER);       // update EE_INI_MUSTER
      schreibe_ee_F1_parameter();                             // and write default data
      schreibe_ee_F2_parameter();                       
      schreibe_ee_allgemein_parameter();
  } 

  temp = teste_turbo();
  lcd_init();
  
  as6501_bestueckt = as6501_aktiv = init_AS6501();            // test if AS6501 ist assembled
  if(as6501_sperre) as6501_aktiv = 0;                         // and disable, if selected
  if(as6501_aktiv) {
    F1_offset_schritt = OFFSET_SCHRITT_AS65;                  // set 'fine' Foffset resolution
    lcd_zeile1((char *)VERSION_LCD1_AS65);
    lcd_zeile2((char *)VERSION_LCD2_AS65);
    version = VERSION_SER_AS65;
  } else {
    F1_offset_schritt = OFFSET_SCHRITT;                       // set 'coarse' Foffset resolution
    lcd_zeile1((char *)VERSION_LCD1);
    lcd_zeile2((char *)VERSION_LCD2);
    version = VERSION_SER;
  }
  if(temp) set_turbo_mode();                                  // setup turbo-mode; SytemCoreClock *= 2
  init_timer();
  init_adc2();
  init_uart1();
  F1_messung(1);                                              // init channel F1
  while(ausgabe_sperre && TIMER->TIMELR <= 2000000) {
    F1_messung(0);                                            // show LCD for 2 s
  }  
  ausgabe_sperre = 0;
  update_LCD = 1;                                             // and start mainloop
}

#ifndef RP2040_ARDUINO
int main(void)                                                // no Arduino-IDE
{
  setup();                                                    // init all
  while(1) {                                                  // startimg 'main' loop
#else
void loop(void)                                               // else Arduino loop()
{
#endif  
int taste;
    __WFE();
    F1_messung(0);                                            // run
    if(!as6501_aktiv)                                         // AS6501 disabled
      Fgps_messung();                                         // so use ext. F-correction
    teste_abgleich();
    taste = lese_taste();                                     // test any key pressed
    if(taste == PLUS_TASTE) {                                 // '+' key
      if(anzeige_funktion < MAX_FUNKTION-1) {                 
        anzeige_funktion++;                                   // change main display: statistics -> F+P
        update_LCD = 1;
        ee_write_uint8(EE_ADR_ANZEIGE_FKT, anzeige_funktion);
      }
    } else if(taste == MINUS_TASTE) {                         // '-' key
      if(anzeige_funktion > 0) {
        anzeige_funktion--;                                   // change main display: F+P -> statistics
        update_LCD = 1;
        ee_write_uint8(EE_ADR_ANZEIGE_FKT, anzeige_funktion);
      }
    } else if(taste == PLUS_MINUS) {                          // '+' and '-' keys
      if(anzeige_funktion >= FMEAN_UND_N) {
        stat_n = 0;                                           // clear statistics if selected
        update_LCD = 1;
      }
    } else if(taste == INIT_TASTE) {                          // mid. key
      ausgabe_sperre = 1;                                     // disabled LCD-update
      FM_AS65_hauptmenue();                                   // select main-menue
      ausgabe_sperre = 0;                                     // and go on
      update_LCD = 1;                                         // showing act. values
    }  
#ifndef RP2040_ARDUINO
  }                                                           // 'main' loop
#endif
}
