/*#define LOCATION "LugarDePrueba"
#define DEVICE_ID "Pruebas"
#define TOKEN "Token-de-Pruebas"#define LOCATION "LugarDePrueba"
*/
#define POZUELO
#define LOCATION "Prueba"
#define DEVICE_ID "Prueba"
#define TOKEN "Token-de-Barco"
#define IS_BME280
#undef CON_SUELO   // con sensor de humedad del suelo
#undef CON_UV
#define CON_LLUVIA
#define PRESSURE_CORRECTION (1.0)  // HPAo/HPHh 0m
#define HUMEDAD_MIN  50  // valores de A0 para suelo seco y empapado
#define HUMEDAD_MAX  450
#define INTERVALO_CONEX 58000 // 1 min
