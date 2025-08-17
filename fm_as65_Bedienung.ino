/*
2024-02-29
Michael Nowak
http://www.mino-elektronik.de
Alle Angaben ohne Gewaehr !
*/

#define FM_AS65_BEDIENUNG

#include "System-Pico-FM-AS65.h"

#define REPEAT_19     19        // Tasten Repeat-Schwellen
#define REPEAT_39     39        // 1,9 3,9 und 5,9 Sekunden
#define REPEAT_59     59
#define INC_DEC_1     1         // Startwert f�r +/-1 
#define INC_DEC_5     5         // inc-dec bei Repeat-Tasten
#define INC_DEC_10    10
#define INC_DEC_50    50
#define INC_DEC_100   100
#define INC_DEC_1000  300       // Geschwindigkeit nach Geschmack anpassen

uint8_t entprellte_tasten, akt_tasten;    	// zur externen Verwendung

static uint8_t    temp_tasten, letzte_aenderung, sperre;
static int8_t     rep_flag, entprell_ctr, rep_ctr;
volatile uint8_t  taste;

uint8_t     tast_tab[] = "ABCD    ";      // 1. Tastencode
uint8_t rep_tast_tab[] = "ab X    ";      // repeatcode teilweise zulassen
uint8_t deko_tab[] = {0,1,2,4,8,16,32,64,128};

void menue_F1(void);
void menue_F2(void);
void menue_allgemein(void);

uint8_t lese_taste_x(void) 
{
uint8_t temp;
  handle_ser_input();
  F1_messung(0);                // immer weiter messen
  sperre = 1;
  temp = taste;                 // Tastencode lesen
  taste=0;                      // und loeschen
  sperre = 0;
  return(temp);
}

uint8_t lese_taste(void)
{
static uint8_t temp_taste;
uint8_t temp = lese_taste_x();
  if(temp == ABBRUCH_TASTE) {
    rep_flag = 0;               // ohne repeat
    temp_taste = 0;
    return(temp);
  }
  if(temp == INIT_TASTE) {
    temp_taste = temp;          // merken
    return(0);                  // und noch abwarten
  }
  if(temp_taste == INIT_TASTE) {
    if((entprellte_tasten & INIT_BIT) == 0) {  // taste wurde losgelassen
      temp = temp_taste;
      temp_taste = 0;
      return(temp);
    } else temp = 0;            // ignorieren
  }
  return(temp);                 // alle anderen Tasten unver�ndert lassen
}

void dekodiere_tasten(void)
{
uint8_t n,  tasten_schalter, aenderung;
static uint8_t delay;

  if(lcd_busy) return;  // GPIO nicht lesbar
  
    delay++;
    if(delay < 10 || sperre) return;          // von 1 ms auf 10 ms Intervall oder sperren
    delay = 0;
    SIO->GPIO_OE_CLR = TASTEN_MASKE << TASTEN_OFFSET; // Ausgaenge abschalten
// Pullup einschalten und kleine Verzoegerung    
    PADS_BANK0->GPIO6 = PADS_BANK0_GPIO1_PUE_Msk | PADS_BANK0_GPIO1_IE_Msk | PADS_BANK0_GPIO1_SCHMITT_Msk; // Pullup = 1
    PADS_BANK0->GPIO7 = PADS_BANK0_GPIO1_PUE_Msk | PADS_BANK0_GPIO1_IE_Msk | PADS_BANK0_GPIO1_SCHMITT_Msk; // Pullup = 1
    PADS_BANK0->GPIO8 = PADS_BANK0_GPIO1_PUE_Msk | PADS_BANK0_GPIO1_IE_Msk | PADS_BANK0_GPIO1_SCHMITT_Msk; // Pullup = 1
    delay_us(1);    // bei LCD-Routinen
// Zustand lesen    
    akt_tasten = (SIO->GPIO_IN >> TASTEN_OFFSET & TASTEN_MASKE) ^ TASTEN_MASKE; // GPIO 6,7,8
// fuer LCD wieder auf Ausgang
    SIO->GPIO_OE_SET = TASTEN_MASKE << TASTEN_OFFSET; // Ausgaenge aktivieren

// in 'tasten_schalter' muss das Eingangsbitmuster stehen hier nur PortA
    tasten_schalter = deko_tab[akt_tasten];
    if(rep_ctr) rep_ctr--;                    // repeat-counter dekrementieren
    entprell_ctr++;                           // Entprell-counter inkrementieren
    if(tasten_schalter != temp_tasten) {      // Aenderung der Bits ?
      temp_tasten = tasten_schalter;          // ja, speichern
      entprell_ctr=0;                         // und neu entprellen
      return;                                 // mehr nicht
    }

    if(entprell_ctr < ENTPRELL_ZEIT) return;  // Entprellzeit abwarten
    entprell_ctr=0;                           // und neu starten
    // Tasten sind entprellt
    // jetzt auf positive Aenderungen testen
    aenderung = (temp_tasten ^ entprellte_tasten) & temp_tasten;
    entprellte_tasten = temp_tasten; // und letzten Zustand merken

    if(aenderung) {                           // falls neue Tasten aktiviert
      letzte_aenderung = aenderung;           // Aenderung merken
      rep_ctr = FIRST_REPEAT;                 // und 1.Repeat aktivieren
      rep_flag = 0;
    }
    else if(rep_ctr == 0 && rep_flag) {       // keine neue Taste aber repeat aktiv
      aenderung = entprellte_tasten & letzte_aenderung; // letzte Aenderung
      rep_ctr = NEXT_REPEAT;                  // wiederholen, mit kurzer repeat-Zeit
    }
    n=0;                          // Auswertung der Tasten
    do {                          // bit suchen
      if(aenderung & 1) {         // bit gefunden
        if(aenderung == 1) {      // und nur eine Taste erlaubt
          if(!rep_flag) {         // kein repeat: normale Codes verwenden
            n = tast_tab[n];
            rep_flag = 1;         // aber beim naechsten Mal repeat
          } else {
            n = rep_tast_tab[n];  // repeat ist schon aktiv
          }
          if(n != ' ') taste = n; // nur gueltigen Code speichern
          else rep_flag = 0;      // kein repeat, wenn keine gueltige taste
        }
        return;                   // taste erkannt, dekodierung fertig
      }
      n++;
      aenderung >>= 1;
    } while(n < MAX_BITS);
    return;                       // keine taste gefunden
}

