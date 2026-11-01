#include "CJavaWrapper.h"
#include <thread>
#include "../main.h"

extern "C" JavaVM* javaVM;

#include <stddef.h>
#include "..//keyboard.h"
#include "..//chatwindow.h"
#include "..//CSettings.h"
#include "../net/netgame.h"
#include "../voice/CVoiceChatClient.h"
#include "CServerManager.h"
#include "../vendor/curl/include/curl/curl.h";
#include "../util/util.h"
#include "../vendor/json/json/json.h"
#include "../dialog.h"
#include "../checkfilehash.h"
//#include "..//JNIStringUtil.h"
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <KillList.h>

extern CVoiceChatClient* pVoice;
extern CKeyBoard* pKeyBoard;
extern CChatWindow* pChatWindow;
extern CSettings* pSettings;
extern CNetGame* pNetGame;
extern CDialogWindow* pDialogWindow;
extern CGame *pGame;
extern KillList *pKillList;
extern CSnapShotHelper* pSnapShotHelper;

Json::Reader reader;

bool isActiveSelectedServer = false;

JNIEnv* CJavaWrapper::GetEnv()
{
	JNIEnv* env = nullptr;
	int getEnvStat = javaVM->GetEnv((void**)& env, JNI_VERSION_1_4);

	if (getEnvStat == JNI_EDETACHED)
	{
		Log(OBFUSCATE("GetEnv: not attached"));
		if (javaVM->AttachCurrentThread(&env, NULL) != 0)
		{
			Log(OBFUSCATE("Failed to attach"));
			return nullptr;
		}
	}
	if (getEnvStat == JNI_EVERSION)
	{
		Log(OBFUSCATE("GetEnv: version not supported"));
		return nullptr;
	}

	if (getEnvStat == JNI_ERR)
	{
		Log(OBFUSCATE("GetEnv: JNI_ERR"));
		return nullptr;
	}

	return env;
}

	
std::string CJavaWrapper::GetClipboardString()
{
	JNIEnv* env = GetEnv();

	if (!env)
	{
		Log(OBFUSCATE("No env"));
		return std::string("");
	}

	auto retn = (jbyteArray)env->CallObjectMethod(activity, s_GetClipboardText);

	if ((env)->ExceptionCheck())
	{
		(env)->ExceptionDescribe();
		(env)->ExceptionClear();
		return std::string("");
	}

	if (!retn)
	{
		return std::string("");
	}

	jboolean isCopy = true;

	jbyte* pText = env->GetByteArrayElements(retn, &isCopy);
	jsize length = env->GetArrayLength(retn);

	std::string str((char*)pText, length);

	env->ReleaseByteArrayElements(retn, pText, JNI_ABORT);
	
	return str;
}

void CJavaWrapper::CallLauncherActivity(int type)
{
	JNIEnv* env = GetEnv();

	if (!env)
	{
		Log(OBFUSCATE("No env"));
		return;
	}

	env->CallVoidMethod(activity, s_CallLauncherActivity, type);

	EXCEPTION_CHECK(env);
}

void CJavaWrapper::SetVisibleKeyboard(bool active, int type){
    JNIEnv* env = GetEnv();

    if (!env)
    {
        Log(OBFUSCATE("No env"));
        return;
    }

    env->CallVoidMethod(activity, s_SetVisibleKeyboardStandard, active ? 1 : 0, type);

    EXCEPTION_CHECK(env);
}

void CJavaWrapper::Process(){
    uint32_t isActiveCamera_36_4 = *(uint32_t*)(SA_ADDR(0x63E048+36));
    if(isActiveCamera_36_4 == 0){
        if(m_bLastSentActiveDialog != true){
            m_bLastSentActiveDialog = true;

            g_pJavaWrapper->ToggleRender(true);
        }
    }
    if(isActiveCamera_36_4 == 1){
        if(m_bLastSentActiveDialog != false){
            m_bLastSentActiveDialog = false;

            g_pJavaWrapper->ToggleRender(false);
        }
    }

    if(m_bWaitResponseDialog){
        m_bWaitResponseDialog = false;
        if (pNetGame && pGame && pGame->FindPlayerPed() && !pNetGame->GetTextDrawPool()->m_bSelectState){
            pGame->FindPlayerPed()->TogglePlayerControllable(true);
            //pGame->FindPlayerPed()->SetTogglePlayerControllable(true, false);
        }
        pDialogWindow->m_bActiveDialog = false;
        if (pNetGame){
            pNetGame->SendDialogResponse(m_infoDialogWaitReponse.dialogid, m_infoDialogWaitReponse.response, m_infoDialogWaitReponse.listitem, m_infoDialogWaitReponse.inputtext);
        }
    }
}

