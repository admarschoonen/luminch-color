#include "Arduino.h"
#include "color_space.h"
#include <math.h>

#define CS_LIMIT(x, l, u) (x < l ? l : (x > u ? u : x))
#define CS_IN_RANGE(x, l, u) (x < l ? false : (x > u ? false : true))
#define CS_PROBE_ANGLE (90 / CS_N_PROBE_POINTS)

cs_LMS_t whitepoint_LMS;

float cs_linear_to_X_official(float X)
{
  float Y;
  
  if (X <= 0.0031308)
    Y = 12.92 * X;
  else
    Y = (1 + 0.055) * pow(X, 1/2.4) - 0.055;
  
  //Y = CS_LIMIT(Y, 0.0, 1.0);
 
  return Y;
}

float cs_linear_to_X_22(float X)
{
  float Y;
  
  Y = pow(X, 1/2.2);
  
  Y = CS_LIMIT(Y, 0.0, 1.0);
 
  return Y;
}

float cs_X_to_linear_official(float X)
{
  float Y;
  
  if (X <= 0.04045)
    Y = X / 12.92;
  else
    Y = pow((X + 0.055) / (1 + 0.055), 2.4);
  
  //Y = CS_LIMIT(Y, 0.0, 1.0);
  return Y;
}

float cs_X_to_linear_22(float X)
{
  float Y;
  
  Y = pow(X, 2.2);
  
  //Y = CS_LIMIT(Y, 0.0, 1.0);
  return Y;
}

void cs_RGB_to_linear(cs_RGB_t *RGB, cs_linear_t *linear)
{
  /* Use gamma of 2.2 instead of official gamma function. 
   * Official gamma function seems to have some visible artefacts
   * on the lower end; gamma 2.2 does not */
  linear->r = cs_X_to_linear_22(RGB->R);
  linear->g = cs_X_to_linear_22(RGB->G);
  linear->b = cs_X_to_linear_22(RGB->B);
}

void cs_RGB_to_linear_official(cs_RGB_t *RGB, cs_linear_t *linear)
{
  linear->r = cs_X_to_linear_official(RGB->R);
  linear->g = cs_X_to_linear_official(RGB->G);
  linear->b = cs_X_to_linear_official(RGB->B);
}

void cs_linear_to_RGB(cs_linear_t *linear, cs_RGB_t *RGB)
{
  /* Use gamma of 2.2 instead of official gamma function. 
   * Official gamma function seems to have some visible artefacts
   * on the lower end; gamma 2.2 does not */
  RGB->R = cs_linear_to_X_22(linear->r);
  RGB->G = cs_linear_to_X_22(linear->g);
  RGB->B = cs_linear_to_X_22(linear->b);
}

void cs_linear_to_RGB_official(cs_linear_t *linear, cs_RGB_t *RGB)
{
  RGB->R = cs_linear_to_X_official(linear->r);
  RGB->G = cs_linear_to_X_official(linear->g);
  RGB->B = cs_linear_to_X_official(linear->b);
}

void cs_RGB_to_XYZ(cs_RGB_t *RGB, cs_XYZ_t *XYZ)
{
  float R, G, B;
  cs_linear_t linear;
  
  cs_RGB_to_linear_official(RGB, &linear);
  
  R = linear.r * 100.0;
  G = linear.g * 100.0;
  B = linear.b * 100.0;
  
  XYZ->X = 0.4124 * R + 0.3576 * G + 0.1805 * B;
  XYZ->Y = 0.2126 * R + 0.7152 * G + 0.0722 * B;
  XYZ->Z = 0.0193 * R + 0.1192 * G + 0.9505 * B;
  
  return;
  
  
  R = RGB->R;
  G = RGB->G;
  B = RGB->B;
  
  XYZ->X = 1 / 0.17697 * (0.49 * R + 0.31 * G + 0.20 * B);
  XYZ->Y = 1 / 0.17697 * (0.17697 * R + 0.81240 * G + 0.01063 * B);
  XYZ->Z = 1 / 0.17697 * (0.00 * R + 0.01 * G + 0.99 * B);
}