// Anpassung f�r 20 Zeichen/Zeile
void relative_lcd_position(uint8_t zeile, uint8_t spalte)
{
uint8_t spalten_offset = (lcd_breite-MIN_LCD_BREITE)/2;  // ggf. 20 Zeichen
  lcd_zeile_spalte(zeile, spalte+spalten_offset);
}  

// PWM f�r Kontrast ver�ndern
int stelle_kontrast(void)
{
int c,i, repeat=0, inc_dec=1, abbruch=0;
int org_wert = lcd_kontrast;
char s[TEMP_STR_LEN];
const char *str[] = {
                "  LCD-Kontrast  ",
                "  LCD-contrast  "};
  loesche_lcd_zeile12();
  lcd_zeile1((char *)str[sprache]);
  i = lcd_kontrast;
  for(;;) {
    sprintf(s,"%3d",i);
    relative_lcd_position(2,7);
    lcd_string(s);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case PLUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_5;
	    if(i < MAX_LCD_KONTRAST) i += inc_dec;
        if(i >= MAX_LCD_KONTRAST) i = MAX_LCD_KONTRAST;
        setze_kontrast(i);
        break;
      case MINUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case MINUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_5;
	    if(i > MIN_LCD_KONTRAST) i -= inc_dec;
        if(i <= MIN_LCD_KONTRAST) i = MIN_LCD_KONTRAST;
        setze_kontrast(i);
        break;
      case PLUS_MINUS:
        setze_kontrast(org_wert);
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        if(i != org_wert) schreibe_kontrast(i);
	    return(abbruch);
    }
  }
}

// Blinkdauer von fertig-LED einstellen
int stelle_led_ein_zeit(void)
{
int c,i, repeat=0, inc_dec=1, abbruch = 0;
int org_wert = led_ein_zeit;
char s[TEMP_STR_LEN];
const char *str[] = {
                "  LED EIN (ms) ",
                "  LED ON (ms)  "};

  loesche_lcd_zeile12();
  lcd_zeile1((char *)str[sprache]);
  i = led_ein_zeit;
  for(;;) {
    sprintf(s,"%5d",i);
    relative_lcd_position(2,5);
    lcd_string(s);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case PLUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_100;
	    if(i < MAX_LED_EIN_ZEIT) i += inc_dec;
        if(i >= MAX_LED_EIN_ZEIT) i = MAX_LED_EIN_ZEIT;
        led_ein_zeit = i;
        break;
      case MINUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case MINUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_100;
	    if(i > MIN_LED_EIN_ZEIT) i -= inc_dec;
        if(i <= MIN_LED_EIN_ZEIT) i = MIN_LED_EIN_ZEIT;
        led_ein_zeit = i;
        break;
      case PLUS_MINUS:
        led_ein_zeit = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        if(i != org_wert) schreibe_led_zeit();
	    return(abbruch);
    }
  }
}

// feste Werte aus Tabelle per baudraten_index ausw�hlen
int stelle_baudrate(void)
{
int c,i, max_i, abbruch = 0;
int org_wert = baudraten_index;
const char *bd[] = {
                " ser. Baudrate",
                " ser. baudrate"};
  i = org_wert;
  loesche_lcd_zeile12();
  lcd_zeile1((char *)bd[sprache]);
  max_i = sizeof(baud_tab)/sizeof(*baud_tab)-1;
  for(;;) {
    relative_lcd_position(2,6);
    lcd_string((char *)baud_string[i]);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
	    if(i < max_i) {
	      i++;
	      set_baudrate1(baud_tab[i]);
	    }
	    break;
      case MINUS_TASTE:
	    if(i > 0) {
	      i--;
	      set_baudrate1(baud_tab[i]);
	    }
        break;
      case PLUS_MINUS:
        baudraten_index = org_wert;
        set_baudrate1(baud_tab[org_wert]);
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        if(i != org_wert) schreibe_baudraten_index(i);
	    return(abbruch);
    }
  }
}

