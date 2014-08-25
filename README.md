Arduino_MicroView_OAuth_TOTP
============================

Arduino MicroView Time-based One-time Password (TOTP) Token Generator

Tested with Google Authenticator. Works well, enjoy.

** Reqired Libraries **

MicroView  http://learn.microview.io/ 
sha1  For HMAC-SHA1 hash this implementation uses the code from Cathedrow / Cryptosuite ( https://github.com/Cathedrow/Cryptosuite ). However a small change was added to sha1.h and sha1.c: The method size_t Sha1Class::writebytes(const uint8_t* data, int length)
TOTP  https://github.com/lucadentella/ArduinoLib_TOTP 
Time  http://www.pjrc.com/teensy/td_libs_Time.html 
DS1307RTC  http://www.pjrc.com/teensy/td_libs_DS1307RTC.html
