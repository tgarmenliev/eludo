#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <FastLED.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Keypad.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define LED_PIN     9
#define NUM_LEDS    52
#define BRIGHTNESS  24
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

#define LED_PIN_HOME     10
#define NUM_LEDS_HOME    32

#define LED_PIN_FINAL     11
#define NUM_LEDS_FINAL    8

#define UPDATES_PER_SECOND 100

#define BOARD_SIZE 52
#define NUM_PLAYERS 4
#define NUM_PAWNS 4

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


const byte ROWS = 2; //four rows
const byte COLS = 2; //three columns
char keys[ROWS][COLS] = {
  {'Y','B'},
  {'R','G'},
};
byte rowPins[ROWS] = {5, 4,}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {8, 7,}; //connect to the column pinouts of the keypad


char holdKey;
unsigned long t_hold;


Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

CRGB leds[2][NUM_LEDS];

CRGB leds_home[NUM_LEDS_HOME];

CRGB leds_final[NUM_LEDS_FINAL];


CRGB Yellow1 = CRGB(255,105,0);



typedef enum {RED, GREEN, BLUE, YELLOW} Color;
typedef enum {START, IN_PLAY, FINISH} Status;

typedef struct {
    Color color;
    Status status;
    int position;
} Pawn;

typedef struct {
    Pawn pawns[NUM_PAWNS];
    int num_pawns_home;
    int num_pawns_finished;
} Player;

Player players[NUM_PLAYERS];


void setup() {
  // put your setup code here, to run once:
  delay( 1000 ); // power-up safety delay

  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();

  // Clear the buffer
  display.clearDisplay();

  //testdrawchar();
  
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds[0], NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
  
  FastLED.addLeds<LED_TYPE, LED_PIN_HOME, COLOR_ORDER>(leds[1], NUM_LEDS_HOME).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
  
  //FastLED.addLeds<LED_TYPE, LED_PIN_FINAL, COLOR_ORDER>(leds[2], NUM_LEDS_FINAL).setCorrection( TypicalLEDStrip );
  //FastLED.setBrightness(  BRIGHTNESS );

  randomSeed(analogRead(0));

  init_players();

  turn_on_display();

  turn_on_players_homes();

  turn_on_players_space();

}

void loop() {
  // put your main code here, to run repeatedly:
  //turn_on_players_space();
  play();
}


void play() {
  delay(100);

  int current_player = 0;
  int dice_roll = 0;

  // Test only
  /*players[0].pawns[0].position = 3;
  players[1].pawns[0].position = 9;
  players[2].pawns[0].position = 15;
  players[3].pawns[0].position = 21;

  players[0].pawns[0].status = (Status)IN_PLAY;
  players[1].pawns[0].status = (Status)IN_PLAY;
  players[2].pawns[0].status = (Status)IN_PLAY;
  players[3].pawns[0].status = (Status)IN_PLAY;

  players[0].pawns[1].position = 5;
  players[1].pawns[1].position = 12;
  players[2].pawns[1].position = 18;
  players[3].pawns[1].position = 24;

  players[0].pawns[1].status = (Status)IN_PLAY;
  players[1].pawns[1].status = (Status)IN_PLAY;
  players[2].pawns[1].status = (Status)IN_PLAY;
  players[3].pawns[1].status = (Status)IN_PLAY;

  players[0].num_pawns_home = 2;
  players[1].num_pawns_home = 2;
  players[2].num_pawns_home = 2;
  players[3].num_pawns_home = 2;*/

  

  while(!is_game_over())
  {

    draw_dice_with_arrow(current_player, -1);

    while(keypad.getKey() != get_key_player(current_player)) {
      //for(int i = 0; i < 3; i++) { 
      blink_players_space(current_player);
    }

    dice_roll = roll_dice();

    draw_dice_with_arrow(current_player, dice_roll);

    // get num of pawns in play
    int num_pawns_in_play = 0;
    for (int i = 0; i < NUM_PAWNS; i++) {
      if (players[current_player].pawns[i].status == IN_PLAY) {
        num_pawns_in_play++;
      }
    }

    Serial.println(num_pawns_in_play);

    // check for 6 or continue
    if (num_pawns_in_play == 0 && dice_roll != 6)
    {
      delay(300);

      for(int i = 0; i < 3; i++)
        blink_players_space(current_player);
       
      current_player = (current_player + 1) % NUM_PLAYERS;
      continue;
    }

    int pawn_choice = -1;
    int curr_pawn_index = -1;

    unsigned long lastPressTime = millis();
    const unsigned long debounceDelay = 3000;

    if(num_pawns_in_play > 1 || dice_roll == 6 || players[current_player].num_pawns_finished != 0)
    {
    
      do {
        int next_pawn_index = get_next_pawn(curr_pawn_index, current_player, dice_roll);
        
        if(next_pawn_index == -1)
          break;
        
        blink_pawn(next_pawn_index, current_player);
  
        while(keypad.getKey() != get_key_player(current_player))
        {
          if((millis() - lastPressTime) > debounceDelay)
          {
            pawn_choice = next_pawn_index;
            break;
          }
          blink_pawn(next_pawn_index, current_player);
        }

        lastPressTime = millis();
  
        curr_pawn_index = next_pawn_index;
        
      } while (pawn_choice == -1);        //pawn_choice <= 0 || pawn_choice >= NUM_PAWNS || players[current_player].pawns[pawn_choice].status != IN_PLAY);

    }
    else {
      pawn_choice = 0;
      delay(700);
    }

    move_pawn(current_player, pawn_choice, dice_roll);

    if(dice_roll != 6)
      current_player = (current_player + 1) % 4;
    
  }
  //end
}





