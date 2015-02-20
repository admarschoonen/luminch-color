//  Luminch One - Copyright 2012 by Francisco Castro <http://fran.cc>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#include <stdio.h>
#include "configuration.h"
#include "captouch.h"
#include "distance_sensor.h"
#include "color_space.h"

#define LED_R_PIN 5 // 980 Hz
#define LED_G_PIN 6 // 980 Hz
#define LED_B_PIN 3 // 490 Hz

#if (INPUT_SPACE == INPUT_SPACE_RGBW)
#define RED 0
#define GREEN 1
#define BLUE 2
#define WHITE 3

#define RED_MASK (1 << RED)
#define GREEN_MASK (1 << GREEN)
#define BLUE_MASK (1 << BLUE)
#define WHITE_MASK (1 << WHITE)
#define YELLOW_MASK (RED_MASK | GREEN_MASK)
#define CYAN_MASK (GREEN_MASK | BLUE_MASK)
#define MAGENTA_MASK (RED_MASK | BLUE_MASK)
#endif

#if (INPUT_SPACE == INPUT_SPACE_LAB)
#define BLUE 0
#define RED 1
#define YELLOW 2
#define GREEN 3

#define BLUE_MASK (1 << BLUE)
#define RED_MASK (1 << RED)
#define YELLOW_MASK (1 << YELLOW)
#define GREEN_MASK (1 << GREEN)
#define WHITE_MASK (BLUE_MASK | RED_MASK | YELLOW_MASK | GREEN_MASK)
#endif

#define LED_PIN(x) (x == RED ? LED_R_PIN : (x == GREEN ? LED_G_PIN : LED_B_PIN))

#define LIMIT(x, l, u) (x < l ? l : (x > u ? u : x))

#define DS_ON_OFF_MAX_LENGTH 60
//#define DS_ON_OFF_MAX_LENGTH 6

#define SENSE_THRESHOLD 150
#define TRACK_THRESHOLD 230
#define DEBOUNCE_CYCLES 30
#define START_TRACKING_CYCLES 180
#define END_TRACKING_CYCLES 210
#define HAND_MINIMUM_CHANGE 15

#define power_off 0
#define power_off_to_on 1
#define power_on 2
#define power_on_to_off 3

int adc_input = 0;
int pwm_output[3] = {0, 0, 0};
int stored_bright[3] = {0xFF, 0xFF, 0xFF};
int target_bright[3] = {0, 0, 0};
int hand_tracked_bright = 0;

boolean lamp_lighted = false;
boolean hand_tracking = false;

unsigned char hand_cycles = 0;
unsigned char debounce_cycles = 0;

int sample_1 = 0;
int sample_2 = 0;
int sample_3 = 0;
int sample_4 = 0;

void led_init()
{
  int n;

  for (n = RED; n < 3; n++)
    analogWrite(LED_PIN(n), 0);
}

void setup()
{
  led_init();
  captouch_init();
  Serial.begin(115200);
}

void serial_write(char *s)
{
  unsigned int n, N;
  
  N = strlen(s);
  
  for (n = 0; n < N; n++)
    Serial.print(s[n]);
}

float add(float x, float y, float limit)
{
  if ((x + y < limit) && (x + y > x))
    return x + y;
  else
    return limit;
}

float sub(float x, float y, float limit)
{
  if ((x - y > limit) && (x - y < x))
    return x - y;
  else
    return limit;
}

/* Calculate t2 - t1 (where usually t1 is smaller than t2). 
 * Overflow safe for input arguments but not for output argument. */
signed long delta_t(unsigned long t1, unsigned long t2)
{
  if (t2 < t1)
    return (t1 + ((1 << 32) - t2));
  else
    return t2 - t1;
}

