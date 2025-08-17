#define FM_AS65_LCD
#include "System-Pico-FM-AS65.h"

/*
Anschluß einer LC-Anzeige 2 x 16 -> 4 x 20
lcd_init() muss als erste Funktion aufgerufen werden. Danach sollten
lcd_cmd() und lcd_zeichen() Befehle und Daten korrekt ausgeben.

Das LCD-Modul wird im 4-Bit Modus betrieben. D0-D3 bleiben daher offen.
Verwendet werden von GPIO die Pins 6-13 wie folgt:

  LCD-Pin   Funktion        RP2040 GPIO

    1         GND            GND-Pin
    2         +5V       +5V Versorgungsspannung
    3       Kontrast   PWM_0A GPIO0
    4         RS              GPIO1
    5         R/W             GPIO2
    6         E               GPIO3
    7       D0(nc)
    8       D1(nc)
    9       D2(nc)
    10      D3(nc)
    11        D4              GPIO6
    12        D5              GPIO7
    13        D6              GPIO8
    14        D7              GPIO9

2023-04-14
Michael Nowak
http://www.mino-elektronik.de
Alle Angaben ohne Gewaehr !
*/

extern uint32_t SystemCoreClock;
#define KONTRAST_PIN  0
#define BL_ON_PIN     13      // zum Einschalten


#define MHZ_FAKTOR      1000000
#define LCD_KONTRAST    (1 << KONTRAST_PIN)
#define LCD_CMD         (1 << 1)
#define LCD_READ        (1 << 2)
#define LCD_STROBE      (1 << 3)
#define LCD_DATA_OFFSET    6  // ab GPIO6
#define BACKLIGHT_ON    (1 << BL_ON_PIN)
/*
#define MIN_LCD_BREITE    16
#define DEF_LCD_BREITE    16
#define MAX_LCD_BREITE    20
#define DEF_LCD_KONTRAST  20
#define MAX_LCD_KONTRAST  50
*/

#define LCD_INIT_DELAY    50000     // 50 ms
#define LCD_NIBBLE_DELAY  10000     // 10 ms

#define ZEILE1      0x7f            // jeweils 1 addieren fuer 1. Spalte
#define ZEILE2      0xbf            // dto.
#define BUSY_FLAG   0x80            // Bit7 vom LCD-Status

#define MAX_CGR_CHAR    8
#define CGR_CHAR_BYTES  8

/*  x-mean      x-max       x-min

    xxxxx       xxxxx       --x--
                --x--       --x--
    x---x       -xxx-       --x--
    -x-x-       x-x-x       x-x-x
    --x--       --x--       -xxx-
    -x-x-       --x--       --x--
    x---x       --x--       xxxxx
*/

const uint8_t CGR[MAX_CGR_CHAR][CGR_CHAR_BYTES] = {
            {0,0,0,0,0,0,0,0},
            {31,0,17,10,4,10,17},   // x-mean
            {31,4,14,21,4,4,4,0},   // x-max
            {4,4,4,21,14,4,31,0},   // x-min
//          {0,0,0,0,0,0,0,0},
//          {0,0,0,0,0,0,0,0},
//          {0,0,0,0,0,0,0,0},
//          {0,0,0,0,0,0,0,0}
            };

  
uint8_t zeilen_tab[] = {0,ZEILE1,ZEILE2,ZEILE1,ZEILE2}; /* alle werte -1 */
uint8_t lcd_zeile_start[5];

uint8_t lcd_breite = DEF_LCD_BREITE;
uint8_t lcd_kontrast = DEF_LCD_KONTRAST;
uint8_t lcd_busy;                   // Tasterabfrage unterdruecken
uint8_t lcd_vorhanden = 1;          // default

void lcd_impuls(); 

void delay_us(uint32_t us)
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

// 16 oder 20 Zeichen/Zeile initialisieren
void init_zeilen_breite(uint8_t breite)
{
int i, offset=1;
  if(breite != MIN_LCD_BREITE) offset += (breite-MIN_LCD_BREITE)/2;       // 2 Stellen weiter nach rechts schreiben
  lcd_breite = breite;
  zeilen_tab[3] = ZEILE1 + breite;            // für Zeile 3
  zeilen_tab[4] = ZEILE2 + breite;            // für Zeile 4
  for(i=0; i<5; i++)
    lcd_zeile_start[i] = zeilen_tab[i]+offset;
} 

