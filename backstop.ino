/*This is the program for Mark's press brake backstop

6400 pulses per revolution

 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.h>

// Set the LCD address to 0x27 for four lines of 20 characters
LiquidCrystal_I2C lcd(0x27, 20, 4);

//Stepper motors
const int motApin = 6;
const int motBpin = 5;
const int dirApin = 8;
const int dirBpin = 7;

//Other
float pos = 0;
float next = 0;
const int stepsPerCM = 12800; //multiply by 2.54 for inches
const int stepsPerIN = 32512;
const float metricnudge = 0.1;
const float englishnudge = 1./32.;
float nudge = englishnudge;
int stepSize = 32512;
bool dire = 1;
const int receiverpin = 11;
bool metric = 0;

//IR setup
IRrecv irrecv(receiverpin);
decode_results results;

//Menu declarations
String mainMenu[] = {
  "Position: ",
  "Input: ",
  "",
  ""
};

float seqqs[200] = {};

String seqsMenu[] = {
  "       Sequence :)",
  "Number of steps: "
};

float presets[8] = {};



void setup() {

  //IR setup
  irrecv.enableIRIn();

  //Initiate LCD screen
  lcd.begin();
  lcd.backlight();
  lcd.blink();

  /*Rotary encoder inputs
  pinMode(button,INPUT);
  pinMode(rotA,INPUT);
  pinMode(rotB,INPUT);
  */

  //Pulse pins
  pinMode(motApin,OUTPUT);
  pinMode(motBpin,OUTPUT);

  //dir pins
  pinMode(dirApin,OUTPUT);
  pinMode(dirBpin,OUTPUT);


  //FOR DEBUGGING
  Serial.begin(9600);

}


//Switches from english to metric and back
void metricswitch(){
  if (metric == 0)
  {
    stepSize = stepsPerCM;
    pos *= 2.54;
    mainMenu[3] = "METRIC    ";
    metric = 1;
    nudge = metricnudge;
    for(int i = 0;i<8;i++){
      presets[i] *= 2.54;
    }
  }
  else{
    stepSize = stepsPerIN;
    pos /= 2.54;
    mainMenu[3] = "ENGLISH   ";
    metric = 0;
    nudge = englishnudge;
    for(int i = 0;i<8;i++){
      presets[i] /= 2.54;
    }
  }
  
  
}


//does the moving based off of a destination
void movee(float destination){
  //Find the distance to travel
  double steps = abs((destination-pos)*stepSize);
  delay(5);
  //Serial.println(steps);
  //Determine the direction of travel
  if(destination-pos>0){dire = 1;}
  else{dire = 0;}

  //Write the direction of travel
  dir(dire);

  
  //Do the traveling

  for(double x=0;x<(steps);x++){

    digitalWrite(motApin,HIGH);
    digitalWrite(motBpin,HIGH);
    delayMicroseconds(10);

    digitalWrite(motApin,LOW);
    digitalWrite(motBpin,LOW);
    delayMicroseconds(1);

    if (checkstop()){
      if (dire ==1){
        pos += x/stepSize;
      }
      else{
        pos -= x/stepSize;
      }
      return;
    }
    
  }

  Serial.print("I'm in a movie!!");
  pos = destination;
}

void tap(char motor, bool direc){

  dir(direc);
  
  switch(motor){
    case 'a':
      digitalWrite(motApin,HIGH);
      delayMicroseconds(30);
      digitalWrite(motApin,LOW);
      delayMicroseconds(30);
      break;
    case 'b':
      digitalWrite(motBpin,HIGH);
      delayMicroseconds(30);
      digitalWrite(motBpin,LOW);
      delayMicroseconds(30);
      break;
  }  
}


void loop(){
  
  lcd.clear();
  
  Serial.println("position is " + String(pos,4));

  
  //print menu
  for(int x=0;x<4;x++){
    lcd.setCursor(0,x);
    lcd.print(mainMenu[x]);
  }
  lcd.setCursor(10,0);
  lcd.print(String(pos,5));
  
  switch (getsig()){
    case 'U':
      //tap forward
      for(float x=0;x<(stepSize/64);x++){
      tap('a',1);
      }
      break;
    case 'D':
      //tap back
      for(float x=0;x<(stepSize/64);x++){
      tap('a',0);
      }
      break;
    case 'W':
      //tap forward
      for(float x=0;x<(stepSize/64);x++){
      tap('b',1);
      }
      break;
    case 'S':
      //tap back
      for(float x=0;x<(stepSize/64);x++){
      tap('b',0);
      }
      break;
    case 'b':
      //back 1/32
      movee(pos-nudge);
      break;
    case 'n':
      //forward 1/32
      movee(pos+nudge);
      break;
    case 'z':
      //back 1
      movee(pos-1);
      break;
    case 'y':
      //forward 
      movee(pos+1);
      break;
    case 'f':
      pos = 0;
      break;
    case 'i':
      lcd.setCursor(0,1);
      lcd.print("Input:            ");
      Serial.println(type(7,1));
      break;
    case 't':
      movee(0);
      break;
    case 'o':
      lcd.setCursor(10,0);
      lcd.print("          ");
      pos = type(10,0).toFloat();
      break;
    case 's':
      movee(presets[0]);
      break;
    case 'p':
      movee(presets[1]);
      break;
    case 'g':
      movee(presets[2]);
      break;
    case 'k':
      movee(presets[3]);
      break;
    case 'j':
      movee(presets[4]);
      break;
    case 'G':
      movee(presets[5]);
      break;
    case 'h':
      movee(presets[6]);
      break;
    case 'v':
      movee(presets[7]);
      break;
    case 'm':
      metricswitch();
      break;
      
   }
}

