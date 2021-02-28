#warning BUILDING SOLAR

#include <Arduino.h>
#include <../../common/types.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <DNSServer.h>

extern "C"
{
#include <espnow.h>
}

AsyncWebServer server(80);
DNSServer dnsServer;

const char *ssid = "";
const char *password = "";
#define WIFI_CHANNEL 1
//                         [..........PREFIX..........][...CARID...][...UNIT...][SUB]
uint8_t accesspointMac[] = {PREFIX0, PREFIX1, PREFIX2, CARID_BULLI, UNIT_SOLAR, 0x00};
uint8_t stationMac[] = {PREFIX0, PREFIX1, PREFIX2, CARID_BULLI, UNIT_SOLAR, 0x01};

void initWifi();
void processMessage(uint8_t *mac, uint8_t *data, uint8_t len);
void sendAck(uint8_t *mac);

consumptionTelemetry lastReceived;

void setup()
{
  // For Soft Access Point (AP) Mode
  wifi_set_macaddr(SOFTAP_IF, &accesspointMac[0]);
  // For Station Mode
  wifi_set_macaddr(STATION_IF, &stationMac[0]);

  Serial.begin(9600);
  Serial.println();

  Serial.print("This node AP mac: ");
  Serial.print(WiFi.softAPmacAddress());
  Serial.print(", STA mac: ");
  Serial.println(WiFi.macAddress());

  initWifi();

  if (esp_now_init() != 0)
  {
    Serial.println("*** ESP_Now init failed");
    ESP.restart();
  }

  // Initialize SPIFFS
  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
  }
  else
  {
    Serial.println("Mounted SPIFFS");

    Dir root = SPIFFS.openDir("/");
    while (root.next())
    {
      Serial.print(root.fileName());
      if (root.fileSize())
      {
        File f = root.openFile("r");
        Serial.println(f.size());
      }
    }
  }

  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.println("NotFound! Serving index.html!");
    request->send(SPIFFS, "/index.html", "text/html");
  });
  server.on("/last", HTTP_GET, [](AsyncWebServerRequest *request) {
    char buffer[100];
    sprintf(buffer, "time: %d ms\ntemp: %f\nhum: %f\nvoltage: %f V\ncurrent: %f A",
            lastReceived.t,
            lastReceived.temp,
            lastReceived.hum,
            lastReceived.voltage,
            lastReceived.current);
    request->send(200, "text/plain", buffer);
  });
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.serveStatic("/", SPIFFS, "/");
  server.begin();

  // Note: When ESP8266 is in soft-AP+station mode, this will communicate through station interface
  // if it is in slave role, and communicate through soft-AP interface if it is in controller role,
  // so you need to make sure the remote nodes use the correct MAC address being used by this gateway.
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);

  esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {
    Serial.print("recv_cb, from mac: ");
    char macString[50] = {0};
    sprintf(macString, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.print(macString);
    sendAck(mac);
    processMessage(mac, data, len);
    // First byte determines message type
  });

  esp_now_register_send_cb([](uint8_t *mac, uint8_t status) {
    char macString[50] = {0};
    sprintf(macString, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.print("STATUS = ");
    Serial.println(status);
    Serial.print("Message sent to: ");
    Serial.println(macString);
  });
}

void loop()
{
}

void sendAck(uint8_t *mac)
{
  char macString[50] = {0};
  sprintf(macString, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print("Sending ACK to: ");
  Serial.println(macString);

  uint8_t ackData = 0x01;
  uint8_t sendData[sizeof(ackData)];
  memcpy(sendData, &ackData, sizeof(ackData));
  esp_now_send(mac, sendData, sizeof(sendData));
}

void processData(uint8_t *mac, uint8_t *data, uint8_t len);

void processMessage(uint8_t *mac, uint8_t *data, uint8_t len)
{
  char macString[50] = {0};
  switch (data[0])
  {
  case (uint8_t)0x00: // register
    sprintf(macString, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.print("Registering MAC: ");
    Serial.println(macString);
    break;
  case (uint8_t)0x01: // data transfer
    Serial.println("RECEIVED DATA.");
    processData(mac, data, len);
    break;
  }
}

void processData(uint8_t *mac, uint8_t *data, uint8_t len)
{
  // Switch for type (included in 5th byte of the MAC)
  switch (mac[4])
  {
  case UNIT_COOLBOX:
    Serial.println("FROM COOLBOX.");
    struct messageStruct
    {
      u8 command;
      consumptionTelemetry data;
    };
    messageStruct receivedMessage;
    memcpy(&receivedMessage, data, len);

    Serial.println("PARSED DATA:");
    Serial.print("command = ");
    Serial.println(receivedMessage.command);
    Serial.print("t = ");
    Serial.println(receivedMessage.data.t);
    Serial.print("temp = ");
    Serial.println(receivedMessage.data.temp);
    Serial.print("hum = ");
    Serial.println(receivedMessage.data.hum);
    Serial.print("voltage = ");
    Serial.println(receivedMessage.data.voltage);
    Serial.print("current = ");
    Serial.println(receivedMessage.data.current);

    memcpy(&lastReceived, &receivedMessage.data, sizeof(lastReceived));
    break;

  default:
    Serial.println("No default data parser available.");
  }

  // memcpy(&lastReceived, receivedMessage.data, sizeof(lastReceived));
}

void initWifi()
{

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("BULLINET", "", WIFI_CHANNEL, false);

  Serial.print("Connecting to ");
  Serial.print(ssid);
  if (strcmp(WiFi.SSID().c_str(), ssid) != 0)
  {
    WiFi.begin(ssid, password);
  }

  int retries = 20; // 10 seconds
  while ((WiFi.status() != WL_CONNECTED) && (retries-- > 0))
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  if (retries < 1)
  {
    Serial.print("*** WiFi connection failed");
    WiFi.softAP("BULLINET", "", WIFI_CHANNEL, false);
    // ESP.restart();
  }

  Serial.print("WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());
}
