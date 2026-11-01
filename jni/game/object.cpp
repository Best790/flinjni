#include "../main.h"
#include "game.h"
#include "../net/netgame.h"
#include <cmath>
#include "materialtext.h"

extern CGame *pGame;
extern CNetGame *pNetGame;
extern CMaterialText *pMaterialText;

std::vector<CObject*> CObject::ms_aPaintedObjects;

float fixAngle(float angle)
{
	if (angle > 180.0f) angle -= 360.0f;
	if (angle < -180.0f) angle += 360.0f;

	return angle;
}

float subAngle(float a1, float a2)
{
	return fixAngle(fixAngle(a2) - a1);
}

CObject::CObject(int iModel, float fPosX, float fPosY, float fPosZ, VECTOR vecRot, float fDrawDistance)
{
	uint32_t dwRetID 	= 0;
	m_pEntity 			= nullptr;
	m_dwGTAId 			= 0;
	m_ObjectModel		= iModel;

	ScriptCommand(&create_object, iModel, fPosX, fPosY, fPosZ, &dwRetID);
	ScriptCommand(&put_object_at, dwRetID, fPosX, fPosY, fPosZ);

	m_pEntity = GamePool_Object_GetAt(dwRetID);
	m_dwGTAId = dwRetID;
	m_byteMoving = 0;
	m_fMoveSpeed = 0.0;

	m_vecRot = vecRot;
	m_vecTargetRotTar = vecRot;
	
	m_bIsPlayerSurfing = false;
	m_bNeedRotate = false;
	
	m_bAttached = false;
	m_bAttachedType = 0;
	m_usAttachedVehicle = 0xFFFF;

	for(int i = 0; i < 2; i++){
		m_pLastTextureAttach[i] = nullptr;
		m_pLastTextureAttach[i] = nullptr;
		m_setTextureColor[i] = 0;
		m_setTextureColor[i] = 0;
		m_setTextureAlpha[i] = 0;
		m_setTextureAlpha[i] = 0;
		//m_setTextureAlpha[i] = 0;
		//m_setTextureAlpha[i] = 0;
		m_cacheTextureColor[i] = nullptr;
		m_cacheTextureColor[i] = nullptr;
	}


	m_bMaterials = false;
	for (auto & m_pMaterial : m_pMaterials)
	{
		m_pMaterial.m_bCreated = 0;
		m_pMaterial.pTex = nullptr;
	}
	
    m_bHasMaterialText = false;
	for(int i = 0; i <= MAX_MATERIALS_PER_MODEL; i++)
    {
        m_MaterialTextTexture[i] = 0;
    }

	InstantRotate(vecRot.X, vecRot.Y, vecRot.Z);
}
// todo
CObject::~CObject()
{
	m_bMaterials = false;
	for (auto & m_pMaterial : m_pMaterials)
	{
		if (m_pMaterial.m_bCreated && m_pMaterial.pTex)
		{
			m_pMaterial.m_bCreated = 0;
			RwTextureDestroy(m_pMaterial.pTex);
			m_pMaterial.pTex = nullptr;
		}
	}
	
	m_bHasMaterialText = false;
	for(int i = 0; i <= MAX_MATERIALS_PER_MODEL; i++)
    {
		if (m_MaterialTextTexture[i])
		{
			RwTextureDestroy(m_MaterialTextTexture[i]);
		}
        m_MaterialTextTexture[i] = 0;
    }
	
	m_pEntity = GamePool_Object_GetAt(m_dwGTAId);
	if(m_pEntity)
		ScriptCommand(&destroy_object, m_dwGTAId);

	pNetGame->GetObjectPool()->RemoveColoredObject(this);

	if(m_pLastTextureAttach[0] != nullptr){
	    RwTextureDestroy(m_pLastTextureAttach[0]);
	}
	if(m_pLastTextureAttach[1] != nullptr){
        RwTextureDestroy(m_pLastTextureAttach[1]);
    }
}

