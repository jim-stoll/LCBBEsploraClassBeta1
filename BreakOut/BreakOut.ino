/* EsploraTFT Break Out game by Joel Krueger 12-5-2013
 
 Use the Esplora slider to control the paddle
 Press the left switch momentarily to turn sound on/off
 
 Hardware is an Arduino Esplora http://arduino.cc/en/Main/ArduinoBoardEsplora
 with an Adafruit 1.8" TFT breakout board http://www.adafruit.com/products/358
 This code used the stock arduino libraries so the arduino TFT screen should work fine
 */

#include <Esplora.h>
#include <TFT.h>
#include <SPI.h>

#include "BreakOut.h"

//TODO:
//BUG: ball sometimes get stuck along right-hand edge in auto - just bounces up/down repeatedly
//ENHANCEMENT: add levels, with additional rows of blocks at completion of each level
//ENHANCEMENT: look for mode button during game end display, to allow skipping delay and mode selection screen

//enum paddleModeEnum {JOYSTICK, SLIDER, TILT, AUTO} paddleMode = TILT;
//enum resultEnum {LOSS, WIN};

static const char paddleModeStringJoystick[] = "Joystick";
static const char paddleModeStringSlider[] =   "Slider";
static const char paddleModeStringTilt[] =     "Tilt";
static const char paddleModeStringAuto[] =     "Auto";

const char* modeStrings[] = {paddleModeStringJoystick, paddleModeStringSlider, paddleModeStringTilt, paddleModeStringAuto};

static const char modeLbl[] = "";
static const char scoreLbl[] = "Score:";
static const char livesLbl[] = "x";
static const char loseTxt[] = "GAME OVER";
static const char winTxt[] = "YOU WIN!!!";

typedef struct {
	long initialSpeedDelayMillis;			//initial 'speed' of ball (this is actually a millis delay between ball moves)
	int paddleW;						//paddle width in pixels
	int speedIncreaseHitCount;		//number of ball/paddle hits between progressive speed increases
	int speedIncreaseIncrementMillis; //number of millis by which to increase ball speed at each progressive increase (actually decrease in ball delay)
	int maxSpeedDelayMillis;					//'max' delay millis (actually minimum...), which sets max speed for ball
	int scoreMultiplier;
} modeParamsStruct;

modeParamsStruct modeParams[4];

int screenX = EsploraTFT.width();
int screenY = EsploraTFT.height();
int ballX = screenX / 2;			//ball start position
int ballY = screenY / 2;
int ballLastX = ballX;				//position for erasing the ball
int ballLastY = ballY;
int ballXDir = 1;						 //ball direction across screen
int ballYDir = -2;						//ball direction and speed, up and down screen
int ballW = 5;					 //ball size
int ballXmax = 8;						 //ball max speed left to right
unsigned long time;					 //used for refreshing ball on screen
unsigned long waitUntil;
int ballDelay = 30;					 //milliseconds to wait to refresh ball position
int paddleX = screenX / 2;		//paddle start position
int paddleY = screenY - 5;
int paddleLastX = ballX;			//gives the last paddle position something
const int paddleH = 4;
const int bricksWide = 14;		//number of bricks across the screen
const int bricksTall = 10;		//number of bricks down the screen
const int brickW = 10;				//width of bricks in pixels
const int brickH = 5;				 //highth of brick in pixels
int totalBricks = 0;					//tracks how many bricks are drawn on the screen
int bricksHit = 0;						//track how many bricks have been hit
boolean brick[bricksWide][bricksTall];		//tracks if each individual brick is active
int colBrickCount[bricksWide];
boolean sound = LOW;				 //a flag to turn sound on/off
int ballHits = 0;						//keep track of number of times bal is hit, incrementing ball speed as more hits accrue
const int statusY = 2;
const int modeLblX = 1;
const int livesLblX = 45;
const int scoreLblX = screenX - 57;
const int screenTopY = 10;
const int loseTxtX = 30;
const int loseTxtY = 65;
const int winTxtX = 30;
const int winTxtY = 65;
const int countdownTxtX = screenX/2 - 3;
const int countdownTxtY = 65;

