//Time to start commenting lol

//Libraries
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.h>
#include <EEPROM.h>

//Stepper motors
const int motApin = 6;
const int motBpin = 5;
const int dirApin = 8;
const int dirBpin = 7;

//Input pins
const int receiverpin = 11;
const int buttonpin = 2;

//Menu declarations
String mainMenu[] = {
  "Position: ",
  "Input: ",
  "",
  ""
};

//LCD setup
LiquidCrystal_I2C lcd(0x27, 20, 4);

//IR setup
IRrecv irrecv(receiverpin);
decode_results results;

class Page {
  public:
    String lines[24]; //These lines will show something. for the main menu, probably something normal, for a different page, something else
    int cursepos = 0; //The position of the cursor, 0-3 on the screen
    int pagepos = 0;  //The position of the page relative to the menu, starting at 0
    bool horizpos = 0;  //The horizontal position of the cursor, 0 or 1
    void cursemove(bool dir, bool hori){  //This function receives button info and moves cursor on screen
      cursepos += dir * 2 - 1;
      if (hori == 1){horizpos = !horizpos;}
      if (cursepos>3){
        cursepos = 3;
        if (pagepos < 6){
          pagepos++;
        }
      }
      else if (cursepos<0){
        cursepos = 0;
        pagepos--;
        if (pagepos<0){
          pagepos=0;
        }
      }
      uDisplay();
      lcd.setCursor(0,cursepos);
    }
    
    void uDisplay(){  //This updates the display based on lines[] and cursor position
      lcd.clear();
      for(int x=0;x<4;x++){
        lcd.setCursor(0,x);
        lcd.print(lines[x+pagepos]);
      }
      lcd.setCursor(0,cursepos);
    }
    
    int action(){ //This will eventually allow things to happen hopefully
      int numbber;

      numbber = cursepos + pagepos;
      return numbber;
    }
    
};

//Structs for storing programs
struct indivstep{ //This struct holds each individual position and duration
  unsigned short int location;
  byte duration;
};

struct program{ //This holds one of tony's programs, there are 24 program save slots in the EEPROM
  byte progname;
  byte numsteps;
  short int location;
  indivstep stepdata[12];
};

bool used[24] = {0};  //This will tell us which program slots are full, will need a function on initialization to get the list of full partitions

class steppers{ //This class controls the steppers
  public:
    float pos = 0;  //Current position of the motors, will end up storing in EEPROM
    const int divisor = 32; //For different step sizes, if you change it change the switches and the delayMicrosecond values, also get rid of accel/decel
    const int stepsPerUnit[2] = {32512/divisor,12800/divisor};  //Tells steps to move an inch and a centimeter
    const float nudge[2] = {1./32.,0.1};  //smaller movement increments
    bool dire = 1;  //The direction of the steppers, global for use in checkstop
    bool metric = 0;  //English mode = 0, metric = 1
    float presets[8]; //The storage for the 8 presets, will end up storing them permanently

    //selects the direction of travel, I don't know why I did them seperately
    void dir(bool direct){
      digitalWrite(dirApin,!direct);
      digitalWrite(dirBpin,!direct);
      dire = direct;
    }

    //Switches from english to metric and back
    void metricswitch(){
      if (metric == 0)
      {
        pos *= 2.54;
        mainMenu[3] = "METRIC    ";
        metric = 1;
        for(int i = 0;i<8;i++){
          presets[i] *= 2.54;
        }
      }
      else{
        pos /= 2.54;
        mainMenu[3] = "ENGLISH   ";
        metric = 0;
        for(int i = 0;i<8;i++){
          presets[i] /= 2.54;
        }
      }
    }

    //does the moving based off of a destination
    void movee(float destination){
      //Find the distance to travel
      double steps = abs((destination-pos)*stepsPerUnit[metric]);
      delay(5);
    
      //Determine the direction of travel
      if(destination-pos>0){dire = 1;}
      else{dire = 0;}
    
      //Write the direction of travel
      dir(dire);

      ////Serial.println(steps);
      
      
      //Do the traveling
      for(double x=0;x<(steps);x++){
        
        digitalWrite(motApin,HIGH);
        digitalWrite(motBpin,HIGH);

        if (x<10){delayMicroseconds(2000-200*x);} //This accelerates
        if (x>steps-10){delayMicroseconds(2000+(x-steps)*200);} //This decelerates
        delayMicroseconds(700);
    
        digitalWrite(motApin,LOW);
        digitalWrite(motBpin,LOW);
        delayMicroseconds(100);
    
        if (checkstop()){
          if (dire == 1){
            pos += x/stepsPerUnit[metric];
          }
          else{
            pos -= x/stepsPerUnit[metric];
          }
          return;
        }
      }
      //update position
      pos = destination;
    }

