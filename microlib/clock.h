#ifndef MICROLIB_CLOCK_SHIM_H
#define MICROLIB_CLOCK_SHIM_H

#include "microlib.h"

#ifdef __cplusplus
extern "C" {
#endif

u64 mc_get_run_time(void);

#ifdef __cplusplus
}
#endif

#endif /* MICROLIB_CLOCK_SHIM_H */
