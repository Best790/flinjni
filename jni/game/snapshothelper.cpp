#include "../main.h"
#include "game.h"
#include "RW/RenderWare.h"
#include "CSettings.h"

extern CGame *pGame;
extern CSettings* pSettings;

CSnapShotHelper::CSnapShotHelper()
{
	m_camera = 0;
	m_light = 0;
	m_frame = 0;
	m_zBuffer = 0;
	m_raster = 0;

    for (int i = 0; i < MAX_CACHE_TEXTURE; ++i) {
        m_cacheTextures[i].iModel = 0;
        m_cacheTextures[i].pTexture = 0;
    }

    //SetActiveCacheTexture(pSettings->GetReadOnly().iCacheTextDraw);

	//fix

	SetActiveCacheTexture(false);


	SetUpScene();
}

void CSnapShotHelper::ClearCacheTexture(){
    for (int i = 0; i < MAX_CACHE_TEXTURE; ++i) {
        if(m_cacheTextures[i].pTexture > 0){
            Log("oooRemoveee %d", m_cacheTextures[i].pTexture);
            RwTextureDestroy((RwTexture*)m_cacheTextures[i].pTexture);
        }
        m_cacheTextures[i].iModel = 0;
        m_cacheTextures[i].pTexture = 0;
    }
}

void CSnapShotHelper::SetActiveCacheTexture(bool bActive){
    ClearCacheTexture();

    m_bActiveCacheTextures = bActive;
}

void CSnapShotHelper::CacheTexture(uintptr_t iModel,uintptr_t pTexture){
    if(!m_bActiveCacheTextures || !m_cacheTextures){
        return;
    }
    if(m_cacheTextures[MAX_CACHE_TEXTURE-1].pTexture != 0){
        RwTextureDestroy((RwTexture*)m_cacheTextures[MAX_CACHE_TEXTURE-1].pTexture);
        for (int i = MAX_CACHE_TEXTURE - 1 - 1; i >= MAX_CACHE_TEXTURE; i--) {
            m_cacheTextures[i+1].iModel = m_cacheTextures[i].iModel;
            m_cacheTextures[i+1].pTexture = m_cacheTextures[i].pTexture;
        }
        m_cacheTextures[0].iModel = iModel;
        m_cacheTextures[0].pTexture = pTexture;
    }else{
        for (int i = 0; i < MAX_CACHE_TEXTURE; i++) {
            if(m_cacheTextures[i].pTexture == 0){
                m_cacheTextures[i].iModel = iModel;
                m_cacheTextures[i].pTexture = pTexture;
                return;
            }
            if(m_cacheTextures[i].iModel == iModel) return;
        }
    }
}

uintptr_t CSnapShotHelper::GetCacheTexture(uintptr_t iModel){
    if(!m_bActiveCacheTextures || !m_cacheTextures){
        return 0;
    }
    for (int i = 0; i < MAX_CACHE_TEXTURE; ++i) {
        if(m_cacheTextures[i].iModel == iModel){
            return m_cacheTextures[i].pTexture;
        }
    }

    return 0;
}

void CSnapShotHelper::SetUpScene()
{
	// RpLightCreate
	m_light = ((uintptr_t(*)(int))(g_libGTASA + 0x1E3860 + 1))(2);
	if (m_light == 0) return;

	// RpLightSetColor
	float rwColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	((void (*)(uintptr_t, float*))(g_libGTASA + 0x1E333C + 1))(m_light, rwColor);

	// RwCameraCreate
	m_camera = ((uintptr_t(*)())(g_libGTASA + 0x1ADA1C + 1))();

	// RwFrameCreate
	m_frame = ((uintptr_t(*)())(g_libGTASA + 0x1AE9E0 + 1))();

	// RwFrameTranslate
	float v[3] = { 0.0f, 0.0f, 50.0f };
	((void(*)(uintptr_t, float*, int))(g_libGTASA + 0x1AED7C + 1))(m_frame, v, 1);

	// RwFrameRotate
	v[0] = 1.0f; v[1] = 0.0f; v[2] = 0.0f;
	((void(*)(uintptr_t, float*, float, int))(g_libGTASA + 0x1AEDC4 + 1))(m_frame, v, 90.0f, 1);

	m_zBuffer = (uintptr_t)RwRasterCreate(256, 256, 0, rwRASTERTYPEZBUFFER);

	*(uintptr_t*)(m_camera + 0x64) = m_zBuffer;

	// RwObjectHasFrameSetFrame
	((void(*)(uintptr_t, uintptr_t))(g_libGTASA + 0x1B2988 + 1))(m_camera, m_frame);

	// RwCameraSetFarClipPlane
	((void(*)(uintptr_t, float))(g_libGTASA + 0x1AD710 + 1))(m_camera, 300.0f);

	// RwCameraSetNearClipPlane
	((void(*)(uintptr_t, float))(g_libGTASA + 0x1AD6F4 + 1))(m_camera, 0.01f);

	// RwCameraSetViewWindow
	float view[2] = { 0.5f, 0.5f };
	((void(*)(uintptr_t, float*))(g_libGTASA + 0x1AD924 + 1))(m_camera, view);

	// RwCameraSetProjection
	((void(*)(uintptr_t, int))(g_libGTASA + 0x1AD8DC+ 1))(m_camera, 1);

	// RpWorldAddCamera
	uintptr_t pRwWorld = *(uintptr_t*)(g_libGTASA + 0x95B060);
	if (pRwWorld) {
		((void(*)(uintptr_t, uintptr_t))(g_libGTASA + 0x1EB118 + 1))(pRwWorld, m_camera);
	}
}