void process_captouch_keys_move_to(cs_Lab_t *Lab_target, cs_Lab_t *Lab)
{
  float a, b;
  
  a = Lab_target->a;
  b = Lab_target->b;
  
  if (Lab->a < a - CS_LAB_STEP_SIZE_AB) {
    Lab->a += CS_LAB_STEP_SIZE_AB;
    //Serial.print("inc a; ");
  } else if (Lab->a > a + CS_LAB_STEP_SIZE_AB) {
    Lab->a -= CS_LAB_STEP_SIZE_AB;
    //Serial.print("dec a; ");
  } else {
    Lab->a = a;
    //Serial.print("no  a; ");
  }
  
  if (Lab->b < b - CS_LAB_STEP_SIZE_AB) {
    Lab->b += CS_LAB_STEP_SIZE_AB;
    //Serial.print("inc b; ");
  } else if (Lab->b > b + CS_LAB_STEP_SIZE_AB) {
    Lab->b -= CS_LAB_STEP_SIZE_AB;
    //Serial.print("dec b; ");
  } else {
    Lab->b = b;
    //Serial.print("no  b; ");
  }
  
  /*Serial.print("target a: ");
  Serial.print(a, DEC);
  Serial.print(", current a: ");
  Serial.print(Lab->a);
  Serial.print("; target b: ");
  Serial.print(b, DEC);
  Serial.print(", current b: ");
  Serial.println(Lab->b);*/
}

#if (INPUT_SPACE == INPUT_SPACE_LAB)
char process_captouch_keys_Lab(cs_Lab_t *Lab, cs_Lab_t *Lab_target, boolean *modified)
{
  char current_keys_mask = 0;

  if (captouch.states[BLUE] >= cap_state_pressed)
    current_keys_mask |= BLUE_MASK;
  if (captouch.states[RED] >= cap_state_pressed)
    current_keys_mask |= RED_MASK;
  if (captouch.states[YELLOW] >= cap_state_pressed)
    current_keys_mask |= YELLOW_MASK;
  if (captouch.states[GREEN] >= cap_state_pressed)
    current_keys_mask |= GREEN_MASK;

  if ((captouch.prev_keys_mask == WHITE_MASK) &&
      (current_keys_mask != WHITE_MASK) &&
      (delta_t(captouch.prev_keys_time, millis()) < 
      CAP_TIME_WAIT_AFTER_KEY_RELEASE))
    return current_keys_mask;
  
  if (current_keys_mask != 0) {
    /* key is pressed -> set current color as target to prevent strange issues when
    L value has decreased and target is now out of gamut */
    Lab_target->a = Lab->a;
    Lab_target->b = Lab->b;
  }
  
  if (current_keys_mask == WHITE_MASK) {
    /* all colors are pressed --> move towards white */
    if (Lab->a < CS_LAB_WHITE_POINT_A - CS_LAB_STEP_SIZE_AB)
      Lab->a += CS_LAB_STEP_SIZE_AB;
    else if (Lab->a > CS_LAB_WHITE_POINT_A + CS_LAB_STEP_SIZE_AB)
      Lab->a -= CS_LAB_STEP_SIZE_AB;
    else
      Lab->a = CS_LAB_WHITE_POINT_A;
    
    if (Lab->b < CS_LAB_WHITE_POINT_B - CS_LAB_STEP_SIZE_AB)
      Lab->b += CS_LAB_STEP_SIZE_AB;
    else if (Lab->b > CS_LAB_WHITE_POINT_B + CS_LAB_STEP_SIZE)
      Lab->b -= CS_LAB_STEP_SIZE_AB;
    else
      Lab->b = CS_LAB_WHITE_POINT_B;
    //current_keys_mask = 0x0F;
    *modified = true;
  } else {
    if (current_keys_mask & BLUE_MASK) {
      /* blue is on the negative axis of b */
      Lab->b -= CS_LAB_STEP_SIZE_AB;
      *modified = true;
    }
    if (current_keys_mask & RED_MASK) {
      /* red is on the positive axis of a */
      Lab->a += CS_LAB_STEP_SIZE_AB;
      *modified = true;
    }
    if (current_keys_mask & YELLOW_MASK) {
      /* yellow is on the positive axis of b */
      Lab->b += CS_LAB_STEP_SIZE_AB;
      *modified = true;
    }
    if (current_keys_mask & GREEN_MASK) {
      /* green is on the negative axis of a */
      Lab->a -= CS_LAB_STEP_SIZE_AB;
      *modified = true;
    }
  }
  return current_keys_mask;
}
#endif