// PWM_0A for LCD contrast
void setze_kontrast(uint8_t wert)
{
  RESETS_CLR->RESET = RESETS_RESET_pwm_Msk;
  while (!(RESETS->RESET_DONE & RESETS_RESET_pwm_Msk));
  IO_BANK0->GPIO0_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_pwm_a_0; // PWM-output
  PWM->CH0_TOP = MAX_LCD_KONTRAST-1;   // 100%
  PWM->CH0_DIV = (SystemCoreClock/MHZ_FAKTOR/5) << PWM_CH0_DIV_INT_Pos;  // ca. 100kHz
  PWM_SET->CH0_CSR = PWM_CH0_CSR_EN_Msk;

  if(wert > MAX_LCD_KONTRAST)                 // max. limit
    wert = DEF_LCD_KONTRAST;
  PWM->CH0_CC = wert;
}  

  
uint8_t lcd_lesen(void)                       // oberstes Bit ist busy-flag, Rest: Adresse
{
  uint8_t temp = 0;
// Datenleitungen auf Eingang
  SIO->GPIO_OE_CLR = 0xf << LCD_DATA_OFFSET;  // datenleitungen auf Eingang
  
  SIO->GPIO_OUT_SET = LCD_READ;               // Lesen aktivieren
  delay_us(1);
  SIO->GPIO_OUT_SET = LCD_STROBE;             // E zum Lesen setzen
  delay_us(1);
  temp = (SIO->GPIO_IN >> LCD_DATA_OFFSET) & 0xf;  // oberes nibble zuerst
  temp <<= 4;
    SIO->GPIO_OUT_CLR = LCD_STROBE;           // und wieder auf 0
  delay_us(1);
    SIO->GPIO_OUT_SET = LCD_STROBE;           // nächstes nibble lesen
  delay_us(1);
  temp |= (SIO->GPIO_IN >> LCD_DATA_OFFSET) & 0xf; // unteres nibble zuletzt
  SIO->GPIO_OUT_CLR = LCD_STROBE;             // und wieder auf 0
  SIO->GPIO_OUT_CLR = LCD_READ;               // dto
  delay_us(1);
  
// Datenleitungen wieder auf Ausgang  
  SIO->GPIO_OE_SET = 0xf << LCD_DATA_OFFSET;  // Datenleitungen auf Ausgang
  delay_us(1);
  return(temp);                               // busy-flag + Adresse
}

// Status lesen; busy-flag == 0, wenn neue Ausgabe möglich
uint8_t lcd_status(void)                      // oberstes Bit ist busy-flag, Rest: Adresse
{
  SIO->GPIO_OUT_CLR = LCD_CMD;                // 0 -> CMD ausführen
  delay_us(1);
  return(lcd_lesen());
} 


void lcd_impuls(void)                         // Schreibimpuls mit Mindestlaenge
{
  delay_us(1);
  SIO->GPIO_OUT_SET = LCD_STROBE;             // 1 = aktiv
  delay_us(1);
  SIO->GPIO_OUT_CLR = LCD_STROBE;             // und wieder auf 0
  delay_us(1);
}

void lcd_nibble(int8_t c)                     // 4-bit-Wert an GPIO10-GPIO13 ausgeben
{
  c >>= 4;
  c &= 0xf;
  SIO->GPIO_OUT_CLR = 0xf << LCD_DATA_OFFSET; // alle Datenleitungen auf 0
  SIO->GPIO_OUT_SET = c << LCD_DATA_OFFSET;   // danach aktive Bits setzen
  lcd_impuls();


}

void lcd_out(int8_t z, int8_t mode)           // Ausgabe von Daten (mode==1) oder CMDs (mode==0)
{
  lcd_busy = 1;
    while(lcd_status() & BUSY_FLAG);          // vor neuer Ausgabe busy-flag abwarten
    if(mode) SIO->GPIO_OUT_SET = LCD_CMD;
    else SIO->GPIO_OUT_CLR = LCD_CMD;
    lcd_nibble(z);                            // obere Bits zuerst
    lcd_nibble(z << 4);                       // untere Bits
    SIO->GPIO_OUT_SET = 0xf << LCD_DATA_OFFSET; // Datenleitungen auf '1'
    
  lcd_busy = 0;
}


// testen, ob LCD vorhanden
uint8_t lcd_test(void)               
{
uint8_t temp = 0;
  lcd_busy = 1;
  SIO->GPIO_OUT_SET = LCD_CMD;        // immer setzen, um Daten zu lesen
  delay_us(1);
  temp = lcd_lesen();
  lcd_busy = 0;
  return(temp);
}  

