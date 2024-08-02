#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int joystickVert = 26;
const int joystickHorz = 27;
const int joystickSel = 28;

const int center = 511;
const int threshold = 200;

#define GRID_SIZE 4
#define MAX_SNAKE_LENGTH 128
#define MAX_OBSTACLES 10

int snakeX[MAX_SNAKE_LENGTH];
int snakeY[MAX_SNAKE_LENGTH];
int snakeLength = 5;
int foodX, foodY;
int direction = 3;

int obstacleX[MAX_OBSTACLES];
int obstacleY[MAX_OBSTACLES];
int obstacleCount = 0;

int score = 0;
bool isPaused = false;
bool useObstacles = false;
int gameSpeed = 100; // Default game speed (milliseconds between updates)
const int speedOptions[] = {150, 100, 50}; // Slow, Normal, Fast
int selectedSpeedIndex = 1; // Default to Normal speed

void setup() {
  Serial1.begin(115200);
  Serial1.println("Hello, Raspberry Pi Pico W!");

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial1.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  pinMode(joystickVert, INPUT);
  pinMode(joystickHorz, INPUT);
  pinMode(joystickSel, INPUT_PULLUP);

  selectGameSpeed();
  selectObstacles();
  startGame();
}

void loop() {
  if (digitalRead(joystickSel) == LOW) {
    while (digitalRead(joystickSel) == LOW); // Debounce button press
    isPaused = !isPaused;
    if (isPaused) {
      displayPauseScreen();
    }
  }

  if (!isPaused) {
    int vert = analogRead(joystickVert);
    int horz = analogRead(joystickHorz);

    if (vert > center + threshold && direction != 1) {
      direction = 0;
    } else if (vert < center - threshold && direction != 0) {
      direction = 1;
    } else if (horz > center + threshold && direction != 2) {
      direction = 2;
    } else if (horz < center - threshold && direction != 3) {
      direction = 3;
    }

    updateSnake();

    if (checkCollision()) {
      gameOver();
    }

    if (snakeX[0] == foodX && snakeY[0] == foodY) {
      if (snakeLength < MAX_SNAKE_LENGTH) {
        snakeLength++;
      }
      score++;
      generateFood();
    }

    displayGame();

    delay(gameSpeed);
  }
}

void updateSnake() {
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }

  if (direction == 0) {
    snakeY[0]--;
  } else if (direction == 1) {
    snakeY[0]++;
  } else if (direction == 2) {
    snakeX[0]--;
  } else if (direction == 3) {
    snakeX[0]++;
  }

  if (snakeX[0] < 0 || snakeX[0] >= SCREEN_WIDTH / GRID_SIZE || snakeY[0] < 0 || snakeY[0] >= SCREEN_HEIGHT / GRID_SIZE) {
    gameOver();
  }
}

bool checkCollision() {
  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      return true;
    }
  }
  
  if (useObstacles) {
    for (int i = 0; i < obstacleCount; i++) {
      if (snakeX[0] == obstacleX[i] && snakeY[0] == obstacleY[i]) {
        return true;
      }
    }
  }

  return false;
}

void gameOver() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Game Over :()");
  display.setCursor(0, 10);
  display.print("Score: ");
  display.print(score);
  display.setCursor(0, 20);
  display.print("Press SEL to Restart");
  display.display();

  while (true) {
    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); // Debounce button press
      selectGameSpeed();
      selectObstacles();
      startGame();
      break;
    }
  }
}

void startGame() {
  snakeLength = 5;
  direction = 3;
  score = 0;
  isPaused = false;

  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = snakeLength - i - 1;
    snakeY[i] = 0;
  }

  if (useObstacles) {
    generateObstacles();
  }

  generateFood();
  display.clearDisplay();
}

void generateFood() {
  bool validPosition = false;
  while (!validPosition) {
    validPosition = true;
    foodX = random(0, SCREEN_WIDTH / GRID_SIZE);
    foodY = random(0, SCREEN_HEIGHT / GRID_SIZE);
    for (int i = 0; i < snakeLength; i++) {
      if (snakeX[i] == foodX && snakeY[i] == foodY) {
        validPosition = false;
        break;
      }
    }
    if (useObstacles) {
      for (int i = 0; i < obstacleCount; i++) {
        if (obstacleX[i] == foodX && obstacleY[i] == foodY) {
          validPosition = false;
          break;
        }
      }
    }
  }
}

void displayGame() {
  display.clearDisplay();

  display.fillRect(foodX * GRID_SIZE, foodY * GRID_SIZE, GRID_SIZE, GRID_SIZE, SSD1306_WHITE);

  for (int i = 0; i < snakeLength; i++) {
    display.fillRect(snakeX[i] * GRID_SIZE, snakeY[i] * GRID_SIZE, GRID_SIZE, GRID_SIZE, SSD1306_WHITE);
  }

  if (useObstacles) {
    for (int i = 0; i < obstacleCount; i++) {
      display.fillRect(obstacleX[i] * GRID_SIZE, obstacleY[i] * GRID_SIZE, GRID_SIZE, GRID_SIZE, SSD1306_WHITE);
    }
  }

  display.display();
}

void displayPauseScreen() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Game Paused");
  display.setCursor(0, 10);
  display.print("Press SEL to Resume");
  display.display();
}

void selectGameSpeed() {
  while (true) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Select Game Speed:");
    display.setCursor(0, 10);
    display.print("1. Slow");
    display.setCursor(0, 20);
    display.print("2. Normal");
    display.setCursor(0, 30);
    display.print("3. Fast");
    display.setCursor(0, 40);
    display.print("Selected: ");
    display.print(selectedSpeedIndex + 1);
    display.display();

    int vert = analogRead(joystickVert);

    if (vert > center + threshold) {
      selectedSpeedIndex = (selectedSpeedIndex + 1) % 3;
      delay(200); // Debounce delay
    } else if (vert < center - threshold) {
      selectedSpeedIndex = (selectedSpeedIndex - 1 + 3) % 3;
      delay(200); // Debounce delay
    }

    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); // Debounce button press
      gameSpeed = speedOptions[selectedSpeedIndex];
      break;
    }
  }
}

void selectObstacles() {
  while (true) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Enable Obstacles?");
    display.setCursor(0, 10);
    display.print("1. Yes");
    display.setCursor(0, 20);
    display.print("2. No");
    display.setCursor(0, 30);
    display.print("Selected: ");
    display.print(useObstacles ? "Yes" : "No");
    display.display();

    int vert = analogRead(joystickVert);

    if (vert > center + threshold) {
      useObstacles = !useObstacles;
      delay(200); // Debounce delay
    } else if (vert < center - threshold) {
      useObstacles = !useObstacles;
      delay(200); // Debounce delay
    }

    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); // Debounce button press
      break;
    }
  }
}

void generateObstacles() {
  obstacleCount = random(1, MAX_OBSTACLES);
  for (int i = 0; i < obstacleCount; i++) {
    bool validPosition = false;
    while (!validPosition) {
      validPosition = true;
      obstacleX[i] = random(0, SCREEN_WIDTH / GRID_SIZE);
      obstacleY[i] = random(0, SCREEN_HEIGHT / GRID_SIZE);
      for (int j = 0; j < snakeLength; j++) {
        if (snakeX[j] == obstacleX[i] && snakeY[j] == obstacleY[i]) {
          validPosition = false;
          break;
        }
      }
      if (foodX == obstacleX[i] && foodY == obstacleY[i]) {
        validPosition = false;
      }
    }
  }
}
