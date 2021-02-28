#warning BUILDING COOLBOX

#include <Arduino.h>
#include <../../common/types.h>
#include <ESP8266WiFi.h>
extern "C"
{
#include <espnow.h>
}

// this is the MAC Address of the remote ESP which this ESP sends its data too
//  62:01:94:0B:69:47
//                         [..........PREFIX..........][...CARID...][...UNIT...][SUB]
uint8_t accesspointMac[] = {PREFIX0, PREFIX1, PREFIX2, CARID_BULLI, UNIT_COOLBOX, 0x00};
uint8_t stationMac[] = {PREFIX0, PREFIX1, PREFIX2, CARID_BULLI, UNIT_COOLBOX, 0x01};

uint8_t remoteMac[] = {PREFIX0, PREFIX1, PREFIX2, CARID_BULLI, UNIT_SOLAR, 0x00};

// uint8_t thisMac[] = {0xA2, 0x76, 0x3F, 0x00, 0x00, 0x00};
// uint8_t remoteMac[] = {0xA2, 0x76, 0x3E, 0x00, 0x00, 0x00};

// uint8_t remoteMac[] = {0x62, 0x01, 0x94, 0x0B, 0x69, 0x47};

#define WIFI_CHANNEL 1
#define SLEEP_TIME 15e6
#define SEND_TIMEOUT 2000
#define SEND_PERIOD 15000

uint lastSend = 0;
uint lastSent = 0;

volatile uint8_t sendStatus;
void sendReading();

void setup()
{
  delay(250);
  // For Soft Access Point (AP) Mode
  wifi_set_macaddr(SOFTAP_IF, &accesspointMac[0]);
  // For Station Mode
  wifi_set_macaddr(STATION_IF, &stationMac[0]);
  delay(250);

  Serial.begin(9600);
  Serial.println();

  Serial.print("This node AP mac: ");
  Serial.print(WiFi.softAPmacAddress());
  Serial.print(", STA mac: ");
  Serial.println(WiFi.macAddress());

  WiFi.mode(WIFI_STA); // Station mode for sensor/controller node
  WiFi.begin();

  if (esp_now_init() != 0)
  {
    Serial.println("*** ESP_Now init failed");
    ESP.restart();
  }

  delay(1); // This delay seems to make it work more reliably???

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(remoteMac, ESP_NOW_ROLE_SLAVE, WIFI_CHANNEL, NULL, 0);

  esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len) {
    Serial.print("Received message from: ");
    char macString[50] = {0};
    sprintf(macString, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.println(macString);

    if (data[0] == 0x01)
    {
      Serial.print("SUCCESSFUL ACK!  ");
      Serial.println(data[0]);
      sendStatus = 0;
      lastSent = millis();
    }
    else
    {
      Serial.print("UNSUCCESFUL ACK!  ");
      Serial.println(data[0]);
      sendStatus = 1;
      lastSend = millis();
    }
  });

  esp_now_register_send_cb([](uint8_t *mac, uint8_t status) {
    sendStatus = status;
    char macString[50] = {0};
    sprintf(macString, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    if (status == 0)
    {
      Serial.print("STATUS = ");
      Serial.println(status);
      Serial.print("Message sent to: ");
      Serial.println(macString);
      Serial.println("NOW WAITING FOR ACK...");
      sendStatus = 255;
    }
    else
    {
      Serial.println("NOT SENT. NO ACK EXPECTED! RESENDING!");
    }
  });
}

void loop()
{
  if ((millis() > (lastSent + SEND_PERIOD) && (sendStatus == 0)) ||
      ((millis() > (lastSend + SEND_TIMEOUT)) && (sendStatus != 0)))
  {
    Serial.print("Sending value at: ");
    Serial.println(millis());
    sendReading();
  }
  else
  {
    Serial.print(".");
    delay(500);
  }
  // if ((millis() > (lastSend + SEND_TIMEOUT)) && (sendStatus != 255)) {
  //   Serial.print("Sending value at: "); Serial.println(millis());
  //   sendStatus = 255;
  //   sendReading();
  // } else {
  //   Serial.print(".");
  //   delay(500);
  // }
}

void sendReading()
{
  struct messageStruct
  {
    u8 command;
    consumptionTelemetry data;
  };
  messageStruct message;
  message.command = 0x01;
  message.data.t = (uint32)millis();
  message.data.temp = 1.337;
  message.data.hum = 276.3;
  message.data.voltage = 13.5;
  message.data.current = 6.7;
  Serial.print("Sending telemetry at t=");
  Serial.print(message.data.t);
  Serial.println(" ms");
  u8 bs[sizeof(message)];
  memcpy(bs, &message, sizeof(message));
  esp_now_send(NULL, bs, sizeof(bs)); // NULL means send to all peers
  lastSend = millis();

  // memcpy(bs, &telemetryObject, sizeof(telemetryObject));
  // esp_now_send(NULL, bs, sizeof(bs)); // NULL means send to all peers
}
