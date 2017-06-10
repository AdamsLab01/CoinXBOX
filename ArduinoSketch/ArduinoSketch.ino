/**********************************************************************************************************************

This sketch was written to work with a simple arcade style coin mechanism in conjunction with an HDMI switch to allow 
you to require the inserting of a coin in order to use the XBOX. While I used this for an XBOX there is nothing XBOX 
specific about the code and it should work swimmingly with any device that outputs video over HDMI. 

For more information see - https://adambyers.com/2013/09/coin-operated-xbox/ or ping adam@adambyers.com

Coin Operated XBOX
Copyright (C) 2013 Adam Byers

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

**********************************************************************************************************************/

// include the Arduino LCD libraries
#include <LiquidCrystal.h>

// States
#define S_ready 1
#define S_coin 2
#define S_pause 3
#define S_override 4
#define S_limit 5
#define S_countdown 6
#define S_choretime 7
#define S_caught 8

// Default start up state
int state = S_ready;

// LCD PINs
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// Display back-light pins
const int LCDr = 3;
const int LCDg = 5;
const int LCDb = 6;

// Other PINs
const int CoinSW = 2;
const int VideoSW = 4;
const int StartSW = A0;
const int PauseSW = A1;
const int CaughtSW = A2; // Was CoinLED
const int OverrideSW = A3;
const int VideoDetect  = A4;
const int Speaker = A5;

// Button debounce vars
int buttonState;
int lastButtonState = HIGH;
long lastDebounceTime = 0;
long debounceDelay = 40; // May have to play with this value to get coins to register properly.

// Coin counting and time vars 
unsigned long AllowedPlaytime;
unsigned long ChoreTime = 30; // How long (in miniutes) do you want the chore time (manditory pause) to last.
unsigned long ChoreCountdown;
unsigned long CurrentMillis;
int CoinCount = 0;
int CoinTimeValue = 30; // Amount of time (in minutes) that the coin is worth.
int DepositStringIndex = 0;
int TimeStringIndex = 0;

// Video on/off switching vars
int VideoSwitchState = LOW;
long VideoSwitchBuffer = 120000; // Add 2 miniutes to the "bought" time to compensate for video turn on delay.
unsigned long SwitchPreviousMillis = 0;
long VideoSwitchInterval = 1000; // How long (in milliseconds) we "hold down" the HDMI input selector button when switching inputs.

// Warning beep vars
unsigned long LastWarningTime = 0;
long WarningBeepInterval1 = 60000; // 60 seconds
long WarningBeepInterval2 = 1000; // 1 seconds

// Used to get the LCD to clear properly when switching between states.
bool ClearLCD = false;
bool PauseRun = false;
bool OverrideRun = false;
bool CountdownRun = false;

// Used for selection of time values to dislayu depending on what state the system is running in.
bool PlaytimeRun = false;
bool ChoretimeRun = false;

// Arms up characters for display on the LCD.
byte armsUp[8] = {
  0b00100,
  0b01010,
  0b00100,
  0b10101,
  0b01110,
  0b00100,
  0b00100,
  0b01010
};

// Strings for the deposit display
char* DepositStrings[]= {
"$0.00", // Place holder does not display.
"$0.25", // 30 min
"$0.50", // 1 hour - does not display
};

char* TimeStrings[]= {
"$0.00", // Place holder does not display.
"30m  ",
"1h   ", // 1 hour - does not display
};

void setup() { 
// Initialize the LCD
lcd.begin(16, 2);

// Set PINs as inputs or outputs. 
pinMode(LCDr, OUTPUT);
pinMode(LCDg, OUTPUT);
pinMode(LCDb, OUTPUT);
pinMode(VideoSW, OUTPUT);
pinMode(Speaker, OUTPUT);
pinMode(CoinSW, INPUT);
pinMode(StartSW, INPUT);
pinMode(PauseSW, INPUT);
pinMode(CaughtSW, INPUT);
pinMode(OverrideSW, INPUT);
pinMode(VideoDetect,INPUT);

// Enable internal resistors (set inputs as HIGH) on button PINs so we don't have to use external resistors.
digitalWrite(CoinSW, HIGH);
digitalWrite(StartSW, HIGH);
digitalWrite(PauseSW, HIGH);
digitalWrite(OverrideSW, HIGH);
digitalWrite(VideoDetect, HIGH);
digitalWrite(CaughtSW, HIGH);

// Initialize special LCD characters.
lcd.createChar(1, armsUp);

}

