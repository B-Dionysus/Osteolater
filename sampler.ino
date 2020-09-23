// TO DO:
// Document
// Fix random start time
// Seed random
// Fix random file?
// Add support for trigger in
// Add support for clock in
// make off button turn volume to 0
// make on button turn volume to previous level


// include SPI, MP3 and SD libraries
#include <SPI.h>
#include "myFilePlayer.h"
#include <SD.h>
 #include <ClickEncoder.h>
#include <TimerOne.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

// These are the pins used for the breakout example
#define BREAKOUT_RESET  9      // VS1053 reset pin (output)
#define BREAKOUT_CS     10     // VS1053 chip select pin (output)
#define BREAKOUT_DCS    8      // VS1053 Data/command select pin (output)
// These are the pins used for the music maker shield
#define SHIELD_RESET  -1      // VS1053 reset pin (unused!)
#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)

// These are common pins between breakout and shield
#define CARDCS 4     // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer = 
  // create breakout-example object!
  //Adafruit_VS1053_FilePlayer(BREAKOUT_RESET, BREAKOUT_CS, BREAKOUT_DCS, DREQ, CARDCS);
  // create shield-example object!
  Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

////

#define MAXVOL 0
#define MINVOL 255
int currentVol=20;

#define DARKMODE_LIGHT 0
#define DARKMODE_DARK 1
#define DARKMODE_LED_OFF 2

int bpm = 120;
int tickStart=0;
int beatsSincePlay=0;
int beatFreq=4;       // How many beats between samples
int sampleLength=5000; // How long to play a sample, in milliseconds
double samplePos=0;
int playTime=0; // How long has the sample been playing?
int sampleStartTime=0;
int newClipChance=100;  // How likely is it (0 to 100) that the next time it plays a sample it will be different
int darkMode=DARKMODE_LIGHT;
int currentDirNum=0;
int totalDirs=0;
char directoryNames[20][10];
int tempDir;
int temp=0;
unsigned long currentFileSize=0;

File currentSample;
File currentDir;
#define BPMLED 1
#define SAMPLED 2


#define REDLED 22
#define BLUELED 24
#define LEDON 100       // How long should the LEDS blink 
int LEDBLINK=1;

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

/************************************************************************************************************************************
 *                                                                                                       Encoder initilization biz
************************************************************************************************************************************/

int16_t encPos1=beatFreq;
int16_t encPos2=sampleLength;
int16_t encPos3=newClipChance;
int16_t encPos4=currentVol;  
int16_t encPos5;

int16_t oldEncPos1=encPos1;
int16_t oldEncPos2=encPos2;
int16_t oldEncPos3=encPos3;
int16_t oldEncPos4=encPos4;
int16_t oldEncPos5=encPos5;

uint8_t buttonState1;
uint8_t buttonState2;
uint8_t buttonState3;
uint8_t buttonState4;
uint8_t buttonState5;

#define pinFREQA A6
#define pinFREQB A7
#define pinFREQBUTTON 53

#define pinLENA A8
#define pinLENB A9
#define pinLENBUTTON 40

#define pinRANDA A10
#define pinRANDB A11
#define pinRANDBUTTON 44

#define pinVOLA A12
#define pinVOLB A13
#define pinVOLBUTTON 51

#define pinDIRA A14
#define pinDIRB A15
#define pinDIRBUTTON 48

#define STEPS 4
#define BPMMODE 1
#define VOLMODE 2
int encoder4Mode=VOLMODE;




ClickEncoder encoder1(pinFREQA, pinFREQB, pinFREQBUTTON, STEPS);
ClickEncoder encoder2(pinLENA, pinLENB, pinLENBUTTON, STEPS);
ClickEncoder encoder3(pinRANDA, pinRANDB, pinRANDBUTTON, STEPS);
ClickEncoder encoder4(pinVOLA, pinVOLB, pinVOLBUTTON, STEPS);
ClickEncoder encoder5(pinDIRA, pinDIRB, pinDIRBUTTON, STEPS);

