#define FM_AS65_MESSUNG
#define REG_LAUFZEIT_TESTn


//#define CORE1_AKTIV  // experimentell 
#define INT128

/*
2024-03-08: neue Version / new version
"pico-fmeter-AS65" / "mino PicoFmeter3"


mino Pico-FMeter-AS6501

***************** GB start *********************
For a reciprocal frequency measurement, the number of incoming events are counted 
and synchronized with the exact times. 

Timing: 
A 74AUP1G74 DFF is used to synchronize the times. After setting the D-input 
to '1' its Q-output ('CAP1') follows to '1' at the next pos. edge of Fin clock. 
Inside the TDC AS6501 'CAP1' captures the reference counter, which runs at 10 MHz, 
and another stop counter. The stop counter finely resolves 
the 100 ns of the 10 MHz clock to 0x10000 intermediate levels. 
After that the 'INTN' output is set to '0' and the DFF is thus blocked. 
The counters can be read out gradually. 

Event measurement: 
The input pulses at 'FIN' are counted by the 16-bit PWM_CH2 counter using
PWM2B input for counting pos. edges.

Time stamp: 
Time measurement and event counting still have to be merged. The exact 
synchronous time is requested by the DFF with 'CAP1' as described.
'CAP1' also triggers DMA_CH0 channel: The current count of PWM_CH2_CTR is 
transfered to PWM_CH2_event variable.. 
This 16 bit counter is sufficient, as it is read about 50,000 times 
at high input frequencies. At low frequencies it is read with each new input pulse.
 
PWM_CH2_event contains the synchronous events to the time event counter of AS6501. 
The eff.  number of events between two readings is as follows: 
(new_state - old_state) & 0xffff 
and for TDC values with their 40 bits as: 
(new_state - old_state) & 0xffffffffff. 
Both values (PWM_CH2_event and TDC-count) are used as a time stamp for further 
evaluation. 

To avoid PIO0_SM0 stands still (blocked if FIFO is full) after each 
PWM_CH2_CTR-transfer, DMA_CH0 triggers channel DMA_CH1, which empties 
PIO0_SM0 FIFO and then activates DMA_CH0 again. 
This action runs without ISR in the hardware. 


Regression calculation: 
To increase the measurement resolution, the recorded time stamps are processed 
until the end of the specified measurement interval frequency calculation takes 
place. In order not to overload the controller, a maximum of 50000 time stamps/s 
(@ SysCoreClock = 133 MHz) are evaluated. 
With PWM-timer 6, the 'INT_REQ' input is queried every 20 µs. If it is set to 
'0' a 'CAP1' event was previously triggered and new values are ready to be read. 
'handle_F1_reg()' performs the preprocessing where 'DMA_CH1_event' must be read 
first! 

By reading out the AS6501, a new time stamp is automatically requested: 
'INTN' is set to '1' again and therefore D-input too.

End of measurement interval: 
After the specified measurement interval (F1_messzeit) has expired, 
the routine 'F1_messung()' performes the final frequency calculation: 
'bewerte_F1_messung() '.

In addition, it is checked which values are to be output in which form and 
whether the ext. 4:1 prescaler must be switched on or off.
 
The rest of the features should be apparent from the comments.

2024-03-05: Dual function without and with AS6501 
The board "pico-fmeter-AS65" can also be used without AS6501. The software then
reports “mino PicoFmeter3”. This version works very similar to that previous 
version "Pico-Fmeter2a". The description of which is referred to: 

http://mino-elektronik.de/download/Pico-FMeter-RP2040-GB.pdf
http://mino-elektronik.de/download/Arbeitsweise_Pico_Fmeter2_GB.pdf

With a subsequently equipped TDC AS6501, the version "mino PicoFM-AS65" is displayed. 
If necessary, you can switch back to the version without TDC by reboot 
the program: power-off and power-on again. 
With the "mino PicoFmeter3" the settings for the exact adjustment have changed. 
When operating with the local RP2040 12 MHz XO, the adjustment can be made with 
an external potentiometer and the offset 'offset_12MHz_fix' can be set. 
If assembled the 10 MHz TCXO can be used to stabilize the local 12 MHz XO. 
10 MHz TCXO is assumed to be exactly and will adjust frequency calculation. 
The correction value is in the variable 'offset_variabel', which is the moving 
average of the difference between 10 MHz TCXO and 12 MHz  XO and is continually 
adjusted. 
The 10 MHz TCXO is temperature and long-term stable, but can also have an offset 
which is compensated with the variable 'offset_TCXO_fix'. 

The third option is to use an external, high-precision external signal as a 
reference frequency. The frequencies 1 Hz (1 PPS-GPS), 10 kHz, 1 MHz and 10 MHz 
are recognized, which continuously adjusts the variable 'offset_variabel'. 
Additionally 'offset_ext' can be set. 
Maybe this is a little confusing at first. But in practical operation it will 
be understandable. 

Compared to the program for the AS6501, the resolution of one offset step (1e-10) 
is coarser by factor 100. 

With the program version "mino PicoFM-AS65" the local 12 MHz XO is not used as 
reference for frequency measurement and there is no automatic correction. 
However it is possible to correct the local (TCXO) or external (ext. ref. input)
10 MHz reference frequency using variables 'offset_TCXO_fix' or 'offset_ext_fix'. 
'offset_ext_fix' will normally remain at 0.

***************** GB end *********************

reziproke Frequenzmessung mit RP2040 und TDC AS6501 fuer hohe Aufloesung

Fuer eine reziproke Frequenzmessung werden die Anzahl der eintreffenden Ereignisse
gezaehlt und synchron dazu die genauen Zeitpunkte.

Zeitmessung:
Zur Synchronisierung der Zeitpunkte dient ein 74AUP1G74 DFF, welches nach Setzen
des D-Eingangs beim naechsten Takt seinen Q-Ausgang auf '1' setzt. Diese 0->1 
Flanke (Signal 'CAP1') speichert im TDC AS6501 die internen Zaehlerstaende eines 
Referenzzaehlers, der mit 10 MHz laeuft, und eines weiteren Stop-Zaehlers, der 
die 100 ns des 10 MHz Taktes auf 0x10000 Zwischenstufen fein aufloest.
Anschließend wird der Ausgang 'INTN' auf '0' gesetzt und das DFF damit gesperrt.
Die Zaehler koennen anschlieissend ausgelesen werden. 
Soweit die Zeitmessung.

Ereignismessung:
Die Eingangsimpulse an 'FIN' werden per 16 Bit PWM-Zaehler PWM_CH2_CTR gezaehlt.
Der PWM2B-Eingang zaehlt die pos. Flanken von 'Fin'.

Zeitstempel:
Zeitmessung und Ereigniszaehlung muessen noch zusammengefuehrt werden. Der genaue, 
synchrone Zeitpunkt wird wie beschrieben durch das DFF mit 'CAP1' angefordert. 

Per 'CAP1'-Signal vom DFF wird zudem der aktuelle Zaehlerstand von PWM_CH2_CTR in 
die Variable PWM_CH2_event per DMA_CH0 uebertragen. Ein 16 Bit Zaehler ist 
ausreichend, da er bei hoher Eingangsfrequenz ca 50000 Mal ausgelesen wird. 
Bei niedrigen Frequenzen sogar bei jedem neuen Eingangsimpuls.
In PWM_CH2_event steht der zum AS6501-Zeitpunkt synchrone Zaehlerstand des 
Ereigniszaehlers.
 
Die eff. Anzahl der Ereignisse zwischen zwei Auslesungen ergibt sich als:
(neuer_Zustand - alter_Zustand) & 0xffff 
und fuer die Zeitmessung mit ihren 40 Bit als: 
(neuer_Zustand - alter_Zustand) & 0xffffffffff.

Beide Werte werden als Zeitstempel fuer die weitere Auswertung verwendet.
Damit PIO0_SM0 nicht stehen bleibt (blockiert durch ein volles FIFO), tiggert 
DMA_CH0 den Kanal DMA_CH1, der das PIO0_SM0 FIFO leert und danch DMA_CH0 wieder 
aktiviert. 
Diese Aktion laueft ohne ISR in der Hardware ab.


Regressionsberechnung:
Zur Erhoehung der Messaufloesung werden die erfassten Zeitstempel solange
aufbereitet, bis nach Ablauf des vorgegeben Messintervalls die abschliessende
Frequenzberechnung stattfindet.
Um den Controller nicht zu ueberlasten, werden maximal 50000 Zeitstempel/s ausgewertet.
Mit Timer 6 wird alle 20 µs der Eingang 'INT_REQ' abgefragt. Liegt er auf '0'
wurde zuvor ein 'CAP1'-Ereignis ausgeloest und neue Werte sind zum Auslesen bereit.
'handle_F1_reg()' führt die Vorverarbeitung durch, wobei 'DMA_CH1_event'
zuerst gelesen werden muss!
Duch Auslesen des AS6501 wird automatisch ein neuer Zeitstempel angefordert:
Mit 'INTN' -> '1' wird auch D-Eingang wieder auf '1' gesetzt.

Ende des Messintervalls:
Nach Ablauf des vorgegebenen Messintervalls (DEF_F1_MESSZEIT) wird in der 
Routine 'F1_messung()' die abschließende Frequenzberechnung durchgeführt: 
'bewerte_F1_messung()'
Nebenbei wird noch geprüft, welche Werte in welcher Form ausgegeben werden sollen
und ob eventuell der ext. 4:1 Vorteiler zugeschaltet werden muss.

Die uebrigen Funktionen sollten aus den Kommentaren erkennbar sein.

2024-03-08: Doppelfunktion ohne und mit AS6501
Das Board "pico-fmeter-AS65" ist auch ohne AS6501 verwendbar. Die Software meldet 
sich dann mit "mino PicoFmeter3". Diese Version funktioniert sehr aehnlich der 
Vorgaengerversion "Pico-Fmeter2a", auf deren Beschreibung verwiesen wird:

http://mino-elektronik.de/download/Pico-FMeter-RP2040-DT.pdf
http://mino-elektronik.de/download/Arbeitsweise_Pico_Fmeter2.pdf

Mit nachtraeglich bestuecktem TDC AS6501 wird die Version "mino PicoFM-AS65" angezeigt.
Bei Bedarf laesst sich wieder auf die Version ohne TDC umschalten, wozu ein Neustart
des Programmes notwendig ist: Aus- und Einschalten.

Beim "mino PicoFmeter3" haben sich die Einstellungen für den genauen Abgleich geaendert.
Beim Betrieb mit lokalem 12 MHz XO (RP2040) kann der Abgleich mit externem Poti und der
Einstellung des Offsets 'offset_12MHz_fix' erfolgen. Mit bestuecktem 10 MHz TCXO
kann dieser dazu verwendet werden, den lokalen 12 MHz XO zu stabilisieren. Mit dem
als genau angenommenen 10 MHz TCXO wird ein Korrekturfaktor ermittelt, der die
Frequenzberechnung anpasst. Der Korrekturwert steht in der Variable 'offset_variabel',
welche aus dem gleitenden Mittwelwert der Differenz zwischen 10 MHz TCXO und 12 MHz 
fortlaufend angepasst wird.
Der 10 MHz TCXO ist zwar temperatur und langzeitstabil, kann aber auch noch einen Offset
ausweisen, der mit der Variablen 'offset_TCXO_fix' kompensiert wird.

Als 3. Moeglichkeit kann ein externes hochgenaues externes Signal als Referenzfrequenz
verwendet werden. Erkannt werden die Frequenzen 1 Hz (1 PPS-GPS), 10 kHz, 1 MHz und
10 MHz, die die Variable 'offset_variabel' fortlaufend anpassen. Zudem kann ein 
'offset_ext' eingestellt werden.
Vielleicht ist das zunaechst ein wenig verwirrend. Im praktischen Betrieb wird es
aber verstaendlich.
Gegenueber dem Programm für den AS6501 ist die Aufloesung eines Offset-Schrittes
Faktor mit 1e-10 Faktor 100 groeber.

Bei der Programmversion "mino PicoFM-AS65" wird der lokale 12 MHz XO nicht fuer
die Frequenzmessung benoetigt und es gibt keine automatische Korrektur. Allerdings
besteht die Moeglichkeit, die lokale (TCXO) oder die externe (ext. ref. Eingang) 
10 MHz Referenzfrequenz mit den Variablen 'offset_TCXO_fix' oder 'offset_ext_fix' 
zu korrigieren. 'offset_ext_fix' wird normalerweise auf 0 bleiben.

2024-03-08
Michael Nowak
http://www.mino-elektronik.de
Alle Angaben ohne Gewaehr !
*/

