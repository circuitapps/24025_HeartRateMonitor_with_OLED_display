/*
  Heart rate monitor

  Operation: This code has been developed and tested on Arduino Nano Every. (It will also work on Arduino Nano without any changes)

  You simply need to connect pin 3 of your Arduino Nano to the 5V pulse output of a photoplethysmographic sensor to count the period between successive
  falling edges of the incoming signal. I build the sensor in the following link and it works beautifully with this software:
  https://embedded-lab.com/blog/introducing-easy-pulse-a-diy-photoplethysmographic-sensor-for-measuring-heart-rate/ You may want to use an alternative
  sensor, which outputs 5V pulses, and that should also work without any issues.

  I will share my heart rate sensor buld videos on my YouTube and TikTok channels soon. So keep an eye on this space for further updates if you are planning
  to build your own heart rate sensor! My step by step instructions will make life easy for you.

  The code takes care of all the visualization aspects on a 0.96 inch OLED display. I used HiLetGo's SSD1306 display for development and testing. You
  can find that part here: www.amazon.com/dp/B06XRBYJR8 You can connect that OLED display over I2C to your Arduino Nano board and it will work
  as intended using the code below.

  You need to install the Adafruit SSD1306 library and its dependencies into your Arduino IDE for using the OLED display above. This is a very easy process and
  you can find the step by step instructions (check Step 5) in this excellent guide: https://www.instructables.com/OLED-I2C-DISPLAY-WITH-ARDUINO-Tutorial/

  The code handles the following cases:
  1 - Display message there is no pulse detected.
  2 - Display message while the pulse is bein gmeasured.
  3 - Display the detected pulse and update it in real time.

  I also experimented with a few bitmap display features in this code. When the board is powered up, you will see a familiar circuitapps themed
  splash screen that will be visible for 2 seconds before the heart rate measurements start. I also created a simple animation that will be played back
  when heart rate acquisition has been completed sucessfully. Feel free to change those bitmap images and animations to your liking. If you are intending to play
  with bitmap images in this code, I strongly suggest checking out the link I provided in "Tips - 2".

  I hope you will find this project compelling and engaging. Enjoy!

  Tips:
  1 - For accessing more OLED drawing and animation library functions, go to Examples > Adafruit SSD1306 > ssd1306_128x64_i2c.
  This assumes the required Adafruit SSD1306 library and its dependencies are already installed in your Arduino development environment.Serial
  2 - Check out https://javl.github.io/image2cpp/ for creating your own bitmap code for directly using in this program!

  circuitapps
  October 2024

  Follow us on:
  www.youtube.com/@circuitapps
  www.tiktok.com/@circuitappschannel
  www.instagram.com/youtubecircuitapps
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdio.h>

//#define FIND_I2C_ADDRESS // uncomment to find address

#ifdef FIND_I2C_ADDRESS
void i2c_address_search()
{
  Serial.println ("I2C scanner. Scanning ...");
  byte count = 0;
  for (byte i = 1; i < 120; i++)
  {
    Wire.beginTransmission (i);
    if (Wire.endTransmission () == 0)
      {
      Serial.print ("Found address: ");
      Serial.print (i, DEC);
      Serial.print (" (0x");
      Serial.print (i, HEX);
      Serial.println (")");
      count++;
      delay (1); 
      } 
  } 
  Serial.println ("Done.");
  Serial.print ("Found ");
  Serial.print (count, DEC);
  Serial.println (" device(s).");
}
#endif

#define OLED_128x64_I2C_ADDR 0x3C  // Found by running FIND_I2C_ADDRESS
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS OLED_128x64_I2C_ADDR ///< See datasheet for Address; 0x3C for 128x64, 0x3D for 128x32
#define HR_INPUT_PIN 3  // D3 pin used as heart rate input pin with interrupt (Arduino Nano and Nano Every compatible!)

#define STARTUP_MESSAGE_STRING "Awaiting first pulse"
#define STARTUP_MESSAGE_FONTSIZE (1)
#define STARTUP_MESSAGE_YPOS (SCREEN_HEIGHT/2 - 8)
#define STARTUP_MESSAGE_XPOS (0)

#define WAIT_MESSAGE_STRING "Initializing..."
#define WAIT_MESSAGE_FONTSIZE (1)
#define WAIT_MESSAGE_YPOS (SCREEN_HEIGHT/2 - 8)
#define WAIT_MESSAGE_XPOS (25)

#define NOPULSE_MESSAGE_STRING "No pulse..."
#define NOPULSE_MESSAGE_FONTSIZE (1)
#define NOPULSE_MESSAGE_YPOS (SCREEN_HEIGHT/2 - 8)
#define NOPULSE_MESSAGE_XPOS (20)

#define AWAITING_MESSAGE_STRING "Awaiting pulse..."
#define AWAITING_MESSAGE_FONTSIZE (1)
#define AWAITING_MESSAGE_YPOS (SCREEN_HEIGHT/2 - 8)
#define AWAITING_MESSAGE_XPOS (20)

#define PULSE_AWAIT_STATE_MSEC_DELAY ((unsigned long)1500)  // If ISR call exceeds this, we enter the pulse await state
#define MAX_MSEC_DELAY_BETWEEN_READINGS (PULSE_AWAIT_STATE_MSEC_DELAY + (unsigned long)1500)  // If ISR call exceeds this, we enter the No pulse state

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

unsigned long last_time_msec = 0;
volatile unsigned long current_time_millis;  // Updated within the ISR. Hence volatile.
volatile bool update_display = false;
bool heart_flag = true;  // flag controls heart bitmap animation

// 'heart_beating_1', 20x20px
#define HEART_HEIGHT   20  // circuitapps logo height
#define HEART_WIDTH    20  // circuitapps logo width
static const unsigned char PROGMEM heart_on[] = {
	0x01, 0x01, 0x00, 0x1c, 0x0c, 0x00, 0x26, 0x22, 0x40, 0x21, 0x98, 0x80, 0x08, 0x30, 0xa0, 0x42, 
	0x30, 0x20, 0x10, 0xb1, 0x00, 0x00, 0x70, 0x40, 0x22, 0x51, 0x20, 0x07, 0x53, 0x00, 0xfd, 0x53, 
	0xf0, 0x41, 0x5e, 0x20, 0x01, 0xcc, 0x00, 0x01, 0x8c, 0x00, 0x08, 0x00, 0x00, 0x00, 0x0c, 0x00, 
	0x00, 0x40, 0x00, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00
};

static const unsigned char PROGMEM heart_off[] = {
	0x2a, 0x05, 0x00, 0x15, 0x15, 0x00, 0xaa, 0xaa, 0x80, 0x25, 0x28, 0x40, 0x90, 0xa4, 0xa0, 0x0a, 
	0x31, 0x20, 0x51, 0x44, 0x40, 0x04, 0x21, 0x20, 0xa1, 0x48, 0xa0, 0x14, 0x12, 0x40, 0xc2, 0xc1, 
	0x70, 0x68, 0x54, 0xa0, 0x02, 0x85, 0x00, 0x11, 0x28, 0x00, 0x0a, 0x44, 0x00, 0x00, 0x14, 0x00, 
	0x01, 0x48, 0x00, 0x00, 0x10, 0x00, 0x00, 0x80, 0x00, 0x00, 0x20, 0x00
};

#define LOGO_HEIGHT   64  // circuitapps logo height
#define LOGO_WIDTH    75  // circuitapps logo width
static const unsigned char PROGMEM circuitapps_logo[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x10, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0xc6, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x08, 0x88, 0x40, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x20, 0x03, 0xd0, 0x20, 0x92, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 
	0xe1, 0x20, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xe0, 0x08, 0x00, 0x40, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x1f, 0xe4, 0x82, 0x40, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xe1, 0x24, 
	0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xe0, 0x41, 0x20, 0x14, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x3f, 0xe4, 0x51, 0xc9, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xc0, 0x0c, 0x20, 0x62, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xc8, 0x81, 0x80, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 
	0xc8, 0x00, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x80, 0x24, 0xc0, 0x02, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0xff, 0x30, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0x02, 0x00, 
	0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xfe, 0x48, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x06, 0xfc, 0x00, 0x10, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x70, 0x20, 0x40, 0x3e, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x08, 0x00, 0x7f, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 
	0xd0, 0x09, 0x74, 0x42, 0x60, 0x00, 0x00, 0x00, 0x00, 0x81, 0x96, 0x64, 0x01, 0x12, 0x0c, 0x00, 
	0x00, 0x00, 0xfc, 0xc1, 0x20, 0x80, 0x88, 0x10, 0x80, 0x00, 0x00, 0x00, 0xfe, 0x03, 0x29, 0x12, 
	0x20, 0x84, 0xb1, 0x80, 0x00, 0x01, 0xff, 0x28, 0x02, 0x40, 0x00, 0x21, 0x08, 0x40, 0x00, 0x01, 
	0xff, 0x00, 0x80, 0x99, 0x09, 0x09, 0x4a, 0x60, 0x00, 0x01, 0xf8, 0x12, 0x20, 0x20, 0x20, 0x40, 
	0x21, 0x20, 0x00, 0x0f, 0x80, 0x80, 0x0e, 0xfe, 0x80, 0x12, 0x24, 0x20, 0x00, 0x30, 0x24, 0x2f, 
	0xfe, 0x1f, 0x04, 0x00, 0x88, 0x80, 0x01, 0x80, 0x01, 0x2f, 0xf1, 0xed, 0x3e, 0x04, 0x08, 0x00, 
	0x2e, 0x08, 0x88, 0x0b, 0x5f, 0xf7, 0x3f, 0x11, 0x22, 0x20, 0x10, 0x40, 0x02, 0x77, 0xf9, 0x3b, 
	0x9d, 0x00, 0x00, 0x00, 0x61, 0x04, 0x10, 0xd6, 0xee, 0x6e, 0xbf, 0x22, 0x4c, 0x00, 0x48, 0x20, 
	0x40, 0x53, 0xe6, 0xde, 0xee, 0x00, 0x00, 0x00, 0x02, 0x02, 0x00, 0xd9, 0xf9, 0xf7, 0x3a, 0x44, 
	0x92, 0x00, 0x20, 0x90, 0x80, 0xbd, 0xbf, 0x7d, 0xfc, 0x10, 0x08, 0x00, 0x24, 0x00, 0x22, 0xe7, 
	0xef, 0xde, 0xcc, 0x80, 0x40, 0x00, 0x19, 0x09, 0x08, 0xff, 0xf6, 0xf7, 0x7c, 0x08, 0x00, 0x00, 
	0x00, 0x00, 0x80, 0xff, 0x77, 0xfd, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0xfb, 0xdd, 0xee, 
	0xe8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xff, 0x7b, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x77, 0xf7, 0xdf, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x4c, 0xdc, 0x80, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x79, 0x01, 0x2e, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7a, 
	0x6f, 0xaa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3a, 0xff, 0xec, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x3b, 0xfe, 0x6c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x7b, 0xec, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1d, 0xce, 0xda, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x0f, 0xed, 0xda, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xa7, 0x68, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xd7, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x03, 0xf7, 0x80, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0xf6, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xdb, 0x20, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


void setup()
{

  #ifdef FIND_I2C_ADDRESS
    Wire.begin();
    Serial.begin (115200);
    while (!Serial)
    {
    }

    Serial.println ();
    pinMode(13,OUTPUT); 
    digitalWrite(13,HIGH);  

    i2c_address_search();  //search for a valid I2C address
  #else
    pinMode(HR_INPUT_PIN, INPUT_PULLUP);  // The falling edge on this pin will be what we will track.
    attachInterrupt(digitalPinToInterrupt(HR_INPUT_PIN), ISR_HeartRate_signal_change, CHANGE);  // Rising and falling edges will trigger this interrupt

    display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

    // Display circuitapps logo at startup:
    display.clearDisplay();  // overrides built in logo display on module.
    display.display();
    display.drawBitmap(20, 0, circuitapps_logo, LOGO_WIDTH, LOGO_HEIGHT, SSD1306_WHITE);
    display.display(); // Show the display buffer on the screen
    delay(2000);        // Pause for 0.5 second

    // Clear the buffer
    display.clearDisplay();
    display_text(WAIT_MESSAGE_FONTSIZE, WAIT_MESSAGE_XPOS, WAIT_MESSAGE_YPOS, WAIT_MESSAGE_STRING);
    display.display();
  #endif

} 

void loop()
{

  #ifndef FIND_I2C_ADDRESS  // Normal operation once I2C address of the display is known.
    long elapsed_millis;
    unsigned long last_sample_instance_ms = millis();
    bool no_pulse_state = false; // pulse absence is NOT confirmed yet

    while(1)  // run forever
    {

      if(update_display)
      { 
        if(last_time_msec > 0)
        {
          // After power up, at least ONE interrupt driven time update was achieved.
          float bpm = (float)(current_time_millis - last_time_msec) / 1000.0;
          bpm = 60 / bpm;  // conversion to beats per minute
          display_heart_rate(3, 10, SCREEN_HEIGHT / 2, bpm);
          last_sample_instance_ms = millis();  // To keep track of when the last sampling occurred.
          no_pulse_state = false;  // No pulse flag toggled
        }

        last_time_msec = current_time_millis;  // prep for next calculation after the interrupt
        update_display = false;  // ISR will update this
      }
      else
      {// display will not be updated with heart rate data

        if(!no_pulse_state)
        {
          if(last_time_msec == 0)
          {// System just started
            display_text(STARTUP_MESSAGE_FONTSIZE, STARTUP_MESSAGE_XPOS, STARTUP_MESSAGE_YPOS, STARTUP_MESSAGE_STRING);
            last_time_msec = 1;  // dummy update for leaving this state until next start up cycle.
          }

          elapsed_millis = millis() - last_sample_instance_ms;

          if (elapsed_millis > MAX_MSEC_DELAY_BETWEEN_READINGS)
          {// ISR trigger took too long. No pulse state entered.
            no_pulse_state = true;  // no pulse state confirmed. Displayed message will persist until pulse detected.
            display_text(NOPULSE_MESSAGE_FONTSIZE, NOPULSE_MESSAGE_XPOS, NOPULSE_MESSAGE_YPOS, NOPULSE_MESSAGE_STRING);
          }
          else
          {
            if(elapsed_millis > PULSE_AWAIT_STATE_MSEC_DELAY)
            {// ISR trigger took longer than expected. Awaiting pulse state entered.
              display_text(AWAITING_MESSAGE_FONTSIZE, AWAITING_MESSAGE_XPOS, AWAITING_MESSAGE_YPOS, AWAITING_MESSAGE_STRING);
            }
            //else no state change yet. Let's wait a little more.
          }

        }
      }

    }// end while(1)
  
  #endif

}

void display_text(uint8_t textSize, uint8_t x_pos, uint8_t y_pos, char *string)
{
  display.clearDisplay();
  display.setTextSize(textSize); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x_pos, y_pos);
  display.println(string);  // Display character string
  display.display();      // Show initial text
}

void display_heart_rate(uint8_t textSize, uint8_t x_pos, uint8_t y_pos, float bpm)
{
  char strBuf[20];


  display.clearDisplay();
  display.setTextSize(textSize); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x_pos, y_pos);
  //display.println(bpm, 0);  // Second digit is the precision for the numbers displayed! (by default, it assumes 2 decimal places!)
  sprintf(strBuf, "%d bpm", (int)bpm);
  display.println(strBuf);  // Second digit is the precision for the numbers displayed! (by default, it assumes 2 decimal places!)

  if(heart_flag == true)
  {
    display.drawBitmap(50, 2, heart_on, HEART_WIDTH, HEART_HEIGHT, SSD1306_WHITE);
    heart_flag = false;
  }
  else
  {
    display.drawBitmap(50, 2, heart_off, HEART_WIDTH, HEART_HEIGHT, SSD1306_WHITE);
    heart_flag = true;
  }

  display.display();      // Show initial text
}

void ISR_HeartRate_signal_change()
{
  // Will be called when there is a falling edge/rising edge (depending on the pin configuration) detected

  if(digitalRead(HR_INPUT_PIN) == LOW)
  { // We are only interested in the falling edge since input pin is declared as pull up
    current_time_millis = millis();  // Grab a snapshot of milliseconds passed since board power up (wraps around after about 49 days of operation!)
    update_display = true;  // new reading triggers display update
  }
}