void CObject::Process(float fElapsedTime)
{
	if (m_bAttachedType == 1 && !m_bAttached)
	{
		CVehicle* pVehicle = pNetGame->GetVehiclePool()->GetAt(m_usAttachedVehicle);
		if (pVehicle)
		{
			if (pVehicle->IsAdded())
			{
				if (m_vecAttachedOffset.X > 10000.0f || m_vecAttachedOffset.Y > 10000.0f || m_vecAttachedOffset.Z > 10000.0f ||
					m_vecAttachedOffset.X < -10000.0f || m_vecAttachedOffset.Y < -10000.0f || m_vecAttachedOffset.Z < -10000.0f)
				{ 
					// пропускаем действие
				}
				else
				{	
					m_bAttached = true;
					ProcessAttachToVehicle(pVehicle);
				}
			}
		}
	}
	m_pEntity = GamePool_Object_GetAt(m_dwGTAId);
	if (!m_pEntity) return;
	if (!(m_pEntity->mat)) return;
	if (m_byteMoving & 1)
	{
		VECTOR vecSpeed = { 0.0f, 0.0f, 0.0f };
		MATRIX4X4 matEnt;
		GetMatrix(&matEnt);
		float distance = fElapsedTime * m_fMoveSpeed;
		float remaining = DistanceRemaining(&matEnt);
		uint32_t dwThisTick = GetTickCount();

		float posX = matEnt.pos.X;
		float posY = matEnt.pos.Y;
		float posZ = matEnt.pos.Z;

		float f1 = ((float)(dwThisTick - m_dwMoveTick)) * 0.001f * m_fMoveSpeed;
		float f2 = m_fDistanceToTargetPoint - remaining;

		if (distance >= remaining)
		{
			StopMoving();
			TeleportTo(m_matTarget.pos.X, m_matTarget.pos.Y, m_matTarget.pos.Z);
	
			Log("[CObject] distance >= remaining");
			InstantRotate(m_vecTargetRot.X, m_vecTargetRot.Y, m_vecTargetRot.Z);
			return;
		}

		if (fElapsedTime <= 0.0f)
			return;

		float delta = 1.0f / (remaining / distance);
		matEnt.pos.X += ((m_matTarget.pos.X - matEnt.pos.X) * delta);
		matEnt.pos.Y += ((m_matTarget.pos.Y - matEnt.pos.Y) * delta);
		matEnt.pos.Z += ((m_matTarget.pos.Z - matEnt.pos.Z) * delta);

		distance = remaining / m_fDistanceToTargetPoint;
		float slerpDelta = 1.0f - distance;

		delta = 1.0f / fElapsedTime;
		vecSpeed.X = (matEnt.pos.X - posX) * delta * 0.02f;
		vecSpeed.Y = (matEnt.pos.Y - posY) * delta * 0.02f;
		vecSpeed.Z = (matEnt.pos.Z - posZ) * delta * 0.02f;

		if (FloatOffset(f1, f2) > 0.1f)
		{
			if (f1 > f2)
			{
				delta = (f1 - f2) * 0.1f + 1.0f;
				vecSpeed.X *= delta;
				vecSpeed.Y *= delta;
				vecSpeed.Z *= delta;
			}

			if (f2 > f1)
			{
				delta = 1.0f - (f2 - f1) * 0.1f;
				vecSpeed.X *= delta;
				vecSpeed.Y *= delta;
				vecSpeed.Z *= delta;
			}
		}

		SetMoveSpeedVector(vecSpeed);
		ApplyMoveSpeed();

		if (m_bNeedRotate)
		{
			float fx, fy, fz;
			GetRotation(&fx, &fy, &fz);
			distance = m_vecRotationTarget.Z - distance * m_vecSubRotationTarget.Z;
			vecSpeed.X = 0.0f;
			vecSpeed.Y = 0.0f;
			vecSpeed.Z = subAngle(remaining, distance) * 0.01f;
			if (vecSpeed.Z <= 0.001f)
			{
				if (vecSpeed.Z < -0.001f)
					vecSpeed.Z = -0.001f;
			}
			else
			{
				vecSpeed.Z = 0.001f;
			}

			SetTurnSpeedVector(vecSpeed);
			GetMatrix(&matEnt);
			CQuaternion quat;
			quat.Slerp(&m_quatStart, &m_quatTarget, slerpDelta);
			quat.Normalize();
			quat.GetMatrix(&matEnt);
		}
		else
		{
			GetMatrix(&matEnt);
		}

		UpdateMatrix(matEnt);
	}
}