#include "System-Pico-FM-AS65.h"

extern uint64_t lese_AS6501(void);

#ifdef CORE1_AKTIV
#define STACK_LEN   200   // je 32 bit
enum core1_bef{CORE1_STOP=0, CORE1_RUN, CORE1_ACK};

volatile uint8_t core1_cmd;
uint32_t core1_stack[STACK_LEN];
void starte_core1(void);
#endif


#define REG_ABTASTRATE    50000
#define DMA_COUNT         0x80000000
#define DMA_COUNT_MASKE   (DMA_COUNT-1)

#define PWM_COUNT         0x10000           // 16 Bit Zaehler per PWM-Kanal
#define PWM_COUNT_MASK    (PWM_COUNT-1)
#define AS65_MASK         0xffffffffff      // Zeitmessung 40 Bit breit


#define F1H_VORTEILER     4                 // gesamter Faktor fuer Skalierung
// prescaler switchung point >= 15 MHz: on, <= 11 MHz off
// wegen 65535/ms: maximal 65 MHz moeglich
#define F1V_FMIN  (11000/F1H_VORTEILER)     // wegen 1 ms Intervall in kHz
#define F1V_FMAX   15000                    // dto.

#define DEF_FAKTOR        1
#define DEF_UPM_DIVISOR   1


enum {SM0 = 0, SM1, SM2, SM3} SMx_KANAL;
enum {CH0 = 0, CH1, CH2, CH3} DMA_CHx_Kanal;

#define FREF_FIL_LEN      8
#define FREF1             1e0         // 1 HZ
#define FREF1_LOW         (FREF1 - FREF1*5e-5)  // -50 ppm
#define FREF1_HIGH        (FREF1 + FREF1*5e-5)  // +50 ppm
#define FREF2             1e4         // 10 kHZ
#define FREF2_LOW         (FREF2 - FREF2*5e-5)  // -50 ppm
#define FREF2_HIGH        (FREF2 + FREF2*5e-5)  // +50 ppm
#define FREF3             1e6         // 1 MHz
#define FREF3_LOW         (FREF3 - FREF3*5e-5)  // -50 ppm
#define FREF3_HIGH        (FREF3 + FREF3*5e-5)  // +50 ppm
#define FREF4             1e7         // 10 MHz
#define FREF4_LOW         (FREF4 - FREF4*5e-5)  // -50 ppm
#define FREF4_HIGH        (FREF4 + FREF4*5e-5)  // +50 ppm


#define MAX_ERGEBNIS_STR  20          // Ausgabelaenge begrenzen

double rundung[] = {5e0, 5e-1,5e-2,5e-3,5e-4,5e-5,
                    5e-6,5e-7,5e-8,5e-9,5e-10,5e-11,
                    5e-12,5e-13,5e-14,5e-15};

char f_dim[][10][6]= {{" GHz"," MHz"," kHz"," Hz "," mHz"," sek"," ms "," us "," ns "," ps "},
                      {"E+9","E+6","E+3","E+0","E-3","E+0","E-3","E-6","E-9","E-12"}};  // Dimensionen zum Messwert
char u_dim[][5][6]=  {{" Grpm"," Mrpm"," krpm"," rpm"," mrpm"},
                      {"E+9","E+6","E+3","E+0","E-3"}};

enum anzeige {WANDEL_F=0, WANDEL_P, WANDEL_D};

uint64_t  sys_clock_stamp;                  // 40 Bit anstatt AS6501 Ergebnis

uint32_t  F1_time_stamp,                    // Zeitpunkt von PIO0-SM0
          pio0_sm1_dummy,                   // Ereignis von PIO0-SM1
          PWM_CH2_event,                    // abwaerts zaehlend: Summe Ereignisse zu CAP1
          sys_clock_low,                    // Timer PWM_CH5 CTR zu CAP1
          sys_clock_high,                   // Ueberlauf dazu
          dma_dummy;

uint16_t  led_count, 
          gps_led_cnt, 
          led_ein_zeit = DEF_LED_EIN_ZEIT;  // Blinkzeit

uint8_t   ausgabe_sperre = 1,               // keine LCD Datenausgage
          anzeige_funktion = F_UND_P,       // F+P, nur F, nur P, nur UPM oder auch aus -1
          update_LCD,                       // LCD neu beschriften
          nur_MHz,                          // festes Format x.xxx MHz
          auto_vorteiler_umschaltung=1,     // 1 = mit Neustart, 0 = schnell
          F1_reg_modus = OPTIMIERT,
          korrektur_modus = 0,                  // 
          anzeige_format = 0,               // bit0: Dimension oder Exponent; bit1: . oder , 
          ser_ausgabe_kanal = F1;           // ser. Datenausgabe: F, P, UPM oder auch aus -1


double    F1_ergebnis;                      // in Hz
double    eff_zeit1;                        // in s fuer automatische Stellenanzahl
volatile double    F1_offset_schritt,
          F1_taktfrequenz;                  // Platzhalter, not used here
double    F1_takt = (double)AS6501_REF * REFCLK_AUFLOESUNG;;
uint32_t  F1_ms_grob;                       // grobe Frequenzmessung mit 1 ms Intervall
uint8_t   auto_vorteiler_aktiv,             // Signaleingang: 0=ohne, 1= mit Vorteiler
          F1_vorteiler_aktiv,               // manuelle Zuschaltung, wenn extern vorhanden
          F2_vorteiler_aktiv;               // manuelle Zuschaltung, wenn extern vorhanden

uint32_t  F1_messzeit = DEF_F1_MESSZEIT,    // Sollintervall
          F1_timeout = DEF_TIMEOUT,         // Abbruch bei fehlendem Signal
          F1_stellen = DEF_STELLEN,         // Vorgabewert; 0 = automatisch
          F1_faktor  = DEF_FAKTOR,          // 1
          F1_upm_divisor = DEF_UPM_DIVISOR, // ebenso
          F2_messzeit = DEF_F2_MESSZEIT,    // Sollintervall
          F2_timeout = DEF_TIMEOUT,         // Abbruch bei fehlendem Signal
          F2_faktor  = DEF_FAKTOR,          // 1
          F2_stellen = DEF_F2_STELLEN;      // Vorgabewert; 0 = automatisch
                    
volatile uint8_t
          F1_messwert_vorhanden;            // gueltige Messung
                                   
volatile uint32_t
          F1_mess_dauer,                    // in ms
          F1_timeout_counter;               // in ms
uint64_t  F1_start_zeit,                    // rel. Zeit: Beginn der Messung
          F1_end_zeit,                      // Zeitpunkt der Auswertung
          F1_mess_zeit;                     // genaue Zeit der Messung
                    
uint32_t  F1_mess_ereignisse,               // Impulse der Messung
          F1_start_ereignis,                // Impulse zu Beginn der Messung
          F1_end_ereignis;                  // Impulse am Ende der Messung

int32_t   F1_offset = 0,                    // aktuell verwendeter Wert
          offset_12MHz_fix,
          offset_TCXO_fix,
          offset_variabel,                  // Ergebnis von gps-Bewertung
          offset_ext_fix,
          Fintern_offset,                   // fuer lokalen TCXO
          Fextern_offset;                   // fuer ext. 10 MHz

void      F1_neustart(void);                // verwerfen der laufenden Messung + Neustart

uint16_t  gps_fil_cnt,                      // Anzahl vorhandener Werte
          gps_fil_laenge = DEF_GPS_FILTER,
          ee_schreib_intervall,             // gleiches intervall wie gps_fil_laenge
          gps_fil_index,                    // Filter, Index auf Einzelwerte
          gps_totzeit;                      // Einschaltverzoegerung abwarten

double    gps_fil[MAX_GPS_FILTER],          // letzte Einzelwerte
          gps_fil_sum,
          ref_frequenz;                     // akt. Summe der Einzelwerte

uint32_t  Fref_time_stamp;                  // Zeitpunkt von PIO1-SM1

uint32_t  abgleich_zeit = DEF_GPS_FILTER;


double    eff_zeit2;                        // in s
double    F_ref;                            // unkorrigierte Frequenz


// fuer Fref
double    Fref_ergebnis;                    // in Hz

volatile uint8_t
          Fref_messwert_vorhanden;          // 1 = gueltige Messung
                                   
volatile uint32_t
          Fref_mess_dauer,                  // in ms
          Fref_timeout_counter;             // in ms
uint32_t  Fref_start_zeit,                  // rel. Zeit: Beginn der Messung
          Fref_end_zeit,                    // Zeitpunkt der Auswertung
          Fref_mess_zeit;                   // genaue Zeit der Messung
                  
