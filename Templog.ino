#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MAX31856.h>

//file vars
File logfile;
File lastFile;
char filename[15];

//temperature vars
float currentT[10];
float currentCJT[10];
float Tav[10];
float CJTav[10];

//internal controls
long currentmillis;
long previousmillis;
int measureNUM;
bool SDfound;
bool continuouslog=false;

//measurement settings
long interval=1000; //measurement interval
bool oneshot=false; // makes measurements happen on demand instead of ever 100ms causing a delay in measuring but reducing power consumption 
long singledelay=60000; //delay before averaging begins
long singlesampnum=30; // number of samples to average
bool buzzer=true; // J9 will be unusable while true


Adafruit_SSD1306 display = Adafruit_SSD1306();

// Use software SPI: CS, DI, DO, CLK
//Adafruit_MAX31856 max = Adafruit_MAX31856(10, 11, 12, 13);
// use hardware SPI, just pass in the CS pin
Adafruit_MAX31856 max[10];
int8_t pinlist[10]={14,15,13,16,12,17,11,18,10,19}; //[J1,J2,J3,J4,J5,J6,J7,J8,J9,J10]
int8_t currentslot;
bool maxfound[10];

 

#define BUTTON_A 9
#define BUTTON_B 6
#define BUTTON_C 5
#define LED      13
#define cardSelect 4
#define buzz 10



#if (SSD1306_LCDHEIGHT != 32)
 #error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif


 
void setup() { 

  if(buzzer){
    pinMode(buzz,OUTPUT);
    digitalWrite(buzz,LOW);
  }
  Serial.begin(9600);
 
  Serial.println("OLED FeatherWing test");
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  // init done
  Serial.println("OLED begun");
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(1000);
 
  // Clear the buffer.
  display.clearDisplay();
  display.display();
  
  Serial.println("IO test");
 
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  //thermocouple setup
  for(int i=0; i<10; i++){
    if(!(buzzer && i==8)){
      max[i]=Adafruit_MAX31856(pinlist[i]);
      max[i].begin();
      max[i].setThermocoupleType(MAX31856_TCTYPE_T);
      maxfound[i]=max[i].getThermocoupleType()==MAX31856_TCTYPE_T;
    }else{
      maxfound[i]=false;
    }
  }
  for(int i=0; i<10&&!maxfound[currentslot];i++){
    currentslot=i;
  }
  

  measureNUM=0;
 
  // text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  
  display.print("Connecting to SSID\n'adafruit':");
  display.print("connected!");
  display.println("IP: 10.0.1.23");
  display.println("Sending val #0");
  display.setCursor(0,0);
  display.display(); // actually display all of the above
  delay(1000);
//  Defuct thermocouple type check
//  display.setCursor(0,0);
//  display.clearDisplay();
//  display.print("Thermocouple type: ");
//  switch ( max.getThermocoupleType() ) {
//    case MAX31856_TCTYPE_B: display.println("B Type"); break;
//    case MAX31856_TCTYPE_E: display.println("E Type"); break;
//    case MAX31856_TCTYPE_J: display.println("J Type"); break;
//    case MAX31856_TCTYPE_K: display.println("K Type"); break;
//    case MAX31856_TCTYPE_N: display.println("N Type"); break;
//    case MAX31856_TCTYPE_R: display.println("R Type"); break;
//    case MAX31856_TCTYPE_S: display.println("S Type"); break;
//    case MAX31856_TCTYPE_T: display.println("T Type"); break;
//    case MAX31856_VMODE_G8: display.println("Voltage x8 Gain mode"); break;
//    case MAX31856_VMODE_G32: display.println("Voltage x8 Gain mode"); break;
//    default: display.println("Unknown"); break;\    
//  }
//  display.display();
//  delay(1000);
  display.setCursor(0,0);
  display.clearDisplay();
  
  if (!SD.begin(cardSelect)) {
    //blinking message that card could not be initialized
    for(int i=0; i<9; i++){
      display.setCursor(0,0);
      display.clearDisplay();
      if(i%2==0){
        display.println("Card init. failed!");
      }
      display.display();
      delay(500);
    }
    SDfound=false;
  }else{SDfound=true;}

  if (SD.exists("lastFile.txt")){
    lastFile=SD.open("lastFile.txt",FILE_READ);
    lastFile.read(filename,15);
    filenamePLUS(filename);
    lastFile.close();
  } else{
    strcpy(filename, "TLog_A32.txt");
  }
  
  while(digitalRead(BUTTON_C) && SDfound){
    display.setCursor(0,0);
    display.clearDisplay();
    display.print("File: ");
    display.print(filename);
    if (!SD.exists(filename)){
      display.print("\n");
      display.print("<New File>");
    } else{
      display.print("\n");
      display.print("<Open>");
    }
    if(! digitalRead(BUTTON_A)){
      filenamePLUS(filename);
    } else if(! digitalRead(BUTTON_B)){
      filenameMINUS(filename);
    }
    display.display();
    delay(100);
    
  }

  //make new file if none exists. If one exists gives option to overwrite
  if(SD.exists(filename)){
    display.setCursor(0,0);
    display.clearDisplay();
    display.println("Y  Overwrite?");
    display.println("N");
    display.display();
    while(digitalRead(BUTTON_A)&&digitalRead(BUTTON_B)){
    }
    if(!digitalRead(BUTTON_A)){
      SD.remove(filename);
      delay(100);
      logfile=SD.open(filename,FILE_WRITE);
      printheader(logfile);
    }else{
      logfile=SD.open(filename,FILE_WRITE);
    }
  }else{
    logfile=SD.open(filename,FILE_WRITE);
    printheader(logfile);
  }
  
  display.setCursor(0,0);
  display.clearDisplay();
  
  if( !SDfound ){
    logfile.close();
  }else if( !logfile ) {
    //blinking message that file could not be created
    for(int i=0; i<9; i++){
      display.setCursor(0,0);
      display.clearDisplay();
      if(i%2==0){
        display.print("Couldnt create \n");
        display.println(filename);
      }
      display.display();
      delay(500);
    }    
    logfile.close();
    
  } else {
    
    logfile.close();
    SD.remove("lastFile.txt");
    lastFile=SD.open("lastFile.txt",FILE_WRITE);
    if( ! lastFile ) {
      display.print("Couldnt create\n"); 
      display.println("lastFile.txt");
      display.display();
      delay(1000);
    } else {
      lastFile.print(filename);
    }
    lastFile.close();
  }

  display.setCursor(0,0);
  display.clearDisplay();
  display.display();
  if(buzzer){
    digitalWrite(buzz,HIGH);
    delay(200);
    digitalWrite(buzz,LOW);
  }else{delay(200);}
  previousmillis=millis();
  currentmillis=millis();
  
}
 

