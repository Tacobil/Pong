#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// PINS //
const int control_pin_y = A1;
const int control_pin_x = A0;
const int speaker = A2;
const int input_pin = 23;
const int output_pin = 22;

// SETTINGS //
const uint8_t SCREEN_HEIGHT = 62, SCREEN_WIDTH = 127;
const uint8_t CENTER_X = SCREEN_WIDTH / 2, CENTER_Y = SCREEN_HEIGHT / 2;

const uint8_t SERVE_X = 20; // pixels from the wall
const uint8_t PADDLE_START_X = 10; // pixels from the wall

const uint8_t PADDLE_HEIGHT = 15;
const uint8_t PADDLE_WIDTH = 5;
const uint8_t QUEUE_CAP = 3; // max amount of inputs in the queue

float PADDLE_SPEED = 1;
float MCU_SPEED = 0.7;
float SPEEDUP = 1.1;


bool DVD_SOUND = true;

// OTHER VARIABLES
float mcu_x, mcu_y;
float player_x, player_y;

float mcu_speed_x, mcu_speed_y;
float player_speed_x, player_speed_y;

float ball_x, ball_y;
float ball_dir_x, ball_dir_y;
float ball_max_speed;

bool win;
bool playing = false;

bool input_currently;
uint8_t queued_inputs = 0;

uint8_t lives;

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

// DVD VARIABLES
int8_t dvd_w = 12, dvd_h = 16;
int8_t dvd_dx = 1, dvd_dy = 1;
int8_t dvd_x = (SCREEN_WIDTH - dvd_w)/2;
int8_t dvd_y = (SCREEN_HEIGHT - dvd_h)/2;

// 'dvd logo', 24x10px
const unsigned char dvd_bitmap [] PROGMEM = {
	0x3f, 0xe1, 0xfe, 0x03, 0xe3, 0x87, 0x31, 0xf6, 0xe7, 0x31, 0xbe, 0xc7, 0x7f, 0x18, 0xfe, 0x7e, 
	0x19, 0xf8, 0x00, 0x10, 0x00, 0x07, 0xff, 0x80, 0xff, 0x07, 0xfc, 0x3f, 0xff, 0xf8
};

// 'Figure', 12x16px
const unsigned char figure_bitmap [] PROGMEM = {
	0x20, 0x40, 0x56, 0xa0, 0xd9, 0xb0, 0x80, 0x10, 0xd0, 0xb0, 0x49, 0x20, 0x40, 0x20, 0x29, 0x40, 
	0x16, 0x80, 0x60, 0x60, 0xd0, 0xb0, 0x99, 0x90, 0x29, 0x40, 0x2f, 0x40, 0x50, 0xa0, 0xf0, 0xf0
};

// 'Heart2', 7x7px
const unsigned char heart_bitmap [] PROGMEM = {
	0x44, 0xee, 0xfe, 0xfe, 0x7c, 0x38, 0x10
};


void setupScreensaver() {
    display.clearDisplay();
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
}

void updateScreensaver() {
  // Vertical walls
  bool hit_x = false, hit_y = false;
  if (dvd_x + dvd_dx < 1 || dvd_x + dvd_dx + dvd_w > SCREEN_WIDTH - 1) {
    dvd_dx = -dvd_dx;
    hit_x = true;
  }

  // Horizontal walls
  if (dvd_y + dvd_dy < 1 || dvd_y + dvd_dy + dvd_h > SCREEN_HEIGHT - 1) {
    dvd_dy = -dvd_dy;
    hit_y = true;
  }


  // Corner
  if (hit_x && hit_y) {
    if (DVD_SOUND) {
        player_scoreTone();
    }
    Serial.println("CORNER");
  }

  display.fillRect(dvd_x, dvd_y, dvd_w, dvd_h, BLACK);
  dvd_x += dvd_dx;
  dvd_y += dvd_dy;
  // display.drawRect(dvd_x, dvd_y, dvd_w, dvd_h, WHITE); hitbox
  //display.drawBitmap(dvd_x+1, dvd_y, dvd_bitmap, 24, 10, WHITE);
  display.drawBitmap(dvd_x, dvd_y, figure_bitmap, dvd_w, dvd_h, WHITE); // figure
  display.display();
  delay(5);
}

void drawCourt() {
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);

    for (int y = 0; y < SCREEN_HEIGHT; y += 4) {
        display.drawFastVLine(CENTER_X, y, 2, WHITE);  // 2-pixel segment, then 2-pixel gap
    }
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

