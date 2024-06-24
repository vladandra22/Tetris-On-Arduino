#include <setup.h>
#include <LiquidCrystal_I2C.h>
#include <LedControl.h>
#include <MD_Parola.h>
#include <MD_MAX72XX.h>
#include <SPI.h>
#include <cantecel_tetris.h>
#include <TimerOne.h>
#include <EEPROM.h>

LedControl lc = LedControl(DIN_PIN, CLK_PIN, CS_PIN, NUM_MAX7219);
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4);
MD_Parola lc_animatie = MD_Parola(HARDWARE_TYPE, DIN_PIN, CLK_PIN, CS_PIN, NUM_MAX7219);


void setup() {
  // Ne asiguram ca matricea de joc este goala.
  memset(gameGrid, 0, sizeof(gameGrid));

  // Extragem valoarea highscore-ului din jocuri anterioare din memoria EEPROM.
  EEPROM.get(address1, retrievedValue1);
  highscore = retrievedValue1;

  // Setare pini Joystick.
  pinMode(WRX_PIN, INPUT);
  pinMode(WRY_PIN, INPUT);
  // Adaugam o rezistenta de pull-up pentru buton.
  pinMode(SW_PIN, INPUT_PULLUP);

  pinMode(BUZZER_PIN, OUTPUT);

  // Setare LCD.
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tetris Game");
  lcd.setCursor(0, 1);
  lcd.print("is Starting...");
  lcd.setCursor(0, 2);
  lcd.print(":D");

  // Setare matrice Led-uri + animatie.
  lc.shutdown(0, false);
  lc.setIntensity(0, 1);
  lc.clearDisplay(0);

  lc_animatie.begin();
  lc_animatie.setIntensity(0);
  lc_animatie.displayText("TETRIS", PA_CENTER, 100, 1000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);

  while (!lc_animatie.displayAnimate()) {
    // Asteptam sa se termine animatia.
  }
  
  // Generam un seed random pentru generarea Tetromine random.
  randomSeed(analogRead(SEED_PIN) + micros());
  selectRandomTetromino();

  // Initializare timer pentru melodie.
  // Timer-ul declanseaza o data la 0.5 sec.
  Timer1.initialize(100000); 
  Timer1.attachInterrupt(urmatoareaNotaCantecel);

  // Pentru scrierea in Serial Monitor.
  Serial.begin(9600);
}

void loop() {
  int currentScore = score;
  // Verificam la fiecare loop statusul jocului.
  for (int i = 0; i < 8; i++) {
    if(gameGrid[4][i] == 3) {
      gameOver = true;
    }
  }
  // Salvam timpul curent pentru verificarea intervalelor.
  unsigned long currentMillis = millis();
  currentMillis = millis();
  int switch_state = digitalRead(SW_PIN);
  if (switch_state == LOW) {
    if (buttonPressTime == 0) {
      buttonPressTime = currentMillis;
    }
    else if (currentMillis - buttonPressTime >= pressDuration) {
      Serial.print("\nRestart game!\n");
      restartGame();
      // Resetam timpul ca sa nu declansam repetat butonul.
      buttonPressTime = 0;
    }
  }
  // Daca butonul nu e apasat, resetam timpul.
  else {
    buttonPressTime = 0;
  }
  currentMillis = millis();
  // Daca nu s-a terminat jocul, verificam periodic inpt-ul joystick-ului.
  if(!gameOver) {
    if (currentMillis - lastJoystickInput > intervalJoystick) {
      lastJoystickInput = currentMillis;
      moveTetromino();
    }
    // Periodic, facem update la joc.
    if (currentMillis - lastGameUpdate > intervalUpdate) {
      lastGameUpdate = currentMillis;
      updateGame();
      // Verificare stare joc.
      Serial.print("\nSTARE JOC\n");
      for(int i = 4; i < 12; i++) {
        for(int j = 0; j < 8; j++) {
          Serial.print(gameGrid[i][j]);
        }
        Serial.print("\n");
      }
      if (lastScore == -1 || lastScore != currentScore) {
        Serial.print("\nLAST SCORE: ");
        Serial.print(lastScore);
        Serial.print("\nSCORE: ");
        Serial.print(score);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Score: ");
        lcd.print(score);
        lcd.setCursor(0, 1);
        lcd.print("Highscore: ");
        lcd.print(highscore);
      }
    }
  }
  else {
    Timer1.stop();
    // Pentru prima oara cand intanlim gameOver, afisam mesajele.
    if(!displayGameOver) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Game Over!");
      lcd.setCursor(0, 1);
      lcd.print("Score: ");
      lcd.print(score);
      lcd.setCursor(0, 2);
      lcd.print("Highscore: ");
      lcd.print(highscore);
      displayGameOver = true;
      ledOnStartTime = currentMillis;
    }
    // Daca avem deja game Over de ceva vreme, inchidem matricea de LED-uri si display-ul.
    else if (displayGameOver == true && (currentMillis - ledOnStartTime >= ledOnDuration)){
      lc.shutdown(0, true);
      lcd.noDisplay();
      lcd.noBacklight();
    }
    // Ca sa nu afisam pe display la infinit.
    playSong = false;
    currentNote = 0;
  }
  lastScore = currentScore;
}