#if (INPUT_SPACE == INPUT_SPACE_RGBW)
char process_captouch_keys_RGBW(cs_Lab_t *Lab, cs_Lab_t *Lab_t, boolean *modified)
{
  char current_keys_mask = 0;
  cs_Lab_t Lab_target;

  if (captouch.states[RED] >= cap_state_pressed)
    current_keys_mask |= RED_MASK;
  if (captouch.states[GREEN] >= cap_state_pressed)
    current_keys_mask |= GREEN_MASK;
  if (captouch.states[BLUE] >= cap_state_pressed)
    current_keys_mask |= BLUE_MASK;
  if (captouch.states[WHITE] >= cap_state_pressed)
    current_keys_mask |= WHITE_MASK;
  
  /*Serial.print("current_keys_mask: 0x");
  Serial.print(current_keys_mask, HEX);*/
  
  if (current_keys_mask != 0) {
    /* key is pressed -> set current color as saved target to prevent strange issues when
    L value has decreased and target is now out of gamut */
    Lab_t->a = Lab->a;
    Lab_t->b = Lab->b;
  }
  
  Lab_target.L = Lab->L;
  if ((current_keys_mask & ~WHITE_MASK) == RED_MASK) {
//    Serial.print("; red");
    Lab_target.a = CS_LAB_RED_POINT_A;
    Lab_target.b = CS_LAB_RED_POINT_B;
    *modified = true;
  }
  if ((current_keys_mask & ~WHITE_MASK) == GREEN_MASK) {
//    Serial.print("; green");
    Lab_target.a = CS_LAB_GREEN_POINT_A;
    Lab_target.b = CS_LAB_GREEN_POINT_B;
    *modified = true;
  }
  if ((current_keys_mask & ~WHITE_MASK) == BLUE_MASK) {
//    Serial.print("; blue");
    Lab_target.a = CS_LAB_BLUE_POINT_A;
    Lab_target.b = CS_LAB_BLUE_POINT_B;
    *modified = true;
  }
  if ((current_keys_mask & ~WHITE_MASK) == YELLOW_MASK) {
//    Serial.print("; yellow");
    Lab_target.a = CS_LAB_YELLOW_POINT_A;
    Lab_target.b = CS_LAB_YELLOW_POINT_B;
    *modified = true;
  }
  if ((current_keys_mask & ~WHITE_MASK) == CYAN_MASK) {
//    Serial.print("; cyan");
    Lab_target.a = CS_LAB_CYAN_POINT_A;
    Lab_target.b = CS_LAB_CYAN_POINT_B;
    *modified = true;
  }
  if ((current_keys_mask & ~WHITE_MASK) == MAGENTA_MASK) {
//    Serial.print("; magenta");
    Lab_target.a = CS_LAB_MAGENTA_POINT_A;
    Lab_target.b = CS_LAB_MAGENTA_POINT_B;
    *modified = true;
  }
  if (current_keys_mask & WHITE_MASK) {
//    Serial.print("; white");
    if (*modified) {
      Lab_target.a = (Lab_target.a + CS_LAB_WHITE_POINT_A) / 2;
      Lab_target.b = (Lab_target.b + CS_LAB_WHITE_POINT_B) / 2;
    } else {
      Lab_target.a = CS_LAB_WHITE_POINT_A;
      Lab_target.b = CS_LAB_WHITE_POINT_B;
    }
    *modified = true;
  }
  if (*modified)
        process_captouch_keys_move_to(&Lab_target, Lab);

//  Serial.println("");
    
  return current_keys_mask;
}
#endif