uint32_t  Fref_mess_ereignisse,             // Impulse der Messung
          Fref_start_ereignis,              // Impulse zu Beginn der Messung
          Fref_end_ereignis;                // Impulse am Ende der Messung

double    Fref_filter[FREF_FIL_LEN];
double    Fref_summe, 
          Fref_aktuell = 0.0,
          Fref_offset = 0.0;
uint32_t  Fref_index;

// Variablen fuer Regressionsberechnung
uint32_t  reg_n, temp_reg_n;                // Anzahl Ereignisse bei Regression
volatile  uint32_t reg_ereignisse;          // und Endwert zur weiteren Auswertung
double    reg_xi, reg_yi,                   // summe fortlaufender Einzelwerte  
          temp_reg_xi, temp_reg_yi;                      
double    reg_sum_xi, reg_sum_yi, 
          temp_reg_sum_xi, temp_reg_sum_yi; // Summe aller Intervalle

double    reg_sum_xiyi, temp_reg_sum_xiyi,  // Summe xi * yi
          reg_sum_xi2, temp_reg_sum_xi2;    // Summe xi²


// Variablen für Regressionsberechnung 64
typedef struct {
  uint64_t  reg_n_64;                       // Anzahl Ereignisse bei Regression
  volatile  uint64_t reg_ereignisse_64;     // und Endwert
  uint64_t  reg_xi_64,                      // Summe aller Intervalle
            reg_yi_64, 
            reg_yi_end_64,                  // lesbare Endwerte 
            reg_sum_xi_64, reg_sum_yi_64,
            zeitpunkt_64;

  uint64_t  xixi_sum_128_h, xixi_sum_128_l;
  uint64_t  xiyi_sum_128_h, xiyi_sum_128_l;
} REG_STRUCT;

static REG_STRUCT  F1_reg, Fx_reg;

// Deklaration ggf. notwendig
void copy_regression_64(REG_STRUCT *Fdest_reg, REG_STRUCT *Fsrc_reg);
void loesche_regression_64(REG_STRUCT *Fx_reg);

void copy_regression_64(REG_STRUCT *Fdest_reg, REG_STRUCT *Fsrc_reg)
{
  Fdest_reg->reg_n_64 = Fsrc_reg->reg_n_64;                    // alle Werte der Regressionsberechnung
  Fdest_reg->reg_xi_64 = Fsrc_reg->reg_xi_64;                  //  umkopieren
  Fdest_reg->reg_yi_64 = Fsrc_reg->reg_yi_64;  
  Fdest_reg->reg_sum_xi_64 = Fsrc_reg->reg_sum_xi_64;
  Fdest_reg->reg_sum_yi_64 = Fsrc_reg->reg_sum_yi_64;
  Fdest_reg->xixi_sum_128_h = Fsrc_reg->xixi_sum_128_h;
  Fdest_reg->xixi_sum_128_l = Fsrc_reg->xixi_sum_128_l;
  Fdest_reg->xiyi_sum_128_h = Fsrc_reg->xiyi_sum_128_h;
  Fdest_reg->xiyi_sum_128_l = Fsrc_reg->xiyi_sum_128_l;
}

void loesche_regression_64(REG_STRUCT *Fx_reg)
{
  Fx_reg->reg_n_64 = 0;                    // alle Werte der Regressionsberechnung
  Fx_reg->reg_xi_64 =  0;                  // löschen
  Fx_reg->reg_yi_64 = 0;  
  Fx_reg->reg_sum_xi_64 = 0;
  Fx_reg->reg_sum_yi_64 = 0;
  Fx_reg->xixi_sum_128_h = 0;
  Fx_reg->xixi_sum_128_l = 0;
  Fx_reg->xiyi_sum_128_h = 0;
  Fx_reg->xiyi_sum_128_l = 0;
}

void mul64to128(uint64_t faktor1, uint64_t faktor2, uint64_t *prod_high, uint64_t *prod_low)
{
  uint64_t temp1, temp2, temp3;
  uint64_t faktor_11 = (faktor1 & 0xffffffff);
  uint64_t faktor_21 = (faktor2 & 0xffffffff);
  uint64_t prod_temp = (faktor_11 * faktor_21);
  
  temp1 = (prod_temp & 0xffffffff);
  temp2 = (prod_temp >> 32);
    
  faktor1 >>= 32;
  prod_temp = (faktor1 * faktor_21) + temp2;
  temp2 = (prod_temp & 0xffffffff);
  temp3 = (prod_temp >> 32);
    
  faktor2 >>= 32;
  prod_temp = (faktor_11 * faktor2) + temp2;
  temp2 = (prod_temp >> 32);
    
  *prod_high = (faktor1 * faktor2) + temp3 + temp2;
  *prod_low = (prod_temp << 32) + temp1;
}

void add128to128(uint64_t sum_high, uint64_t sum_low, uint64_t *res_high, uint64_t *res_low)
{
uint64_t temp = *res_low;
  *res_low += sum_low;
  if(*res_low < temp) (*res_high)++;    // Ueberlauf aufgetreten
  *res_high += sum_high;
}  

double from128to64double(uint64_t high, uint64_t low)
{
uint64_t temp_l = low, temp_h = high;
uint64_t faktor = 1;
double res;
  while(temp_h) {         // solange obere 64 Bit != 0
    temp_l += 1;          // Rundung
    temp_l >>= 1;
    faktor <<= 1;
    if(temp_h & 1) temp_l |= 0x8000000000000000;  // MSB ergänzen
    temp_h >>= 1;
  }  
  res = (double) temp_l * (double) faktor;
  return(res);
}  

void handle_F1_reg_64(void)
{
static uint64_t temp_low, temp_high; 
static uint8_t akt_vorteiler=255, discard_next;  // zum Umschalten des Vorteilers
// Test auf Vorteiler Umschaltung
  if(akt_vorteiler != auto_vorteiler_aktiv) {
    akt_vorteiler = auto_vorteiler_aktiv;
    if(akt_vorteiler) {
      SIO->GPIO_OUT_SET = BIT(PRE_MUX_OUT);
    } else {
      SIO->GPIO_OUT_CLR = BIT(PRE_MUX_OUT);
    }
    discard_next = 3;                   // drei nachfolgende Ergebisse verwerfen
    if(auto_vorteiler_umschaltung) {    // 1 = Neustart der laufenden Messung erzwingen
      F1_neustart();                    // laufende Messung sofort abschliessen
    }  
  }
#ifdef REG_LAUFZEIT_TEST
  SIO->GPIO_OUT_SET = BIT(GPS_LED)  ;
#endif

// Zeitstempel fuer Einzelintervall lesen und auswerten
// Ereignisse immer zuerst!
  F1_end_ereignis = PWM_CH2_event;      // von PWM->CH2_CTR
  F1_mess_ereignisse = (F1_end_ereignis - F1_start_ereignis) & PWM_COUNT_MASK;
  F1_start_ereignis = F1_end_ereignis;
  if(as6501_aktiv)  {
    F1_end_zeit = lese_AS6501();
  } else {
    F1_end_zeit = sys_clock_stamp;
  }
  F1_mess_zeit = (F1_end_zeit-F1_start_zeit) & AS65_MASK;
  F1_start_zeit = F1_end_zeit;
  
  if(F1_mess_ereignisse) {              // nur gueltige Messwerte verwenden
    if(discard_next) {                  // Werte nach dem Umschalten verwerfen
      discard_next--;
    } else {
// Anzahl und Einzelwerte fuer die Regression verarbeiten 
      if(akt_vorteiler) 
        F1_mess_ereignisse *= F1H_VORTEILER;   // skalieren
      
      F1_reg.reg_n_64++;
      F1_reg.reg_yi_64 += (uint64_t)F1_mess_ereignisse;
      F1_reg.reg_xi_64 += F1_mess_zeit;
      F1_reg.reg_sum_xi_64 += F1_reg.reg_xi_64;
      F1_reg.reg_sum_yi_64 += F1_reg.reg_yi_64;
      mul64to128(F1_reg.reg_xi_64, F1_reg.reg_yi_64, &temp_high, &temp_low);
      add128to128(temp_high, temp_low, &F1_reg.xiyi_sum_128_h, &F1_reg.xiyi_sum_128_l);
      mul64to128(F1_reg.reg_xi_64, F1_reg.reg_xi_64, &temp_high, &temp_low);
      add128to128(temp_high, temp_low, &F1_reg.xixi_sum_128_h, &F1_reg.xixi_sum_128_l); 
    }  
  } 
#ifdef REG_LAUFZEIT_TEST
//  SIO->GPIO_OUT_CLR = BIT(GPS_LED)  ;
#endif
}  

// Auswertung nach Ablauf der Messdauer starten  
double bewerte_F1_messung_64(void) 
{
static double reg_divisor;
double F1_temp;
double d_temp1, d_temp2;
double reg_xq_64, reg_yq_64;              // Mittelwerte xi/n und yi/n

#ifdef CORE1_AKTIV
  core1_cmd = CORE1_STOP;
  while(core1_cmd != CORE1_ACK);
#else  
  PWM->CH6_CSR =  0;                      // disable reg-timer
#endif

  copy_regression_64(&Fx_reg, &F1_reg);
  loesche_regression_64(&F1_reg);
#ifdef CORE1_AKTIV
  core1_cmd = CORE1_RUN;
#else  
  PWM->CH6_CSR =  1;                      // und neu erfassen
#endif
	if(Fx_reg.reg_n_64) {
      d_temp1 = from128to64double(Fx_reg.xiyi_sum_128_h, Fx_reg.xiyi_sum_128_l);
      d_temp2 = from128to64double(Fx_reg.xixi_sum_128_h, Fx_reg.xixi_sum_128_l);
      reg_xq_64 = (double)Fx_reg.reg_sum_xi_64/(double)Fx_reg.reg_n_64;
      reg_yq_64 = (double)Fx_reg.reg_sum_yi_64/(double)Fx_reg.reg_n_64;
      reg_divisor = (d_temp2 - (double)Fx_reg.reg_n_64 * (reg_xq_64 * reg_xq_64));
      if((F1_reg_modus != NORMAL) && Fx_reg.reg_n_64 >= 2 && reg_divisor != 0.0) {   // Regression erst ab zwei Werten          
        F1_temp = (d_temp1 - (double)Fx_reg.reg_n_64 * (reg_xq_64 * reg_yq_64)) / reg_divisor;
      } else {
       F1_temp = ((double)Fx_reg.reg_yi_64/(double)Fx_reg.reg_xi_64);
      }
	} else F1_temp = 0.0;		
  F1_temp *= F1_takt;                   // skalieren
// Endwerte für Debugger  
  reg_ereignisse = Fx_reg.reg_n_64;
	if(Fx_reg.reg_n_64) eff_zeit1 = Fx_reg.reg_xi_64/F1_takt; // Gesamtzeit für autom. Stellen
  return(F1_temp);
}





