//Автор данного слива: Александр Блок
//VK: vk.com/alexblock1
//TG: @alexblockone
//Ссылка на студию TG: https://t.me/+DYahAwsC2KxiZGEy
//Обязательно указывать автора: @alexblockone
#include "CServerManager.h"
#include "dialog.h"

#include "main.h"
#include "util/CJavaWrapper.h"

extern CDialogWindow *pDialogWindow;

#ifdef FLIN
	const char* g_szServerNames[MAX_SERVERS] = {
		OBFUSCATE("{ff9729}[1]{ffffff} NAME_SERVER(tg:alexblockone) | Server: 01"),
		OBFUSCATE("{ff9729}[2]{ffffff} NAME_SERVER(tg:alexblockone) | Server: 02"),
		OBFUSCATE("{ff9729}[3]{ffffff} NAME_SERVER(tg:alexblockone) | Server: Test"),
		OBFUSCATE("{ff9729}[3]{ffffff} NAME_SERVER(tg:alexblockone) | Server: Test"),

		OBFUSCATE("{ff9729}[1]{ffffff} Flin RP | Iran Server: 01"),
		OBFUSCATE("{ff9729}[2]{ffffff} Flin RP | Iran Server: 02")
	};

	const CServerInstance::CServerInstanceEncrypted g_sEncryptedAddresses[MAX_SERVERS] = {
		CServerInstance::create(OBFUSCATE("80.242.59.112"), 1, 16, 8973, false), // 0
		CServerInstance::create(OBFUSCATE("80.242.59.112"), 1, 16, 8973, false), // 1
		CServerInstance::create(OBFUSCATE("80.242.59.112"), 1, 16, 8973, false), // 2
		CServerInstance::create(OBFUSCATE("80.242.59.112"), 1, 16, 8973, false), // 3

		CServerInstance::create(OBFUSCATE("80.242.59.112"), 1, 20, 8973, false), // 4
		CServerInstance::create(OBFUSCATE("80.242.59.112"), 1, 20, 8973, false) // 5
	};
#elif JUST
	const char* g_szServerNames[MAX_SERVERS] = {
		OBFUSCATE("{ff9729}[1]{ffffff} JUST RP | Server: Test")
	};

	const CServerInstance::CServerInstanceEncrypted g_sEncryptedAddresses[MAX_SERVERS] = {
			CServerInstance::create(NUM_TO_STR_IP(213, 159, 212, 12), 16, 7777, false)
	};
#else
	const char* g_szServerNames[MAX_SERVERS] = {
		OBFUSCATE("{ff9729}[1]{ffffff} Brilliant RP | Server: Test")
	};

	const CServerInstance::CServerInstanceEncrypted g_sEncryptedAddresses[MAX_SERVERS] = {
			CServerInstance::create(NUM_TO_STR_IP(51, 75, 34, 217), 16, 7777, false)
	};
#endif