char process_captouch_keys(cs_Lab_t *Lab, cs_Lab_t *Lab_target)
{
  char current_keys_mask = 0;
  cs_Lab_t Lab_new;
  boolean modified = false;
  
  Lab_new.L = Lab->L;
  Lab_new.a = Lab->a;
  Lab_new.b = Lab->b;
  
  #if (INPUT_SPACE == INPUT_SPACE_RGBW)
  current_keys_mask = process_captouch_keys_RGBW(&Lab_new, Lab_target, &modified);
  #endif
  #if (INPUT_SPACE == INPUT_SPACE_LAB)
  current_keys_mask = process_captouch_keys_Lab(&Lab_new, Lab_target, &modified);
  #endif
  
  captouch.prev_keys_mask = current_keys_mask;
  if (current_keys_mask) {
    captouch.prev_keys_time = millis();
  } else {
    /* prevent overflow issue when keys are not touched for ~ 50 days */
    captouch.prev_keys_time = millis() - CAP_TIME_WAIT_AFTER_KEY_RELEASE;
  }

  if (modified == false)
    return current_keys_mask;
    
  if (cs_Lab_inside_RGB(&Lab_new)) {
    Lab->L = Lab_new.L;
    Lab->a = Lab_new.a;
    Lab->b = Lab_new.b;
    //cs_print_Lab(Lab);
  } else {
    //Serial.println("");
    //Serial.println("Lab_new not inside RGB; searching for closest match");
    if (cs_Lab_find_closest_RGB_with_fixed_L(Lab, &Lab_new)) {
      //Serial.println("Match found:");
      Lab->L = Lab_new.L;
      Lab->a = Lab_new.a;
      Lab->b = Lab_new.b;
      //cs_print_Lab(Lab);
    } else {
      //Serial.println("No match found:");
      //cs_print_Lab(Lab);
    }
  }
  return current_keys_mask;
}

