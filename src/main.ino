#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <FastLED.h>
#include <FastLEDPainter.h>


const int duration = 20000; //number of loops to run each animation for

//#define NUMBEROFPIXELS 170 //Number of LEDs on the strip
#define NUMBEROFPIXELS 12*9 //Number of LEDs on the strip
#define DATA_PIN D1 //Pin where WS281X pixels are connected
#define CLOCK_PIN D2
CRGB leds[NUMBEROFPIXELS];

int recibe = 0;

//create one canvas and one brush with global scope
FastLEDPainterCanvas pixelcanvas = FastLEDPainterCanvas(NUMBEROFPIXELS); //create canvas, linked to the FastLED library (canvas must be created before the brush)
FastLEDPainterBrush pixelbrush = FastLEDPainterBrush(&pixelcanvas); //crete brush, linked to the canvas to paint to

// network ettings --------------
//

char ssid[] = "C3P";//"TC";
char psswd[] = "trespatios";//"chOc0l4t1n4";
IPAddress ip(192, 168, 0, 204);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiUDP Udp;
const IPAddress outIp(192,168,0,131);
const unsigned int localPort = 4001;

// osc settings
OSCErrorCode error;



void setup(){
  //initilize FastLED library
  FastLED.addLeds<LPD8806, DATA_PIN, CLOCK_PIN, GRB>(leds, NUMBEROFPIXELS);

  Serial.begin(115200);
  Serial.println(" ");
  Serial.println(F("FastLED Painter simple demo"));

  //check if ram allocation of brushes and canvases was successful (painting will not work if unsuccessful, program should still run though)
  //this check is optional but helps to check if something does not work, especially on low ram chips like the Arduino Uno
  if (pixelcanvas.isvalid() == false) Serial.println(F("canvas allocation problem (out of ram, reduce number of pixels)"));
  else  Serial.println(F("canvas allocation ok"));

  if (pixelbrush.isvalid() == false) Serial.println(F("brush allocation problem"));
  else  Serial.println(F("brush allocation ok"));

  //initialize the animation, this is where the magic happens:

  CHSV brushcolor; //the brush and the canvas operate on HSV color space only
  brushcolor.h = 68; //zero is red in HSV. Library uses 0-255 instead of 0-360 for colors (see https://en.wikipedia.org/wiki/HSL_and_HSV)
  brushcolor.s = 0; //full color saturation
  brushcolor.v = 255; //about half the full brightness

  pixelbrush.setSpeed(3000); //set the brush movement speed (4096 means to move one pixel per update)
  pixelbrush.setColor(brushcolor); //set the brush color
  pixelbrush.setFadeSpeed(255); //fading speed of pixels (255 is maximum fading speed)
  pixelbrush.setFadeout(true); //do brightness fadeout after painting
  pixelbrush.setBounce(false); //bounce the brush when it reaches the end of the strip

  //this sets up the brush to paint pixels in red, the pixels fade out after they are painted (the brush is the size of one pixel and can only one pixel per brush update, see other examples to paint multiple pixels at once)

  // Static IP Setup Info Here...
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid,psswd);

  while(WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");

  }

  Serial.println("wifi connected");
  Serial.println(WiFi.localIP());
  Serial.println("starting udp");
  Udp.begin(localPort);


}

bool available = true;

//--------- osc message listener 
void listen_osc_messages(OSCMessage &inc_msg) {
  recibe = inc_msg.getInt(0);
  if(recibe == 1) available = false;
  Serial.print(recibe);
  Serial.println(" recibe");
}

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 500;    // the debounce time; increase if the output flickers

void clearLeds () {
  for(int i=0; i < NUMBEROFPIXELS; i++) leds[i] = CRGB::Black;
  FastLED.show();
}

//-----------
void loop() {

  if(available && recibe == 0){  // incoming osc messages are dispatched 
    // to the listener

    OSCMessage inc_msg;
    int sizeOfmsg = Udp.parsePacket();

    if (sizeOfmsg > 0) {
      while (sizeOfmsg--) {
        inc_msg.fill(Udp.read());
      }
      if (!inc_msg.hasError()) {
        inc_msg.dispatch("/gesto/1",listen_osc_messages );
      } else {
        error = inc_msg.getError();
        Serial.print("error: ");
        Serial.println(error);
      }
    }
  }

  FastLED.clear(); //always need to clear the pixels, the canvas' colors will be added to whatever is on the pixels before calling a canvas update

  if (recibe == 1 && (millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:
    recibe = 0;
    available = true;
    lastDebounceTime = millis();
    clearLeds();
    Serial.println("ENTRA");
  }

  if(recibe == 1){
    pixelbrush.paint(); //paint the brush to the canvas (and update the brush, i.e. move it a little)
    pixelcanvas.transfer(); //transfer the canvas to the LEDs

    FastLED.show();
  }

  yield();
}



