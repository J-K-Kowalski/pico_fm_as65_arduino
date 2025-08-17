/*
2024-03-08
Michael Nowak
http://www.mino-elektronik.de
Alle Angaben ohne Gewaehr !
*/

#define FM_AS65_SERIO_INPUT

#include "System-Pico-FM-AS65.h"

int zahl, zahl1, zahl2, zahl3;          // durch ',' getrennte, eingelesene Wert
static uint8_t ser_status;              // Startkennung (ESC, '.') und Vorzeichen
void handle_ser_input(void);                // scanner zur Auswertung der Eingaben

void sende_cr_lf(void)
{
  uart1_putc(13);
  uart1_putc(10);
}


void long2a(int l, char *s, char mit0)
{
const uint32_t tab[] = {1000000000, 100000000, 10000000, 1000000,
	       100000, 10000, 1000, 100, 10, 1, 0};
uint32_t temp;
char i=0,j;
  if(l) {
    if(l < 0) {
      l = -l;
      *s++ = '-';             // add sign
    }
    if(mit0) {                
      i=2;
      while(tab[i] > l) {
        i++;
        *s++ = '0';           // add '0's
      }
    } else
      while(tab[i] > l) i++;  // or no leading '0's
    do {
      temp = tab[i];
      j = '0';
      if(temp) {
        for(;j <= '9'; j++) {
          l-=temp;            // convert digit
          if(l < 0) {
            l += temp;        // correct underflow
            break;            // and continue with next digit
          }
        }
        *s++ = j;             // save digit
        i++;
      }
    } while(temp);            // until done
  } else *s++ = '0';          // if l == 0
  *s = 0;                     // finish string
}

void sende_zahl(long l)
{
char s[TEMP_STR_LEN];
int i=0;
  long2a(l,s,0);
  while(s[i]) uart1_putc(s[i++]);
}

void sende_integer(long l)
{
char s[TEMP_STR_LEN];
  long2a(l,s,0);
  sende_str1(s);
}