int roll_dice() {
    return random(1, 7);
}

void init_players() {
  for (int i = 0; i < NUM_PLAYERS; i++) {
      players[i].num_pawns_home = NUM_PAWNS;
      players[i].num_pawns_finished = 0;
      for (int j = 0; j < NUM_PAWNS; j++) {
          players[i].pawns[j].color = (Color)i;
          players[i].pawns[j].status = START;
          players[i].pawns[j].position = -1;
      }
  }
}

void turn_on_display() {
  display.clearDisplay();
  display.setTextSize(4);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.setRotation(2);
  display.cp437(true);
  
  display.write(69);
  display.write(76);
  display.write(85);
  display.write(68);
  display.write(79);
  display.display();
  
  
  delay(1000);
}

void turn_on_players_homes() {
  CRGB color_led = CRGB::Red;
  //Color color_home = (Color)0;
  for(int i = 0; i < 32; i++) {
    leds[1][i] = color_led;
    FastLED.show();
    if(i == 7)
      color_led = CRGB::Green;//green
    else if(i == 15)
      color_led = CRGB::Blue;//blue
    else if(i == 23)
      color_led = Yellow1;//yellow
    delay(200);
  }
  delay(1000);
}

void turn_on_players_space() {
  CRGB color_led = CRGB::Red;
    for(int i = 0; i < 52; i++)
    {
      leds[0][i] = color_led;
      FastLED.show();
      if(i == 12)
        color_led = CRGB::Green;
      else if(i == 25)
        color_led = CRGB::Blue;
      else if(i == 38)
        color_led = Yellow1;
      delay(200);
    }
    delay(3000);

    for(int i = 0; i < 52; i++)
    {
      leds[0][i] = CRGB::Black;
    }
    FastLED.show();
    delay(1000);
}

int is_game_over() {
  for (int i = 0; i < NUM_PLAYERS; i++) {
        if (players[i].num_pawns_finished == NUM_PAWNS) {
            return 1;
        }
    }
    return 0;
}

void draw_dice_with_arrow(int current_player_number, int dice_number)
{
  display.clearDisplay();

  display.setTextSize(4);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);

  if(current_player_number == 0) {// red 2
    display.setRotation(2);
    display.write(92);
    display.write(47);
  } else if(current_player_number == 1) {// green 1
    // nadqsno
    
    display.setRotation(1);
    if(dice_number != -1)
    {
      display.setTextSize(2);
      display.write(92);
      display.write(47);
    }
    else
    {
      display.write(124);
      display.write(124);
    }
     
  } else if(current_player_number == 2) {// blue 0
    // nagore
    display.setRotation(0);
    display.write(92);
    display.write(47);
  } else { // 3
    display.setRotation(3);
    if(dice_number != -1)
    {
      display.setTextSize(2);
      display.write(92);
      display.write(47);
    }
    else
    {
      display.write(124);
      display.write(124);
    }
  }

  display.write(32);
  
  if(dice_number != -1) {
    display.setTextSize(4);
    display.write(32);
    display.write(dice_number + 48);
    display.write(32);
  }
  else {
    if(current_player_number == 0) {// red
      // nadolu
      display.write(92);
      display.write(47);
    } else if(current_player_number == 1) {// green
      // nadqsno
      display.setTextSize(2);
      display.write(92);
      display.write(47);
      
    } else if(current_player_number == 2) {// blue
      // nagore
      display.write(92);
      display.write(47);
    } else {
      // nalqvo
      display.setTextSize(2);
      display.write(92);
      display.write(47);
    }
  }
  
  display.display();
  delay(100);
}

