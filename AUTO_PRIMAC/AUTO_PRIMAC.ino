#include <FastLED.h>    //importovanie kniznic
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>


/*
#define enA 9  // Arduino pin D9 - CH6
#define in1 8  // D8 - CH5  ---
#define in2 7  // D7 - CH4  ---
#define in3 6  // D6 - CH3  PWM
#define in4 4  // D4 - CH1  PWM
#define enB 5  // D5 - CH2 - PWM*/

//piny pohonu
#define motorPin1   4
#define motorPin2   5

//podsvietenie
#define LED_PIN     7
#define NUM_LEDS    20
int brightness = 128;
CRGB leds[NUM_LEDS];

uint8_t hue = 0;
int ledCount = 0;
int ledMode = 4; 


//vlastny mod podsvietenia
DEFINE_GRADIENT_PALETTE (heatmap_qp){
  0, 0, 0, 0,
  50, 0, 0, 0,
  128, 255, 0, 0,
  205, 0, 0, 0,   
  255, 0, 0, 0
};

CRGBPalette16 myPal = heatmap_qp;
TBlendType    currentBlending;


Servo servo;
#define hornPin 8
#define lightPin A0
boolean light = false;
boolean res = true;

//radiokomunikacia
RF24 radio(3, 2);   // nRF24L01 (CE, CSN)
const byte address[6] = "00001";
unsigned long lastReceiveTime = 0;
unsigned long currentTime = 0;

//max velkost je 32 bytov - NRF24L01 buffer limit
struct Data_Package {
  byte j1PotX;
  byte j1PotY;
  byte j1Button;
  byte j2PotX;
  byte j2PotY;
  byte j2Button;
  byte pot1;
  byte pot2;
  byte tSwitch1;
  byte tSwitch2;
  byte button1;
  byte button2;
  byte button3;
  byte button4;
};

Data_Package data; //premenna zo struktury nad

void setup() {

  
  
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening(); //nastavenie modulu ako primaca
  resetData();
  
  //ovladanie a klakson
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(hornPin, OUTPUT);
  
  servo.attach(6);
  delay(20);
  servo.write(80);
  delay(50);
  
  //podsvietenie
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness);
  FastLED.setCorrection(TypicalLEDStrip);
}
void loop() {
 
  //zistenie ci stale prijmame data
  currentTime = millis();
  if ( currentTime - lastReceiveTime > 1000 ) { //ak sme nepriali data viac ako sekundu
    resetData(); //resetujeme data  a  komunikaciu
  }

  //sledujeme ci tu mame nejake data
  if (radio.available()) {
    radio.read(&data, sizeof(Data_Package)); // precitame ich a zapiseme do  premenej data
    lastReceiveTime = millis();
  }

  //tuy piseme co  ma co robit!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  //led mody
  if(data.button1 == 0){
    ledMode = 1;
  }
  if(data.button2 == 0){
    ledMode = 2;
  }
  if(data.button3 == 0){
    ledMode = 3;
  }
  if(data.button4 == 0){
    ledMode = 4;
  }

  //svetlo
  if((data.j2PotY > 180) and (res == true)){
    if(light == true){
      light = false;
    }else{
      light = true;
    }
    res = false;
  }

  if((data.j2PotY < 180) and (data.j2PotY > 50)){
    res = true;
  }
  
  if(light == true){
    analogWrite(lightPin, 1024);
  }else{
    analogWrite(lightPin, 0);
  }

  //klakson
  if(data.j2PotY < 50){
    digitalWrite(hornPin, HIGH);
  }else{
    digitalWrite(hornPin, LOW);
  }
  
  
  //nastavenie podsvietenia
  FastLED.setBrightness(map(data.pot2, 0, 1023, 25, 255));
  if(ledCount < 4){
    ledCount++;
  } else {
    ledCount = 0;
    hue++;
    switch(ledMode){
      case 1:
        for(int i = 0; i < NUM_LEDS;  i++){
         leds[i] = CHSV(data.pot1, 255, 255);//plna farba
        }
        break;
        
      case 2:
        for(int i = 0; i < NUM_LEDS;  i++){
         leds[i] = CHSV(hue + (i * 2), 255, 255);//farebna paleta
        }
        break;
        
      case 3:
        fill_palette(leds, NUM_LEDS, hue, 255 / NUM_LEDS, myPal, 255, LINEARBLEND);//tocenie
        break;

      case 4:
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        break;

      default:
        fill_solid(leds, NUM_LEDS, CRGB::Black);
        break;
    }
 }
 FastLED.show();

  
  if(data.j1PotY > 134){
    //dopredu
    analogWrite(motorPin1, map(data.j1PotY, 134, 255, 150, 1024));
    analogWrite(motorPin2, 0);
  } else if(data.j1PotY < 100){
    //dozadu
    analogWrite(motorPin2, map(data.j1PotY, 0, 100, 1000, 512));
    analogWrite(motorPin1, 0);
  } else {
    analogWrite(motorPin2, 0);
    analogWrite(motorPin1, 0);
  }
  
  //zatacanie
 servo.write(data.j2PotX);

 
}

void resetData() {
  //resetovanie zakladnych hodnot
  data.j1PotX = 127;
  data.j1PotY = 127;
  data.j2PotX = 80;
  data.j2PotY = 127;
  data.j1Button = 1;
  data.j2Button = 1;
  data.pot1 = 127;
  data.pot2 = 127;
  data.tSwitch1 = 1;
  data.tSwitch2 = 1;
  data.button1 = 1;
  data.button2 = 1;
  data.button3 = 1;
  data.button4 = 0;
}