///////////////Main Body///////////////

void loop() {
  //switch the thermocouple temp displayed
  if (! digitalRead(BUTTON_A)){
    switchDisplay();
  }

  //Log singgle value. Uses and average of singlesampnum points after a delay of singledelay milliseconds if oneshot is off. If oneshot is on it just takes a single point (don't turn on oneshot)
  if (!digitalRead(BUTTON_B) && SDfound && !continuouslog){
    previousmillis=millis();
    measureNUM++;
    if(!oneshot){
      logfile = SD.open(filename, FILE_WRITE);
      if(checkSD()){
        for(int j=0;j<10;j++){
          Tav[j]=0;
          CJTav[j]=0;
        }
        display.println("Waiting...");
        display.display();
        while(currentmillis-previousmillis<singledelay){
          currentmillis=millis();
        }
        for (int i=0;i<singlesampnum;i++){
          while(currentmillis-previousmillis<150){
            currentmillis=millis();
          }
          measuretemps();
          previousmillis=millis();
          for(int j=0;j<10;j++){
            Tav[j]+=currentT[j];
            CJTav[j]+=currentCJT[j];
          }
        }
        for(int j=0;j<10;j++){
          Tav[j]=Tav[j]/float(singlesampnum);
          CJTav[j]=CJTav[j]/float(singlesampnum);
        }
      
        for(int j=0;j<10;j++){
          currentT[j]=Tav[j];
          currentCJT[j]=CJTav[j];
        }    
      
        logtemps(logfile);
        logfile.close();
        printtemps();
        display.print("M");display.print(measureNUM);display.println(" Logged");
        display.display();
        previousmillis=millis();
        beep(interval);
    
      }
    }else{
    
      logfile = SD.open(filename, FILE_WRITE);
      if(checkSD()){
    
        logtemps(logfile);
        logfile.close();
    
        display.print("M");display.print(measureNUM);display.println(" Logged");
        display.display();
        beep(200);
        while(currentmillis-previousmillis<interval){
          currentmillis=millis();
        }
        previousmillis+=interval;
      }
    }
  }

  //toggle continuous temperature logging
  if (! digitalRead(BUTTON_C) && SDfound){
    if(!continuouslog){
      beginCmode();
    } else{
      endCmode();
    }  
  }


  // wait loop
  currentmillis=millis();
  while(digitalRead(BUTTON_A)&&(digitalRead(BUTTON_B)||continuouslog)&&digitalRead(BUTTON_C)&&(currentmillis-previousmillis<interval)){
    currentmillis=millis();
  }

   //Read the temps and set next read time
  if(currentmillis-previousmillis>=interval){
    previousmillis+=interval;
    
    measuretemps();
    printtemps();
    if(continuouslog){
      display.print("Continuous Logging M");display.print(measureNUM);
      logtemps(logfile);
    }
    display.display();
  }
}


/////////////Functions/////////////////

