#include <math.h>
#include "distance_sensor.h"

#if (DS_SENSOR == DS_SENSOR_SHARP_GP2Y0A02YK)
#define DS_LOGLOG_P0 5.9475
#define DS_LOGLOG_P1 -1.0918
#endif

#if (DS_SENSOR == DS_SENSOR_SHARP_GP2Y0A21YK)
#define DS_LOGLOG_P0 4.7351
#define DS_LOGLOG_P1 -1.2106
#endif

#define log2(x) (log(x) / log(2.0))

#define DS_BUF_LENGTH 800

ds_t ds;

int ds_log2(int x)
{
  int y = 0;
  
  if (x == 0)
    return y; // error; cannot compute log of 0
  
  while (x >>= 1)
    y++;

  return y;
}

void ds_get_reading(void)
{
  int n, tmp = 0;
  
  tmp = 0;
  
  for (n = 0; n < DS_OVERSAMPLING; n++)
    tmp += analogRead(DS_PIN);
  
  ds.reading = tmp / DS_OVERSAMPLING;
}

void ds_reading_to_distance(void)
{
  float log2_d, log2_v, v;
  
  v = ((float) ds.reading) * 5.0 / 1023;
  log2_v = log2(v);
  log2_d = DS_LOGLOG_P0 + DS_LOGLOG_P1 * log2_v;
  ds.distance = (unsigned int) (10 * pow((float) 2.0, (float) log2_d));
}

void ds_serial_write(char *s)
{
  unsigned int n, N;
  
  N = strlen(s);
  
  for (n = 0; n < N; n++)
    Serial.print(s[n]);
}

void ds_print_debug(void)
{
  int buf_index;
  char buf[DS_BUF_LENGTH];
  
  buf_index = 0;
  buf_index += snprintf(&(buf[buf_index]), DS_BUF_LENGTH - buf_index, \
     "raw: % 5u; distance: % 5d\n", ds.reading, ds.distance);
  ds_serial_write(buf);
}

void ds_print_log(void)
{
  int x, buf_index;
  float v, log2_d, log2_v, d;
  char buf[DS_BUF_LENGTH];
  
  for (x = 0; x < 1023; x = x + 10) {
    v = ((float) x) * 5 / 1023;
    log2_v = log2(v);
    log2_d = DS_LOGLOG_P0 + DS_LOGLOG_P1 * log2_v;
    d = pow((float) 2.0, (float) log2_d);
  
    buf_index = 0;
    buf_index += snprintf(&(buf[buf_index]), DS_BUF_LENGTH - buf_index, \
       "x: % 5d, v: %d, log2_v: %d, log2_d: %d, d: %d\n", \
       x, (int) (v * 1000), (int) (log2_v * 1000), (int) (log2_d * 1000), (int) d);
    ds_serial_write(buf);
  }
}