    void manual(){  //This one lets you do a whole bunch of things with buttons like move around and enable input
      
      

      lcd.clear();
      
      do{

        ////Serial.println("position is " + String(pos,4));
        
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
            for(float x=0;x<(stepsPerUnit[metric]/100);x++){
            tap('a',1);
            }
            break;
          case 'D':
            //tap back
            for(float x=0;x<(stepsPerUnit[metric]/100);x++){
            tap('a',0);
            }
            break;
          case 'W':
            //tap forward
            for(float x=0;x<(stepsPerUnit[metric]/100);x++){
            tap('b',1);
            }
            break;
          case 'S':
            //tap back
            for(float x=0;x<(stepsPerUnit[metric]/100);x++){
            tap('b',0);
            }
            break;
          case 'b':
            //back 1/32
            movee(pos-nudge[metric]);
            break;
          case 'n':
            //forward 1/32
            movee(pos+nudge[metric]);
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
            type(7,1);
            //Serial.println();
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
          case 'a':
            return;
            break;
         }
      }
      while(1);
    }
    
    void tap(char motor, bool direc){
      dir(direc);
      switch(motor){
        case 'a':
          digitalWrite(motApin,HIGH);
          delayMicroseconds(1000);
          digitalWrite(motApin,LOW);
          delayMicroseconds(1000);
          break;
        case 'b':
          digitalWrite(motBpin,HIGH);
          delayMicroseconds(1000);
          digitalWrite(motBpin,LOW);
          delayMicroseconds(1000);
          break;
      }  
    }
    bool checkstop(){ //returns 1 when mute has been pressed.. could add the stops if necessary now that it's slow
      if(irrecv.decode(&results)){
        unsigned int readResults = results.value;
        irrecv.resume();
        if(results.value ==  1637904527){
          return 1; 
        }
      }
      return 0;
    }

    char getsigSingle(){
      //reads hand button press
        if(!digitalRead(buttonpin)){return 'N';}
        //if a button press is received, put it into readResults
        if(irrecv.decode(&results)){
          unsigned int readResults = results.value;
    
          //Prepare to receive next button press
          irrecv.resume();
          
          delay(2);
    
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
            default:
              return 'X';
          }
        }
      return 'X';
    }
    
    //waits for button and returns button pressed
    char getsig(){
      char value;
      do{
        value = getsigSingle();
        if (value != 'X'){break;}
      }
      while(1);
      return value;
    }
    
    String type(int slot,int line){ //big complicated thing for typing somthing and seeing it live.. you can also go back and change digits
      //declare string (character holder)
      char number[10] = "          ";
      int first = slot;
      lcd.setCursor(slot,line);
      lcd.print("       ");
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
        //Serial.println(String(number));
        //Serial.println("maybe");
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
        EEPROM.put(984,presets[0]);
      }
      else if(typed == 'p'){
        presets[1] = String(number).toFloat();
        EEPROM.put(984+4,presets[1]);
      }
      else if(typed == 'g'){
        presets[2] = String(number).toFloat();
        EEPROM.put(984+8,presets[2]);
      }
      else if(typed == 'k'){
        presets[3] = String(number).toFloat();
        EEPROM.put(984+12,presets[3]);
      }
      else if(typed == 'j'){
        presets[4] = String(number).toFloat();
        EEPROM.put(984+16,presets[4]);
      }
      else if(typed == 'G'){
        presets[5] = String(number).toFloat();
        EEPROM.put(984+20,presets[5]);
      }
      else if(typed == 'h'){
        presets[6] = String(number).toFloat();
        EEPROM.put(984+24,presets[6]);
      }
      else if(typed == 'v'){
        presets[7] = String(number).toFloat();
        EEPROM.put(984+28,presets[7]);
      }
      else{
        //Serial.println("this " + String(number));
        String result = String(number);
        return result;
      }
      return "";
    }
};

class sequencer: public Page, public steppers{
  public:
    short int locations[24] = {
      24,
      64,
      104,
      144,
      184,
      224,
      264,
      304,
      344,
      384,
      424,
      464,
      504,
      544,
      584,
      624,
      664,
      704,
      744,
      784,
      824,
      864,
      904,
      944
    };

    byte stepArray[24]; //containts the number of steps of each program

    byte worknum; //contains the ID of the current working program
    
    program workingprog;  //The struct we're working with
    
