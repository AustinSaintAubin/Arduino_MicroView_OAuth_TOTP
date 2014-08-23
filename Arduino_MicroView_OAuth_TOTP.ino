/*===========================================================================
  Title: OAuth Token Generator for MicroView
   - Gernerate OAuth Authentication Token, like the tokens used for Google Authenticator, on the Arduino MicroView.
  
  Version: 001
  Date: 2014 / 08 / 23
  Author: Austin St. Aubin
  Email: AustinSaintAubin@gmail.com
  
  Notes:
    Uses Real Time Clock Module DS1307, but can be easly changes to somthing else
    Tokens are genrerated every 30 secionds.
  ============================================================================= */
// [# Included Library's #]
#include <MicroView.h>  // http://learn.microview.io/ 
#include <sha1.h>  // For HMAC-SHA1 hash this implementation uses the code from Cathedrow / Cryptosuite ( https://github.com/Cathedrow/Cryptosuite ). However a small change was added to sha1.h and sha1.c: The method size_t Sha1Class::writebytes(const uint8_t* data, int length)
#include "TOTP.h"  // https://github.com/lucadentella/ArduinoLib_TOTP 
#include <Time.h>  // http://www.pjrc.com/teensy/td_libs_Time.html 
#include <Wire.h>
#include <DS1307RTC.h>  // http://www.pjrc.com/teensy/td_libs_DS1307RTC.html

// [# Helper Macros #]
//#define LINE(name,val) uView.print(name); uView.print(":\t"); Serial.println(val);  // LINE("F_CPU", F_CPU);

// [# Preprocessor Statements #]
#define SERIAL_OUTPUT false  // Output statments over serial

// [# Global Pin Constants #]
//const int LEDPin = 13;  // LED pin for flashing or error.

// [# Global Constants #]
// Fill this with your own secret as described on http://blog.jfedor.org/2013/02/google-authenticator-watch.html 
// ======================================
// Python Code:
// import base64
// print ', '.join([hex(ord(i)) for i in base64.b32decode('YCAV7MGYKYLILAHJ')])
// ======================================
static char    HMAC_KEY_1_DESCRIPTION[4] = { "GOG" };
static uint8_t HMAC_KEY_1[]={ 0xbd, 0xc2, 0xaa, 0xfe, 0x1c, 0xd9, 0x53, 0x12, 0x7f, 0x75, 0xd4, 0x98, 0x55, 0xdd, 0x1c, 0x83, 0xce, 0x7a, 0xd9, 0x63 }; // XXBKV7Q43FJRE73V2SMFLXI4QPHHVWLD
static char    HMAC_KEY_2_DESCRIPTION[4] = { "NAS" };
static uint8_t HMAC_KEY_2[]={ 0xc0, 0x81, 0x5f, 0xb0, 0xd8, 0x56, 0x16, 0x85, 0x80, 0xe9  };  // YCAV7MGYKYLILAHJ
static char    HMAC_KEY_3_DESCRIPTION[4] = { "OCA" };
static uint8_t HMAC_KEY_3[]={ 0xbd, 0xc2, 0xaa, 0xfe, 0x1c, 0xd9, 0x53, 0x12, 0x7f, 0x75, 0xd4, 0x98, 0x55, 0xdd, 0x1c, 0x83, 0xce, 0x7a, 0xd9, 0x63 }; // XXBKV7Q43FJRE73V2SMFLXI4QPHHVWLD
// - - - - - - - - - - - - - - - - - - - - - - - - -
const int  TIME_ZONE = -5;  // Your RTC's Timezone / your timezone when you set the RTC.
const long TIME_ZONE_GMT_OFFSET = (TIME_ZONE * -1) * 3600;  // The the offset needed to render the time in GMT, the Tocken is genertated using GMT time.
const int  TIME_OFFSET_GENERAL = 1;  // General time offset for inacurate moduals or calculation times

// [# Global Variables #]

