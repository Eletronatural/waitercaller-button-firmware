/*
 * Waiter Caller Firmeware
 * Validated for devices: ESP8266 ESP-01 
 * Author: Gustavo Rubin (gusrubin@gmail.com)
*/

#include <FS.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

// Update these with values suitable for your network.
const char* ssid = "Mofos House";
const char* password = "stptff26";
const char* mqtt_server = "192.168.0.104";
const char* mqtt_user = "agent";
const char* mqtt_password = "agentagent";

// Device Serial Number
const char *device_serial_number_file = "/device-serial-number.cfg";
char deviceSerialNumber[30] = "";

// constants won't change. They're used here to set pin numbers:
const int buttonPin = 2; // the number of the pushbutton pin
const int ledPin =  0; // the number of the LED pin

// variables will change:
int buttonState = 0; // variable for reading the pushbutton status
bool buttonWasPressed = false;

// Loads serial number from a file
void loadDeviceSerialNumber() {
  File fileLoaded = SPIFFS.open(device_serial_number_file, "r");
  if (!fileLoaded) {
    Serial.println("Failed to open device-serial-number.cfg file.");    
    return;
  }
  char buffer[30];
  while (fileLoaded.available()) {
    int i = fileLoaded.readBytesUntil('\n', buffer, sizeof(buffer));
    buffer[i] = 0;
  }
  strcpy(deviceSerialNumber, buffer);
  Serial.print("Waiter Caller Device serial number: ");
  Serial.println(deviceSerialNumber);
  fileLoaded.close();
  return;
}

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

  randomSeed(micros());

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

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("waitercaller/device-start-notice", deviceSerialNumber);
      // ... and resubscribe
      // client.subscribe("inTopic");
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
  Serial.begin(115200);
  setup_wifi();
  Serial.println("Booting...");
 
  pinMode(BUILTIN_LED, OUTPUT); // Initialize the BUILTIN_LED pin as an output
  // initialize the LED pin as an output:
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT);
  buttonWasPressed = false;

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  
  loadDeviceSerialNumber();
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // read the state of the pushbutton value:
  buttonState = digitalRead(buttonPin);

  // check if the pushbutton is pressed. If it is, the buttonState is LOW:
  if (buttonState == LOW && buttonWasPressed == false) {
    buttonWasPressed = true;
    // turn LED on:
    digitalWrite(ledPin, LOW);
    client.publish("waitercaller/desk/1", "1");
  }
  if (buttonState == HIGH && buttonWasPressed == true) {
    buttonWasPressed = false;
    // turn LED off:
    digitalWrite(ledPin, HIGH);
    client.publish("waitercaller/desk/1", "0");
  }

}