void loop() {
  
  switch(state) {
    
    case S_ready:
      F_ready();
    break;
	  
    case S_override:
      F_override();
    break;
      
    case S_coin:
      F_coin();
    break;
      
    case S_limit:
      F_limit();
    break; 
    
    case S_countdown:
      F_countdown();
    break;
    
    case S_choretime:
      F_choretime();
    break;
      
    case S_pause:
      F_pause();
    break;   
   
    case S_caught:
      F_caught();
    break;   
   }
   
}

// Functions //

////////////////////////////////////////////////////////////

void F_ready() { 
  
  // Because the HDMI switch will automatically switch from an inactive input to an active input we have to make sure that the video is off and stays off in every state to prevent abuse.
  while (digitalRead(VideoDetect) == LOW) {
    F_video_off();
  }
  
  LCDBacklight(0, 255, 0); // Green
  lcd.setCursor(4, 0);
  lcd.write(1);
  lcd.print(" READY ");
  lcd.write(1);
  lcd.setCursor(2, 1);
  lcd.print("Insert Coins");
  
  DepositStringIndex = 0; // Reset
  TimeStringIndex = 0; // Reset
  CoinCount = 0; // Reset
  
  // Only state the override can be turned on is here.
  if (digitalRead(OverrideSW) == LOW){
    lcd.clear();
    state = S_override;
  }
  
  //digitalWrite(CoinLED, HIGH);
  
  if (digitalRead(CoinSW) == LOW) {
    lcd.clear();
    state = S_coin;
  }
  
  if (digitalRead(CaughtSW) == LOW) {
    lcd.clear();
    state = S_caught;
  }
  
}

////////////////////////////////////////////////////////////

void F_override() { 
  
  // Turn the video on.
  while (digitalRead(VideoDetect) == HIGH) {
    F_video_on();
  }
  
  if (ClearLCD == true) {
    ClearLCD = false;
    lcd.clear();
  }
  
  OverrideRun = true;
  
  LCDBacklight(255, 0, 0); // Red
  lcd.setCursor(3, 0);
  lcd.print("Override On");
  lcd.setCursor(2, 1);
  lcd.print(" Lucky You ");
  
  //digitalWrite(CoinLED, LOW);
  
  if (digitalRead(OverrideSW) == HIGH){
    lcd.clear();
    state = S_ready;
  }    
  
}

////////////////////////////////////////////////////////////

void F_coin() {
  
  // If it's on, turn the video off.
  while (digitalRead(VideoDetect) == LOW) { 
    F_video_off();
  }
  
  LCDBacklight(0, 0, 255); // Blue
  lcd.setCursor(0, 0);
  lcd.print("Deposited:");
  lcd.print(DepositStrings[DepositStringIndex]);
  lcd.setCursor(0, 1);
  lcd.print("Playtime:");
  lcd.print(TimeStrings[TimeStringIndex]);
  
  // We need to debounce the coin counting switch. Otherwise we'd count several coins when only one was deposited.
  int reading = digitalRead(CoinSW) == LOW;
  
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) { // Logic is inverted since we're using the internal resistors. When button is pressed the PIN goes LOW, when not pressed it's HIGH.
        CoinCount = CoinCount + CoinTimeValue; // Count the coin deposit.
        DepositStringIndex++;
        TimeStringIndex++;
      }
    }
  }
  
  lastButtonState = reading;
  // Done debouncing and depositing.
  
  if (DepositStringIndex == 2) { // We only allow 1 hour of playtime. Player can deposit more coins if they want but the system won't count them after this point.
    lcd.clear();
    //digitalWrite(CoinLED, LOW);
    state = S_limit;
  } 
  
  if (digitalRead(StartSW) == LOW) { // Logic is inverted since we're using the internal resistors. When button is pressed the PIN goes LOW, when not pressed it's HIGH.
    lcd.clear();
    //digitalWrite(CoinLED, LOW);
    AllowedPlaytime = CoinCount * 60000; // Multiply the CoinCount by 60000 (milliseconds) to get the AllowedPlaytime.
    AllowedPlaytime = AllowedPlaytime + VideoSwitchBuffer; // Add some time (VideoSwitchBuffer) to make up for the time it took the TV to sync back up and display video.
    state = S_countdown;
  } 
  
  if (digitalRead(CaughtSW) == LOW) {
    lcd.clear();
    state = S_caught;
  }
  
}

////////////////////////////////////////////////////////////