void moveTetromino() {
  unsigned long currentMillis = millis();
  int X_val = analogRead(WRX_PIN);
  int Y_val = analogRead(WRY_PIN);
  if (X_val < CENTRU - 200) {
    moveLeft();
    Serial.print("MOVE LEFT\n");
  } else if (X_val > CENTRU + 200) {
    moveRight();
    Serial.print("MOVE RIGHT\n");
  } else if (Y_val < CENTRU - 200) {
    rotateLeft(pieceId);
    Serial.print("ROTATE\n");
    // Pentru forceDown adaugam un delay mai mare.
  } else if (Y_val > CENTRU + 200 && (currentMillis - lastForceDownInput > intervalForceInput)) {
    bool isPieceVisible = false;
    for (int i = 4; i < 12; ++i) {
      for (int j = 0; j < 8; ++j) {
          if (isTetromino(i, j)) {
              isPieceVisible = true;
          }
      }
    }
    lastForceDownInput = currentMillis;
    // Nu putem face force down pana cand nu se vede piesa pe matrice.
    if(isPieceVisible) {
      forceTetrominoDown();
    }
    score = addScore();
    selectRandomTetromino();
    Serial.print("FORCE PIECE DOWN\n");
  }
  // Dupa orice mutare, actualizam matricea de LED-uri.
  displayGameOnLedMatrix();
}

void moveLeft() {
  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < 8; j++) {
      if (isTetromino(i, j)) {
        // Daca matricea nu se poate muta stanga (suntem deja pe prima coloana sau
        // coloana dinainte e ocupata), nu e o miscare valida.
        if (j == 0 || isNotValidMove(i, j - 1)) {
          Serial.print("INVALID MOVE LEFT\n");
          return;
        }
      }
    }
  }
  for (int i = 0; i < 12; i++) {
    for (int j = 1; j < 8; j++) {
      if (isTetromino(i, j)) {
        gameGrid[i][j - 1] = gameGrid[i][j];
        gameGrid[i][j] = 0;
      }
    }
  }
}

void moveRight() {
  for (int i = 0; i < 12; i++) {
    for (int j = 7; j >= 0; j--) {
      // Daca matricea nu se poate muta dreata (suntem deja pe ultima coloana
      // sau urmatoarea coloana e ocupata), nu e o miscare valida.
      if (isTetromino(i, j)) {
        if (j == 7 || isNotValidMove(i, j + 1)) {
          Serial.print("INVALID MOVE RIGHT\n");
          return;
        }
      }
    }
  }
  for (int i = 0; i < 12; i++) {
    for (int j = 6; j >= 0; j--) {
      if (isTetromino(i, j)) {
        gameGrid[i][j + 1] = gameGrid[i][j];
        gameGrid[i][j] = 0;
      }
    }
  }
}
void forceTetrominoDown() {
  bool canMove = true;
  // Facem move down cat timp se poate.
  while (canMove) {
    canMove = false;
    for (int i = 11; i >= 0; i--) {
      for (int j = 0; j < 8; j++) {
        if (isTetromino(i, j)) {
          // Daca ajungem jos sau daca urmatoarea miscare nu mai e valida,
          // atunci asezam piesa.
          if (i == 11 || isNotValidMove(i + 1, j)) {
            for (int k = 11; k >= 0; k--) {
              for (int l = 0; l < 8; l++) {
                if (isTetromino(k, l)) {
                  gameGrid[k][l] = 3;
                }
              }
            }
            // Daca piesa nu poate fi mutata, iesim din functie.
            return;
          }
        }
      }
    }
    // Mutam efectiv piesa.
    for (int i = 11; i >= 0; i--) {
      for (int j = 0; j < 8; j++) {
        if (isTetromino(i, j)) {
          gameGrid[i + 1][j] = gameGrid[i][j];
          gameGrid[i][j] = 0;
          canMove = true;
        }
      }
    }
  }
}

