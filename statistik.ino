/*
2023-03-01
Michael Nowak
http://www.mino-elektronik.de
Alle Angaben ohne Gewaehr !
*/

#define FM_STATISTIK
 
#include <math.h>

int stat_n, stat_new_value, stat_F1_stellen;
double stat_offset, stat_sum_x, stat_sum_x2;
double stat_value, stat_max, stat_min, stat_mean;
	
void init_stat()
{
  stat_n = 0;
	stat_sum_x = stat_sum_x2 = stat_offset = 0.0;
  stat_mean = stat_max = stat_min = 0.0;
}	
	
void stat_new(double x, int stellen)
{
	if(stat_n == 0) {
    stat_offset = x;			// Differenz merken
    stat_max = stat_min = x;
    stat_sum_x = stat_sum_x2 = 0.0;
  } 
  stat_F1_stellen = stellen;
  stat_value = x;
  if(x > stat_max) stat_max = x;
  if(x < stat_min) stat_min = x;
  stat_new_value = 1;
	stat_n++;
	stat_sum_x	+= x - stat_offset;					// einfache Summe
	stat_sum_x2 += (x - stat_offset) * (x - stat_offset);	// Summe der Quadrate
  stat_mean = stat_offset + stat_sum_x / stat_n;
}
	
double stat_varianz()
{
  if(stat_n < 2) return(0.0);
	return(stat_sum_x2 - (stat_sum_x * stat_sum_x) / stat_n) / (stat_n-1);
}

double stat_sdev()
{
	return(sqrt(stat_varianz()));
}