int stelle_ser_ausgabe(void)
{
int c,i, max_i, abbruch = 0;
int org_wert = ser_ausgabe_kanal;
const char *text[][5] = {
               {" abgeschaltet ",
                " F1 Frequenz  ", 
                " F1 Periode   ", 
                " F1 Drehzahl  ",
                "F-Ref Frequenz"},
               {" switched off ",
                " F1 frequency ", 
                "  F1 periode  ", 
                "    F1 RPM    ",
                "Fref frequency"}};
const char *ausgabe[] = {
                "ser. Ausgabewert",
                "  serial output "};
  i = org_wert;
  max_i = MAX_SER_KANAL;
  if(as6501_aktiv) max_i--;              // keine F2 Anzeige
  if(i >= max_i) i = 0;
  loesche_lcd_zeile12();
  lcd_zeile1((char *)ausgabe[sprache]);
  for(;;) {
    relative_lcd_position(2,2);
    lcd_string((char *)text[sprache][i]);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
	    if(i < max_i-1) {
	      i++;
	      ser_ausgabe_kanal = i;
	    }
	    break;
      case MINUS_TASTE:
	    if(i > 0) {
	      i--;
	      ser_ausgabe_kanal = i;
	    }
        break;
      case PLUS_MINUS:
        ser_ausgabe_kanal = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        ee_write_uint8(EE_ADR_SER_AUSGABE, ser_ausgabe_kanal);
	    return(abbruch);
    }
  }
}

int stelle_anzeige_format(void)
{
int c,i, max_i, abbruch = 0;
int org_wert = anzeige_format;
const char *text[] = 
                {"1.23456789 Hz",
                 "1.23456789E+0", 
                 "1,23456789 Hz",
                 "1,23456789E+0"};
const char *format[] = 
                 {" Anzeigeformat  ",
                  " display format "};
  i = org_wert;
  max_i = sizeof(text)/sizeof(*text)-1;
  if(i > max_i) i = 0;
  loesche_lcd_zeile12();
  lcd_zeile1((char *)format[sprache]);
  for(;;) {
    relative_lcd_position(2,2);
    lcd_string((char *)text[i]);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
	    if(i < max_i) {
	      i++;
	      anzeige_format = i;
	    }
	    break;
      case MINUS_TASTE:
	    if(i > 0) {
	      i--;
	      anzeige_format = i;
	    }
        break;
      case PLUS_MINUS:
        anzeige_format = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        ee_write_uint8(EE_ADR_ANZEIGE_FORMAT, anzeige_format);
	    return(abbruch);
    }
  }
}


int korrektur_on_off(void)
{
int c,i, abbruch = 0;
int org_wert;
const char *ein_aus[][3] = {
                  {"lokal 12 MHz ",
                   " TCXO 10 MHz ",
                   "ext. Ref-Ein."},
                  {"local 12 MHz ",
                   " TCXO 10 MHz ",
                   "ext. Ref-inp."}};
                   
const char *src[] =
                  {"  Fref Quelle   ",
                   "  Fref source   "};
  if(korrektur_modus >= MAX_FREF_OPTION)
    korrektur_modus = LOKAL_12MHz;    // begrenzen falls zu gro�
  i = org_wert = korrektur_modus;
  loesche_lcd_zeile12();
  lcd_zeile1((char *)src[sprache]);
  for(;;) {
    relative_lcd_position(2,2);
    lcd_string((char *)ein_aus[sprache][i]);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
        i++;
        if(i >= MAX_FREF_OPTION)
          i = LOKAL_12MHz;
        break;
      case MINUS_TASTE:
        i--;
        if(i < LOKAL_12MHz)
          i = FREF_EXTERN;
        break;
      case PLUS_MINUS:
        korrektur_modus = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        korrektur_modus = i;
        if(i != org_wert) {
//          loesche_gps_filter();
          ee_write_uint8(EE_ADR_KORREKTUR_MODUS, korrektur_modus);
          stelle_Fref_mux(korrektur_modus);   // Multiplexer anpassen
        }  
 	    return(abbruch);
    }
  }
}

// Zeit f�r gleitenden Mittelwert einstellen: lokale Fref
int stelle_korrektur_filter(void)
{
int c,i, repeat=0, inc_dec=1, abbruch = 0;
int org_wert = abgleich_zeit;
char s[TEMP_STR_LEN];
const char *str[] = {
                "F-Ref Mittelwert",
                "Fref mean value "};
const char *strx[] = {
                "Zyklen: %4d s",
                "cycles: %4d s"};
  loesche_lcd_zeile12();
  lcd_zeile1((char *)str[sprache]);
  i = org_wert;
  for(;;) {
  sprintf(s,(char *)strx[sprache],i);
    lcd_zeile2(s);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case PLUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_50;
	    if(i < MAX_GPS_FILTER) i += inc_dec;
        if(i >= MAX_GPS_FILTER) i = MAX_GPS_FILTER;
        break;
      case MINUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case MINUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_50;
	    if(i > MIN_GPS_FILTER) i -= inc_dec;
        if(i <= MIN_GPS_FILTER) i = MIN_GPS_FILTER;
        break;
      case PLUS_MINUS:
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        if(i != org_wert) {
          abgleich_zeit = i;
          ee_write_uint32(EE_ADR_Fintern_ABGLEICH, abgleich_zeit);
          gps_fil_laenge = abgleich_zeit;
          loesche_gps_filter();       // neu starten
        }  
	    return(abbruch);
    }
  }
}


