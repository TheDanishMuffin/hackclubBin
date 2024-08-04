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
int snakeX[MAX_SNAKE_LENGTH];
int snakeY[MAX_SNAKE_LENGTH];
int snakeLength = 5;
int foodX, foodY;
int direction = 3;

int score = 0;
bool isPaused = false;
bool adjustingMultiplier = false;
int gameSpeed = 100; 
const int speedOptions[] = {150, 100, 50}; 
int selectedSpeedIndex = 1; 

enum GameMode { NORMAL, GROWTH_MULTIPLIER };
GameMode selectedGameMode = NORMAL;
int growthMultiplier = 1;

enum ObstacleMode { NO_OBSTACLES, STATIC_OBSTACLES, DYNAMIC_OBSTACLES };
ObstacleMode selectedObstacleMode = NO_OBSTACLES;

#define MAX_OBSTACLES 5
int obstacleX[MAX_OBSTACLES];
int obstacleY[MAX_OBSTACLES];
int obstacleCount = 0;
int dynamicObstacleSpeed = 1;

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

  selectGameMode();
  selectGameSpeed();
  selectObstacleMode();
  startGame();
}

void loop() {
  if (digitalRead(joystickSel) == LOW) {
    while (digitalRead(joystickSel) == LOW); // Debounce button press
    if (!isPaused && !adjustingMultiplier) {
      isPaused = true;
      displayPauseScreen();
    } else if (isPaused && !adjustingMultiplier) {
      isPaused = false;
    } else if (!isPaused && adjustingMultiplier) {
      adjustingMultiplier = false;
      isPaused = true;
      displayPauseScreen();
    }
  }

  if (!isPaused && !adjustingMultiplier) {
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
    if (selectedObstacleMode == DYNAMIC_OBSTACLES) {
      updateDynamicObstacles();
    }

    if (checkCollision()) {
      gameOver();
    }

    if (snakeX[0] == foodX && snakeY[0] == foodY) {
      for (int i = 0; i < growthMultiplier; i++) {
        if (snakeLength < MAX_SNAKE_LENGTH) {
          snakeLength++;
        }
      }
      score += growthMultiplier;
      generateFood();
    }

    displayGame();

    delay(gameSpeed);
  } else if (isPaused && !adjustingMultiplier) {
    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); // Debounce button press
      isPaused = false;
    }
  } else if (adjustingMultiplier) {
    displayMultiplierAdjustment();
  }

  if (digitalRead(joystickSel) == LOW && isPaused) {
    while (digitalRead(joystickSel) == LOW); // Debounce button press
    adjustingMultiplier = true;
    isPaused = false;
    displayMultiplierAdjustment();
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

  if (selectedObstacleMode != NO_OBSTACLES) {
    for (int i = 0; i < obstacleCount; i++) {
      int ox = obstacleX[i];
      int oy = obstacleY[i];
      if ((snakeX[0] == ox || snakeX[0] == ox + 1) && (snakeY[0] == oy || snakeY[0] == oy + 1)) {
        return true;
      }
    }
  }

  return false;
}

void gameOver() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Game Over :(");
  display.setCursor(0, 10);
  display.print("Score: ");
  display.print(score);
  display.setCursor(0, 20);
  display.print("Press SEL to Restart");
  display.display();

  while (true) {
    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); // Debounce button press
      selectGameMode();
      selectGameSpeed();
      selectObstacleMode();
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

  generateFood();
  generateObstacles();
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
    for (int i = 0; i < obstacleCount; i++) {
      int ox = obstacleX[i];
      int oy = obstacleY[i];
      if ((foodX == ox || foodX == ox + 1) && (foodY == oy || foodY == oy + 1)) {
        validPosition = false;
        break;
      }
    }
  }
}

void generateObstacles() {
  if (selectedObstacleMode == NO_OBSTACLES) {
    obstacleCount = 0;
    return;
  }

  obstacleCount = random(1, MAX_OBSTACLES);
  for (int i = 0; i < obstacleCount; i++) {
    bool validPosition = false;
    while (!validPosition) {
      validPosition = true;
      obstacleX[i] = random(0, SCREEN_WIDTH / GRID_SIZE - 1);
      obstacleY[i] = random(0, SCREEN_HEIGHT / GRID_SIZE - 1);
      for (int j = 0; j < snakeLength; j++) {
        if ((snakeX[j] == obstacleX[i] || snakeX[j] == obstacleX[i] + 1) && 
            (snakeY[j] == obstacleY[i] || snakeY[j] == obstacleY[i] + 1)) {
          validPosition = false;
          break;
        }
      }
      for (int j = 0; j < i; j++) {
        if ((obstacleX[j] == obstacleX[i] || obstacleX[j] == obstacleX[i] + 1) &&
            (obstacleY[j] == obstacleY[i] || obstacleY[j] == obstacleY[i] + 1)) {
          validPosition = false;
          break;
        }
      }
      if ((foodX == obstacleX[i] || foodX == obstacleX[i] + 1) && (foodY == obstacleY[i] || foodY == obstacleY[i] + 1)) {
        validPosition = false;
      }
    }
  }
}

