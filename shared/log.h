#ifndef _LOG_H_
#define _LOG_H_

#include "common.h"

#define FILE_NAME "log.txt"

void log_init();
void log_init_name(char* filename);
void log_debug(char* message);
void log_error(char* message);
void log_info(char* message);
void log_save();

#endif // _LOG_H_