uintptr_t CSnapShotHelper::CreatePedSnapShot(int iModel, uint32_t dwColor, VECTOR* vecRot, float fZoom)
{
	Log(OBFUSCATE("CreatePedSnapShot: %d, %f, %f, %f"), iModel, vecRot->X, vecRot->Y, vecRot->Z);

    if(m_bActiveCacheTextures){
        uintptr_t getTextureCache = GetCacheTexture(iModel);
        if(getTextureCache != 0){
            return getTextureCache;
        }
    }

	uintptr_t raster = (uintptr_t)RwRasterCreate(256, 256, 32, rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE);
	// RwTextureCreate
	uintptr_t bufferTexture = ((uintptr_t(*)(uintptr_t))(g_libGTASA + 0x1B1B4C + 1))(raster);

	CPlayerPed *pPed = new CPlayerPed(208, 0, 0.0f, 0.0f, 0.0f, 0.0f);

	if(!raster || !bufferTexture || !pPed) return 0;

	float posZ = iModel == 162 ? 50.15f : 50.05f;
	float posY = fZoom * -2.25f;
	pPed->TeleportTo(0.0f, posY, posZ);
	pPed->SetModelIndex(iModel);
	pPed->SetGravityProcessing(false);
	pPed->SetCollisionChecking();

	MATRIX4X4 mat;
	pPed->GetMatrix(&mat);

	if(vecRot->X != 0.0f)
		RwMatrixRotate(&mat, eAxis::X, vecRot->X);
	if(vecRot->Y != 0.0f)
		RwMatrixRotate(&mat, eAxis::Y, vecRot->Y);
	if(vecRot->Z != 0.0f)
		RwMatrixRotate(&mat, eAxis::Z, vecRot->Z);

	pPed->UpdateMatrix(mat);

	// set camera frame buffer //
	*(uintptr_t*)(m_camera + 0x60) = raster;
	// CVisibilityPlugins::SetRenderWareCamera
	((void(*)(uintptr_t))(g_libGTASA + 0x55CFA4 + 1))(m_camera);

	// RwCameraClear
	((void(*)(uintptr_t, uint32_t*, int))(g_libGTASA + 0x1AD8A0 + 1))(m_camera, &dwColor, 3);

	RwCameraBeginUpdate((RwCamera*)m_camera);

	// RpWorldAddLight
	uintptr_t pRwWorld = *(uintptr_t*)(g_libGTASA + 0x95B060);
	if (pRwWorld) {
		((void(*)(uintptr_t, uintptr_t))(g_libGTASA + 0x1EB868 + 1))(pRwWorld, m_light);
	}

	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
	RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODENASHADEMODE);
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)0);
	RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODENACULLMODE);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)false);

	// DefinedState
	((void(*) (void))(g_libGTASA + 0x559008 + 1))();

	pPed->Add();
	
	pPed->ClumpUpdateAnimations(100.0f, 1);
	pPed->Render();

	RwCameraEndUpdate((RwCamera*)m_camera);

	// RpWorldRemoveLight
	if (pRwWorld) {
		((void(*)(uintptr_t, uintptr_t))(g_libGTASA + 0x1EB8F0 + 1))(pRwWorld, m_light);
	}

	pPed->Remove();

	delete pPed;
	Log(OBFUSCATE("Done"));

    CacheTexture(iModel, bufferTexture);

	return bufferTexture;
}
#include "..//chatwindow.h"
extern CChatWindow* pChatWindow;
uintptr_t CSnapShotHelper::CreateVehicleSnapShot(int iModel, uint32_t dwColor, VECTOR* vecRot, float fZoom, int dwColor1, int dwColor2)
{
	Log(OBFUSCATE("CreateVehicleSnapShot: %d %f, %f, %f"), iModel, vecRot->X, vecRot->Y, vecRot->Z, dwColor1, dwColor2);

    if(m_bActiveCacheTextures){
        uintptr_t getTextureCache = GetCacheTexture(iModel);
        if(getTextureCache != 0){
            return getTextureCache;
        }
    }

	uintptr_t raster = (uintptr_t)RwRasterCreate(256, 256, 32, rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE);
	// RwTextureCreate
	uintptr_t bufferTexture = ((uintptr_t(*)(uintptr_t))(g_libGTASA + 0x1B1B4C + 1))(raster);

	if(iModel == 570)
	{
		iModel = 538;
	}
	else if(iModel == 569)
	{
		iModel = 537;
	}

	CVehicle *pVehicle = new CVehicle(iModel, 0.0f, 0.0f, 50.0f, 0.0f);

	if(!raster || !bufferTexture || !pVehicle)
		return 0;

	pVehicle->SetGravityProcessing(false);
	pVehicle->SetCollisionChecking();

	float fRadius = GetModelColSphereRadius(iModel);
	float posY = (-1.0 - (fRadius + fRadius)) * fZoom;

	if(pVehicle->GetVehicleSubtype() == VEHICLE_SUBTYPE_BOAT)
	{
		posY = -5.5 - fRadius * 2.5;
	}

	pVehicle->TeleportTo(0.0f, posY, 50.0f);

	if(dwColor1 != 0xFFFF && dwColor2 != 0xFFFF)
		pVehicle->SetColor(dwColor1, dwColor2);

	MATRIX4X4 mat;
	pVehicle->GetMatrix(&mat);

	if(vecRot->X != 0.0f)
		RwMatrixRotate(&mat, eAxis::X, vecRot->X);
	if(vecRot->Y != 0.0f)
		RwMatrixRotate(&mat, eAxis::Y, vecRot->Y);
	if(vecRot->Z != 0.0f)
		RwMatrixRotate(&mat, eAxis::Z, vecRot->Z);

	pVehicle->UpdateMatrix(mat);

	// set camera frame buffer //
	*(uintptr_t*)(m_camera + 0x60) = raster;
	// CVisibilityPlugins::SetRenderWareCamera
	((void(*)(uintptr_t))(g_libGTASA + 0x55CFA4 + 1))(m_camera);

	// RwCameraClear

	((void(*)(uintptr_t, uint32_t*, int))(g_libGTASA + 0x1AD8A0 + 1))(m_camera, &dwColor, 3);

	RwCameraBeginUpdate((RwCamera*)m_camera);

	// RpWorldAddLight
	uintptr_t pRwWorld = *(uintptr_t*)(g_libGTASA + 0x95B060);
	if (pRwWorld) {
		((void(*)(uintptr_t, uintptr_t))(g_libGTASA + 0x1EB868 + 1))(pRwWorld, m_light);
	}

	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
	RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODENASHADEMODE);
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)0);
	RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODENACULLMODE);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)false);

	// DefinedState
	((void(*) (void))(g_libGTASA + 0x559008 + 1))();

	pVehicle->Add();
	pVehicle->Render();

	RwCameraEndUpdate((RwCamera*)m_camera);

	// RpWorldRemoveLight
	if (pRwWorld) {
		((void(*)(uintptr_t, uintptr_t))(g_libGTASA + 0x1EB8F0 + 1))(pRwWorld, m_light);
	}

	pVehicle->Remove();

	delete pVehicle;

    CacheTexture(iModel, bufferTexture);

	return bufferTexture;
}