const int modeX = 2;//modeLblX + strlen(modeLbl) * 6 + 2;
const int scoreX = scoreLblX + strlen(scoreLbl) * 6 + 2;
const int livesX = livesLblX + strlen(livesLbl) * 6 + 2;
const int startLives = 3;
int lives = startLives;

int tiltZero = 0;		//offset from level reading, taken when go into tilt mode, to allow for level not reading zero
const int tiltDeadZone = 6;
const int tiltSlowZone = 80;

//int freeRam () {
//  extern int __heap_start, *__brkval;
//  int v;
//  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
//  }
//
//char *getFreeRamAsText() {
//	static char sBuf[11] = {'\0'};
//	sprintf_P(sBuf, PSTR("%d bytes"), freeRam());
//	return sBuf;
//}


void setup(){
//	Serial.begin(115200);
	// initialize the display
	EsploraTFT.begin();
	// set the background the black
	EsploraTFT.background(0,0,0);

	//setup mode parameters
	modeParams[JOYSTICK].initialSpeedDelayMillis = 30;
	modeParams[JOYSTICK].paddleW = 25;
	modeParams[JOYSTICK].speedIncreaseHitCount = 3;
	modeParams[JOYSTICK].speedIncreaseIncrementMillis = 2;
	modeParams[JOYSTICK].maxSpeedDelayMillis = 24;
	modeParams[JOYSTICK].scoreMultiplier = 2;
	modeParams[SLIDER].initialSpeedDelayMillis = 25;
	modeParams[SLIDER].paddleW = 20;
	modeParams[SLIDER].speedIncreaseHitCount = 2;
	modeParams[SLIDER].speedIncreaseIncrementMillis = 2;
	modeParams[SLIDER].maxSpeedDelayMillis = 16;
	modeParams[SLIDER].scoreMultiplier = 1;
	modeParams[TILT].initialSpeedDelayMillis = 50;
	modeParams[TILT].paddleW = 30;
	modeParams[TILT].speedIncreaseHitCount = 2;
	modeParams[TILT].speedIncreaseIncrementMillis = 1;
	modeParams[TILT].maxSpeedDelayMillis = 32;
	modeParams[TILT].scoreMultiplier = 1;
	modeParams[AUTO].initialSpeedDelayMillis = 15;
	modeParams[AUTO].paddleW = 20;
	modeParams[AUTO].speedIncreaseHitCount = 2;
	modeParams[AUTO].speedIncreaseIncrementMillis = 1;
	modeParams[AUTO].maxSpeedDelayMillis = 2;
	modeParams[AUTO].scoreMultiplier = 1;

//	delay(10000);
//	int r = 0;
//	for (int x = 0; x < 100; x++) {
//		r = map(random(3), 0, 2, -1, 1);
//		Serial.println(r);
//	}
//	int c1;
//	int c2;
//for (int x = 0; x < 160; x++) {
//	ballX = x;
//	mapBallToCol(&c1, &c2);
//	Serial.print(x);
//	Serial.print(":");
//	Serial.print(c1);
//	Serial.print(":");
//	Serial.println(c2);
//}
	newGame();							 //routine to redraw a fresh screen
}
void loop(){
	paddle();									//routine to read the slider and draw the paddle
	checkSoundButton();
	//this section tracks the time and only runs at intervals set by initialBallSpeed
	time=millis();
	if (time>waitUntil){			 //when its time to refresh the ball
		waitUntil=time+ballDelay;//add the delay until the next refresh cycle
		moveBall();							//routine to calculate new position and if the ball runs into anything
	}
}

void drawPaddle() {
	EsploraTFT.fill(0,0,0);				//erase the old paddle
	EsploraTFT.rect(paddleLastX,paddleY,modeParams[paddleMode].paddleW,paddleH);
	EsploraTFT.fill(255,255,255);	//draw the new paddle
	EsploraTFT.rect(paddleX,paddleY,modeParams[paddleMode].paddleW,paddleH);

}