void blink_players_space(int player) {
  
  CRGB crgb_color;
  if((Color)player == (Color)RED)
    crgb_color = CRGB::Red;
  else if((Color)player == (Color)GREEN)
    crgb_color = CRGB::Green;
  else if((Color)player == (Color)BLUE)
    crgb_color = CRGB::Blue;
  else
    crgb_color = Yellow1;

  for(int i = 0; i < 4; i++)
  {
    int curr_pos = players[player].pawns[i].position;
    if(curr_pos == -1 && players[player].pawns[i].status == (Status)START)
    {
      // blink pawn from home
      leds[1][(player * 8) + (i * 2)] = CRGB::Black;
      leds[1][(player * 8) + (i * 2) + 1] = CRGB::Black;
    }
    else
      leds[0][curr_pos] = CRGB::Black;
  }

  FastLED.show();
  delay(80);

  for(int i = 0; i < 4; i++)
  {
    int curr_pos = players[player].pawns[i].position;
    if(curr_pos == -1 && players[player].pawns[i].status == (Status)START)
    {
      // blink pawn from home
      leds[1][(player * 8) + (i * 2)] = crgb_color;
      leds[1][(player * 8) + (i * 2) + 1] = crgb_color;
    }
    else
      leds[0][curr_pos] = crgb_color;
  }

  FastLED.show();
  delay(80);
}

char get_key_player(int current_player) {
  if(current_player == 0)
    return 'R';
  else if(current_player == 1)
    return 'G';
  else if(current_player == 2)
    return 'B';
  else
    return 'Y';
}

int get_next_pawn(int prev_pawn_index, int player_index, int dice_num) {
  int curr = prev_pawn_index;
  
  do
  {
    curr = (curr + 1) % 4;

    Status pawn_status = players[player_index].pawns[curr].status;

    if(pawn_status == (Status)IN_PLAY)
      return curr;

    if(pawn_status == (Status)START && dice_num == 6)
      return curr;
    
    if(pawn_status == (Status)FINISH)
      continue;
    
  } while(curr != prev_pawn_index);
  return -1;
}

void blink_pawn(int pawn_index, int player_index)
{
  int pos = players[player_index].pawns[pawn_index].position;
  
  CRGB crgb_color;
  if((Color)player_index == (Color)RED)
    crgb_color = CRGB::Red;
  else if((Color)player_index == (Color)GREEN)
    crgb_color = CRGB::Green;
  else if((Color)player_index == (Color)BLUE)
    crgb_color = CRGB::Blue;
  else
    crgb_color = Yellow1;

  if(pos != -1)
    leds[0][pos] = CRGB::Black;
  else {
    leds[1][(player_index * 8) + (pawn_index * 2)] = CRGB::Black;
    leds[1][(player_index * 8) + (pawn_index * 2) + 1] = CRGB::Black;
  }
  
  FastLED.show();
  delay(80);

  if(pos != -1)
    leds[0][pos] = crgb_color;
  else {
    leds[1][(player_index * 8) + (pawn_index * 2)] = crgb_color;
    leds[1][(player_index * 8) + (pawn_index * 2) + 1] = crgb_color;
  }
  
  FastLED.show();
  delay(80);
}

int get_start_index(int current_player) {
  if(current_player == 0) // red
    return 13;
  else if(current_player == 1) // green
    return 26;
  else if(current_player == 2) // blue
    return 39;
  else // yellow
    return 0;
}

int get_final_index(int current_player) {
  if(current_player == 0) // red
    return 12;
  else if(current_player == 1) // green
    return 25;
  else if(current_player == 2) // blue
    return 38;
  else // yellow
    return 51;
}

void move_pawn(int current_player, int pawn_index, int spaces) {
  Pawn pawn = players[current_player].pawns[pawn_index];

  if(pawn.status == (Status)START)
  {
    players[current_player].num_pawns_home--;
    players[current_player].pawns[pawn_index].status = (Status)IN_PLAY;
    players[current_player].pawns[pawn_index].position = get_start_index(current_player);
    Serial.println("Blink_from_home");
    blink_moving_from_home(current_player, pawn_index);
    return;
  }

  int current_pos = pawn.position, new_pos = current_pos + spaces;
  int other_player = 0, other_pawn = 0, num_pawns_on_new_pos = 0;

  if(current_pos <= get_final_index(current_player) && new_pos > get_final_index(current_player)) { //final
    players[current_player].num_pawns_finished++;
    players[current_player].pawns[pawn_index].status = (Status)FINISH;
    blink_moving_to_final(current_pos);
    players[current_player].pawns[pawn_index].position = -1;
    return;
  }

  for (int i = 0; i < 4; i++) {
        if (i != current_player) {  // skip current player
            for (int j = 0; j < NUM_PAWNS; j++) {
                Pawn p = players[i].pawns[j];
                if (p.status == (Status)IN_PLAY && p.position == new_pos) {
                    num_pawns_on_new_pos++;
                    other_player = i;
                    other_pawn = j;
                }
            }
        }
    }

  if(num_pawns_on_new_pos > 0)
  {
    bool flag = 0;
    CRGB color_there;
    if(new_pos != 0)
    {
      if(leds[0][(new_pos - 1) % 52]) {
        color_there = leds[0][(new_pos - 1) % 52];
        flag = 1;
      }
    }
    else
    {
      if(leds[0][51]) {
        color_there = leds[0][(new_pos - 1) % 52];
        flag = 1;
      }
    }

    
    blink_moving(current_player, current_pos, spaces - 1);
    
    blink_push(current_player, new_pos, pawn_index, other_player, other_pawn);
    if(flag) {
      if(new_pos == 0)
        leds[0][51] = color_there;
      else
        leds[0][(new_pos - 1) % 52] = color_there;
      FastLED.show();
      delay(50);
    }
    
    players[other_player].pawns[other_pawn].position = -1;
    players[other_player].pawns[other_pawn].status = (Status)START;
    players[other_player].num_pawns_home++;
    players[current_player].pawns[pawn_index].position = new_pos % BOARD_SIZE;
    return;
  }

  players[current_player].pawns[pawn_index].position = new_pos % BOARD_SIZE;
  blink_moving(current_player, current_pos, spaces);
  
}

