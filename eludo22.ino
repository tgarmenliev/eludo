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



void setup()
{
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
}


void loop()
{
  play();  
}


void play()
{
  Player players[NUM_PLAYERS];
  for (int i = 0; i < NUM_PLAYERS; i++) {
      players[i].num_pawns_home = NUM_PAWNS;
      players[i].num_pawns_finished = 0;
      for (int j = 0; j < NUM_PAWNS; j++) {
          players[i].pawns[j].color = (Color)i;
          players[i].pawns[j].status = START;
          players[i].pawns[j].position = -1;
      }
  }

  set_up_home();
  delay(2000);

  blink_full();
  
  int current_player = 0;
  while (!is_game_over(players))
  {
    // ADD DELAYS WHEN BLINKING AND ANYTHING ELSE!!!
    
    // print arrow on the display, with the current player

    draw_dice_with_arrow(current_player, -1);
    
    // blink once the home of the current player and its pawns

    delay(100000);
    
    // wait for click of the button
    // and while waiting blink something
    while(keypad.getKey() != get_key_player(current_player)) {
      Serial.println(get_key_player(current_player));
      blink_players_space(players[current_player], (Color)current_player);
    }

    // roll the dice
    // print it on the display!
    
    //printf("Player %d's turn:\n", current_player + 1);
    
    int dice_roll = roll_dice();
    
    //printf("Rolled a %d!\n", dice_roll);

    draw_dice_with_arrow(current_player, dice_roll);

    // get num of pawns in play
    int num_pawns_in_play = 0;
    for (int i = 0; i < NUM_PAWNS; i++) {
      if (players[current_player].pawns[i].status == IN_PLAY) {
        num_pawns_in_play++;
      }
    }

    // check for 6 or continue
    if (num_pawns_in_play == 0 && dice_roll != 6)
    {
      //printf("No pawns in play. Waiting for 6 to start.\n");
      
      // delay(1000);
      
      // blink something
      for(int i = 0; i < 3; i++)
        blink_players_space(players[current_player], (Color)current_player);
      // print on the display next player
      
      current_player = (current_player + 1) % NUM_PLAYERS;
      continue;
    }
    
    int pawn_choice = -1;
    int curr_pawn_index = -1;
    
    do {
      // make function to check which pawns can be move
      // get the next pawn in list
      int next_pawn_index = get_next_pawn(curr_pawn_index, players[current_player], dice_roll);
      Pawn next_pawn = players[current_player].pawns[next_pawn_index];
      
      // blink
      blink_pawn(next_pawn);

      // then make with button to change smth
      // finally choose pawn to move

      while(1)
      {
        char key = keypad.getKey();
    
         if (key){
           holdKey = key;
           Serial.println(key);
         }
         else
          blink_pawn(next_pawn);
        
         if (keypad.getState() == HOLD) {
            // choose the current pawn if the button is in hold
            if ((millis() - t_hold) > 100 ) {
              pawn_choice = next_pawn_index;
              break;
            }
         }
         else {
          // change to next pawn
          blink_pawn(next_pawn);
          break;
         }
      }

      curr_pawn_index = next_pawn_index;
       
      //printf("Choose pawn to move (1-%d): ", NUM_PAWNS);
      //printf("%d", players[current_player].pawns[pawn_choice].status);
      //scanf("%d", &pawn_choice);
        //pawn_choice--;
    } while (pawn_choice == -1);//pawn_choice <= 0 || pawn_choice >= NUM_PAWNS || players[current_player].pawns[pawn_choice].status != IN_PLAY);

    // move pawn and blink
    // move pawn in system
    // then move pawn on the light board with blinking!
    // check for butane!!!
    // sega napravi premestvaneto da sveti :) !
    
    move_pawn(players, 4, &players[current_player], &players[current_player].pawns[pawn_choice], dice_roll);
   
    //printf("Pawn moved to position %d.\n", players[current_player].pawns[pawn_choice].position);

    // change current player to the next player
    current_player = (current_player + 1) % NUM_PLAYERS;
  }
  
  // blink with the winner!!!
  // end the game
  //printf("Player %d wins!\n", current_player + 1);
  
  while(1) {
    //blink with the winner, while the game is not turned off :)
    blink_winner(players[current_player]);
  }
}



