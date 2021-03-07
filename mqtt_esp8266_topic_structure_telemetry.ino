#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

const char* THING_NAME = "thing1";

// Create a "thing name"/client ID
static String strThingName(THING_NAME);

const char* ssid = "<WIFI NAME>";
const char* password = "<WIFI PASSWORD>"
const char* mqtt_server = "<MQTT BROKER ADDRESS>";
const int CONNECTED_FAN = 4; // GOIP D2

#define DHTTYPE DHT11   // DHT 11
// DHT Sensor
const int DHTPin = 5; // GPIO D1
// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

// Temporary variables
static char celsiusTemp[7];
static char fahrenheitTemp[7];
static char humidityTemp[7];

#define  FLT_MIN   1.17549435E-38F

static float h_old = FLT_MIN;
static float t_old = FLT_MIN;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  String strTopic(topic);
  if (strTopic.endsWith("/builtinled")) {
    if ((char)payload[0] == '1') {
      digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    } else {
      digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
    }
  }
  if (strTopic.endsWith("/fan")) {
    if ((char)payload[0] == '1') {
      digitalWrite(CONNECTED_FAN, LOW);   // Turn the FAN off
    } else {
      digitalWrite(CONNECTED_FAN, HIGH);  // Turn the FAN on
    }
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    String fullpathThingName = "cmd/hvac/building3/floor2/confroom2/" + strThingName + "/+";

    // Attempt to connect
    if (client.connect(THING_NAME)) {
      Serial.println("connected");
      // resubscribe
      client.subscribe(fullpathThingName.c_str());
      client.subscribe("cmd/hvac/building3/floor2/confroom2/+");
      client.subscribe("cmd/hvac/building3/floor2/+");
      client.subscribe("cmd/hvac/building3/+");
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
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(CONNECTED_FAN, OUTPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 5000) {

    // SENSOR READ START

    // Sensor readings may also be up to 5 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
      strcpy(celsiusTemp, "Failed");
      strcpy(fahrenheitTemp, "Failed");
      strcpy(humidityTemp, "Failed");
    }
    else {
      // Computes temperature values in Celsius + Fahrenheit and Humidity
      float hic = dht.computeHeatIndex(t, h, false);
      dtostrf(hic, 6, 2, celsiusTemp);
      float hif = dht.computeHeatIndex(f, h);
      dtostrf(hif, 6, 2, fahrenheitTemp);
      dtostrf(h, 6, 2, humidityTemp);
      // You can delete the following Serial.print's, it's just for debugging purposes
      Serial.print("Humidity: ");
      Serial.print(h);
      Serial.print(" %\t Temperature: ");
      Serial.print(t);
      Serial.print(" *C ");
      Serial.print(f);
      Serial.print(" *F\t Heat index: ");
      Serial.print(hic);
      Serial.print(" *C ");
      Serial.print(hif);
      Serial.println(" *F");

      if (h != h_old) {
        String fullpathThingNameHumidity = "dt/hvac/building3/floor2/confroom2/" +
                                           strThingName + "/humidity";
        client.publish(fullpathThingNameHumidity.c_str(), humidityTemp);
        h_old = h;
      }
      if (t != t_old) {
        String fullpathThingNameTemperature = "dt/hvac/building3/floor2/confroom2/"
                                              + strThingName + "/temperature";
        client.publish(fullpathThingNameTemperature.c_str(), celsiusTemp);
        t_old=t;
      }
    }

    // SENSOR READ END

    lastMsg = now;
  
  }
}
