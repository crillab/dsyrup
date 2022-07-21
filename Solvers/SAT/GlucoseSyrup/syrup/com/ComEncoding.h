#ifndef ENC
#define ENC
#include <stdint.h>
namespace Glucose {
/*
 * Encode value dans le tableau d'octect bytes a la position currentPos
 * Return the new currentPos
 */
inline int VWEencodeUInt(unsigned int value, unsigned char* bytes, int currentPos) {
  while (value > 127) {bytes[currentPos++] = ((uint8_t)(value & 127)) | 128;value >>= 7;}
  bytes[currentPos++] = ((uint8_t)value) & 127;
  return currentPos;
}

inline int VWEencodeUIntTab(unsigned int* values, unsigned char* bytes, int nbValues) {
  int value = 0;
  int currentPos = 0;
  for(int i = 0; i < nbValues;i++){
    value = values[i];
    while (value > 127) {bytes[currentPos++] = ((uint8_t)(value & 127)) | 128;value >>= 7;}
    bytes[currentPos++] = ((uint8_t)value) & 127;
    
  }
  return currentPos;
}

/*
 * Decode bytes in currentPos of bytes in value
 * Return the new currentPos
 */
inline int VWEdecodeUInt(unsigned char* bytes, int currentPos, int* value){
  *value=0;
  for(int i =0;!i || (bytes[currentPos++] & 128);){
    *value |= (bytes[currentPos] & 127) << (7 * i++);
  }
  return currentPos;
}

inline int VWEdecodeUIntTab(unsigned char* bytes, unsigned int* values, int nbBytes){
  int pos=0;
  for(int b = 0; b < nbBytes;pos++){
    values[pos]=0;
    for(int i =0;!i || (bytes[b++] & 128);){
      values[pos] |= (bytes[b] & 127) << (7 * i++);
    }
  }
  return pos;
}

}
#endif