int roll_dice() {
    return random(1, 7);
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

void move_pawn(Player *players, int num_players, Player *current_player, Pawn *pawn, int spaces) {
    int current_position = pawn->position;
    int new_position = current_position + spaces;
    int num_pawns_on_new_position = 0;
    Player *other_player = NULL;
    Pawn *other_pawn = NULL;

    // Check if the pawn lands on a space with other pawns
    for (int i = 0; i < num_players; i++) {
        if (&players[i] != current_player) {  // skip current player
            for (int j = 0; j < NUM_PAWNS; j++) {
                Pawn *p = &players[i].pawns[j];
                if (p->status == IN_PLAY && p->position == new_position) {
                    num_pawns_on_new_position++;
                    other_player = &players[i];
                    other_pawn = p;
                }
            }
        }
    }

    int special_case = 0;

    // If there is a collision, move the other pawn back to its home
    if (num_pawns_on_new_position == 1) {
        //printf("Collision with player %d's pawn!\n", other_player - players);
        other_pawn->status = START;
        other_pawn->position = -1;
        other_player->num_pawns_home++;
        special_case = 1;
    }

    int prev_pos = pawn->position;

    // Move the current pawn
    if (new_position >= BOARD_SIZE) { // opravi!!!
        pawn->status = FINISH;
        current_player->num_pawns_finished++;
        special_case = 2;
    } else {
        pawn->position = new_position;
    }

    blink_moving(pawn, prev_pos, spaces, special_case, other_player);
}

int is_game_over(Player players[NUM_PLAYERS]) {
    for (int i = 0; i < NUM_PLAYERS; i++) {
        if (players[i].num_pawns_finished == NUM_PAWNS) {
            return 1;
        }
    }
    return 0;
}


int get_position_on_the_board(int position, Color color)
{
  int res = position;
  for(int i = 0; i < (int)color; i++)
    res += 13;
  return res % BOARD_SIZE;
}

int get_next_pawn(int prev_pawn, Player player, int dice_num) {
  for(int i = (prev_pawn == 3 ? 0 : prev_pawn + 1); i < 4; i++)
  {
    Pawn curr = player.pawns[i];
    if(curr.status == (Status)START && dice_num == 6)
      return i;
    else if (curr.status == (Status)FINISH)
      continue;
    else if(curr.position + dice_num >= BOARD_SIZE)
      return -1;
    return i; 
  }
}



void set_up_home() {
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
}

void blink_full() {
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
    delay(100000);
}

void blink_players_space(Player player, Color color) {
  for(int i = 0; i < 4; i++) {
    if(player.num_pawns_home > 0)
    {
      for(int j = 0; j < (player.num_pawns_home * 2); j++)
        leds[1][((int)color * 8) + j] = CRGB::Black;
    }
    leds[0][player.pawns[i].position] = CRGB::Black;
  }
  FastLED.show();
  delay(100);
  
  CRGB crgb_color;
  switch(color) {
    case((Color)RED):
      crgb_color = CRGB::Red;
    case((Color)GREEN):
      crgb_color = CRGB::Green;
    case((Color)BLUE):
      crgb_color = CRGB::Blue;
    case((Color)YELLOW):
      crgb_color = Yellow1;
  }

  for(int i = 0; i < 4; i++) {
    if(player.num_pawns_home > 0)
    {
      for(int j = 0; j < (player.num_pawns_home * 2); j++)
        leds[1][((int)color * 8) + j] = crgb_color;
    }
    leds[0][player.pawns[i].position] = crgb_color;
  }
  FastLED.show();
  delay(100);
}

void blink_pawn(Pawn pawn)
{
  int pos = get_position_on_the_board(pawn.position, pawn.color);
  
  CRGB crgb_color;
  switch(pawn.color) {
    case((Color)RED):
      crgb_color = CRGB::Red;
    case((Color)GREEN):
      crgb_color = CRGB::Green;
    case((Color)BLUE):
      crgb_color = CRGB::Blue;
    case((Color)YELLOW):
      crgb_color = Yellow1;
  }
    
  leds[0][pos] = crgb_color;
  FastLED.show();

  delay(70);

  leds[0][pos] = CRGB::Black;

  FastLED.show();
}

void blink_moving(Pawn* pawn, int prev_pos, int spaces, int special_case, Player* other_player)
{
  CRGB main_color;
  switch(pawn->color) {
    case((Color)RED):
      main_color = CRGB::Red;
    case((Color)GREEN):
      main_color = CRGB::Green;
    case((Color)BLUE):
      main_color = CRGB::Blue;
    case((Color)YELLOW):
      main_color = Yellow1;
  }
  
  int old_pos = get_position_on_the_board(prev_pos, pawn->color), new_pos = get_position_on_the_board(pawn->position, pawn->color);
  switch(special_case) {
    case 0:
    {
      if(new_pos < old_pos)
      {
        for(int i = old_pos; i < BOARD_SIZE; i++)
          blink_next_led(i, main_color);
        for(int i = 0; i < new_pos; i++)
          blink_next_led(i, main_color);
      }
      else
      {
        for(int i = old_pos; i < new_pos; i++) // opravi!!!
          blink_next_led(i, main_color);
      }
      //leds[0][get_position_on_the_board(int position, Color color)] = crgb_color;
    }
    case 1: // push another pawn
    {
      if(new_pos < old_pos)
      {
        for(int i = old_pos; i < BOARD_SIZE; i++)
          blink_next_led(i, main_color);
        for(int i = 0; i < (new_pos - 1); i++)
          blink_next_led(i, main_color);
      }
      else
      {
        for(int i = old_pos; i < (new_pos - 1); i++) // opravi!!!
          blink_next_led(i, main_color);
      }

      CRGB second_color;
      switch(other_player->pawns[0].color) {
        case((Color)RED):
          second_color = CRGB::Red;
        case((Color)GREEN):
          second_color = CRGB::Green;
        case((Color)BLUE):
          second_color = CRGB::Blue;
        case((Color)YELLOW):
          second_color = Yellow1;
      }

      leds[1][((int)other_player->pawns[0].color * 8) + (other_player->num_pawns_home * 2)] = second_color;
      FastLED.show();
      delay(100);
      
      leds[1][((int)other_player->pawns[0].color * 8) + (other_player->num_pawns_home * 2) + 1] = second_color;
      FastLED.show();
      delay(100);
      
      leds[0][new_pos] = main_color;
      FastLED.show();
      delay(100);
    }
    case 2: // curr pawn goes to finish
    {
      leds[0][old_pos] = CRGB::Black;
      FastLED.show();
      (delay(50));
    }
  }
}

void blink_next_led(int prev_position, CRGB color)
{
  CRGB color_black = CRGB::Black;
  if(leds[0][prev_position] == color_black || leds[0][prev_position] == color)
  {
    leds[0][prev_position] = CRGB::Black;
    FastLED.show();
    delay(100);
    
    if(prev_position + 1 < BOARD_SIZE)
    {
      leds[0][prev_position + 1] = color;
      FastLED.show();
      delay(100);
    }
  }
}

void blink_winner(Player player)
{
  Color color = player.pawns[0].color;
  for(int j = 0; j < 8; j++)
    leds[1][((int)color * 8) + j] = CRGB::Black;

  FastLED.show();
  delay(200);

  CRGB crgb_color;
  switch(color) {
    case((Color)RED):
      crgb_color = CRGB::Red;
    case((Color)GREEN):
      crgb_color = CRGB::Green;
    case((Color)BLUE):
      crgb_color = CRGB::Blue;
    case((Color)YELLOW):
      crgb_color = Yellow1;
  }

  for(int j = 0; j < (player.num_pawns_home * 2); j++)
    leds[1][((int)color * 8) + j] = crgb_color;

  FastLED.show();
  delay(200);
}



void draw_dice_with_arrow(int current_player_number, int dice_number)
{
  display.clearDisplay();

  display.setTextSize(4.5);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);

  if(current_player_number == 0) {// red
    display.write(92);
    display.write(47);
  } else if(current_player_number == 1) {// green
    // nadqsno
    display.write(62);
  } else if(current_player_number == 2) {// blue
    // nagore
    display.write(94);
  } else {
    display.write(60);
  }

  display.write(32);
  
  if(dice_number != -1) {
    display.write(dice_number + 48);
    display.write(32);
  }

  if(current_player_number == 0) {// red
    // nadolu
    display.write(92);
    display.write(47);
  } else if(current_player_number == 1) {// green
    // nadqsno
    display.write(62);
  } else if(current_player_number == 2) {// blue
    // nagore
    display.write(94);
  } else {
    // nalqvo
    display.write(60);
  }
  
  display.display();
  delay(100);
}
