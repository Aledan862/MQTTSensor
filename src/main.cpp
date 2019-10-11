#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <TM74HC595Display.h>
#include <GyverTimer.h>

GTimer_ms potenTimer(300);
long poten;

int SCLK = 14;  //di5
int RCLK = 4;  //di2
int DIO = 0;   //di3

TM74HC595Display disp(SCLK, RCLK, DIO);
GTimer_us dispTimer(1500);
uint8 screen = 1;
//unsigned char LED_0F[29];

DHT dht;
int DHTPIN = 12; //di6
float temp, hum = 0;
// Update these with values suitable for your network.

const char* ssid = "Technolink15_2G";
const char* password = "TL12345678";
const char* mqtt_server = "192.168.99.128";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());



}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("/ESP8266", "Connected");
      // ... and resubscribe
      //client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  //pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(57600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  //client.setCallback(callback);

  dht.setup(DHTPIN);
  Serial.println(dht.getStatus());

}

void loop() {
  char dht_msg_t[100];
  char dht_msg_h[100];
  char poten_msg[100];
  if (potenTimer.isReady()) {
    poten = constrain(analogRead(PIN_A0),0,1023);
    poten = map(poten,0,1023,0,9999);
    snprintf (poten_msg, 100, "[{\"id\" : \"MQTTsensor.Device1.Rolling\", \"v\" = %ld}]", poten);
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    //++value;
    //snprintf (msg, 50, "value = %d", enc_value);
    //Serial.print("Publish message: ");
    //Serial.println(msg);
    //client.publish("/encoder", msg);
    if (dht.getStatus() == 0){
      hum = dht.getHumidity();
      temp = dht.getTemperature();
      snprintf (dht_msg_h, 100, "[{\"id\" : \"MQTTsensor.Device1.Humudity\", \"v\" = %f}]", hum);
      snprintf (dht_msg_t, 100, "[{\"id\" : \"MQTTsensor.Device1.Temperature\", \"v\" = %f}]", temp);
    }else
    {
      snprintf (dht_msg_t, 50, dht.getStatusString());
    }
    Serial.print("Publish message: ");
    Serial.println(dht_msg_t);
    client.publish("MQTTsensor", dht_msg_t);
    Serial.println(dht_msg_h);
    client.publish("MQTTsensor", dht_msg_h);
    Serial.print("Publish message: ");
    Serial.println(poten_msg);

    client.publish("MQTTsensor", poten_msg);
    screen=(screen+1)%3;
  }

  switch (screen)
  {
  case 0:
    disp.send(0x89, 0b1000);
    disp.digit2((int)hum, 0xb0001);
    break;
  case 1:
    disp.send(0x87, 0b1000);
    disp.digit2((int)temp, 0xb0001);
    break;

  default:
    disp.digit4(poten);
    break;
  }

  if (dispTimer.isReady()) disp.timerIsr(); //обновить дисплей
}
