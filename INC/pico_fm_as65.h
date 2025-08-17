/*
2024-02-29
Michael Nowak
http://www.mino-elektronik.de
Alle Angaben ohne Gewaehr !
*/

#ifndef PICO_FMETER_H
#define PICO_FMETER_H

#define BIT(x) (1<<x)

#define DEF_SYSCLOCK     133000000  // default
#define MHZ_12            12000000  // default
#define AS6501_REF        10000000  // TCXO or ext. clock

#define MHZ_FAKTOR        1000000   // 1e6
#define MS_TEILER         1000      // us -> ms

#define REF_DIV           2         // PLL-Schrittweite 6 MHZ @ 12 MHz
#define PLL_DIV1          5         // force high F_vco
#define PLL_DIV2          2         // default = 2; 1 => turbo-mode

#define PIO_TIME_DIV      4         // 1/4 Taktfrequenz

enum {PRIO_HIGH = 0, PRIO_MID_HIGH, PRIO_MID_LOW, PRIO_LOW};


/* Verwendung der PWM-Kanaele
PWM_CH0   LCD-Kontrast
PWM_CH1   LCD-delay 1 us
PWM_CH2   Ereigniszähler
PWM_CH3
PWM_CH4
PWM_CH5   freilaufend fuer alt. Zeitmessung
PWM_CH6   50 kHz fuer Regression
PWM_CH7   1 kHz fuer Timing der Messung
*/
#define INT_TIMER         5         // Timer 5 alternative Zeitmessung
#define REG_TIMER         6         // Timer 6 50 kHz
#define F1_TIMER          7         // Timer 7 1 kHz

/* Verwendung der GPIO
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
*/
#define SPI1_SCLK         10        // fuer AS6501
#define SPI1_TX           11
#define SPI1_RX           12
#define SPI1_SSN          13

#define INT_REQ           14        // Eingang
#define EXT_LED           15        // externe LED
#define PRE_MUX_OUT       16        // 1=Vorteiler 4:1 aktiv
#define CAP1_PIN          17        // PIO0-SM2: capture Ereigniszaehler 
#define GPS_LED           18        // GPS_LED bzw. Laufzeitmessung
#define EXT_REF_IN        19        // Test auf ext. 10 MHz
#define REF_MUX_IN_OUT    20        // Abfrage oder gezielte Umschaltung
#define FIN1_PIN          21        // PIO-SM1-Input: Ereigniszaehler per DMA_CH1
#define FREF_PIN          22        // PIO-Input, wenn benutzt
#define LOCAL_LED         25        // on board bei WIFI abschalten!
#define IIC_SDA           26        // 
#define IIC_SCL           27        //

#define ADC_POTI          2         // GPIO28
#define ADC_MAX           4095
#define ADC_FILTER        30        // ext. R-trimm

#define DEF_BAUDRATE      115200

#endif