void moveDown() {
  // Verificam daca toata piesa poate fi mutata in jos.
  for (int i = 11; i >= 0; i--) {
    for (int j = 0; j < 8; j++) {
      if (isTetromino(i, j)) {
        if (i == 11 || isNotValidMove(i + 1, j)) {
          Serial.print("\nINVALID MOVE DOWN\n");
          return;
        }
      }
    }
  }
  for (int i = 11; i >= 0; i--) {
    for (int j = 0; j < 8; j++) {
      if (isTetromino(i , j)) {
        gameGrid[i + 1][j] = gameGrid[i][j];
        gameGrid[i][j] = 0;
      }
    }
  }
}



void rotateLeft(int currentPiece) {
  // Daca piesa e O Tetromino, nu o rotim.
  if(currentPiece == 1)
    return;

  // Daca piesa se afla inainte de partea vizibila a matricei de LED-uri, nu o rotim.
  bool notYetDisplayed = false;
  for (int i = 0; i < 5; ++i) {
      for (int j = 0; j < 8; ++j) {
          if (isTetrominoCenter(i, j)) { 
              notYetDisplayed = true;
          }
      }
  }
  if (notYetDisplayed) 
    return;

  int newGameGrid[12][8] = {0};

  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < 8; j++) {
      // Daca am gasit pivotul pentru rotatie.
      if (isTetrominoCenter(i ,j)) { 
        for (int k = 0; k < 12; k++) {
          for (int l = 0; l < 8; l++) {
            // Pentru bucatile din piesa calculam noul lor index.
            if (isTetrominoPart(k, l)) {
              int newY = l - j + i;
              int newX = i - k + j;
              // Daca noii indecsi depasesc marimea tablei noastre de joc sau 
              // daca se intersecteaza cu randurile deja asezate 
              if (newX < 0 || newX >= 8 || newY < 0 || newY >= 12 || (newY >= 4 && isNotValidMove(newY, newX))) {
                return;
              } 
              newGameGrid[newY][newX] = 1;
            }
          }
        }
        newGameGrid[i][j] = 2; // Noul centru
        break;
      }
    }
  }

  // Actualizează matricea cu piesa rotita.
  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < 8; j++) 
      if (!isNotValidMove(i, j))
      {
        gameGrid[i][j] = newGameGrid[i][j];
      }
  }
}

void updateGame() {
  bool pieceSettled = false;
  for (int i = 11; i >= 0; i--) {
    for (int j = 0; j < 8; j++) {
      if (isTetromino(i ,j)) {
        if (i == 11 || isNotValidMove(i + 1, j)) {
          pieceSettled = true;
        }
      }
    }
  }

  if (pieceSettled) {
    for (int x = 0; x < 12; x++) {
      for (int y = 0; y < 8; y++) {
        if (isTetromino(x, y)) {
          gameGrid[x][y] = 3;
        }
      }
    }
    lastScore = score;
    score = addScore();
    selectRandomTetromino();
  } else {
    moveDown();
  }
  displayGameOnLedMatrix();
}

