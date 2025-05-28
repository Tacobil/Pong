
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// PINS //
const int speaker = A1;
const int paddle_control_pin = A0;
const int input_pin = 23;
const int output_pin = 22;

/* screen pins
GND - -
VCC - +
SCL - 23
SDA - 22
*/


// SETTINGS //
const uint8_t SCREEN_HEIGHT = 63, SCREEN_WIDTH = 127;
const uint8_t CENTER_X = SCREEN_WIDTH / 2, CENTER_Y = SCREEN_HEIGHT / 2;

const uint8_t SCORE_LIMIT = 3;

const uint8_t BALL_SPEED = 5;
const uint8_t PADDLE_SPEED = 5;

const uint8_t PADDLE_HEIGHT = 12;
const uint8_t MCU_X = 4;
const uint8_t PLAYER_X = SCREEN_WIDTH - MCU_X;

// GAME STATS
bool game_over, win;
bool playing = false;

bool update_needed;

unsigned long previous_time = 0;
unsigned long current_time = 0;
unsigned long delta_time = 0;

uint8_t player_score = 0, mcu_score = 0;

uint8_t ball_x = CENTER_X, ball_y = CENTER_Y;
uint8_t ball_dir_x = 1, ball_dir_y = 1;

int mcu_y = CENTER_Y, player_y = CENTER_Y;

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

void drawCourt() {
    // display.drawRect(0, 0, 128, 64, WHITE);
    display.drawFastHLine(0, 0, SCREEN_WIDTH, WHITE); // top line
    display.drawFastHLine(0, SCREEN_HEIGHT, SCREEN_WIDTH, WHITE); // bot line
    /*
    for (int y = 0; y < 64; y += 4) {
        display.drawFastVLine(CENTER_X, y, 2, WHITE);  // 2-pixel segment, then 2-pixel gap
    }
    */
}

void playerPaddleTone() {
    tone(speaker, 459, 96);
}

void mcuPaddleTone() {
    tone(speaker, 459, 96);
}

void wallTone() {
    tone(speaker, 226, 16);
}

void player_scoreTone() {
    tone(speaker, 200, 25);
    delay(50);
    tone(speaker, 250, 25);
    delay(25);
}

void mcu_scoreTone() {
    tone(speaker, 250, 25);
    delay(50);
    tone(speaker, 200, 25);
    delay(25);
}


void reset_pong() {
    drawCourt();
    player_score = 0;
    mcu_score = 0;

    game_over = false;

    ball_dir_x = 1;
    ball_dir_y = 1;

    current_time = millis();
}

void on_game_over() {
    playing = false;

    // Display text: win or lose
    const char* text = win ? "YOU WIN!!" : "YOU LOSE!";
    display.clearDisplay();
    display.setCursor(40, 28);
    display.print(text);
    display.display();

    // output signal
    digitalWrite(output_pin, LOW);

    delay(3000);
    digitalWrite(output_pin, HIGH); // stop output signal
    display.clearDisplay();
    display.display();
}


