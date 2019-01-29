#include <stdint.h>

__attribute__((__weak__, visibility("default")))
uint32_t __loader_android_get_application_target_sdk_version();


__attribute__((__weak__))
uint32_t android_get_application_target_sdk_version() {
  return __loader_android_get_application_target_sdk_version();
}
