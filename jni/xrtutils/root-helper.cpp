#include "../main.h"

void SAMP_DUMP(const char* tag, const char* fmt, ...) {
	char buffer[8196+1] = { '\0' };
	memset(buffer, 0, sizeof(buffer));

	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg);
	va_end(arg);

	__android_log_write(ANDROID_LOG_INFO, tag, buffer);
	memset(buffer, 0, sizeof(buffer));
}