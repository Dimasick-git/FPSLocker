#pragma once

#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

Result fpslockerOmmInitialize(void);
void fpslockerOmmExit(void);
Service* fpslockerOmmGetServiceSession(void);
Result fpslockerOmmGetDefaultDisplayResolution(s32* width, s32* height);

#ifdef __cplusplus
} // extern "C"
#endif