// F1-offset fuer lokalen/externen Takt
void setze_F1_offset(int offset)
{
  if(as6501_aktiv) {
    if(ex_ref_aktiv == 0) {
      Fintern_offset = offset;
    } else {
      Fextern_offset = offset;
    } 
  } else {
    switch(korrektur_modus) {
    case LOKAL_12MHz:
      offset_12MHz_fix = offset;
      break;
    case TCXO_10MHz:
      offset_TCXO_fix = offset;
      break;
    case FREF_EXTERN:
      offset_ext_fix = offset;
      break;
    }
  }
}

// F1-offset ins EEPROM
void speicher_EE_offset(int offset)
{
  if(as6501_aktiv) {
    if(ex_ref_aktiv == 0) {
      ee_write_uint32(EE_ADR_Fintern_OFFSET, offset);
    } else {
      ee_write_uint32(EE_ADR_Fextern_OFFSET, offset);
    } 
  } else {
    switch(korrektur_modus) {
    case LOKAL_12MHz:
      ee_write_uint32(EE_ADR_12MHZ_OFFSET, offset);
      break;
    case TCXO_10MHz:
      ee_write_uint32(EE_ADR_TCXO_OFFSET, offset);
      break;
    case FREF_EXTERN:
      ee_write_uint32(EE_ADR_EXT_OFFSET, offset);
      break;
    }
  }
}


// F1-offset fuer lokalen/externen Takt
int hole_F1_offset(void)
{
int offset = 0;  
  if(as6501_aktiv) {
    if(ex_ref_aktiv == 0) {
      offset = Fintern_offset;
    } else {
      offset = Fextern_offset;
    } 
  } else {
    switch(korrektur_modus) {
    case LOKAL_12MHz:
      offset = offset_12MHz_fix;
      break;
    case TCXO_10MHz:
      offset = offset_TCXO_fix;
      break;
    case FREF_EXTERN:
      offset = offset_ext_fix;
      break;
    }
  }
  return(offset);
}

void loesche_regression(void)
{
  reg_n = 0;                          // alle Werte der Regressionsberechnung
  reg_xi = reg_yi = 0;                // loeschen
  reg_sum_xi = reg_sum_yi = 0;
  reg_sum_xiyi = reg_sum_xi2 = 0;
}

// ISR zu PWM6 mit 50 kHz
// jedes (Teil-)Intervall berechnen und weiterverarbeiten
void handle_F1_reg(void)
{
static uint8_t akt_vorteiler=255, discard_next;  // zum Umschalten des Vorteilers
// Test auf Vorteiler Umschaltung
  if(akt_vorteiler != auto_vorteiler_aktiv) {
    akt_vorteiler = auto_vorteiler_aktiv;
    if(akt_vorteiler) {
      SIO->GPIO_OUT_SET = BIT(PRE_MUX_OUT);
    } else {
      SIO->GPIO_OUT_CLR = BIT(PRE_MUX_OUT);
    }
    discard_next = 3;                   // drei nachfolgende Ergebisse verwerfen
    if(auto_vorteiler_umschaltung) {    // 1 = Neustart der laufenden Messung erzwingen
      F1_neustart();                    // laufende Messung sofort abschliessen
    }  
  }
#ifdef REG_LAUFZEIT_TEST
  SIO->GPIO_OUT_SET = BIT(GPS_LED)  ;
#endif
// Zeitstempel fuer Einzelintervall lesen und auswerten
// Ereignisse immer zuerst!
  F1_end_ereignis = PWM_CH2_event;      // von PWM->CH2_CTR
  F1_mess_ereignisse = (F1_end_ereignis - F1_start_ereignis) & PWM_COUNT_MASK;
  F1_start_ereignis = F1_end_ereignis;
  
  F1_end_zeit = lese_AS6501();
  F1_mess_zeit = (F1_end_zeit-F1_start_zeit) & AS65_MASK;
  F1_start_zeit = F1_end_zeit;
  
  if(F1_mess_ereignisse) {              // nur gueltige Messwerte verwenden
    if(discard_next) {                  // Werte nach dem Umschalten verwerfen
      discard_next--;
    } else {
// Anzahl und Einzelwerte fuer die Regression verarbeiten 
      reg_n++;                          // anzahl der Einzelwerte
      if(akt_vorteiler)                 // wenn aktiv
        F1_mess_ereignisse *= F1H_VORTEILER;   // skalieren
      reg_xi += F1_mess_ereignisse;     // Einzelwerte addieren
      reg_yi += F1_mess_zeit;           // dto
      reg_sum_xi += reg_xi;             // Summe aus Summe der Einzelwerte
      reg_sum_yi += reg_yi;             // dto
      reg_sum_xiyi += reg_xi*reg_yi;    // Summe der Produkte
      reg_sum_xi2  += reg_xi*reg_xi;    // Summe der Quadrate
     }  
  }  
#ifdef REG_LAUFZEIT_TEST
  SIO->GPIO_OUT_CLR = BIT(GPS_LED)  ;
#endif  
}  

// Auswertung nach Ablauf der Messdauer starten  
double bewerte_F1_messung(void) 
{
double temp_reg_divisor;
double temp_reg_xq;
double temp_reg_yq;
double F1_temp;

#ifdef CORE1_AKTIV
  core1_cmd = CORE1_STOP;
  while(core1_cmd != CORE1_ACK);
#else  
  PWM->CH6_CSR =  0;                      // disable reg-timer
#endif  
  uint32_t temp_reg_n = reg_n;            // alle Werte der Regressionsberechnung
  temp_reg_xi = reg_xi;                   // umkopieren
  temp_reg_yi = reg_yi;                
  temp_reg_sum_xi = reg_sum_xi;
  temp_reg_sum_yi = reg_sum_yi;
  temp_reg_sum_xiyi = reg_sum_xiyi;
  temp_reg_sum_xi2 = reg_sum_xi2;
  loesche_regression();                   // loeschen
#ifdef CORE1_AKTIV
  core1_cmd = CORE1_RUN;
#else  
  PWM->CH6_CSR =  1;                      // und neu erfassen
#endif
  if(temp_reg_n) {
		temp_reg_xq = temp_reg_sum_xi / temp_reg_n;
		temp_reg_yq = temp_reg_sum_yi / temp_reg_n;
		temp_reg_divisor = (temp_reg_sum_xi2 - temp_reg_n * (temp_reg_xq * temp_reg_xq));
		if((F1_reg_modus != NORMAL) && temp_reg_n >= 2 && temp_reg_divisor != 0.0) {  // Regression erst ab zwei Werten          
			F1_temp = F1_takt / ((temp_reg_sum_xiyi - temp_reg_n * (temp_reg_xq * temp_reg_yq)) / temp_reg_divisor);
		} else {                                          // wenn nur ein Ereignis in temp_reg_xi
			F1_temp = ((double)temp_reg_xi/(double)temp_reg_yi)*F1_takt;  // Berechnung ohne Regression
		}  
	} else F1_temp = 0.0;
// Endwerte merken      
  reg_ereignisse = temp_reg_n;            // fuer Berechnung der Stellenanzahl           
  if(temp_reg_n)  eff_zeit1 = temp_reg_yi / F1_takt;      // dto. eff. Messzeit in s
  return(F1_temp);
}

uint8_t berechne_stellen(double zeit, uint32_t ereignisse)
{
static double offset;
uint8_t stellen = MIN_STELLEN;	          // minimal
  if(!as6501_aktiv) zeit /= 1000;       // reduzierte Aufloesung
  if(ereignisse > 50) zeit *= 10;         // bei Regression mehr Stellen zulassen
  if(ereignisse > 5000) zeit *= 10;       // eine weitere
  if(ereignisse > 500000) zeit *= 10;     // noch eine weitere
  if(zeit+offset >= 990.0) {
    stellen = 13;
    offset = 100.0;
  } else if(zeit+offset >= 99.0) {
    stellen = 12;
    offset = 10.0;
  } else if(zeit+offset >= 9.9) {         // Anzahl der gültigen Stellen
    stellen = 11;                         // verringert sich bei kürzeren Messzeiten
    offset = 1.0;
  } else if(zeit+offset >= 9.9e-1) {
    stellen = 10;
    offset = 0.1;
  } else if(zeit+offset >= 9.9e-2) {
    stellen = 9;
    offset = 0.01;
  } else offset = 0;
  if(stellen < F1_MIN_STELLEN) stellen = F1_MIN_STELLEN;
  return(stellen);
}

// bedingte Anzeige von Messwerten
void messwert_lcd1(char *s)
{
  if(!ausgabe_sperre) lcd_messwert_zeile1(s);
}  

void messwert_lcd2(char *s)
{
  if(!ausgabe_sperre) lcd_messwert_zeile2(s);
} 

// Routine zur Wandlung und Ausgabe eines Messwertes
void wandel_Fx(const char *header, double x, char zeige_periode, char *s, uint8_t stellen)    // Anzeige von F, P oder UPM
{
int i,j,dez_punkt,dimension;
char *p;
  if(zeige_periode == WANDEL_F && nur_MHz)
  {
    if(x >= 1e6) stellen--;
    if(x >= 1e7) stellen--;
    if(x >= 1e8) stellen--;
    if(x >= 1e9) stellen--;
    sprintf(s,"%.*f MHz",stellen,x*1e-6);   // festes Format bei "nur MHz"
    if(*s == '0') *s = ' ';
    return;
  }    

  dez_punkt = 0;
  dimension = 3;
  p = (char *)header;
  while(*p) *s++ = *p++;                    // Bezeichner kopieren, wenn angegeben
  if(x >= 0.001) {                          // 1mHz ist Untergrenze
    if(zeige_periode == WANDEL_P) {
      x = 1/x;                              // Kehrwert bilden
      dimension=5;                          // und in Sekunden als Dimension
    }
    while(x<1.0) {x*=1000.0;dimension++;}   // in den Hz-Bereich bringen
    while(x>=1000.0) {x*=0.001;dimension--;}
    while(x >= 10.0) {
      x *= 0.1;
      dez_punkt++;
    }
    x += rundung[stellen];                  // runden
    if(x >= 10.0) {			                    // Ueberlauf bei Rundung
      x *= 0.1;                             // korrigieren
      dez_punkt++;
      if(dez_punkt > 2) {		                // Ueberlauf der Dimension
        dimension--;
        dez_punkt = 0;
      }
    }
  } else {
    x = 0.0; dez_punkt=0; dimension=3;      // 0.00000 Hz ausgeben
  }
  
  for(i=0;i<stellen;i++) {
    j = (uint8_t)x;
    *s++ = (j+'0');
    if(i == dez_punkt) {
      if(anzeige_format & 2)
        *s++ = (',');
      else
        *s++ = ('.');
    }  
    x -= j;
    x *= 10;
  }

  if(zeige_periode == WANDEL_D)
    p = u_dim[anzeige_format&1][dimension];
  else
    p = f_dim[anzeige_format&1][dimension];
  while(*p) *s++ = *p++;                    // Dimension anhaengen
  *s++ = 0;                                 // string abschliessen
  *s = 0;                                   // string abschliessen
  s[MAX_ERGEBNIS_STR] = 0;                  // zur Sicherheit begrenzen
}