float CObject::DistanceRemaining(MATRIX4X4 *matPos)
{
	float	fSX,fSY,fSZ;
	fSX = (matPos->pos.X - m_matTarget.pos.X) * (matPos->pos.X - m_matTarget.pos.X);
	fSY = (matPos->pos.Y - m_matTarget.pos.Y) * (matPos->pos.Y - m_matTarget.pos.Y);
	fSZ = (matPos->pos.Z - m_matTarget.pos.Z) * (matPos->pos.Z - m_matTarget.pos.Z);
	return (float)sqrt(fSX + fSY + fSZ);
}

float CObject::RotaionRemaining(VECTOR matPos)
{
	float fSX, fSY, fSZ;
	fSX = (matPos.X - m_vecTargetRot.X) * (matPos.X - m_vecTargetRot.X);
	fSY = (matPos.Y - m_vecTargetRot.Y) * (matPos.Y - m_vecTargetRot.Y);
	fSZ = (matPos.Z - m_vecTargetRot.Z) * (matPos.Z - m_vecTargetRot.Z);
	return (float)sqrt(fSX + fSY + fSZ);
}

float CObject::DistanceRemaining(MATRIX4X4 *matPos, MATRIX4X4 *m_matPositionTarget)
{
	float	fSX,fSY,fSZ;
	fSX = (matPos->pos.X - m_matPositionTarget->pos.X) * (matPos->pos.X - m_matPositionTarget->pos.X);
	fSY = (matPos->pos.Y - m_matPositionTarget->pos.Y) * (matPos->pos.Y - m_matPositionTarget->pos.Y);
	fSZ = (matPos->pos.Z - m_matPositionTarget->pos.Z) * (matPos->pos.Z - m_matPositionTarget->pos.Z);
	return (float)sqrt(fSX + fSY + fSZ);
}

float CObject::RotaionRemaining(VECTOR matPos, VECTOR m_vecRot)
{
	float fSX,fSY,fSZ;
	fSX = (matPos.X - m_vecRot.X) * (matPos.X - m_vecRot.X);
	fSY = (matPos.Y - m_vecRot.Y) * (matPos.Y - m_vecRot.Y);
	fSZ = (matPos.Z - m_vecRot.Z) * (matPos.Z - m_vecRot.Z);
	return (float)sqrt(fSX + fSY + fSZ);
}

void CObject::SetPos(float x, float y, float z)
{
	if (GamePool_Object_GetAt(m_dwGTAId))
		ScriptCommand(&put_object_at, m_dwGTAId, x, y, z);
}

void CObject::SetColorAlpha(uint8_t alpha1, uint8_t alpha2){
	m_setTextureAlpha[0] = alpha1;
	m_setTextureAlpha[1] = alpha2;

	pNetGame->GetObjectPool()->AddColoredObject(this);
}

void CObject::SetColor(uint32_t color1, uint32_t color2){
	m_setTextureColor[0] = color1;
	m_setTextureColor[1] = color2;

	pNetGame->GetObjectPool()->AddColoredObject(this);
}

void CObject::StopMoving()
{
	m_byteMoving = 0;
}