    void writeprogtoEE(){ //uses num 0-23
      EEPROM.put(locations[worknum],workingprog);
      EEPROM.put(worknum,1);
    }
    
    void readprog(){ //uses num 0-23
      EEPROM.get(locations[worknum],workingprog);
    }

    void getSteps(){
      for(int x = 0;x<24;x++){
        EEPROM.get(locations[x],workingprog);
        stepArray[x] = workingprog.numsteps;
      }
    }

    void clearslot(){
      workingprog.progname = worknum;
      workingprog.numsteps = 0;
      workingprog.location = locations[worknum];
      for(int x = 0;x<12;x++){
        workingprog.stepdata[x].location = 0;
        workingprog.stepdata[x].duration = 10;
      }
      EEPROM.put(worknum,0);
    }

    void initialize(){
      for(int x = 0;x<8;x++){  //restore shortcuts
         EEPROM.get(984+x*4,presets[x]);
      }

      getSteps();
      
    }

    char programselect(){
      int pagepos = 0;
      bool horizpos = 0;
      int vertipos = 0;
      do{
        //print out the screen
        for(int x = 1;x<5;x++){
          lcd.setCursor(0,x-1);
          lcd.print("Pro: ");
          if(pagepos+x<10){lcd.print("0");}
          lcd.print(pagepos+x);
          lcd.print(" St: ");
          if(stepArray[pagepos+x-1]<10){lcd.print("0");}
          lcd.print(stepArray[pagepos+x-1]);
          lcd.print(" Us Up");
        }

        //put cursor on correct spot
        if (!horizpos){
          lcd.setCursor(15,vertipos);
        }
        else{
          lcd.setCursor(18,vertipos);
        }
  
        //move cursor on horizontals 15 and 18 and around the verticals like normal
        switch (getsig()){
          case 'u':
            vertipos--;
            break;
          case 'd':
            vertipos++;
            break;
          case 'l':
            horizpos = !horizpos;
            break;
          case 'r':
            horizpos = !horizpos;
            break;
          case 'e':
            worknum = vertipos+pagepos; //set working program
            if(!horizpos){
              //go to running program
              readprog();
              return 'f';
            }
            else{
              //go to program editor
              return 'o';
              readprog();
            }
          case 't':
            return 't';
        }
  
        //fix off screen problems
        if (vertipos>3){vertipos = 3; pagepos++;}
        else if (vertipos<0){vertipos = 0; pagepos--;}
        if (pagepos<0){pagepos = 0;}
        else if (pagepos > 20){pagepos = 20;}
  
        lcd.clear();
      }
      while(1);
      
    }

    char navFill(){
      readprog();
      int pagepos = 0;
      bool horizpos = 0;
      int vertipos = 0;
      do{
        //print out the screen

          lcd.clear();
        if(pagepos == 0){
          lcd.setCursor(0,0);
          lcd.print("Prog ");
          lcd.setCursor(5,0);
          lcd.print(worknum+1);
          lcd.setCursor(8,0);
          lcd.print("Steps: ");
          lcd.setCursor(15,0);
          lcd.print(workingprog.numsteps);
        }
        for(int x = 0;x<4;x++){
          if(x+pagepos==0){x++;}
          lcd.setCursor(0,x);
          lcd.print("L: ");
          lcd.setCursor(3,x);
          lcd.print(locationStoreToUse(x+pagepos-1),5);
          lcd.setCursor(11,x);
          lcd.print("Ti: ");
          lcd.setCursor(15,x);
          lcd.print(durationStoreToUse(x+pagepos-1),1);
        }

        //put cursor on correct spot
        if (!horizpos){
          lcd.setCursor(3,vertipos);
        }
        else{
          lcd.setCursor(15,vertipos);
        }
  
        //move cursor on horizontals 15 and 18 and around the verticals like normal
        switch (getsig()){
          case 'u':
            vertipos--;
            break;
          case 'd':
            vertipos++;
            break;
          case 'l':
            horizpos = !horizpos;
            break;
          case 'r':
            horizpos = !horizpos;
            break;
          case 'e':
            if(horizpos){
              if(pagepos+vertipos==0){
                workingprog.numsteps = type(15,0).toInt(); //This should be fine
              }
              else{
                durationTypeToStore(pagepos+vertipos-1,type(15,vertipos).toFloat());
              }
            }
            else{
              locationTypeToStore(pagepos+vertipos-1,type(3,vertipos).toFloat());
            }
            break;
          case 'i':
            writeprogtoEE();
            break;
          case 'f':
            return 'f';
          case 'o':
            return 'o';
          case 'm':
            return 'm';
          case 't':
            return 't';
          
            break;
        }
        
        //fix off screen problems
        if (vertipos>3){vertipos = 3; pagepos++;}
        else if (vertipos<0){vertipos = 0; pagepos--;}
        if (pagepos<0){pagepos = 0;}
        else if (pagepos > 9){pagepos = 9;}
  
      }  
      while(1);
    }