void cs_XYZ_to_RGB(cs_XYZ_t *XYZ, cs_RGB_t *RGB)
{
  float X, Y, Z;
  cs_linear_t linear;

  X = XYZ->X / 100.0;
  Y = XYZ->Y / 100.0;
  Z = XYZ->Z / 100.0;
  
  linear.r =  3.2406 * X + -1.5372 * Y + -0.4986 * Z;
  linear.g = -0.9689 * X +  1.8758 * Y +  0.0415 * Z;
  linear.b =  0.0557 * X + -0.2040 * Y +  1.0570 * Z;

  cs_linear_to_RGB_official(&linear, RGB);
  return;


  X = XYZ->X;
  Y = XYZ->Y;
  Z = XYZ->Z;
  
  RGB->R = 0.41847 * X + -0.15866 * Y + -0.082835 * Z;
  RGB->G = -0.091169 * X + 0.25243 * Y + 0.015708 * Z;
  RGB->B = 0.00092090 * X + -0.0025498 * Y + 0.17860 * Z;
  
  RGB->R = CS_LIMIT(RGB->R, 0, 1);
  RGB->G = CS_LIMIT(RGB->G, 0, 1);
  RGB->B = CS_LIMIT(RGB->B, 0, 1);  
}

void cs_xyY_to_XYZ(cs_xyY_t *xyY, cs_XYZ_t *XYZ)
{
  float f;
  
  f = xyY->Y / xyY->y;
  
  XYZ->X = f * xyY->x;
  XYZ->Y = xyY->Y;
  XYZ->Z = f * (1 - xyY->x - xyY->y);
}

void cs_XYZ_to_xyY(cs_XYZ_t *XYZ, cs_xyY_t *xyY)
{
  float S;
  
  S = XYZ->X + XYZ->Y + XYZ->Z;
  
  xyY->x = XYZ->X / S;
  xyY->y = XYZ->Y / S;
  xyY->Y = XYZ->Y;
}

void cs_XYZ_to_Lab_Hunter(cs_XYZ_t *XYZ, cs_Lab_t *Lab)
{
  float sq_Y;
  
  sq_Y = sqrt(XYZ->Y);
  Lab->L = 10 * sq_Y;
  Lab->a = 17.5 * ( ((1.02 * XYZ->X) - XYZ->Y) / sq_Y );
  Lab->b = 7 * ( (XYZ->Y - (0.847 * XYZ->Z)) / sq_Y );
}

void cs_Lab_to_XYZ_Hunter(cs_Lab_t *Lab, cs_XYZ_t *XYZ)
{
  float X, Y, Z;
  
  Y = Lab->L / 10;
  X = Lab->a / 17.5 * Lab->L / 10;
  Z = Lab->b / 7 * Lab->L / 10;
  
  XYZ->Y = Y * Y;
  XYZ->X = (X + XYZ->Y) / 1.02;
  XYZ->Z = -(Z - Y) / 0.847;
}

float cs_XYZ_to_Lab_x(float x)
{
  float y;
  
  if (x > 0.008856)
    y = pow(x, 1.0/3.0);
  else
    y = 7.787 * x + 16.0 / 116.0;
  
  return y;
}

void cs_XYZ_to_Lab(cs_XYZ_t *XYZ, cs_Lab_t *Lab)
{
  float X, Y, Z;
  /* Observer: 2 degrees, illuminant: D65 */
  const float X_ref = 95.047, Y_ref = 100.000, Z_ref = 108.8833;
  
  X = XYZ->X / X_ref;
  Y = XYZ->Y / Y_ref;
  Z = XYZ->Z / Z_ref;
/*  Serial.print("  X: ");
  Serial.println(X * 100, DEC);
  Serial.print("  Y: ");
  Serial.println(Y * 100, DEC);
  Serial.print("  Z: ");
  Serial.println(Z * 100, DEC);*/
  
  X = cs_XYZ_to_Lab_x(X);
  Y = cs_XYZ_to_Lab_x(Y);
  Z = cs_XYZ_to_Lab_x(Z);
/*  Serial.print("  X: ");
  Serial.println(X * 100, DEC);
  Serial.print("  Y: ");
  Serial.println(Y * 100, DEC);
  Serial.print("  Z: ");
  Serial.println(Z * 100, DEC);*/
  
  Lab->L = 116.0 * Y - 16.0;
  Lab->a = 500.0 * (X - Y);
  Lab->b = 200.0 * (Y - Z);
}

float cs_Lab_to_XYZ_x(float x)
{
  float x3, y;
  
  x3 = x * x * x;
  
  if (x3 > 0.008856)
    y = x3;
  else
    y = (x - 16.0 / 116.0) / 7.787;
    
  return y;
}

