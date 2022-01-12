#define POZUELO
#define LOCATION "Pozuelo"
#define DEVICE_ID "Jardin"
#define TOKEN "Token-del-Jardin"
#define IS_BME280
#undef ESP32 
#undef IP_FIJA
#ifdef IP_FIJA
  byte ip[] = {192,168,1,30};   
  byte gateway[] = {192,168,1,1};   
  byte subnet[] = {255,255,255,0};  
#endif
#undef CON_LLUVIA
#undef CON_UV
#define CON_SUELO   // con sensor de humedad del suelo
#define PRESSURE_CORRECTION (1.080)  // HPAo/HPHh 647m
#define HUMEDAD_MIN  50  // valores de A0 para suelo seco y empapado
#define HUMEDAD_MAX  450
#define INTERVALO_CONEX 58000 // 5 min en milisecs
