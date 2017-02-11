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

//enum paddleModeEnum {JOYSTICK, SLIDER, TILT} paddleMode = TILT;
//enum resultEnum {LOSS, WIN};

static const char paddleModeStringJoystick[] = "Joystick";
static const char paddleModeStringSlider[] =   "Slider";
static const char paddleModeStringTilt[] =     "Tilt";
static const char paddleModeStringAuto[] =     "Auto";

const char* modeStrings[] = {paddleModeStringJoystick, paddleModeStringSlider, paddleModeStringTilt, paddleModeStringAuto};

static const char modeLbl[] = "Mode:";
static const char scoreLbl[] = "Score:";
static const char loseTxt[] = "GAME OVER";
static const char winTxt[] = "YOU WIN!!!";

typedef struct {
	long initialBallSpeed;			//initial 'speed' of ball (this is actually a millis delay between ball moves)
	int paddleW;						//paddle width in pixels
	int speedIncreaseHitCount;		//number of ball/paddle hits between progressive speed increases
	int speedIncreaseIncrementMillis; //number of millis by which to increase ball speed at each progressive increase (actually decrease in ball delay)
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
int ballRadius = 5;					 //ball size
int ballXmax = 8;						 //ball max speed left to right
unsigned long time;					 //used for refreshing ball on screen
unsigned long waitUntil;
int ballDelay = 30;					 //milliseconds to wait to refresh ball position
int paddleX = screenX / 2;		//paddle start position
int paddleY = screenY - 5;
int paddleLastX = ballX;			//gives the last paddle position something
int paddleW = 20;						 //width of paddle
const int paddleH = 4;
const int bricksWide = 14;		//number of bricks across the screen
const int bricksTall = 10;		//number of bricks down the screen
const int brickW = 10;				//width of bricks in pixels
const int brickH = 5;				 //highth of brick in pixels
int totalBricks = 0;					//tracks how many bricks are drawn on the screen
int bricksHit = 0;						//track how many bricks have been hit
int scoreMultiplier = 1;				//relative point value of brick in different modes
boolean brick[bricksWide][bricksTall];		//tracks if each individual brick is active
boolean sound = LOW;				 //a flag to turn sound on/off
int ballHits = 0;						//keep track of number of times bal is hit, incrementing ball speed as more hits accrue
const int statusY = 2;
const int modeLblX = 1;
const int scoreLblX = screenX - 57;
const int screenTopY = 10;
const int loseTxtX = 30;
const int loseTxtY = 65;
const int winTxtX = 30;
const int winTxtY = 65;
const int countdownTxtX = screenX/2 - 3;
const int countdownTxtY = 65;

const int modeX = modeLblX + strlen(modeLbl) * 6 + 2;
const int scoreX = scoreLblX + strlen(scoreLbl) * 6 + 2;

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
	modeParams[JOYSTICK].initialBallSpeed = 30;
	modeParams[JOYSTICK].paddleW = 25;
	modeParams[JOYSTICK].speedIncreaseHitCount = 3;
	modeParams[JOYSTICK].speedIncreaseIncrementMillis = 2;
	modeParams[JOYSTICK].scoreMultiplier = 2;
	modeParams[SLIDER].initialBallSpeed = 25;
	modeParams[SLIDER].paddleW = 20;
	modeParams[SLIDER].speedIncreaseHitCount = 2;
	modeParams[SLIDER].speedIncreaseIncrementMillis = 2;
	modeParams[SLIDER].scoreMultiplier = 1;
	modeParams[TILT].initialBallSpeed = 50;
	modeParams[TILT].paddleW = 30;
	modeParams[TILT].speedIncreaseHitCount = 2;
	modeParams[TILT].speedIncreaseIncrementMillis = 1;
	modeParams[TILT].scoreMultiplier = 1;
	modeParams[AUTO].initialBallSpeed = 25;
	modeParams[AUTO].paddleW = 20;
	modeParams[AUTO].speedIncreaseHitCount = 2;
	modeParams[AUTO].speedIncreaseIncrementMillis = 2;
	modeParams[AUTO].scoreMultiplier = 1;

	newScreen();							 //routine to redraw a fresh screen
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
	EsploraTFT.rect(paddleLastX,paddleY,paddleW,paddleH);
	EsploraTFT.fill(255,255,255);	//draw the new paddle
	EsploraTFT.rect(paddleX,paddleY,paddleW,paddleH);

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

	if (paddleX>screenX-paddleW){	//if the paddle tries to go too far right
		paddleX=screenX-paddleW;		 //set the position to the far right - the paddle width
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
	paddleX=map(Esplora.readSlider(),0,1023,screenX,0)-paddleW/2;
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

void autoPaddle() {
	//keep the paddle under the ball, but randomly move the paddle around by 1/3 paddle width, to prevent
	// getting 'stuck' in one place, in the case of a straight vertical hit
	paddleX = ballX - paddleW/2;

	if (ballY == paddleY - paddleH - 1) {
		paddleX += map(random(3), 0, 2, -1, 1) * paddleW/3;
	}

}

void joystickPaddle() {
	paddleX=map(Esplora.readJoystickX(),-512, 512, screenX,0)-paddleW/2;
}

void setupNewPaddle() {
	EsploraTFT.fill(0,0,0);				//erase the entire paddle area
	EsploraTFT.rect(0, paddleY, screenX, paddleH);
	ballDelay = modeParams[paddleMode].initialBallSpeed;
	paddleW = modeParams[paddleMode].paddleW;
	scoreMultiplier = modeParams[paddleMode].scoreMultiplier;
	EsploraTFT.noStroke();
	drawPaddle();

}

void setupNewBall() {
	ballX = random(screenX - ballRadius);
	ballY = screenTopY + bricksTall * brickH + 10;
	ballLastX = ballX;
	ballLastY = ballY;
	ballXDir = map(random(3), 0, 2, -1, 1);
	ballYDir = abs(ballYDir);
	bricksHit = 0;
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
		EsploraTFT.rect(ballX,ballY,ballRadius,ballRadius);
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
	newScreen();
}

void moveBall(void){
	int oldHits = bricksHit;
	//check if the ball hits the side walls
	if (ballX<ballXDir*-1 | ballX>screenX-ballXDir-ballRadius){
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
				if (ballX>a*brickW+10-ballRadius & ballX<a*brickW+brickW+10 & ballY>b*brickH+10-ballRadius & ballY<b*brickH+brickH+10){
					//we determined that a brick was hit
					EsploraTFT.fill(0,0,0);				//erase the brick
					EsploraTFT.rect(a*brickW+10,b*brickH+10,brickW,brickH);
					brick[a][b]=LOW;							 //set the brick inactive in the array
					ballYDir = -ballYDir;					//change ball direction
					bricksHit=bricksHit+1;				 //add to the bricks hit count
					if (sound == HIGH){
						Esplora.tone(330,10);
					}
				}
			}
		}
	}

	if (oldHits != bricksHit) {
		showScore();
	}
	//check if the ball hits the paddle
	if (ballX>paddleX-ballXDir-ballRadius & ballX<paddleX+paddleW+ballXDir*-1 & ballY>paddleY-ballYDir-ballRadius & ballY<paddleY){
		ballXDir = ballXDir-(((paddleX+paddleW/2-ballRadius/2)-ballX)*.3);		//change ball angle in relation to hitting the paddle
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
			if (ballDelay < 1) {
				ballDelay = 1;
			}
		}
	}
	//check if the ball went past the paddle
	if (ballY>paddleY+10){
		gameEnd(LOSS);
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
	EsploraTFT.rect(ballLastX,ballLastY,ballRadius,ballRadius);
	// draw the new ball
	EsploraTFT.fill(255,255,255);
	EsploraTFT.rect(ballX, ballY,ballRadius,ballRadius);
	//update the last ball position to the new ball position
	ballLastX = ballX;
	ballLastY = ballY;

}