// [# Global Class Constructors #]
MicroViewWidget *widget[1];    // declaring an array of 4 MicroViewWidget
// - - - - - - - - - - - - - - - - - - - - - - - - -
TOTP oauth_totp_1 = TOTP(HMAC_KEY_1, sizeof(HMAC_KEY_1)/sizeof(uint8_t));  // Initialize OAuth TOTP Token Generator
TOTP oauth_totp_2 = TOTP(HMAC_KEY_2, sizeof(HMAC_KEY_2)/sizeof(uint8_t));  // Initialize OAuth TOTP Token Generator
TOTP oauth_totp_3 = TOTP(HMAC_KEY_3, sizeof(HMAC_KEY_3)/sizeof(uint8_t));  // Initialize OAuth TOTP Token Generator

void setup()  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
{ // put your setup code here, to run once:
  
  // Setup MicroView
  uView.begin();    // init and start MicroView
  uView.clear(PAGE);  // erase the memory buffer, when next uView.display() is called, the OLED will be cleared.
  widget[0] = new MicroViewSlider(0,40,0,30, WIDGETSTYLE1);  // declare widget0 as a Slider, used to show when token will expire
  
  // Set Font
  uView.setFontType(0);
  
  // Begin RTC Clock Class
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
//  setSyncInterval(3600)  // Configure how often the getTimeFunction is called.
  adjustTime(TIME_OFFSET_GENERAL);  // Time to adjust for calculations and lag as to make the time accurate.
  
//  // Check if RTC is connected and working
//  if(timeStatus()!= timeNeedsSync) 
//    {
//      uView.setCursor(0,0); 
//      uView.print("RTC Error!");
//      uView.display();
//      delay(4000);
//      uView.clear(PAGE);
//    }
}

void loop()  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
{ // put your main code here, to run repeatedly:
  
  // Render Clock
  uView.setCursor(0,0); 
  printDigits(hour());uView.print(":");printDigits(minute());uView.print(":");printDigits(second());  // Display current time
  
  // Render Clock Lines
  uView.rectFill(0,9,64,3, LOW, LOW);  // Clear old clock lines
  uView.line(0,9,map(second(),0,60,0,64),9);
  uView.line(0,10,map(minute(),0,60,0,64),10);
  uView.line(0,11,map(hour(),0,24,0,64),11);
  
  // [ Generate & Output OAuth Tocken / Hash & Experation Counter ]
  // Varables
  byte token_expiration_time = second() % 30;  // Tokens expire every 30 seconds
  static byte token_expiration_time_last = 100;
  
  // Check if new token is ready to be generated
  if (token_expiration_time >= token_expiration_time_last) {
    // Render Widget for Token Experation Time
    widget[0]->setValue(token_expiration_time);  // Tokens expire every 30 seconds
  } else {
    // Gernerate & Display new tokens
      // Token 1
      uView.setCursor(0,14);
      uView.print(HMAC_KEY_1_DESCRIPTION); uView.print(":"); uView.print(oauth_totp_1.getCode(now() + TIME_ZONE_GMT_OFFSET));  // Render Token from unix time (GMT)
      // Token 2
      uView.setCursor(0,23);
      uView.print(HMAC_KEY_2_DESCRIPTION); uView.print(":"); uView.print(oauth_totp_2.getCode(now() + TIME_ZONE_GMT_OFFSET));  // Render Token from unix time (GMT)
      // Token 3
      uView.setCursor(0,32);
      uView.print(HMAC_KEY_3_DESCRIPTION); uView.print(":"); uView.print(oauth_totp_3.getCode(now() + TIME_ZONE_GMT_OFFSET));  // Render Token from unix time (GMT)
      
      // Fade Slider back to starting position
      for(int i=30; i>=token_expiration_time;i--) {
        widget[0]->setValue(i);
        uView.display();
        delay(28);
      }
  }
  
  // Update token time
  token_expiration_time_last = token_expiration_time;
  
  // Render Microview Screen
  uView.display();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// -- Function [ Print Digits with Leading Zero ]   Utility function for digital clock display: prints preceding colon and leading 0
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void printDigits(int digits){
  if(digits < 10)
    uView.print('0');
  uView.print(digits);
}
