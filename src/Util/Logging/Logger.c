//
// Created by Kasimir on 15.10.2022.
//

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include "Logger.h"

const enum CMC_LOG_LEVEL ACTIVE_LOG_LEVEL = INFO;

// TODO: Reimplement colors
void cmc_log(enum CMC_LOG_LEVEL level, const char *message, ...) {
	if (level < ACTIVE_LOG_LEVEL) {
		return;
	}
	va_list args;

	va_start(args, message);
	if (level == INFO) {
//        printf("\x1b[33m");
	} else if (level == ERR) {
//        printf("\x1b[31m");
	}

	time_t mytime = time(NULL);
	char *time_str = ctime(&mytime);
	time_str[strlen(time_str) - 1] = '\0';
	printf("%s [%s]: ", time_str, stringFromLevel(level));

	vprintf(message, args);
	va_end(args);
	printf("\n");

//    printf("\x1b[0m");
}
