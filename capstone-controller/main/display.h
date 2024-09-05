#ifndef DISPLAY_DEFINE
#define DISPLAY_DEFINE

#include <stdint.h>

void display_task(void *pvParams);
void set_dpp_key(const char*, unsigned int len);
void set_data(float data_to[5]) ;

#endif // #ifndef DISPLAY_DEFINE