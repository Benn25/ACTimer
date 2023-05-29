#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "RTClib.h"
#include "OneButton.h"
#include <Smoothed.h> 	// Include the library
#include <EEPROM.h>
 

RTC_DS3231 rtc;

#define but1 D5
#define but2 D6
#define but3 D7
#define potPin A0
//#define CH1 D4 //Not working apparently...
//#define CH2 D3 //NO !! need HIGH at startup, or no boot, TX marche pas non plu
#define CH1 RX
#define CH2 D0
#define CH3 D8

Smoothed <int> POT;

// Setup a new OneButton on pin PIN_INPUT
// The 2. parameter activeLOW is true, because external wiring sets the button to LOW when pressed.
OneButton button1(but1, true);
OneButton button2(but2, true);
OneButton button3(but3, true);

int pot = 0;
int smoothedPot;
int prevPot = 0;
int stateCH3 = 1; // link state for chan3, 0 = OFF, 1 = LINKED TO CH1, 2 = LINKED TO CH2, 3 = ON

int addr = 0; //EEPROM address

int SetHour;
int SetMin;

int NowMinute; //take the time now from RTC or NTP, then convert it to minute number

unsigned long screenOFFTimer = 0;

const char *ssid     = "2.4 Higashi-laia";
const char *password = "mamosukonepi2";

String pointerTime;

bool UpOrDwn;

const long utcOffsetInSeconds = 9 * 60 * 60;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

 //Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R2, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ SCL, /* data=*/ SDA);   // pin remapping with ESP8266 HW I2C

bool timeLine1[96];
bool timeLine2[96];

void drawLines(){
 //u8g2.clearBuffer();
  for (int a = 0; a < 96; a++)
  {
    u8g2.setDrawColor(1);//write mode
    u8g2.drawLine(a, timeLine1[a], a, timeLine1[a]*3);
    u8g2.drawLine(a, u8g2.getHeight()-1 - timeLine2[a], a, u8g2.getHeight()-1 - 3 * timeLine2[a]);
  }
  //u8g2.sendBuffer();
}

void setPointerTime(){ //time to display at the cursor location
  int minAtPointer = min(smoothedPot,95) * 15; //this is the nb of min of this day
  pointerTime = "";
  pointerTime += minAtPointer/60;
  pointerTime += ':';
  pointerTime += minAtPointer%60;
}

void updateRelay(){

if(timeLine1[NowMinute/15] == HIGH){ //chan 1 is UP, activate relay CH1
    digitalWrite(CH1, HIGH);
   // Serial.print("CH1 is UP   ");
}
else{
    digitalWrite(CH1, LOW);
  //  Serial.print("CH1 is LOW   ");
}
if(timeLine2[NowMinute/15] == HIGH){ //chan 1 is UP, activate relay CH1
    digitalWrite(CH2, HIGH);
   // Serial.print("CH2 is UP   ");
}
else{
    digitalWrite(CH2, LOW);
  //  Serial.print("CH2 is LOW   ");
}
if(stateCH3==0||stateCH3==3){
digitalWrite(CH3, stateCH3/3);
}
if(stateCH3 == 1){//link to chan 1, this is default
digitalWrite(CH3, digitalRead(CH1));
  //  Serial.print("CH3 is linked to CH1   ");
}
if(stateCH3 == 2){//link to chan 2
digitalWrite(CH3, digitalRead(CH2));
  //  Serial.print("CH3 is linked to CH2   ");
}
//Serial.println("");
}

void clic1(){
     timeLine1[smoothedPot] = !timeLine1[smoothedPot] ;
  //Serial.println(timeLine1[smoothedPot]);
u8g2.setDrawColor(0); // errase mode
u8g2.drawBox(0, 0, 96, 32);//clear the timelines zone
drawLines();
}
void longPress1(){
timeLine1[smoothedPot] = UpOrDwn;
u8g2.setDrawColor(0); // errase mode
u8g2.drawBox(0, 0, 96, 32);//clear the timelines zone
drawLines();
}
void longPressStart1(){
timeLine1[smoothedPot] == 0 ? UpOrDwn = 1 : UpOrDwn = 0 ;
}
void doubleClic1(){
timeLine1[smoothedPot] == 0 ? UpOrDwn = 1 : UpOrDwn = 0 ;
for (int a = 0; a < 96; a++){
    timeLine1[a] = UpOrDwn;
}
u8g2.setDrawColor(0); // errase mode
u8g2.drawBox(0, 0, 96, 32);//clear the timelines zone
drawLines();
}