void cs_Lab_to_XYZ(cs_Lab_t *Lab, cs_XYZ_t *XYZ)
{
  float X, Y, Z;
  /* Observer: 2 degrees, illuminant: D65 */
  const float X_ref = 95.047, Y_ref = 100.000, Z_ref = 108.8833;
  
  Y = (Lab->L + 16.0) / 116.0;
  X = Lab->a / 500.0 + Y;
  Z = Y - Lab->b / 200.0;
  
  X = cs_Lab_to_XYZ_x(X);
  Y = cs_Lab_to_XYZ_x(Y);
  Z = cs_Lab_to_XYZ_x(Z);
  
  XYZ->X = X_ref * X;
  XYZ->Y = Y_ref * Y;
  XYZ->Z = Z_ref * Z;
}

void cs_XYZ_to_LMS(cs_XYZ_t *XYZ, cs_LMS_t *LMS)
{
  float X, Y, Z;
  
  X = XYZ->X;
  Y = XYZ->Y;
  Z = XYZ->Z;
  
  LMS->L =  0.8951 * X +  0.2664 * Y + -0.1614 * Z;
  LMS->M = -0.7502 * X +  1.7135 * Y +  0.0367 * Z;
  LMS->S =  0.0389 * X + -0.0685 * Y +  1.0296 * Z;
}

void cs_LMS_to_XYZ(cs_LMS_t *LMS, cs_XYZ_t *XYZ)
{
  float L, M, S;
  
  L = LMS->L;
  M = LMS->M;
  S = LMS->S;
  
  XYZ->X =  0.9869929 * L + -0.1470543 * M +  0.1599627 * S;
  XYZ->Y =  0.4323053 * L +  0.5183603 * M +  0.0492912 * S;
  XYZ->Z = -0.0085287 * L +  0.0400428 * M +  0.9684867 * S;
}

boolean cs_RGB_in_range(cs_RGB_t *RGB)
{
  if (CS_IN_RANGE(RGB->R, 0, 1) &&
      CS_IN_RANGE(RGB->G, 0, 1) &&
      CS_IN_RANGE(RGB->B, 0, 1))
    return true;
  else
    return false;
}

boolean cs_Lab_inside_RGB(cs_Lab_t *Lab)
{
  cs_XYZ_t XYZ;
  cs_RGB_t RGB;
  
  cs_Lab_to_XYZ(Lab, &XYZ);
  cs_XYZ_to_RGB(&XYZ, &RGB);
  
  if (cs_RGB_in_range(&RGB)) {
    return true;
  } else {
    //Serial.println("outside gamut");
    return false;
  }
}

