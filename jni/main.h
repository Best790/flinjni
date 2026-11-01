#pragma once

#include <cstdlib>
#include <string>
#include <vector>
#include <list>
#include <sstream>
#include <jni.h>
#include <android/log.h>
#include <ucontext.h>
#include <pthread.h>
#include <unistd.h> // system api
#include <sys/mman.h>
#include <cassert> // assert()
#include <dlfcn.h> // dlopen
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "game/RW/common.h"

#include "vendor/obfuscate/str_obfuscate.hpp"

#define SAMP_VERSION	"0.3.7"
#define PORT_VERSION	"0.69"

#define MAX_PLAYERS		1004
#define MAX_VEHICLES	2000
#define MAX_PLAYER_NAME	24

#define RAKSAMP_CLIENT
#define NETCODE_CONNCOOKIELULZ 0x6969

#include "vendor/raknet/SAMP/samp_netencr.h"
#include "vendor/raknet/SAMP/SAMPRPC.h"

#include "util/util.h"

extern uintptr_t g_libGTASA;
extern const char* g_pszStorage;

void Log(const char *fmt, ...);

#ifdef _CDEBUG
#define DLOG(str, ...) \
	Log(str, __VA_ARGS__)
#else
#define DLOG(str, ...)
#endif

#define SA_ADDR(addr) (g_libGTASA + (addr))

uint32_t GetTickCount();