void clic2(){
     timeLine2[smoothedPot] = !timeLine2[smoothedPot] ;
  //Serial.println(timeLine1[smoothedPot]);
u8g2.setDrawColor(0); // errase mode
u8g2.drawBox(0, 0, 96, 32);//clear the timelines zone
drawLines();
}
void longPress2(){
timeLine2[smoothedPot] = UpOrDwn;
u8g2.setDrawColor(0); // errase mode
u8g2.drawBox(0, 0, 96, 32);//clear the timelines zone
drawLines();
}
void longPressStart2(){
timeLine2[smoothedPot] == 0 ? UpOrDwn = 1 : UpOrDwn = 0 ;
}
void doubleClic2(){
timeLine2[smoothedPot] == 0 ? UpOrDwn = 1 : UpOrDwn = 0 ;
for (int a = 0; a < 96; a++){
    timeLine2[a] = UpOrDwn;
}
u8g2.setDrawColor(0); // errase mode
u8g2.drawBox(0, 0, 96, 32);//clear the timelines zone
drawLines();
}

void clic3(){
   if(smoothedPot<100){
stateCH3 >= 3 ? stateCH3 = 0 : stateCH3++ ;
Serial.print("StateCH3 = ");
Serial.println(stateCH3);
delay(80);
   }
}

void recoverSave(){
  for(int a = 0; a < 96; a++){
    timeLine1[a] = EEPROM.read(a);
    timeLine2[a] = EEPROM.read(a+100);
    stateCH3 = EEPROM.read(200);
  }
}
void SaveProg(){
  for(int a = 0; a < 96; a++){
 EEPROM.write(a, timeLine1[a]);
 EEPROM.write(a+100, timeLine2[a]);
  }
  EEPROM.write(200, stateCH3);
  EEPROM.commit();

  delay(250);
}