void lcd_cmd(uint8_t z)               // einen LCD-Befehl ausgeben
{
  lcd_out(z,0);
}

void lcd_zeichen(char z)            // ein Zeichen ausgeben
{
  lcd_out(z,1);
}

void lcd_string(char *s)            // Zeichenkette auf LCD
{   
int8_t temp;
  if(lcd_vorhanden) {
    while((temp=*s++) != 0) {
      lcd_zeichen(temp);
    }
  }
}

void lcd_init_nibble(int8_t c)      // 4-bit-Wert ausgeben LCD_INIT
{
  lcd_busy = 1;
  lcd_nibble(c);
  lcd_busy = 0;
  delay_us(LCD_NIBBLE_DELAY);       // jeweils 10 ms
}

void lcd_zeile1(char *s)            // komplette Zeile ausgeben
{
  if(lcd_vorhanden) {
    lcd_cmd(lcd_zeile_start[1]);    // 1.Zeile, 1.Spalte
    lcd_string(s);
  }
}
void lcd_zeile2(char *s)            // komplette Zeile ausgeben
{
  if(lcd_vorhanden) {
    lcd_cmd(lcd_zeile_start[2]);    // 2.Zeile, 1.Spalte
    lcd_string(s);
  }
}

void lcd_zeile3(char *s)            // komplette Zeile ausgeben
{
  if(lcd_vorhanden) {
    lcd_cmd(lcd_zeile_start[3]);    // 3.Zeile, 1.Spalte
    lcd_string(s);
  }
}
void lcd_zeile4(char *s)            // komplette Zeile ausgeben
{
  if(lcd_vorhanden) {
    lcd_cmd(lcd_zeile_start[4]);    // 4.Zeile, 1.Spalte
    lcd_string(s);
  }
}

void lcd_cursor_on()
{
  lcd_cmd(0xf);
}

void lcd_cursor_off()
{
  lcd_cmd(0xc);
}

void lcd_zeile_spalte(uint8_t zeile, uint8_t spalte)  // Schreibposition setzen
{
  if(zeile && spalte) lcd_cmd(zeilen_tab[zeile] + spalte);
}

void loesche_lcd_zeile(uint8_t zeile, uint8_t n)
{
uint8_t i;  
  if(lcd_vorhanden) {
    lcd_cmd(zeilen_tab[zeile] + 1);         // Startposition
    for(i=0; i<n; i++) lcd_zeichen(' ');    // Leerzeichen schreiben
  }  
}  

void lcd_clear()
{
  lcd_cursor_off();
  lcd_cmd(1);
}

void loesche_lcd_zeile1()
{
  loesche_lcd_zeile(1, lcd_breite);
}
void loesche_lcd_zeile2()
{
  loesche_lcd_zeile(2, lcd_breite);
}
void loesche_lcd_zeile12()
{
  loesche_lcd_zeile(1, lcd_breite);
  loesche_lcd_zeile(2, lcd_breite);
}

void loesche_lcd_zeile34()
{
  loesche_lcd_zeile(3, lcd_breite);
  loesche_lcd_zeile(4, lcd_breite);
}

void lcd_messwert_zeile1(char *s)         // komplette Zeile ausgeben
{
  if(lcd_vorhanden) {
    if(lcd_breite == DEF_LCD_BREITE)
      lcd_cmd(lcd_zeile_start[1]);        // 1.Zeile, 1.Spalte
    else
      lcd_cmd(lcd_zeile_start[1]-2);      // 1.Zeile, 1.Spalte
    lcd_string(s);
  }
}

void lcd_messwert_zeile2(char *s)         // komplette Zeile ausgeben
{
  if(lcd_vorhanden) {
    if(lcd_breite == DEF_LCD_BREITE)
      lcd_cmd(lcd_zeile_start[2]);        // 2.Zeile, 1.Spalte
    else
      lcd_cmd(lcd_zeile_start[2]-2);      // 2.Zeile, 1.Spalte
    lcd_string(s);
  }
}

void lcd_messwert_zeile3(char *s)         // komplette Zeile ausgeben
{
  if(lcd_vorhanden) {
    if(lcd_breite == DEF_LCD_BREITE)
      lcd_cmd(lcd_zeile_start[3]);        // 3.Zeile, 1.Spalte
    else
      lcd_cmd(lcd_zeile_start[3]-2);      // 3.Zeile, 1.Spalte
    lcd_string(s);
  }
}

