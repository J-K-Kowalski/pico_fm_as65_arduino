/*
2024-02-29
Michael Nowak
http://www.mino-elektronik.de
Alle Angaben ohne Gewaehr !
*/
#ifndef _F_LIMIT_H_
#define _F_LIMIT_H_

#define REFCLK_AUFLOESUNG 0x10000   // 10 MHz / 65536 = 1,52 ps

// Messintervall für Auswertung
#define MIN_MESSZEIT      1       // 1 ms
#define MAX_MESSZEIT      30000   // 30 s
#define DEF_F1_MESSZEIT   1000    // 1 s
#define DEF_F2_MESSZEIT   990     // 0,990 s

// Bei Überschreiten von Timeout "kein Signal" anzeigen
#define MIN_TIMEOUT       10      // 10 ms
#define MAX_TIMEOUT       1600    // 1,6 s
#define DEF_TIMEOUT       1500    // 1.5 s FIN_min = 0,4 Hz

// Bei Stellenanzahl 0 wird automatisch bewertet
#define MIN_STELLEN       5     // für Ergebnisanzeige
#define F1_MAX_STELLEN    15    // manuell einstellbar
#define DEF_STELLEN       10    // typ. Stellen anzeigen
#define F1_MIN_STELLEN    5     // bei 1 ms Messzeit ist dies das Minimum

#define F2_MAX_STELLEN    10
#define DEF_F2_STELLEN    8     // typ. 9 Stellen anzeigen

// Skalierungsfaktor
#define MIN_FAKTOR        1
#define MAX_FAKTOR        99999
#define DEF_FAKTOR        1

#define MIN_UPM_DIVISOR   1
#define MAX_UPM_DIVISOR   99999
#define DEF_UPM_DIVISOR   1

#define DEF_OFFSET	      0           // 
#define MAX_OFFSET	      5000000     // für jeweils 5 ppm
#define MIN_OFFSET	      -5000000    // max. Korrektur

#define DEF_LED_EIN_ZEIT  50
#define MIN_LED_EIN_ZEIT  1
#define MAX_LED_EIN_ZEIT  10000

#define MIN_LCD_BREITE    16          // kleinstes Format
#define MAX_LCD_BREITE    20          // max. Format
#define DEF_LCD_BREITE    16

#define DEF_LCD_KONTRAST  20
#define MIN_LCD_KONTRAST  0
#define MAX_LCD_KONTRAST  50


#define DEF_GPS_FILTER    60          // 60 s
#define MIN_GPS_FILTER    10          // 10 s
#define MAX_GPS_FILTER    1800        // max. 0,5 Stunden Mittelwert bilden
#define GPS_LED_STUFEN    30
#define GPS_TIMEOUT       1300        // 1,3 s: Referenzfrequenz zu klein / fehlt
#define GPS_TOTZEIT       3           // GPS-Auswertung nach Einschalten 3 Sekunden unterdruecken
#define OFFSET_SCHRITT    1e-10       // 0,1 ppb pro Offset-Schritt

#define OFFSET_SCHRITT_AS65  1e-12    // 0,001 ppb pro Offset-Schritt

#define DEF_BAUD_INDEX    4           // für 115k2
enum sprache        {DT = 0, GB, MAX_SPRACHE};
enum regression     {NORMAL = 0, OPTIMIERT};
enum Fref_optionen  {LOKAL_12MHz, TCXO_10MHz, FREF_EXTERN, MAX_FREF_OPTION};
enum mess_funktion  {F_UND_P, FREQUENZ, PERIODE, DREHZAHL, 
                    FMEAN_UND_N, FMAX_FMIN, SDEV_UND_N, MAX_FUNKTION};
enum ser_kanal      {KEINE_AUSGABE, F1, P1, UPM1, F2, MAX_SER_KANAL};

#define MAX_ANZEIGE_FORMAT      3     // 1.2 Hz; 1,2 Hz; 1.2E+0; 1,2E+0
#endif