void timerIsr() {
  encoder2.service();
  encoder1.service();
  encoder3.service();
  encoder4.service();
  encoder5.service();
}

/************************************************************************************************************************************
 *                                                                                                                    loadDir()
************************************************************************************************************************************/

File loadDir(char* dir){
  Serial.println(dir);
  File myFile;
  myFile=SD.open(dir);
  currentDir=myFile;
  Serial.print("Directory: ");Serial.println(myFile.name());
  chooseNewSample(100);

}
/************************************************************************************************************************************
 *                                                                                                                    loadSample()
************************************************************************************************************************************/

void loadSample(char* sampleName){
  File myFile;
  char fullSamplePath[50];
  sprintf(fullSamplePath, "/%s/%s",currentDir.name(),sampleName);
      Serial.print("Name 1: ");Serial.println(fullSamplePath);
      
  myFile=SD.open(fullSamplePath);
  
  //    Serial.print("Name: ");Serial.println(myFile.name());
  if(! myFile){
    Serial.println("Can't open file");
  }
  else{
    currentSample=myFile;
  }
}
/************************************************************************************************************************************
 *                                                                                                                    setup()
************************************************************************************************************************************/

void setup() {
  Serial.begin(9600);
   randomSeed(analogRead(0));
  // initialise the music player
  if (! musicPlayer.begin()) { // initialise the music player
     Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
     while (1);
  }
  Serial.println(F("VS1053 found"));
 
  if (!SD.begin(CARDCS)) {
    Serial.println(F("SD failed, or not present"));
    while (1);  // don't do anything more
  }
  Serial.println("SD OK!");
  
  totalDirs=catalogDirectory();
  loadDir(directoryNames[random(totalDirs)]);
  
  chooseNewSample(newClipChance);
  
  pinMode(REDLED, OUTPUT);
  pinMode(BLUELED, OUTPUT);

  pinMode(32, INPUT);
  pinMode(34, INPUT);
  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(currentVol, currentVol);

  if (! musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT))
    Serial.println(F("DREQ pin is not an interrupt pin"));
  
  
  // SCREEN SET UP
    // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  
  display.clearDisplay();
   // text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print("0x"); display.println(0xDEADBEEF, HEX);
  
//  display.setCursor(0,0);
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.print("HELLO!");
  if(darkMode!=DARKMODE_DARK) display.display();
  delay(2000);
  
  // Encoder setup
  
  encoder2.setAccelerationEnabled(true);
    Serial.println((encoder2.getAccelerationEnabled()) ? "Encoder 2 accelerator is enabled" : "Encoder 2 accelerator is disabled");


  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr);
}
/************************************************************************************************************************************
 *                                                                                                                    catalog Directory!
 ************************************************************************************************************************************/
