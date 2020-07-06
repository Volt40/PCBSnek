/*
 * Snake for ATMEGA16A-AU to be run on PCBSnek.
 */
#include <CapacitiveSensor.h>

// Constants
const int matrix_size = 4; // Size of the LED matrix (x * x)
const int sensor_threshold = 50; // Minimum value for sensors.
const int sample_size = 30; // Sensor sample size per read.
const int start_x = 0; // The snake's start x position.
const int start_y = 0; // The snake's start y position.
const int led_gnd_pins[] = {0, 1, 2, 3}; // The led matrix's ground pins.
const int led_pow_pins[] = {4, 5, 6, 7}; // The led matrix's power pins.
const int d_pad_pins[] = {10, 11, 12, 9}; // D-Pad sensor pins, Up, Down, Left, Right.
const int d_pad_send_pin = 13; // D-Pad send sensor pin.
const int update_rate = 150; // Amount of cycles before a game update.
const int blink_rate = 40; // Amount of cycles before a blink update.
const int restart_animation_time = 300; // Amount of time the restart animation takes.

// Snake variables
int snake_x[matrix_size * matrix_size] = {start_x}; // Snake's x positions.
int snake_y[matrix_size * matrix_size] = {start_y}; // Snake's y positions.
int head_direction = 3; // Direction of the snake. Up - 0, Down - 1, Left - 2, Right - 3.
int snake_length = 1; // Length of the snake.
int food_x; // The food's x position.
int food_y; // The food's y position.

// Hardware variables.
boolean led_matrix[matrix_size][matrix_size]; // LED Matrix pointers. True = on.
int current_row = 0; // The current row being lit up.
CapacitiveSensor up = CapacitiveSensor(d_pad_send_pin, d_pad_pins[0]); // Up dpad sensor.
CapacitiveSensor down = CapacitiveSensor(d_pad_send_pin, d_pad_pins[1]); // Down dpad sensor.
CapacitiveSensor left = CapacitiveSensor(d_pad_send_pin, d_pad_pins[2]); // Left dpad sensor.
CapacitiveSensor right = CapacitiveSensor(d_pad_send_pin, d_pad_pins[3]); // Right dpad sensor.
int update_count = 0; // Counter for game updator.
int blink_count = 0; // Counter for blinking objects.
boolean restarting = false; // True if the game is restarting.
int restart_timer_count = 0; // Counter for the restart animation. 

// Repaints the matrix.
void repaintMatrix() {
  // Clear the matrix.
  for (int i = 0; i < matrix_size; i++)
    for (int j = 0; j < matrix_size; j++)
      led_matrix[i][j] = false;
  // Paint the snake.
  for (int i = 0; i < snake_length; i++)
    led_matrix[snake_x[i]][snake_y[i]] = true;
  // Paint the food.
  if (blink_count < blink_rate / 2)
    led_matrix[food_x][food_y] = true;
}

// Reads the sensors and updates the direction.
void readSensors() {
  int up_reading = up.capacitiveSensor(sample_size);
  int down_reading = down.capacitiveSensor(sample_size);
  int left_reading = left.capacitiveSensor(sample_size);
  int right_reading = right.capacitiveSensor(sample_size);
  int new_direction = -1;
  if (up_reading > down_reading && up_reading > left_reading && up_reading > right_reading && up_reading > sensor_threshold)
    new_direction = 0;
  else if (down_reading > up_reading && down_reading > left_reading && down_reading > right_reading && down_reading > sensor_threshold)
    new_direction = 1;
  else if (left_reading > up_reading && left_reading > down_reading && left_reading > right_reading && left_reading > sensor_threshold)
    new_direction = 2;
  else if (right_reading > up_reading && right_reading > left_reading && right_reading > down_reading && right_reading > sensor_threshold)
    new_direction = 3;
  if (new_direction != -1 && snake_length > 1) { // Check to make sure the snake isnt running back through itself.
    switch(new_direction) {
      case 0: // Up
        if ((snake_y[snake_length - 2] - 1 < 0 ? matrix_size - 1 : snake_y[snake_length - 2]) - 1 == snake_y[snake_length - 1])
          return;
      break; // Down
      case 1:
        if ((snake_y[snake_length - 2] + 1) % matrix_size == snake_y[snake_length - 1])
          return;
      break; // Left
      case 2:
        if ((snake_x[snake_length - 2] + 1) % matrix_size == snake_x[snake_length - 1])
          return;
      break; // Right
      case 3:
        if ((snake_x[snake_length - 2] - 1 < 0 ? matrix_size - 1 : snake_x[snake_length - 2]) - 1 == snake_x[snake_length - 1])
          return;
      break;
    }
  }
  // If everything is good, change the direction.
  if (new_direction != -1)
    head_direction = new_direction;
}