void updateDynamicObstacles() {
  for (int i = 0; i < obstacleCount; i++) {
    obstacleX[i] += random(-dynamicObstacleSpeed, dynamicObstacleSpeed + 1);
    obstacleY[i] += random(-dynamicObstacleSpeed, dynamicObstacleSpeed + 1);

    if (obstacleX[i] < 0) obstacleX[i] = 0;
    if (obstacleX[i] >= SCREEN_WIDTH / GRID_SIZE - 1) obstacleX[i] = SCREEN_WIDTH / GRID_SIZE - 2;
    if (obstacleY[i] < 0) obstacleY[i] = 0;
    if (obstacleY[i] >= SCREEN_HEIGHT / GRID_SIZE - 1) obstacleY[i] = SCREEN_HEIGHT / GRID_SIZE - 2;
  }
}

void selectGameMode() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Select Game Mode:");
  display.setCursor(0, 10);
  display.print("> Normal");
  display.setCursor(0, 20);
  display.print("  Growth Multiplier");
  display.display();

  int cursorY = 10;
  while (true) {
    int vert = analogRead(joystickVert);
    if (vert > center + threshold) {
      cursorY = 10;
      selectedGameMode = NORMAL;
    } else if (vert < center - threshold) {
      cursorY = 20;
      selectedGameMode = GROWTH_MULTIPLIER;
    }

    display.setCursor(0, cursorY);
    display.print(">");
    display.setCursor(0, cursorY == 10 ? 20 : 10);
    display.print(" ");

    display.display();
    delay(200);

    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); // Debounce button press
      break;
    }
  }
}

void selectGameSpeed() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Select Game Speed:");
  for (int i = 0; i < 3; i++) {
    display.setCursor(0, 10 + i * 10);
    display.print((i == selectedSpeedIndex ? ">" : " ") + String(speedOptions[i]));
  }
  display.display();

  int cursorY = 10 + selectedSpeedIndex * 10;
  while (true) {
    int vert = analogRead(joystickVert);
    if (vert > center + threshold && cursorY > 10) {
      cursorY -= 10;
      selectedSpeedIndex--;
    } else if (vert < center - threshold && cursorY < 30) {
      cursorY += 10;
      selectedSpeedIndex++;
    }

    for (int i = 0; i < 3; i++) {
      display.setCursor(0, 10 + i * 10);
      display.print((i == selectedSpeedIndex ? ">" : " ") + String(speedOptions[i]));
    }
    display.display();
    delay(200);

    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); // Debounce button press
      gameSpeed = speedOptions[selectedSpeedIndex];
      break;
    }
  }
}

void selectObstacleMode() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Select Obstacle Mode:");
  display.setCursor(0, 10);
  display.print("> No Obstacles");
  display.setCursor(0, 20);
  display.print("  Static Obstacles");
  display.setCursor(0, 30);
  display.print("  Dynamic Obstacles");
  display.display();

  int cursorY = 10;
  while (true) {
    int vert = analogRead(joystickVert);
    if (vert > center + threshold && cursorY > 10) {
      cursorY -= 10;
      selectedObstacleMode = static_cast<ObstacleMode>((cursorY - 10) / 10);
    } else if (vert < center - threshold && cursorY < 30) {
      cursorY += 10;
      selectedObstacleMode = static_cast<ObstacleMode>((cursorY - 10) / 10);
    }

    display.setCursor(0, cursorY);
    display.print(">");
    display.setCursor(0, cursorY == 10 ? 20 : cursorY == 20 ? 10 : 20);
    display.print(" ");

    display.display();
    delay(200);

    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW); // Debounce button press
      break;
    }
  }
}

void displayGame() {
  display.clearDisplay();


  for (int i = 0; i < snakeLength; i++) {
    display.fillRect(snakeX[i] * GRID_SIZE, snakeY[i] * GRID_SIZE, GRID_SIZE, GRID_SIZE, SSD1306_WHITE);
  }

  // Draw food with this form
  display.fillRect(foodX * GRID_SIZE, foodY * GRID_SIZE, GRID_SIZE, GRID_SIZE, SSD1306_WHITE);

  for (int i = 0; i < obstacleCount; i++) {
    display.fillRect(obstacleX[i] * GRID_SIZE, obstacleY[i] * GRID_SIZE, GRID_SIZE * 2, GRID_SIZE * 2, SSD1306_WHITE);
  }

  // Display score
  display.setCursor(0, 0);
  display.print("Score: ");
  display.print(score);

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

void displayMultiplierAdjustment() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Adjust Multiplier");
  display.setCursor(0, 10);
  display.print("> x");
  display.print(growthMultiplier);
  display.display();

  while (true) {
    int vert = analogRead(joystickVert);
    if (vert > center + threshold) {
      growthMultiplier++;
    } else if (vert < center - threshold) {
      growthMultiplier--;
    }

    if (growthMultiplier < 1) growthMultiplier = 1;
    if (growthMultiplier > 5) growthMultiplier = 5;

    display.setCursor(0, 10);
    display.print("> x");
    display.print(growthMultiplier);
    display.display();
    delay(200);

    if (digitalRead(joystickSel) == LOW) {
      while (digitalRead(joystickSel) == LOW);
      break;
    }
  }
}