void paddle() {
	switch (paddleMode) {
		case JOYSTICK:
			joystickPaddle();
			break;

		case SLIDER:
			sliderPaddle();
			break;

		case TILT:
			tiltPaddle();
			break;

		case AUTO:
			autoPaddle();
			break;
	}

	if (paddleX<1){	//if the paddle tries to go too far left
		paddleX=1;		 //position it on the far left
	}

	if (paddleX>screenX-modeParams[paddleMode].paddleW){	//if the paddle tries to go too far right
		paddleX=screenX-modeParams[paddleMode].paddleW;		 //set the position to the far right - the paddle width
	}

	//this checks to see if the paddle has moved from the last position
	if (paddleX != paddleLastX) {		//if the new position is not equal to the old position
		drawPaddle();
		paddleLastX=paddleX;					 //assign the last posisition ot the new position
	}

}

void sliderPaddle() {
	//read the slider, map it to the screen width, then subtract the witdh of the paddle
	//this gives us the position relative the left corner of the paddle
	paddleX=map(Esplora.readSlider(),0,1023,screenX,0)-modeParams[paddleMode].paddleW/2;
}

void tiltPaddle() {
	int tiltVal = Esplora.readAccelerometer(X_AXIS) - tiltZero;
	int scaledTiltVal = 0;

	if (abs(tiltVal) > tiltDeadZone) {
		if (tiltVal < 0) {
			if (tiltVal < -1*tiltSlowZone) {
				scaledTiltVal = 2;
			} else {
				scaledTiltVal = 1;
			}
		} else {
			if (tiltVal > tiltSlowZone) {
				scaledTiltVal = -2;
			} else {
				scaledTiltVal = -1;
			}
		}
		paddleX = paddleX + scaledTiltVal;
	}
}

int mapBallToCol(int* col1, int* col2) {
	if (ballX >= brickW && ballX <= brickW*bricksWide + brickW - 1) {
		*col1 = ballX/brickW - 1;
		if (ballX >= *col1 * brickW + brickW + ballW + 1) {
			*col2 = *col1 + 1;

			if (*col2 > bricksWide - 1) {
				*col2 = *col1;
			}
		} else {
			*col2 = *col1;
		}
	} else {
		*col1 = -1;
		*col2 = -1;
	}
}

void autoPaddle() {
	//keep the paddle centered under the ball, but randomly move the paddle around by 1/3 paddle width, to prevent
	// getting 'stuck' in one place, in the case of a straight vertical hit
	paddleX = ballX - modeParams[paddleMode].paddleW/2 + ballW/2;

	int col1;
	int col2;
	static int lastPaddleX;

	if (ballY == paddleY - paddleH - 1) {
		mapBallToCol(&col1, &col2);
		if (col1 >= 0) {		//mapBallToCol will return col1 = -1, if ball is outside of block range
//			Serial.print(col1);
//			Serial.print(":");
//			Serial.println(colBrickCount[col1]);
			if (ballXDir == 0 && colBrickCount[col1] == 0 && colBrickCount[col2] == 0) {
//				Serial.print("Empty vertical: ");
//				Serial.print(ballX);
//				Serial.print(":");
//				Serial.print(col1);
//				Serial.print(":");
//				Serial.println(col2);
//				int o = map(random(2), 0, 1, -1, 1) * modeParams[paddleMode].paddleW/3;
//				paddleX += o;
//				Serial.print("offset: ");
//				Serial.println(o);
				paddleX += map(random(2), 0, 1, -1, 1) * modeParams[paddleMode].paddleW/3;
			} else {
				paddleX += map(random(3), 0, 2, -1, 1) * modeParams[paddleMode].paddleW/3;
			}
		}

		if (paddleX == 0) {
			paddleX += modeParams[paddleMode].paddleW/3;
		}

		if (paddleX >= screenX - modeParams[paddleMode].paddleW) {
			//paddleX =-  paddleW/3;
			paddleX = screenX - modeParams[paddleMode].paddleW;
		}
	}
}

