#ifndef TASTEN_DEF
#define TASTEN_DEF

#define ENTPRELL_ZEIT   5       	// 5 Zyklen -> 50ms entprellen
#define FIRST_REPEAT    70      	// 0,5 Sek. Pause bei gedrueckter Taste
#define NEXT_REPEAT     10      	// danach Wiederholung alle 0,1 Sek.
#define TASTEN_OFFSET    6        // ab GPIO6
#define TASTEN_MASKE     7        // 3 bits


#define MAX_BITS            8
#define PLUS_TASTE          'A'
#define PLUS_REPEAT         'a'
#define MINUS_TASTE         'B'
#define MINUS_REPEAT        'b'
#define PLUS_MINUS          'C'
#define INIT_TASTE          'D'
#define ABBRUCH_TASTE       'X'

#define PLUS_BIT            1       // Bits in dekodierter Form lt. deko_tab[]
#define MINUS_BIT           2
#define PLUS_MINUS_BIT      4
#define INIT_BIT            8

#endif