#ifndef MY_CRITICAL_H
#define MY_CRITICAL_H
	
#include "port.h"

#define MY_ENTER_CRITICAL() vPortEnterCritical()

#define MY_EXIT_CRITICAL() vPortExitCritical()

#endif