void CJavaWrapper::ToggleRender(bool active) {

	JNIEnv* env = GetEnv();

	if (!env)
	{
		Log(OBFUSCATE("No env"));
		return;
	}

	env->CallVoidMethod(activity, s_ToggleRender, active ? 1 : 0);

	EXCEPTION_CHECK(env);
}

void CJavaWrapper::SetVisibleDialog(bool active) {

	JNIEnv* env = GetEnv();

	if (!env)
	{
		Log(OBFUSCATE("No env"));
		return;
	}

	env->CallVoidMethod(activity, s_SetVisibleDialog, active ? 1 : 0);

	EXCEPTION_CHECK(env);
}


void CJavaWrapper::ShowInputLayout()
{
	JNIEnv* env = GetEnv();

	if (!env)
	{
		Log(OBFUSCATE("No env"));
		return;
	}


	env->CallVoidMethod(activity, s_ShowInputLayout);

	EXCEPTION_CHECK(env);
}

void CJavaWrapper::HideInputLayout()
{
	JNIEnv* env = GetEnv();

	if (!env)
	{
		Log(OBFUSCATE("No env"));
		return;
	}

	env->CallVoidMethod(activity, s_HideInputLayout);

	EXCEPTION_CHECK(env);
}

void CJavaWrapper::ShowClientSettings()
{
	JNIEnv* env = GetEnv();

	if (!env)
	{
		Log(OBFUSCATE("No env"));
		return;
	}

	env->CallVoidMethod(activity, s_ShowClientSettings);

	EXCEPTION_CHECK(env);
}

void CJavaWrapper::BuildDialog(uint16_t dialogId, char* title, char* content, char* button1, char* button2, uint8_t typeDialog){
	//s_BuildDialog
    JNIEnv* env = GetEnv();

    if (!env)
    {
        Log(OBFUSCATE("No env"));
        return;
    }

    jbyteArray bytes = env->NewByteArray(strlen(title));
    env->SetByteArrayRegion(bytes, 0, strlen(title), (jbyte*)title);

	jbyteArray bytes2 = env->NewByteArray(strlen(content));
    env->SetByteArrayRegion(bytes2, 0, strlen(content), (jbyte*)content);

	jbyteArray bytes3 = env->NewByteArray(strlen(button1));
    env->SetByteArrayRegion(bytes3, 0, strlen(button1), (jbyte*)button1);

	jbyteArray bytes4 = env->NewByteArray(strlen(button2));
    env->SetByteArrayRegion(bytes4, 0, strlen(button2), (jbyte*)button2);

    env->CallVoidMethod(activity, s_BuildDialog, dialogId, bytes, bytes2, bytes3, bytes4,(int) typeDialog);
	env->DeleteLocalRef(bytes);
	env->DeleteLocalRef(bytes2);
	env->DeleteLocalRef(bytes3);
	env->DeleteLocalRef(bytes4);
    
	EXCEPTION_CHECK(env);
}



void CJavaWrapper::ShowToastText(char* text){

	JNIEnv* env = GetEnv();

 
	if (!env)
	{
		Log(OBFUSCATE("No env"));
		return;
	}

    jbyteArray bytes = env->NewByteArray(strlen(text));
    env->SetByteArrayRegion(bytes, 0, strlen(text), (jbyte*)text);

	env->CallVoidMethod(activity, s_ToastMakeText, bytes);
	env->DeleteLocalRef(bytes);
	EXCEPTION_CHECK(env);

}