void joystickPaddle() {
	paddleX=map(Esplora.readJoystickX(),-512, 512, screenX,0)-modeParams[paddleMode].paddleW/2;
}

void setupNewPaddle() {
	EsploraTFT.fill(0,0,0);				//erase the entire paddle area
	EsploraTFT.rect(0, paddleY, screenX, paddleH);
	ballDelay = modeParams[paddleMode].initialSpeedDelayMillis;
	EsploraTFT.noStroke();
	drawPaddle();

}

void setupNewBall() {
	ballX = random(screenX - ballW);
	ballY = screenTopY + bricksTall * brickH + 10;
	ballLastX = ballX;
	ballLastY = ballY;
	ballXDir = map(random(3), 0, 2, -1, 1);
	ballYDir = abs(ballYDir);
}

void checkSoundButton() {
	if (Esplora.readButton(SWITCH_RIGHT) == LOW) {
		sound = !sound;
		delay(250);
	}
}

bool checkModeButtons(void) {
	if (Esplora.readButton(SWITCH_LEFT) == LOW) {
		paddleMode = JOYSTICK;
		setupNewPaddle();
		delay(250);
		return true;
	}

	if (Esplora.readButton(SWITCH_UP) == LOW) {
		int tiltSum = 0;
		int tiltSamples = 20;
		paddleMode = TILT;
		delay(250);
		for (int x = 0; x < tiltSamples; x++) {
			tiltSum += Esplora.readAccelerometer(X_AXIS);
			delay(50);
		}
		tiltZero = tiltSum/tiltSamples;
		setupNewPaddle();
		delay(250);
		return true;
	}

	if (Esplora.readButton(SWITCH_DOWN) == LOW) {
		paddleMode = SLIDER;
		setupNewPaddle();
		delay(250);
		return true;
	}

	if (Esplora.readButton(SWITCH_RIGHT) == LOW) {
		paddleMode = AUTO;
		setupNewPaddle();
		delay(250);
		return true;
	}

	return false;
}

void gameEnd(enum resultEnum result) {
	if (result == LOSS) {
		if (sound == HIGH){
			Esplora.tone(130,1000);
		}
		for (int i=0; i<75; i++){
			EsploraTFT.stroke(255, 0, 0);
			EsploraTFT.setTextSize(2);
			EsploraTFT.text(loseTxt, loseTxtX, loseTxtY);
			EsploraTFT.stroke(0,0,255);
			EsploraTFT.text(loseTxt, loseTxtX, loseTxtY);
		}
	} else {
		EsploraTFT.fill(0,0,0);
		EsploraTFT.rect(ballX,ballY,ballW,ballW);
		if (sound == HIGH){
			Esplora.tone(400,250);
			delay(250);
			Esplora.tone(415,250);
			delay(250);
			Esplora.tone(430,500);
		}
		for (int i=0; i<10; i++){
			EsploraTFT.stroke(255, 0, 0);
			EsploraTFT.setTextSize(2);
			EsploraTFT.text(winTxt, winTxtX, winTxtY);
			EsploraTFT.stroke(0, 255, 0);
			EsploraTFT.text(winTxt, winTxtX, winTxtY);
			delay(10);
		}
	}


	EsploraTFT.stroke(0,0,0);
	delay(1000);
	newGame();
}