void Fx_str_mittig(const char *header, double x, char zeige_periode, char *s, uint8_t stellen)    // Anzeige von F, P oder UPM
{
char p[TEMP_STR_LEN];
int i,j;
  *p = 0;
  for(i = 0; i < MAX_ERGEBNIS_STR; i++)     // Ergebnisstring loeschen
    s[i] = ' ';
  s[MAX_ERGEBNIS_STR] = 0;                  // Ende von s
  
  wandel_Fx(header,x,zeige_periode,p,stellen);// wandeln
  i = strlen(p);
  i = lcd_breite - i;                       // auf 16 Zeichen/Zeile einmitten
  if(i < 0) i = 0;                          // Unterlauf verhindern
  j = i/2;
  i = 0;
  while(p[i]) s[j++] = p[i++];              // in den Ergebnisstring schreiben
  s[lcd_breite] = 0;                        // Laenge sicher begrenzen
} 



// steigende Flanken von CAP1 speichert PWM->CH2_CTR
// SMx testet zwar auf -Flanken, aber CAP1-Eingang ist per Hardware invertiert
void init_PIO0_SM0_1(void)
{
// PIO0 aktivieren  
  RESETS_CLR->RESET = RESETS_RESET_pio0_Msk;
  while (!(RESETS->RESET_DONE & RESETS_RESET_pio0_Msk));
// Programm zur Erkennung pos. Flanken  CAP1
// Eingangssignal CAP1 invertiert bewerten: geht schneller!  
// ADR0
  PIO0->INSTR_MEM0 = 0x00c0+2;      // jmp FIN1_PIN -> ADR2 wenn '1'  
// ADR1
  PIO0->INSTR_MEM1 = 0x0000+0;      // jmp ADR0 weiter auf '1' warten  
// ADR2
  PIO0->INSTR_MEM2 = 0x00c0+2;      // jmp FIN1_PIN -> ADR2 warten bis '0'
// input-pin 1 -> 0
// ADR3  
  PIO0->INSTR_MEM3 = 0x8000;        // push sofort nach CAP1 = aktiv
// ADR4  
//  PIO0->INSTR_MEM4 = 0xc000;        // nur bei Bedarf set IRQ0
  
// fuer SM0: invertierter CAP1-Eingang
  IO_BANK0->GPIO17_CTRL = IO_BANK0_GPIO0_CTRL_INOVER_INVERT << IO_BANK0_GPIO0_CTRL_INOVER_Pos |
                          IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;
// zunaechst SM0 aktivieren 
  PIO0->SM0_EXECCTRL = (3 << PIO0_SM0_EXECCTRL_WRAP_TOP_Pos) |        // autowrap für max.
                       (0 << PIO0_SM0_EXECCTRL_WRAP_BOTTOM_Pos) |     // Geschwindigkeit
                       (CAP1_PIN  << PIO0_SM0_EXECCTRL_JMP_PIN_Pos);  // Pin fuer JMP
// Startadresse
  PIO0->SM0_INSTR =  0x0000;        // jmp ADR0
  PIO0->SM0_CLKDIV = 1 << PIO0_SM0_CLKDIV_INT_Pos |
                     0 << PIO0_SM0_CLKDIV_FRAC_Pos;
// starten  
  PIO0_SET->CTRL = (BIT(SM0)) << (PIO0_CTRL_SM_ENABLE_Pos);
  
// mit gleichem Programm SM1 aktivieren  
  PIO0->SM1_EXECCTRL = (3 << PIO0_SM1_EXECCTRL_WRAP_TOP_Pos) |        // autowrap für max.
                       (0 << PIO0_SM1_EXECCTRL_WRAP_BOTTOM_Pos) |     // Geschwindigkeit
                       (CAP1_PIN  << PIO0_SM1_EXECCTRL_JMP_PIN_Pos);  // Pin fuer JMP
  
// Startadresse
  PIO0->SM1_INSTR =  0x0000;        // jmp ADR0
  PIO0->SM1_CLKDIV = 1 << PIO0_SM1_CLKDIV_INT_Pos |
                     0 << PIO0_SM1_CLKDIV_FRAC_Pos;
// starten  
  PIO0_SET->CTRL = (BIT(SM1)) << (PIO0_CTRL_SM_ENABLE_Pos);
}


void DMA_IRQ_0_IRQHandler(void)
{
uint32_t DMA_temp;

  if(DMA->INTR & BIT(2)) {                  // DMA_CH2 ?
    DMA->INTS0 = BIT(2);                    // clear flag
    DMA_temp = sys_clock_low;
    sys_clock_stamp = (uint64_t)DMA_temp + (uint64_t)(sys_clock_high) * PWM_COUNT;
    if(PWM->INTS & BIT(INT_TIMER) && DMA_temp < PWM_COUNT/2)
      sys_clock_stamp += PWM_COUNT;
    SIO->GPIO_OUT_CLR = BIT(INT_REQ);       // neue Auswertung starten
  }
}

// Zeitstempel erzeugen:
// DMA-Wert = Zeitpunkt, DMA-TRANS-COUNT = Ereignisse
void init_dma_pio0_sm0_1(void)
{
  RESETS_CLR->RESET = RESETS_RESET_dma_Msk | RESETS_RESET_busctrl_Msk;
  while (!(RESETS->RESET_DONE & RESETS_RESET_dma_Msk));
  BUSCTRL->BUS_PRIORITY = BUSCTRL_BUS_PRIORITY_DMA_W_Msk |
                          BUSCTRL_BUS_PRIORITY_DMA_R_Msk;

// PIO0_SM0  
  DMA->CH0_READ_ADDR = (uint32_t) &PWM->CH2_CTR;                // capture CH2-CTR
  DMA->CH0_WRITE_ADDR = (uint32_t) &PWM_CH2_event;              // und sichern
  DMA->CH0_TRANS_COUNT = 1;
  DMA->CH0_CTRL_TRIG =  DREQ_PIO0_RX0 << DMA_CH1_CTRL_TRIG_TREQ_SEL_Pos |     // PIO0_RXF0
                        1 << DMA_CH0_CTRL_TRIG_CHAIN_TO_Pos |   // trigger CH1
                        2 << DMA_CH0_CTRL_TRIG_DATA_SIZE_Pos |  // word
                        DMA_CH0_CTRL_TRIG_EN_Msk;               // enable
// triggered by CH0 and triggers CH0 again  
  DMA->CH1_READ_ADDR = (uint32_t) &PIO0->RXF0;                  // transfer FIFO loeschen     
  DMA->CH1_WRITE_ADDR = (uint32_t) &dma_dummy;                  // to dummy location
  DMA->CH1_TRANS_COUNT = 1;                                     // only 1 transfer
  DMA->CH1_CTRL_TRIG =  DREQ_AUTO << DMA_CH0_CTRL_TRIG_TREQ_SEL_Pos |  // free running
                        0 << DMA_CH0_CTRL_TRIG_CHAIN_TO_Pos |   // Trigger CH0  
                        2 << DMA_CH0_CTRL_TRIG_DATA_SIZE_Pos |  // word
                        DMA_CH0_CTRL_TRIG_EN_Msk;               // enable
  
// PIO0_SM1  
  DMA->CH2_READ_ADDR = (uint32_t) &PWM->CH5_CTR;                // capture CH2-CTR
  DMA->CH2_WRITE_ADDR = (uint32_t) &sys_clock_low;              // und sichern
  DMA->CH2_TRANS_COUNT = 1;
  DMA->CH2_CTRL_TRIG =  DREQ_PIO0_RX1 << DMA_CH3_CTRL_TRIG_TREQ_SEL_Pos |     // PIO0_RXF1
                        3 << DMA_CH0_CTRL_TRIG_CHAIN_TO_Pos |   // trigger CH3
                        2 << DMA_CH0_CTRL_TRIG_DATA_SIZE_Pos |  // word
                        DMA_CH0_CTRL_TRIG_EN_Msk;               // enable
// triggered by CH0 and triggers CH0 again  
  DMA->CH3_READ_ADDR = (uint32_t) &PIO0->RXF1;                  // transfer FIFO loeschen     
  DMA->CH3_WRITE_ADDR = (uint32_t) &dma_dummy;                  // to dummy location
  DMA->CH3_TRANS_COUNT = 1;                                     // only 1 transfer
  DMA->CH3_CTRL_TRIG =  DREQ_AUTO << DMA_CH0_CTRL_TRIG_TREQ_SEL_Pos |  // free running
                        2 << DMA_CH0_CTRL_TRIG_CHAIN_TO_Pos |   // Trigger CH0  
                        2 << DMA_CH0_CTRL_TRIG_DATA_SIZE_Pos |  // word
                        DMA_CH0_CTRL_TRIG_EN_Msk;               // enable
//DMA_CH3 mit Interrupt um PWM_CH5 Ueberlaefe zu verarbeiten  
  irq_tab[DMA_IRQ_0_IRQn] = (uint32_t)DMA_IRQ_0_IRQHandler;  // Vektor im RAM
  NVIC_EnableIRQ(DMA_IRQ_0_IRQn);
  NVIC_SetPriority(DMA_IRQ_0_IRQn,PRIO_MID_HIGH);       // hoehere Prioritaet verwenden
  DMA->INTE0 |= BIT(2);                     // und freigeben
}


