#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* THING_NAME = "thing1";

const char* ssid = "<WIFI NAME>";
const char* password = "<WIFI PASSWORD>"
const char* mqtt_server = "<MQTT BROKER ADDRESS>";
const int CONNECTED_FAN = 4; // GOIP D2

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
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

    // Create a "thing name"/client ID
    String strThingName(THING_NAME);
    String fullpathThingName = "cmd/hvac/building3/floor2/confroom2/"+strThingName+"/+";

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
  pinMode(CONNECTED_FAN, OUTPUT);   // Initialize CONNECTED_FAN  - GOIP D2 - as an output
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

}