void showLabels() {
	EsploraTFT.stroke(0,255,0);
	EsploraTFT.text(modeLbl, modeLblX, statusY);
	EsploraTFT.text(scoreLbl, scoreLblX, statusY);
	EsploraTFT.noStroke();

}

void showMode() {
	EsploraTFT.fill(0,0,0);				//erase the old paddle
	EsploraTFT.rect(modeX, statusY, modeX + 6*5, statusY + 6);
	EsploraTFT.stroke(0,255,0);
	EsploraTFT.text(modeStrings[paddleMode], modeLblX + 5*6 + 2, statusY);
	EsploraTFT.noStroke();

}

void showScore() {
	char sBricksHit[4];
	EsploraTFT.fill(0,0,0);				//erase the old paddle
	EsploraTFT.rect(scoreX - 1, statusY, scoreX + 3*5, statusY + 6);
	EsploraTFT.stroke(0,255,0);
	itoa(bricksHit * scoreMultiplier, sBricksHit, 10);
	EsploraTFT.text(sBricksHit, scoreLblX + 5*7 + 2, statusY);
	EsploraTFT.noStroke();

}

void getMode() {
	EsploraTFT.stroke(0, 255, 0);
	EsploraTFT.textSize(2);
	EsploraTFT.text("Select Mode", 10, 10);
	EsploraTFT.text("1: Slider", 25, 30);
	EsploraTFT.text("2: Joystick", 25, 50);
	EsploraTFT.text("3: Tilt", 25, 70);
	EsploraTFT.text("4: Auto", 25, 90);
	EsploraTFT.textSize(1);
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

void newScreen(void) {						//this is the setup for clearing the screen for a new game
	EsploraTFT.background(0,0,0);	//set the screen black
	getMode();
	setupNewBall();
	paddleLastX = ballX;					 //set the last paddle position to something (makes the game draw a new paddle every time)
	paddle();											//routine draws the paddle on the screen
	setupNewPaddle();
	drawPaddle();
	blocks();											//routine draws the bricks on the screen
	ballHits = 0;
	showLabels();
	showMode();
	showScore();
	showCountdown();

}

void blocks(void){
	//assign the individual bricks to active in an array
	totalBricks = 0;
	for (int a=0; a<bricksWide; a++){
		for (int b=0; b<bricksTall; b++){
			brick[a][b]=HIGH;
		}
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