// 1 kHz Aufrufe fuer Messzeiten F1_TIMER (PWM7)
// bzw. 50 kHz fuer Regressionsberechnung REG_TIMER (PWM6)
void PWM_IRQ_WRAP_IRQHandler(void)
{
static uint32_t F1_ms_alt;  // fuer grobe Messung in ms: Vorteilerumschaltung
uint32_t temp;

#ifndef CORE1_AKTIV
  if(PWM->INTS & BIT(REG_TIMER)) {          // PWM_CH6?
    PWM->INTR = BIT(REG_TIMER);             // clear flag
    if(!(SIO->GPIO_IN & BIT(INT_REQ))) {    // 0 = neuer Zeitstempel vorhanden
      F1_timeout_counter = 0;               // reset timeout

#ifdef INT128
      handle_F1_reg_64();                   // Verarbeiten der Zeitstempel
#else      
      handle_F1_reg();                      // Verarbeiten der Zeitstempel
#endif      
      SIO->GPIO_OUT_SET = BIT(INT_REQ);     // neuen CAP1 anfordern
    }  
  }  
#endif

  if(PWM->INTS & BIT(INT_TIMER)) {          // Ueberlauf PWM_CH5?
    DMA_IRQ_0_IRQHandler();                 // Zeitstempel vorrangig verarbeiten
    PWM->INTR = BIT(INT_TIMER);             // clear flag
    sys_clock_high++;                       // Ueberlaeufe
  }
  
  if(PWM->INTS & BIT(F1_TIMER)) {           // 1 kHZ Aufrufe PWM_CH7?
    PWM->INTR = BIT(F1_TIMER);              // clear flag
    F1_mess_dauer++;                        // fuer Messablauf
    F1_timeout_counter++;                   // dto.
    temp = PWM->CH2_CTR;                    // neu ms-Wert Zaehlerstand lesen
// fuer maximal 65 MHz !
    F1_ms_grob = (temp - F1_ms_alt) & 0xffff;  // eff. Differenz
    F1_ms_alt = temp;                       // fuer naechsten Vergleich
    Fref_mess_dauer++;                      // fuer Messablauf
    Fref_timeout_counter++;                 // dto.
   
// Umschaltung von Fref testen    
    if(SIO->GPIO_IN & BIT(REF_MUX_IN_OUT))  // Signal von Ladungspumpe
      temp = 1;                             // ext. 10 MHz Referenz
    else
      temp = 0;
    if(ex_ref_aktiv ^ temp) {               // geaenderte Referenz
      ex_ref_aktiv = temp;
      F1_neustart();                        // akt. Messung neu beginnen
    }  

    dekodiere_tasten();                     // einlesen und auswerten
// fertig-LED und 1pps-LED blinken lassen
    if(led_count) {                         // ready-LEDs
      SIO->GPIO_OUT_SET = BIT(LOCAL_LED) | BIT(EXT_LED);  // on
      led_count--;
    } else {
      SIO->GPIO_OUT_CLR = BIT(LOCAL_LED) | BIT(EXT_LED);  // off
    }  
    if(gps_led_cnt) {                         // ready-LEDs
      SIO->GPIO_OUT_SET = BIT(GPS_LED);  // on
      gps_led_cnt--;
    } else {
      SIO->GPIO_OUT_CLR = BIT(GPS_LED);  // off
    }  
  }
  __DSB();
}

// ms-Raster fuer Messablauf erzeugen
// 50 kHz per PWM6 fuer Regressionsberechnung starten
void init_F1_timer(void)
{
  RESETS_CLR->RESET = RESETS_RESET_pwm_Msk;
  while (!(RESETS->RESET_DONE & RESETS_RESET_pwm_Msk));

  PWM->CH7_DIV = 100 << PWM_CH0_DIV_INT_Pos;      // Vorteiler / 100
  PWM->CH7_TOP = SystemCoreClock/100/1000-1;      // 1 kHz
  PWM->CH7_CSR =  1;                              // enable ms-timer
  PWM_SET->INTE = BIT(F1_TIMER);                  // per iSR
  
#ifndef CORE1_AKTIV
  PWM->CH6_DIV = 100 << PWM_CH0_DIV_INT_Pos;      // Vorteiler / 100
  PWM->CH6_TOP = SystemCoreClock/100/REG_ABTASTRATE-1;     // 50 kHz
  PWM->CH6_CSR =  1;                              // enable reg-timer
  PWM_SET->INTE = BIT(REG_TIMER);                 // per iSR
#endif
  
  irq_tab[PWM_IRQ_WRAP_IRQn] = (uint32_t)PWM_IRQ_WRAP_IRQHandler;  // Vektor im RAM
  NVIC_EnableIRQ(PWM_IRQ_WRAP_IRQn);
  NVIC_SetPriority(PWM_IRQ_WRAP_IRQn,PRIO_MID_HIGH);          // hoehere Prioritaet verwenden
  
// Fin mit PWM->CH2_CTR zaehlen  
  IO_BANK0->GPIO21_CTRL = IO_BANK0_GPIO21_CTRL_FUNCSEL_pwm_b_2;  // eingang PWM2B
  PWM->CH2_CSR =  PWM_CH2_CSR_DIVMODE_rise << PWM_CH0_CSR_DIVMODE_Pos | // +Flanke
                  PWM_CH0_CSR_EN_Msk;
  PWM->CH5_DIV = 1 << PWM_CH5_DIV_INT_Pos;        // SystemCoreClock
  PWM->CH5_TOP = PWM_COUNT_MASK;                  // 16 Bit
  PWM->CH5_CSR =  1;                              // enable
  PWM_SET->INTE = BIT(INT_TIMER);                 // per iSR
  
}

void stelle_Fref_mux(int k)
{
  if(as6501_aktiv) {
    SIO->GPIO_OE_CLR = BIT(REF_MUX_IN_OUT);     // auf Eingang 
    return;
  }  

  SIO->GPIO_OE_SET = BIT(REF_MUX_IN_OUT);       // sonst Ausgang
  if(k == TCXO_10MHz) {
    SIO->GPIO_OUT_CLR = BIT(REF_MUX_IN_OUT);    // auf 0 
  } else {
    SIO->GPIO_OUT_SET = BIT(REF_MUX_IN_OUT);    // auf 1 fuer externe Quelle 
  }
}  
    
void init_F1_io(void)
{
// INT_REQ GPIO14 bleibt auf Eingang
// CAP1 bleibt auf Eingang und wird bei PIO0_SM2 passend initialisiert
// FIN bleibt auf Eingang und wird bei PIO0_SM1 passend initialisiert
// EX_REF wird nicht verwendet
// GPIO22 wird nicht verwendet
  
  IO_BANK0->GPIO14_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;  // INT_REG Ausgang
  IO_BANK0->GPIO15_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;  // LED-Ausgang
  IO_BANK0->GPIO20_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;  // zum Fref-Multiplexer
  IO_BANK0->GPIO25_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;  // lokal LED-Ausgang
  SIO->GPIO_OE_SET = BIT(INT_REQ) | BIT(LOCAL_LED) | BIT(EXT_LED); 
  
// PRE_MUX_OUT fuer Vorteilerumschaltung 
  IO_BANK0->GPIO16_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;
  SIO->GPIO_OE_SET = BIT(PRE_MUX_OUT);
// GPIO18 als GPS_LED-Ausgang und fuer Laufzeittest
  IO_BANK0->GPIO18_CTRL = IO_BANK0_GPIO0_CTRL_FUNCSEL_sio_0;
  SIO->GPIO_OE_SET = BIT(GPS_LED);
  stelle_Fref_mux((int)korrektur_modus);
}  

  
void F1_neustart(void)
{
    F1_mess_dauer =  F1_messzeit;       // 1. Messung sofort abschliessen
    F1_messwert_vorhanden  = 0;         // und 1. Ergebnis verwerfen
    F1_timeout_counter = 0; 
    init_stat();                        // Statistik loeschen
    F1_offset = hole_F1_offset();
}    


