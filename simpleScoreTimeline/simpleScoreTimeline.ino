/*
    Blinks Simulator by Move38

    Find out more about this project:
    https://github.com/Move38/Blinks-Simulator
    Click and drag blinks to re-arrange them.
    Click and drag in an open space to break them.
    You can use console for debugging info
*/


byte score;
#define PIP_IN_ROUND 6
#define PIP_DURATION_IN_ROUND 100
#define PIP_DURATION_IN_SCORE 500
uint16_t roundDuration = PIP_DURATION_IN_ROUND * PIP_IN_ROUND;
byte currentRound;
byte numberOfRounds;
byte numberOfPips;

Timer scoreboardTimer;
#define DURATION 20000
uint16_t timeSinceScoreboard;

void setup() {
}

void loop() {

  // start a scoreboard with a score of 1
  if (buttonSingleClicked()) {
    score = 1;
    setColor(OFF);  // clear background
    scoreboardTimer.set(DURATION);
  }

  // start a scoreboard with a score of 2
  if (buttonDoubleClicked()) {
    score = 2;
    setColor(OFF);  // clear background
    scoreboardTimer.set(DURATION);
  }

  // start a scoreboard with a score of 3+
  if (buttonMultiClicked()) {
    score = buttonClickCount();
    setColor(OFF);  // clear background
    scoreboardTimer.set(DURATION);
  }

  
  // Display Scoreboard for scoreboard duration
  if (!scoreboardTimer.isExpired()) {
    displayScoreboard();
  }
  else {
    setColor(OFF);  // clear background
  }

}


void displayScoreboard() {

  numberOfRounds = score / PIP_IN_ROUND;
  numberOfPips = score % PIP_IN_ROUND;
  
  currentRound = timeSinceScoreboard / roundDuration;

  timeSinceScoreboard = DURATION - scoreboardTimer.getRemaining();

  displayBackground();
  displayForeground();
}

void displayBackground() {

  if ( currentRound >= numberOfRounds ) {
    currentRound = numberOfRounds; // cap the rounds at the score
  }

  // display background color on face based on how much time has passed
  FOREACH_FACE(f) {
    uint16_t timeSinceRoundBegan = timeSinceScoreboard - (currentRound * roundDuration);  // time passed in this round
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

void displayForeground() {

  //  if( currentRound == numberOfRounds ) { // DO NOT USE - this is still drawing the background, don't want to begin yet
  
  byte nextRound = numberOfRounds + 1;  // since we are drawing this in our timeline after we painted the background for our current round

  if (timeSinceScoreboard >= nextRound * roundDuration ) { // begins drawing after all backgrounds have been drawn

    // great, lets draw the pip to its final destination
    FOREACH_FACE(f) {
      uint16_t timeSincePipStarted = timeSinceScoreboard - (nextRound * roundDuration);  // time passed in this round
      uint16_t faceTime = f * PIP_DURATION_IN_SCORE; // after this amount of time has passed, draw on this pip
      

      if( timeSincePipStarted > faceTime && f < numberOfPips) {
        // able to display pip
        setColorOnFace(WHITE,f);
      }
    }
  }
}