// Korrekturwert manuell einstellen: lokale Fref
int manueller_abgleich(void)
{
int c,i, repeat=0, inc_dec=1, abbruch = 0;
int org_wert = hole_F1_offset();
char s[TEMP_STR_LEN];
const char *str[] = {
                " manu. Abgleich ",
                "manu. correction"};
  loesche_lcd_zeile12();
  lcd_zeile1((char *)str[sprache]);
  i = org_wert;
  for(;;) {
    if(as6501_aktiv) {
      if(ex_ref_aktiv == 0) {
        sprintf(s," intern:%7d",i);
      } else {
        sprintf(s," extern:%7d",i);
      } 
    } else {
      switch(korrektur_modus) {
      case LOKAL_12MHz:
        sprintf(s," 12 MHz:%7d",i);
        break;
      case TCXO_10MHz:
        sprintf(s," TCXO:%7d",i);
        break;
      case FREF_EXTERN:
        sprintf(s," Fref:%7d",i);
        break;
      }
    }

    lcd_zeile2(s);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case PLUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_100;
        if(repeat >= REPEAT_59) inc_dec = INC_DEC_1000;
	    if(i < MAX_OFFSET) i += inc_dec;
        if(i >= MAX_OFFSET) i = MAX_OFFSET;
        F1_offset = 1;
        setze_F1_offset(i);
        break;
      case MINUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case MINUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_100;
        if(repeat >= REPEAT_59) inc_dec = INC_DEC_1000;
	    if(i > MIN_OFFSET) i -= inc_dec;
        if(i <= MIN_OFFSET) i = MIN_OFFSET;
        F1_offset = 1;
        setze_F1_offset(i);
        break;
      case PLUS_MINUS:
        i = org_wert;
        setze_F1_offset(i);
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        F1_offset = i;
        setze_F1_offset(i);
        speicher_EE_offset(i);
	    return(abbruch);
    }
  }
}

int stelle_sprache(void)
{
int c,i, max_i, abbruch = 0;
int org_wert = sprache;
const char *text[] = 
                {"deutsch",
                 "english"};
const char *format[] = 
                 {"    Sprache     ",
                  "    language    "};
  if(org_wert >= MAX_SPRACHE) org_wert = DT;
  i = org_wert;
  max_i = sizeof(text)/sizeof(*text)-1;
  if(i > max_i) i = 0;
  loesche_lcd_zeile12();
  for(;;) {
    lcd_zeile1((char *)format[sprache]);
    relative_lcd_position(2,5);
    lcd_string((char *)text[i]);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
	    if(i < max_i) {
	      i++;
	      sprache = i;
	    }
	    break;
      case MINUS_TASTE:
	    if(i > 0) {
	      i--;
	      sprache = i;
	    }
        break;
      case PLUS_MINUS:
        sprache = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        ee_write_uint8(EE_ADR_SPRACHE, sprache);
	    return(abbruch);
    }
  }
}


// an vorhandene Anzeige anpassen
int lcd_breite_16_20(void)
{
int c,i, abbruch = 0;
int org_wert = lcd_breite;
const char *ein_aus[][2] = {
               {"   16 Zeichen", "   20 Zeichen"},
               {"   16 digits ", "   20 digits "}};
const char *str[] = {
                "   LCD-Breite   ",
                "   LCD-width    "};
  i = org_wert/20;    // 0 oder 1
  for(;;) {
    loesche_lcd_zeile12();
    lcd_zeile1((char *)str[sprache]);
    lcd_zeile2((char *)ein_aus[sprache][i]);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
        if(i == 1) break;
        i=1;
        lcd_breite = 20;
        init_zeilen_breite(lcd_breite);
        loesche_lcd_zeile34();
	    break;
      case MINUS_TASTE:
        if(i == 0) break;
        i=0;
        loesche_lcd_zeile34();
        lcd_breite = 16;
        init_zeilen_breite(lcd_breite);
        break;
      case PLUS_MINUS:
        lcd_breite = org_wert;
        init_zeilen_breite(lcd_breite);
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        if(lcd_breite != org_wert) {
          loesche_lcd_zeile34();
          ee_write_uint8(EE_ADR_LCD_BREITE, lcd_breite);
        }  
	    return(abbruch);
    }
  }
}

// spezielles Format f�r Messwertanzeige
int MHz_on_off(void)
{
int c,i, abbruch = 0;
int org_wert = nur_MHz;
const char *ein_aus[][2]= {
               {"auto. Skalierung",
                "    nur MHz     "},
               {" auto. scaling  ",
               "    MHz only     "}};
const char *str[] = {
                "Frequenzbereich ",
                "frequency range "};
  i = org_wert;
  loesche_lcd_zeile12();
  lcd_zeile1((char *)str[sprache]);
  for(;;) {
    relative_lcd_position(2,1);
    lcd_string((char *)ein_aus[sprache][i]);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
      case MINUS_TASTE:
        i ^= 1;
        break;
      case PLUS_MINUS:
        nur_MHz = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        nur_MHz = i;
        if(i != org_wert) {
          ee_write_uint8(EE_ADR_NUR_MHZ, nur_MHz);
        }  
 	    return(abbruch);
    }
  }
}