boolean cs_Lab_find_closest_RGB_with_fixed_L(cs_Lab_t *Lab, cs_Lab_t *Lab_new)
{
  float dist, angle0, angle_ccw[CS_N_PROBE_POINTS - 1], angle_cw[CS_N_PROBE_POINTS - 1], da, db;
  bool in_RGB_ccw[CS_N_PROBE_POINTS - 1], in_RGB_cw[CS_N_PROBE_POINTS - 1];
  char n;
  cs_Lab_t Lab_probe_ccw, Lab_probe_cw;
  boolean found = false;
  
  da = Lab_new->a - Lab->a;
  db = Lab_new->b - Lab->b;
  dist = sqrt(da * da + db * db);
  
  angle0 = atan2(db, da);
  Lab_probe_ccw.L = Lab_new->L;
  Lab_probe_cw.L = Lab_new->L;
  
/*  Serial.print("angle0: ");
  Serial.print(angle0 * 180 / PI, DEC);
  Serial.print(", dist: ");
  Serial.println(dist, DEC);*/
  
  /* Probe upto +-90 degrees */
  for (n = 0; n < CS_N_PROBE_POINTS - 1; n++) {
    /* probe counter clockwise */
    //Serial.println(n, DEC);
    angle_ccw[n] = angle0 + (n + 1) * CS_PROBE_ANGLE;
    Lab_probe_ccw.a = Lab->a + dist * cos(angle_ccw[n]);
    Lab_probe_ccw.b = Lab->b + dist * sin(angle_ccw[n]);
    in_RGB_ccw[n] = cs_Lab_inside_RGB(&Lab_probe_ccw);
    
    /* probe clockwise */
    angle_cw[n] = angle0 - (n + 1) * CS_PROBE_ANGLE;
    Lab_probe_cw.a = Lab->a + dist * cos(angle_cw[n]);
    Lab_probe_cw.b = Lab->b + dist * sin(angle_cw[n]);
    in_RGB_cw[n] = cs_Lab_inside_RGB(&Lab_probe_cw);

    if (in_RGB_ccw[n] && in_RGB_cw[n]) {
      /* This happens if CS_PROBE_ANGLE is too large. (Even 1 degree is too large.)
       * Proper solution would be to do binary search, but I'm lazy. Judging from
       * Lab chromaticity diagrams, going clockwise is the proper decision in this
       * case */
      /*Serial.print("WARNING! BOTH CCW AND CW ARE IN RGB! ANGLE: ");
      Serial.println(n * CS_PROBE_ANGLE, DEC);*/
      Lab_new->a = Lab_probe_cw.a;
      Lab_new->b = Lab_probe_cw.b;
      found = true;
      break;
    }
    if (in_RGB_ccw[n]) {
      /*Serial.print("CCW match found; angle: ");
      Serial.println((n + 1) * CS_PROBE_ANGLE, DEC);*/
      Lab_new->a = Lab_probe_ccw.a;
      Lab_new->b = Lab_probe_ccw.b;
      found = true;
      break;
    }
    if (in_RGB_cw[n]) {
      /*Serial.print("CW match found; angle: ");
      Serial.println(-(n + 1) * CS_PROBE_ANGLE, DEC);*/
      Lab_new->a = Lab_probe_cw.a;
      Lab_new->b = Lab_probe_cw.b;
      found = true;
      break;
    }
  }

  return found;
}

boolean cs_Lab_move_towards_white_with_fixed_L(cs_Lab_t *Lab_new, float dist)
{
  float angle, dist_a, dist_b;
  cs_Lab_t Lab_probe;
  
  Lab_probe.L = Lab_new->L;
  Lab_probe.a = Lab_new->a;
  Lab_probe.b = Lab_new->b;

  /* angle with respect to white point (0, 0) */
  angle = atan2(-Lab_new->b, -Lab_new->a);
  dist_a = dist * cos(angle);
  dist_b = dist * sin(angle);
  Serial.print("dist_a: ");
  Serial.print(dist_a, DEC);
  Serial.print(", dist_b: ");
  Serial.print(dist_b, DEC);
  while (!cs_Lab_inside_RGB(&Lab_probe)) {
    if (abs(Lab_probe.a) < dist_a)
      Lab_probe.a = 0;
    else
      Lab_probe.a += dist_a;
    
    if (abs(Lab_probe.b) < dist_b)
      Lab_probe.b = 0;
    else
      Lab_probe.b += dist_b;
    
    if ((Lab_probe.a == 0) && (Lab_probe.b == 0)) {
      //Serial.println("WARNING: FORCING WHITE POINT");
      break; // safety catch
    }
  }
  Lab_new->a = Lab_probe.a;
  Lab_new->b = Lab_probe.b;
  Serial.print("; new a: ");
  Serial.print(Lab_new->a);
  Serial.print(", new b: ");
  Serial.println(Lab_new->b);
}

boolean cs_limit_RGB(cs_RGB_t *RGB)
{
  if (cs_RGB_in_range(RGB)) {
    return false;
  } else {
    CS_LIMIT(RGB->R, 0, 1);
    CS_LIMIT(RGB->G, 0, 1);
    CS_LIMIT(RGB->B, 0, 1);
    return true;
  }
}

void cs_init_white_balance_correction(void)
{
  cs_Lab_t Lab;
  cs_XYZ_t XYZ;
  
  Lab.L = CS_LAB_WHITE_POINT_L;
  Lab.a = CS_LAB_WHITE_POINT_A;
  Lab.b = CS_LAB_WHITE_POINT_B;
  
  cs_Lab_to_XYZ(&Lab, &XYZ);
  cs_XYZ_to_LMS(&XYZ, &whitepoint_LMS);
  if ((whitepoint_LMS.L == 0) || (whitepoint_LMS.M == 0) || (whitepoint_LMS.S == 0)) {
    //Serial.println("Error! whitepoint contains 0 in LMS space!");
    ///cs_print_LMS(&whitepoint_LMS);
  }
}