void moveBall(void){
	int oldHits = bricksHit;
	//check if the ball hits the side walls
	if (ballX<ballXDir*-1 | ballX>screenX-ballXDir-ballW){
		ballXDir = -ballXDir;
		if (sound == HIGH){
			Esplora.tone(230,10);
		}
	}
	//check if the ball hits the top of the screen
	if (ballY <= screenTopY){
		//at top of screen ball direction will always be postive (ran into bug trying to just complement the direction)
		ballYDir = abs(ballYDir);
		if (sound == HIGH){
			Esplora.tone(530,10);
		}
	}
	//check if ball hits bottom of bricks
	//we run through the array, if the brick is active, check its position against the balls position
	for (int a=0; a<bricksWide; a++){
		for (int b=0; b<bricksTall; b++){
			if (brick[a][b]==HIGH){
				if (ballX>a*brickW+10-ballW & ballX<a*brickW+brickW+10 & ballY>b*brickH+10-ballW & ballY<b*brickH+brickH+10){
					//we determined that a brick was hit
					EsploraTFT.fill(0,0,0);				//erase the brick
					EsploraTFT.rect(a*brickW+10,b*brickH+10,brickW,brickH);
					brick[a][b]=LOW;							 //set the brick inactive in the array
					ballYDir = -ballYDir;					//change ball direction
					bricksHit=bricksHit+1;				 //add to the bricks hit count
					if (sound == HIGH){
						Esplora.tone(330,10);
					}
					if (colBrickCount[a] > 0) {
						colBrickCount[a]--;				//reduce count of bricks in this column
					}
//					Serial.print("col:ct ");
//					Serial.print(a);
//					Serial.print(":");
//					Serial.println(colBrickCount[a]);
				}
			}
		}
	}

	if (oldHits != bricksHit) {
		showScore();
	}
	//check if the ball hits the paddle
	if (ballX>paddleX-ballXDir-ballW & ballX<paddleX+modeParams[paddleMode].paddleW+ballXDir*-1 & ballY>paddleY-ballYDir-ballW & ballY<paddleY){
		ballXDir = ballXDir-(((paddleX+modeParams[paddleMode].paddleW/2-ballW/2)-ballX)*.3);		//change ball angle in relation to hitting the paddle
		if (ballXDir<-ballXmax){			//this wont allow the ball to go too fast left/right
			ballXDir=-ballXmax;
		}
		else if (ballXDir>ballXmax){	//this wont allow the ball to go too fast left/right
			ballXDir=ballXmax;
		}
		ballYDir = -ballYDir;				 //change direction up/down
		if (sound == HIGH){
			Esplora.tone(730,10);
		}
		ballHits ++;

		if (ballHits % modeParams[paddleMode].speedIncreaseHitCount == 0) {
			ballDelay = ballDelay - modeParams[paddleMode].speedIncreaseIncrementMillis;
			if (ballDelay < modeParams[paddleMode].maxSpeedDelayMillis) {
				ballDelay = modeParams[paddleMode].maxSpeedDelayMillis;
			}
//			if (ballDelay < 1) {
//				ballDelay = 1;
//			}
		}
	}
	//check if the ball went past the paddle
	if (ballY>paddleY+10) {
		if (lives > 1) {
			lives--;
			delay(1000);
			newScreen();
		} else {
			lives--;
			showLives();
			gameEnd(LOSS);
		}
	}
	//check if there are any more bricks
	if (bricksHit==totalBricks){
		gameEnd(WIN);
	}
	//calculate the new position for the ball
	ballX=ballX+ballXDir;	//move the ball x
	ballY=ballY+ballYDir;	//move the ball y
	//erase the old ball
	EsploraTFT.fill(0,0,0);
	EsploraTFT.rect(ballLastX,ballLastY,ballW,ballW);
	// draw the new ball
	EsploraTFT.fill(255,255,255);
	EsploraTFT.rect(ballX, ballY,ballW,ballW);
	//update the last ball position to the new ball position
	ballLastX = ballX;
	ballLastY = ballY;

}

void showLabels() {
	EsploraTFT.stroke(0,255,0);
	EsploraTFT.text(modeLbl, modeLblX, statusY);
	EsploraTFT.text(livesLbl, livesLblX, statusY);
	EsploraTFT.text(scoreLbl, scoreLblX, statusY);
	EsploraTFT.noStroke();

}

void showMode() {
	EsploraTFT.fill(0,0,0);
	EsploraTFT.rect(modeX, statusY, 6*5, 6);
	EsploraTFT.stroke(0,255,0);
	EsploraTFT.text(modeStrings[paddleMode], modeLblX, statusY);
	EsploraTFT.noStroke();

}

void showLives() {
	char sLives[2];
	EsploraTFT.fill(0,0,0);
	EsploraTFT.rect(livesX - 1, statusY, 2*5, 6);
	EsploraTFT.stroke(0,255,0);
	itoa(lives, sLives, 10);
	EsploraTFT.text(sLives, livesX, statusY);
	EsploraTFT.noStroke();

}

