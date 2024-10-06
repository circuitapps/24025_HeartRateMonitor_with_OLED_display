# 24025 - PHOTOPLETHYSMOGRAPHY HEART RATE MONITOR WITH ARDUINO NANO
---
## Description
 This project is designed to help you build your own photoplethysmography heart rate sensor and the Arduino Nano code to create an OLED display to monitor heart rate in real time. You can watch a quick video of the project here:
        
![2][]
 
## Operation
 
This code has been developed and tested on Arduino Nano Every. (It will also work on Arduino Nano without any changes)

You simply need to connect pin 3 of your Arduino Nano to the 5V pulse output of a photoplethysmographic sensor to count the period between successive falling edges of the incoming signal. I build the sensor in **[this link][1]** and it works beautifully with the software here.

You may also use an alternative sensor, which outputs 5V pulses, and that should also work without any issues.

I will share my heart rate sensor buld videos on my YouTube and TikTok channels soon. So keep an eye on this space for further updates if you are planning to build your own heart rate sensor! My step by step instructions will make life easy for you.

The code takes care of all the visualization aspects on a 0.96 inch OLED display. I used HiLetGo's SSD1306 display for development and testing. You can find the **[OLED display here][3]**. You can connect that OLED display over I2C to your Arduino Nano board and it will work as intended using the code below.

You also need to install the Adafruit SSD1306 library and its dependencies into your Arduino IDE for using the OLED display above. This is a very easy process and you can find the step by step instructions (check Step 5) in **[this excellent guide][4]**.

The code handles the following cases:
1 - Display message there is no pulse detected.
2 - Display message while the pulse is bein gmeasured.
3 - Display the detected pulse and update it in real time.

I also experimented with a few bitmap display features in this code. When the board is powered up, you will see a familiar circuitapps themed splash screen that will be visible for 2 seconds before the heart rate measurements start. I also created a simple animation that will be played back when heart rate acquisition has been completed sucessfully. Feel free to change those bitmap images and animations to your liking. If you are intending to play
with bitmap images in this code, I strongly suggest checking out the link I provided in "Tips - 2".

I hope you will find this project compelling and engaging. Enjoy!

## Useful Tips
 
1. For accessing more OLED drawing and animation library functions, in your Arduino IDE go to Examples > Adafruit SSD1306 > ssd1306_128x64_i2c.
***This assumes the required Adafruit SSD1306 library and its dependencies are already installed in your Arduino development environment!***
2. Check out **[this free online tool][5]** for easily creating your own bitmap code for a copy & paste use in this program!


*GOOD LUCK & ENJOY EXPERIMENTING WITH OUR HEART RATE MONITOR!*


---
*Follow our channel on **[YouTube][6]**, **[TikTok][7]** and **[Instagram][8]** for hundreds of engaging build video tutorials:*

---

***circuitapps* - October 2024**

[1]: https://embedded-lab.com/blog/introducing-easy-pulse-a-diy-photoplethysmographic-sensor-for-measuring-heart-rate
[2]: ./HeartRate_Monitor_320x240_final.gif
[3]: www.amazon.com/dp/B06XRBYJR8
[4]: https://www.instructables.com/OLED-I2C-DISPLAY-WITH-ARDUINO-Tutorial/
[5]: https://javl.github.io/image2cpp/
[6]: www.youtube.com/@circuitapps
[7]: www.tiktok.com/@circuitappschannel
[8]: www.instagram.com/youtubecircuitapps
