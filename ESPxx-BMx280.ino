/**
    ESP-12E  (ESP8266) manejado desde IBM IoT
    Morrastronics -- by chuRRuscat
    v1.0 2017 initial version
    v2.0 2018 mqtt & connectivity  functions separated
    v2.1 2019 anyadido define CON_ LLUVIA y cambios en handleupdate
    v2.5 2020 reprogrammed handling of reconnections
    v2.6 2020 added define CON_UV to handle UV sensors
          added #define BME280 to diferentiate BMP280 and BME280 in <device.h>
    v3.0 2021 added #define ESP32 to  Integrate ESP12 and ESP32 versions       
*/
#undef  ESP32
//#define ESP32  // to use an ESP32 or (if undefined) an ESP12

#define PRINT_SI
#ifdef  PRINT_SI
  #define DPRINT(...)    Serial.print(__VA_ARGS__)
  #define DPRINTLN(...)  Serial.println(__VA_ARGS__)
  #define DPRINTF(...)   Serial.printf(__VA_ARGS__)
#else
  #define DPRINT(...)     //blank line
  #define DPRINTLN(...)   //blank line
  #define DPRINTF(...)
#endif
/*************************************************
 ** -------- Personalised values ----------- **
 * *****************************************/
/* select sensor and its values */ 
#include <ArduinoJson.h>
#include "mqtt_mosquitto.h"  /* mqtt values */
//include "jardn.h"   // I moved these (device) includes to "personal.h"
#ifdef ESP32
  #include <ESPmDNS.h>
  #include <WiFiUdp.h>
  #include <ArduinoOTA.h>
  #define SCL 22
  #define SDA 21
  #define ADC1_CH0 36  // 
  #define ADC1_CH3 39  // 
  #define PIN_UV   36   
  #define interruptPin 04 // PIN where I'll connect the rain gauge (GPIO04)
  #define hSueloPin    34  // analog PIN  of Soil humidity sensor
  #define CONTROL_HUMEDAD 02  // Transistor base that switches on&off soil sensor
#else    // it is an ESP12 (NodeMCU)
  #include <ESP8266mDNS.h>
  #include <WiFiUdp.h>
  #include <ArduinoOTA.h>
  #define SDA D5   // for BME280 I2C 
  #define SCL D6
  #define interruptPin D7 // PIN where I'll connect the rain gauge
  #define PIN_UV D8
  #define hSueloPin    A0  // analog PIN  of Soil humidity sensor
  #define CONTROL_HUMEDAD D2  // Transistor base that switches on&off soil sensor
#endif
#ifdef IS_BME280
   #define ACTUAL_BME280_ADDRESS BME280_ADDRESS_ALTERNATE   // (0x76)depends on sensor manufacturer
   //#define ACTUAL_BME280_ADDRESS BME280_ADDRESS           // (0x77)
#else
   #define ACTUAL_BME280_ADDRESS 0x76
#endif   
/*************************************************
 ** ----- End of Personalised values ------- **
 * ***********************************************/
#define AJUSTA_T 10000 // To adjust delay in some places along the program
#
#define L_POR_BALANCEO 0.2794 // liter/m2 for evey rain gauge interrupt
#include <Wire.h>             //libraries for sensors and so on
#include <Adafruit_Sensor.h>
#ifdef IS_BME280
   #include <Adafruit_BME280.h>
   Adafruit_BME280 sensorBMX280;     // this represents the sensor BME
#else
   #include <Adafruit_BMP280.h>
   Adafruit_BMP280 sensorBMX280;     // this represents the sensor BMP
#endif
#include "Pin_NodeMCU.h"

#define _BUFFSIZE 250
#define DATOS_SIZE 250

volatile int contadorPluvi = 0; // must be 'volatile',for counting interrupt 
volatile long lastTrigger=0;
/* ********* these are the sensor variables that will be exposed **********/ 
int numError=0;
float lluvia=0;
int humedadMin=HUMEDAD_MIN,
        humedadMax=HUMEDAD_MAX, 
        humedadSuelo=0,humedadCrudo=HUMEDAD_MIN;
int humedadCrudo1,humedadCrudo2,
        intervaloConex=INTERVALO_CONEX;