// spezielles Format f�r Messwertanzeige
int stelle_neustart(void)
{
int c,i, abbruch = 0;
int org_wert = as6501_sperre;
const char *ein_aus[][2]= {
               {"belassen ", "Neustart "},
               {"continue ", " reboot  "}};
const char *str[][2] = {
               {"  AS6501 aktiv  ",
                "  AS6501 passiv "},
               {" AS6501 enabled ",
                "AS6501 disabled "}};
const char *strx[] = {
                "Ausschalten und ",
                " Power OFF and  "};
const char *stry[] = {
                "dann Einschalten",
                " Power ON again "};
  i = org_wert;
  loesche_lcd_zeile12();
  for(;;) {
    lcd_zeile1((char *)str[sprache][i]);

    relative_lcd_position(2,5);
    lcd_string((char *)ein_aus[sprache][(i == as6501_aktiv)]);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
      case MINUS_TASTE:
        i ^= 1;
        break;
      case PLUS_MINUS:
//        nur_MHz = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	      abbruch = 1;
      case INIT_TASTE:
        if(i != org_wert) {
          as6501_sperre = i;
          schreibe_as65_sperre();
          loesche_lcd_zeile12();
          lcd_zeile1((char *)strx[sprache]);
          lcd_zeile2((char *)stry[sprache]);
          while(1);
        }  
 	    return(abbruch);
    }
  }
}


// Grundeinstellungen
void menue_allgemein(void)
{
static uint8_t marke;
  for(;;) {
    switch(marke) {
      case(0):
        if(stelle_ser_ausgabe()) return;
        marke++;
        break;
      case(1):
        if(stelle_led_ein_zeit()) return;
        marke++;
        break;
      case(2):      
        if(stelle_baudrate()) return;
        marke++;
        if(as6501_aktiv) marke += 2;          // GPS-Einstellungen ueberspringen
        break;
      case(3):      
        if(korrektur_on_off()) return;
        marke++;
        if(korrektur_modus == LOKAL_12MHz) marke++; // keine Filter-Zyklen einstellen
        break;
      case(4):      
        if(stelle_korrektur_filter()) return;
        marke++;
        break;
      case(5):      
        if(manueller_abgleich()) return;
        marke ++;
        break;
      case(6):  
        if(stelle_sprache()) return;
        marke ++;
        break;
      case(7):      
        if(lcd_breite_16_20()) return;
        marke++;
        break;
      case(8):      
        if(stelle_kontrast()) return;
        marke++;
        break;
      case(9):
        if(stelle_anzeige_format()) return;
        marke++;
        break;
      case(10):
        if(MHz_on_off()) return;
        marke++;
        break;
      case(11):
        if(as6501_bestueckt) {
          if(stelle_neustart()) return;
          marke++;
        }  
        break;
      default:
        marke = 0;    // wieder auf Anfang
    }
  }
}

// Bedienmenue f�r manuelle Einstellungen
void FM_AS65_hauptmenue(void)
{
static uint8_t key, temp;
#define MAX_MENUE 3
const char *strx[][3] = {
               {"   Eingang F1   ",
                " Eingang F-Ref  ",
                "   allgemein    "},
               {"   F1 channel   ",
                " F-ref channel  ",
                "common settings "}};
static void (*menue_funktion[])(void) = {menue_F1, menue_F2, menue_allgemein};
const char *str[] = {
                " Einstellungen  ",
                "    settings    "};

  loesche_lcd_zeile12();
  do {
    lcd_zeile1((char *)str[sprache]);
    lcd_zeile2((char *)strx[sprache][key]);
    while((temp = lese_taste()) == 0);
    switch(temp) {
      case PLUS_TASTE:
        key++;
        if(as6501_aktiv && key == 1) key++;         // F2-Menue ausblenden
        if(key >= MAX_MENUE) key = 0;
      break;
      case MINUS_TASTE:
        if(key) key--;
        if(as6501_aktiv && key == 1) key--;         // F2-Menue ausblenden
        else key = MAX_MENUE-1;
      break;
      case INIT_TASTE:
        menue_funktion[key]();      // angew�hltes Menue aufrufen
      case ABBRUCH_TASTE:
      case PLUS_MINUS:
        return;
    }
  }while(1);
}  
      
int stelle_F1_messzeit(void)
{
int c,i, repeat=0, inc_dec=1, abbruch = 0;
int org_wert = F1_messzeit;
char s[TEMP_STR_LEN];
const char *str[] = {
                "  F1 Messzeit  ",
                "  F1 meas.time "};
  loesche_lcd_zeile12();
  lcd_zeile1((char *)str[sprache]);
  i = org_wert;
  for(;;) {
    sprintf(s,"%5d",i);
    relative_lcd_position(2,5);
    lcd_string(s);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case PLUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_100;
	    if(i < MAX_MESSZEIT) i += inc_dec;
        if(i >= MAX_MESSZEIT) i = MAX_MESSZEIT;
        F1_messzeit = i;
        break;
      case MINUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case MINUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_100;
	    if(i > MIN_MESSZEIT) i -= inc_dec;
        if(i <= MIN_MESSZEIT) i = MIN_MESSZEIT;
        F1_messzeit = i;
        break;
      case PLUS_MINUS:
        F1_messzeit = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        if(i != org_wert) {
          F1_messzeit = i;
          ee_write_uint32(EE_ADR_F1_MESSZEIT, F1_messzeit);
        } 
	    return(abbruch);
    }
  }
}
   

   
int stelle_F1_timeout(void)
{
int c,i, repeat=0, inc_dec=1, abbruch = 0;
int org_wert = F1_timeout;
char s[TEMP_STR_LEN];
const char *str[] = {
                "  F1 Timeout  ",
                "  F1 timeout  "};
  loesche_lcd_zeile12();
  lcd_zeile1((char *)str[sprache]);
  i = org_wert;
  for(;;) {
    sprintf(s,"%5d",i);
    relative_lcd_position(2,5);
    lcd_string(s);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case PLUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_100;
	    if(i < MAX_MESSZEIT) i += inc_dec;
        if(i >= MAX_MESSZEIT) i = MAX_MESSZEIT;
        F1_timeout = i;
        break;
      case MINUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case MINUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_100;
	    if(i > MIN_MESSZEIT) i -= inc_dec;
        if(i <= MIN_MESSZEIT) i = MIN_MESSZEIT;
        F1_timeout = i;
        break;
      case PLUS_MINUS:
        F1_timeout = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        if(i != org_wert) {
          F1_timeout = i;
          ee_write_uint32(EE_ADR_F1_TIMEOUT, F1_timeout);
        } 
	    return(abbruch);
    }
  }
}

