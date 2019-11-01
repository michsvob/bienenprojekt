
#define UINT16_t_MAX  65536
#define INT16_t_MAX   UINT16_t_MAX/2
#define UINT8_t_MAX   256
#define INT8_t_MAX    UINT8_t_MAX/2

int16_t convertFloatToInt16(float value, long max, long min) {
  float conversionFactor = (float) (INT16_t_MAX) / (float)(max - min);
  return (int16_t)(value * conversionFactor);
}

uint16_t convertFloatToUInt16(float value, long max, long min = 0) {
  float conversionFactor = (float) (UINT16_t_MAX) / (float)(max - min);
  return (uint16_t)(value * conversionFactor);
}

int8_t convertFloatToInt8(float value, long max, long min) {
  float conversionFactor = (float) (INT8_t_MAX) / (float)(max - min);
  return (int8_t)(value * conversionFactor);
}

uint8_t convertFloatToUInt8(float value, long max, long min = 0) {
  float conversionFactor = (float) (UINT8_t_MAX) / (float)(max - min);
  return (uint8_t)(value * conversionFactor);
}

int8_t convertTemperature(float value){
  //-128 - +128
  return (uint8_t)(value);
}

uint8_t convertHumidity(float value){
  //max uint = 256
  return (uint8_t)(value);
}

uint16_t convertPressure(float value){
  return (uint16_t)(value);
}

int16_t convertWeight(float value){
  return (int16_t)(100*value);
}
