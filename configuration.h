#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#define DS_SENSOR_SHARP_GP2Y0A02YK 0
#define DS_SENSOR_SHARP_GP2Y0A21YK 1

/* Maxim: use DS_SENSOR_SHARP_GP2Y0A21YK */
#define DS_SENSOR DS_SENSOR_SHARP_GP2Y0A21YK
/* Admar: use DS_SENSOR_SHARP_GP2Y0A02YK */
//#define DS_SENSOR DS_SENSOR_SHARP_GP2Y0A02YK

/* DS_CLOSEST and DS_FARTHEST define the range in which the lamp should operate */

#if (DS_SENSOR == DS_SENSOR_SHARP_GP2Y0A21YK)
#define DS_CLOSEST 150
#define DS_FARTHEST 440
#else
#define DS_CLOSEST 210
#define DS_FARTHEST 500
#endif

/* CS_LAB_STEP_SIZE_AB defines the speed at which the color changes */
#define CS_LAB_STEP_SIZE_AB 0.75 // 0.25

/* CS_LAB_STEP_SIZE_L defines the speed at which the lightness changes */
#define CS_LAB_STEP_SIZE_L 1

/* Lightness (L) values of lowest intensity red (0x010000), green (0x000100)
 * blue (0x000001) and white (0x010101) are 0.058, 0.196, 0.020 and 0.274. 
 * --> Choose 0.274 as minimum lightness. That way, lamp power state will never be 
 * on with a lightness that is so low that the actual output value is 0x000000 (and 
 * thus appears to be off, causing potential confusion to the user). */
//#define CS_LAB_MIN_L 0.196
#define CS_LAB_MIN_L 0.274

/* Lightness (L) values of highest intensity red (0xFF0000), green (0x00FF00)
 * and blue (0x0000FF) are 53.233, 87.737 and 32.303. --> Choose 32.303 as maximum
 * lightness. That way, full color spectrum is available at highest lightness. */
//#define CS_LAB_MAX_L 32.303
#define CS_LAB_MAX_L 53.233

//#define CS_LAB_WHITE_POINT_A 0
//#define CS_LAB_WHITE_POINT_B 0
#define CS_LAB_WHITE_POINT_A 10
#define CS_LAB_WHITE_POINT_B 30

/* Enable next line to test electronics. This will repeatedly show red, green and blue
 * in low and high intensity. The serial port will show raw and processed values of
 * captouch and distance sensors. An example output is shown here:
 *
 *   raw:   521; avg:   522; delta:     1; state: 1   raw:   508; avg:   547; delta:    39; state: 3   raw:   488; avg:   489; delta:     1; state: 1   raw:   550; avg:   549; delta:    -1; state: 1	raw:   148; distance:   394
 *
 * The first 4 columns (raw, avr, delta and state) show the output of the first
 * captouch sensor (red). The most important value is state. State can be one of the
 * following values:
 *   0: calibrating
 *   1: released (not touched)
 *   2: released to touched (sensor is touched but not yet long enough to be registered as a real touch)
 *   3: touched
 *   4: touched to released (sensor is not touched but not yet long enough to be registered as released)
 *
 * Columns 5 - 8 show the output of the 2nd captouch sensor (green), 9 - 12 show the
 * output of the 3rd captouch sensor (blue), 13 - 16 show the output of the 4th
 * captouch sensor (white). Finally, columns 17 and 18 show the raw and processed value
 * of the distance sensor. */
//#define TEST_ELECTRONICS
#endif
