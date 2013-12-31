#include <Adafruit_NeoPixel.h>
#include <Wire.h> // Enable this line if using Arduino Uno, Mega, etc.

#define PIN 6

const int max_pixels = 24;

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(max_pixels, PIN, NEO_GRB + NEO_KHZ800);

#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

Adafruit_7segment matrix = Adafruit_7segment();

typedef struct {
  void (*loop_fun)();
  void (*init_fun)();
  void (*menu_fun)();
} Game;


#define MENU_GAME 0
#define REDVBLUE_GAME 1
#define FILLIT_GAME 2
#define AIM_GAME 3
#define CALIBRATE 4

const int game_count = 5;
Game games[game_count] = {
  {menu,menu_init,0},
  {redvblue,redvblue_init,redvblue_menu},
  {fillit,fillit_init,fillit_menu},
  {aim,aim_init,aim_menu},
  {calibrate,calibrate_init,calibrate_menu}};
int current_game = REDVBLUE_GAME;

const uint16_t num_pads = 4;
uint16_t pads[4];
int pad_hit;
int fader;
uint32_t fade_to;

#define PAD_M 0
#define PAD_B 1
#define PAD_L 2
#define PAD_R 3
#define PAD_M_PIX 23
#define PAD_B_PIX 22
#define PAD_L_PIX 20
#define PAD_R_PIX 21
#define HIT_THRESHOLD 100
#define pad_pixel(p) (23-p)
#define COUNT_DOWN_TIME 400
#define SCORE_BLINKS 4
#define ROUNDS 4
#define LEVEL_BAR_PIXELS 20

int *points;
int points_1 = 0;
int points_2 = 0;
long color_1,color_2;
long *color;
long time = 0;
long turn;
int rounds;

void setup() {
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  matrix.begin(0x70);
  pad_hit = -1;
  set_game(MENU_GAME);
}

void loop() {
  (*games[current_game].loop_fun)();
}

//------- UTILS -------------
void clear_all() {
  matrix.clear();
  for(int i=0;i<max_pixels;i++) {
    strip.setPixelColor(i, 0);
  }
  matrix.writeDisplay();
  strip.show();
}

int pad_check() {
  read_pads();
  if (pad_hit >= 0) {
    set_pixel_brightness(pad_pixel(pad_hit),fader,*color);
    fader--;
    if (fader == 0) {
       strip.setPixelColor(pad_pixel(pad_hit), fade_to);
       strip.show();
       pad_hit = -1;
    }
  }
  else {
    if (pads[PAD_L]> HIT_THRESHOLD && pads[PAD_R] > HIT_THRESHOLD) {
      set_game(MENU_GAME);
      return 0;
    }
    for(uint16_t i=0;i<num_pads;i++) {
      if (pads[i] > HIT_THRESHOLD ) {
        pad_hit = i;fader = 255;
        return 1;
      }
    }
  }
  return 0;
}


int game_choice;

void set_game(int game) {
  if (game == MENU_GAME && current_game != MENU_GAME) game_choice = current_game;
  current_game = game;
  (*games[current_game].init_fun)();
}

//----------------------------------------------------------------------
// MENU
void name() {
  matrix.clear();
  if (games[game_choice].menu_fun == 0) {
    matrix.print(game_choice);
  }
  else {
    (*games[game_choice].menu_fun)();
  }
    matrix.writeDisplay();
 }

void menu() {
  if (pad_check()) {
    if (pad_hit == PAD_L || pad_hit == PAD_R) {
      game_choice+= (pad_hit == PAD_L) ? -1 : 1;
      if (game_choice >= game_count) {
        game_choice = 1;
      }
      else if (game_choice <= 0) {
        game_choice = game_count -1;
      }
      name();
    }
    else {
      set_game(game_choice);
    }
  }
}
void menu_init() {
  clear_all();
  color_1 = strip.Color(255,255,255);
  fade_to = strip.Color(255,128,0);
  color = &color_1;
  strip.setPixelColor(PAD_L_PIX, 255,128,0);
  strip.setPixelColor(PAD_R_PIX, 255,128,0);
  strip.show();
  name();
}

//----------------------------------------------------------------------
// RED VERSUS BLUE

