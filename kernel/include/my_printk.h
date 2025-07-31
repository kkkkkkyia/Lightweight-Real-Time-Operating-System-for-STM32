#ifndef MY_PRINTK_H
#define MY_PRINTK_H

#include "my_config.h"
#include <stdio.h>

#ifdef MY_USE_PRINTK

	#if (MY_PRINTK_LEVEL<=MY_PRINTK_ERROR_LEVEL)
		#define MY_PRINTK_ERROR(format, ...) ( printf("[OS_ERROR]"),printf(format,  ##__VA_ARGS__))
	#else
		#define MY_PRINTK_ERROR(format, args...)
	#endif

	#if (MY_PRINTK_LEVEL<=MY_PRINTK_WARNING_LEVEL)
		#define MY_PRINTK_WARNING(format, ...) ( printf("[OS_WARNING]"),printf(format,  ##__VA_ARGS__))
	#else
		#define MY_PRINTK_WARNING(format, args...)
	#endif

	#if (MY_PRINTK_LEVEL<=MY_PRINTK_INFO_LEVEL)
		#define MY_PRINTK_INFO(format, ...)( printf("[OS_INFO]"),printf(format,  ##__VA_ARGS__))
	#else
		#define MY_PRINTK_INFO(format, args...)
	#endif

	#if (MY_PRINTK_LEVEL<=MY_PRINTK_DEBUG_LEVEL)
		#define MY_PRINTK_DEBUG(format, ...) ( printf("[OS_DEBUG]"),printf(format,  ##__VA_ARGS__))
	#else
		#define MY_PRINTK_DEBUG(format, args...)
	#endif
#else
	#define MY_PRINTK_ERROR(format, args...)
	#define MY_PRINTK_WARNING(format, args...)
	#define MY_PRINTK_INFO(format, args...)
	#define MY_PRINTK_DEBUG(format, args...)
#endif
#define DEBUG_TRACE_SHOW 1
#if DEBUG_TRACE_SHOW 
	#define DEBUG_TRACE(format,...) ( printf("[OS_DEBUG]"),printf(format,  ##__VA_ARGS__))
#else
	#define DEBUG_TRACE(format,...)
#endif
#endif
