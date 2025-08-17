#ifndef _SERIO_BEF
#define _SERIO_BEF

#define ESC_BIT		1
#define MINUS_VORZEICHEN	2
#define CTRL_S		0x13
#define ESC		27
#define MINUS		    '-'


#define BEF_F1_MESSZEIT     'A'
#define BEF_F2_MESSZEIT     'B'
#define BEF_F1_TIMEOUT      'C'
#define BEF_F2_TIMEOUT      'D'
#define BEF_F1_STELLEN      'E'
#define BEF_F2_STELLEN      'F'
#define BEF_F1_VORT_AKTIV   'G'
#define BEF_F2_VORT_AKTIV   'H'
#define BEF_F1_FAKTOR       'I'
#define BEF_F2_FAKTOR       'J'
#define BEF_LCD_KONTRAST    'K'
#define BEF_LED_EIN_ZEIT    'L'
#define BEF_F1_DAUERLAUF    'M'
#define BEF_F1_SOFT_TRIGGER 'N'

#define BEF_OFFSET          'O'
#define BEF_F1_UPM_DIVISOR  'P'
#define BEF_F2_UPM_DIVISOR  'Q'
#define BEF_SER_AUSGABE     'R'
#define BEF_GPS_AKTIV       'S'
#define BEF_GPS_FIL_INTERN  'T'
#define BEF_GPS_FIL_EXTERN  'U'
#define BEF_LCD_BREITE      'W'
#define BEF_F1_AUTO_VORTEILER  'x'
#define BEF_TRIGGER_MESSZEIT  'X'
#define BEF_ANZEIGE_FORMAT  'Y'
#define BEF_REGRESSION      'Z'

#define BEF_SENDE_VERSION   'V'
#define BEF_SYNC            '*'
#define BEF_STATUS_ABFRAGE  '?'
#define BEF_STATISTIK       '#'
// #define BEF_MARKENBREITE    '%'

#endif