void F1_messung(uint8_t init_timer)
{
static uint8_t  init = 0, alte_funktion = 255;
uint8_t eff_stellen, temp;
static char p_str[TEMP_STR_LEN];
static char s[TEMP_STR_LEN];
const char *str0[] = {
                "    Signal?     ",
                "    signal?     "};
const char *stry[] = {
                "    Signal F?    ",
                "    signal F?    "};
const char *strz[] = {
                "    Signal P?    ",
                "    signal P?    "};
const char *str1[] = {
                "    Frequenz    ",
                "   frequency    "};
const char *str2[] = {
                "    Periode    ",
                "    periode    "};
const char *str3[] = {
                "    Drehzahl    ",
                " rotation/min.  "};

  if(init_timer) { 
    init = 0;                           // Anzeige + Messung initialisieren
    if(!as6501_aktiv)
      F1_takt = SystemCoreClock;  // default
    init_F1_io();
    init_F1_timer();                    // 1 ms Takt und 50 kHz Regression
    init_dma_pio0_sm0_1();              // time stamps DMA
    init_PIO0_SM0_1();                  // Capture fuer Zeitstempel
    F1_offset = hole_F1_offset();       // beim Start festlegen
    SIO->GPIO_OUT_CLR = BIT(INT_REQ);   // D-FF loeschen
    SIO->GPIO_OUT_SET = BIT(INT_REQ);   // D-FF aktivieren

#ifdef CORE1_AKTIV
    starte_core1();
#endif      
  }
   
  if(!init) {                           // bei Neustart
    F1_mess_dauer =  F1_messzeit;       // 1. Messung sofort abschliessen
    F1_messwert_vorhanden  = 0;         // und 1. Ergebnis verwerfen
    F1_timeout_counter = 0; 
    init = 1;
    update_LCD = 1;
    init_stat();                        // Statistik loeschen
  }
 
  if(!ausgabe_sperre && (anzeige_funktion != alte_funktion || update_LCD)) {  
    update_LCD = 0;
    if(anzeige_funktion >= MAX_FUNKTION) 
      anzeige_funktion = F_UND_P;       // sinnvollen Wert einstellen
    alte_funktion = anzeige_funktion;
    loesche_lcd_zeile12();
    switch(anzeige_funktion) {
      case F_UND_P:
        lcd_zeile1((char *)stry[sprache]);
        lcd_zeile2((char *)strz[sprache]);
        break;
      case FREQUENZ:
        lcd_zeile1((char *)str1[sprache]);
        lcd_zeile2((char *)str0[sprache]);
        break;
      case PERIODE:
        lcd_zeile1((char *)str2[sprache]);
        lcd_zeile2((char *)str0[sprache]);
        break;
      case DREHZAHL:
        lcd_zeile1((char *)str3[sprache]);
        lcd_zeile2((char *)str0[sprache]);
        break;
      case FMEAN_UND_N:
        lcd_zeile1((char *)"  Fmean: n=     ");
        lcd_zeile2((char *)str0[sprache]);
        break;
      case FMAX_FMIN:
        lcd_zeile1((char *)"   Fmax-Fmin    ");
        lcd_zeile2((char *)str0[sprache]);
        break;
      case SDEV_UND_N:
        lcd_zeile1((char *)"  S-dev: n =    ");
        lcd_zeile2((char *)str0[sprache]);
        break;
    }
  }
  
  if(F1_timeout_counter >= F1_timeout) {      // bei fehlenden Eingangsimpulsen
    init = 0;                                 // neu initialisieren
  }  
// grobe bewertung von Fin zur Auswahl des Vorteilers

  if(!auto_vorteiler_aktiv && F1_ms_grob > F1V_FMAX) 
    auto_vorteiler_aktiv = 1;                 // Vorteiler aktiv fuer f > 1 MHz
  if(auto_vorteiler_aktiv && F1_ms_grob < F1V_FMIN) 
    auto_vorteiler_aktiv = 0;                 // Vorteiler abschalten f < 400 kHz 

  if(F1_mess_dauer >= F1_messzeit) {          // wenn Mindestzeit erreicht
#ifdef INT128
    F1_ergebnis = bewerte_F1_messung_64();    // F1_ergebnis und eff_zeit1 berechnen
#else      
    F1_ergebnis = bewerte_F1_messung();       // F1_ergebnis und eff_zeit1 berechnen
#endif      
    if(F1_ergebnis > 0.0) {
// lokale Taktfrequenz wird mit Poti korrigiert
      if(!ex_ref_aktiv) F1_ergebnis *= f_ref_korrektur;
// digitalen Abgleich mit F1_offset durchfuehren  
      F1_offset = hole_F1_offset();           // immer aktuellen Wert holen
      if(!as6501_aktiv && 
        (korrektur_modus == TCXO_10MHz || 
         korrektur_modus == FREF_EXTERN))
        F1_offset += offset_variabel;         // variablen Korrekturwert addieren
      F1_ergebnis += F1_ergebnis*(F1_offset*F1_offset_schritt);
      F1_mess_dauer = 0;                      // neue Messung beginnen
      if(F1_messwert_vorhanden) { 
        eff_stellen = F1_stellen;
        if(!eff_stellen) eff_stellen = berechne_stellen(eff_zeit1, reg_ereignisse);
        if(F1_vorteiler_aktiv)
          F1_ergebnis *= (double)F1_faktor;   // und ggf. skalieren
        stat_new(F1_ergebnis, eff_stellen);  
        led_count = led_ein_zeit;
// serielle Datenausgabe zuerst        
        if(ser_ausgabe_kanal == P1) {
          wandel_Fx("",F1_ergebnis, WANDEL_P, s, eff_stellen);
          sende_str1(s);
        }
        if(ser_ausgabe_kanal == UPM1) {
          wandel_Fx("",F1_ergebnis*60.0/F1_upm_divisor, WANDEL_D, s, eff_stellen);
          sende_str1(s);
        }
        wandel_Fx("",F1_ergebnis, WANDEL_F, s, eff_stellen);
        if(ser_ausgabe_kanal == F1) 
          sende_str1(s);
// danach Anzeige auf LCD        
        switch(anzeige_funktion) {
          case F_UND_P:
            Fx_str_mittig("F ",F1_ergebnis, WANDEL_F, p_str, eff_stellen);
            messwert_lcd1(p_str);
            Fx_str_mittig("P ",F1_ergebnis, WANDEL_P, p_str, eff_stellen);
            messwert_lcd2(p_str);
            break;
          case FREQUENZ:
            Fx_str_mittig("",F1_ergebnis, WANDEL_F, p_str, eff_stellen);
            messwert_lcd2(p_str);
            Fx_str_mittig("",F1_ergebnis+0.1, WANDEL_F, p_str, eff_stellen);
            messwert_lcd2(p_str);
            break;
          case PERIODE:
            Fx_str_mittig("",F1_ergebnis, WANDEL_P, p_str, eff_stellen);
            messwert_lcd2(p_str);
            break;
          case DREHZAHL:
            Fx_str_mittig("",F1_ergebnis*60.0/F1_upm_divisor, WANDEL_D, s, eff_stellen);
            messwert_lcd2(s);
            break;
          case FMEAN_UND_N:
            strcpy(s,"Fmean: n=     ");
            long2a(stat_n, &s[10], 0);
            LCD_str_einmitten(s, lcd_breite);
            messwert_lcd1(s);
            Fx_str_mittig("\001 ",stat_mean, WANDEL_F, s, eff_stellen);
            messwert_lcd2(s);
            break;
          case FMAX_FMIN:
            Fx_str_mittig("\002 ",stat_max, WANDEL_F, p_str, eff_stellen);
            messwert_lcd1(p_str);
            Fx_str_mittig("\003 ",stat_min, WANDEL_F, p_str, eff_stellen);
            messwert_lcd2(p_str);
            break;
          case SDEV_UND_N:
            strcpy(s,"S-dev: n=     ");
            long2a(stat_n, &s[10], 0);
            LCD_str_einmitten(s, lcd_breite);
            messwert_lcd1(s);
            temp = anzeige_format;
            anzeige_format |= 1;
            if(eff_stellen > 10) eff_stellen = 10;  // bei 2x16 LCD noch darstellbar
            if(stat_n >= 1) 
              Fx_str_mittig("\345 ",stat_sdev(), WANDEL_F, s, eff_stellen);
            anzeige_format = temp;
            messwert_lcd2(s);
            break;
        }
      } else F1_messwert_vorhanden  = 1;    // sperre wieder aufheben
    }
  }   
}  


#ifdef CORE1_AKTIV

void core1_loop(void)
{
volatile int n;
  while(1) {
    switch(core1_cmd) {
    case CORE1_STOP:
      core1_cmd = CORE1_ACK;
    case CORE1_ACK:
      while(core1_cmd == CORE1_ACK);
      break;
    case CORE1_RUN:
      if(!(SIO->GPIO_IN & BIT(INT_REQ))) {    // 0 = neuer Zeitstempel vorhanden
        F1_timeout_counter = 0;               // reset timeout
#ifdef INT128
        handle_F1_reg_64();                      // Verarbeiten der Zeitstempel
#else      
        handle_F1_reg();                      // Verarbeiten der Zeitstempel
#endif      
      }  
      break;
    }  
  }
} 

void starte_core1(void) {
uint32_t cmd_tab[] = {0, 0, 1,                      // default
                     (uint32_t) 0x10000100,         // std. vec_tab 
                     (uint32_t)&core1_stack[STACK_LEN-1], // stack
                     (uint32_t)core1_loop};         // start
uint32_t seq = 0, cmd;
// reset core1
  PSM_SET->FRCE_OFF = PSM_FRCE_OFF_proc1_Msk;       // core1 passiv
  while(!(PSM->FRCE_OFF & PSM_FRCE_OFF_proc1_Msk)); // abwarten
  PSM_CLR->FRCE_OFF = PSM_FRCE_OFF_proc1_Msk;       // Bit wieder loeschen

  do {
    cmd = cmd_tab[seq];
    if (!cmd) {
      while ((SIO->FIFO_ST & SIO_FIFO_ST_VLD_Msk))  
        SIO->FIFO_RD;                               // FIFO loeschen
    }
    while (!(SIO->FIFO_ST & SIO_FIFO_ST_RDY_Msk));  // FIFO abwarten
    SIO->FIFO_WR = cmd;                             // neuer Befehl
    __SEV();                                        // trigger
    while (!(SIO->FIFO_ST & SIO_FIFO_ST_VLD_Msk))
      __WFE();
    if(cmd == SIO->FIFO_RD) seq++;                  // Antwort vergleichen
    else seq = 0;                   // bei Fehler: neuer Zyklus
  } while (seq < 6);                // Tabellenlaenge abarbeiten
}

#endif

void init_PIO1_SM1(void)
{
// PIO1 aktivieren  
// Programm ab ADR0  
  RESETS_CLR->RESET = RESETS_RESET_pio1_Msk;
  while (!(RESETS->RESET_DONE & RESETS_RESET_pio1_Msk));

// FREF_PIN fuer SM0 initialisieren
  IO_BANK0->GPIO22_CTRL = IO_BANK0_GPIO22_CTRL_FUNCSEL_sio_22;// GPIO-Eingang
  PADS_BANK0->GPIO22 = PADS_BANK0_GPIO22_IE_Msk |             // pullup + schmitttrigger
                      PADS_BANK0_GPIO22_PUE_Msk | 
                      PADS_BANK0_GPIO22_SCHMITT_Msk;

// Eingangspin SM1 festlegen  
  PIO1->SM1_EXECCTRL = (31 << PIO0_SM1_EXECCTRL_WRAP_TOP_Pos) | 
                        (0 << PIO0_SM1_EXECCTRL_WRAP_BOTTOM_Pos) |
                        (FREF_PIN  << PIO0_SM1_EXECCTRL_JMP_PIN_Pos);  // Pin fuer JMP
  PIO1_SET->SM1_SHIFTCTRL = PIO0_SM1_SHIFTCTRL_FJOIN_RX_Msk | // FIFO = 8
                            PIO0_SM1_SHIFTCTRL_AUTOPUSH_Msk;  // autopush
// ADR0  
  PIO1->INSTR_MEM0 = 0x0241;      // jmp x-- -> ADR1, 2 x delay
// ADR1
  PIO1->INSTR_MEM1 = 0x00c0;      // jmp FIN1_PIN -> ADR0 warten bis '0'
// input-pin 1 -> 0
  PIO1->INSTR_MEM2 = 0x4020;      // mov x->isr, x-Wert ausgeben
// ADR3
  PIO1->INSTR_MEM3 = 0x0044;      // jmp x-- -> ADR4
// ADR4  
  PIO1->INSTR_MEM4 = 0x00c1;      // jmp FIN1_PIN 0-> 1 dann ADR1
  PIO1->INSTR_MEM5 = 0x0103;      // jmp ADR3, 1 x delay
  
  PIO1->SM1_CLKDIV = 1 << PIO0_SM0_CLKDIV_INT_Pos |
                     0 << PIO0_SM0_CLKDIV_FRAC_Pos;
  PIO1_SET->CTRL = (BIT(SM1) << (PIO0_CTRL_SM_ENABLE_Pos));   // PIO1_SM1 starten
}