void setup() {

  EEPROM.begin(256);  //Initialize EEPROM
  u8g2.begin();
  Serial.begin(115200);

 if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

// Initialise the first sensor value store. We want this to be the simple average of the last 10 values.
	// Note: The more values you store, the more memory will be used.
	POT.begin(SMOOTHED_AVERAGE, 10);	
	// Initialise the second sensor value store. We want this one to be a simple linear recursive exponential filter. 
	// We set the filter level to 10. Higher numbers will result in less filtering/smoothing. Lower number result in more filtering/smoothing
	//POT.begin(SMOOTHED_EXPONENTIAL, 10);
  //POT.clear();

//  pinMode(but1, INPUT_PULLUP);//no need with oneButton
  pinMode(CH1, OUTPUT);
  pinMode(CH2, OUTPUT);
  pinMode(CH3, OUTPUT);

  u8g2.clearBuffer(); // clear the internal memory
   ////////////////////internet shit////////////////
  WiFi.begin(ssid, password);

    while ( WiFi.status() != WL_CONNECTED && millis() < 5000) {
 u8g2.setFont(u8g2_font_helvB10_tf);
 u8g2.drawStr(8, 22, "Connecting...");
 u8g2.sendBuffer(); // draw shit  
 delay(500);
 Serial.print(".");
 u8g2.clearBuffer(); // clear all for next loop
    }

while (WiFi.status() == WL_CONNECTED && millis() < 8000) {
 u8g2.drawStr(8, 22, "Connected!");
 u8g2.sendBuffer(); // draw shit
 delay(200);
 u8g2.clearBuffer(); // clear all for next loop

    timeClient.begin();
  timeClient.update();
    Serial.print(daysOfTheWeek[timeClient.getDay()]);
    Serial.print(", ");
    Serial.print(timeClient.getHours());
    Serial.print(":");
    Serial.print(timeClient.getMinutes());
    Serial.print(":");
    Serial.println(timeClient.getSeconds());

delay(200);

rtc.adjust(DateTime(2023, 1, 1,
   timeClient.getHours(),
    timeClient.getMinutes(),
     timeClient.getSeconds()));

}

while (WiFi.status() != WL_CONNECTED && millis() > 5000 && millis() < 8000) {
 u8g2.drawStr(0, 22, "Can't connect...");
 u8g2.sendBuffer(); // draw shit
 delay(500);
 u8g2.clearBuffer(); // clear all for next loop
}


  /////////////////////end internet shit ///////////////////
  
  // RTC adjust to now time
  //  January 21, 2014 at 3am you would call:
  //  rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  
  
  
/* /////full white for debug and mesure
  u8g2.drawBox(0, 0, u8g2.getWidth(), u8g2.getHeight());
  u8g2.sendBuffer();
  while(1){
    delay(10);
  }
  */

  bool ENTERSETTINGS = 0;
  bool ENTERSETMIN = 0;
  ///////START SETTING BY HAND

  if (digitalRead(but3) == LOW){
    ENTERSETTINGS = 1;
while(digitalRead(but3) == LOW){
  delay(100); //stop everything while button is still pressed
 u8g2.setFont(u8g2_font_helvB18_tf);
}
delay(100);
}

while(ENTERSETTINGS==1){ //SET THE TIME BY HAND
  POT.add(analogRead(potPin));
  smoothedPot = POT.get();

  SetHour = constrain(map(smoothedPot, 50, 1010, 0, 23), 0, 23);

//  Serial.print("SetHour");
//  Serial.print(" : ");
//  Serial.println(SetHour);

//  pointerTime = SetHour;
  u8g2.setCursor(32, u8g2.getHeight() / 2+7);
  u8g2.print(SetHour);
  
  u8g2.sendBuffer();  // draw shit
  u8g2.clearBuffer(); // clear all for next loop
delay(25);
  if (digitalRead(but3) == LOW){
  ENTERSETMIN = 1; //get out of the settings while loop
 // rtc.adjust(DateTime(2018,1,1,SetHour,SetMin,0 )); //set the time into the RTC chip
  delay(50);
  while (digitalRead(but3) == LOW){
    delay(100); // stop everything while button is still pressed
}
delay(100);
}
while(ENTERSETMIN==1){
delay(25);
  POT.add(analogRead(potPin));
  smoothedPot = POT.get();

  SetMin = constrain(map(smoothedPot, 50, 1010, 0, 59), 0, 59);

 // Serial.print("Minutes");
 // Serial.print(" : ");
 // Serial.println(SetMin);

//  pointerTime = SetHour;
  u8g2.setCursor(32, u8g2.getHeight() / 2+7);
  u8g2.print(SetHour);
  u8g2.setCursor(62, u8g2.getHeight() / 2+7);
  u8g2.print(SetMin);

  u8g2.sendBuffer();  // draw shit
  u8g2.clearBuffer(); // clear all for next loop

  

  if (digitalRead(but3) == LOW)
  {
    ENTERSETMIN = 0;
    ENTERSETTINGS = 0; // get out of the settings while loop
    rtc.adjust(DateTime(2018,1,1,SetHour,SetMin,0 )); //set the time into the RTC chip
        
    delay(100);
}
}
}
////////end setting by hand

  button1.attachClick(clic1);
  button1.attachDoubleClick(doubleClic1);
  button1.attachLongPressStart(longPressStart1);
//  button1.attachLongPressStop(longPressStop1);
  button1.attachDuringLongPress(longPress1);
  button1.setClickTicks(300);

    button2.attachClick(clic2);
  button2.attachDoubleClick(doubleClic2);
  button2.attachLongPressStart(longPressStart2);
//  button1.attachLongPressStop(longPressStop1);
  button2.attachDuringLongPress(longPress2);
  button2.setClickTicks(300);

  button3.attachClick(clic3);


  recoverSave(); //read the EEPROM to recover the saved data

 DateTime now = rtc.now();

    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();


  u8g2.setFont(u8g2_font_5x7_tf);

}