int catalogDirectory() {
  File dir=SD.open("/");
  delay(1000);
  Serial.println("Ok to proceed, sorry for the delay.");
  int totalDirs=0;
  
  dir.rewindDirectory();
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      return totalDirs;    
      break;
    }
    if (entry.isDirectory()) {
      char buf[4];
      buf[0]=entry.name()[0];
      buf[1]=entry.name()[1];
      buf[2]=entry.name()[2];
      buf[3]=entry.name()[3];
      buf[4]=NULL;
      if(strcmp(buf,"AUD_")==0){
        strcpy(directoryNames[totalDirs],entry.name());
      //  Serial.print("Adding this-->");
      //  Serial.println(directoryNames[totalDirs]);
        totalDirs++;
      }   
    }
    entry.close();
  }
  return totalDirs;
}
/************************************************************************************************************************************
 *                                                                                                                    chooseNewSample()
************************************************************************************************************************************/
void chooseNewSample(int chance) {
  Serial.print("New File from");
  Serial.println(currentDir.name());
  //Do we choose a different sample?
  int r=random(100); 
  //Serial.print("R: "); Serial.print(r);  Serial.print(" C: "); Serial.println(chance);
  if(r<chance){
    // Pick a random file from the current folder
    File entry;
    int count = 0;
    currentDir.rewindDirectory();
    while ( entry = currentDir.openNextFile() ) {
      entry.close();
      count++;
    }
    int fileNum=random(count);
    count=0;
    currentDir.rewindDirectory();
    while(entry = currentDir.openNextFile()){
      if(count==fileNum){
        Serial.print("File: ");
        Serial.println(entry.name());
        //loadSample(entry.name());
        currentSample=entry;            
        currentFileSize=currentSample.size();             
      }

      entry.close();
      count++;
    }
  }
  playSample(chance);
}
/************************************************************************************************************************************
 *                                                                                                                    playSample()
************************************************************************************************************************************/
void playSample(int chance){
  Serial.println("playing...");
  char fullSamplePath[50];

  Serial.print(currentDir.name()); Serial.println(currentSample.name());
  sprintf(fullSamplePath, "%s/%s",currentDir.name(),currentSample.name()); 
//  Serial.print("Full path: "); Serial.println(fullSamplePath);
 
  // If file is longer than a few seconds, start from a random spot 
  // (unless were starting from the same spot as last time, of course, due to rolling over newClipChance)
  if(currentFileSize>100000){
    int r=random(100);
   // Serial.print("R: "); Serial.print(r);  Serial.print(" C: "); Serial.println(chance);
    if(r<chance)
      samplePos=random(currentFileSize*.9);       //Pick a random position to start from. But don't go too far.
  }
  if(!musicPlayer.playingMusic){

    if (! musicPlayer.startPlayingFile(fullSamplePath, samplePos)) {
      display.clearDisplay();
      display.setCursor(0,0);
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.println("Error");
      
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.print(fullSamplePath);
      if(darkMode!=DARKMODE_DARK) display.display();
    
      Serial.print("Could not open file.");
      Serial.println(fullSamplePath); 
      loadDir(directoryNames[0]);
    }
  }
} 

/************************************************************************************************************************************
 *                                                                                                                    playTick()
************************************************************************************************************************************/

void playTick(int currentTime){
  int change=currentTime-tickStart; // change slowly counts up to millisecondsPerBeat, e.g., 500, and is reset each beat 

  if(change>LEDON){
     digitalWrite(REDLED, LOW);     
     digitalWrite(BLUELED, LOW);     
  }
  // Should we stop the music?
  if(musicPlayer.playingMusic){
    playTime=currentTime-sampleStartTime; // How long, in milliseconds, has the sample been playing?
    
    if(playTime>sampleLength){
      musicPlayer.stopPlaying(); 
 //      Serial.println("Stop the music!!");
      playTime=0;
    }
  }
  
  float millisecondsPerBeat=(60.0/bpm)*1000; // For example, at 120 bpm this would be 60/120*1000=500ms per beat, two beats a second
    
  if(change>(millisecondsPerBeat)) {
    tickStart=currentTime;
    // REGULAR Beat---------------------------------------------------------------------------------
    if(LEDBLINK && !darkMode) digitalWrite(BLUELED, HIGH);  
    beatsSincePlay++;
    if(beatsSincePlay>=beatFreq){
      beatsSincePlay=0;
      // PLAY THAT SAMPLE!!!!!--------------------------------------------------------------------
      if(!musicPlayer.playingMusic){
        if(LEDBLINK && !darkMode) digitalWrite(REDLED, HIGH);  
        chooseNewSample(newClipChance);
        playSample(newClipChance);
        sampleStartTime=currentTime;
        playTime=0;
      }
     // toggleLED(SAMPLED, 1);      
    }
  }
}

 
/****************************************************************
 * Here it is, the main loop!!!!
 * **************************************************************/