void serve(bool player_serve) {
  ball_dir_x = 0;
  ball_dir_y = 0;

  ball_y = CENTER_Y;

  if (player_serve) {
    ball_x = SCREEN_WIDTH - SERVE_X;
  }
  else{
    ball_x = SERVE_X;
  }
}

bool pointInRect(int px, int py, int rx, int ry, int rw, int rh) {
  return (px >= rx) && (px < rx + rw) &&
         (py >= ry) && (py < ry + rh);
}

void gameOver() {
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
    setupScreensaver();
}

void setupPong() {
    lives = 3;

    player_x = SCREEN_WIDTH - PADDLE_START_X;
    player_y = CENTER_Y - PADDLE_HEIGHT/2;

    mcu_x = PADDLE_START_X;
    mcu_y = CENTER_Y - PADDLE_HEIGHT/2;

    ball_max_speed = 0.7;
    
    display.clearDisplay();
    drawCourt();
}

void updatePong() {
    display.clearDisplay();
    // BALL
    float new_x = ball_x + ball_dir_x;
    float new_y = ball_y + ball_dir_y;

    // Check if we hit the vertical walls
    if(new_x <= 0 || new_x >= SCREEN_WIDTH) 
    {
        player_x = SCREEN_WIDTH - PADDLE_START_X;
        player_y = CENTER_Y - PADDLE_HEIGHT/2;

        mcu_x = PADDLE_START_X;
        mcu_y = CENTER_Y - PADDLE_HEIGHT/2;


        if (new_x < CENTER_X)  // player scores
        {
            win = true;
            player_scoreTone();
            gameOver();
            return;
        }
        else  // cpu scores
        {
            mcu_scoreTone();
            lives--;  // Increment CPU score when ball hits right side (CPU side)
            serve(true);
        }

        new_x = ball_x;
        new_y = ball_y;
        ball_max_speed = 1;

        // Check if the score limit is reached
        if (lives == 0)
        {
            win = false;
            gameOver();
            return;
        }
    }

    // Check if we hit the horizontal walls
    if(new_y <= 1 || new_y >= SCREEN_HEIGHT - 1) {
        wallTone();
        ball_dir_y = -ball_dir_y;
        new_y += ball_dir_y + ball_dir_y;
    }

    // Check if we hit the CPU paddle
    if(pointInRect(new_x, new_y, mcu_x, mcu_y, PADDLE_WIDTH, PADDLE_HEIGHT)) {
        mcuPaddleTone();

        ball_dir_x = abs(ball_dir_x);
        ball_dir_y = -ball_dir_y;

        ball_dir_x = constrain(ball_dir_x, -ball_max_speed, ball_max_speed);

        ball_max_speed *= SPEEDUP;

        new_x += ball_dir_x;
        new_y += ball_dir_y;
    }

    // Check if we hit the player paddle
    if(pointInRect(new_x, new_y, player_x, player_y, PADDLE_WIDTH, PADDLE_HEIGHT)) {
        playerPaddleTone();

        uint8_t center_x = player_x + PADDLE_WIDTH / 2;
        uint8_t center_y = player_y + PADDLE_HEIGHT / 2;
        int8_t offset_x = ball_x - center_x;
        int8_t offset_y = ball_y - center_y;

        ball_dir_x = offset_x / ((float)PADDLE_WIDTH / 2) + player_speed_x + ball_dir_x;
        ball_dir_y = offset_y / ((float)PADDLE_HEIGHT / 2) + player_speed_y + ball_dir_y;

        ball_dir_x = constrain(ball_dir_x, -ball_max_speed, ball_max_speed);
        ball_dir_y = constrain(ball_dir_y, -ball_max_speed, ball_max_speed);

        ball_max_speed *= SPEEDUP;

        new_x += ball_dir_x;
        new_y += ball_dir_y;
    }

    display.drawPixel((int)new_x, (int)new_y, WHITE);
    
    ball_x = new_x;
    ball_y = new_y;
    
    // PADDLE
    // move paddles
    mcu_x = constrain(mcu_x + mcu_speed_x, 1, CENTER_X - PADDLE_WIDTH);
    mcu_y = constrain(mcu_y + mcu_speed_y, 1, SCREEN_HEIGHT - PADDLE_HEIGHT - 1);

    player_x = constrain(player_x + player_speed_x, CENTER_X, SCREEN_WIDTH - PADDLE_WIDTH - 1);  
    player_y = constrain(player_y + player_speed_y, 1, SCREEN_HEIGHT - PADDLE_HEIGHT - 1); 

    // show paddles
    display.drawRect((int)mcu_x, (int)mcu_y, PADDLE_WIDTH, PADDLE_HEIGHT, WHITE);

    display.drawRect((int)player_x, (int)player_y, PADDLE_WIDTH, PADDLE_HEIGHT, WHITE);
    display.drawFastVLine((int)player_x+2, (int)player_y+2, PADDLE_HEIGHT - 4, WHITE);
    //display.drawRect((int)player_x+1, (int)player_y+1, PADDLE_WIDTH-2, PADDLE_HEIGHT-2, WHITE);


    // CPU paddle AI

    const int8_t prediction_frames = 6;        // Predict ahead by this many frames
    const int8_t mcu_react_zone = CENTER_X;    // When the MCU starts reacting

    // Predict future ball position
    int8_t predicted_x = ball_x + ball_dir_x * prediction_frames;
    int8_t predicted_y = ball_y + ball_dir_y * prediction_frames;
    int8_t offset_x, offset_y;

    if (predicted_x <= mcu_react_zone) {
        // Track the predicted ball position
        offset_x = predicted_x - (mcu_x + PADDLE_WIDTH / 2) + 2;
        offset_y = predicted_y - (mcu_y + PADDLE_HEIGHT / 2);
    } else {
        // Return to idle position smoothly
        offset_x = PADDLE_START_X - (mcu_x + PADDLE_WIDTH / 2);
        offset_y = CENTER_Y - (mcu_y + PADDLE_HEIGHT / 2);
    }
        mcu_speed_x = constrain(offset_x, -PADDLE_SPEED, PADDLE_SPEED)*MCU_SPEED;
        mcu_speed_y = constrain(offset_y, -PADDLE_SPEED, PADDLE_SPEED)*MCU_SPEED;

    // player paddle
    int x = analogRead(control_pin_x);
    int y = analogRead(control_pin_y);

    // Convert raw values to -512 to +511 range
    int dx = x - 512;
    int dy = y - 512;

    // Apply dead zone
    int dead_zone = 20;
    if (abs(dx) < dead_zone) dx = 0;
    if (abs(dy) < dead_zone) dy = 0;

    // Remap from -512..511 to -1000..1000
    player_speed_x = -dx * 1000.0 / 512.0 * 0.001;
    player_speed_y =  dy * 1000.0 / 512.0 * 0.001;

    player_speed_x = constrain(player_speed_x, -PADDLE_SPEED, PADDLE_SPEED);
    player_speed_x = constrain(player_speed_x, -PADDLE_SPEED, PADDLE_SPEED);

    /*float length = getLength(player_speed_x, player_speed_y);
    if (length > PADDLE_SPEED && length != 0) {
        player_speed_x = player_speed_x * PADDLE_SPEED / length;
        player_speed_y = player_speed_y * PADDLE_SPEED / length;
    }*/

    //player_y = map(y, 0, 1023, 1, SCREEN_HEIGHT - PADDLE_HEIGHT); // used for potentiometer
    drawCourt();

    // Display hearts
    for (int i = 0; i < lives; i ++) {
        uint8_t x = i*10 + 3, y = 3;
        display.drawBitmap(x, y, heart_bitmap, 7, 7, WHITE);
        //display.drawFastVLine(3+i*3, 5, 4, WHITE);  // 2-pixel segment, then 2-pixel gap
    }


    display.display();
}

void setup() {
    pinMode(input_pin, INPUT);
    pinMode(output_pin, OUTPUT);
    pinMode(control_pin_x, INPUT);
    pinMode(control_pin_y, INPUT);
    pinMode(speaker, OUTPUT);
    
    digitalWrite(output_pin, HIGH);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextColor(WHITE, BLACK);
    display.display();

    setupScreensaver();
}

void loop() {
    if (digitalRead(input_pin) == HIGH) {
        if ((input_currently == false && queued_inputs < QUEUE_CAP)) {
            // Runs when input turns high
            input_currently = true;
            queued_inputs += 1;
            Serial.print("+1 input. queue: ");
            Serial.println(queued_inputs);
        }
    }
    else {
        input_currently = false;
    }

    if (queued_inputs > 0 && playing == false) {
        playing = true;
        queued_inputs--;
        setupPong();
        serve(true);

        Serial.print("-1 input. queue: ");
        Serial.println(queued_inputs);
    }

    if (playing == true) {
        updatePong();
    } else {
        updateScreensaver();
    }
}