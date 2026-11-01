#include "../main.h"
#include "../crashlytics.h"
#include "../util/CStackTrace.h"
#include "CrashHandler.h"
#include <sys/system_properties.h>

#define PROP_VALUE_MAX  92

#ifdef GAME_EDITION_CR
	extern uint16_t g_usLastProcessedModelIndexAutomobile;
	extern int g_iLastProcessedModelIndexAutoEnt;
#endif

extern int g_iLastProcessedSkinCollision;
extern int g_iLastProcessedEntityCollision;
extern int g_iLastRenderedObject;

extern char g_bufRenderQueueCommand[200];
extern uintptr_t g_dwRenderQueueOffset;

struct sigaction act_old;
struct sigaction act1_old;
struct sigaction act2_old;
struct sigaction act3_old;
struct sigaction act4_old;
struct sigaction act5_old;
struct sigaction act6_old;
struct sigaction act7_old;

#define SIGEMT 7

void PrintSymbols(void* pc, void* lr)
{
	Dl_info info_pc, info_lr;

	if (dladdr(pc, &info_pc) != 0) {
		CrashHandler::Log(OBFUSCATE("PC: %s"), info_pc.dli_sname);
	}

	if (dladdr(lr, &info_lr) != 0) {
		CrashHandler::Log(OBFUSCATE("LR: %s"), info_lr.dli_sname);
	}
}

void SIGBUS_handler(int signum, siginfo_t* info, void* contextPtr)
{
	auto* context = (ucontext_t*)contextPtr;

	if (act3_old.sa_sigaction)
		act3_old.sa_sigaction(signum, info, contextPtr);

	if (info->si_signo == SIGBUS)
	{
		CrashHandler::Report(context, info, "SIGBUS");
		exit(0);
	}
}

void SIGSEGV_handler(int signum, siginfo_t *info, void* contextPtr)
{
	auto* context = (ucontext_t*)contextPtr;

	if (act_old.sa_sigaction)
		act_old.sa_sigaction(signum, info, contextPtr);

	if(info->si_signo == SIGSEGV)
	{
		CrashHandler::Report(context, info, "SIGSEGV");

		exit(0);
	}
}

void SIGFPE_handler(int signum, siginfo_t* info, void* contextPtr)
{
	auto* context = (ucontext_t*)contextPtr;

	if (act2_old.sa_sigaction)
		act2_old.sa_sigaction(signum, info, contextPtr);

	if (info->si_signo == SIGFPE)
	{
		CrashHandler::Report(context, info, "SIGFPE");

		exit(0);
	}
}

void SIGABRT_handler(int signum, siginfo_t* info, void* contextPtr)
{
	auto* context = (ucontext_t*)contextPtr;

	if (act1_old.sa_sigaction)
		act1_old.sa_sigaction(signum, info, contextPtr);

	if (info->si_signo == SIGABRT)
	{
		CrashHandler::Report(context, info, "SIGABRT");

		exit(0);
	}
}

void SIGILL_handler(int signum, siginfo_t* info, void* contextPtr)
{
	auto* context = (ucontext_t*)contextPtr;

	if (act4_old.sa_sigaction)
		act4_old.sa_sigaction(signum, info, contextPtr);

	if (info->si_signo == SIGILL)
	{
		CrashHandler::Report(context, info, "SIGILL");
		exit(0);
	}
}

void SIGEMT_handler(int signum, siginfo_t* info, void* contextPtr)
{
	auto* context = (ucontext_t*)contextPtr;

	if (act5_old.sa_sigaction)
		act5_old.sa_sigaction(signum, info, contextPtr);

	if (info->si_signo == SIGEMT)
	{
		CrashHandler::Report(context, info, "SIGEMT");
		exit(0);
	}
}

