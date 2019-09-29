#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <GyverEncoder.h>
#include <TM74HC595Display.h>
#include <DHT.h>
#define CLK 0
#define DT 4
#define SW 5
Encoder enc1(CLK, DT, SW);
int enc_value = 0;


int SCLK = 14;  //di5
int RCLK = 12;  //di6
int DIO = 13;   //di7

TM74HC595Display disp(SCLK, RCLK, DIO);
uint8 screen = 1;
//unsigned char LED_0F[29];

DHT dht;
float temp, hum = 0;
// Update these with values suitable for your network.

const char* ssid = "WHITE HOUSE";
const char* password = "donaldtrumP";
const char* mqtt_server = "185.228.232.60";

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
      client.publish("/outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
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

  enc1.setTickMode(AUTO);
  enc1.setType(2);

  pinMode(17, INPUT);
  dht.setup(17);
  Serial.println(dht.getStatus());
}

void loop() {
  char dht_msg[50];
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    //snprintf (msg, 50, "value = %d", enc_value);
    //Serial.print("Publish message: ");
    //Serial.println(msg);
    //client.publish("/encoder", msg);
    
    temp = dht.getTemperature();
    hum = dht.getHumidity();
    snprintf (dht_msg, 50, "temp = %f; hum = %f", temp, hum);
    Serial.print("Publish message: ");
    Serial.println(dht_msg);
    client.publish("/DHT22", dht_msg);
    screen = (screen=1)?2:1;
  }

  if (enc1.isRight()) enc_value++;     	// если был поворот направо, увеличиваем на 1
  if (enc1.isLeft()) enc_value--;	    // если был поворот налево, уменьшаем на 1
  enc_value = constrain(enc_value, 0 , 9999);

  if (enc1.isTurn()) {       // если был совершён поворот (индикатор поворота в любую сторону)
    Serial.println(enc_value);  // выводим значение при повороте
  }


  if (enc1.isClick()) {
    screen=(screen+1)%3;
  }

  switch (screen)
  {
  case 1:
    disp.send(0x89, 0b1000);
    disp.digit2((int)hum, 0xb0001);
    break;
  case 2:
    disp.send(0x87, 0b1000);
    disp.digit2((int)temp, 0xb0001);
    break; 
  
  default:
    disp.digit4(enc_value);
    break;
  }


}