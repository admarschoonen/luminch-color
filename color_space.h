#ifndef _COLOR_SPACE_H_
#define _COLOR_SPACE_H_

#include "configuration.h"

/* For 8 bit PWM, the maximum step size for RGB color space is
 * approximately 0.0017. (Converting the RGB number (1 - 0.0017) to 
 * linear space scaled to 255 gives 254.01.)
 * We probably also would like the actual step size to be an integer
 * fraction of 1 (to ensure that it can cover full scale from 0 to 255) -->
 * actual step size is 1 / ceil(1 / 0.0017) = 1 / 589.
 *
 * Note that the lowest brightness in that case is 3 * step size 
 * (30 when using the official gamma function) */

#define CS_RGB_N_STEPS 589
#define CS_RGB_STEP_SIZE (1.0 / CS_RGB_N_STEPS)
#define CS_RGB_SMALLEST_POSITIVE (3 * CS_RGB_STEP_SIZE)
/* Lightness (L) values of lowest intensity red (0x010000), green (0x000100)
 * blue (0x000001) and white (0x010101) are 0.058, 0.196, 0.020 and 0.274. 
 * --> Choose 0.274 as minimum lightness. That way, lamp power state will never be 
 * on with a lightness that is so low that the actual output value is 0x000000 (and 
 * thus appears to be off, causing potential confusion to the user). */
#define CS_LAB_MIN_L 0.196
/* Lightness (L) values of highest intensity red (0xFF0000), green (0x00FF00)
 * and blue (0x0000FF) are 53.233, 87.737 and 32.303. --> Choose 32.303 as maximum
 * lightness. That way, full color spectrum is available at highest lightness. */
#define CS_LAB_MAX_L 53.233 /* 100.000 */
//#define CS_LAB_MIN_A -110 /*-85*/ /* -86.185 */
//#define CS_LAB_MAX_A 110 /*95*/ /* 98.254 */
//#define CS_LAB_MIN_B -110 /*-105*/ /* -107.864 */
//#define CS_LAB_MAX_B 110 /*90*/ /* 94.482 */
//#define CS_RGB_SMALLEST_POSITIVE (1 * CS_RGB_STEP_SIZE)

#define CS_LAB_WHITE_POINT_L 50
//#define CS_LAB_WHITE_POINT_A 0
//#define CS_LAB_WHITE_POINT_B 0
#define CS_LAB_WHITE_POINT_A 10
#define CS_LAB_WHITE_POINT_B 30

#define CS_LAB_RED_POINT_A 80.109
#define CS_LAB_RED_POINT_B 67.220
#define CS_LAB_GREEN_POINT_A -86.185
#define CS_LAB_GREEN_POINT_B 83.181
#define CS_LAB_BLUE_POINT_A 79.197
#define CS_LAB_BLUE_POINT_B -107.864

//#define CS_LAB_YELLOW_POINT_A -21.556
//#define CS_LAB_YELLOW_POINT_B 94.482
#define CS_LAB_YELLOW_POINT_A 21.556
#define CS_LAB_YELLOW_POINT_B 94.482
#define CS_LAB_CYAN_POINT_A -48.080
#define CS_LAB_CYAN_POINT_B -14.138
//#define CS_LAB_MAGENTA_POINT_A 98.254
//#define CS_LAB_MAGENTA_POINT_B -60.843
#define CS_LAB_MAGENTA_POINT_A 80.10
#define CS_LAB_MAGENTA_POINT_B 24.97

/* CS_N_PROBE_POINTS defines the number of probing points in 90 degrees
 * angle when next Lab point is outside gamut. */
#define CS_N_PROBE_POINTS 90

typedef struct cs_linear_t {
  float r;
  float g;
  float b;
} cs_linear_t;

typedef struct cs_RGB_t {
  float R;
  float G;
  float B;
} cs_RGB_t;

typedef struct cs_XYZ_t {
  float X;
  float Y;
  float Z;
} cs_XYZ_t;

typedef struct cs_xyY_t {
  float x;
  float y;
  float Y;
} cs_xyY_t;

typedef struct cs_Lab_t {
  float L;
  float a;
  float b;
} cs_Lab_t;

typedef struct cs_LMS_t {
  float L;
  float M;
  float S;
} cs_LMS_t;

extern void cs_RGB_to_XYZ(cs_RGB_t *RGB, cs_XYZ_t *XYZ);
extern void cs_XYZ_to_RGB(cs_XYZ_t *XYZ, cs_RGB_t *RGB);
extern void cs_RGB_to_linear(cs_RGB_t *RGB, cs_linear_t *linear);
extern void cs_linear_to_RGB(cs_linear_t *linear, cs_RGB_t *RGB);
extern void cs_xyY_to_XYZ(cs_xyY_t *xyY, cs_XYZ_t *XYZ);
extern void cs_XYZ_to_xyY(cs_XYZ_t *XYZ, cs_xyY_t *xyY);
extern void cs_XYZ_to_Lab(cs_XYZ_t *XYZ, cs_Lab_t *Lab);
extern void cs_Lab_to_XYZ(cs_Lab_t *Lab, cs_XYZ_t *XYZ);
extern void cs_XYZ_to_LMS(cs_XYZ_t *XYZ, cs_LMS_t *LMS);
extern void cs_LMS_to_XYZ(cs_LMS_t *LMS, cs_XYZ_t *XYZ);

extern boolean cs_Lab_inside_RGB(cs_Lab_t *Lab);
extern boolean cs_Lab_find_closest_RGB_with_fixed_L(cs_Lab_t *Lab, cs_Lab_t *Lab_new);
extern boolean cs_Lab_move_towards_white_with_fixed_L(cs_Lab_t *Lab_new, float d);

extern void cs_init_white_balance_correction(void);
extern void cs_white_balance_correction(cs_XYZ_t *XYZ1, cs_XYZ_t *XYZ2, float L, float s);

extern void cs_print_RGB(cs_RGB_t *RGB);
extern void cs_print_XYZ(cs_XYZ_t *XYZ);
extern void cs_print_Lab(cs_Lab_t *Lab);
extern void cs_print_LMS(cs_LMS_t *LMS);
#endif // _COLOR_SPACE_H_