void F_limit() {
  
  // If it's on, turn the video off.
  while (digitalRead(VideoDetect) == LOW) { 
    F_video_off();
  }
  
  LCDBacklight(255, 0, 0); // Red
  lcd.setCursor(2, 0);
  lcd.print("1 Hour Limit");
  lcd.setCursor(3, 1);
  lcd.print("Press START");
  
  if (digitalRead(StartSW) == LOW) { // Logic is inverted since we're using the internal resistors. When button is pressed the PIN goes LOW, when not pressed it's HIGH.
    lcd.clear();
    AllowedPlaytime = CoinCount * 60000; // Multiply the CoinCount by 60000 (milliseconds) to get the AllowedPlaytime.
    AllowedPlaytime = AllowedPlaytime + VideoSwitchBuffer; // Add some time (VideoSwitchBuffer) to make up for the time it took the TV to sync back up and display video.
    state = S_countdown;
  }
  
  if (digitalRead(CaughtSW) == LOW) {
    lcd.clear();
    state = S_caught;
  }
  
}

////////////////////////////////////////////////////////////

void F_countdown() { 
  
  // Turn the video on.
  while (digitalRead(VideoDetect) == HIGH) {  
    F_video_on();
  }
  
  if (ClearLCD == true) {
    lcd.clear();
    ClearLCD = false;
  }
  
  CountdownRun = true; // Used to get the LCD to clear properly when switching between states.
  PlaytimeRun = true; // Used to set the correct time values to display.
  
  LCDBacklight(0, 0, 255); // Blue
  lcd.setCursor(2, 0);
  lcd.print("Time Started:");
  
  // If second has passed then we subtract that from the AllowedPlaytime
  if (millis() - CurrentMillis > 1000) {
    AllowedPlaytime = AllowedPlaytime - 1000;
    CurrentMillis = millis();
    F_timeDisplay();
    
  // If there is less than 5 minutes of playtime left start sounding an audible alarm at the interval defined.  
    if (AllowedPlaytime <= 300000) { // 5 min
      if (millis() - LastWarningTime > WarningBeepInterval1) {
        F_warning1();
      }
  
  // Or if there is less than 10 seconds of playtime left start sounding an audible alarm at the interval defined. 
    else if (AllowedPlaytime <= 10000) { // 10 seconds
      if (millis() - LastWarningTime > WarningBeepInterval2) {
        F_warning2();
      }
    }
    }
  }

  if (digitalRead(PauseSW) == LOW) { // Logic is inverted since we're using the internal resistors. When button is pressed the PIN goes LOW, when not pressed it's HIGH.
    lcd.clear();
    state = S_pause;
  }
  
  if (AllowedPlaytime == 0) {
    lcd.clear();
    PlaytimeRun = false; 
    ChoreCountdown = ChoreTime * 60000;
    F_warning3();
    state = S_choretime;
  } 
  
  if (digitalRead(CaughtSW) == LOW) {
    lcd.clear();
    state = S_caught;
  }
  
}

////////////////////////////////////////////////////////////

void F_pause() {
  
  // Turn the video off to prevent abuse of the pause.
  while (digitalRead(VideoDetect) == LOW) { 
    F_video_off();
  }
  
  LCDBacklight(255, 0, 0); // Red
  lcd.setCursor(2, 0);
  lcd.print("Time Paused:");
  F_timeDisplay();
   
  if (digitalRead(StartSW) == LOW) { // Logic is inverted since we're using the internal resistors. When button is pressed the PIN goes LOW, when not pressed it's HIGH.
    lcd.clear();
    AllowedPlaytime = AllowedPlaytime + VideoSwitchBuffer; // Add some time (VideoSwitchBuffer) to make up for the time it took to turn the video on.
    state = S_countdown;
  }
  
  if (digitalRead(CaughtSW) == LOW) {
    lcd.clear();
    state = S_caught;
  }
  
}

////////////////////////////////////////////////////////////
 
void F_choretime() { 
  
  while (digitalRead(VideoDetect) == LOW) { 
    F_video_off();
  }
  
  ChoretimeRun = true;
  
  LCDBacklight(255, 0, 0); // Red
  lcd.setCursor(1, 0);
  lcd.write(1);
  lcd.print(" Chore Time ");
  lcd.write(1);

  // If second has passed then we subtract that from the ChoreTime
  if (millis() - CurrentMillis > 1000) {
    ChoreCountdown = ChoreCountdown - 1000;
    CurrentMillis = millis();
    F_timeDisplay();
    
  // If there is less than 5 minutes of ChoreTime left start sounding an audible alarm at the interval defined.  
    if (ChoreCountdown <= 300000) { // 5 min
      if (millis() - LastWarningTime > WarningBeepInterval1) {
        F_warning1();
      }  
  
  // Or if there is less than 10 seconds of ChoreTime left start sounding an audible alarm at the interval defined. 
    else if (ChoreCountdown <= 10000) { // 10 seconds
      if (millis() - LastWarningTime > WarningBeepInterval2) {
        F_warning2();
      }
    }
    }
  }
  
  if (ChoreCountdown == 0) {
    lcd.clear();
    F_warning3();
    state = S_ready; 
  }
  
  if (digitalRead(CaughtSW) == LOW) {
    lcd.clear();
    state = S_caught;
  }
  
}

