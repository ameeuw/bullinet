#include <Arduino.h>

// This is a shared file for typedefinitions. Do not move this file.

// MAC ADDRESSES

#define PREFIX0 0xA2
#define PREFIX1 0x76
#define PREFIX2 0x3E
#define CARID_BULLI 0x00
#define UNIT_SOLAR 0xA0
#define UNIT_COOLBOX 0xA1

// keep in sync with slave struct
struct consumptionTelemetry
{
  float temp;
  float hum;
  float voltage;
  float current;
  uint32 t;
};

struct productionTelemetry
{
  float temp;
  float voltage;
  float current;
  uint32 t;
};

struct consumptionCharacteristics
{
  float fuse;
  uint8_t services;
};

enum class consumptionServices
{
  normal = 0x00,
  setpoint = 0x01,
  undercool = 0x02,
  overheat = 0x04
};

struct productionCharacteristics
{
  uint8_t Ppeak;
  uint8_t maxVoltage;
  uint8_t services;
};

enum class productionServices
{
  normal = 0x00
};

struct storageCharacteristics
{
  uint8_t soc;
  uint8_t capacity;
  uint8_t fuse;
  uint8_t services;
};

enum class storageServices
{
  normal = 0x00
};

// struct thermalValues
// {
//   float waterTank;
//   float element1;
//   float element2;
//   float cycleIn;
//   float cycleOut;
//   float airBox;
//   float humidityBox;
//   float airOutside;
//   float humidityOutside;
// };

// struct currentValues
// {
//   float element1;
//   float element2;
//   float battery;
//   float outletPump;
//   float other;
// };

// struct sensorValues
// {
//   thermalValues thermals;
//   currentValues currents;
//   float flowCycle;
//   float flowOutlet;
// };