void SIGURG_handler(int signum, siginfo_t* info, void* contextPtr)
{
	auto* context = (ucontext_t*)contextPtr;

	if (act6_old.sa_sigaction)
		act6_old.sa_sigaction(signum, info, contextPtr);

	if (info->si_signo == SIGURG)
	{
		CrashHandler::Report(context, info, "SIGURG");
		exit(0);
	}
}

void SIGPIPE_handler(int signum, siginfo_t* info, void* contextPtr)
{
	auto* context = (ucontext_t*)contextPtr;

	if (act7_old.sa_sigaction)
		act7_old.sa_sigaction(signum, info, contextPtr);

	if (info->si_signo == SIGPIPE)
	{
		CrashHandler::Report(context, info, "SIGPIPE");
		exit(0);
	}
}

#define CRASHER_NAME_A "crasher"
#define CRASHER_NAME_B "crasher"

void CrashHandler::Report(void* contextPtr, siginfo_t* info, const char* sign)
{
	int iTmpVal = 0;
	auto* context = (ucontext_t*)contextPtr;
	int crashId = (int)rand() % 20000;
	
	char build_fingerprint[PROP_VALUE_MAX+1] = { '\0' };
   	__system_property_get("ro.build.fingerprint", build_fingerprint);
   	char product_cpu_abi[PROP_VALUE_MAX+1] = { '\0' };
   	__system_property_get("ro.product.cpu.abi", product_cpu_abi);

	CrashHandler::Log("*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***");
	CrashHandler::Log(OBFUSCATE("Build fingerprint: %s"), build_fingerprint);
	CrashHandler::Log(OBFUSCATE("Build time: %s %s"), __TIME__, __DATE__); 
	CrashHandler::Log(OBFUSCATE("Revision: 0"));
	CrashHandler::Log(OBFUSCATE("ABI: %s"), product_cpu_abi);
	CrashHandler::Log(OBFUSCATE("pid: 0x%X, tid: 0x%X, name: %s  >>> %s <<<"), getpid(), gettid(), CRASHER_NAME_A, CRASHER_NAME_B);
	CrashHandler::Log(OBFUSCATE("signal %d (%s), code %d (%s), fault addr 0x%X"), info->si_signo, sign, info->si_code, info->si_code == 1 ? "SEGV_MAPERR" : "SEGV_ACCERR", info->si_addr);
	CrashHandler::Log(OBFUSCATE("r0: 0x%X, r1: 0x%X, r2: 0x%X, r3: 0x%X"), (context)->uc_mcontext.arm_r0, (context)->uc_mcontext.arm_r1, (context)->uc_mcontext.arm_r2, (context)->uc_mcontext.arm_r3); 
	CrashHandler::Log(OBFUSCATE("r4: 0x%x, r5: 0x%x, r6: 0x%x, r7: 0x%x"), (context)->uc_mcontext.arm_r4, (context)->uc_mcontext.arm_r5, (context)->uc_mcontext.arm_r6, (context)->uc_mcontext.arm_r7); 
	CrashHandler::Log(OBFUSCATE("r8: 0x%x, r9: 0x%x, sl: 0x%x, fp: 0x%x"), (context)->uc_mcontext.arm_r8, (context)->uc_mcontext.arm_r9, (context)->uc_mcontext.arm_r10, (context)->uc_mcontext.arm_fp); 
	CrashHandler::Log(OBFUSCATE("ip: 0x%x, sp: 0x%x, lr: 0x%x, pc: 0x%x, cpsr: 0x%X"), (context)->uc_mcontext.arm_ip, (context)->uc_mcontext.arm_sp, (context)->uc_mcontext.arm_lr, (context)->uc_mcontext.arm_pc, (context)->uc_mcontext.arm_cpsr);

	CrashHandler::Log(OBFUSCATE("Last rendered object: %d"), g_iLastRenderedObject); 
	CrashHandler::Log(OBFUSCATE("Last processed auto and entity: %d %d"), g_usLastProcessedModelIndexAutomobile, g_iLastProcessedModelIndexAutoEnt);
	CrashHandler::Log(OBFUSCATE("Last processed skin and entity: %d %d"), g_iLastProcessedSkinCollision, g_iLastProcessedEntityCollision);

	CrashHandler::Log(OBFUSCATE("base address:")); 
	CrashHandler::Log(OBFUSCATE("#00 libGTASA.so 0x%X"), g_libGTASA);
	CrashHandler::Log(OBFUSCATE("#01 libSCAnd.so 0x%X"), FindLibrary(OBFUSCATE("libSCAnd.so")));
	CrashHandler::Log(OBFUSCATE("#02 libImmEmulatorJ.so 0x%X"), FindLibrary(OBFUSCATE("libImmEmulatorJ.so")));
	CrashHandler::Log(OBFUSCATE("#03 libsamp.so 0x%X"), FindLibrary(OBFUSCATE("libsamp.so")));
	CrashHandler::Log(OBFUSCATE("#04 libc.so 0x%X"), FindLibrary(OBFUSCATE("libc.so")));
	CrashHandler::Log(OBFUSCATE("#05 libEGL.so 0x%X"), FindLibrary(OBFUSCATE("libEGL.so")));
	CrashHandler::Log(OBFUSCATE("#06 libGLESv2.so 0x%X"), FindLibrary(OBFUSCATE("libGLESv2.so")));
	CrashHandler::Log(OBFUSCATE("#07 libOpenSLES.so 0x%X"), FindLibrary(OBFUSCATE("libOpenSLES.so")));

	CrashHandler::Log(OBFUSCATE("backtrace:")); 
	CrashHandler::Log(OBFUSCATE("#00 pc libGTASA.so + 0x%X"), (context)->uc_mcontext.arm_pc - g_libGTASA); 
	CrashHandler::Log(OBFUSCATE("#01 lr libGTASA.so + 0x%X"), (context)->uc_mcontext.arm_lr - g_libGTASA); 					
	CrashHandler::Log(OBFUSCATE("#02 pc libSCAnd.so + 0x%X"), (context)->uc_mcontext.arm_pc - FindLibrary(OBFUSCATE("libSCAnd.so")));
	CrashHandler::Log(OBFUSCATE("#03 lr libSCAnd.so + 0x%X"), (context)->uc_mcontext.arm_lr - FindLibrary(OBFUSCATE("libSCAnd.so"))); 			
	CrashHandler::Log(OBFUSCATE("#04 pc libImmEmulatorJ.so + 0x%X"), (context)->uc_mcontext.arm_pc - FindLibrary(OBFUSCATE("libImmEmulatorJ.so")));
	CrashHandler::Log(OBFUSCATE("#05 lr libImmEmulatorJ.so + 0x%X"), (context)->uc_mcontext.arm_lr - FindLibrary(OBFUSCATE("libImmEmulatorJ.so"))); 			
	CrashHandler::Log(OBFUSCATE("#06 pc libsamp.so + 0x%X"), (context)->uc_mcontext.arm_pc - FindLibrary(OBFUSCATE("libsamp.so")));
	CrashHandler::Log(OBFUSCATE("#07 lr libsamp.so + 0x%X"), (context)->uc_mcontext.arm_lr - FindLibrary(OBFUSCATE("libsamp.so"))); 		
	CrashHandler::Log(OBFUSCATE("#08 pc libc.so + 0x%X"), (context)->uc_mcontext.arm_pc - FindLibrary(OBFUSCATE("libc.so"))); 
	CrashHandler::Log(OBFUSCATE("#09 lr libc.so + 0x%X"), (context)->uc_mcontext.arm_lr - FindLibrary(OBFUSCATE("libc.so"))); 
	CrashHandler::Log(OBFUSCATE("#10 pc libEGL.so + 0x%X"), (context)->uc_mcontext.arm_pc - FindLibrary(OBFUSCATE("libEGL.so"))); 
	CrashHandler::Log(OBFUSCATE("#11 lr libEGL.so + 0x%X"), (context)->uc_mcontext.arm_lr - FindLibrary(OBFUSCATE("libEGL.so"))); 
	CrashHandler::Log(OBFUSCATE("#12 pc libGLESv2.so + 0x%X"), (context)->uc_mcontext.arm_pc - FindLibrary(OBFUSCATE("libGLESv2.so"))); 
	CrashHandler::Log(OBFUSCATE("#13 lr libGLESv2.so + 0x%X"), (context)->uc_mcontext.arm_lr - FindLibrary(OBFUSCATE("libGLESv2.so"))); 
	CrashHandler::Log(OBFUSCATE("#14 pc libOpenSLES.so + 0x%X"), (context)->uc_mcontext.arm_pc - FindLibrary(OBFUSCATE("libOpenSLES.so"))); 
	CrashHandler::Log(OBFUSCATE("#15 lr libOpenSLES.so + 0x%X"), (context)->uc_mcontext.arm_lr - FindLibrary(OBFUSCATE("libOpenSLES.so"))); 
																							
	PrintSymbols((void*)((context)->uc_mcontext.arm_pc), (void*)((context)->uc_mcontext.arm_lr)); 															
	CStackTrace::printBacktrace(context);
}