void CJavaWrapper::SetUseFullScreen(int b)
{
	JNIEnv* env = GetEnv();

	if (!env)
	{
		Log(OBFUSCATE("No env"));
		return;
	}

	env->CallVoidMethod(activity, s_SetUseFullScreen, b);

	EXCEPTION_CHECK(env);
}
extern int g_iStatusDriftChanged;
#include "..//CDebugInfo.h"
extern "C"
{
	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_onInputEnd(JNIEnv* pEnv, jobject thiz, jbyteArray str)
	{
		if (pKeyBoard)
		{
			pKeyBoard->OnNewKeyboardInput(pEnv, thiz, str);
		}
	}
	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_onEventBackPressed(JNIEnv* pEnv, jobject thiz)
	{
		if (pKeyBoard)
		{
			if (pKeyBoard->IsOpen())
			{
				Log(OBFUSCATE("Closing keyboard"));
				pKeyBoard->Close();
			}
		}
	}
	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_onNativeHeightChanged(JNIEnv* pEnv, jobject thiz, jint orientation, jint height)
	{
		if (pChatWindow)
		{
			pChatWindow->SetLowerBound(height);
		}
	}

	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNativeCutoutSettings(JNIEnv* pEnv, jobject thiz, jboolean b)
	{
		if (pSettings)
		{
			pSettings->GetWrite().iCutout = b;
		}
	}

	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNativeKeyboardSettings(JNIEnv* pEnv, jobject thiz, jboolean b)
	{
		if (pSettings)
		{
			pSettings->GetWrite().iAndroidKeyboard = b;
		}

		if (pKeyBoard && b)
		{
			pKeyBoard->EnableNewKeyboard();
		}
		else if(pKeyBoard)
		{
			pKeyBoard->EnableOldKeyboard();
		}
	}

	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNativeFpsCounterSettings(JNIEnv* pEnv, jobject thiz, jboolean b)
	{
		if (pSettings)
		{
			pSettings->GetWrite().iFPSCounter = b;
		}

		CDebugInfo::SetDrawFPS(b);
	}

	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNativeHpArmourText(JNIEnv* pEnv, jobject thiz, jboolean b)
	{
		if (pSettings)
		{
			if (!pSettings->GetWrite().iHPArmourText && b)
			{
				if (CAdjustableHudColors::IsUsingHudColor(HUD_HP_TEXT) == false)
				{
					CAdjustableHudColors::SetHudColorFromRGBA(HUD_HP_TEXT, 255, 0, 0, 255);
				}
				if (CAdjustableHudPosition::GetElementPosition(HUD_HP_TEXT).X == -1 || CAdjustableHudPosition::GetElementPosition(HUD_HP_TEXT).Y == -1)
				{
					CAdjustableHudPosition::SetElementPosition(HUD_HP_TEXT, 500, 500);
				}
				if (CAdjustableHudScale::GetElementScale(HUD_HP_TEXT).X == -1 || CAdjustableHudScale::GetElementScale(HUD_HP_TEXT).Y == -1)
				{
					CAdjustableHudScale::SetElementScale(HUD_HP_TEXT, 400, 400);
				}

				if (CAdjustableHudColors::IsUsingHudColor(HUD_ARMOR_TEXT) == false)
				{
					CAdjustableHudColors::SetHudColorFromRGBA(HUD_ARMOR_TEXT, 255, 0, 0, 255);
				}
				if (CAdjustableHudPosition::GetElementPosition(HUD_ARMOR_TEXT).X == -1 || CAdjustableHudPosition::GetElementPosition(HUD_ARMOR_TEXT).Y == -1)
				{
					CAdjustableHudPosition::SetElementPosition(HUD_ARMOR_TEXT, 300, 500);
				}
				if (CAdjustableHudScale::GetElementScale(HUD_ARMOR_TEXT).X == -1 || CAdjustableHudScale::GetElementScale(HUD_ARMOR_TEXT).Y == -1)
				{
					CAdjustableHudScale::SetElementScale(HUD_ARMOR_TEXT, 400, 400);
				}
			}

			pSettings->GetWrite().iHPArmourText = b;
		}

		CInfoBarText::SetEnabled(b);
	}

	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNativeOutfitGunsSettings(JNIEnv* pEnv, jobject thiz, jboolean b)
	{
		if (pSettings)
		{
			pSettings->GetWrite().iOutfitGuns = b;

			CWeaponsOutFit::SetEnabled(b);
		}
	}

		JNIEXPORT void JNICALL
    	Java_com_nvidia_devtech_NvEventQueueActivity_onKeyboardClose(JNIEnv *pEnv, jobject thiz,
    															jint dialogid, jint response,
    															jint listitem, jbyteArray inputtext) {
pKeyBoard->Close();
    															}


	JNIEXPORT void JNICALL
	Java_com_nvidia_devtech_NvEventQueueActivity_responseDialog(JNIEnv *pEnv, jobject thiz,
															jint dialogid, jint response,
															jint listitem, jbyteArray inputtext) {
		if (pEnv->ExceptionCheck()) {
           return;
        }

            jboolean isCopy = false;
            jbyte* pMsg = pEnv->GetByteArrayElements(inputtext, &isCopy);
			jsize length = pEnv->GetArrayLength(inputtext);
			std::string szStr((char*)pMsg, length);
			//pNetGame->SendDialogResponse(dialogid, response, listitem, (char*) szStr.c_str());
			//std::thread responseDialog_thread(responseDialogThread, dialogid, response, listitem, std::string(szStr.c_str()));
            g_pJavaWrapper->m_infoDialogWaitReponse.dialogid = dialogid;
            g_pJavaWrapper->m_infoDialogWaitReponse.response = response;
            g_pJavaWrapper->m_infoDialogWaitReponse.listitem = listitem;
            //if(g_pJavaWrapper->m_infoDialogWaitReponse.inputtext){
            //    free(g_pJavaWrapper->m_infoDialogWaitReponse.inputtext);
            //    g_pJavaWrapper->m_infoDialogWaitReponse.inputtext = (char*)0;
            //}
            int lenInputText = strlen(szStr.c_str());
            if(lenInputText > 256){
                lenInputText = 256;
            }
            memcpy(&g_pJavaWrapper->m_infoDialogWaitReponse.inputtext, szStr.c_str(), lenInputText);
            g_pJavaWrapper->m_infoDialogWaitReponse.inputtext[lenInputText] = '\0';
            //g_pJavaWrapper->m_infoDialogWaitReponse.inputtext = (char*) malloc(strlen(szStr.c_str()) * sizeof(char) + 1);
            //strcpy(g_pJavaWrapper->m_infoDialogWaitReponse.inputtext, szStr.c_str());
            //g_pJavaWrapper->m_infoDialogWaitReponse.inputtext = (char*)std::string(szStr.c_str()).c_str();
            g_pJavaWrapper->m_bWaitResponseDialog = true;

            //responseDialog_thread.detach();
			pEnv->ReleaseByteArrayElements(inputtext, pMsg, JNI_ABORT);
	}

	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNativePcMoney(JNIEnv* pEnv, jobject thiz, jboolean b)
	{
		if (pSettings)
		{
			pSettings->GetWrite().iPCMoney = b;
		}

		CGame::SetEnabledPCMoney(b);
	}

	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNativeRadarrect(JNIEnv* pEnv, jobject thiz, jboolean b)
	{
		if (pSettings)
		{
			pSettings->GetWrite().iRadarRect = b;

			CRadarRect::SetEnabled(b);
		}
	}

	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNativeNameTag(JNIEnv* pEnv, jobject thiz, jboolean b)
	{
		if (pSettings)
			pSettings->GetWrite().iNameTag = b;
	}

	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setUIP(JNIEnv* pEnv, jobject thiz, jbyteArray data)
	{
		jbyte* dataBytes = pEnv->GetByteArrayElements(data, nullptr);
		jsize dataLen = pEnv->GetArrayLength(data);

		char* dataChars = new char[dataLen + 1];
		memcpy(dataChars, dataBytes, dataLen);
		dataChars[dataLen] = '\0';

		//pChatWindow->AddDebugMessage("UIP %s", dataChars);
		pNetGame->SendRPCUIP(dataChars);
		delete[] dataChars;

		pEnv->ReleaseByteArrayElements(data, dataBytes, 0);
	}

	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setUID(JNIEnv* pEnv, jobject thiz, jbyteArray data)
	{
		jbyte* dataBytes = pEnv->GetByteArrayElements(data, nullptr);
		jsize dataLen = pEnv->GetArrayLength(data);

		char* dataChars = new char[dataLen + 1];
		memcpy(dataChars, dataBytes, dataLen);
		dataChars[dataLen] = '\0';

		//pChatWindow->AddDebugMessage("UID %s", dataChars);
		pNetGame->SendRPCUID(dataChars);
		delete[] dataChars;

		pEnv->ReleaseByteArrayElements(data, dataBytes, 0);
	}


	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNative3DText(JNIEnv* pEnv, jobject thiz, jboolean b)
	{
		if (pSettings)
			pSettings->GetWrite().i3DText = b;
	}

	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNativeVoice(JNIEnv* pEnv, jobject thiz, jboolean b)
	{
		if (pSettings)
			pSettings->GetWrite().iVoice = b;

		if (pVoice)
		{
			if (b)
			{
				if (pVoice->IsDisconnected())
					pVoice->Connect(pNetGame->m_szHostOrIp, pNetGame->m_iPort + 100);
			}
			else pVoice->FullDisconnect();
		}
	} 
	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNativeSkyBox(JNIEnv* pEnv, jobject thiz, jboolean b)
	{
		if (pSettings)
			pSettings->GetWrite().iSkyBox = b;
	}

        JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNativeCacheTextDraw(JNIEnv* pEnv, jobject thiz, jboolean b)
        {
            if (pSettings){
                pSettings->GetWrite().iCacheTextDraw = b;
                pSnapShotHelper->SetActiveCacheTexture(b);
            }

        }
		JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNativeDialogNew(JNIEnv* pEnv, jobject thiz, jboolean b)
    	{
    		if (pSettings)
    			pSettings->GetWrite().iDialogNew = b;
    	}
		JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNativeCrossHair(JNIEnv* pEnv, jobject thiz, jboolean b)
    	{
    		if (pSettings)
    			pSettings->GetWrite().iCrossHair = b;
    	}

	JNIEXPORT jboolean JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNativeSkyBox(JNIEnv* pEnv, jobject thiz)
	{
		if (pSettings)
			return pSettings->GetReadOnly().iSkyBox;

		return 0;
	}

		JNIEXPORT jboolean JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNativeDialogNew(JNIEnv* pEnv, jobject thiz)
    	{
    		if (pSettings)
    			return pSettings->GetReadOnly().iDialogNew;

    		return 0;
    	}
		JNIEXPORT jboolean JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNativeCrossHair(JNIEnv* pEnv, jobject thiz)
    	{
    		if (pSettings)
    			return pSettings->GetReadOnly().iCrossHair;

    		return 0;
    	}

        JNIEXPORT jboolean JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNativeCacheTextDraw(JNIEnv* pEnv, jobject thiz)
        {
            if (pSettings)
                return pSettings->GetReadOnly().iCacheTextDraw;

            return 0;
        }

	JNIEXPORT jboolean JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNativeCutoutSettings(JNIEnv* pEnv, jobject thiz)
	{
		if (pSettings)
		{
			return pSettings->GetReadOnly().iCutout;
		}
		return 0;
	}

	JNIEXPORT jboolean JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNativeKeyboardSettings(JNIEnv* pEnv, jobject thiz)
	{
		if (pSettings)
		{
			return pSettings->GetReadOnly().iAndroidKeyboard;
		}
		return 0;
	}

	JNIEXPORT jboolean JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNativeFpsCounterSettings(JNIEnv* pEnv, jobject thiz)
	{
		if (pSettings)
		{
			return pSettings->GetReadOnly().iFPSCounter;
		}
		return 0;
	}

	JNIEXPORT jboolean JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNativeHpArmourText(JNIEnv* pEnv, jobject thiz)
	{
		if (pSettings)
		{
			return pSettings->GetReadOnly().iHPArmourText;
		}
		return 0;
	}

	JNIEXPORT jboolean JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNativeOutfitGunsSettings(JNIEnv* pEnv, jobject thiz)
	{
		if (pSettings)
		{
			return pSettings->GetReadOnly().iOutfitGuns;
		}
		return 0;
	}

	JNIEXPORT jboolean JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNativePcMoney(JNIEnv* pEnv, jobject thiz)
	{
		if (pSettings)
		{
			return pSettings->GetReadOnly().iPCMoney;
		}
		return 0;
	}

	JNIEXPORT jboolean JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNativeRadarrect(JNIEnv* pEnv, jobject thiz)
	{
		if (pSettings)
		{
			return pSettings->GetReadOnly().iRadarRect;
		}
		return 0;
	}

	JNIEXPORT jboolean JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNative3DText(JNIEnv* pEnv, jobject thiz)
	{
		if (pSettings)
			return pSettings->GetReadOnly().i3DText;

		return 0;
	}

	JNIEXPORT jboolean JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNativeNameTag(JNIEnv* pEnv, jobject thiz)
	{
		if (pSettings)
			return pSettings->GetReadOnly().iNameTag;

		return 0;
	}

	JNIEXPORT jboolean JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNativeVoice(JNIEnv* pEnv, jobject thiz)
	{
		if (pSettings)
			return pSettings->GetReadOnly().iVoice;

		return 0;
	}

	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_onSettingsWindowSave(JNIEnv* pEnv, jobject thiz)
	{
		if (pSettings)
		{
			pSettings->Save();
		}
	}

	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_onSettingsWindowDefaults(JNIEnv* pEnv, jobject thiz, jint category)
	{
		if (pSettings)
		{
			pSettings->ToDefaults(category);
			if (pChatWindow)
			{
				pChatWindow->m_bPendingReInit = true;
			}
		}
	}

    void DefaultSelectServer(jint b){
		pNetGame = new CNetGame(
				g_sEncryptedAddresses[b].decrypt(),
				g_sEncryptedAddresses[b].getPort(),
				pSettings->GetReadOnly().szNickName,
				pSettings->GetReadOnly().szPassword);
	}
    void OneStageConnect(int b){

		struct curl_slist *headers = NULL;
		headers = curl_slist_append(headers, OBFUSCATE("accept: application/dns-json"));
		char urlStart[] = "https://1.1.1.1/dns-query?name=";
		char urlRequest[strlen(urlStart)+strlen(g_sEncryptedAddresses[b].decrypt())];
		sprintf(urlRequest, OBFUSCATE("https://1.1.1.1/dns-query?name=%s"), g_sEncryptedAddresses[b].decrypt());
		std::string reqString = curlRequestGet(urlRequest, headers);

		if(strcmp(reqString.c_str(), "") == 0) {
			Log(OBFUSCATE("Not requested dns-query"));
			DefaultSelectServer(b);
			return;
		}

		Json::Value root;
		bool parsingSuccessful = reader.parse( reqString.c_str(), root );

		if(!parsingSuccessful){
			Log(OBFUSCATE("Request dns-query not parsed: %s"), reqString.c_str());
			DefaultSelectServer(b);
			return;
		}

		Json::Value resAnswer = root["Answer"];
		if(resAnswer.size() != 1){
		    Log(OBFUSCATE("Request dns-query Answers count == %d"), resAnswer.size());
			DefaultSelectServer(b);
            return;
		}

		std::string resAnswerNameServer = resAnswer[0]["name"].asString();

		if(strcmp(resAnswerNameServer.c_str(), g_sEncryptedAddresses[b].decrypt()) != 0){
			Log(OBFUSCATE("Request dns-query Answer name != %s server name: %s"),g_sEncryptedAddresses[b].decrypt(), resAnswerNameServer.c_str());
			DefaultSelectServer(b);
			return;
		}

		std::string resAnswerIP = resAnswer[0]["data"].asString();

		if(strcmp(resAnswerIP.c_str(), OBFUSCATE("")) == 0){
			Log(OBFUSCATE("Request dns-query Answer ip == null: %s"), resAnswerNameServer.c_str());
			DefaultSelectServer(b);
			return;
		}

		pNetGame = new CNetGame(
				resAnswerIP.c_str(),
				g_sEncryptedAddresses[b].getPort(),
				pSettings->GetReadOnly().szNickName,
				pSettings->GetReadOnly().szPassword);
    }
	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNativeSelectServer(JNIEnv* pEnv, jobject thiz, jint b)
	{

        if(isActiveSelectedServer){
            return;
        }
        isActiveSelectedServer = true;

		/*if(!FileCheckSum()) {
			g_pJavaWrapper->ShowToastText(OBFUSCATE("пїЅ пїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ SAMP.\n\nпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅ пїЅпїЅпїЅпїЅпїЅ, пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ."));
		    Log(OBFUSCATE("Folder SAMP is modify"));
		}else{
		    Log(OBFUSCATE("Folder SAMP is original"));
		}*/


		if(!FileCheckSumGTA7z()) {
			pChatWindow->AddDebugMessageNonFormatted(OBFUSCATE("[!] Вы должны обновить версию кэша gta3.img в лаунчере"));
			pChatWindow->AddDebugMessageNonFormatted(OBFUSCATE("[!] Если вы установили моды, то игнорируйте данные сообщения"));
		}else{
			pChatWindow->AddDebugMessageNonFormatted(OBFUSCATE("Вы используйте последнюю версию кэша gta3.img"));

		}


		/*if(!FileCheckSumWeapon()) {
			g_pJavaWrapper->CallLauncherActivity(123);
	   
	   		pChatWindow->AddDebugMessage(OBFUSCATE("пїЅ пїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ, пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅ пїЅпїЅпїЅпїЅ!"));
			pChatWindow->AddDebugMessage(OBFUSCATE("пїЅ пїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ, пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅ пїЅпїЅпїЅпїЅ!"));
			pChatWindow->AddDebugMessage(OBFUSCATE("пїЅ пїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅ, пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅ пїЅпїЅпїЅпїЅ!"));
		    
			Log(OBFUSCATE("Folder SAMP is modify"));
		}
		else 
		{*/
		    Log(OBFUSCATE("Folder SAMP is original"));

			g_pJavaWrapper->CallLauncherActivity(122);

			if(b == 2) pChatWindow->AddDebugMessageNonFormatted(OBFUSCATE("Connect to test server #5555"));
			if(b == 3) pChatWindow->AddDebugMessageNonFormatted(OBFUSCATE("Connect to test server #5557"));
			if(b == 4 || b == 5) pChatWindow->AddDebugMessageNonFormatted(OBFUSCATE("Connect to Iran Proxy"));

			pNetGame = new CNetGame(
			g_sEncryptedAddresses[b].decrypt(),
			g_sEncryptedAddresses[b].getPort(),
			pSettings->GetReadOnly().szNickName,
			pSettings->GetReadOnly().szPassword);
		//}


        //std::thread connect_thread(OneStageConnect, (int)b);
       // connect_thread.detach();

	}

	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNativeHudElementColor(JNIEnv* pEnv, jobject thiz, jint id, jint a, jint r, jint g, jint b)
	{
		CAdjustableHudColors::SetHudColorFromRGBA((E_HUD_ELEMENT)id, r, g, b, a);
	}

	JNIEXPORT jbyteArray JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNativeHudElementColor(JNIEnv* pEnv, jobject thiz, jint id)
	{
		char pTemp[9];
		jbyteArray color = pEnv->NewByteArray(sizeof(pTemp));

		if (!color)
		{
			return nullptr;
		}

		pEnv->SetByteArrayRegion(color, 0, sizeof(pTemp), (const jbyte*)CAdjustableHudColors::GetHudColorString((E_HUD_ELEMENT)id).c_str());

		return color;
	}

	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNativeHudElementPosition(JNIEnv* pEnv, jobject thiz, jint id, jint x, jint y)
	{
	    if(id == -1){
	        if (pSettings){
            	pSettings->GetWrite().fKillListPosX = x;
                pSettings->GetWrite().fKillListPosY = y;
                pKillList->UpdateValuesRender();
                return;
            }
	    }
		if (id == 7)
		{
			if (pSettings)
			{
				pSettings->GetWrite().fChatPosX = x;
				pSettings->GetWrite().fChatPosY = y;
				if (pChatWindow)
				{
					pChatWindow->m_bPendingReInit = true;
				}
				return;
			}
			return;
		}
		if (id == HUD_SNOW)
		{
			if (pSettings)
			{
				pSettings->GetWrite().iSnow = x;
			}
			CSnow::SetCurrentSnow(pSettings->GetReadOnly().iSnow);
			return;
		}
		CAdjustableHudPosition::SetElementPosition((E_HUD_ELEMENT)id, x, y);

		if (id >= HUD_WEAPONSPOS && id <= HUD_WEAPONSROT)
		{
			CWeaponsOutFit::OnUpdateOffsets();
		}
	}

	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNativeHudElementScale(JNIEnv* pEnv, jobject thiz, jint id, jint x, jint y)
	{
	    if(id == -1){
	    if (pSettings){
            pSettings->GetWrite().fKillListScale = x;
            pKillList->UpdateValuesRender();
            }
	        return;
	    }
		CAdjustableHudScale::SetElementScale((E_HUD_ELEMENT)id, x, y);

		if (id >= HUD_WEAPONSPOS && id <= HUD_WEAPONSROT)
		{
			CWeaponsOutFit::OnUpdateOffsets();
		}
	}


	JNIEXPORT jintArray JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNativeHudElementScale(JNIEnv* pEnv, jobject thiz, jint id)
	{
		jintArray color = pEnv->NewIntArray(2);

		if (!color)
		{
			return nullptr;
		}
		int arr[2];
		if(id == -1){
		if (pSettings){
            arr[0] = pSettings->GetReadOnly().fKillListScale;
            }else{
            arr[0] = 0;
            }
            arr[1] = 0;
        }else{
		    arr[0] = CAdjustableHudScale::GetElementScale((E_HUD_ELEMENT)id).X;
		    arr[1] = CAdjustableHudScale::GetElementScale((E_HUD_ELEMENT)id).Y;
		}
		pEnv->SetIntArrayRegion(color, 0, 2, (const jint*)& arr[0]);

		return color;
	}

	JNIEXPORT void JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_setNativeWidgetPositionAndScale(JNIEnv* pEnv, jobject thiz, jint id, jint x, jint y, jint scale)
	{
		if (id == 0)
		{
			if (pSettings)
			{
				pSettings->GetWrite().fButtonMicrophoneX = x;
				pSettings->GetWrite().fButtonMicrophoneY = y;
				pSettings->GetWrite().fButtonMicrophoneSize = scale;
			}

			if (g_pWidgetManager)
			{
				if (g_pWidgetManager->GetSlotState(WIDGET_MICROPHONE))
				{
					g_pWidgetManager->GetWidget(WIDGET_MICROPHONE)->SetPos(x, y);
					g_pWidgetManager->GetWidget(WIDGET_MICROPHONE)->SetHeight(scale);
					g_pWidgetManager->GetWidget(WIDGET_MICROPHONE)->SetWidth(scale);
				}
			}
		}
		
		if (id == 1)
		{
			if (pSettings)
			{
				pSettings->GetWrite().fButtonEnterPassengerX = x;
				pSettings->GetWrite().fButtonEnterPassengerY = y;
				pSettings->GetWrite().fButtonEnterPassengerSize = scale;
			}
		}

		if (id == 2)
		{
			if (pSettings)
			{
				pSettings->GetWrite().fButtonCameraCycleX = x;
				pSettings->GetWrite().fButtonCameraCycleY = y;
				pSettings->GetWrite().fButtonCameraCycleSize = scale;
			}

			if (g_pWidgetManager)
			{
				if (g_pWidgetManager->GetSlotState(WIDGET_CAMERA_CYCLE))
				{
					g_pWidgetManager->GetWidget(WIDGET_CAMERA_CYCLE)->SetPos(x, y);
					g_pWidgetManager->GetWidget(WIDGET_CAMERA_CYCLE)->SetHeight(scale);
					g_pWidgetManager->GetWidget(WIDGET_CAMERA_CYCLE)->SetWidth(scale);
				}
			}
		}
	}


	JNIEXPORT jintArray JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNativeKillListPosition(JNIEnv* pEnv, jobject thiz, jint id)
	{
		jintArray color = pEnv->NewIntArray(3);
		if(!color){
			return nullptr;
		}
		int arr[3];
		if(pSettings){
			if(id == 16){
				arr[0] = pSettings->GetReadOnly().fKillListPosX;
				arr[1] = pSettings->GetReadOnly().fKillListPosY;
				arr[2] = pSettings->GetReadOnly().fKillListScale;

				pEnv->SetIntArrayRegion(color, 0, 2, (const jint*)&arr[0]);

				return color;
			}
		}

	}

	JNIEXPORT jintArray JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNativeHudElementPosition(JNIEnv* pEnv, jobject thiz, jint id)
	{
		jintArray color = pEnv->NewIntArray(2);

		if (!color)
		{
			return nullptr;
		}
		int arr[2];

		if (id == 7 && pSettings)
		{
			arr[0] = pSettings->GetReadOnly().fChatPosX;
			arr[1] = pSettings->GetReadOnly().fChatPosY;
		}
		else if (id == HUD_SNOW && pSettings)
		{
			arr[0] = CSnow::GetCurrentSnow();
			arr[1] = CSnow::GetCurrentSnow();
		}else if(id == -1){
		    arr[0] = pSettings->GetReadOnly().fKillListPosX;
        			arr[1] = pSettings->GetReadOnly().fKillListPosY;
		}else
		{
			arr[0] = CAdjustableHudPosition::GetElementPosition((E_HUD_ELEMENT)id).X;
			arr[1] = CAdjustableHudPosition::GetElementPosition((E_HUD_ELEMENT)id).Y;
		}

		pEnv->SetIntArrayRegion(color, 0, 2, (const jint*)&arr[0]);

		return color;
	}

	JNIEXPORT jintArray JNICALL Java_com_nvidia_devtech_NvEventQueueActivity_getNativeWidgetPositionAndScale(JNIEnv* pEnv, jobject thiz, jint id)
	{
		jintArray color = pEnv->NewIntArray(3);

		if (!color)
		{
			return nullptr;
		}
		int arr[3] = { -1, -1, -1 };
		

		if (pSettings)
		{
			if (id == 0)
			{
				arr[0] = pSettings->GetWrite().fButtonMicrophoneX;
				arr[1] = pSettings->GetWrite().fButtonMicrophoneY;
				arr[2] = pSettings->GetWrite().fButtonMicrophoneSize;
			}
			if (id == 1)
			{
				arr[0] = pSettings->GetWrite().fButtonEnterPassengerX;
				arr[1] = pSettings->GetWrite().fButtonEnterPassengerY;
				arr[2] = pSettings->GetWrite().fButtonEnterPassengerSize;
			}
			if (id == 2)
			{
				arr[0] = pSettings->GetWrite().fButtonCameraCycleX;
				arr[1] = pSettings->GetWrite().fButtonCameraCycleY;
				arr[2] = pSettings->GetWrite().fButtonCameraCycleSize;
			}
		}
		

		pEnv->SetIntArrayRegion(color, 0, 3, (const jint*)& arr[0]);

		return color;
	}
}