void lcd_messwert_zeile4(char *s)         // komplette Zeile ausgeben
{
  if(lcd_vorhanden) {
    if(lcd_breite == DEF_LCD_BREITE)
      lcd_cmd(lcd_zeile_start[4]);        // 4.Zeile, 1.Spalte
    else
      lcd_cmd(lcd_zeile_start[4]-2);      // 4.Zeile, 1.Spalte
    lcd_string(s);
  }
}


void init_CGR()
{
  int i,j;
  lcd_cmd(0x40);          // set CGR-adr
  for(i = 0; i < MAX_CGR_CHAR; i++) {
    for(j = 0; j < CGR_CHAR_BYTES; j++) {
      lcd_zeichen(CGR[i][j]);
    }
  }
}  
  

void LCD_str_einmitten(char *s, int breite)
{
char temp_str[TEMP_STR_LEN];
int i, j, n;
  strcpy(temp_str, s);
  n = strlen(temp_str);
  while(temp_str[--n] == ' ')     // ' ' am Ende entfernen
    temp_str[n] = 0;
  i = 0;
  while(temp_str[i] == ' ')       // ' ' am Anfang entfernen
    i++;
  n = strlen(temp_str) - i;
  j = n = (breite-n)/2;
  while(j--)
    s[j] = ' ';                   // links ' ' auffuellen
  while(temp_str[i])
    s[n++] = temp_str[i++];
  while(n < breite)
    s[n++] = ' ';                 // rechts ' ' auffuellen
  s[n] = 0;
}  

uint8_t lcd_init(void)
{
// Enable GPIOs
  RESETS_CLR->RESET = RESETS_RESET_io_bank0_Msk | 
                      RESETS_RESET_pads_bank0_Msk;
  while((RESETS->RESET_DONE & (RESETS_RESET_io_bank0_Msk | RESETS_RESET_pads_bank0_Msk)) !=
        (RESETS_RESET_io_bank0_Msk | RESETS_RESET_pads_bank0_Msk));

  IO_BANK0->GPIO0_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_pio1_0;       // Kontrast-Signal als Ausgang zu SM3
  IO_BANK0->GPIO1_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;
  IO_BANK0->GPIO2_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;
  IO_BANK0->GPIO3_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;
  IO_BANK0->GPIO6_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;
  IO_BANK0->GPIO7_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;
  IO_BANK0->GPIO8_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;
  IO_BANK0->GPIO9_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;
  IO_BANK0->GPIO13_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;  // Licht

  
// Ausgänge push-pull output
  SIO->GPIO_OE_SET =  LCD_KONTRAST |  // PWM3 über PIO1_SM3
                      BACKLIGHT_ON | 
                      LCD_CMD | 
                      LCD_READ | 
                      LCD_STROBE;     // Steuerleitungen auf Ausgang
  SIO->GPIO_OE_SET = 0xf << LCD_DATA_OFFSET;// Datenleitungen auf Ausgang

  init_zeilen_breite(lcd_breite);

  delay_us(LCD_INIT_DELAY);   // 50 ms nach Reset

  lcd_init_nibble(0x30);      // 8 Bit Modus
  lcd_init_nibble(0x30);      // wiederholen
  lcd_init_nibble(0x30);      // wiederholen
  lcd_init_nibble(0x20);      // dann auf 4 Bit Modus
  lcd_cmd(0x28);              // 2 oder 4 Zeilen
  lcd_cmd(0x28);              // wiederholen

  lcd_cmd(0x8);               // LCD off            
  lcd_cmd(0x6);               // LCD auto. increment
  lcd_cmd(0x17);              // LCD char. mode; PWR on
  lcd_cmd(0x1);               // LCD clear
  lcd_cmd(0x2);               // LCD home
  
  lcd_cmd(0xc);               // LCD on; ohne Cusor und Blinken
  lcd_cmd(1);                 // LCD clear
  delay_us(LCD_INIT_DELAY);   // 50 ms nach Init
  init_CGR();
  loesche_lcd_zeile12();
  loesche_lcd_zeile34();
  setze_kontrast(lcd_kontrast);
//  SIO->GPIO_OUT_SET =  BACKLIGHT_ON;  // Beleuchtung einschalten
  
  return(lcd_status());
}