void loop(){
  playTick(millis());
      
//  Serial.print(digitalRead(32));
//  Serial.print(digitalRead(34));
//  Serial.print(analogRead(32));
//  Serial.print(analogRead(34));
/************************************************************************************************************************************
 *                                                                                                                    rotary encoders!
 ************************************************************************************************************************************/
  encPos1 += encoder1.getValue();
  encPos2 += encoder2.getValue(); 
  encPos3 += encoder3.getValue();
  
  if(encoder4Mode==VOLMODE)
    encPos4 -= encoder4.getValue();// On account of the volume control is 0=high, 255=low
  else encPos4 += encoder4.getValue();
  
  encPos5 += encoder5.getValue();

  if (encPos1 != oldEncPos1) {
  
    if(encPos1<1) encPos1=1;
    beatFreq=encPos1;
    oldEncPos1 = encPos1;
    //Serial.print("Encoder F Value: ");    Serial.println(encPos1);
    
        
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.println("Frequency");
    
    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.print(encPos1);
    display.setTextSize(1);
    display.print("beats");
    if(darkMode!=DARKMODE_DARK) display.display();
  }

  if (encPos2 != oldEncPos2) {
    int jump=encPos2-oldEncPos2;
    jump*=100;
    encPos2+=jump;
    if(encPos2<100) encPos2=100;
    oldEncPos2 = encPos2;
    
    sampleLength=encPos2;
    
    //Serial.print("Encoder L Value: ");    Serial.println(encPos2);
    
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.println("Clip Lngth");
    
    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.print(encPos2);
    display.setTextSize(1);
    display.print("ms");
    if(darkMode!=DARKMODE_DARK)   display.display();
    //Serial.println(encPos2);
  }

  if (encPos3 != oldEncPos3) {
    if(encPos3<0) encPos3=0;
    else if(encPos3>100) encPos3=100;
    newClipChance=encPos3;
    oldEncPos3 = encPos3;
    //Serial.print("Encoder R Value: ");    Serial.println(encPos3);
   
        
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.println("New Clip?");
    
    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.print(encPos3);
    display.setTextSize(1);
    display.print("%");
    if(darkMode!=DARKMODE_DARK) display.display();
  }

  if (encPos4 != oldEncPos4) {
    oldEncPos4 = encPos4;
    if(encoder4Mode==VOLMODE){

      if(encPos4>255) encPos4=255;
      else if(encPos4<0) encPos4=0;
      currentVol=encPos4;
      musicPlayer.setVolume(currentVol, currentVol);
      
      int volPer=((255-encPos4)*100)/255;   // So at 0, this would be 100% and at 255 this would be 0
      
      display.clearDisplay();
      display.setCursor(0,0);
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.println("Set Volume");
      
      display.setTextSize(3);
      display.setTextColor(WHITE);
      display.print(volPer);
      display.print("%");
      if(darkMode!=DARKMODE_DARK) display.display();
     // Serial.println(encPos4);
    }
    else if(encoder4Mode==BPMMODE){
      if(encPos4<0) encPos4=0;
      oldEncPos4 = encPos4;
      bpm=encPos4;
      
      display.clearDisplay();
      display.setCursor(0,0);
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.println("Set BPM");
      
      display.setTextSize(3);
      display.setTextColor(WHITE);
      display.print(bpm);
      display.setTextSize(1);
      display.print("bpm");
      if(darkMode!=DARKMODE_DARK) display.display();
     //Serial.println(encPos4);
    }
  }

  if (encPos5 != oldEncPos5) {
    if(encPos5>=totalDirs) encPos5=0;
    else if(encPos5<0) encPos5=totalDirs-1;
    oldEncPos5=encPos5;
    
    tempDir=encPos5;

    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.println("Directory");
    
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.print(directoryNames[encPos5]);
    if(darkMode!=DARKMODE_DARK) display.display();
    
  } 
  /************************************************************************************************************************************
 *                                                                                                                    Encoder buttons!
************************************************************************************************************************************/
  
  ClickEncoder::Button a = encoder1.getButton();
  if (a != ClickEncoder::Open) {
    Serial.print("Button 1: ");
    #define VERBOSECASE(label) case label: Serial.println(#label); break;
    switch (a) {
      VERBOSECASE(ClickEncoder::Pressed);
      VERBOSECASE(ClickEncoder::Held)
      VERBOSECASE(ClickEncoder::Released)
      case ClickEncoder::Clicked:
        loadDir(directoryNames[tempDir]);
      break;
      VERBOSECASE(ClickEncoder::DoubleClicked)
    }
  }   
    ClickEncoder::Button b = encoder2.getButton();
  if (b != ClickEncoder::Open) {
    Serial.print("Button 2: ");
    #define VERBOSECASE(label) case label: Serial.println(#label); break;
    switch (b) {
      case ClickEncoder::Pressed:
        break;
      VERBOSECASE(ClickEncoder::Held)
      VERBOSECASE(ClickEncoder::Released)
      case ClickEncoder::Clicked:            
        if(encoder4Mode==BPMMODE){
          encoder4Mode=VOLMODE;
          encPos4=currentVol;
          oldEncPos4=currentVol;    
          
          display.clearDisplay();
          display.setCursor(0,0);
          display.setTextSize(1);
          display.setTextColor(WHITE);
          display.println("Set Volume");
          if(darkMode!=DARKMODE_DARK) display.display();
        }
        else if(encoder4Mode==VOLMODE){
          encoder4Mode=BPMMODE;
          encPos4=bpm;
          oldEncPos4=bpm;    

          display.clearDisplay();
          display.setCursor(0,0);
          display.setTextSize(1);
          display.setTextColor(WHITE);
          display.println("Set BPM");
          if(darkMode!=DARKMODE_DARK) display.display();
        }
        break;
      VERBOSECASE(ClickEncoder::DoubleClicked)
    }
  }   
    ClickEncoder::Button c = encoder3.getButton();
  if (c != ClickEncoder::Open) {
    Serial.print("Button 3: ");
    #define VERBOSECASE(label) case label: Serial.println(#label); break;
    switch (c) {
      VERBOSECASE(ClickEncoder::Pressed);
      VERBOSECASE(ClickEncoder::Held)
      VERBOSECASE(ClickEncoder::Released)
      case ClickEncoder::Clicked:
        if(!darkMode){
          darkMode=DARKMODE_LED_OFF;
          digitalWrite(BLUELED, LOW);  
          digitalWrite(REDLED, LOW);  
        } 
        else if(darkMode==DARKMODE_LED_OFF){
          darkMode=DARKMODE_DARK;
          digitalWrite(BLUELED, LOW);  
          digitalWrite(REDLED, LOW);   
          display.clearDisplay();
          display.setCursor(0,0);
          display.display();
        }
        else{
          darkMode=DARKMODE_LIGHT;
          display.clearDisplay();
          display.setTextSize(1);
          display.setTextColor(WHITE);
          display.print("0x"); display.println(0xDEADBEEF, HEX);  
          display.setTextSize(3);
          display.setTextColor(WHITE);
          display.print("HELLO!");
          display.display();
        } 
      break;
      VERBOSECASE(ClickEncoder::DoubleClicked)
    }
  }   
    ClickEncoder::Button d = encoder4.getButton();
  if (d != ClickEncoder::Open) {
    Serial.print("Button 4: ");
    #define VERBOSECASE(label) case label: Serial.println(#label); break;
    switch (d) {
      VERBOSECASE(ClickEncoder::Pressed);
      VERBOSECASE(ClickEncoder::Held)
      VERBOSECASE(ClickEncoder::Released)
      case ClickEncoder::Clicked:     
        //playSample(100);
         samplePos=random(currentSample.size()*.9);       //Pick a random position to start from   
        chooseNewSample(100);
      break;
      VERBOSECASE(ClickEncoder::DoubleClicked)
    }
  } 
  ClickEncoder::Button e = encoder5.getButton();
  if (e != ClickEncoder::Open) {
    Serial.print("Button 5: ");
    #define VERBOSECASE(label) case label: Serial.println(#label); break;
    switch (e) {
      VERBOSECASE(ClickEncoder::Pressed);
      VERBOSECASE(ClickEncoder::Held)
      VERBOSECASE(ClickEncoder::Released)
      VERBOSECASE(ClickEncoder::Clicked)
      VERBOSECASE(ClickEncoder::DoubleClicked)
    }
  } 
}