void process_ds_distance(cs_Lab_t *Lab, cs_Lab_t *Lab_target, char *power) {
  static int ds_hand_present_counter = 0;
  static cs_Lab_t Lab_stored = {CS_LAB_MAX_L, CS_LAB_WHITE_POINT_A, CS_LAB_WHITE_POINT_B};
  float intensity = 0;
  
  if (ds.distance <= DS_FARTHEST) {
    /* hand is present */
    if (++ds_hand_present_counter > DS_ON_OFF_MAX_LENGTH) {  
      /* hand is present for too long to do an on/off gesture -> adjust intensity */
      
      /* Initially, set Lab color to same as Lab. Lab color will be
       * modified later on if it is out of gamut */
      Lab->L = Lab_target->L;
      Lab->a = Lab_target->a;
      Lab->b = Lab_target->b;

      intensity = ((float) ds.distance - DS_CLOSEST) / (DS_FARTHEST - DS_CLOSEST);
      intensity = LIMIT(intensity, 0.0, 1.0);
      //Serial.println((int) (1000 * intensity), DEC);
      
      if (intensity >= ((float) CS_LAB_STEP_SIZE_AB) / ((float) (CS_LAB_MAX_L - CS_LAB_MIN_L))) {
        //if ((*power == power_off) || (*power == power_on_to_off))
          //Serial.println("intensity adjustment: power on");
        *power = power_off_to_on;
        Lab->L = (CS_LAB_MAX_L - CS_LAB_MIN_L) * intensity + CS_LAB_MIN_L;
        Lab_target->L = Lab->L;
        //Serial.print("L: ");
        //Serial.println(Lab->L, DEC);
      } else {
        if (*power != power_off)
          *power = power_on_to_off;
        Lab->L = CS_LAB_MIN_L;
        Lab_target->L = Lab->L;
        Lab_stored.L = Lab_target->L;
        Lab_stored.a = Lab_target->a;
        Lab_stored.b = Lab_target->b;
        /* ugly hack: if hand is removed sideways, received signal will look like hand is further away -->
         * ignore signal for a relatively long time after lamp is powered off via dimming to prevent
         * turning it back on */
        delay(1000);
        //Serial.println("hand too low -> power off");
      }
      if (!cs_Lab_inside_RGB(Lab)) {
        //Serial.println("intensity out of gamut");
        cs_Lab_move_towards_white_with_fixed_L(Lab, CS_LAB_STEP_SIZE_AB);
      }
    } else {
      /* hand is present, but not yet long enough to distinguish beteen on/of and intensity gestures */
      //Serial.print("hand present; counter: ");
      //Serial.println(ds_hand_present_counter, DEC);
    }
  } else {
    /* hand is not present */
    if (ds_hand_present_counter > 0) {
      /* hand was present */
      if (ds_hand_present_counter <= DS_ON_OFF_MAX_LENGTH) {
        /* hand was present for a short period -> on/off gesture */
        
        /* Initially, set Lab color to same as Lab. Lab color will be
         * modified later on if it is out of gamut */
        Lab->L = Lab_target->L;
        Lab->a = Lab_target->a;
        Lab->b = Lab_target->b;

        //*power_on = 1 - *power_on;
        switch (*power) {
        case power_off:
          *power = power_off_to_on;
          Lab_target->L = Lab_stored.L;
          break;
        case power_off_to_on:
          *power = power_on_to_off;
          Lab_target->L = 0;
          break;
        case power_on:
          *power = power_on_to_off;
          Lab_stored.L = Lab_target->L;
          Lab_stored.a = Lab_target->a;
          Lab_stored.b = Lab_target->b;
          Lab_target->L = 0;
          break;
        case power_on_to_off:
          *power = power_off_to_on;
          Lab_target->L = Lab_stored.L;
          break;
        }
        //Serial.print("hand removed -> power toggle: ");
        //Serial.println(*power, DEC);
        ds_hand_present_counter = 0;
      } else {
        //Serial.print("hand removed, no power toggle; counter: ");
        //Serial.println(ds_hand_present_counter, DEC);
        ds_hand_present_counter = 0;
        /* ugly hack: if hand was removed upwards, it could be at the edge and cause flickering
         * (rapid on/off gestures)  --> ignore signal for a relatively long time */
        delay(1000);
      }
    }
  }
}

float approach(float x1, float x2, float d)
{
  float ret;

  if (x1 < x2 - d) {
    ret = x1 + d;
  } else {
    if (x1 > x2 + d) {
      ret = x1 - d;
    } else {
      ret = x2;
    }
  }
  return ret;
}  

void led_write(cs_linear_t *cs_linear)
{
  analogWrite(LED_PIN(RED), (int) (255 * (cs_linear->r)));
  analogWrite(LED_PIN(GREEN), (int) (255 * (cs_linear->g)));
  analogWrite(LED_PIN(BLUE), (int) (255 * (cs_linear->b)));
}