void CrashHandler::Initialise()
{
	struct sigaction act;
	act.sa_sigaction = SIGSEGV_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &act, &act_old);

	struct sigaction act1;
	act1.sa_sigaction = SIGABRT_handler;
	sigemptyset(&act1.sa_mask);
	act1.sa_flags = SA_SIGINFO;
	sigaction(SIGABRT, &act1, &act1_old);

	struct sigaction act2;
	act2.sa_sigaction = SIGFPE_handler;
	sigemptyset(&act2.sa_mask);
	act2.sa_flags = SA_SIGINFO;
	sigaction(SIGFPE, &act2, &act2_old);

	struct sigaction act3;
	act3.sa_sigaction = SIGBUS_handler;
	sigemptyset(&act3.sa_mask);
	act3.sa_flags = SA_SIGINFO;
	sigaction(SIGBUS, &act3, &act3_old);

	struct sigaction act4;
	act4.sa_sigaction = SIGILL_handler;
	sigemptyset(&act4.sa_mask);
	act4.sa_flags = SA_SIGINFO;
	sigaction(SIGILL, &act4, &act4_old);

	struct sigaction act5;
	act5.sa_sigaction = SIGEMT_handler;
	sigemptyset(&act5.sa_mask);
	act5.sa_flags = SA_SIGINFO;
	sigaction(SIGEMT, &act5, &act5_old);

	struct sigaction act6;
	act6.sa_sigaction = SIGURG_handler;
	sigemptyset(&act6.sa_mask);
	act6.sa_flags = SA_SIGINFO;
	sigaction(SIGURG, &act6, &act6_old);

	struct sigaction act7;
	act7.sa_sigaction = SIGPIPE_handler;
	sigemptyset(&act7.sa_mask);
	act7.sa_flags = SA_SIGINFO;
	sigaction(SIGPIPE, &act7, &act7_old);
}

void CrashHandler::Log(const char* fmt, ...)
{
	char buffer[2048+1] = { '\0' };
	static FILE* flLog = nullptr;

	if (flLog == nullptr && g_pszStorage != nullptr)
	{
		sprintf(buffer, OBFUSCATE("%sSAMP/crashdump.log"), g_pszStorage);
		flLog = fopen(buffer, OBFUSCATE("a"));
	}

	memset(buffer, 0, sizeof(buffer));

	va_list arg;
	va_start(arg, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, arg);
	va_end(arg);

	__android_log_write(ANDROID_LOG_INFO, OBFUSCATE("crashdump"), buffer);

	firebase::crashlytics::Log(buffer);

	if (flLog == nullptr) return;
	fprintf(flLog, OBFUSCATE("%s\n"), buffer);
	fflush(flLog);

	// close file
	fclose(flLog);
	flLog = nullptr;
}