int addScore() {
  // Salvam randurile care trebuie sterse
  int rowsUpdated = 0;

  for (int i = 0; i < 12; i++) {
    bool isValid = true;
    for (int j = 0; j < 8; j++) {
      if (!isNotValidMove(i, j)) {
        isValid = false;
        break;
      }
    }
    if (isValid) {
      Timer1.stop();
      rowsUpdated++;
      noTone(BUZZER_PIN);
      tone(BUZZER_PIN, 1000, 200);
      displayGameOnLedMatrix();
      delay(1000);
      Timer1.start();
      // Curatam randul.
      for (int j = 0; j < 8; j++) {
        gameGrid[i][j] = 0;
      }
      // Mutam randurile jos.
      for (int k = i; k > 0; k--) {
        for (int l = 0; l < 8; l++) {
          gameGrid[k][l] = gameGrid[k - 1][l];
        }
      }
      // Curatam randul de sus.
      for (int l = 0; l < 8; l++) {
        gameGrid[0][l] = 0;
      }
    }
  }

  // Actualizam scorul in functie de cat de multe randuri facem.
  const int scoreMultipliers[] = {0, 10, 30, 50, 100, 300, 500, 1000};
  int new_score = score;
  if (rowsUpdated >= 1 && rowsUpdated <= 7) {
      new_score += scoreMultipliers[rowsUpdated];
  }

  if(new_score > highscore) {
    highscore = new_score;
    EEPROM.put(address1, highscore);
  }
  return new_score;
}

void selectRandomTetromino() {
  int probabilitate[5] = {100, 100, 100, 100, 100};
  if (pieceId != -1) {
    // Reducem probabilitatea unei piese de a aparea de doua ori
    probabilitate[pieceId] = max(probabilitate[pieceId] - 50, 0);
  }
    // Daca apare o piesa de 2 ori, ii scadem din nou probabilitatea de a aparea.
  if (freqPiece >= 2) {
    probabilitate[pieceId] = max(probabilitate[pieceId] - 30, 0);
  }

  // Calculam suma tuturor probabilitatilor.
  int totalWeight = 0;
  for (int i = 0; i < 5; ++i) {
    totalWeight += probabilitate[i];
  }

  // Alegem o valoare random intre 0 si totalWeight - 1.
  int randomValue = random(totalWeight);
  int sum = 0;
  int selectedPiece = -1;
  // Adunam probabilitatile pieselor una cate una -> construim intervale
  // pentru fiecare piesa pe baza greutatilor. Fiecare interval
  // reprezinta sansa piesei de a fi selectata.
  for (int i = 0; i < 5; ++i) {
    sum += probabilitate[i];
    if (randomValue < sum) {
      selectedPiece = i;
      break;
    }
  }
  int randomPiece = selectedPiece;
  if (pieceId != randomPiece) {
    freqPiece = 0;
  }
  else {
    freqPiece++;
  }
  pieceId = randomPiece;

  // Rotim piesa de maxim 3 ori in mod aleatoriu.
  int randomRotations = random(0, 4);
  
  // Setam piesa pe tabla (initial deasupra a ceea ce e vizibil pe matricea de leduri).
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 8; j++) {
      if (tetrominoes[pieceId][i][j] != 0) {
        gameGrid[i][j] = tetrominoes[pieceId][i][j];
      }
    }
  }

  for (int i = 0; i < randomRotations; i++) {
    rotateLeft(pieceId);
  }
}

// Afisam jocul.
void displayGameOnLedMatrix() {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      lc.setLed(0, i, j, gameGrid[i + 4][j] != 0);
    }
  }
}


void urmatoareaNotaCantecel() {
  if (playSong) {
    int divider = melody[currentNote + 1];
    int noteDuration;
    
    if (divider > 0) {
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; 
    }
    // Acceleram putin melodia originala.
    noteDuration /= 1.33;
    tone(BUZZER_PIN, melody[currentNote], noteDuration * 0.9);
    Timer1.setPeriod(noteDuration * 1.30 * 1000); 
    currentNote += 2;
    if (currentNote >= notes * 2) {
      currentNote = 0; 
    }
  }
}

// Restartam jocul.
void restartGame() {
  Timer1.stop();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Restaring Game...");
  lc.shutdown(0, false);
  lc.setIntensity(0, 1);
  lc.clearDisplay(0);
  // Lasam un semnal luminos timp de 5 sec.
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      lc.setLed(0, i, j, 1);
    }
  }
  delay(5000);
  gameOver = false;
  playSong = true;
  currentNote = 0;
  displayGameOver = false;
  lastScore = -1;
  score = 0;
  pieceId = -1;
  freqPiece = 0;
  memset(gameGrid, 0, sizeof(gameGrid));
  ledOnStartTime = millis();
  selectRandomTetromino();
  Timer1.start();
}