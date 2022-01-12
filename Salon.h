#undef CON_SUELO
#define POZUELO
#define LOCATION "Pozuelo"
#define CON_SUELO 
#define PRESSURE_CORRECTION (1.080)  // HPAo/HPHh 647m
#define IS_BME280
#undef ESP32 

#define DEVICE_ID "Salon"
#define TOKEN "Token-del-Salon"

#undef IP_FIJA
#ifdef IP_FIJA
  byte ip[] = {192,168,1,31};   
  byte gateway[] = {192,168,1,1};   
  byte subnet[] = {255,255,255,0};  
#endif
#undef  CON_LLUVIA  // con pluviómetro
#undef CON_UV

#define HUMEDAD_MIN  100  // valores de A0 para suelo seco y empapado
#define HUMEDAD_MAX  600
#define INTERVALO_CONEX 58000 // 5 min en milisecs