// f�r Vorteiler, falls verwendet
int stelle_F1_faktor(void)
{
int c,i, repeat=0, inc_dec=1, abbruch = 0;
int org_wert = F1_faktor;
char s[TEMP_STR_LEN];
const char *str[] = {
                "   F1 Faktor  ",
                "   F1 factor  "};
  loesche_lcd_zeile12();
  lcd_zeile1((char *)str[sprache]);
  i = org_wert;
  for(;;) {
    sprintf(s,"%5d",i);
    relative_lcd_position(2,5);
    lcd_string(s);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case PLUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_100;
	    if(i < MAX_FAKTOR) i += inc_dec;
        if(i >= MAX_FAKTOR) i = MAX_FAKTOR;
        F1_faktor = i;
        break;
      case MINUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case MINUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_100;
	    if(i > MIN_FAKTOR) i -= inc_dec;
        if(i <= MIN_FAKTOR) i = MIN_FAKTOR;
        F1_faktor = i;
        break;
      case PLUS_MINUS:
        F1_faktor = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        if(i != org_wert) {
          F1_faktor = i;
          ee_write_uint32(EE_ADR_F1_FAKTOR, F1_faktor);
        } 
	    return(abbruch);
    }
  }
}
  
// nur bei UPM aktiv
int stelle_F1_upm_divisor(void)
{
int c,i, repeat=0, inc_dec=1, abbruch = 0;
int org_wert = F1_upm_divisor;
char s[TEMP_STR_LEN];
const char *str[] = {
                " F1 UPM-Divisor",
                " F1 rpm-divisor"};
  loesche_lcd_zeile12();
  lcd_zeile1((char *)str[sprache]);
  i = org_wert;
  for(;;) {
    sprintf(s,"%5d",i);
    relative_lcd_position(2,5);
    lcd_string(s);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case PLUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_100;
	    if(i < MAX_UPM_DIVISOR) i += inc_dec;
        if(i >= MAX_UPM_DIVISOR) i = MAX_UPM_DIVISOR;
        F1_upm_divisor = i;
        break;
      case MINUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case MINUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_100;
	    if(i > MIN_UPM_DIVISOR) i -= inc_dec;
        if(i <= MIN_UPM_DIVISOR) i = MIN_UPM_DIVISOR;
        F1_upm_divisor = i;
        break;
      case PLUS_MINUS:
        F1_upm_divisor = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        if(i != org_wert) {
          F1_upm_divisor = i;
          ee_write_uint32(EE_ADR_F1_UPM_DIV, F1_upm_divisor);
        } 
	    return(abbruch);
    }
  }
}
   
int stelle_F1_stellen(void)
{
int c,i, abbruch = 0;
int org_wert = F1_stellen;
char s[TEMP_STR_LEN];
const char *str[] = {
                "  F1 Stellen  ",
                "   F1 digits  "};
const char *strx[] = {
                "  automatisch  ",
                " automatically "};
  loesche_lcd_zeile12();
  lcd_zeile1((char *)str[sprache]);
  i = org_wert;
  for(;;) {
    
    if(i) {                               // bei fester Stellenzahl
      sprintf(s,"  %5d     ",i);
      relative_lcd_position(2,2);
      lcd_string(s);
    }  
    else     // bei Stellenanzahl == 0
      lcd_zeile2((char *)strx[sprache]);
    

    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
	    if(i < F1_MAX_STELLEN) i++;
        if(i < MIN_STELLEN) i = MIN_STELLEN;
        if(i >= F1_MAX_STELLEN) i = F1_MAX_STELLEN;
        F1_stellen = i;
        break;
      case MINUS_TASTE:
	    if(i >= MIN_STELLEN) i--;
        if(i < MIN_STELLEN) i = 0;   // automatisch
        F1_stellen = i;
        break;
      case PLUS_MINUS:
        F1_stellen = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        if(i != org_wert) {
          F1_stellen = i;
          ee_write_uint32(EE_ADR_F1_STELLEN, F1_stellen);
        } 
	    return(abbruch);
    }
  }
}
   
// Multiplikator einschalten
int F1_vorteiler_on_off(void)
{
int c,i, abbruch = 0;
int org_wert = F1_vorteiler_aktiv;
const char *ein_aus[][2] = {
               {" aus ", "aktiv"},
               {" off ", "activ"}};
const char *str[] = {
                "  F1 Vorteiler  ",
                "  F1 prescaler  "};
  i = org_wert;
  loesche_lcd_zeile12();
  lcd_zeile1((char *)str[sprache]);
  for(;;) {
    relative_lcd_position(2,6);
    lcd_string((char *)ein_aus[sprache][i]);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
      case MINUS_TASTE:
        i ^= 1;
        break;
      case PLUS_MINUS:
        F1_vorteiler_aktiv = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        F1_vorteiler_aktiv = i;
        if(i != org_wert) {
          ee_write_uint8(EE_ADR_F1_VORTEILER, F1_vorteiler_aktiv);
        }  
	    return(abbruch);
    }
  }
}