void handle_ser_input(void)
{
static char ziffern;
char s[TEMP_STR_LEN];
int temp;
  if((temp = getchar_rp()) == EOF) {    // Zeichen per UART
    if((temp = USB_getc()) == EOF)      // oder per USB
      return;	                          // kein zeichen
  }  
  if(temp == ESC || temp == '.') {	    // Datensatz beginnt mit ESC oder '.'
    ser_status = ESC_BIT;
    zahl = zahl1 = zahl2 = zahl3 = 0;
    ziffern = 0;
    return;
  }
  if((ser_status & ESC_BIT) == 0) { // ESC fehlt
    ser_status = 0;
    ziffern = 0;
    zahl=0; 
    return;
  }  
  if(temp==MINUS) {
    ser_status = ESC_BIT+MINUS_VORZEICHEN;
    zahl = 0;
    ziffern = 1;
    return;
  }
  if(temp>= '0' && temp<= '9') {
    ziffern++;
    zahl = zahl*10 + (temp & 0xf);	/* x 10 + neue Ziffer */
    return;
  }
  if(temp == ',') {
    zahl3 = zahl2;
    zahl2 = zahl1;
    zahl1 = zahl;
    zahl = 0;
    ser_status &= ~MINUS_VORZEICHEN;
    return;
  }
// ab hier die Befehle
// zunächst kleine Buchstaben und Sonderzeichen auswerten  
  if(temp == CTRL_S) {    // Speichern der Offsets nur mit extra Steuerzeichen
      ee_write_uint32(EE_ADR_Fintern_OFFSET, Fintern_offset);
      ee_write_uint32(EE_ADR_Fextern_OFFSET, Fextern_offset);
  }
  if(temp == BEF_F1_AUTO_VORTEILER) { // sehr speziell
      if(ziffern) {
        if(zahl >=0 && zahl <= 1) {
          auto_vorteiler_umschaltung = zahl;
          ee_write_uint8(EE_ADR_F1_AUTO_VORTEILER, auto_vorteiler_umschaltung);
        }  
      } else {
        uart1_putc(temp);
        sende_integer(auto_vorteiler_umschaltung);
      }
  } 
  
// Befehle als Klein- oder Grossbuchstaben  
  temp = toupper(temp);
  switch(temp) {
    case BEF_SYNC:
      uart1_putc(temp);
      sende_cr_lf();
      break;
    case BEF_STATISTIK:
      if(ziffern) {
        switch(zahl) {
          case 0:
            init_stat();
            if(anzeige_funktion >= FMEAN_UND_N) {
              update_LCD = 1;
            }
          break;
          case 1:
            uart1_putc('+');
            sende_integer(stat_n);
          break;
          case 2:
            wandel_Fx("+",stat_mean, 0, s, stat_F1_stellen);
            sende_str1(s);
          break;
          case 3:
            wandel_Fx("+",stat_max, 0, s, stat_F1_stellen);
            sende_str1(s);
          break;
          case 4:
            wandel_Fx("+",stat_min, 0, s, stat_F1_stellen);
            sende_str1(s);
          break;
          case 5:
            temp = anzeige_format;
            anzeige_format |= 1;
            wandel_Fx("+",stat_sdev(), 0, s, stat_F1_stellen);
            anzeige_format = temp;
            sende_str1(s);
          break;
        }  
      } else {
        uart1_putc('+');
        sende_integer(stat_n);
        wandel_Fx("+",stat_mean, 0, s, stat_F1_stellen);
        sende_str1(s);
        wandel_Fx("+",stat_max, 0, s, stat_F1_stellen);
        sende_str1(s);
        wandel_Fx("+",stat_min, 0, s, stat_F1_stellen);
        sende_str1(s);
        temp = anzeige_format;
        anzeige_format |= 1;
        wandel_Fx("+",stat_sdev(), 0, s, stat_F1_stellen);
        anzeige_format = temp;
        sende_str1(s);
      }
      break;
  
    case  BEF_OFFSET:
      if(ziffern) {
        if(zahl == 0) {
          F1_offset = 0;	// loeschen
        } else {  
          if(ser_status&MINUS_VORZEICHEN) zahl = -zahl;
          F1_offset += zahl;		// relativer Wert
        } 
        setze_F1_offset(F1_offset);
      } else {
        uart1_putc(temp);
        sende_integer(F1_offset);
      }
      break;
   
    case BEF_LED_EIN_ZEIT:
      if(ziffern) {
        if(zahl >= MIN_LED_EIN_ZEIT && zahl < MAX_LED_EIN_ZEIT) {
          led_ein_zeit = zahl;
          schreibe_led_zeit();
        }  
      } else {
        uart1_putc(temp);
        sende_integer(led_ein_zeit);
      }
      break;
 
    case BEF_GPS_FIL_INTERN:
      if(ziffern) {
        if(zahl < MIN_GPS_FILTER) zahl = MIN_GPS_FILTER;	// ist minimum Abgleich abschalten
        if(zahl > MAX_GPS_FILTER) zahl = MAX_GPS_FILTER;	// ist maximum
        if(zahl != abgleich_zeit) {
          ee_write_uint32(EE_ADR_Fintern_ABGLEICH, zahl);
          abgleich_zeit = zahl;
        }
        if(ex_ref_aktiv == 0) {
          gps_fil_laenge = abgleich_zeit = zahl;          // Länge anpasse
          loesche_gps_filter();                           // neu starten
        }  
      } else {
        uart1_putc(temp);
        sende_integer(abgleich_zeit);
      }
      break;
    case BEF_GPS_AKTIV:
      if(ziffern) {
        if(zahl >= LOKAL_12MHz && zahl < MAX_FREF_OPTION && zahl != korrektur_modus) {
          korrektur_modus = zahl;
          ee_write_uint8(EE_ADR_KORREKTUR_MODUS, korrektur_modus);
          gps_fil_laenge = abgleich_zeit;               // Länge anpasse
          loesche_gps_filter();                         // neu starten
        }  
      } else {
        uart1_putc(temp);
        sende_integer(korrektur_modus);
      }
      break;      
      
      
      
      
      
    case BEF_F1_MESSZEIT:
      if(ziffern) {
        if(zahl < MAX_MESSZEIT) {
          F1_messzeit = zahl;
          ee_write_uint32(EE_ADR_F1_MESSZEIT, F1_messzeit);
        }  
      } else {
        uart1_putc(temp);
        sende_integer(F1_messzeit);
      }
      break;
    case BEF_F1_TIMEOUT:
      if(ziffern) {
        if(zahl < MAX_TIMEOUT) {
          F1_timeout = zahl;
          ee_write_uint32(EE_ADR_F1_TIMEOUT, F1_timeout);
        }  
      } else {
        uart1_putc(temp);
        sende_integer(F1_timeout);
      }
      break;
    case BEF_F1_STELLEN:
      if(ziffern) {
        if(!zahl || zahl <= F1_MAX_STELLEN && zahl >= MIN_STELLEN) {
          F1_stellen = zahl;
          ee_write_uint32(EE_ADR_F1_STELLEN, F1_stellen);
        }
      } else {
        uart1_putc(temp);
        sende_integer(F1_stellen);
      }
      break;
  
  
    case BEF_F1_FAKTOR:
      if(ziffern) {
        if(zahl < MAX_FAKTOR) {
          F1_faktor = zahl;
          ee_write_uint32(EE_ADR_F1_FAKTOR, F1_faktor);
        }  
      } else {
        uart1_putc(temp);
       sende_integer(F1_faktor);
      }
      break;
  
  
    case BEF_F1_VORT_AKTIV:
      if(ziffern) {
        if(zahl >=0 && zahl <= 1) {
          F1_vorteiler_aktiv = zahl;
          ee_write_uint8(EE_ADR_F1_VORTEILER, F1_vorteiler_aktiv);
        }  
      } else {
        uart1_putc(temp);
        sende_integer(F1_vorteiler_aktiv);
      }
      break;
  
  
    case BEF_F1_UPM_DIVISOR:
      if(ziffern) {
        if(zahl > 0 && zahl <= MAX_UPM_DIVISOR) {
          F1_upm_divisor = zahl;
          ee_write_uint32(EE_ADR_F1_UPM_DIV, F1_upm_divisor);
        }  
      } else {
        uart1_putc(temp);
        sende_integer(F1_upm_divisor);
      }
      break;
      
  
    case BEF_F2_MESSZEIT:
      if(ziffern) {
        if(zahl < MAX_MESSZEIT) {
          F2_messzeit = zahl;
          ee_write_uint32(EE_ADR_F2_MESSZEIT, F2_messzeit);
        }  
      } else {
        uart1_putc(temp);
        sende_integer(F2_messzeit);
      }
      break;
    case BEF_F2_TIMEOUT:
      if(ziffern) {
        if(zahl < MAX_TIMEOUT) {
          F2_timeout = zahl;
          ee_write_uint32(EE_ADR_F2_TIMEOUT, F2_timeout);
        }  
      } else {
        uart1_putc(temp);
        sende_integer(F2_timeout);
      }
      break;
    case BEF_F2_STELLEN:
      if(ziffern) {
        if(!zahl || zahl <= F2_MAX_STELLEN && zahl >= MIN_STELLEN) {
          F2_stellen = zahl;
          ee_write_uint32(EE_ADR_F2_STELLEN, F2_stellen);
        }
      } else {
        uart1_putc(temp);
        sende_integer(F2_stellen);
      }
      break;
  
  
    case BEF_F2_FAKTOR:
      if(ziffern) {
        if(zahl < MAX_FAKTOR) {
          F2_faktor = zahl;
          ee_write_uint32(EE_ADR_F2_FAKTOR, F2_faktor);
        }  
      } else {
        uart1_putc(temp);
       sende_integer(F2_faktor);
      }
      break;
  
  
      case BEF_F2_VORT_AKTIV:
      if(ziffern) {
        if(zahl >=0 && zahl <= 1) {
          F2_vorteiler_aktiv = zahl;
          ee_write_uint8(EE_ADR_F2_VORTEILER, F2_vorteiler_aktiv);
        }  
      } else {
        uart1_putc(temp);
        sende_integer(F2_vorteiler_aktiv);
      }
      break;
      case BEF_LCD_KONTRAST:
      if(ziffern) {
        if(zahl <= MAX_LCD_KONTRAST && zahl >= MIN_LCD_KONTRAST) {
          lcd_kontrast=zahl;
          ee_write_uint8(EE_ADR_KONTRAST, lcd_kontrast);
          setze_kontrast(lcd_kontrast);
        }
      } else {
        uart1_putc(temp);
        sende_integer(lcd_kontrast);
      }
      break;

    case BEF_SER_AUSGABE:
      if(ziffern) {
        if(zahl <= MAX_SER_KANAL && zahl >= KEINE_AUSGABE) {
	      ser_ausgabe_kanal = zahl;
          ee_write_uint8(EE_ADR_SER_AUSGABE, ser_ausgabe_kanal);
        }
      } else {
        uart1_putc(temp);
        sende_integer(ser_ausgabe_kanal);
      }
      break;
  
    case BEF_LCD_BREITE:
      if(ziffern) {
        if(zahl == DEF_LCD_BREITE || zahl == MAX_LCD_BREITE) {
          if(zahl != lcd_breite) {
            lcd_breite = zahl;
            init_zeilen_breite(lcd_breite);
            loesche_lcd_zeile34();
            update_LCD = 1;
            ee_write_uint8(EE_ADR_LCD_BREITE, lcd_breite);
          }  
        }  
      } else {
        uart1_putc(temp);
        sende_integer(lcd_breite);
      }
      break;
  
    case BEF_ANZEIGE_FORMAT:
      if(ziffern) {
        if(zahl <= MAX_ANZEIGE_FORMAT && zahl >= 0) {
	      anzeige_format = zahl;
          ee_write_uint8(EE_ADR_ANZEIGE_FORMAT, anzeige_format);
        }
      } else {
        uart1_putc(temp);
        sende_integer(anzeige_format);
      }
      break;
      
    case BEF_REGRESSION:
      if(ziffern) {
        if(zahl >=NORMAL && zahl <= OPTIMIERT && zahl != F1_reg_modus) {
          F1_reg_modus = zahl;
          ee_write_uint8(EE_ADR_F1_REGRESSION, F1_reg_modus);
        }  
      } else {
        uart1_putc(temp);
        sende_integer(F1_reg_modus);
      }
      break;

    case BEF_SENDE_VERSION:
      zahl=0;
      sende_str1((char *)version);
      break;
  } // switch

  ser_status = 0;	// abschluss von Befehlen
  ziffern = 0;
  zahl=0; 
}