void redvblue_init() {
  fade_to = 0;
  color_1 = strip.Color(127,0,0);
  color_2 = strip.Color(0,0,127);
  color = &color_1;
  points = &points_1;
  points_1 = 0;
  points_2 = 0;
  time = millis()+COUNT_DOWN_TIME;
  turn = LEVEL_BAR_PIXELS;
  rounds = ROUNDS;
  set_score();
  matrix.writeDisplay();
  score_flash();
}

void set_score() {
    matrix.writeDigitNum(0, (points_1 / 10), false);
    matrix.writeDigitNum(1, (points_1 % 10), false);
    matrix.writeDigitNum(3, (points_2 / 10), false);
    matrix.writeDigitNum(4, (points_2 % 10), false);
    matrix.writeDigitRaw(2, 2); // central colon
}

void set_pixel_brightness(int pixel,int brightness,uint32_t color) {
  uint8_t r,g,b;
  r = (uint8_t)(color >> 16),
  g = (uint8_t)(color >>  8),
  b = (uint8_t)color;
  r = (r * brightness) >> 8;
  g = (g * brightness) >> 8;
  b = (b * brightness) >> 8;
  
  strip.setPixelColor(pixel, r,g,b);
  strip.show();
}

void score_flash() {
  for(int i=0;i<4;i++) strip.setPixelColor(LEVEL_BAR_PIXELS+(3-i),rounds > i ? strip.Color(255,128,0) : 0);
  strip.show();
  for(int i=0;i<SCORE_BLINKS*2;i++) {
    if (i%2) {
      set_score();
    }
    else {
      matrix.clear();
    }
    matrix.writeDisplay();
    delay(500);
  }
  for(int i=0;i<4;i++) strip.setPixelColor(LEVEL_BAR_PIXELS+(3-i),0);
  strip.show();
}

void game_over() {
  uint32_t c1,c2 = 0;
  if (points_2 > points_1) {
    c1 = color_2;
  }
  else {
    c1 = color_1;
  }
  if (points_2 == points_1) {
    c2 = color_2;
  };
  for(long i=0;!pad_check();i++) {
     for(int p=0;p<4;p++) strip.setPixelColor(LEVEL_BAR_PIXELS+p,(i%2)? c1 : c2);
     strip.show();
     delay(500);
  }
  redvblue_init(); 
}

void turn_check() {
  long t = millis();
  if (t > time) {
    --turn; 
    for(int i = 19;i>=0;i--) {
      strip.setPixelColor(19-i,i<=turn ? *color : 0); 
    }
    strip.show();
    if (turn == -1) {
      turn = LEVEL_BAR_PIXELS;
      if (points == &points_1) {
        points = &points_2;
        color = &color_2;
      }
      else {
        points = &points_1;
        color = &color_1;
        rounds--;
        if (rounds == 0) {
          game_over();
        }               
      }
      score_flash();
    }
    time = t+COUNT_DOWN_TIME;
  }
  else {
    set_pixel_brightness(19-turn,(time-t)*255/COUNT_DOWN_TIME,*color);
  }
}

void redvblue() {
  turn_check();
  if (pad_check()) {
    if (pad_hit == PAD_M) {
      (*points) += 3;
    }
    else (*points)++;
    set_score();
    matrix.writeDisplay();
  }
}

void read_pads() {
  for(uint16_t i=0;i<num_pads;i++) pads[i] = analogRead(i);
}

void redvblue_menu() {
  matrix.writeDigitRaw(1, 0x50); // r
  matrix.writeDigitRaw(2, 2); // central colon
  matrix.writeDigitRaw(3, 0x7C); // b
}

//----------------------------------------------------------------------
// CALIBRATE
int m = 10;
int d= 1;
uint16_t counter = 0;
int current_pad;
void calibrate_init() {
  counter = 0;
}

