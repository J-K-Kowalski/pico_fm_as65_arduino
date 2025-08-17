/*
2024-02-29
Michael Nowak
http://www.mino-elektronik.de
Alle Angaben ohne Gewaehr !
*/

#define FM_AS65_EE_SAVE_RECALL

#include "System-Pico-FM-AS65.h"

void speicher_var_offset(int32_t f_offset)
{
    ee_write_uint32(EE_ADR_VAR_OFFSET, f_offset);
}

void schreibe_led_zeit()
{
  ee_write_uint16(EE_ADR_LED_EIN, led_ein_zeit);
}

void schreibe_as65_sperre()
{
  ee_write_uint8(EE_ADR_AS65_SPERRE, as6501_sperre);
}

void schreibe_kontrast(uint8_t kontrast)
{
  lcd_kontrast = kontrast;
  ee_write_uint8(EE_ADR_KONTRAST, kontrast);
  setze_kontrast(kontrast);
}

void lese_kontrast()
{
  ee_read_uint8(EE_ADR_KONTRAST, &lcd_kontrast);
  setze_kontrast(lcd_kontrast);
}

void schreibe_baudraten_index(uint8_t index)
{
  baudraten_index = index;
  ee_write_uint8(EE_ADR_BAUDRATE, index);
}

void lese_baudraten_index()
{
  ee_read_uint8(EE_ADR_BAUDRATE, &baudraten_index);
  set_baudrate1(baud_tab[baudraten_index]);
}

void lese_ee_F1_parameter()
{
  ee_read_double(EE_ADR_F1_TAKT, (double *)&F1_taktfrequenz);
  ee_read_uint32(EE_ADR_F1_MESSZEIT, &F1_messzeit);
  ee_read_uint32(EE_ADR_F1_TIMEOUT, &F1_timeout);
  ee_read_uint32(EE_ADR_F1_STELLEN, &F1_stellen);
  ee_read_uint32(EE_ADR_F1_FAKTOR, &F1_faktor);
  ee_read_uint8(EE_ADR_F1_VORTEILER, &F1_vorteiler_aktiv);
  ee_read_uint32(EE_ADR_F1_UPM_DIV, &F1_upm_divisor);
  ee_read_uint32(EE_ADR_12MHZ_OFFSET, (uint32_t *)&offset_12MHz_fix);
  ee_read_uint32(EE_ADR_TCXO_OFFSET, (uint32_t *)&offset_TCXO_fix);
  ee_read_uint32(EE_ADR_VAR_OFFSET, (uint32_t *)&offset_variabel);
  ee_read_uint32(EE_ADR_EXT_OFFSET, (uint32_t *)&offset_ext_fix);
  ee_read_uint32(EE_ADR_Fintern_OFFSET, (uint32_t *)&Fintern_offset);
  ee_read_uint32(EE_ADR_Fextern_OFFSET, (uint32_t *)&Fextern_offset);
  ee_read_uint8(EE_ADR_F1_AUTO_VORTEILER, &auto_vorteiler_umschaltung);
}

void schreibe_ee_F1_parameter()
{
  ee_write_double(EE_ADR_F1_TAKT, (double)F1_taktfrequenz);
  ee_write_uint32(EE_ADR_F1_MESSZEIT, F1_messzeit);
  ee_write_uint32(EE_ADR_F1_TIMEOUT, F1_timeout);
  ee_write_uint32(EE_ADR_F1_STELLEN, F1_stellen);
  ee_write_uint32(EE_ADR_F1_FAKTOR, F1_faktor);
  ee_write_uint8(EE_ADR_F1_VORTEILER, F1_vorteiler_aktiv);
  ee_write_uint32(EE_ADR_F1_UPM_DIV, F1_upm_divisor);
  ee_write_uint32(EE_ADR_12MHZ_OFFSET, offset_12MHz_fix);
  ee_write_uint32(EE_ADR_TCXO_OFFSET, offset_TCXO_fix);
  ee_write_uint32(EE_ADR_VAR_OFFSET, offset_variabel);
  ee_write_uint32(EE_ADR_EXT_OFFSET, offset_ext_fix);
  ee_write_uint32(EE_ADR_Fintern_OFFSET, Fintern_offset);
  ee_write_uint32(EE_ADR_Fextern_OFFSET, Fextern_offset);
  ee_write_uint8(EE_ADR_F1_AUTO_VORTEILER, auto_vorteiler_umschaltung);
} 

void lese_ee_F2_parameter()
{
  ee_read_uint32(EE_ADR_F2_MESSZEIT, &F2_messzeit);
  ee_read_uint32(EE_ADR_F2_TIMEOUT, &F2_timeout);
  ee_read_uint32(EE_ADR_F2_STELLEN, &F2_stellen);
  ee_read_uint32(EE_ADR_F2_FAKTOR, &F2_faktor);
  ee_read_uint8(EE_ADR_F2_VORTEILER, &F2_vorteiler_aktiv);
}

void schreibe_ee_F2_parameter()
{
  ee_write_uint32(EE_ADR_F2_MESSZEIT, F2_messzeit);
  ee_write_uint32(EE_ADR_F2_TIMEOUT, F2_timeout);
  ee_write_uint32(EE_ADR_F2_STELLEN, F2_stellen);
  ee_write_uint32(EE_ADR_F2_FAKTOR, F2_faktor);
  ee_write_uint8(EE_ADR_F2_VORTEILER, F2_vorteiler_aktiv);
} 


void lese_ee_allgemein_parameter()
{
  lese_kontrast();
  lese_baudraten_index();
  ee_read_uint16(EE_ADR_LED_EIN, &led_ein_zeit);
  ee_read_uint8(EE_ADR_KORREKTUR_MODUS, &korrektur_modus);
  ee_read_uint8(EE_ADR_SPRACHE, &sprache);
  ee_read_uint8(EE_ADR_AS65_SPERRE, &as6501_sperre);

  ee_read_uint8(EE_ADR_LCD_BREITE, &lcd_breite);
  ee_read_uint8(EE_ADR_SER_AUSGABE, &ser_ausgabe_kanal);
  ee_read_uint8(EE_ADR_ANZEIGE_FKT, &anzeige_funktion);
  if(anzeige_funktion >= MAX_FUNKTION) anzeige_funktion = 0;
  ee_read_uint8(EE_ADR_ANZEIGE_FORMAT, &anzeige_format);
  ee_read_uint8(EE_ADR_NUR_MHZ, &nur_MHz);
  ee_read_uint8(EE_ADR_F1_REGRESSION, &F1_reg_modus);
}

void schreibe_ee_allgemein_parameter()
{
  schreibe_kontrast(lcd_kontrast);
  schreibe_baudraten_index(baudraten_index);
  ee_write_uint16(EE_ADR_LED_EIN, led_ein_zeit);
  ee_write_uint8(EE_ADR_KORREKTUR_MODUS, korrektur_modus);
  ee_write_uint8(EE_ADR_SPRACHE, sprache);

  ee_write_uint8(EE_ADR_LCD_BREITE, lcd_breite);
  ee_write_uint8(EE_ADR_SER_AUSGABE, ser_ausgabe_kanal);
  ee_write_uint8(EE_ADR_ANZEIGE_FKT, anzeige_funktion);
  ee_write_uint8(EE_ADR_ANZEIGE_FORMAT, anzeige_format);
  ee_write_uint8(EE_ADR_NUR_MHZ, nur_MHz);
  ee_write_uint8(EE_ADR_F1_REGRESSION, F1_reg_modus);
} 