void CObject::MoveTo(float fX, float fY, float fZ, float fSpeed, float fRotX, float fRotY, float fRotZ)
{
	MATRIX4X4 mat;
	GetMatrix(&mat);

	if (m_byteMoving & 1) {
		StopMoving();
	}

	m_dwMoveTick = GetTickCount();
	m_fMoveSpeed = fSpeed;
	m_matTarget.pos.X = fX;
	m_matTarget.pos.Y = fY;
	m_matTarget.pos.Z = fZ;

	m_vecTargetRot.X = fRotX;
	m_vecTargetRot.Y = fRotY;
	m_vecTargetRot.Z = fRotZ;
	m_byteMoving |= 1;

	if (fRotX <= -999.0f || fRotY <= -999.0f || fRotZ <= -999.0f) {
		m_bNeedRotate = false;
	}
	else
	{
		m_bNeedRotate = true;

		VECTOR vecRot;
		MATRIX4X4 matrix;
		GetRotation(&vecRot.X, &vecRot.Y, &vecRot.Z);
		m_vecRotationTarget.X = fixAngle(fRotX);
		m_vecRotationTarget.Y = fixAngle(fRotY);
		m_vecRotationTarget.Z = fixAngle(fRotZ);

		m_vecSubRotationTarget.X = subAngle(vecRot.X, fRotX);
		m_vecSubRotationTarget.Y = subAngle(vecRot.Y, fRotY);
		m_vecSubRotationTarget.Z = subAngle(vecRot.Z, fRotZ);

		Log("[MoveTo] InstantRotate1");
		InstantRotate(fRotX, fRotY, fRotZ);
		GetMatrix(&matrix);

		m_matTarget.right = matrix.right;
		m_matTarget.at = matrix.at;
		m_matTarget.up = matrix.up;

		Log("[MoveTo] InstantRotate2");
		InstantRotate(vecRot.X, vecRot.Y, vecRot.Z);
		GetMatrix(&matrix);

		m_quatStart.SetFromMatrix(matrix);
		m_quatTarget.SetFromMatrix(m_matTarget);
		m_quatStart.Normalize();
		m_quatTarget.Normalize();
	}

	m_fDistanceToTargetPoint = GetDistanceFromPoint(m_matTarget.pos.X, m_matTarget.pos.Y, m_matTarget.pos.Z);

	// sub_1009F070
	m_pEntity->m_nEntityFlags &= 0xFFFFFFF7;
}

void CObject::ApplyMoveSpeed()
{
	if (m_pEntity)
	{
		float fTimeStep = *(float*)(g_libGTASA + 0x8C9BB4);

		MATRIX4X4 mat;
		GetMatrix(&mat);
		mat.pos.X += fTimeStep * m_pEntity->vecMoveSpeed.X;
		mat.pos.Y += fTimeStep * m_pEntity->vecMoveSpeed.Y;
		mat.pos.Z += fTimeStep * m_pEntity->vecMoveSpeed.Z;
		UpdateMatrix(mat);
	}
}

void CObject::GetRotation(float* pfX, float* pfY, float* pfZ)
{
	if (m_pEntity)
	{
		MATRIX4X4* mat = m_pEntity->mat;

		if (mat)
		{
			// CMatrix::ConvertToEulerAngles
			((void(*)(PMATRIX4X4, float*, float*, float*, int))(g_libGTASA + 0x3E8098 + 1))(mat, pfX, pfY, pfZ, 21);
		}

		*pfX = *pfX * 57.295776 * -1.0;
		*pfY = *pfY * 57.295776 * -1.0;
		*pfZ = *pfZ * 57.295776 * -1.0;
	}
}

void CObject::AttachToVehicle(uint16_t usVehID, VECTOR* pVecOffset, VECTOR* pVecRot)
{
	m_bAttached = false;
	m_bAttachedType = 1;
	m_usAttachedVehicle = usVehID;
	m_vecAttachedOffset.X = pVecOffset->X;
	m_vecAttachedOffset.Y = pVecOffset->Y;
	m_vecAttachedOffset.Z = pVecOffset->Z;

	m_vecAttachedRotation.X = pVecRot->X;
	m_vecAttachedRotation.Y = pVecRot->Y;
	m_vecAttachedRotation.Z = pVecRot->Z;
}

void CObject::ProcessAttachToVehicle(CVehicle* pVehicle)
{
	if (GamePool_Object_GetAt(m_dwGTAId))
	{
		m_pEntity = GamePool_Object_GetAt(m_dwGTAId);
		*(uint32_t*)((uintptr_t)m_pEntity + 28) &= 0xFFFFFFFE;

		if (!ScriptCommand(&is_object_attached, m_dwGTAId))
		{
			ScriptCommand(&attach_object_to_car, m_dwGTAId, pVehicle->m_dwGTAId, m_vecAttachedOffset.X,
				m_vecAttachedOffset.Y, m_vecAttachedOffset.Z, m_vecAttachedRotation.X, m_vecAttachedRotation.Y, m_vecAttachedRotation.Z);
		}
	}
}