void game_update() {
    current_time = millis();
    delta_time = current_time - previous_time; // in milliseconds
    previous_time = current_time;

    uint8_t new_x = ball_x + ball_dir_x;
    uint8_t new_y = ball_y + ball_dir_y;

    // Check if we hit the vertical walls
    if(new_x <= 0 || new_x >= SCREEN_WIDTH) 
    {

        if (new_x < 64)  // Ball passed the player
        {
            player_scoreTone();
            player_score++;  // Increment player score when ball hits left side (player's side)
        }
        else  // Ball passed the CPU
        {
            mcu_scoreTone();
            mcu_score++;  // Increment CPU score when ball hits right side (CPU side)
        }

        // Check if the score limit is reached
        if (player_score == SCORE_LIMIT || mcu_score == SCORE_LIMIT)
        {
            win = player_score > mcu_score;
            game_over = true;
        }
        
        ball_dir_x = -ball_dir_x;
        ball_dir_y = BALL_SPEED;

        new_x = CENTER_X;
        new_y = CENTER_Y;
    }
    
    // Check if we hit the horizontal walls.
    if(new_y <= 0 || new_y >= SCREEN_HEIGHT) 
    {
        wallTone();
        ball_dir_y = -ball_dir_y;
        new_y = ball_y + ball_dir_y;
    }

    display.drawPixel(ball_x, ball_y, BLACK);
    display.drawPixel(new_x, new_y, WHITE);
    ball_x = new_x;
    ball_y = new_y;

    delay(5);

    
    return; // STOP HERE

    // Check if we hit the CPU paddle
    if(new_x == MCU_X && new_y >= mcu_y && new_y <= mcu_y + PADDLE_HEIGHT) 
    {
        mcuPaddleTone();

        uint8_t center_y = mcu_y + PADDLE_HEIGHT / 2;

        // Ball direction change, clamping to avoid steep angles
        ball_dir_x = 1;  // Ensure the ball always goes right when it hits the CPU paddle
        ball_dir_y = constrain(ball_y - center_y, -1, 1);  // Clamping the Y direction change to avoid steep angles
    }

    // Check if we hit the player paddle
    if(new_x == PLAYER_X && new_y >= player_y && new_y <= player_y + PADDLE_HEIGHT)
    {
        playerPaddleTone();
        
        uint8_t center_y = player_y + PADDLE_HEIGHT / 2;

        // Ball direction change, clamping to avoid steep angles
        ball_dir_x = -1;  // Ensure the ball always goes left when it hits the player paddle
        // Serial.println(ball_y - center_y);
        ball_dir_y = constrain(ball_y - center_y, -1, 1);  // Clamping the Y direction change to avoid steep angles
        
        // Serial.println(ball_dir_y);
    }

    display.drawPixel(ball_x, ball_y, BLACK);
    display.drawPixel(new_x, new_y, WHITE);
    ball_x = new_x;
    ball_y = new_y;


    update_needed = true;
    

    if(update_needed) 
    {
        // CPU paddle
        display.drawFastVLine(MCU_X, mcu_y, PADDLE_HEIGHT, BLACK); // hide
        
        const uint8_t half_paddle = PADDLE_HEIGHT >> 1;

        if(mcu_y + half_paddle > ball_y)
        {
            int8_t dir = ball_x > MCU_X ? -1 : 1;
            mcu_y += dir;
        }

        if(mcu_y + half_paddle < ball_y)
        {
            int8_t dir = ball_x > MCU_X ? 1 : -1;
            mcu_y += dir;
        }

        mcu_y = constrain(mcu_y, 1, SCREEN_HEIGHT - PADDLE_HEIGHT);

        display.drawFastVLine(MCU_X, mcu_y, PADDLE_HEIGHT, WHITE); // show

        // Player paddle
        display.drawFastVLine(PLAYER_X, player_y, PADDLE_HEIGHT, BLACK); // hide

        int val = analogRead(paddle_control_pin);

        player_y += map(val, 0, 1023, -5, 5); // change
        //player_y = map(val, 0, 1023, 1, SCREEN_HEIGHT - PADDLE_HEIGHT); // set
        
        player_y = constrain(player_y, 1, SCREEN_HEIGHT - PADDLE_HEIGHT);

        display.drawFastVLine(PLAYER_X, player_y, PADDLE_HEIGHT, WHITE); // show

        update_needed = true;
    }

    if(update_needed) {
        display.setTextColor(WHITE, BLACK);
        display.setCursor(0, 56);
        display.print(mcu_score);
        display.setCursor(122, 56);
        display.print(player_score);
        display.display();

        if (game_over) {
            on_game_over();
        }
    }
}



void setup() 
{
    pinMode(input_pin, INPUT);
    pinMode(output_pin, OUTPUT);
    pinMode(paddle_control_pin, INPUT);
    pinMode(speaker, OUTPUT);
    
    digitalWrite(output_pin, HIGH);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.display();
}

void loop() 
{
    if (digitalRead(input_pin) == HIGH && playing == false) {
        reset_pong();
        playing = true;
    }

    if (playing == true) {
        game_update();
    }

}