// Multiplikator einschalten
int F1_vorteiler_umschaltung(void)
{
int c,i, abbruch = 0;
int org_wert = auto_vorteiler_umschaltung;
const char *ein_aus[][2] = {
               {" weiter messen  ", "    Neustart    "},
               {"   continue     ", "start new meas. "}};
const char *str[] = {
                "F1 +/- Vorteiler",
                "F1 +/- prescaler"};
  i = org_wert;
  loesche_lcd_zeile12();
  lcd_zeile1((char *)str[sprache]);
  for(;;) {
    relative_lcd_position(2,1);
    lcd_string((char *)ein_aus[sprache][i]);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
      case MINUS_TASTE:
      i ^= 1;
      break;
      case PLUS_MINUS:
        auto_vorteiler_umschaltung = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        auto_vorteiler_umschaltung = i;
        if(i != org_wert) {
          ee_write_uint8(EE_ADR_F1_AUTO_VORTEILER, auto_vorteiler_umschaltung);
        }  
	    return(abbruch);
    }
  }
}

int F1_regression_on_off(void)
{
int c,i, abbruch = 0;
int org_wert = F1_reg_modus;
const char *ein_aus[][2] = {
               {" aus ", "aktiv"},
               {" off ", "activ"}};
const char *str[] = {
                "lin. Regression",
                "lin. regression"};
  if(org_wert > OPTIMIERT) org_wert = OPTIMIERT;
  i = org_wert;
  loesche_lcd_zeile12();
  lcd_zeile1((char *)str[sprache]);
  for(;;) {
    relative_lcd_position(2,6);
    lcd_string((char *)ein_aus[sprache][i]);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
	      i++;
        if(i > OPTIMIERT) i = 0;
	    break;
      case MINUS_TASTE:
	      i--;
        if(i < 0) i = OPTIMIERT;
        break;
      case PLUS_MINUS:
        F1_reg_modus = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        F1_reg_modus = i;
        if(i != org_wert) {
          ee_write_uint8(EE_ADR_F1_REGRESSION, F1_reg_modus);
        }  
 	    return(abbruch);
    }
  }
}

// manuelle Einstellungen f�r Kanal F1    
void menue_F1(void)
{
static uint8_t marke;
  for(;;) {
    switch(marke) {
      case(0):
        if(stelle_F1_messzeit()) return;
        marke++;
        break;
      case(1):
        if(stelle_F1_timeout()) return;
        marke++;
        break;
      case(2):      
        if(stelle_F1_stellen()) return;
        marke++;
        break;
      case(3):      
        if(F1_vorteiler_umschaltung()) return;
        marke++;
        break;
      case(4):      
        if(F1_vorteiler_on_off()) return;
        if(!F1_vorteiler_aktiv) marke++;   // Vorteiler bei Bedarf
        marke++;
        break;
      case(5):      
        if(stelle_F1_faktor()) return;
        marke++;
        break;
      case(6):      
        if(stelle_F1_upm_divisor()) return;
        marke++;
        break;
      case(7):      
        if(F1_regression_on_off()) return;
        marke++;
        break;
      default:
        marke = 0;    // wieder auf Anfang
    }
  }
}


int stelle_F2_messzeit(void)
{
int c,i, repeat=0, inc_dec=1, abbruch = 0;
int org_wert = F2_messzeit;
char s[TEMP_STR_LEN];
const char *str[] = {
                " F-Ref Messzeit  ",
                " F-ref meas.time "};
  loesche_lcd_zeile12();
  lcd_zeile1((char *)str[sprache]);
  i = org_wert;
  for(;;) {
    sprintf(s,"%5d",i);
    relative_lcd_position(2,5);
    lcd_string(s);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case PLUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_100;
	    if(i < MAX_MESSZEIT) i += inc_dec;
        if(i >= MAX_MESSZEIT) i = MAX_MESSZEIT;
        F2_messzeit = i;
        break;
      case MINUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case MINUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_100;
	    if(i > MIN_MESSZEIT) i -= inc_dec;
        if(i <= MIN_MESSZEIT) i = MIN_MESSZEIT;
        F2_messzeit = i;
        break;
      case PLUS_MINUS:
        F2_messzeit = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        if(i != org_wert) {
          F2_messzeit = i;
          ee_write_uint32(EE_ADR_F2_MESSZEIT, F2_messzeit);
        } 
	    return(abbruch);
    }
  }
}
   
int stelle_F2_timeout(void)
{
int c,i, repeat=0, inc_dec=1, abbruch = 0;
int org_wert = F2_timeout;
char s[TEMP_STR_LEN];
const char *str[] = {
                " F-Ref Timeout  ",
                " F-ref timeout  "};
  loesche_lcd_zeile12();
  lcd_zeile1((char *)str[sprache]);
  i = org_wert;
  for(;;) {
    sprintf(s,"%5d",i);
    relative_lcd_position(2,5);
    lcd_string(s);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case PLUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_100;
	    if(i < MAX_MESSZEIT) i += inc_dec;
        if(i >= MAX_MESSZEIT) i = MAX_MESSZEIT;
        F2_timeout = i;
        break;
      case MINUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case MINUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_100;
	    if(i > MIN_MESSZEIT) i -= inc_dec;
        if(i <= MIN_MESSZEIT) i = MIN_MESSZEIT;
        F2_timeout = i;
        break;
      case PLUS_MINUS:
        F2_timeout = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        if(i != org_wert) {
          F2_timeout = i;
          ee_write_uint32(EE_ADR_F2_TIMEOUT, F2_timeout);
        } 
	    return(abbruch);
    }
  }
}