void showLevel() {

}

void showScore() {
	char sBricksHit[4];
	EsploraTFT.fill(0,0,0);				//erase the old paddle
	EsploraTFT.rect(scoreX - 1, statusY, 3*5, 6);
	EsploraTFT.stroke(0,255,0);
	itoa(bricksHit * modeParams[paddleMode].scoreMultiplier, sBricksHit, 10);
	EsploraTFT.text(sBricksHit, scoreLblX + 5*7 + 2, statusY);
	EsploraTFT.noStroke();

}

void getMode() {
	EsploraTFT.stroke(0, 255, 0);
	EsploraTFT.textSize(2);
	EsploraTFT.text("Select Mode", 15, 5);
	EsploraTFT.text("1: Slider", 20, 25);
	EsploraTFT.text("2: Joystick", 20, 45);
	EsploraTFT.text("3: Tilt", 20, 65);
	EsploraTFT.text("4: Auto", 20, 85);
	EsploraTFT.textSize(1);
	EsploraTFT.stroke(0, 255, 255);
	EsploraTFT.text("Press 4 during play to", 15, 105);
	EsploraTFT.text("toggle sound", 50, 115);
	EsploraTFT.noStroke();

	while (!checkModeButtons()) {
	}

	EsploraTFT.background(0,0,0);	//set the screen black
	EsploraTFT.stroke(0, 0, 0);

}

void delayWithPaddle(long delayMillis) {
	long start = millis();

	while (millis() - start < delayMillis) {
		paddle();
	}
}

void showCountdown() {
	char secsBuff[2];
	int secs = 3;

	for (secs; secs > 0; secs--) {
		sprintf(secsBuff, "%d", secs);

		EsploraTFT.stroke(0, 0, 255);
		EsploraTFT.textSize(2);
		EsploraTFT.text(secsBuff, countdownTxtX, countdownTxtY);
		EsploraTFT.noStroke();
		delayWithPaddle(1000);
		EsploraTFT.noStroke();
		EsploraTFT.fill(0,0,0);
		EsploraTFT.rect(countdownTxtX, countdownTxtY, 15, 15);
	}

	EsploraTFT.setTextSize(1);

}

void newGame() {					//setup a new game
	EsploraTFT.background(0,0,0);	//set the screen black
	getMode();
	bricksHit = 0;
	lives = startLives;

	setupBlocks();											//routine draws the bricks on the screen

	newScreen();
}

void newScreen(void) {						//setup for next ball of same game
	setupNewBall();
	paddleLastX = ballX;					 //set the last paddle position to something (makes the game draw a new paddle every time)
	paddle();											//routine draws the paddle on the screen
	setupNewPaddle();
	drawPaddle();
	ballHits = 0;
	showLabels();
	showMode();
	showLives();
	showScore();
	showCountdown();

}

void setupBlocks(void){
	//assign the individual bricks to active in an array
	totalBricks = 0;
	for (int x = 0; x < bricksWide; x++) {
		colBrickCount[x] = 0;
	}
	for (int a=0; a<bricksWide; a++){
		for (int b=0; b<bricksTall; b++){
			brick[a][b]=HIGH;
//			if (b < 3 && a > 10) {
//				brick[a][b] = HIGH;
//				colBrickCount[a]++;
//			} else {
//				brick[a][b] = LOW;
//			}
		}
		colBrickCount[a] = bricksTall;
	}
	EsploraTFT.stroke(0, 0, 0);
	//now run trough the array and draw the bricks on the screen
	for (int a=0; a<bricksWide; a++){
		for (int b=0; b<bricksTall; b++){
			int c = map(b,0,bricksWide,0,255);
			if (brick[a][b] == HIGH) {
				totalBricks += 1;
				EsploraTFT.fill(c,255-c/2,255);
				EsploraTFT.rect(a*brickW+10,b*brickH+10,brickW,brickH );
			}
		}
	}
}


