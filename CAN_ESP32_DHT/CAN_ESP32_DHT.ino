// Example testing sketch for various DHT humidity/temperature sensors written by ladyada
// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor
#include <ESP32CAN.h>
#include <CAN_config.h>
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h> //Jika protocol tidak terdeteksi
#include <ir_Panasonic.h> //Protocol Panasonic (lihat library untuk protocol remote lain)
#include "DHT.h"

//Pin IRLed TX
const uint16_t kIrLed = 19; //D2 - GPIO4

#define DHTPIN 4    // Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);
CAN_device_t CAN_cfg;

IRPanasonicAc ac(kIrLed);  
IRsend irsend(kIrLed);

int temp = 16;
int pushFan = 0;
int pushSwing = 0;
int notifFan, notifSwing;

void setup() {
  ac.begin();
  irsend.begin();
  Serial.begin(115200);
  dht.begin();

  CAN_cfg.speed=CAN_SPEED_250KBPS;
  CAN_cfg.tx_pin_id = GPIO_NUM_5;
  CAN_cfg.rx_pin_id = GPIO_NUM_18;
  CAN_cfg.rx_queue = xQueueCreate(10,sizeof(CAN_frame_t));
  //start CAN Module
  ESP32Can.CANInit();
}

void loop() {
  // Wait a few seconds between measurements.
  //delay(2000);
  CAN_frame_t rx_frame;

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  // Serial.print(F("Humidity: "));
  // Serial.print(h);
  // Serial.print(F("%  Temperature: "));
  // Serial.print(t);
  // Serial.print(F("°C "));
  
  if(xQueueReceive(CAN_cfg.rx_queue,&rx_frame, 3*portTICK_PERIOD_MS)==pdTRUE){
    if(rx_frame.FIR.B.FF == CAN_frame_ext) {
      // Print the extended ID and DLC
      printf("Extended ID: 0x%.8lX  DLC: %1d  Data:", (rx_frame.MsgID & 0x1FFFFFFF), rx_frame.FIR.B.DLC);
      // Print the data bytes
      for(int i = 0; i < rx_frame.FIR.B.DLC; i++) {
          printf(" 0x%02X", rx_frame.data.u8[i]);
      }
      printf("\n");
    }
    
//ac on
    if((rx_frame.MsgID & 0x1FFFFFFF) == 0x18FEA5FE) {
      ac.on();
      ac.setTemp(temp);
      ac.send();
    } 
    
//ac off
    else if((rx_frame.MsgID & 0x1FFFFFFF) == 0x18FEA6FE) {
      ac.off();
      ac.send();
    }
    
//suhu naik
    else if((rx_frame.MsgID & 0x1FFFFFFF) == 0x18FCB1FE) {
      temp++;
      if(temp > 30) {
        temp = 30;
    }
    ac.send();
          
    Serial.println("Suhu Naik: ");
    Serial.print(temp);
    Serial.print(" derajat Celsius");
    Serial.println();
    }
    
//suhu turun
    else if((rx_frame.MsgID & 0x1FFFFFFF) == 0x18FBB2FE) {
      temp--;
      if(temp < 16) {
        temp = 16;
      }
      ac.send();
          
      Serial.println("Suhu Turun: ");
      Serial.print(temp);
      Serial.print(" derajat Celsius");
     Serial.println();
    }
    
//fanspeed
    else if((rx_frame.MsgID & 0x1FFFFFFF) == 0x18FEBDFE) {
    pushFan++;
    if (pushFan > 3) {
      pushFan = 0;
    }
    if(pushFan==1) {
      notifFan = 0; //Quiet
      Serial.println();
      Serial.print("-   ");
      Serial.println();
      //ac.setFan(0);
    }
    else if(pushFan==2) {
      notifFan = 2; //Medium
      Serial.println();
      Serial.println("---  ");
      Serial.println();
            //ac.setFan(2);
    }
    else if(pushFan==3) {
      notifFan = 3; //Max
      Serial.println();
      Serial.println("----- ");
      Serial.println();
            //ac.setFan(3);
    }
    else {
      notifFan = 7; //Auto
      Serial.println();
      Serial.println("AUTO");
      Serial.println();
      //ac.setFan(7);
    }
    ac.setFan(notifFan);
    ac.send();
    }
    
//air swing
    else if((rx_frame.MsgID & 0x1FFFFFFF) == 0x18F1FBFE) {
    pushSwing++;
    if (pushSwing>5) {
      pushSwing=0;
    }
    if(pushSwing==1) {
      notifSwing=1; //"highest"
      Serial.println();
      Serial.print("1   ");
      Serial.println();
      //ac.setFan(0);
    }
    else if(pushSwing == 2) {
      notifSwing = 2; //high
      Serial.println();
      Serial.println("2  ");
      Serial.println();
            //ac.setFan(2);
    }
    else if(pushSwing == 3) {
      notifSwing = 3; //midle
      Serial.println();
      Serial.println("3 ");
      Serial.println();
            //ac.setFan(3);
    }
    else if(pushSwing == 4) {
      notifSwing = 4; //low
      Serial.println();
      Serial.println("4 ");
      Serial.println();
            //ac.setFan(3);
    }
    else if(pushSwing == 5) {
      notifSwing = 5; //lowest
      Serial.println();
      Serial.println("5 ");
      Serial.println();
            //ac.setFan(3);
    }    
    else {
      notifSwing = 15; //Auto
      Serial.println();
      Serial.println("AUTO");
      Serial.println();
      //ac.setFan(7);
    }
    ac.setSwingVertical(notifSwing);
    ac.send();
    }

  }

  else
  {
      rx_frame.FIR.B.FF = CAN_frame_ext;
      rx_frame.MsgID = 0x1CFE8CFE;
      rx_frame.FIR.B.DLC = 8;
      rx_frame.data.u8[0] = h;
      rx_frame.data.u8[1] = t;
      rx_frame.data.u8[2] = 0xAA;
      rx_frame.data.u8[3] = 0xBB;
      rx_frame.data.u8[4] = 0xCC;
      rx_frame.data.u8[5] = 0xDD;
      rx_frame.data.u8[6] = 0x12;
      rx_frame.data.u8[7] = 0x56;

      ESP32Can.CANWriteFrame(&rx_frame);
  }
}
