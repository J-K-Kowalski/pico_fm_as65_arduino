#ifndef FM_AS65_EXT_REFERENZ
#define FM_AS65_EXT_REFERENZ

#ifndef FM_AS65_BEDIENUNG
extern uint8_t lese_taste();
extern void dekodiere_tasten(void);
extern void FM_AS65_hauptmenue(void);
#endif

#ifndef FM_AS65_UART1_PICO
extern void set_baudrate1(uint32_t baud);
extern void sende_str1(char *s);
extern void sende_strx(char *s ,int cr_lf);
extern void uart1_putc(int zeichen);

extern int getchar_rp(void);
extern uint32_t baud_tab[8];
extern const char *baud_string[8];
extern uint8_t  baudraten_index;        // default 115200
#endif

#ifndef FM_AS65_MESSUNG
extern void F1_messung(uint8_t init);
extern void wandel_Fx(char *header, double x, char zeige_periode, char *s, uint8_t stellen);    // Anzeige von Frequenz oder Periode
extern void loesche_gps_filter(void);
extern void stelle_Fref_mux(int k);

extern void setze_F1_offset(int);
extern int hole_F1_offset(void);
extern void speicher_EE_offset(int offset);

extern uint16_t   led_ein_zeit,
                  gps_fil_laenge;
extern uint8_t    F1_vorteiler_aktiv,
                  F2_vorteiler_aktiv,
                  korrektur_modus,
                  anzeige_funktion,
                  anzeige_format,
                  nur_MHz,
                  F1_reg_modus,
                  ser_ausgabe_kanal,
                  iic_oled,
                  ausgabe_sperre, 
                  update_header;
extern uint8_t    update_LCD;                 // LCD neu beschriften
extern uint8_t    auto_vorteiler_umschaltung; // 0 = schnell


extern volatile double  F1_offset_schritt,
                        F1_taktfrequenz;         // grundfrequenz
extern uint32_t         F1_messzeit,
                        F1_timeout,
                        F2_messzeit,
                        F2_timeout,
                        F1_stellen,
                        F2_stellen,
                        F1_faktor,
                        F2_faktor,
                        F1_upm_divisor;
extern int32_t  Fintern_offset;
extern int32_t  Fextern_offset;
extern uint32_t abgleich_zeit;
extern int32_t  F1_offset, 
                offset_12MHz_fix,
                offset_TCXO_fix,
                offset_variabel,                // Ergebnis von gps-Bewertung
                offset_ext_fix;


#endif

#ifndef FM_AS65_EE_IIC
extern uint8_t ee_write_uint8(uint16_t adr, uint8_t wert);      // ein byte schreiben
extern uint8_t ee_write_uint16(uint16_t adr, uint16_t wert);    // zwei byte schreiben
extern uint8_t ee_write_uint32(uint16_t adr, uint32_t wert);    // vier byte schreiben
extern uint8_t ee_write_double(uint16_t adr, double wert);      // acht byte schreiben

extern uint8_t ee_read_uint8(uint16_t adr, uint8_t *wert);      // ein byte lesen
extern uint8_t ee_read_uint16(uint16_t adr, uint16_t *wert);    // zwei byte lesen
extern uint8_t ee_read_uint32(uint16_t adr, uint32_t *wert);    // vier byte lesen
extern uint8_t ee_read_double(uint16_t adr, double *wert);      // acht byte lesen
extern uint8_t ee_read_n(uint16_t adr, void *ptr, uint16_t anzahl); // array lesen
extern uint8_t ee_write_n(uint16_t adr, void *ptr, uint16_t anzahl);    // array schreiben
#endif

#ifndef FM_AS65_SERIO_INPUT
extern void long2a(int l, char *s, char mit0);
extern void handle_ser_input();
extern void init_uart1(void);
#endif

#ifndef PICO_FM_AS65
extern uint32_t SystemCoreClock;
extern uint32_t *irq_tab;                          // fuer IRQ-Vektoren im RAM
extern void USB_putc(int zeichen);
extern int USB_getc(void);
extern double f_ref_korrektur;
extern void kurz_warten(void);
extern void sende_str(char *s);
extern const char *version;

extern uint8_t
                sprache,
                as6501_aktiv,
                as6501_bestueckt,
                as6501_sperre;

extern uint8_t   ex_ref_aktiv;           // zur einfachen Abfrage
extern uint8_t   NMI_flag;

#endif

#ifndef FM_AS65_LCD
extern uint8_t lcd_vorhanden, 
               lcd_busy;
extern uint8_t lcd_breite;
extern uint8_t lcd_kontrast;
extern void delay_us(uint32_t us);

extern uint8_t lcd_test(void);
extern uint8_t lcd_init(void);
extern void lcd_messwert_zeile1(char *s);
extern void lcd_messwert_zeile2(char *s);
extern void lcd_zeile1(char *s);
extern void lcd_zeile2(char *s);
extern void lcd_zeile3(char *s);
extern void lcd_zeile4(char *s);
extern void lcd_messwert_zeile1(char *s);    // Zeilenlänge angepasst
extern void lcd_messwert_zeile2(char *s);    // Zeilenlänge angepasst
extern void lcd_messwert_zeile3(char *s);    // Zeilenlänge angepasst
extern void lcd_messwert_zeile4(char *s);    // Zeilenlänge angepasst

extern void lcd_cmd(uint8_t cmd);
extern void lcd_zeichen(char s);
extern void lcd_string(char *s);
extern void lcd_clear();
extern void lcd_cursor_on();
extern void lcd_cursor_off();
extern void lcd_zeile_spalte(uint8_t zeile, uint8_t spalte);
extern void setze_kontrast(uint8_t wert);
extern void loesche_lcd_zeile(uint8_t zeile, uint8_t n);
extern void init_zeilen_breite(uint8_t breite);
extern void LCD_str_einmitten(char *s, int breite);
extern void loesche_lcd_zeile1();
extern void loesche_lcd_zeile2();
extern void loesche_lcd_zeile12();
extern void loesche_lcd_zeile34();
#endif

#ifndef FM_AS65_EE_SAVE_RECALL
extern void schreibe_kontrast(uint8_t wert);
extern void schreibe_led_zeit();
extern void schreibe_as65_sperre();

extern void schreibe_baudraten_index(uint8_t index);
extern void speicher_akt_offset(int32_t frequenz_offset);

extern void lese_baudraten_index(void);
extern void speicher_var_offset(int32_t f_offset);
extern void lese_ee_F1_parameter(void);
extern void schreibe_ee_F1_parameter(void);
extern void lese_ee_F2_parameter(void);
extern void schreibe_ee_F2_parameter(void);
extern void lese_ee_allgemein_parameter(void);
extern void schreibe_ee_allgemein_parameter(void);
#endif

#ifndef FM_STATISTIK
extern int stat_n, stat_new_value, stat_F1_stellen;
extern double stat_offset, stat_sum_x, stat_sum_x2;
extern double stat_value, stat_max, stat_min, stat_mean;
extern void init_stat();
extern void stat_new(double x, int stellen);
extern double stat_varianz();
extern double stat_sdev();
#endif

#endif

