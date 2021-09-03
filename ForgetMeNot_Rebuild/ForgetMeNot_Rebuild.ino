/*
   Forget-me-not

   by Jeph Stahl
   Additional Development by Move38

*/

enum pieceTypes {
  CENTER,
  PETAL
};

byte pieceType = PETAL; // default to PETAL
bool bPossibleCenter = false;

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

  switch (pieceType) {

    case CENTER:
      centerLoop();
      centerDisplay();
      break;

    case PETAL:
      petalLoop();
      petalDisplay();
      break;

    default:
      break;
  }

}


/*
   Center update loop
*/
void centerLoop() {

  // basic reset from the center
  if(buttonLongPressed()) {
    pieceType = PETAL;
  }
}

/*
   Center display loop
*/

void centerDisplay() {
  setColor(ORANGE); // show we are the center piece
}

/*
   Petal update loop
*/

void petalLoop() {

  if(isCenterPossible()) {
    if(buttonSingleClicked()) {
      pieceType = CENTER;
    }
  }

}

/*
   Petal display loop
*/
void petalDisplay() {

  setColor(GREEN);  // by default, let's color petals green

  if(isCenterPossible()) {
    setColor(YELLOW); // show we are possible center pieces
  }
}

/*
 * returns true if this Blink is positioned and ready to be a center
 */
bool isCenterPossible() {
    
  FOREACH_FACE(f) {
    if(isValueReceivedOnFaceExpired(f)) {
      return false; // missing neighbor, this is not a possible center
    }
  }

  return true;  // we looped through all neighbors without returning, great, we are a possible center
}