void calibrate() {
  read_pads();
  for(uint16_t i=0;i<num_pads;i++) {
      if (pads[i] > HIT_THRESHOLD ) current_pad = i;
  }
  if (pads[PAD_M]> HIT_THRESHOLD && pads[PAD_R] > HIT_THRESHOLD) {
    set_game(MENU_GAME);
    return;
  }

  counter = pads[current_pad];
  matrix.print(counter);
  for(uint16_t i=0; i<strip.numPixels(); i++) {
      if (i<counter/m) {
         strip.setPixelColor(i, strip.Color(50,50,50));
      }
      else if (i== counter/m) { int v = (counter%m*5);
         strip.setPixelColor(i, strip.Color(v,v,v));}
         else {
           strip.setPixelColor(i,0);
         }
  }
  strip.show();
  matrix.writeDisplay();
  if (counter >= strip.numPixels() || counter == 0) d*=-1;
}

void calibrate_menu() {
  matrix.writeDigitRaw(1, 0x39); // C
  matrix.writeDigitRaw(3, 0x77); // A
  matrix.writeDigitRaw(4, 0x38); // L
}

//----------------------------------------------------------------------
// FILL_IT

#define STARTING_LEVEL_TIME 50

long level_time;
int level,level_minus;
long second;
void fillit_init() {
  points_1 = -1;
  fade_to = 0;
  points = &points_1;
  color_1 = strip.Color(127,127,127);
  color = &color_1;
  level_time = STARTING_LEVEL_TIME;
  level_minus = 10;
  level = 1;
  time = millis()+level_time*1000;
  second = 0;
  clear_all();
}

void fillit() {
  long t = millis();
  if (t > time) {
    while(!pad_check()) {
      t++;
      if (t%2) matrix.clear();
      else matrix.print(level);
      matrix.writeDisplay();

      delay(500);
    }
    fillit_init();
    return;
  }
  if (t > second) {
    long s = (time-t)/1000;
    matrix.print(s);
    matrix.writeDigitNum(0, level, false);
    matrix.writeDisplay();
    second = t + 1000;
  }
  if (pad_check()) {
    if (pad_hit == PAD_M) {
      (*points) += 2;
    }
    else (*points)++;
    for(int i = 19;i>=0;i--) {
      strip.setPixelColor(19-i,i<=*points ? *color : 0); 
    }
    strip.show();
    if (*points >= 19) {
      level++;
      level_time -= level_minus;
      if (level_minus > 1) level_minus--;
      second = 0;
      *points = 0;
      clear_all();
      for(int i=0;i<SCORE_BLINKS*2;i++) {
        if (i%2) {
          matrix.print(level_time);
          matrix.writeDigitNum(0, level, false);
          matrix.writeDisplay();
        }
        else {
          matrix.clear();
        }
        matrix.writeDisplay();
        delay(500);
      }
      time = millis()+level_time*1000;
    }
  }
}

void fillit_menu() {
  matrix.writeDigitRaw(0, 0x71); // F
  matrix.writeDigitRaw(1, 0x06); // I
  matrix.writeDigitRaw(3, 0x38); // L  
  matrix.writeDigitRaw(4, 0x38); // L
}
//----------------------------------------------------------------------
// AIM

void aim_score() {
  matrix.print(*points);
  matrix.writeDisplay();

}

int target;
uint32_t targetColor = strip.Color(255,255,255);
void aim_init() {
  fade_to = 0;
  color_1 = strip.Color(127,0,0);
  color_2 = strip.Color(0,0,127);
  color = &color_1;
  points = &points_1;
  points_1 = 0;
  points_2 = 0;
  pick_target();
  aim_score();
}

void pick_target() {
  int t;
  while((t = random(4)) == target);
  target = t;
  for(int i=0;i<4;i++) strip.setPixelColor(pad_pixel(i),target == i ? targetColor : 0);
  strip.show();
}

void aim() {
  if (pad_check()) {
    int c = 1;
    if (pad_hit == PAD_M) c++;
    if (pad_hit == target) {
      color = &color_2;
      (*points)+=c;
       pick_target();
    }
    else {
      color = &color_1;
      (*points)--;
    }
    aim_score();
  }
}

void aim_menu() {
  matrix.writeDigitRaw(0, 0x77); // A
  matrix.writeDigitRaw(1, 0x06); // I
  matrix.writeDigitRaw(3, 0x33); // M  
  matrix.writeDigitRaw(4, 0x27); // M

}


