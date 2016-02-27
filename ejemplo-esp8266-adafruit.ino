
#include <ESP8266WiFi.h>
#include <Adafruit_NeoPixel.h>

#include "DHT.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"



/************************* Acceso Wifi *********************************/

WiFiClient client;
#define WLAN_SSID       "nuestro wifi"
#define WLAN_PASS       "nuestro password"

/************************* Adafruit.io  *********************************/

#define AIO_SERVER      "io.adafruit.com" //Dirección IP de los servidores de Adafruit
#define AIO_SERVERPORT  1883                  
#define AIO_USERNAME    "nuestro username"
#define AIO_KEY         "nuestra api key"


/************ Global State (you don't need to change this!) ******************/
const char MQTT_SERVER[] PROGMEM    = AIO_SERVER;
const char MQTT_USERNAME[] PROGMEM  = AIO_USERNAME;
const char MQTT_PASSWORD[] PROGMEM  = AIO_KEY;

Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, AIO_SERVERPORT, MQTT_USERNAME, MQTT_PASSWORD);




/****************************** Feeds ***************************************/
const char SOIL[] PROGMEM = AIO_USERNAME "/feeds/soil";
const char HUMIDITY[] PROGMEM = AIO_USERNAME "/feeds/humidity";
const char TEMPERATURE[] PROGMEM = AIO_USERNAME "/feeds/temperature";
const char BUTTON[] PROGMEM = AIO_USERNAME "/feeds/button";
const char COLOR[] PROGMEM = AIO_USERNAME "/feeds/color";



// Feeds a los que ENVIAREMOS datos
Adafruit_MQTT_Publish soil = Adafruit_MQTT_Publish(&mqtt, SOIL);
Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, HUMIDITY);
Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, TEMPERATURE);

// Feeds de los que RECIBIREMOS datos
Adafruit_MQTT_Subscribe color = Adafruit_MQTT_Subscribe(&mqtt, COLOR);
Adafruit_MQTT_Subscribe button = Adafruit_MQTT_Subscribe(&mqtt, BUTTON);


//Neopixel al que le asignaremos el color
Adafruit_NeoPixel rgb = Adafruit_NeoPixel(1, D7, NEO_GRB + NEO_KHZ800);

//LED normalque encenderemos con el botón

int LED = D1;

//Usaremos un sensor de humedad DHT en el pin 2
DHT dht(D2, DHT11);
  
void MQTT_connect();


void setup() {
  Serial.begin(115200);
  delay(10);
  
   
  Serial.println(F("Adafruit MQTT demo"));

  // Conectamos el ESP al Wifi
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID,WLAN_PASS);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); 
  Serial.println(WiFi.localIP());

  //Nos subscribimos a los FEED
  mqtt.subscribe(&button);
  mqtt.subscribe(&color);

  //Ponemos nuestro LED a punto
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  //Arrancamos el DTH
  dht.begin();
  
  //Iniciamos nuestro RGB
  rgb.begin();
  rgb.setBrightness(255);
  rgb.show();



}



void loop() {
  MQTT_connect();


  //Publimamos los datos
  int valueSoil = analogRead(A0);  //Sensor de humedad del suelo en el analógico 
  soil.publish(valueSoil);
  humidity.publish(dht.readHumidity());
  temperature.publish(dht.readTemperature());

  Adafruit_MQTT_Subscribe *subscription = mqtt.readSubscription(20L*60L*1000L);

  //Nos llega un nuevo color	
  if (subscription == &color) {
    //Convertimos el color en hexadecimal con # en entero
    char * data = (char*) color.lastread;
    int value = (int) strtol(data+1, NULL, 16);
    rgb.setPixelColor(0, value);
    rgb.show();
  }

  //Nos llega el estado del botón
  if (subscription == &button) {
    char * data = (char *) button.lastread;
    digitalWrite(LED, strcmp(data, "ON") == 0);
  }

      
  /*
  * No ponemos ningún delay, será el propio
  * readSubscription quién nos haga de delay también.
  * Por otro lado, configuraremos IFTTT para que, en caso de que
  * la humedad del suelo sea inferior 300 ponga un tuit indicando
  * que hace falta regalarla
  **/

}



void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 6;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
