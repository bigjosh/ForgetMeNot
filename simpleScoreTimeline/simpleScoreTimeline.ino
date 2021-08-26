/*
    Simple Score Timeline

    Click a number of times to simulate a score
    Display the score after clicks finish
*/


byte score;
#define PIP_IN_ROUND 6
#define PIP_DURATION_IN_ROUND 100
#define PIP_DURATION_IN_SCORE 500
uint16_t roundDuration = PIP_DURATION_IN_ROUND * PIP_IN_ROUND;
byte currentRound;
byte numberOfRounds;
byte numberOfPips;

uint32_t timeOfGameEnding;
uint32_t timeSinceScoreboardBegan;
bool bDisplayScoreboard = false;

void setup() {
}

void loop() {

  if (buttonLongPressed()) {
    bDisplayScoreboard = false;
  }

  // start a scoreboard with a score of 1
  if (buttonSingleClicked()) {
    score = 1;
    setColor(OFF);  // clear background
    timeOfGameEnding = millis();
    bDisplayScoreboard = true;
  }

  // start a scoreboard with a score of 2
  if (buttonDoubleClicked()) {
    score = 2;
    setColor(OFF);  // clear background
    timeOfGameEnding = millis();
    bDisplayScoreboard = true;
  }

  // start a scoreboard with a score of 3+
  if (buttonMultiClicked()) {
    score = buttonClickCount();
    setColor(OFF);  // clear background
    timeOfGameEnding = millis();
    bDisplayScoreboard = true;
  }


  // Display Scoreboard for scoreboard duration
  if (bDisplayScoreboard) {
    displayScoreboard();
  }
  else {
    setColor(OFF);  // clear background
  }

}


void displayScoreboard() {

  numberOfRounds = score / PIP_IN_ROUND;
  numberOfPips = score % PIP_IN_ROUND;

  timeSinceScoreboardBegan = millis() - timeOfGameEnding;

  currentRound = timeSinceScoreboardBegan / roundDuration;

  displayBackground();
  displayForeground();
}

/*
   Display the build of the rounds completed
*/
void displayBackground() {

  if ( currentRound >= numberOfRounds ) {
    currentRound = numberOfRounds; // cap the rounds at the score
  }

  // display background color on face based on how much time has passed
  FOREACH_FACE(f) {
    uint16_t timeSinceRoundBegan = timeSinceScoreboardBegan - (currentRound * roundDuration);  // time passed in this round
    uint16_t faceTime = f * PIP_DURATION_IN_ROUND; // after this amount of time has passed, draw on this pip

    if ( timeSinceRoundBegan > faceTime ) {

      //      setColorOnFace(RED, f);
      switch (currentRound) {
        case 0: setColorOnFace(dim(RED,200), f); break;
        case 1: setColorOnFace(dim(ORANGE,200), f); break;
        case 2: setColorOnFace(dim(YELLOW,200), f); break;
        case 3: setColorOnFace(dim(GREEN,200), f); break;
        case 4: setColorOnFace(dim(BLUE,200), f); break;
      }
    }
  }

}

/*
   Display the final score on the current round
*/
void displayForeground() {

  //  if( currentRound == numberOfRounds ) { // DO NOT USE - this is still drawing the background, don't want to begin yet

  byte nextRound = numberOfRounds + 1;  // since we are drawing this in our timeline after we painted the background for our current round

  uint16_t timeSincePipStarted = timeSinceScoreboardBegan - (nextRound * roundDuration);  // time passed in this round

  byte currentPip = timeSincePipStarted / PIP_DURATION_IN_SCORE;
  
  if(currentPip >= numberOfPips) {
    currentPip = numberOfPips;
  }

  if (timeSinceScoreboardBegan >= nextRound * roundDuration ) { // begins drawing after all backgrounds have been drawn

    // great, lets draw the pip to its final destination
    FOREACH_FACE(f) {
      uint16_t faceTime = f * PIP_DURATION_IN_SCORE; // after this amount of time has passed, draw on this pip


      if ( timeSincePipStarted > faceTime && f < numberOfPips) {
        // able to display pip
        // if the front pip, pulse
        if ( f == currentPip) {
          // go down and up once every pip duration
          // set the brightness based on the time passed during this pip display duration
          byte bri = sin8_C(map(timeSincePipStarted % PIP_DURATION_IN_SCORE, 0, PIP_DURATION_IN_SCORE, 0, 255)); // time passed in this current pip converted to 0-255
          setColorOnFace(dim(WHITE, bri), f);
        }
        // else stay iluminated
        else {
          setColorOnFace(WHITE, f);
        }
      }
    }
  }
}