void cs_white_balance_correction(cs_XYZ_t *XYZ1, cs_XYZ_t *XYZ2, float L, float s)
{
  static int t = millis();
  cs_LMS_t LMS1, LMS2;
  cs_Lab_t Lab_ref1, Lab_ref2;
  cs_XYZ_t XYZ_ref1, XYZ_ref2;
  cs_LMS_t LMS_ref1, LMS_ref2;
  float L_scale, M_scale, S_scale;
  
  Lab_ref1.L = L;
  Lab_ref1.a = 0;
  Lab_ref1.b = 0;
  
  cs_Lab_to_XYZ(&Lab_ref1, &XYZ_ref1);
  cs_XYZ_to_LMS(&XYZ_ref1, &LMS_ref1);

  Lab_ref2.L = L;
  Lab_ref2.a = CS_LAB_WHITE_POINT_A;
  Lab_ref2.b = CS_LAB_WHITE_POINT_B;
  
  if (!cs_Lab_inside_RGB(&Lab_ref2))
    cs_Lab_move_towards_white_with_fixed_L(&Lab_ref2, s);
  
  cs_Lab_to_XYZ(&Lab_ref2, &XYZ_ref2);
  cs_XYZ_to_LMS(&XYZ_ref2, &LMS_ref2);
  
  L_scale = LMS_ref1.L / LMS_ref2.L;
  M_scale = LMS_ref1.M / LMS_ref2.M;
  S_scale = LMS_ref1.S / LMS_ref2.S;
  
  cs_XYZ_to_LMS(XYZ1, &LMS1);
  
  LMS2.L = LMS1.L * L_scale;
  LMS2.M = LMS1.M * M_scale;
  LMS2.S = LMS1.S * S_scale;
  
  cs_LMS_to_XYZ(&LMS2, XYZ2);
  
/*  XYZ2->X = XYZ1->X * XYZ_ref1.X / XYZ_ref2.X;
  XYZ2->Y = XYZ1->Y;
  XYZ2->Z = XYZ1->Z * XYZ_ref1.Z / XYZ_ref2.Z;*/
  
  if ((millis() > t + 500) || (millis() < t)) {
    t = millis();
/*    cs_print_LMS(&LMS_ref1);
    cs_print_LMS(&LMS_ref2);
    cs_print_LMS(&LMS1);
    cs_print_LMS(&LMS2);*/
/*    cs_print_XYZ(&XYZ_ref1);
    cs_print_XYZ(&XYZ_ref2);
    cs_print_XYZ(XYZ1);
    cs_print_XYZ(XYZ2);
    Serial.print(L_scale, DEC);
    Serial.print(", ");
    Serial.print(M_scale, DEC);
    Serial.print(", ");
    Serial.println(S_scale, DEC);
    Serial.println("");*/
  }
}

void cs_print_RGB(cs_RGB_t *RGB)
{
  Serial.print("R: ");
  Serial.print((int) (100 * RGB->R), DEC);
  Serial.print(", G: ");
  Serial.print((int) (100 * RGB->G), DEC);
  Serial.print(", B: ");
  Serial.println((int) (100 * RGB->B), DEC);
}

void cs_print_XYZ(cs_XYZ_t *XYZ)
{
  Serial.print("X: ");
  Serial.print((int) (100 * XYZ->X), DEC);
  Serial.print(", Y: ");
  Serial.print((int) (100 * XYZ->Y), DEC);
  Serial.print(", Z: ");
  Serial.println((int) (100 * XYZ->Z), DEC);
}

void cs_print_Lab(cs_Lab_t *Lab)
{
  Serial.print("L: ");
  Serial.print((int) (100 * Lab->L), DEC);
  Serial.print(", a: ");
  Serial.print((int) (100 * Lab->a), DEC);
  Serial.print(", b: ");
  Serial.println((int) (100 * Lab->b), DEC);
}

void cs_print_LMS(cs_LMS_t *LMS)
{
  Serial.print("L: ");
  Serial.print((int) (100 * LMS->L), DEC);
  Serial.print(", M: ");
  Serial.print((int) (100 * LMS->M), DEC);
  Serial.print(", S: ");
  Serial.println((int) (100 * LMS->S), DEC);
}

//cs_rgb_to_RGB(cs_rgb_t *rgb, cs_RGB_t *RGB)