String type(int slot,int line)  {
  //declare string (character holder)
  char number[] = "               ";
  int first = slot;
  //a place to return to to keep typing
  typing:
  //set the cursor in the right place
  lcd.setCursor(slot,line);
  //get a character
  char typed = getsig();
  //is the character part of a location? If yes, add it to the word. If not, see if it's a back or an enter
  if (isDigit(typed) || typed == '.' || typed == '-'){
    lcd.print(typed);  //print to lcd screen
    number[slot-first] = typed;
    delay(1);
    Serial.println(String(number) + " is being made");
    slot ++;
    goto typing;
    }
  if (typed == 'l'){
    slot --;
    goto typing;
    }
  else if(typed == 'r'){
    slot ++;
    goto typing;
    }
  else if(typed == 'i'){
    movee(String(number).toFloat());
  }
  else if(typed == 'w'){
    movee(String(number).toFloat()+pos);
  }
  else if(typed == 'o'){
    pos = String(number).toFloat();
  }
  else if(typed == 's'){
    presets[0] = String(number).toFloat();
  }
  else if(typed == 'p'){
    presets[1] = String(number).toFloat();
  }
  else if(typed == 'g'){
    presets[2] = String(number).toFloat();
  }
  else if(typed == 'k'){
    presets[3] = String(number).toFloat();
  }
  else if(typed == 'j'){
    presets[4] = String(number).toFloat();
  }
  else if(typed == 'G'){
    presets[5] = String(number).toFloat();
  }
  else if(typed == 'h'){
    presets[6] = String(number).toFloat();
  }
  else if(typed == 'v'){
    presets[7] = String(number).toFloat();
  }
  else{
    Serial.println("this " + String(number));
    return number;
  }
  return "";
}

bool checkstop(){
  if(irrecv.decode(&results)){
    unsigned int readResults = results.value;
    irrecv.resume();
    if(results.value ==  1637904527){
      return 1; 
    }
  }
  return 0;
}

//selects the direction of travel
void dir(bool direct){
  digitalWrite(dirApin,direct);
  digitalWrite(dirBpin,direct);
  dire = direct;
}

//gets signal for navigating menus and inputting numbers
char getsig(){
  do{
    if(irrecv.decode(&results)){
      unsigned int readResults = results.value;
      irrecv.resume();
      //Serial.println(results.value);
      delay(20);


      switch(readResults){
        case 1637937167:
          return 'w';
        case 1637922887:
          return 'i';
        case 1637875967:
          return '1';
        case 1637908607:
          return '2';
        case 1637892287:
          return '3';
        case 1637924927:
          return '4';
        case 1637884127:
          return '5';
        case 1637916767:
          return '6';
        case 1637900447:
          return '7';
        case 1637933087:
          return '8';
        case 1637880047:
          return '9';
        case 1637909117:
          return '.';
        case 1637912687:
          return '0';
        case 1637878007:
          return '-';
        case 1637886167:
          return 'm';
        case 1637929517:
          return 'f';
        case 1637892797:
          return 'u';
        case 1637902487:
          return 'l';
        case 1637882087:
          return 'e';
        case 1637918807:
          return 'r';
        case 1637925437:
          return 'd';
        case 1637931047:
          return 't';
        case 1637935127:
          return 'o';
        case 1637923142:
          return 'a';
        case 1637890247:
          return 'c';
        case 1637888207:
          return 'U';
        case 1637920847:
          return 'D';
        case 1637904527:
          return 'M';
        case 1637896367:
          return 'W';
        case 1637929007:
          return 'S';
        case 1637926967:
          return 's';
        case 1637884637:
          return 'p';
        case 1637882342:
          return 'g';
        case 1637913197:
          return 'k';
        case 1637879537:
          return 'j';
        case 1637908862:
          return 'G';
        case 1637912177:
          return 'h';
        case 1637880557:
          return 'v';
        case 1637908097:
          return 'b';
        case 1637924417:
          return 'n';
        case 1637876222:
          return 'z';
        case 1637940737:
          return 'y';
      }
      
      /*
      switch(readResults){
        case 16738455:
          return '0';
        case 16724175:
          return '1';
        case 16718055:
          return '2';
        case 16743045:
          return '3';
        case 16716015:
          return '4';
        case 16726215:
          return '5';
        case 16734885:
          return '6';
        case 16728765:
          return '7';
        case 16730805:
          return '8';
        case 16732845:
          return '9';
        case 16753245:
          return 'x';
        case 16736925:
          return 'w';
        case 16769565:
          return 'f';
        case 16720605:
          return 'l';
        case 16712445:
          return 'c';
        case 16761405:
          return 'r';
        case 16769055:
          return 'd';
        case 16754775:
          return 's';
        case 16748655:
          return 'u';
        case 16750695:
          return '.';
        case 16756815:
          return 'n';
      }
      */
    }
  }
  while(1);
}