void application()
{
  cs_RGB_t RGB;
  cs_XYZ_t XYZ;
  cs_linear_t cs_linear;
  static cs_linear_t cs_linear_prev = {0, 0, 0}; // all leds off
  static cs_Lab_t Lab = {CS_LAB_MAX_L, CS_LAB_WHITE_POINT_A, CS_LAB_WHITE_POINT_B};
  static cs_Lab_t Lab_target = {CS_LAB_MAX_L, CS_LAB_WHITE_POINT_A, CS_LAB_WHITE_POINT_B};
  static cs_Lab_t Lab_current = {CS_LAB_MIN_L, CS_LAB_WHITE_POINT_A, CS_LAB_WHITE_POINT_B};
//  static int power_on = 0;
//  static power_state_t power = power_off;
  static char power = power_off;
  static int t = millis();
  char tmp;
  
  captouch_get_readings();
  captouch_process_readings();
  //captouch_print_debug();
  
  ds_get_reading();
  ds_reading_to_distance();
  //ds_print_debug();
  
  if (power != power_off) {
    tmp = process_captouch_keys(&Lab, &Lab_target);
    if (tmp) {
      /* key is pressed --> color is selected --> force target equal to current color */
      Lab_target.L = Lab.L;
      Lab_target.a = Lab.a;
      Lab_target.b = Lab.b;
    }
  } else
    /* prevent overflow issue when keys are not touched for ~ 50 days */
    captouch.prev_keys_time = millis() - CAP_TIME_WAIT_AFTER_KEY_RELEASE;
  
  process_ds_distance(&Lab, &Lab_target, &power);
  Serial.println(power, DEC);;
  /*Serial.print("Lab current: ");
  cs_print_Lab(&Lab);
  Serial.print("Lab target:  ");
  cs_print_Lab(&Lab_target);*/
  
  if (power != power_off) {
    Lab_current.L = approach(Lab_current.L, Lab.L, CS_LAB_STEP_SIZE_L);
    Lab_current.a = approach(Lab_current.a, Lab.a, CS_LAB_STEP_SIZE_AB);
    Lab_current.b = approach(Lab_current.b, Lab.b, CS_LAB_STEP_SIZE_AB);
    
    if ((power == power_off_to_on) && (Lab_current.L == Lab.L) && 
        (Lab_current.a == Lab.a) && (Lab_current.b == Lab.b))
      power = power_on;
    if ((power == power_on_to_off) && (Lab_current.L <= CS_LAB_MIN_L + CS_LAB_STEP_SIZE_AB))
      power = power_off;
  }
  
  cs_Lab_to_XYZ(&Lab_current, &XYZ);
  
  //cs_white_balance_correction(&XYZ, &XYZ_wb_corrected, Lab.L, CS_LAB_STEP_SIZE_AB);

  cs_XYZ_to_RGB(&XYZ, &RGB);
  cs_RGB_to_linear(&RGB, &cs_linear);

  if (power != power_off) {
    //cs_print_Lab(&Lab_current);
    //cs_print_RGB(&RGB);
    if (cs_linear_prev.r != cs_linear.r) {
      analogWrite(LED_PIN(RED), (int) (255 * (cs_linear.r)));
      cs_linear_prev.r = cs_linear.r;
    }
    if (cs_linear_prev.g != cs_linear.g) {
      analogWrite(LED_PIN(GREEN), (int) (255 * (cs_linear.g)));
      cs_linear_prev.g = cs_linear.g;
    }
    if (cs_linear_prev.b != cs_linear.b) {
      analogWrite(LED_PIN(BLUE), (int) (255 * (cs_linear.b)));
      cs_linear_prev.b = cs_linear.b;
    }
  } else {
    analogWrite(LED_PIN(RED), 0);
    analogWrite(LED_PIN(GREEN), 0);
    analogWrite(LED_PIN(BLUE), 0);
  }
}

void test_electronics()
{
  static unsigned long t_start = 0;
  unsigned long t_now;
  int a;
  cs_linear_t linear;
  
  t_now = millis();
  
  a = (t_now - t_start) / 500;
  
  switch(a) {
  case 0:
    linear = {0.05, 0.0, 0.0};
    break;
  case 1:
    linear = {1.0, 0.0, 0.0};
    break;
  case 2:
    linear = {0.0, 0.05, 0.0};
    break;
  case 3:
    linear = {0.0, 1.0, 0.0};
    break;
  case 4:
    linear = {0.0, 0.0, 0.05};
    break;
  case 5:
    linear = {0.0, 0.0, 1.0};
    break;
  default:
    t_start = t_now;
  }
   
  led_write(&linear); 

  captouch_get_readings();
  captouch_process_readings();
  captouch_print_debug();

  ds_get_reading();
  ds_reading_to_distance();
  ds_print_debug();
}

void loop()
{
  #ifdef TEST_ELECTRONICS
  test_electronics();
  #else
  application();
  #endif
}