uintptr_t CSnapShotHelper::CreateObjectSnapShot(int iModel, uint32_t dwColor, VECTOR* vecRot, float fZoom)
{
	if (iModel == 1373 || iModel == 3118 || iModel == 3552 || iModel == 3553)
		iModel = 18631;

    Log(OBFUSCATE("CreateObjectSnapShot: %d %f, %f, %f, (%d, %d)"), iModel, vecRot->X, vecRot->Y, vecRot->Z);


    if(m_bActiveCacheTextures){
        uintptr_t getTextureCache = GetCacheTexture(iModel);
        if(getTextureCache != 0){
            return getTextureCache;
        }
    }

	bool bNeedRemoveModel = false;
	if (!pGame->IsModelLoaded(iModel))
	{
		pGame->RequestModel(iModel);
		pGame->LoadRequestedModels();
		while (!pGame->IsModelLoaded(iModel)) sleep(1);
		bNeedRemoveModel = true;
	}

	uintptr_t pRwObject = ModelInfoCreateInstance(iModel);

	float fRadius = GetModelColSphereRadius(iModel);

	VECTOR vecCenter = { 0.0f, 0.0f, 0.0f };
	GetModelColSphereVecCenter(iModel, &vecCenter);

	uintptr_t parent = *(uintptr_t*)(pRwObject + 4);

	if (parent == 0) return 0;
	// RwFrameTranslate
	float v[3] = {
		-vecCenter.X,
		(-0.1f - fRadius * 2.25f) * fZoom,
		50.0f - vecCenter.Z };
	((void(*)(uintptr_t, float*, int))(g_libGTASA + 0x1AED7C + 1))(parent, v, 1);

	if (iModel == 18631)
		{
			// RwFrameRotate X
			v[0] = 0.0f;
			v[1] = 0.0f;
			v[2] = 1.0f;
			((void(*)(uintptr_t, float*, float, int))(g_libGTASA + 0x1AEDC4 + 1))(parent, v, 180.0f, 1);
		}
	else
	{
		if (vecRot->X != 0.0f)
		{
			// RwFrameRotate X
			v[0] = 1.0f;
			v[1] = 0.0f;
			v[2] = 0.0f;
			((void(*)(uintptr_t, float*, float, int))(g_libGTASA + 0x1AEDC4 + 1))(parent, v, vecRot->X, 1);
		}

		if (vecRot->Y != 0.0f)
		{
			// RwFrameRotate Y
			v[0] = 0.0f;
			v[1] = 1.0f;
			v[2] = 0.0f;
			((void(*)(uintptr_t, float*, float, int))(g_libGTASA + 0x1AEDC4 + 1))(parent, v, vecRot->Y, 1);
		}

		if (vecRot->Z != 0.0f)
		{
			// RwFrameRotate Z
			v[0] = 0.0f;
			v[1] = 0.0f;
			v[2] = 1.0f;
			((void(*)(uintptr_t, float*, float, int))(g_libGTASA + 0x1AEDC4 + 1))(parent, v, vecRot->Z, 1);
		}
	}

	// RENDER DEFAULT //

	// set camera frame buffer //

	uintptr_t raster = (uintptr_t)RwRasterCreate(256, 256, 32, rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE);
	// RwTextureCreate
	uintptr_t bufferTexture = ((uintptr_t(*)(uintptr_t))(g_libGTASA + 0x1B1B4C + 1))(raster);
	*(uintptr_t*)(m_camera + 0x60) = raster;

	// CVisibilityPlugins::SetRenderWareCamera
	((void(*)(uintptr_t))(g_libGTASA + 0x55CFA4 + 1))(m_camera);

	ProcessCamera(pRwObject, dwColor);

	DestroyAtomicOrClump(pRwObject);

	if (bNeedRemoveModel) {
		pGame->RemoveModel(iModel, false);
	}

    CacheTexture(iModel, bufferTexture);

	return (uintptr_t)bufferTexture;
}

void CSnapShotHelper::ProcessCamera(uintptr_t pRwObject, uint32_t dwColor)
{
	// RwCameraClear
	((void(*)(uintptr_t, uint32_t*, int))(g_libGTASA + 0x1AD8A0 + 1))(m_camera, &dwColor, 3);

	RwCameraBeginUpdate((RwCamera*)m_camera);

	// RpWorldAddLight
	uintptr_t pRwWorld = *(uintptr_t*)(g_libGTASA + 0x95B060);
	if (pRwWorld) {
		((void(*)(uintptr_t, uintptr_t))(g_libGTASA + 0x1EB868 + 1))(pRwWorld, m_light);
	}

	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
	RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODENASHADEMODE);
	RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)0);
	RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODENACULLMODE);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)false);

	// DefinedState
	((void(*) (void))(g_libGTASA + 0x559008 + 1))();

	RenderClumpOrAtomic(pRwObject);

	RwCameraEndUpdate((RwCamera*)m_camera);

	// RpWorldRemoveLight
	if (pRwWorld) {
		((void(*)(uintptr_t, uintptr_t))(g_libGTASA + 0x1EB8F0 + 1))(pRwWorld, m_light);
	}
}