// Restarts the game.
void restartGame() {
  // Blink whole screen for a second.
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < matrix_size; j++)
      if (i % 2 == 0)
        digitalWrite(led_gnd_pins[j], LOW);
      else
        digitalWrite(led_gnd_pins[j], HIGH);
    for (int j = 0; j < matrix_size; j++)
      if (i % 2 == 0)
        digitalWrite(led_pow_pins[j], HIGH);
      else
        digitalWrite(led_pow_pins[j], LOW);
     delay(100);
  }
  // Reset all values.
  head_direction = 3;
  snake_length = 1;
  snake_x[0] = start_x;
  snake_y[0] = start_y;
  randomizeFood();
  restarting = false;
  restart_timer_count = 0;
  repaintMatrix();
  delay(200);
}

// Updates the game.
void updateGame() {
  // Get new head position.
  int head_x = snake_x[snake_length - 1];
  int head_y = snake_y[snake_length - 1];
  switch(head_direction) {
    case 0: // Up
      head_y = (head_y + 1) % matrix_size;
    break;
    case 1: // Down
      head_y--;
      if (head_y < 0)
        head_y = matrix_size - 1;
    break;
    case 2: // Left
      head_x--;
      if (head_x < 0)
        head_x = matrix_size - 1;
    break;
    case 3: // Right
      head_x = (head_x + 1) % matrix_size;
    break;
  }
  // Check to see if food is going to be eaten.
  if (head_x == food_x && head_y == food_y) {
    // The food was eaten.
    snake_length++;
    snake_x[snake_length - 1] = head_x;
    snake_y[snake_length - 1] = head_y;
    randomizeFood(); // Pick new food position.
    // Stop the method here.
    return;
  }
  // Check to see if the snake is going to die.
  for (int i = 0; i < snake_length - 1; i++)
    if (head_x == snake_x[i] && head_y == snake_y[i]) {
      // Snake is dead, restart the game.
      restarting = true;
      return;
    }
  // Move the snake along.
  for (int i = 1; i < snake_length; i++) {
    snake_x[i - 1] = snake_x[i];
    snake_y[i - 1] = snake_y[i];
  }
  // Set the head.
  snake_x[snake_length - 1] = head_x;
  snake_y[snake_length - 1] = head_y;
  // Done!
}

// Randomizes the food's position.
void randomizeFood() {
  while(true) {
    // Pick random position.
    food_x = (millis() * random()) % matrix_size;
    food_y = (millis() * random()) % matrix_size;
    boolean good_position = true;
    // Check to make sure it is not inside the snake.
    for (int i = 0; i < snake_length; i++)
      if (food_x == snake_x[i] && food_y == snake_y[i]) {
        good_position = false;
        break;
      }
    if (good_position)
      break;
  }
}

void setup() {
  // Set pin modes.
  for (int i = 0; i < matrix_size; i++) {
    pinMode(led_pow_pins[i], OUTPUT);
    pinMode(led_gnd_pins[i], OUTPUT);
  }
  randomizeFood(); // Randomize food.
  repaintMatrix(); // First paint.
}

void loop() {
  // First, turn everything off.
  for (int i = 0; i < matrix_size; i++)
    digitalWrite(led_gnd_pins[i], HIGH);
  // Check if the game is restarting.
  if (restarting) {
    if (restart_timer_count < restart_animation_time) {
      // If first animation stage.
      // Stop food blink.
      led_matrix[food_x][food_y] = false;
      // Blink the head.
      if (millis() % 100 < 50)
        led_matrix[snake_x[snake_length - 1]][snake_y[snake_length - 1]] = false;
      else
        led_matrix[snake_x[snake_length - 1]][snake_y[snake_length - 1]] = true;
    } else {
      // Second animation stage.
      restartGame(); // Restart the game.
    }
    restart_timer_count++;
  } else if (update_count == update_rate - 1) { // Game update.
    // Read sensors.
    readSensors();
    // Update the game.
    updateGame();
    // Repaint matrix.
    repaintMatrix();
  }
  // Repaint food.
  if (!restarting)
    if (blink_count < blink_rate / 2)
      led_matrix[food_x][food_y] = true;
    else
      led_matrix[food_x][food_y] = false;
  for (int i = 0; i < matrix_size; i++)
    if (led_matrix[current_row][i])
      digitalWrite(led_gnd_pins[matrix_size - 1 - i], LOW);
    else
      digitalWrite(led_gnd_pins[matrix_size - 1 - i], HIGH);
  for (int i = 0; i < matrix_size; i++)
    if (i == current_row)
      digitalWrite(led_pow_pins[i], HIGH);
    else
      digitalWrite(led_pow_pins[i], LOW);
  // Increment counts.
  update_count = (update_count + 1) % update_rate;
  blink_count = (blink_count + 1) % blink_rate;
  current_row = (current_row + 1) % matrix_size;
  delay(4); // Loop delay.
}
