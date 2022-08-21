#include "pagetest.h"

//Declare the machine Tony's
  steppers tonysMachine;
  sequencer maybe;

void setup() {

  //FOR DEBUGGING
  //Serial.begin(9600);

  //IR setup
  irrecv.enableIRIn();

  //Initiate LCD screen
  lcd.begin();
  //lcd.backlight();
  lcd.blink();

  //Pulse pins
  pinMode(motApin,OUTPUT);
  pinMode(motBpin,OUTPUT);

  //dir pins
  pinMode(dirApin,OUTPUT);
  pinMode(dirBpin,OUTPUT);

  //button setup
  pinMode(buttonpin,INPUT_PULLUP);

  //initialize program thing
  maybe.initialize();
  
}

void loop(){
  char switcher = 't';
  do{

    switch (switcher){
      case 'm':
        switcher = maybe.programselect();
        break;
      case 'f':
        switcher = maybe.create();
        break;
      case 'o':
        switcher = maybe.navFill();
        break;
      case 't':
        maybe.manual();
        switcher = 'm';
        break;
    }
  }
  while(1);
}
