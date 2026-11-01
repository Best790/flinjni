#pragma once

#define MAX_CACHE_TEXTURE   256

struct sCacheTexture{
    uintptr_t iModel;
    uintptr_t pTexture;
};

class CSnapShotHelper
{
public:
	CSnapShotHelper();

	uintptr_t CreatePedSnapShot(int iModel, uint32_t dwColor, VECTOR* vecRot, float fZoom);
	uintptr_t CreateVehicleSnapShot(int iModel, uint32_t dwColor, VECTOR* vecRot, float fZoom, int dwColor1, int dwColor2);
	uintptr_t CreateObjectSnapShot(int iModel, uint32_t dwColor, VECTOR* vecRot, float fZoom);

    void SetActiveCacheTexture(bool bActive);
    void CacheTexture(uintptr_t iModel,uintptr_t pTexture);
    void ClearCacheTexture();
    uintptr_t GetCacheTexture(uintptr_t iModel);

private:
	void SetUpScene();
	void ProcessCamera(uintptr_t pRwObject, uint32_t dwColor);

	uintptr_t m_camera;
	uintptr_t m_light;
	uintptr_t m_frame;
	uintptr_t m_zBuffer;
	uintptr_t m_raster;

    sCacheTexture m_cacheTextures[MAX_CACHE_TEXTURE];
    bool m_bActiveCacheTextures;
};