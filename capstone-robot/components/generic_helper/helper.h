#ifndef C_HELPER_DEFINE
#define C_HELPER_DEFINE

#if __cplusplus
extern "C" {
#endif

#include <esp_timer.h>

#define NULL_ARG_CHECK(x) do { if(!x) return ESP_ERR_INVALID_ARG; } while(0)
#define ERR_CHECK_CRITICAL(x, mux) do { esp_err_t __e = x; if(__e != ESP_OK) { taskEXIT_CRITICAL(mux); return __e; } } while(0);
#define TIMEOUT_CHECK_US(start, len) ((esp_timer_get_time() - (start)) >= (len))
#define WAIT_FOR_US(start, len) while((esp_timer_get_time() - (start)) < (len))
#define ERR_CHECK(x) do { esp_err_t __e = x; if(__e != ESP_OK) return __e; } while (0)

#if __cplusplus
}
#endif

#endif // #ifndef C_HELPER_DEFINE