#define JSONBUFFSIZE 350
#define DATOSJSONSIZE 350   
char datosJson[DATOS_SIZE];
StaticJsonDocument<DATOSJSONSIZE> docJson;
JsonObject valores=docJson.createNestedObject();  //Read values
JsonObject claves=docJson.createNestedObject();   // key values (location and deviceId)
#ifdef CON_LLUVIA
    // Interrupt counter for rain gauge
    void ICACHE_RAM_ATTR balanceoPluviometro() {
     if ((millis()-lastTrigger) > 1000){
         lastTrigger=millis();
           contadorPluvi++;
     }  
  }
#endif
#ifdef CON_UV
  int lecturaUV;
  float UV_Watt;
  int   UV_Index;  
#endif  

// let's start, setup variables
void setup() {
boolean status;
    
  Serial.begin(115200);
  DPRINTLN("starting ... "); 
  Wire.begin(SDA,SCL);
  status = sensorBMX280.begin(ACTUAL_BME280_ADDRESS);  
  if (!status) {
     DPRINTLN("Can't connect to BME Sensor!  ");    
  }
  /* start PINs first soil Humidity, then Pluviometer */
  #ifdef CON_LLUVIA
        pinMode(interruptPin, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(interruptPin), balanceoPluviometro, CHANGE);
  #endif
  #ifdef CON_SUELO
    pinMode(CONTROL_HUMEDAD,OUTPUT);
    digitalWrite(CONTROL_HUMEDAD, HIGH); // prepare to read soil humidity sensor
    espera(1000);
    humedadCrudo1 = analogRead(hSueloPin); //first read to have date to get averages
    espera(1000);
    humedadCrudo2 = analogRead(hSueloPin);  //second read
    digitalWrite(CONTROL_HUMEDAD, LOW);
  #endif
  wifiConnect();
  mqttConnect();
  #ifdef IS_BME280
      sensorBMX280.setSampling(Adafruit_BME280::MODE_NORMAL);
  #else
     sensorBMX280.setSampling(Adafruit_BMP280::MODE_NORMAL);
  #endif
  #ifdef CON_UV
    pinMode(PIN_UV, INPUT);
    //analogReadResolution(12);
    //analogSetPinAttenuation(PIN_UV,ADC_11db) ;    
  #endif
  DPRINTLN(" los dos connect hechos, ahora OTA");
  ArduinoOTA.setHostname(DEVICE_ID); 
  ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
          type = "sketch";
      } else { // U_FS
          type = "filesystem";
      }
      // NOTE: if updating FS this would be the place to unmount FS using FS.end()
      DPRINTLN("Start updating " + type);
   });
   ArduinoOTA.onEnd([]() {
     DPRINTLN("\nEnd");
   });
   ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      DPRINTF("Progress: %u%%\r\n",(progress / (total / 100)));
   });
   ArduinoOTA.onError([](ota_error_t error) {
            DPRINTF("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) {
            DPRINTLN("Auth Failed");
        } else if (error == OTA_BEGIN_ERROR) { 
            DPRINTLN("Begin Failed");
        } else if (error == OTA_CONNECT_ERROR) {
            DPRINTLN("Connect Failed");
        } else if (error == OTA_RECEIVE_ERROR) {
            DPRINTLN("Receive Failed");
        } else if (error == OTA_END_ERROR) {
            DPRINTLN("End Failed");
        }
    });
  ArduinoOTA.begin(); 
  #ifdef CON_UV
      valores["indexUV"]=0;
  #endif
  #ifdef CON_SUELO
      valores["hSuelo"]=0;
      valores["hCrudo"]=0;
      if (humedadMin==humedadMax) humedadMax+=1;
  #endif
  #ifdef CON_LLUVIA
      valores["l/m2"]=0;
  #endif  
  valores["temp"]=0;      
  valores["HPa"]=0;
  #ifdef IS_BME280
      valores["hAire"]=0;
  #endif  
  claves["deviceId"]=DEVICE_ID;
  claves["location"]=LOCATION;
  delay(50);
  publicaDatos();      // and publish data. This is the function that gets and sends
}

uint32_t ultima=0;

void loop() {
  int i=0;
    DPRINT("#");
    if (!loopMQTT()) {  // Check if there are MQTT messages and if the device is connected  
        DPRINTLN("Connection lost; retrying");
        sinConectividad();        
        mqttConnect();
    }
    ArduinoOTA.handle(); 
    if ((millis()-ultima)>intervaloConex) {   // if it is time to send data, do it
      DPRINT("interval:");DPRINT(intervaloConex);
      DPRINT("\tmillis :");DPRINT(millis());
      DPRINT("\tultima :");DPRINTLN(ultima);
/*      while (!publicaDatos() && i<10) {     // repeat until publicadatos sends data
        DPRINTLN("PublicaDatos returned false");
        espera(1000);        // publish data. This is the function that gets and sends
        i++;
        }
        i=0;
        */
      publicaDatos();
      ultima=millis();
    }
    espera(1000); //and wait
}