CJavaWrapper::CJavaWrapper(JNIEnv* env, jobject activity)
{
	this->activity = env->NewGlobalRef(activity);

	jclass nvEventClass = env->GetObjectClass(activity);
	if (!nvEventClass)
	{
		Log(OBFUSCATE("nvEventClass null"));
		return;
	}

	s_CallLauncherActivity = env->GetMethodID(nvEventClass, OBFUSCATE("callLauncherActivity"), OBFUSCATE("(I)V"));

	s_GetClipboardText = env->GetMethodID(nvEventClass, OBFUSCATE("getClipboardText"), OBFUSCATE("()[B"));

	s_ShowInputLayout = env->GetMethodID(nvEventClass, OBFUSCATE("showInputLayout"), OBFUSCATE("()V"));
	s_HideInputLayout = env->GetMethodID(nvEventClass, OBFUSCATE("hideInputLayout"), OBFUSCATE("()V"));

	s_ShowClientSettings = env->GetMethodID(nvEventClass, OBFUSCATE("showClientSettings"), OBFUSCATE("()V"));
	s_SetUseFullScreen = env->GetMethodID(nvEventClass, OBFUSCATE("setUseFullscreen"), OBFUSCATE("(I)V"));

	s_ToastMakeText = env->GetMethodID(nvEventClass, OBFUSCATE("ToastMakeText"), OBFUSCATE("([B)V"));

	s_BuildDialog = env->GetMethodID(nvEventClass, OBFUSCATE("BuildDialog"), OBFUSCATE("(I[B[B[B[BI)V"));
    s_ToggleRender = env->GetMethodID(nvEventClass, OBFUSCATE("ToggleRender"), OBFUSCATE("(I)V"));
	s_SetVisibleDialog = env->GetMethodID(nvEventClass, OBFUSCATE("SetVisibleDialog"), OBFUSCATE("(I)V"));

    s_SetVisibleKeyboardStandard = env->GetMethodID(nvEventClass, OBFUSCATE("SetVisibleKeyboardStandard"), OBFUSCATE("(II)V"));


    env->DeleteLocalRef(nvEventClass);
}

CJavaWrapper::~CJavaWrapper()
{
	JNIEnv* pEnv = GetEnv();
	if (pEnv)
	{
		pEnv->DeleteGlobalRef(this->activity);
	}
}

CJavaWrapper* g_pJavaWrapper = nullptr;