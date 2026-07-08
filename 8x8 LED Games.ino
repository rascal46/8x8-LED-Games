#include <LedControl.h>
#include <Wire.h>
#include <U8g2lib.h>

// ===================== OLED =====================
U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0);

// ===================== LED MATRIX =====================
const int DIN = 12, CS = 11, CLK = 10;
LedControl lc = LedControl(DIN, CLK, CS, 1);

// ===================== BUTTONS =====================
#define BTN_UP     3
#define BTN_DOWN   4
#define BTN_LEFT   5
#define BTN_RIGHT  6
#define BTN_SELECT 7

// ===================== GAME STATE =====================
enum GameState { MENU, INTRO, PLAYING, GAME_OVER, PONG, PONG_INTRO };
GameState state = MENU;

// ===================== MENU =====================
const char* menuItems[] = {"Snake", "Pong"};
int menuIndex = 0;

// ===================== TRACK =====================
int lastGame = 0;

// ===================== COMMON =====================
byte pic[8];

// ===================== SNAKE =====================
struct Snake {
  int head[2];
  int body[64][2];
  int len;
  int dir[2];
};

struct Apple {
  int rPos;
  int cPos;
};

Snake snake;
Apple apple;
int score = 0;

// ===================== PONG =====================
#define PADSIZE 3
int xball, yball;
byte direction;
int xpad = 2;
int pongScore = 0;

// ===================== SETUP =====================
void setup() {
  lc.shutdown(0, false);
  lc.setIntensity(0, 8);
  lc.clearDisplay(0);

  u8g2.begin();

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  randomSeed(analogRead(0));
}

// ===================== LOOP =====================
void loop() {
  switch (state) {
    case MENU: menuLoop(); break;
    case INTRO: snakeIntro(); break;
    case PLAYING: snakeLoop(); break;
    case PONG_INTRO: pongIntro(); break;
    case PONG: pongLoop(); break;
    case GAME_OVER: gameOverLoop(); break;
  }
}

// ===================== BUTTON =====================
bool pressed(int pin) {
  if (digitalRead(pin) == LOW) {
    delay(150);
    return true;
  }
  return false;
}

// ===================== MENU =====================
void menuLoop() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB08_tr);

    for (int i = 0; i < 2; i++) {
      int y = 15 + i * 12;
      if (i == menuIndex) u8g2.drawStr(0, y, ">");
      u8g2.drawStr(10, y, menuItems[i]);
    }

  } while (u8g2.nextPage());

  if (pressed(BTN_UP)) menuIndex = (menuIndex + 1) % 2;
  if (pressed(BTN_DOWN)) menuIndex = (menuIndex + 1) % 2;

  if (pressed(BTN_SELECT)) {
    if (menuIndex == 0) {
      lastGame = 1;
      state = INTRO;
    } else {
      lastGame = 2;
      state = PONG_INTRO;
    }
  }
}

// ===================== SNAKE =====================
void snakeIntro() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(10, 25, "SNAKE");
    u8g2.drawStr(10, 50, "Press Select");
  } while (u8g2.nextPage());

  if (pressed(BTN_SELECT)) {
    resetSnake();
    state = PLAYING;
  }
}

void snakeLoop() {
  handleSnakeInput();
  updateSnake();
  render();
  delay(200);

  if (digitalRead(BTN_SELECT) == LOW) {
    state = GAME_OVER;
  }
}

void handleSnakeInput() {
  if (digitalRead(BTN_LEFT) == LOW && snake.dir[1] == 0)
    snake.dir[0]=0, snake.dir[1]=-1;
  if (digitalRead(BTN_RIGHT) == LOW && snake.dir[1] == 0)
    snake.dir[0]=0, snake.dir[1]=1;
  if (digitalRead(BTN_UP) == LOW && snake.dir[0] == 0)
    snake.dir[0]=-1, snake.dir[1]=0;
  if (digitalRead(BTN_DOWN) == LOW && snake.dir[0] == 0)
    snake.dir[0]=1, snake.dir[1]=0;
}

void resetSnake() {
  snake.head[0]=1; snake.head[1]=5;
  snake.body[0][0]=0; snake.body[0][1]=5;
  snake.body[1][0]=1; snake.body[1][1]=5;
  snake.len=2;
  snake.dir[0]=1; snake.dir[1]=0;
  score=0;
  spawnApple();
}

void spawnApple() {
  apple.rPos=random(0,8);
  apple.cPos=random(0,8);
}

void updateSnake() {
  clearMatrix();

  int newHead[2] = {
    snake.head[0]+snake.dir[0],
    snake.head[1]+snake.dir[1]
  };

  if(newHead[0]<0) newHead[0]=7;
  if(newHead[0]>7) newHead[0]=0;
  if(newHead[1]<0) newHead[1]=7;
  if(newHead[1]>7) newHead[1]=0;

  if(newHead[0]==apple.rPos && newHead[1]==apple.cPos){
    snake.len++;
    score++;
    spawnApple();
  } else {
    for(int i=1;i<snake.len;i++){
      snake.body[i-1][0]=snake.body[i][0];
      snake.body[i-1][1]=snake.body[i][1];
    }
  }

  snake.body[snake.len-1][0]=newHead[0];
  snake.body[snake.len-1][1]=newHead[1];
  snake.head[0]=newHead[0];
  snake.head[1]=newHead[1];

  for(int i=0;i<snake.len;i++)
    pic[snake.body[i][0]] |= (128 >> snake.body[i][1]);

  pic[apple.rPos] |= (128 >> apple.cPos);
}

// ===================== PONG =====================
void pongIntro() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(10, 25, "PONG");
    u8g2.drawStr(10, 50, "Press Select");
  } while (u8g2.nextPage());

  if (pressed(BTN_SELECT)) {
    resetPong();
    state = PONG;
  }
}

void pongLoop() {
  if (digitalRead(BTN_LEFT)==LOW && xpad>0) xpad--;
  if (digitalRead(BTN_RIGHT)==LOW && xpad<5) xpad++;

  xball += (direction==0||direction==1||direction==2)?1:(direction>=4&&direction<=6?-1:0);
  yball += (direction>=2&&direction<=4)?1:(direction==6||direction==7||direction==0?-1:0);

  if(xball<=0||xball>=7) direction=(direction+4)%8;
  if(yball<=0) direction=(direction+4)%8;

  if(yball==6){
    if(xball>=xpad && xball<xpad+PADSIZE){
      pongScore++;
      direction=(direction+4)%8;
    } else {
      state=GAME_OVER;
    }
  }

  renderPong();
  delay(80);
}

void renderPong() {
  clearMatrix();

  pic[yball] |= (1 << (7-xball));

  for(int i=0;i<PADSIZE;i++)
    pic[7] |= (1 << (7-(xpad+i)));

  render();
}

void resetPong() {
  xball=random(1,7);
  yball=1;
  direction=random(0,8);
  xpad=2;
  pongScore=0;
}

// ===================== GAME OVER =====================
void gameOverLoop() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB08_tr);

    char buf[20];
    sprintf(buf,"Score:%d", lastGame==1?score:pongScore);
    u8g2.drawStr(10,30,buf);
    u8g2.drawStr(10,50,"Press Select");

  } while (u8g2.nextPage());

  if (pressed(BTN_SELECT)) state = MENU;
}

// ===================== COMMON =====================
void render() {
  for(int i=0;i<8;i++)
    lc.setRow(0,i,pic[i]);
}

void clearMatrix() {
  for(int i=0;i<8;i++)
    pic[i]=0;
}