void CObject::InstantRotate(float x, float y, float z)
{
	if (GamePool_Object_GetAt(m_dwGTAId))
	{
		m_pEntity = GamePool_Object_GetAt(m_dwGTAId);
		
		if (!m_pEntity) return;
		if (!(m_pEntity->mat)) return;
		
		MATRIX4X4 m_matObject;
		GetMatrix(&m_matObject);
		
		float f1 = x * 0.0174533;
		float f2 = y * 0.0174533;
		float f3 = z * 0.0174533;

		float f1cos = cos(f1);
		float f1sin = sin(f1);
		float f2cos = cos(f2);
		float f2sin = sin(f2);
		float f3cos = cos(f3);
		float f3sin = sin(f3);

		m_matObject.right.X = f3cos * f2cos - (f3sin * f1sin) * f2sin;
		m_matObject.right.Y = (f3cos * f1sin) * f2sin + f3sin * f2cos;
		m_matObject.right.Z = -(f2sin * f1cos);

		m_matObject.up.X = -(f3sin * f1cos);
		m_matObject.up.Y = f3cos * f1cos;
		m_matObject.up.Z = f1sin;
				
		m_matObject.at.X = (f3sin * f1sin) * f2cos + f3cos * f2sin;
		m_matObject.at.Y = f3sin * f2sin - (f3cos * f1sin) * f2cos;
		m_matObject.at.Z = f2cos * f1cos;
		UpdateMatrix(m_matObject);
	}
}

void CObject::SetMaterialText(int iMaterialIndex, uint8_t byteMaterialSize, const char *szFontName, uint8_t byteFontSize, uint8_t byteFontBold, uint32_t dwFontColor, uint32_t dwBackgroundColor, uint8_t byteAlign, const char *szText)
{
    if (iMaterialIndex < 16)
	{
		if(m_MaterialTextTexture[iMaterialIndex])
    	{
        	RwTextureDestroy(m_MaterialTextTexture[iMaterialIndex]);
        	m_MaterialTextTexture[iMaterialIndex] = 0;
    	}
    	m_MaterialTextTexture[iMaterialIndex] = pMaterialText->Generate(byteMaterialSize, szFontName, byteFontSize, byteFontBold, dwFontColor, dwBackgroundColor, byteAlign, szText);
    	//m_dwMaterialTextColor[iMaterialIndex] = 0;
    	m_bHasMaterialText = true;
	}
}

void CObject::PaintMaterial(uint8_t ucColor1, uint8_t ucColor2)
{
	RwObject* pObject = (RwObject*)m_pEntity->m_RwObject;
	if(!pObject) {
		return;
	}
	
	if (pObject->type != rpATOMIC) {
		return;
	}

	RpAtomic* pAtomic = (RpAtomic*)pObject;
	RpGeometry* pGeometry = pAtomic->geometry;
	if (!pGeometry) {
		return;
	}
	
	if(pGeometry->matList.numMaterials <= 0) {
		return;
	}
	
	bool bPainted = false;
	
	if(ucColor1 != 0) {
		RpMaterial* pMaterial = pGeometry->matList.materials[0];
		if (pMaterial) {
			m_MaterialTextTexture[0] = pMaterialText->PaintTexture(pMaterial->texture, ucColor1);
			bPainted = true;
		}
	}
	
	if(ucColor2 != 0) {
		RpMaterial* pMaterial = pGeometry->matList.materials[1];
		if (pMaterial) {
			m_MaterialTextTexture[1] = pMaterialText->PaintTexture(pMaterial->texture, ucColor2);
			bPainted = true;
		}
	}
	
	if(bPainted) {
		ms_aPaintedObjects.push_back(this);
		m_bHasMaterialText = true;
	}
}