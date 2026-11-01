#pragma once

#include "RW/RenderWare.h"

#include <vector>

class CObject : public CEntity
{
public:
	MATRIX4X4	m_matTarget;
	MATRIX4X4	m_matCurrent;
	uint8_t		m_byteMoving;
	float		m_fMoveSpeed;
	bool		m_bIsPlayerSurfing;
	bool		m_bNeedRotate;
	
	int			m_ObjectModel;

	CQuaternion m_quatTarget;
	CQuaternion m_quatStart;

	MaterialInfo m_pMaterials[MAX_MATERIALS];
	bool		m_bMaterials;


	VECTOR 		m_vecAttachedOffset;
	VECTOR 		m_vecAttachedRotation;
	uint16_t 	m_usAttachedVehicle;
	uint8_t 	m_bAttachedType;
	bool		m_bAttached;

	uint32_t m_setTextureColor[2];
	uint8_t m_setTextureAlpha[2];
	RwTexture* m_pLastTextureAttach[2];
	//uint32_t m_setTextureAlpha[2];
	RwTexture* m_cacheTextureColor[2];

	VECTOR 		m_vecRot;
	VECTOR		m_vecTargetRot;
	VECTOR		m_vecTargetRotTar;
	VECTOR		m_vecRotationTarget;
	VECTOR		m_vecSubRotationTarget;
	float		m_fDistanceToTargetPoint;
	uint32_t	m_dwMoveTick;
	// MATERIAL TEXT
    RwTexture*	m_MaterialTextTexture[MAX_MATERIALS_PER_MODEL];
    RwRGBA*		m_dwMaterialTextColor[MAX_MATERIALS_PER_MODEL];
    bool		m_bHasMaterialText;
    // ---
	
	static std::vector<CObject*> ms_aPaintedObjects;
	void PaintMaterial(uint8_t ucColor1, uint8_t ucColor2);

	CObject(int iModel, float fPosX, float fPosY, float fPosZ, VECTOR vecRot, float fDrawDistance);
	~CObject();

	void Process(float fElapsedTime);
	float DistanceRemaining(MATRIX4X4 *matPos);
	float RotaionRemaining(VECTOR matPos);
	float DistanceRemaining(MATRIX4X4 *matPos, MATRIX4X4 *m_matPositionTarget);
	float RotaionRemaining(VECTOR matPos, VECTOR m_vecRot);

	void SetPos(float x, float y, float z);

	void SetColor(uint32_t color1, uint32_t color2);
	void SetColorAlpha(uint8_t alpha1, uint8_t alpha2);
	//void SetColorAlpha(uint32_t alpha1, uint32_t alpha2);

	void MoveTo(float x, float y, float z, float speed, float rX, float rY, float rZ);
	void ApplyMoveSpeed();
	void GetRotation(float* pfX, float* pfY, float* pfZ);

	void AttachToVehicle(uint16_t usVehID, VECTOR* pVecOffset, VECTOR* pVecRot);
	void ProcessAttachToVehicle(CVehicle* pVehicle);

	void InstantRotate(float x, float y, float z);
	void StopMoving();
// MT
   void SetMaterialText(int iMaterialIndex, uint8_t byteMaterialSize, const char *szFontName, uint8_t byteFontSize, uint8_t byteFontBold, uint32_t dwFontColor, uint32_t dwBackgroundColor, uint8_t byteAlign, const char *szText);
};