//increments the file name up one
void filenamePLUS(char* filename) {
  filename[5]=filename[5]+((filename[7]-'0'+1)/10+(filename[6]-'0'))/10;
  if(filename[5]>'Z'){
    filename[5]='Z';
  }
  filename[6]='0'+((filename[7]-'0'+1)/10+(filename[6]-'0'))%10;
  filename[7]='0'+(filename[7]-'0'+1)%10;
}

//increments the file name down one
void filenameMINUS(char* filename) {
  filename[5]=filename[5]+((filename[7]-'0'+9)/10+(filename[6]-'0')+9)/10-1;
  if(filename[5]<'A'){
    filename[5]='A';
  }
  filename[6]='0'+((filename[6]-'0')+(filename[7]-'0'+9)/10+9)%10;
  filename[7]='0'+(filename[7]-'0'+9)%10;
}

//Prints lables to the file to identify subsiquent data
void printheader(File lfile){
  lfile.print("meas_num\t");
    lfile.print("time(ms)\t");
    for(int i=0;i<9;i++){
      lfile.print("J");lfile.print(i+1);lfile.print("_TC\t");
      lfile.print("J");lfile.print(i+1);lfile.print("_CJ\t");
    }
    lfile.print("\r\n");
}

//Saves the most recent temperature measurement from the chip to the temperature vars if global var oneshot is false. Otherwise it takes the measurements when called which could take a up to a few seconds.
void measuretemps(void) {
  for (int i=0; i<9;i++){         
    if(maxfound[i]){
      currentT[i]=max[i].readThermocoupleTemperature(oneshot);
      currentCJT[i]=max[i].readCJTemperature(oneshot);
      faultCheck(i);
    }
  }
  currentmillis=millis();
}

// Logs temps to file passed
void logtemps(File lfile){
  lfile.print(measureNUM);lfile.print("\t");
  lfile.print(currentmillis);lfile.print("\t");
  for(int i=0;i<9;i++){
    if(maxfound[i]){
      lfile.print(currentT[i]);lfile.print("\t");
      lfile.print(currentCJT[i]);lfile.print("\t");
    } else{
      lfile.print("NA\tNA\t");
    }
  }
  lfile.print("\r\n");
}

//clears then prints temps to screen. Must be followed by display command to make visible
void printtemps(void){
  display.setCursor(0,0);
  display.clearDisplay();
  display.print("CJ");display.print(currentslot+1);display.print(" Temp: "); display.print(currentCJT[currentslot]);display.println("C");
  display.print("TC");display.print(currentslot+1);display.print(" Temp: "); display.print(currentT[currentslot]);display.println("C");
  if(!SDfound){
    display.print("No SDcard");
  }
}

//checks for SD card. Only use in loop after logfile has been opened
bool checkSD(void){
  logfile.seek(logfile.position()-1);
  SDfound=!(logfile.read()==-1);
  return SDfound;
}

void beep(long t){
  digitalWrite(buzz,HIGH);
  delay(t);
  digitalWrite(buzz,LOW);
}

void beginCmode(void){
  measureNUM++;
  printtemps();
  logfile = SD.open(filename, FILE_WRITE);
  if(checkSD()){
    continuouslog=true;
    display.print("Continuous Logging M");display.print(measureNUM);
  }
  display.display();
  beep(200);
  previousmillis=millis();
}

void endCmode(void){
  logfile.close();
  printtemps();
  display.display();
  beep(200);
  continuouslog=false;
}

void switchDisplay(void){
  currentslot=(currentslot+1)%10;
  for (int i=0; i<9&&!maxfound[currentslot];i++){
    currentslot=(currentslot+1)%10;
  }
  printtemps();
  if(continuouslog){
    display.print("Continuous Logging M");display.print(measureNUM);
  }
  display.display();
  delay(100);
}

void faultCheck(int i){
  uint8_t fault =max[i].readFault();
  if(fault){
    digitalWrite(buzz,HIGH);
            
    display.setCursor(0,0);
    display.clearDisplay();
    display.print("CJ");display.print(i+1);

    if (fault & MAX31856_FAULT_CJRANGE) display.println("Cold Junction Range Fault");
    if (fault & MAX31856_FAULT_TCRANGE) display.println("Thermocouple Range Fault");
    if (fault & MAX31856_FAULT_CJHIGH)  display.println("Cold Junction High Fault");
    if (fault & MAX31856_FAULT_CJLOW)   display.println("Cold Junction Low Fault");
    if (fault & MAX31856_FAULT_TCHIGH)  display.println("Thermocouple High Fault");
    if (fault & MAX31856_FAULT_TCLOW)   display.println("Thermocouple Low Fault");
    if (fault & MAX31856_FAULT_OVUV)    display.println("Over/Under Voltage Fault");
    if (fault & MAX31856_FAULT_OPEN)    display.println("Thermocouple Open Fault");
    display.display();
  }
  while(fault){
    fault =max[i].readFault();
  }
  digitalWrite(buzz,LOW);
}
