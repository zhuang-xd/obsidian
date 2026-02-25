#ifndef EQ_GLASSES_ENTRY_MAIN_H_
#define EQ_GLASSES_ENTRY_MAIN_H_

#include "base/app_common_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

int start_application(AppConfigPtr config);
int stop_application(void);
int reboot_hardware(AppRestartReson reason);

#ifdef __cplusplus
}
#endif

#endif // EQ_GLASSES_ENTRY_MAIN_H_