////////////////////////////////////////////////////////////

void F_timeDisplay() {
  
  lcd.setCursor(4, 1);
  
  if (PlaytimeRun == true) {
    int seconds = AllowedPlaytime / 1000;
    int minutes = seconds / 60;
    int hours = minutes / 60;
      
    seconds = seconds % 60;
    minutes = minutes % 60;
    
    if (hours < 10) {
      lcd.print("0");
      lcd.print(hours);
      lcd.print(":");
      
    if (minutes < 10)
      lcd.print("0");
      lcd.print(minutes);
      lcd.print(":");
        
    if (seconds < 10)      
      lcd.print("0");    
      lcd.print(seconds);
    }
  }
  
  else if (ChoretimeRun == true) {
    int seconds = ChoreCountdown / 1000;
    int minutes = seconds / 60;
    int hours = minutes / 60;
    
    seconds = seconds % 60;
    minutes = minutes % 60;
  
    if (hours < 10) {
      lcd.print("0");
      lcd.print(hours);
      lcd.print(":");
      
    if (minutes < 10)
      lcd.print("0");
      lcd.print(minutes);
      lcd.print(":");
        
    if (seconds < 10)      
      lcd.print("0");    
      lcd.print(seconds);
    }
  }
  
  if (digitalRead(CaughtSW) == LOW) {
    lcd.clear();
    state = S_caught;
  }
  
}

////////////////////////////////////////////////////////////

void F_video_on() {
  
  if (OverrideRun == true) {
    OverrideRun = false;
    lcd.clear();
  }
  
  if (CountdownRun == true) {
    CountdownRun = false;
    lcd.clear();
  }
  
  LCDBacklight(0, 255, 0); // Green
  lcd.setCursor(5, 0);
  lcd.print("Standby");
  lcd.setCursor(0, 1);
  lcd.print("Turning Video On");
  
  unsigned long CurrentMillis = millis();
    
  if(CurrentMillis - SwitchPreviousMillis > VideoSwitchInterval) {
    SwitchPreviousMillis = CurrentMillis;   
      if (VideoSwitchState == LOW)
        VideoSwitchState = HIGH;
      else
        VideoSwitchState = LOW;
    digitalWrite(VideoSW, VideoSwitchState);
  }
  
  ClearLCD = true;
  
  if (digitalRead(CaughtSW) == LOW) {
    lcd.clear();
    state = S_caught;
  }

}

////////////////////////////////////////////////////////////
  
void F_video_off() {
  
  unsigned long CurrentMillis = millis();
    
  if(CurrentMillis - SwitchPreviousMillis > VideoSwitchInterval) {
    SwitchPreviousMillis = CurrentMillis;   
      if (VideoSwitchState == LOW)
        VideoSwitchState = HIGH;
      else 
        VideoSwitchState = LOW;     
    digitalWrite(VideoSW, VideoSwitchState);
  }
  
  if (digitalRead(CaughtSW) == LOW) {
    lcd.clear();
    state = S_caught;
  }
  
}

////////////////////////////////////////////////////////////

void F_warning1() {
  
  tone(Speaker, 500, 1000);
  LastWarningTime = millis();
  
}

////////////////////////////////////////////////////////////

void F_warning2() {
  
  tone(Speaker, 1000, 200);
  LastWarningTime = millis();
  
}

////////////////////////////////////////////////////////////

void F_warning3() {
  
  tone(Speaker, 2000, 1000);
  LastWarningTime = millis();
}

////////////////////////////////////////////////////////////

// Taken from the adafruit LCD turorial
void LCDBacklight(uint8_t r, uint8_t g, uint8_t b) {
  
  // common anode so invert!
  r = map(r, 0, 255, 255, 0);
  g = map(g, 0, 255, 255, 0);
  b = map(b, 0, 255, 255, 0);
  analogWrite(LCDr, r);
  analogWrite(LCDg, g);
  analogWrite(LCDb, b);
  
}

////////////////////////////////////////////////////////////

void F_caught() {
  
  tone(Speaker, 2000);

  LCDBacklight(255, 0, 0); // Red
  lcd.setCursor(2, 0);
  lcd.print("YOU'RE");
  lcd.setCursor(3, 1);
  lcd.print("FUCKED");
}