/****************************************** 
* this function sends data to MQTT broker, 
* first, it calls to tomaDatos() to read data 
*********************************************/
boolean publicaDatos() {
    int k=0;
    char signo;
    boolean pubresult=true;
         
   tomaDatos();
   serializeJson(docJson,datosJson);
    // and publish them.
    DPRINTLN("preparing to send");
    pubresult = enviaDatos(publishTopic,datosJson); 
    if (pubresult){
        lluvia=0.0;      // data sent successfully, set rain to zero 
    }
    return pubresult;
}

/* get data function. Read the sensors and set values in global variables */
/* get data function. Read the sensors and set values in global variables */
boolean tomaDatos (){
    boolean escorrecto=true;  //return value will be false unless it works
    float temperatura,humedadAire,presionHPa;
    int i=0;
    
    /* read and then get the mean */
    DPRINTLN("begin tomadatos");
    #ifdef CON_SUELO
        /* activate soil sensor setting the transistor base */
        digitalWrite(CONTROL_HUMEDAD, HIGH);
        espera(1000);  
        humedadCrudo = analogRead(hSueloPin); // and read soil moisture
        humedadCrudo=constrain(humedadCrudo,humedadMin,humedadMax); 
        digitalWrite(CONTROL_HUMEDAD, LOW);  // disconnect soil sensor
        // calculate the moving average of soil humidity of last three values 
        humedadCrudo=(humedadCrudo1+humedadCrudo2+humedadCrudo)/3;
        humedadSuelo = map(humedadCrudo,humedadMin,humedadMax,0,100);
        humedadCrudo2=humedadCrudo1;
        humedadCrudo1=humedadCrudo;
        valores["hCrudo"]=humedadCrudo;
        valores["hSuelo"]=humedadSuelo;    
    #endif
    // read from BME280 sensor
    #ifdef IS_BME280
       humedadAire= sensorBMX280.readHumidity();
       if (isnan(humedadAire)){
            humedadAire=200;
       }
    #endif   
    temperatura= sensorBMX280.readTemperature();
    if (isnan(temperatura)){
            temperatura=200;
    }    
    presionHPa= sensorBMX280.readPressure();
    if (!isnan(presionHPa)){
            presionHPa=presionHPa/100.0F*PRESSURE_CORRECTION;
    } else presionHPa=0;
    #ifdef CON_LLUVIA 
        lluvia+=contadorPluvi*L_POR_BALANCEO;  
    if (lluvia==0) 
        valores["l/m2"]=serialized(String(0.0));
    else
       valores["l/m2"]=lluvia;     
    contadorPluvi=0;   
    #endif
    #ifdef CON_UV
      lecturaUV = analogRead(PIN_UV);
      UV_Watt=10/1.2*(3.3*lecturaUV/1023-1);  // mWatt/cm^2, at 2.2 V read there are 10mw/cm2
      UV_Index=(int) UV_Watt;
      DPRINT(" analog ");     DPRINTLN(lecturaUV);
      DPRINT("/t UV Watt  ");DPRINTLN(UV_Watt);
      DPRINT("/t UV index  ");DPRINTLN(UV_Index);
      valores["indexUV"]=UV_Watt;
    #endif
    #ifdef IS_BME280
	    valores["hAire"]=(int)humedadAire;	    
	    if (humedadAire>100 || humedadAire==0){
	      valores["hAire"]=0;
	      valores.remove("hAire");   // y values are out of range, donn't send them	    
	    }
    #endif    
    if (temperatura > 90 || temperatura <-50) {
        valores["temp"]=0;
        valores["HPa"]=0;      
        valores.remove("temp");
        valores.remove("HPa"); 
        escorrecto=false;           
    } else {
     valores["HPa"]=(int)presionHPa;
      if (((int)temperatura-temperatura)==0){
          valores["temp"]=temperatura+0.001;
      } else {    
        valores["temp"]=temperatura;
      }  
    }
    /****************************************
    if (!escorrecto) {
      numError++;
      if (numError >5) {
         numError=0;
        // ESP.restart();
      }  
    }
    *******************************************/
    return escorrecto;
}