// Zeitstempel erzeugen:
// DMA-Wert = Zeitpunkt, DMA-TRANS-COUNT = Ereignisse
void init_dma_pio1_sm1(void)
{
  RESETS_CLR->RESET = RESETS_RESET_dma_Msk | RESETS_RESET_busctrl_Msk;
  while (!(RESETS->RESET_DONE & RESETS_RESET_dma_Msk));
  BUSCTRL->BUS_PRIORITY = BUSCTRL_BUS_PRIORITY_DMA_W_Msk |
                          BUSCTRL_BUS_PRIORITY_DMA_R_Msk;

// PIO1_SM1  
  DMA->CH4_READ_ADDR = (uint32_t)  &PIO1->RXF1;                // capture 
  DMA->CH4_WRITE_ADDR = (uint32_t) &Fref_time_stamp;              // und sichern
  DMA->CH4_TRANS_COUNT = DMA_COUNT;
  DMA->CH4_CTRL_TRIG =  DREQ_PIO1_RX1 << DMA_CH4_CTRL_TRIG_TREQ_SEL_Pos |     // PIO1_RXF1
                        5 << DMA_CH4_CTRL_TRIG_CHAIN_TO_Pos |   // trigger CH5
                        2 << DMA_CH4_CTRL_TRIG_DATA_SIZE_Pos |  // word
                        DMA_CH4_CTRL_TRIG_EN_Msk;               // enable
// triggered by CH5 and triggers CH4 again  
  DMA->CH5_READ_ADDR = (uint32_t) &PIO1->RXF1;                  // transfer FIFO loeschen     
  DMA->CH5_WRITE_ADDR = (uint32_t) &dma_dummy;                  // to dummy location
  DMA->CH5_TRANS_COUNT = 1;                                     // only 1 transfer
  DMA->CH5_CTRL_TRIG =  DREQ_AUTO << DMA_CH5_CTRL_TRIG_TREQ_SEL_Pos |  // free running
                        4 << DMA_CH5_CTRL_TRIG_CHAIN_TO_Pos |   // Trigger CH4  
                        2 << DMA_CH5_CTRL_TRIG_DATA_SIZE_Pos |  // word
                        DMA_CH5_CTRL_TRIG_EN_Msk;               // enable
  
}

// Zeitstempel lesen
void get_Fref_time_count(uint32_t *time, uint32_t *count)
{
uint32_t temp1, temp2;  
  DMA_CLR->CH4_CTRL_TRIG = DMA_CH4_CTRL_TRIG_EN_Msk;  // stop DMA
  temp1 =   DMA->CH4_AL1_TRANS_COUNT_TRIG;            // count
  temp2 = Fref_time_stamp;                              // get time stamp
  DMA_SET->CH4_CTRL_TRIG = DMA_CH4_CTRL_TRIG_EN_Msk;  // continue DMA
  *count = temp1;
  *time  = temp2;
}  
    
void loesche_gps_filter(void)
{
  gps_fil_laenge = abgleich_zeit;
  for(gps_fil_index = 0; gps_fil_index < gps_fil_laenge; gps_fil_index++)
    gps_fil[gps_fil_index] = 0l;
  gps_fil_index = 0;
  gps_fil_cnt = 0;
  gps_fil_sum = 0.0;
  gps_led_cnt = 0;                          // GPS-Signal fehlt: LED ausschalten
  ee_schreib_intervall = gps_fil_laenge;    // neu warten
  gps_totzeit = GPS_TOTZEIT;
}

void bewerte_gps_signal(double frequenz_1Hz) {    // auf 1 Hz normiert
uint16_t  led_temp; 
double f;

  gps_fil_sum -= gps_fil[gps_fil_index];    // letzen Wert abziehen
  gps_fil_sum += frequenz_1Hz;              // neuen Wert addieren
  gps_fil[gps_fil_index++] = frequenz_1Hz;  // und ablegen
  if(gps_fil_index >= gps_fil_laenge)
    gps_fil_index = 0;
  gps_fil_cnt++;                            // mitzaehlen, ob Filter 'gefuellt'
  if(gps_fil_cnt >= gps_fil_laenge) {       // Filter ist 'eingeschwungen'
    gps_fil_cnt = gps_fil_laenge;           // nicht weiter zaehlen
    ref_frequenz = (gps_fil_sum / gps_fil_laenge) * F_ref; // wieder skalieren
    gps_led_cnt = GPS_TIMEOUT;              // Dauerlicht durch Nachtriggern
//    F1_taktfrequenz = ref_frequenz * PIO_TIME_DIV;         // jede Sekunde neu anpassen
// Warnung wg. volatile vermeiden
    f = F_ref - ref_frequenz;               // auf 0,1 ppb/Schritt
    offset_variabel = (int)(f / F1_offset_schritt / F_ref); // skalieren
    ee_schreib_intervall++;
    if(ee_schreib_intervall >= gps_fil_laenge) {
      ee_schreib_intervall = 0;             // neu zaehlen
      speicher_var_offset(offset_variabel); // zyklisch letzten Wert speichern                                      
    }
  } else {
    if(gps_fil_laenge >= GPS_LED_STUFEN) {  // Abstufung, um zu kurze Zeiten zu verhindern
      led_temp = (gps_fil_cnt/(gps_fil_laenge/GPS_LED_STUFEN))*(gps_fil_laenge/GPS_LED_STUFEN);
      if(led_temp <= (gps_fil_laenge/GPS_LED_STUFEN)/2) 
        led_temp = (gps_fil_laenge/GPS_LED_STUFEN)/2+1;  // minimal 1
    } else led_temp = gps_fil_cnt;                  // Dauerlicht, wenn Filter gefuellt ist
    gps_led_cnt = (1000*led_temp)/gps_fil_laenge;   // Blinkzeit in ms, wenn Filter noch nicht voll ist
  }
}

void Fgps_messung(void)
{
static uint8_t  init = 0, korrektur_laeuft = 0;
uint32_t eff_stellen;
static char s[TEMP_STR_LEN];
double F_temp, Fref_temp;
const char *str0[] = {
                "  Ref. Signal   ",
                "  ref. signal   "};
const char *str1[] = {
                " Frequenz F-Ref ",
                "frequency F-ref "};
const char *str2[] = {
                "    Signal?     ",
                "    signal?     "};

    if(!init) {
      init = 1;
      init_dma_pio1_sm1();                    // time stamps DMA
      init_PIO1_SM1();                        // PIO1 SM1 nur bei lokalem Takt aktivieren
      F_ref = (SystemCoreClock/PIO_TIME_DIV); // hier mit 1/4 Takt
      Fref_timeout_counter = F2_timeout;      // init nach Timeout erzwingen
    }
    if(Fref_timeout_counter >= F2_timeout) {
      Fref_mess_dauer =  F2_messzeit;         // und 1. Messung sofort abschliessen
      Fref_messwert_vorhanden  = 0;           // dabei 1. Ergebnis verwerfen
      Fref_timeout_counter = 0;
      loesche_gps_filter();                   // neu bewerten
      if(korrektur_modus)
        lcd_zeile3((char *)str0[sprache]); 
      else
        lcd_zeile3((char *)str1[sprache]); 
      lcd_zeile4((char *)str2[sprache]);
    } 
  

  if(Fref_mess_dauer >= F2_messzeit) {        // wenn Mindestzeit erreicht
    get_Fref_time_count(&Fref_end_zeit, &Fref_end_ereignis);
// Fref
    Fref_mess_zeit = Fref_start_zeit-Fref_end_zeit;
    Fref_start_zeit = Fref_end_zeit;
    Fref_mess_ereignisse = Fref_start_ereignis-Fref_end_ereignis & DMA_COUNT_MASKE;
    Fref_start_ereignis = Fref_end_ereignis;
    
    Fref_timeout_counter = 0;

    Fref_mess_dauer = 0;                        // und Messzeit abwarten
    if(Fref_mess_ereignisse) {
      if(Fref_messwert_vorhanden) {             // Ergebnis berechnen und anzeigen/ausgeben

// unkorrigierter Takt (SystemCoreClock) zum Vergleich mit 1pps-Signal
        if(korrektur_modus) {
          if(!korrektur_laeuft) {
            loesche_gps_filter();               // gps war noch nicht aktiviert
            korrektur_laeuft = 1;                     // ist jetzt initialisiert
          }  
          F_temp = (double)Fref_mess_ereignisse/(double)Fref_mess_zeit * 
                          (SystemCoreClock)/PIO_TIME_DIV;
          if(F_temp >= FREF1_LOW && F_temp <= FREF1_HIGH) {     
            Fref_temp = FREF1;                  // 1 Hz
          }   
          else if(F_temp >= FREF2_LOW && F_temp <= FREF2_HIGH) {
            Fref_temp = FREF2;                  // 10 kHz
          } 
          else if(F_temp >= FREF3_LOW && F_temp <= FREF3_HIGH) {
            Fref_temp = FREF3;                  // 1 MHz
          }  
          else if(F_temp >= FREF4_LOW && F_temp <= FREF4_HIGH) {
            Fref_temp = FREF4;                  // 10 MHz
          }  
          else {
            F_temp = 0.0;                       // keine gueltige Fref gefunden
            loesche_gps_filter();               // gps war noch nicht aktiviert
          }  
          if(Fref_temp != Fref_aktuell) {       // Fref geaendert
            loesche_gps_filter();               // gps war noch nicht aktiviert
            Fref_aktuell = Fref_temp;
          }  
          if(gps_totzeit) {                     // Einschwingen abwarten
            gps_totzeit--;
          } else {
            F_temp /= Fref_temp;                // auf 1 Hz normiert
            bewerte_gps_signal(F_temp);         // nur engtoleriertes Signal verwenden
          }  
        } else {
          korrektur_laeuft = 0;                 // kein gueltiges Signal
        }  
      
        eff_zeit2 = (double)(Fref_mess_zeit ) / F_ref;  // Zeit in Sekunden
        Fref_ergebnis = ((double)Fref_mess_ereignisse) / eff_zeit2; // Frequenz berechnen
        Fref_ergebnis += Fref_ergebnis * (offset_variabel * F1_offset_schritt);
        Fref_ergebnis *= F2_faktor;
        eff_stellen = F2_stellen;
        if(!eff_stellen) eff_stellen = berechne_stellen(eff_zeit2,1);
        Fx_str_mittig((char *)"",Fref_ergebnis, WANDEL_F, s, eff_stellen);
        if(korrektur_modus)
          lcd_zeile3((char *)str0[sprache]); 
        else
          lcd_zeile3((char *)str1[sprache]); 
        lcd_messwert_zeile4(s); 
        if(ser_ausgabe_kanal == F2) {
          wandel_Fx("",Fref_ergebnis, WANDEL_F, s, eff_stellen);
          sende_str1(s);
        }  
  
      } else 
        Fref_messwert_vorhanden = 1;            // sperre wieder aufheben
    }
  }
}  