    void durationTypeToStore(int Step, float value){
      if (value>120){value = 120;}
      workingprog.stepdata[Step].duration = String(value * 2).toInt();
    }
    void locationTypeToStore(int Step, float value){
      if (value>30){value=30;} //make sure it won't leave the range by too much
      workingprog.stepdata[Step].location = String(value * 2000).toInt();;
    }
    float durationStoreToUse(int Step){
      float value;
      value = workingprog.stepdata[Step].duration/2.;
      return value;
    }
    float locationStoreToUse(int Step){
      float value;
      value = workingprog.stepdata[Step].location/2000.;
      return value;
    }
    

    char create(){
      bool timed = 0;
      int count = workingprog.numsteps;
      int cycles = 0;
      
      do{

        for (int x = 0 ; x < count ; x++)
        {
          
          //get relevant values
          float locat = locationStoreToUse(x);
          float durat = durationStoreToUse(x);
          
          float locnext = locationStoreToUse(x+1);
          float durnext = durationStoreToUse(x+1);
          
          if(x==(count-1)){
            locnext = locationStoreToUse(0);
            durnext = durationStoreToUse(0);
          }
          
          
          //prints current and next information
          lcd.clear();

          lcd.setCursor(8,0);
          lcd.print("Program ");
          lcd.print(worknum+1);
          
          lcd.setCursor(0,1);
          lcd.print("L: ");
          lcd.setCursor(3,1);
          lcd.print(locat,5);
          lcd.setCursor(11,1);
          lcd.print("Ti: ");
          lcd.setCursor(15,1);
          lcd.print(durat,1);
  
          lcd.setCursor(0,2);
          lcd.print("L: ");
          lcd.setCursor(3,2);
          lcd.print(locnext,5);
          lcd.setCursor(11,2);
          lcd.print("Ti: ");
          lcd.setCursor(15,2);
          lcd.print(durnext,1);

          lcd.setCursor(0,3);
          lcd.print("Cycles: ");
          lcd.print(cycles);
          lcd.setCursor(11,3);
          lcd.print("St: ");
          lcd.print(x+1);
          lcd.print("/");
          lcd.print(count);
          
          movee(locat); //Move to the location
  
          double startTime = millis();  //Start the timer
          int clockupdate = millis();   //This is for the counting clock
          
          do{
            
            //Timer
            if (timed){
              if (millis()-startTime>durat*1000){
                goto nextMove;
              }
            }
            else {
              startTime = millis();
            }
  
            //update time
            lcd.setCursor(0,0);
            lcd.print(durat-(millis()-startTime)/1000.);

            
            //Gets signal from remote or button
            switch (getsigSingle()){
              case 'X':
                //do nothing
                break;
              case 'm':
                //go to sequence page
                return 'm';
                break;
              case 'f':
                //toggle timer
                timed = !timed;
                break;
              case 't':
                //back to manual
                return 't';
                break;
              case 'o':
                //to edit page with current move selected
                return 'o';
                break;
              case 'e':
                goto nextMove;
                break;
              case 'N':
                goto nextMove;
                break;
              case 'U':
                //tap forward
                for(float x=0;x<(stepsPerUnit[metric]/100);x++){
                tap('a',1);
                }
                break;
              case 'D':
                //tap back
                for(float x=0;x<(stepsPerUnit[metric]/100);x++){
                tap('a',0);
                }
                break;
              case 'W':
                //tap forward
                for(float x=0;x<(stepsPerUnit[metric]/100);x++){
                tap('b',1);
                }
                break;
              case 'S':
                //tap back
                for(float x=0;x<(stepsPerUnit[metric]/100);x++){
                tap('b',0);
                }
                break;
              case '-':
                x -= 2;
                if (x<-1){x=count-2; cycles--;}
                goto nextMove;
                break;
              default:
                ;
                break;
            }
          }
          while(1);
          
          nextMove:;
            
        }
        
      cycles++; //increments part counter
      }
      while(1);
    }
    
      
    
};



  
/*
void show(String message){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(message);
  lcd.setCursor(0,4);
  lcd.print("Press any button...");
  getsig();
  lcd.clear();
  return;
}
*/
