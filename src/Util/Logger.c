//
// Created by Kasimir on 15.10.2022.
//

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include "Logger.h"

#ifndef ACTIVE_LOG_LEVEL
#define ACTIVE_LOG_LEVEL INFO
#endif

// TODO: Reimplement colors
void sc_log(enum CMC_LOG_LEVEL level, const char *message, ...) {
    CMC_LOG_LEVEL active_level = ACTIVE_LOG_LEVEL;
	if (level < active_level) {
		return;
	}
	va_list args;

	va_start(args, message);
	if (level == INFO) {
        printf("\033[0;34m");
	} else if (level == ERR) {
       printf("\033[1;31m");
	} else if (level == DEBUG) {
        printf("\033[0;36m");
    }  else if (level == WARN) {
        printf("\033[0;33m");
    }

	time_t mytime = time(NULL);
	char *time_str = ctime(&mytime);
	time_str[strlen(time_str) - 1] = '\0';
	printf("%s [%s]: ", time_str, stringFromLevel(level));

	vprintf(message, args);
	va_end(args);
	printf("\n");

    printf("\x1b[0m");
}
