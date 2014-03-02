#ifndef __CC3000_SERVER_H__
#define __CC3000_SERVER_H__

#include "ch.h"
#include "clarity_api.h"

uint32_t cc3000HttpServerKill(void);

uint32_t cc3000HttpServerStart(controlInformation * control, httpInformation * http,
                               Mutex * cc3000ApiMutex);

#endif /* __CC3000_SERVER_H__ */

