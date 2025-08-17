#include "INC/rp2040_arduino.h"
#include "INC/pico_fm_as65.h"

#include "INC/FM_AS65_ext_referenzen.h"          // alle ext. verf�gbaren Varibalen/Funktionen
#include "INC/FM_AS65_ee_adressen.h"             // Speicheradressen
#include "INC/FM_AS65_f_limit.h"                 // Default-Werte und Grenzwerte f�r Einstellungen
#include "INC/FM_AS65_tasten_def.h"              // Tastenwerte und Entprellzeiten
#include "INC/FM_AS65_serio_bef.h"               // Befehle f�r Fernbedienung

#define TEMP_STR_LEN      50                // fuer Ergebnis String