void blink_moving_from_home(int player_index, int pawn_index) {
  CRGB crgb_color;
  if((Color)player_index == (Color)RED)
    crgb_color = CRGB::Red;
  else if((Color)player_index == (Color)GREEN)
    crgb_color = CRGB::Green;
  else if((Color)player_index == (Color)BLUE)
    crgb_color = CRGB::Blue;
  else
    crgb_color = Yellow1;
  
  leds[1][(player_index * 8) + (pawn_index * 2)] = CRGB::Black;
  leds[1][(player_index * 8) + (pawn_index * 2) + 1] = CRGB::Black;

  FastLED.show();
  delay(100);

  leds[0][players[player_index].pawns[pawn_index].position] = crgb_color;
  FastLED.show();
  delay(100);
}

void blink_moving_to_final(int position) {
  leds[0][position] = CRGB::Black;
  FastLED.show();
  delay(100);
}

void blink_moving(int player_index, int prev_pos, int spaces) {
  CRGB crgb_color;
  if((Color)player_index == (Color)RED)
    crgb_color = CRGB::Red;
  else if((Color)player_index == (Color)GREEN)
    crgb_color = CRGB::Green;
  else if((Color)player_index == (Color)BLUE)
    crgb_color = CRGB::Blue;
  else
    crgb_color = Yellow1;

  CRGB on_prev_pos = CRGB::Black;
  bool flag_prev = 0;
  for(int curr = prev_pos, i = 0; i <= spaces; i++, curr = (curr + 1) % BOARD_SIZE)
  {
    if(flag_prev == 1) {
      if(curr == 0 && curr != (prev_pos + 1))
        leds[0][51] = on_prev_pos;
      else
        leds[0][curr - 1] = on_prev_pos;
      flag_prev = 0;
    }
    else {
      if(curr != prev_pos)
      {
        if(curr == 0)
          leds[0][51] = CRGB::Black;
        else
          leds[0][curr - 1] = CRGB::Black;
      }
    }
    
    FastLED.show();
    delay(90);

    if(leds[0][(curr) % BOARD_SIZE] && curr != prev_pos) {
      on_prev_pos = leds[0][(curr) % BOARD_SIZE];
      flag_prev = 1;
    }
    leds[0][(curr) % BOARD_SIZE] = crgb_color;

    FastLED.show();
    delay(100);
  }
}

void blink_push(int player_index, int new_pos, int pawn_index, int other_player, int other_pawn) {
  CRGB crgb_color;
  if((Color)player_index == (Color)RED)
    crgb_color = CRGB::Red;
  else if((Color)player_index == (Color)GREEN)
    crgb_color = CRGB::Green;
  else if((Color)player_index == (Color)BLUE)
    crgb_color = CRGB::Blue;
  else
    crgb_color = Yellow1;

  CRGB second_crgb_color;
  if((Color)other_player == (Color)RED)
    second_crgb_color = CRGB::Red;
  else if((Color)other_player == (Color)GREEN)
    second_crgb_color = CRGB::Green;
  else if((Color)other_player == (Color)BLUE)
    second_crgb_color = CRGB::Blue;
  else
    second_crgb_color = Yellow1;

  leds[0][new_pos] = CRGB::Black;

  FastLED.show();
  delay(70);

  leds[1][(other_player * 8) + (other_pawn * 2)] = second_crgb_color;
  leds[1][(other_player * 8) + (other_pawn * 2) + 1] = second_crgb_color;
  
  FastLED.show();
  delay(70);

  if(new_pos != 0)
    leds[0][new_pos - 1] = CRGB::Black;
  else
    leds[0][51] = CRGB::Black;

  FastLED.show();
  delay(70);

  leds[0][new_pos] = crgb_color;

  FastLED.show();
  delay(70);
}