int stelle_F2_faktor(void)
{
int c,i, repeat=0, inc_dec=1, abbruch = 0;
int org_wert = F2_faktor;
char s[TEMP_STR_LEN];
const char *str[] = {
                "  F-Ref Faktor  ",
                "  F-ref factor  "};
  loesche_lcd_zeile12();
  lcd_zeile1((char *)str[sprache]);
  i = org_wert;
  for(;;) {
    sprintf(s,"%5d",i);
    relative_lcd_position(2,5);
    lcd_string(s);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case PLUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_100;
	    if(i < MAX_FAKTOR) i += inc_dec;
        if(i >= MAX_FAKTOR) i = MAX_FAKTOR;
        F2_faktor = i;
        break;
      case MINUS_TASTE:
        inc_dec = INC_DEC_1;        // Startwert f�r +/-1
        repeat = 0;
      case MINUS_REPEAT:  
        repeat++;
        if(repeat >= REPEAT_19) inc_dec = INC_DEC_10;
        if(repeat >= REPEAT_39) inc_dec = INC_DEC_100;
	    if(i > MIN_FAKTOR) i -= inc_dec;
        if(i <= MIN_FAKTOR) i = MIN_FAKTOR;
        F2_faktor = i;
        break;
      case PLUS_MINUS:
        F2_faktor = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        if(i != org_wert) {
          F2_faktor = i;
          ee_write_uint32(EE_ADR_F2_FAKTOR, F2_faktor);
        } 
	    return(abbruch);
    }
  }
}
   
int stelle_F2_stellen(void)
{
int c,i, abbruch = 0;
int org_wert = F2_stellen;
char s[TEMP_STR_LEN];
const char *str[] = {
                " F-Ref Stellen ",
                " F-ref digits  "};
const char *strx[] = {
                "  automatisch  ",
                " automatically "};
  loesche_lcd_zeile12();
  lcd_zeile1((char *)str[sprache]);
  i = org_wert;
  for(;;) {
    
    if(i) {
      sprintf(s,"  %5d    ",i);
      relative_lcd_position(2,2);
      lcd_string(s);
    }  
    else lcd_zeile2((char *)strx[sprache]);
    

    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
	    if(i < F2_MAX_STELLEN) i++;
        if(i < MIN_STELLEN) i = MIN_STELLEN;
        if(i >= F2_MAX_STELLEN) i = F2_MAX_STELLEN;
        F2_stellen = i;
        break;
      case MINUS_TASTE:
	    if(i >= MIN_STELLEN) i--;
        if(i < MIN_STELLEN) i = 0;   // automatisch
        F2_stellen = i;
        break;
      case PLUS_MINUS:
        F2_stellen = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
	    abbruch = 1;
      case INIT_TASTE:
        if(i != org_wert) {
          F2_stellen = i;
          ee_write_uint32(EE_ADR_F2_STELLEN, F2_stellen);
        } 
	    return(abbruch);
    }
  }
}
   

int F2_vorteiler_on_off(void)
{
int c,i, abbruch = 0;
int org_wert = F2_vorteiler_aktiv;
const char *ein_aus[][2] = {
               {" aus ", "aktiv"},
               {" off ", "activ"}};
const char *str[] = {
                "F-Ref Vorteiler ",
                "F-ref prescaler "};
  i = org_wert;
  loesche_lcd_zeile12();
  lcd_zeile1((char *)str[sprache]);
  for(;;) {
    relative_lcd_position(2,6);
    lcd_string((char *)ein_aus[sprache][i]);
    while((c=lese_taste()) == 0);
    switch(c) {
      case PLUS_TASTE:
      case MINUS_TASTE:
        i ^= 1;
        break;
      case PLUS_MINUS:
        F2_vorteiler_aktiv = org_wert;
        return(1);                    // Abbruch ohne �nderungen
      case ABBRUCH_TASTE:
        abbruch = 1;
      case INIT_TASTE:
        F2_vorteiler_aktiv = i;
        if(i != org_wert) {
          ee_write_uint8(EE_ADR_F2_VORTEILER, F2_vorteiler_aktiv);
        }  
        return(abbruch);
    }
  }
}

    
void menue_F2(void)
{
static uint8_t marke;
  for(;;) {
    switch(marke) {
      case(0):
        if(stelle_F2_messzeit()) return;
        marke++;
        break;
      case(1):
        if(stelle_F2_timeout()) return;
        marke++;
        break;
      case(2):      
        if(stelle_F2_stellen()) return;
        marke++;
        break;
      case(3):      
        if(F2_vorteiler_on_off()) return;
        if(!F2_vorteiler_aktiv) marke++;   // Vorteiler bei Bedarf
        marke++;
        break;
      case(4):      
        if(stelle_F2_faktor()) return;
        marke++;
        break;
      default:
        marke = 0;    // wieder auf Anfang
    }
  }
}
