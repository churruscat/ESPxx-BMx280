#define POZUELO
#define LOCATION "Pozuelo"
#define DEVICE_ID "Terraza"
#define TOKEN "Token-dela-Terraza"
#define IS_BME280
#undef IP_FIJA
#ifdef IP_FIJA
  byte ip[] = {192,168,1,32};   
  byte gateway[] = {192,168,1,1};   
  byte subnet[] = {255,255,255,0};  
#endif
#undef CON_SUELO   // con sensor de humedad del suelo
#undef CON_UV
#define CON_LLUVIA  // con pluviómetro
#define PRESSURE_CORRECTION (1.080)  // HPAo/HPHh 647m
#define HUMEDAD_MIN  150  /* valores de A0 para suelo seco y empapado*/
#define HUMEDAD_MAX  850
#define INTERVALO_CONEX 58000 // 1 min en milisecs
