/***************************************************

  SHENKAR - SMART SYSTEMS
  By: Michael Struk and Noam Roytman
  DATE: May-2021

 ****************************************************/

//Libraries
#include <DHT.h>;
#include <WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"


/**************************Pin Set*******************************************/
// setting PWM properties
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

//Constants
#define DHTPIN 14     // what pin we're connected to
#define BlueLedPin 27 //Blue Led Pin
#define RedLedPin 26 //Red Led Pin
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino


/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "<Wifi name>"
#define WLAN_PASS       "<Wifi Password>"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "<AIO_USERNAME>"
#define AIO_KEY         "<AIO_KEY>"

/************************* Variables for DHT *********************************/
//Variables
int chk;
float hum;  //Stores humidity value
float temp; //Stores temperature value
float TEMP_POINT = 26.0; // Defult Room Tempeture to Change leds




/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

//Variables uses to update and read temp data
// Setup a feed called 'temp_input_data_var' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish temp_input_data_var = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Shenkar_IOT");

// Setup a feed called 'temp_read_data_var' for subscribing to changes.
Adafruit_MQTT_Subscribe temp_read_data_var = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/Shenkar_IOT");


//Variables uses to update and read temp point of change for the led change
// Setup a feed called 'temp_point' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish temp_point_input = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Shenkar_Temp_Decider");

// Setup a feed called 'temp_read_data_var' for subscribing to changes.
Adafruit_MQTT_Subscribe temp_point_read = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/Shenkar_Temp_Decider");



/*************************** Sketch Code ************************************/



void setup()
{
  pinMode(BlueLedPin, OUTPUT); // Declare the Blue LED as an output
  pinMode(RedLedPin, OUTPUT); // Declare the Red LED as an output
  Serial.begin(115200);
  dht.begin();
  delay(1000);
  Serial.println(F("Starting..."));
  delay(1000);
  

  Serial.println(F("\n\n##################################"));
  Serial.println(F("Diffrent Led turn on based on room tempeture"));
  Serial.println(F("Made By Michael Struk And Noam Roytman "));
  Serial.println(F("##################################"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
  Serial.println();


  // SETUP ALL THE SUBSRCIPTIONS HERE
  mqtt.subscribe(&temp_read_data_var);     // Setup MQTT subscription for temp_read_data_var feed.
  mqtt.subscribe(&temp_point_read);     // Setup MQTT subscription for temp_point_read feed.

}

void TurnBlueLED() {
  Serial.println(" Blue Led Should turn on ");
  digitalWrite(BlueLedPin, HIGH); // Turn the Blue LED on
  digitalWrite(RedLedPin, LOW); // Turn the Red LED on
}
void TurnRedLED() {
  Serial.println(" Red Led Should turn on ");
  digitalWrite(BlueLedPin, LOW); // Turn the Blue LED on
  digitalWrite(RedLedPin, HIGH); // Turn the Red LED on
}
void loop()
{
  // Connection to the Cloud
  MQTT_connect();

  //Read data and store it to variables hum and temp
  hum = dht.readHumidity();
  temp = dht.readTemperature();
  //Print temp and humidity values to serial monitor
  Serial.print("\n###  New Reading From sensor ####\n");
  Serial.print("Current Reading from sensor:   ");
  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.print(" %, Temp: ");
  Serial.print(temp);
  Serial.println(" Celsius");
  
  //Sending Data To the cloud of the tempeture
  Serial.print(F("Sending Tempeture "));
  Serial.print(temp);
  Serial.print("...");
  if (! temp_input_data_var.publish(temp)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }
  delay(2000); //Waiting 2 seconds

  //Getting The data from the cloud
  //Getting Data
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &temp_read_data_var) {
//      Serial.println((char *)temp_read_data_var.lastread);
//      String val = String((char *)temp_read_data_var.lastread);
//      int value_from_cloud = val.toInt();  //Converts string to integer
      float value_from_cloud_float;
      value_from_cloud_float = atof((char *)temp_read_data_var.lastread);
      Serial.print(F("Got temp from cloud in float: "));
      Serial.println(value_from_cloud_float);
      //Led Changin based on the reading
      Serial.print(F("Current switching led tempeture set to: "));
      Serial.println(TEMP_POINT);
      if (value_from_cloud_float <= TEMP_POINT) {
        TurnBlueLED();
      }
      else {
        TurnRedLED();
      }
    }
    
    //Cheking reading from phone to change break point of tempeture
    if(subscription == &temp_point_read){
      Serial.print("######## Temp Point Change #####\n");
      Serial.print("Break point Tempeture was changed into: ");
      Serial.println((char *)temp_point_read.lastread);
      String val = String((char *)temp_point_read.lastread);
      int value_from_cloud = val.toInt();  //Converts string to integer
      Serial.print("Setting the tempeture point... ");
      TEMP_POINT = value_from_cloud;
      Serial.print("Break point tempeture was set");
    }
  }
  delay(5000); //Waiting 5 seconds for next reading and testing
}



// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
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