void loop() {
  DateTime now = rtc.now();
  //NowMinute = timeClient.getHours() * 60 + timeClient.getMinutes(); // Refresh the time we are NOW. change to Now.hour and minutes for read from RTC
  NowMinute = (now.hour(), DEC) * 60 + (now.minute(), DEC);
  
  u8g2.setFont(u8g2_font_5x7_tf);
 
  u8g2.setDrawColor(0);       // errase mode
  u8g2.drawBox(100, 16, 128, 22); // clear the txt
  u8g2.setDrawColor(1); // write mode

  u8g2.setFontPosTop();
  u8g2.drawStr(100, -1, "CH2");
  u8g2.drawStr(100, 8, "CH3");
  u8g2.setFontPosBottom();
  u8g2.drawStr(100, 33, "CH1");
  u8g2.drawLine(98, 12, 98, 32 - 13);

  //Serial.println(stateCH3);

  if(stateCH3==0){// link state for chan3, 0 = OFF, 1 = LINKED TO CH1, 2 = LINKED TO CH2, 3 = ON
  u8g2.drawStr(100, 24, "OFF");
  }
  if(stateCH3==1){
  u8g2.drawStr(100, 24, "CH1");
  }
  if(stateCH3==2){
  u8g2.drawStr(100, 24, "CH2");
  }
  if(stateCH3==3){
  u8g2.drawStr(100, 24, "ON");
  }


  POT.add(analogRead(potPin));
  smoothedPot = POT.get();
  smoothedPot = map(smoothedPot, 0, 1023, 102, 0)-1;

  button1.tick(); //check the button state
  button2.tick(); //check the button state
  button3.tick(); //check the button state

  // timeClient.update();
  setPointerTime();

  u8g2.setDrawColor(0); // errase mode
  u8g2.drawBox(0, 0, 97, 31); // clear the timelines zone
  drawLines();
  u8g2.setDrawColor(1); // write mode

  // u8g2.setCursor(pot-10, 30);
  // u8g2.print(timeClient.getHours());
  // u8g2.setCursor(pot, 30);
  // u8g2.print(timeClient.getMinutes());
 
  int pointerOffset;

if(smoothedPot<98){
if(millis() < screenOFFTimer + 5000){ //the moving cursor
u8g2.drawLine(min(smoothedPot, 95), u8g2.getHeight() / 2 - 9, min(smoothedPot, 95), u8g2.getHeight() / 2 + 9); // cursor
smoothedPot < 48 ? pointerOffset = 4 : pointerOffset = -39;
u8g2.setFont(u8g2_font_helvB10_tf);
u8g2.drawStr(min(smoothedPot, 95) + pointerOffset, u8g2.getHeight() / 2 + 9, pointerTime.c_str());
// Serial.println("OK");
}
else{ //no touch pot = displaying the time
  u8g2.setFont(u8g2_font_helvB12_tf);
  int offsetCursor;
  (now.hour(), DEC) < 10 ? offsetCursor = 19 : offsetCursor = 0;
  u8g2.setCursor(25 + offsetCursor, u8g2.getHeight() / 2 + 11);
  u8g2.print(now.hour(), DEC);//u8g2.print(timeClient.getHours()); to get from internet //call .print(now.hour(), DEC); from the RTC
  u8g2.setCursor(50, u8g2.getHeight() / 2+11);
  u8g2.print(now.minute(), DEC);//u8g2.print(SetMin); //.print(now.minute(), DEC); from the RTC

//////add here a moving point to show the NOW time on screen
  u8g2.drawLine(map(NowMinute,0,1440,0,96),5,map(NowMinute,0,1440,0,96),6);
  u8g2.drawLine(map(NowMinute,0,1440,0,96),u8g2.getHeight()-6,map(NowMinute,0,1440,0,96),u8g2.getHeight()-7);
}
}

else{  //pot max to the right, enter save menu
  
   //u8g2.setFont(u8g2_font_unifont_t_symbols);
 u8g2.setFont(u8g2_font_helvB08_tf);
  u8g2.setFontPosTop();
  u8g2.drawStr(0, 5, "Save (press CH1)");
    u8g2.setFontPosBottom();
  u8g2.drawStr(0, 31-4, "Load (press CH2)");

  if (digitalRead(but1) == LOW)
  {
  u8g2.setDrawColor(0); // errase mode
  u8g2.drawBox(0, 0, 96, 33); // clear the timelines zone
  u8g2.setDrawColor(1); // write mode
  u8g2.setFont(u8g2_font_helvB10_tf);
  u8g2.drawStr(5, 22, "Saving...");
  u8g2.sendBuffer();
  delay(500);
  SaveProg();
  u8g2.setDrawColor(0);       // errase mode
  u8g2.drawBox(0, 0, 96, 33); // clear the timelines zone
  u8g2.setDrawColor(1);       // write mode
  u8g2.setFont(u8g2_font_helvB14_tf);
  u8g2.drawStr(5, 25, "Done!");
  u8g2.sendBuffer();
  delay(1500);
  while(analogRead(potPin)>700){
delay(10);
  }
}
if(digitalRead(but2) == LOW){
  u8g2.setDrawColor(0); // errase mode
  u8g2.drawBox(0, 0, 96, 33); // clear the timelines zone
  u8g2.setDrawColor(1); // write mode
  recoverSave();
  delay(80);
  u8g2.setFont(u8g2_font_helvB08_tf);
  u8g2.drawStr(0, 21, "Data recovered");
  u8g2.sendBuffer();
  u8g2.setFont(u8g2_font_helvB08_tf);
  delay(1000);
}
}
  u8g2.sendBuffer();
//  Serial.println(smoothedPot);
  updateRelay(); //update relay state

  if (prevPot != smoothedPot)
  {                          // someone if fucking with the Pot !!
  screenOFFTimer = millis(); //reset the time stamp
}
  prevPot = smoothedPot;
}
