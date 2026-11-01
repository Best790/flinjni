#include "../main.h"
#include "game.h"
#include "../net/netgame.h"
#include "../util/armhook.h"
#include "RW/common.h"
#include "chatwindow.h"
#include "dialog.h"

#include "..//CDebugInfo.h"

extern CGame* pGame;
extern CNetGame* pNetGame;
extern CChatWindow* pChatWindow;
extern CDialogWindow* pDialogWindow;

CPlayerPed* g_pCurrentFiredPed;
BULLET_DATA* g_pCurrentBulletData;

CPlayerPed::CPlayerPed()
{
    Log("created player 2");
	m_dwGTAId = 1;
	m_pPed = (PED_TYPE*)GamePool_FindPlayerPed();
	m_pEntity = (ENTITY_TYPE*)GamePool_FindPlayerPed();

	m_bytePlayerNumber = 0;
	SetPlayerPedPtrRecord(m_bytePlayerNumber,(uintptr_t)m_pPed);
	ScriptCommand(&set_actor_weapon_droppable, m_dwGTAId, 1);
	ScriptCommand(&set_char_never_targeted, m_dwGTAId, 1);
	ScriptCommand(&set_actor_can_be_decapitated, m_dwGTAId, 0);
	m_dwArrow = 0;


    m_bLockTogglePlayerControllable = false;


	for (int i = 0; i < MAX_ATTACHED_OBJECTS; i++)
	{
		m_aAttachedObjects[i].bState = false;
	}
	m_bHaveBulletData = false;
    m_bWaitAnimationGunFlash = false;
}

CPlayerPed::CPlayerPed(uint8_t bytePlayerNumber, int iSkin, float fX, float fY, float fZ, float fRotation)
{
    Log("created player 1");
	CDebugInfo::uiStreamedPeds++;

	uint32_t dwPlayerActorID = 0;
	int iPlayerNum = bytePlayerNumber;
	m_bHaveBulletData = false;
	m_pPed = nullptr;
	m_dwGTAId = 0;
	m_dwArrow = 0;

    m_bLockTogglePlayerControllable = false;

    m_bWaitAnimationGunFlash = false;

	ScriptCommand(&create_player, &iPlayerNum, fX, fY, fZ, &dwPlayerActorID);
	ScriptCommand(&create_actor_from_player, &iPlayerNum, &dwPlayerActorID);

	m_dwGTAId = dwPlayerActorID;
	m_pPed = GamePool_Ped_GetAt(m_dwGTAId);
	m_pEntity = (ENTITY_TYPE*)GamePool_Ped_GetAt(m_dwGTAId);

	m_pData = new CRemoteDataStorage();
	CRemoteData::AddRemoteDataStorage(m_pPed, m_pData);

	m_bytePlayerNumber = bytePlayerNumber;
	SetPlayerPedPtrRecord(m_bytePlayerNumber, (uintptr_t)m_pPed);
	ScriptCommand(&set_actor_weapon_droppable, m_dwGTAId, 1);
	ScriptCommand(&set_actor_immunities, m_dwGTAId, 0, 0, 1, 0, 0);
	ScriptCommand(&set_actor_can_be_decapitated, m_dwGTAId, 0);
	ScriptCommand(&set_char_never_targeted, m_dwGTAId, 1);

	if(pNetGame)
		SetMoney(pNetGame->m_iDeathDropMoney);

	SetModelIndex(iSkin);
	ForceTargetRotation(fRotation);

	MATRIX4X4 mat;
	GetMatrix(&mat);
	mat.pos.X = fX;
	mat.pos.Y = fY;
	mat.pos.Z = fZ + 0.15f;
	SetMatrix(mat);
	
	for (int i = 0; i < MAX_ATTACHED_OBJECTS; i++)
	{
		m_aAttachedObjects[i].bState = false;
	}

	memset(&RemotePlayerKeys[m_bytePlayerNumber], 0, sizeof(PAD_KEYS));


}

void CPlayerPed::InitSkin(){
    if(!m_pPed) return;
}

CPlayerPed::~CPlayerPed()
{
	CRemoteData::RemoveRemoteDataStorage(m_pPed);
	Destroy();
}

void CPlayerPed::Destroy()
{
	CDebugInfo::uiStreamedPeds--;
	FlushAttach();
	memset(&RemotePlayerKeys[m_bytePlayerNumber], 0, sizeof(PAD_KEYS));
	SetPlayerPedPtrRecord(m_bytePlayerNumber, 0);

	if(!m_pPed || !GamePool_Ped_GetAt(m_dwGTAId) || m_pPed->entity.vtable == 0x5C7358)
	{
		Log(OBFUSCATE("CPlayerPed::Destroy: invalid pointer/vtable"));
		m_pPed = nullptr;
		m_pEntity = nullptr;
		m_dwGTAId = 0;
		return;
	}

	/*
		if(m_dwParachute) ... (��������)
	*/

	Log(OBFUSCATE("Removing from vehicle.."));
	if(IN_VEHICLE(m_pPed))
		RemoveFromVehicleAndPutAt(100.0f, 100.0f, 10.0f);

	Log(OBFUSCATE("Setting flag state.."));
	uintptr_t dwPedPtr = (uintptr_t)m_pPed;
	*(uint32_t*)(*(uintptr_t*)(dwPedPtr + 1088) + 76) = 0;
	// CPlayerPed::Destructor
	Log(OBFUSCATE("Calling destructor.."));
	(( void (*)(PED_TYPE*))(*(void**)(m_pPed->entity.vtable+0x4)))(m_pPed);

	m_pPed = nullptr;
	m_pEntity = nullptr;
}

extern uint32_t (*CWeapon_FireInstantHit)(WEAPON_SLOT_TYPE* _this, PED_TYPE* pFiringEntity, VECTOR* vecOrigin, VECTOR* muzzlePos, ENTITY_TYPE* targetEntity, VECTOR *target, VECTOR* originForDriveBy, int arg6, int muzzle);
extern uint32_t (*CWeapon__FireSniper)(WEAPON_SLOT_TYPE *pWeaponSlot, PED_TYPE *pFiringEntity, ENTITY_TYPE *a3, VECTOR *vecOrigin); 

void CPlayerPed::FireInstant()
{
	if(!IsValidGamePed(m_pPed) || !GamePool_Ped_GetAt(m_dwGTAId))
		return;
	uint8_t byteSavedCameraMode;
	uint16_t wSavedCameraMode2;
	if(m_bytePlayerNumber) 
	{
		byteSavedCameraMode = *pbyteCameraMode;
		*pbyteCameraMode = GameGetPlayerCameraMode(m_bytePlayerNumber);

		wSavedCameraMode2 = *wCameraMode2;
		*wCameraMode2 = GameGetPlayerCameraMode(m_bytePlayerNumber);
		if(*wCameraMode2 == 4) *wCameraMode2 = 0;

		GameStoreLocalPlayerCameraExtZoom();
		GameSetRemotePlayerCameraExtZoom(m_bytePlayerNumber);

		GameStoreLocalPlayerAim();
		GameSetRemotePlayerAim(m_bytePlayerNumber);
	}
	else
	{
		byteSavedCameraMode = 0;
		wSavedCameraMode2 = 0;
	}

	g_pCurrentFiredPed = this;

	if(m_bHaveBulletData)
		g_pCurrentBulletData = &m_bulletData;
	else 
		g_pCurrentBulletData = nullptr;

	WEAPON_SLOT_TYPE* pSlot = GetCurrentWeaponSlot();
	if(pSlot) 
	{
		if (pSlot->dwType == WEAPON_SNIPER)
		//if(GetCurrentWeapon() == WEAPON_SNIPER) 
		{
			if(m_pPed){
			pSlot->dwState = 1;
				CWeapon__FireSniper(pSlot, m_pPed, nullptr, nullptr);

				}
			else 
				CWeapon__FireSniper(nullptr, m_pPed, nullptr, nullptr);
		} 
		else 
		{
			VECTOR vecBonePos;
			VECTOR vecOut;

			GetWeaponInfoForFire(true, &vecBonePos, &vecOut);

			if(m_pPed){
			    pSlot->dwState = 1;
				CWeapon_FireInstantHit(pSlot, m_pPed, &vecBonePos, &vecOut, nullptr, nullptr, nullptr, 0, 1);

				}
			else
				CWeapon_FireInstantHit(nullptr, m_pPed, &vecBonePos, &vecOut, nullptr, nullptr, nullptr, 0, 1);
		}
	}

	g_pCurrentFiredPed = nullptr;
	g_pCurrentBulletData = nullptr;
	if(m_bytePlayerNumber) 
	{
		*pbyteCameraMode = byteSavedCameraMode;
		*wCameraMode2 = wSavedCameraMode2;

		// wCamera2
		GameSetLocalPlayerCameraExtZoom();
		GameSetLocalPlayerAim();
	}
}

bool CPlayerPed::GetWaitAnimationGunFlash(){
    return m_bWaitAnimationGunFlash;
}

void CPlayerPed::SetWaitAnimationGunFlash(bool value){
    m_bWaitAnimationGunFlash = value;
}

void CPlayerPed::GetWeaponInfoForFire(int bLeft, VECTOR *vecBone, VECTOR *vecOut)
{
	if (!m_pPed) return;
	if (!GamePool_Ped_GetAt(m_dwGTAId)) return;
	if(m_pPed->entity.vtable == g_libGTASA+0x5C7358) return;
	
	VECTOR *pFireOffset = GetCurrentWeaponFireOffset();
	if(pFireOffset && vecBone && vecOut)
	{
		vecOut->X = pFireOffset->X / 5;
		vecOut->Y = pFireOffset->Y / 5;
		vecOut->Z = pFireOffset->Z / 5;

		int bone_id = 24;
		if(bLeft)
			bone_id = 34;
		
		GetBonePosition(bone_id, vecBone);

		GetTransformedBonePosition(bone_id, vecOut);
	}
}

VECTOR* CPlayerPed::GetCurrentWeaponFireOffset()
{
	if(!m_pPed) return nullptr;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return nullptr;

	WEAPON_SLOT_TYPE *pSlot = GetCurrentWeaponSlot();
	if(pSlot)
		return (VECTOR *)(GetWeaponInfo(pSlot->dwType, 1) + 0x24);
	
	return nullptr;
}

// 0.3.7
void CPlayerPed::GetTransformedBonePosition(int iBoneID, VECTOR* vecOut)
{
	if(!m_pPed) return;
	if(m_pPed->entity.vtable == 0x5C7358) return;

	(( void (*)(PED_TYPE*, VECTOR*, int, int))(g_libGTASA+0x4383C0+1))(m_pPed, vecOut, iBoneID, 0);
}

void CPlayerPed::ProcessBulletData(BULLET_DATA* btData)
{
	if (!m_pPed || !GamePool_Ped_GetAt(m_dwGTAId)) {
		return;
	}

	BULLET_SYNC_DATA bulletSyncData;

	if (btData) {
		m_bHaveBulletData = true;
		m_bulletData.pEntity = btData->pEntity;
		m_bulletData.vecOrigin.X = btData->vecOrigin.X;
		m_bulletData.vecOrigin.Y = btData->vecOrigin.Y;
		m_bulletData.vecOrigin.Z = btData->vecOrigin.Z;

		m_bulletData.vecPos.X = btData->vecPos.X;
		m_bulletData.vecPos.Y = btData->vecPos.Y;
		m_bulletData.vecPos.Z = btData->vecPos.Z;

		m_bulletData.vecOffset.X = btData->vecOffset.X;
		m_bulletData.vecOffset.Y = btData->vecOffset.Y;
		m_bulletData.vecOffset.Z = btData->vecOffset.Z;

		uint8_t byteHitType = 0;
		unsigned short InstanceID = 0xFFFF;

		if (m_bytePlayerNumber == 0)
		{
			if (pNetGame)
			{
				CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
				if (pPlayerPool)
				{
					CPlayerPed* pPlayerPed = pPlayerPool->GetLocalPlayer()->GetPlayerPed();
					if (pPlayerPed)
					{
						memset(&bulletSyncData, 0, sizeof(BULLET_SYNC_DATA));
						
						WEAPON_SLOT_TYPE* pSlot = GetCurrentWeaponSlot();
						if (pSlot->dwType != WEAPON_SNIPER || btData->pEntity)
						//if (pPlayerPed->GetCurrentWeapon() != WEAPON_SNIPER || btData->pEntity)
						{
							if (btData->pEntity)
							{
								CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();
								CObjectPool* pObjectPool = pNetGame->GetObjectPool();

								uint16_t PlayerID;
								uint16_t VehicleID;
								uint16_t ObjectID;

								if (pVehiclePool && pObjectPool)
								{
									PlayerID = pPlayerPool->FindRemotePlayerIDFromGtaPtr((PED_TYPE*)btData->pEntity);
									if (PlayerID == INVALID_PLAYER_ID)
									{
										VehicleID = pVehiclePool->FindIDFromGtaPtr((VEHICLE_TYPE*)btData->pEntity);
										if (VehicleID == INVALID_VEHICLE_ID)
										{
											ObjectID = pObjectPool->FindIDFromGtaPtr(btData->pEntity);
											if (ObjectID == INVALID_OBJECT_ID)
											{
												VECTOR vecOut;
												vecOut.X = 0.0f;
												vecOut.Y = 0.0f;
												vecOut.Z = 0.0f;

												if (btData->pEntity->mat)
												{
													ProjectMatrix(&vecOut, btData->pEntity->mat, &btData->vecOffset);
													btData->vecOffset.X = vecOut.X;
													btData->vecOffset.Y = vecOut.Y;
													btData->vecOffset.Z = vecOut.Z;
												}
												else
												{
													btData->vecOffset.X = btData->pEntity->mat->pos.X + btData->vecOffset.X;
													btData->vecOffset.Y = btData->pEntity->mat->pos.Y + btData->vecOffset.Y;
													btData->vecOffset.Z = btData->pEntity->mat->pos.Z + btData->vecOffset.Z;
												}
											}
											else
											{
												// object
												byteHitType = 3;
												InstanceID = ObjectID;
											}
										}
										else
										{
											// vehicle
											byteHitType = 2;
											InstanceID = VehicleID;
										}
									}
									else
									{
										// player
										byteHitType = 1;
										InstanceID = PlayerID;
									}
								}
							}

							bulletSyncData.vecOrigin.X = btData->vecOrigin.X;
							bulletSyncData.vecOrigin.Y = btData->vecOrigin.Y;
							bulletSyncData.vecOrigin.Z = btData->vecOrigin.Z;

							bulletSyncData.vecPos.X = btData->vecPos.X;
							bulletSyncData.vecPos.Y = btData->vecPos.Y;
							bulletSyncData.vecPos.Z = btData->vecPos.Z;

							bulletSyncData.vecOffset.X = btData->vecOffset.X;
							bulletSyncData.vecOffset.Y = btData->vecOffset.Y;
							bulletSyncData.vecOffset.Z = btData->vecOffset.Z;

							bulletSyncData.byteHitType = byteHitType;
							bulletSyncData.PlayerID = InstanceID;
							WEAPON_SLOT_TYPE* pSlot = GetCurrentWeaponSlot();
							if (pSlot) bulletSyncData.byteWeaponID = pSlot->dwType;
							else bulletSyncData.byteWeaponID = 0;
							//bulletSyncData.byteWeaponID = pPlayerPed->GetCurrentWeapon();

							RakNet::BitStream bsBullet;
							bsBullet.Write((char)ID_BULLET_SYNC);
							bsBullet.Write((char*)&bulletSyncData, sizeof(BULLET_SYNC_DATA));
							pNetGame->GetRakClient()->Send(&bsBullet, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0);
						}
					}
				}
			}
		}
	}
	else
	{
		m_bHaveBulletData = false;
		memset(&m_bulletData, 0, sizeof(BULLET_DATA));
	}
}

CAMERA_AIM * CPlayerPed::GetCurrentAim()
{
	return GameGetInternalAim();
}

void CPlayerPed::SetCurrentAim(CAMERA_AIM * pAim)
{
	GameStoreRemotePlayerAim(m_bytePlayerNumber, pAim);
}

uint16_t CPlayerPed::GetCameraMode()
{
	return GameGetLocalPlayerCameraMode();
}

void CPlayerPed::SetCameraMode(uint16_t byteCamMode)
{
	GameSetPlayerCameraMode(byteCamMode, m_bytePlayerNumber);
}

float CPlayerPed::GetCameraExtendedZoom()
{
	return GameGetLocalPlayerCameraExtZoom();
}

void CPlayerPed::ApplyCrouch()
{
	
	if (!m_pPed || !m_dwGTAId)
	{
		return;
	}
	if (!GamePool_Ped_GetAt(m_dwGTAId))
	{
		return;
	}

	if (!(m_pPed->dwStateFlags & 256))
	{
		if (!IsCrouching())
		{
			if (m_pPed->pPedIntelligence)
			{
				((int (*)(CPedIntelligence*, uint16_t))(SA_ADDR(0x44E0F4 + 1)))(m_pPed->pPedIntelligence, 0);
			}
		}
	}
}

void CPlayerPed::ResetCrouch()
{
	
	if (!m_pPed || !m_dwGTAId)
	{
		return;
	}
	if (!GamePool_Ped_GetAt(m_dwGTAId))
	{
		return;
	}
	m_pPed->dwStateFlags &= 0xFBFFFFFF;
}

bool CPlayerPed::IsCrouching()
{
	
	if (!m_pPed || !m_dwGTAId)
	{
		return false;
	}
	if (!GamePool_Ped_GetAt(m_dwGTAId))
	{
		return false;
	}
	return IS_CROUCHING(m_pPed);
}

void CPlayerPed::SetCameraExtendedZoom(float fZoom)
{
	GameSetPlayerCameraExtZoom(m_bytePlayerNumber, fZoom);
}

void CPlayerPed::SetDead()
{
	
	if (!m_dwGTAId || !m_pPed)
	{
		return;
	}
	if (!GamePool_Ped_GetAt(m_dwGTAId))
	{
		return;
	}
	
	MATRIX4X4 mat;
	GetMatrix(&mat);
	// will reset the tasks
	TeleportTo(mat.pos.X, mat.pos.Y, mat.pos.Z);
	m_pPed->fHealth = 0.0f;

	uint8_t old = *(uint8_t*)(g_libGTASA + 0x008E864C); // CWorld::PlayerInFocus - 0x008E864C
	*(uint8_t*)(SA_ADDR(0x8E864C)) = m_bytePlayerNumber;
	ScriptCommand(&kill_actor, m_dwGTAId);
	*(uint8_t*)(SA_ADDR(0x8E864C)) = 0;
}

// 0.3.7
bool CPlayerPed::IsInVehicle()
{
	if(!m_pPed) return false;

	if(IN_VEHICLE(m_pPed))
		return true;

	return false;
}
int GameGetWeaponModelIDFromWeaponID(int iWeaponID)
{
	switch (iWeaponID)
	{
	case WEAPON_BRASSKNUCKLE:
		return WEAPON_MODEL_BRASSKNUCKLE;

	case WEAPON_GOLFCLUB:
		return WEAPON_MODEL_GOLFCLUB;

	case WEAPON_NITESTICK:
		return WEAPON_MODEL_NITESTICK;

	case WEAPON_KNIFE:
		return WEAPON_MODEL_KNIFE;

	case WEAPON_BAT:
		return WEAPON_MODEL_BAT;

	case WEAPON_SHOVEL:
		return WEAPON_MODEL_SHOVEL;

	case WEAPON_POOLSTICK:
		return WEAPON_MODEL_POOLSTICK;

	case WEAPON_KATANA:
		return WEAPON_MODEL_KATANA;

	case WEAPON_CHAINSAW:
		return WEAPON_MODEL_CHAINSAW;

	case WEAPON_DILDO:
		return WEAPON_MODEL_DILDO;

	case WEAPON_DILDO2:
		return WEAPON_MODEL_DILDO2;

	case WEAPON_VIBRATOR:
		return WEAPON_MODEL_VIBRATOR;

	case WEAPON_VIBRATOR2:
		return WEAPON_MODEL_VIBRATOR2;

	case WEAPON_FLOWER:
		return WEAPON_MODEL_FLOWER;

	case WEAPON_CANE:
		return WEAPON_MODEL_CANE;

	case WEAPON_GRENADE:
		return WEAPON_MODEL_GRENADE;

	case WEAPON_TEARGAS:
		return WEAPON_MODEL_TEARGAS;

	case WEAPON_MOLTOV:
		return -1;

	case WEAPON_COLT45:
		return WEAPON_MODEL_COLT45;

	case WEAPON_SILENCED:
		return WEAPON_MODEL_SILENCED;

	case WEAPON_DEAGLE:
		return WEAPON_MODEL_DEAGLE;

	case WEAPON_SHOTGUN:
		return WEAPON_MODEL_SHOTGUN;

	case WEAPON_SAWEDOFF:
		return WEAPON_MODEL_SAWEDOFF;

	case WEAPON_SHOTGSPA:
		return WEAPON_MODEL_SHOTGSPA;

	case WEAPON_UZI:
		return WEAPON_MODEL_UZI;

	case WEAPON_MP5:
		return WEAPON_MODEL_MP5;

	case WEAPON_AK47:
		return WEAPON_MODEL_AK47;

	case WEAPON_M4:
		return WEAPON_MODEL_M4;

	case WEAPON_TEC9:
		return WEAPON_MODEL_TEC9;

	case WEAPON_RIFLE:
		return WEAPON_MODEL_RIFLE;

	case WEAPON_SNIPER:
		return WEAPON_MODEL_SNIPER;

	case WEAPON_ROCKETLAUNCHER:
		return WEAPON_MODEL_ROCKETLAUNCHER;

	case WEAPON_HEATSEEKER:
		return WEAPON_MODEL_HEATSEEKER;

	case WEAPON_FLAMETHROWER:
		return WEAPON_MODEL_FLAMETHROWER;

	case WEAPON_MINIGUN:
		return WEAPON_MODEL_MINIGUN;

	case WEAPON_SATCHEL:
		return WEAPON_MODEL_SATCHEL;

	case WEAPON_BOMB:
		return WEAPON_MODEL_BOMB;

	case WEAPON_SPRAYCAN:
		return WEAPON_MODEL_SPRAYCAN;

	case WEAPON_FIREEXTINGUISHER:
		return WEAPON_MODEL_FIREEXTINGUISHER;

	case WEAPON_CAMERA:
		return WEAPON_MODEL_CAMERA;

	case -1:
		return WEAPON_MODEL_NIGHTVISION;

	case -2:
		return WEAPON_MODEL_INFRARED;

	case WEAPON_PARACHUTE:
		return WEAPON_MODEL_PARACHUTE;

	}

	return -1;
}

void CPlayerPed::GiveWeapon(int iWeaponID, int iAmmo)
{
	if (!m_pPed || !m_dwGTAId)
	{
		return;
	}

	if (!GamePool_Ped_GetAt(m_dwGTAId))
	{
		return;
	}

	int iModelID = 0;
	iModelID = GameGetWeaponModelIDFromWeaponID(iWeaponID);
	
	if (iModelID == -1) return;

	if (!pGame->IsModelLoaded(iModelID))
	{
		pGame->RequestModel(iModelID);
		pGame->LoadRequestedModels();
		while (!pGame->IsModelLoaded(iModelID)) sleep(1);
	}

	((int(*)(uintptr_t, unsigned int, int))(SA_ADDR(0x43429C + 1)))((uintptr_t)m_pPed, iWeaponID, iAmmo); // CPed::GiveWeapon(thisptr, weapoid, ammo)
	((int(*)(uintptr_t, unsigned int))(SA_ADDR(0x434528 + 1)))((uintptr_t)m_pPed, iWeaponID);	// CPed::SetCurrentWeapon(thisptr, weapid)
}

void CPlayerPed::SetArmedWeapon(int iWeaponID)
{
	if (!m_pPed || !m_dwGTAId)
	{
		return;
	}

	if (!GamePool_Ped_GetAt(m_dwGTAId))
	{
		return;
	}

	((int(*)(uintptr_t, unsigned int))(SA_ADDR(0x434528 + 1)))((uintptr_t)m_pPed, iWeaponID);	// CPed::SetCurrentWeapon(thisptr, weapid)
}

void CPlayerPed::SetPlayerAimState()
{
	if (!m_pPed || !m_dwGTAId)
	{
		return;
	}

	if (!GamePool_Ped_GetAt(m_dwGTAId))
	{
		return;
	}

	uintptr_t ped = (uintptr_t)m_pPed;
	uint8_t old = *(uint8_t*)(SA_ADDR(0x8E864C)); // CWorld::PlayerInFocus - 0x008E864C
	*(uint8_t*)(SA_ADDR(0x8E864C)) = m_bytePlayerNumber;

	((uint32_t(*)(uintptr_t, int, int, int))(SA_ADDR(0x454A6C + 1)))(ped, 1, 1, 1); // CPlayerPed::ClearWeaponTarget
	*(uint8_t *)(*(uint32_t *)(ped + 1088) + 52) = *(uint8_t *)(*(uint32_t *)(ped + 1088) + 52) & 0xF7 | 8 * (1 & 1); // magic 

	*(uint8_t*)(SA_ADDR(0x8E864C)) = old;
}

void CPlayerPed::ApplyCommandTask(char* a2, int a4, int a5, int a6, VECTOR* a7, char a8, float a9, int a10, int a11, char a12)
{
	uint32_t dwPed = (uint32_t)m_pPed;
	if (!dwPed) return;
	// 00958484 - g_ikChainManager
	// 00463188 addr
	((int(*)(uintptr_t a1, char* a2, uint32_t a3, int a4, int a5, int a6, VECTOR* a7, char a8, float a9, int a10, int a11, char a12))(SA_ADDR(0x463188 + 1)))
		(SA_ADDR(0x958484), a2, dwPed, a4, a5, a6, a7, a8, a9, a10, a11, a12);

}

void CPlayerPed::ClearPlayerAimState()
{
	if (!m_pPed || !m_dwGTAId)
	{
		return;
	}

	if (!GamePool_Ped_GetAt(m_dwGTAId))
	{
		return;
	}

	uintptr_t ped = (uintptr_t)m_pPed;
	uint8_t old = *(uint8_t*)(SA_ADDR(0x8E864C));	// CWorld::PlayerInFocus - 0x008E864C
	*(uint8_t*)(SA_ADDR(0x8E864C)) = m_bytePlayerNumber;

	*(uint32_t *)(ped + 1432) = 0;	// unk
	((uint32_t(*)(uintptr_t, int, int, int))(SA_ADDR(0x454A6C + 1)))(ped, 0, 0, 0);	// CPlayerPed::ClearWeaponTarget
	*(uint8_t *)(*(uint32_t *)(ped + 1088) + 52) = *(uint8_t *)(*(uint32_t *)(ped + 1088) + 52) & 0xF7 | 8 * (0 & 1);	// magic...

	*(uint8_t*)(SA_ADDR(0x8E864C)) = old;
}

uint8_t CPlayerPed::GetCurrentWeapon()
{
	if (!m_pPed) return 0;
	if (GamePool_Ped_GetAt(m_dwGTAId) == 0) return 0;

	uint32_t dwRetVal;
	ScriptCommand(&get_actor_armed_weapon, m_dwGTAId, &dwRetVal);
	return (uint8_t)dwRetVal;
}

// 0.3.7
bool CPlayerPed::IsAPassenger()
{
	if(m_pPed->pVehicle && IN_VEHICLE(m_pPed))
	{
		VEHICLE_TYPE *pVehicle = (VEHICLE_TYPE *)m_pPed->pVehicle;

		if(	pVehicle->pDriver != m_pPed ||
			pVehicle->entity.nModelIndex == TRAIN_PASSENGER ||
			pVehicle->entity.nModelIndex == TRAIN_FREIGHT )
			return true;
	}

	return false;
}

// 0.3.7
VEHICLE_TYPE* CPlayerPed::GetGtaVehicle()
{
	return (VEHICLE_TYPE*)m_pPed->pVehicle;
}

// 0.3.7
void CPlayerPed::RemoveFromVehicleAndPutAt(float fX, float fY, float fZ)
{
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;
	if(m_pPed && IN_VEHICLE(m_pPed))
		ScriptCommand(&remove_actor_from_car_and_put_at, m_dwGTAId, fX, fY, fZ);
}

// 0.3.7
void CPlayerPed::SetInitialState()
{
	// Log("CPlayerPed::SetInitialState()");

	if(!m_pPed)
		return;

	// (( int (*)(PED_TYPE*, bool))(g_libGTASA + 0x458D1C + 1))(m_pPed, false);
	(( void (*)(PED_TYPE*))(SA_ADDR(0x458D1C + 1)))(m_pPed);

}

// 0.3.7
void CPlayerPed::SetHealth(float fHealth)
{
	if(!m_pPed) return;
	m_pPed->fHealth = fHealth;
}

// 0.3.7
float CPlayerPed::GetHealth()
{
	if(!m_pPed)
		return 0.0f;

	return m_pPed->fHealth;
}

// 0.3.7
void CPlayerPed::SetArmour(float fArmour)
{
	if(!m_pPed)
		return;

	m_pPed->fArmour = fArmour;
}

float CPlayerPed::GetArmour()
{
	if(!m_pPed) return 0.0f;
	return m_pPed->fArmour;
}

uint32_t m_dwLastUpdatePlayer = GetTickCount();
void CPlayerPed::SetInterior(uint8_t byteID)
{
	if (!m_pPed) return;
	if (!GamePool_Ped_GetAt(m_dwGTAId)) return;

	ScriptCommand(&select_interior, byteID);
	ScriptCommand(&link_actor_to_interior, m_dwGTAId, byteID);
	m_bPlayerInterior = byteID;
	m_dwLastUpdatePlayer = GetTickCount();

	//RemoveUnusedTextures();

	MATRIX4X4 mat;
	GetMatrix(&mat);
	ScriptCommand(&refresh_streaming_at, mat.pos.X, mat.pos.Y);
}

uint8_t CPlayerPed::GetInterior()
{
	return m_bPlayerInterior;
}

bool CPlayerPed::GetUpdatePlayer()
{
	bool UpdatePlayer = false;
	if (GetTickCount() - m_dwLastUpdatePlayer < 10000) UpdatePlayer = true;
	return UpdatePlayer;
}

void CPlayerPed::PutDirectlyInVehicle(int iVehicleID, int iSeat)
{
	if(!m_pPed) return;
	if(!GamePool_Vehicle_GetAt(iVehicleID)) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	/* ��������
	if(GetCurrentWeapon() == WEAPON_PARACHUTE) {
		SetArmedWeapon(0);
	}*/

	VEHICLE_TYPE *pVehicle = GamePool_Vehicle_GetAt(iVehicleID);

	if(pVehicle->fHealth == 0.0f) return;
	// check is cplaceable
	if (pVehicle->entity.vtable == SA_ADDR(0x5C7358)) return;
	// check seatid (��������)

	if(iSeat == 0)
	{
		if(pVehicle->pDriver && IN_VEHICLE(pVehicle->pDriver)) return;
		ScriptCommand(&put_actor_in_car, m_dwGTAId, iVehicleID);
	}
	else
	{
		iSeat--;
		ScriptCommand(&put_actor_in_car2, m_dwGTAId, iVehicleID, iSeat);
	}

	if(m_pPed == GamePool_FindPlayerPed() && IN_VEHICLE(m_pPed))
		pGame->GetCamera()->SetBehindPlayer();

	if(pNetGame)
	{
		// �������� (��������)
	}
}

void CPlayerPed::EnterVehicle(int iVehicleID, bool bPassenger)
{
	if(!m_pPed) return;
	VEHICLE_TYPE* ThisVehicleType;
	if((ThisVehicleType = GamePool_Vehicle_GetAt(iVehicleID)) == 0) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	if(bPassenger)
	{
		if(ThisVehicleType->entity.nModelIndex == TRAIN_PASSENGER &&
			(m_pPed == GamePool_FindPlayerPed()))
		{
			ScriptCommand(&put_actor_in_car2, m_dwGTAId, iVehicleID, -1);
		}
		else
		{
			ScriptCommand(&send_actor_to_car_passenger,m_dwGTAId,iVehicleID, 3000, -1);
		}
	}
	else
		ScriptCommand(&send_actor_to_car_driverseat, m_dwGTAId, iVehicleID, 3000);
}

// 0.3.7
void CPlayerPed::ExitCurrentVehicle()
{
	if(!m_pPed) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	VEHICLE_TYPE* ThisVehicleType = 0;

	if(IN_VEHICLE(m_pPed))
	{
		if(GamePool_Vehicle_GetIndex((VEHICLE_TYPE*)m_pPed->pVehicle))
		{
			int index = GamePool_Vehicle_GetIndex((VEHICLE_TYPE*)m_pPed->pVehicle);
			ThisVehicleType = GamePool_Vehicle_GetAt(index);
			if(ThisVehicleType)
			{
				if(	ThisVehicleType->entity.nModelIndex != TRAIN_PASSENGER &&
					ThisVehicleType->entity.nModelIndex != TRAIN_PASSENGER_LOCO)
				{
					ScriptCommand(&make_actor_leave_car, m_dwGTAId, GetCurrentVehicleID());
				}
			}
		}
	}
}

// 0.3.7
int CPlayerPed::GetCurrentVehicleID()
{
	if(!m_pPed) return 0;

	VEHICLE_TYPE *pVehicle = (VEHICLE_TYPE *)m_pPed->pVehicle;
	return GamePool_Vehicle_GetIndex(pVehicle);
}

int CPlayerPed::GetVehicleSeatID()
{
	VEHICLE_TYPE *pVehicle;

	if( GetActionTrigger() == ACTION_INCAR && (pVehicle = (VEHICLE_TYPE *)m_pPed->pVehicle) != 0 ) 
	{
		if(pVehicle->pDriver == m_pPed) return 0;
		if(pVehicle->pPassengers[0] == m_pPed) return 1;
		if(pVehicle->pPassengers[1] == m_pPed) return 2;
		if(pVehicle->pPassengers[2] == m_pPed) return 3;
		if(pVehicle->pPassengers[3] == m_pPed) return 4;
		if(pVehicle->pPassengers[4] == m_pPed) return 5;
		if(pVehicle->pPassengers[5] == m_pPed) return 6;
		if(pVehicle->pPassengers[6] == m_pPed) return 7;
	}

	return (-1);
}
void CPlayerPed::SetLockTogglePlayerControllable(bool bToggle){
    m_bLockTogglePlayerControllable = bToggle;
}
// 0.3.7
void CPlayerPed::TogglePlayerControllable(bool bToggle)
{

	MATRIX4X4 mat;

	if (!GamePool_Ped_GetAt(m_dwGTAId)) return;

    if(m_bLockTogglePlayerControllable) return;

	if (!bToggle){
		ScriptCommand(&toggle_player_controllable, m_bytePlayerNumber, 0);
		ScriptCommand(&lock_actor, m_dwGTAId, 1);
		// Turn off invulnerability
		*(uint8_t*)((uintptr_t)m_pPed + 66) &= 0xBF;
		*(uint8_t*)((uintptr_t)m_pPed + 66) &= 0xFB;
		*(uint8_t*)((uintptr_t)m_pPed + 66) &= 0xF7;
		*(uint8_t*)((uintptr_t)m_pPed + 66) &= 0x7F;
		*(uint8_t*)((uintptr_t)m_pPed + 66) &= 0xEF;
		*(uint8_t*)((uintptr_t)m_pPed + 66) &= 0xDF;
		*(uint8_t*)(*(uint32_t*)((uintptr_t)m_pPed + 1088) + 52) |= 0x10;
	}else{
        ScriptCommand(&toggle_player_controllable, m_bytePlayerNumber, 1);
        ScriptCommand(&lock_actor, m_dwGTAId, 0);
        if (!IsInVehicle())
        {
            GetMatrix(&mat);
            TeleportTo(mat.pos.X, mat.pos.Y, mat.pos.Z);
        }
	}
}

extern void (*CClothes__RebuildPlayer)(PED_TYPE* cped, bool r1);
extern char* tessstt;

// 0.3.7
void CPlayerPed::SetModelIndex(unsigned int uiModel)
{
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;
	if(!IsPedModel(uiModel))
		uiModel = 0;

	if(m_pPed)
	{

        WriteMemory(SA_ADDR(0x3F1030), (uintptr_t)"\x70\x47", 2);
        DestroyFollowPedTask();
        CEntity::SetModelIndex(uiModel);

		// reset the Ped Audio Attributes
		(( void (*)(uintptr_t, uintptr_t))(SA_ADDR(0x34B2A8 + 1)))(((uintptr_t)m_pPed+660), (uintptr_t)m_pPed);
	}
}

bool CPlayerPed::IsAnimationPlaying(char* szAnimName)
{
	if (!m_pPed) return false;
	if (!GamePool_Ped_GetAt(m_dwGTAId)) return false;
	if (!szAnimName || !strlen(szAnimName)) return false;

	if (ScriptCommand(&is_char_playing_anim, m_dwGTAId, szAnimName)) {
		return true;
	}

	return false;
}

void CPlayerPed::ClearAllTasks()
{
	if (!GamePool_Ped_GetAt(m_dwGTAId) || !m_pPed) {
		return;
	}

	ScriptCommand(&clear_char_tasks, m_dwGTAId);
}
void CPlayerPed::SetPlayerSpecialAction(int iAction)
{
	if (iAction == -1)
	{
		return;
	}
	if (iAction == 0)
	{
		ClearAllTasks();
		iAction = -1;
		return;
	}

	m_iSpecialAction = iAction;
}
void CPlayerPed::ProcessSpecialAction()
{
	if (m_iSpecialAction == SPECIAL_ACTION_CARRY)
	{
		if (IsInVehicle())
		{
			SetPlayerSpecialAction(-1);
			return;
		}

		if (!IsAnimationPlaying(OBFUSCATE("CRRY_PRTIAL"))) {
			ApplyAnimation(OBFUSCATE("CRRY_PRTIAL"), OBFUSCATE("CARRY"), 4.1, 0, 0, 0, 1, 1);
		}
	}
}
// ��������
void CPlayerPed::DestroyFollowPedTask()
{

}

// ��������
void CPlayerPed::ClearAllWeapons()
{
	uintptr_t dwPedPtr = (uintptr_t)m_pPed;
	uint8_t old = *(uint8_t*)(SA_ADDR(0x8E864C));	// CWorld::PlayerInFocus - 0x008E864C
	*(uint8_t*)(SA_ADDR(0x8E864C)) = m_bytePlayerNumber;

	((uint32_t(*)(uintptr_t, int, int, int))(SA_ADDR(0x4345AC + 1)))(dwPedPtr, 1, 1, 1); // CPed::ClearWeapons(void)

	*(uint8_t*)(SA_ADDR(0x8E864C)) = old;
}

// ��������
void CPlayerPed::ResetDamageEntity()
{

}

// 0.3.7
void CPlayerPed::RestartIfWastedAt(VECTOR *vecRestart, float fRotation)
{	
	ScriptCommand(&restart_if_wasted_at, vecRestart->X, vecRestart->Y, vecRestart->Z, fRotation, 0);
}

// 0.3.7
void CPlayerPed::ForceTargetRotation(float fRotation)
{
	if(!m_pPed) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	m_pPed->fRotation1 = DegToRad(fRotation);
	m_pPed->fRotation2 = DegToRad(fRotation);

	ScriptCommand(&set_actor_z_angle,m_dwGTAId,fRotation);
}

void CPlayerPed::SetRotation(float fRotation)
{
	if(!m_pPed) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	m_pPed->fRotation1 = DegToRad(fRotation);
	m_pPed->fRotation2 = DegToRad(fRotation);
}

// 0.3.7
uint8_t CPlayerPed::GetActionTrigger()
{
	return (uint8_t)m_pPed->dwAction;
}

void CPlayerPed::SetActionTrigger(uint8_t action)
{
	m_pPed->dwAction = (uint32_t)action;
}

void CPlayerPed::AttachObject(ATTACHED_OBJECT_INFO* pInfo, int iSlot)
{
	if (m_aAttachedObjects[iSlot].bState)
	{
		DeattachObject(iSlot);
	}
	memcpy((void*)& m_aAttachedObjects[iSlot], (const void*)pInfo, sizeof(ATTACHED_OBJECT_INFO));
	MATRIX4X4 matPos;
	GetMatrix(&matPos);
	VECTOR vecRot{ 0.0f, 0.0f, 0.0f };
	m_aAttachedObjects[iSlot].pObject = new CObject(pInfo->dwModelId, matPos.pos.X, matPos.pos.Y, matPos.pos.Z, vecRot, 200.0f);
	m_aAttachedObjects[iSlot].pColoredObject = nullptr;
	//m_aAttachedObjects[iSlot].pObject->PaintMaterial(pInfo->dwColor[0], pInfo->dwColor[1]);
	*(uint32_t*)((uintptr_t)m_aAttachedObjects[iSlot].pObject->m_pEntity + 28) &= 0xFFFFFFFE; // disable collision
	m_aAttachedObjects[iSlot].bState = true;

	if(pInfo->dwColor[0] || pInfo->dwColor[1]){
		m_aAttachedObjects[iSlot].pColoredObject = new CObject(pInfo->dwModelId, matPos.pos.X, matPos.pos.Y, matPos.pos.Z, vecRot, 200.0f);
        *(uint32_t*)((uintptr_t)m_aAttachedObjects[iSlot].pColoredObject->m_pEntity + 28) &= 0xFFFFFFFE; // disable collision

        //m_aAttachedObjects[iSlot].pObject->SetColorAlpha(pInfo->dwColor[0], pInfo->dwColor[1]);
        //m_aAttachedObjects[iSlot].pColoredObject->SetColorAlpha(pInfo->dwColor[0], pInfo->dwColor[1]);

		CRGBA argb1;
		uint32_t dwReversed1 = __builtin_bswap32(pInfo->dwColor[0] ? pInfo->dwColor[0] : 0xFFFFFFFF);
		argb1.Set(dwReversed1);
		CRGBA argb2;
		uint32_t dwReversed2 = __builtin_bswap32(pInfo->dwColor[1] ? pInfo->dwColor[1] : 0xFFFFFFFF);
		argb2.Set(dwReversed2);
		m_aAttachedObjects[iSlot].pObject->SetColorAlpha(argb1.A, argb2.A);
		m_aAttachedObjects[iSlot].pColoredObject->SetColor(pInfo->dwColor[0], pInfo->dwColor[1]);
	}
}

void CPlayerPed::SetAttachOffset(int iSlot, VECTOR pos, VECTOR rot)
{
	if (iSlot < 0 || iSlot >= MAX_ATTACHED_OBJECTS)
	{
		return;
	}
	m_aAttachedObjects[iSlot].vecOffset = pos;
	m_aAttachedObjects[iSlot].vecRotation = rot;
}

void CPlayerPed::DeattachObject(int iSlot)
{
	if (m_aAttachedObjects[iSlot].bState)
	{
		if(m_aAttachedObjects[iSlot].pColoredObject){
		    pNetGame->GetObjectPool()->RemoveColoredObject(m_aAttachedObjects[iSlot].pColoredObject);
			delete m_aAttachedObjects[iSlot].pColoredObject;
		}

		delete m_aAttachedObjects[iSlot].pObject;
	}
	m_aAttachedObjects[iSlot].bState = false;
}

bool CPlayerPed::IsHasAttach()
{
	for (int i = 0; i < MAX_ATTACHED_OBJECTS; i++)
	{
		if (m_aAttachedObjects[i].bState) return true;
	}
	return false;
}

void CPlayerPed::FlushAttach()
{
	for (int i = 0; i < MAX_ATTACHED_OBJECTS; i++)
	{
		DeattachObject(i);
	}
}

MATRIX4X4* RwMatrixMultiplyByVector(VECTOR* out, MATRIX4X4* a2, VECTOR* in)
{
	MATRIX4X4* result;
	VECTOR* v4;

	result = a2;
	v4 = in;
	out->X = a2->at.X * in->Z + a2->up.X * in->Y + a2->right.X * in->X + a2->pos.X;
	out->Y = result->at.Y * v4->Z + result->up.Y * v4->Y + result->right.Y * v4->X + result->pos.Y;
	out->Z = result->at.Z * v4->Z + result->up.Z * v4->Y + a2->right.Z * in->X + result->pos.Z;
	return result;
}

MATRIX4X4* RwMatrixDivisionByVector(VECTOR* out, MATRIX4X4* a2, MATRIX4X4* in)
{
    MATRIX4X4* result;
    VECTOR* v4;

    result = a2;
    VECTOR vecOne{1.0f, 1.0f, 1.0f};
    VECTOR vecOut;
    RwMatrixMultiplyByVector(&vecOut, a2, &vecOne);

    out->X = (in->pos.X - a2->pos.X)/(a2->at.X + a2->up.X + a2->right.X);
    out->Y = (in->pos.Y - a2->pos.Y)/(a2->at.Y + a2->up.Y + a2->right.Y);
    out->Z = (in->pos.Z - a2->pos.Z)/(a2->at.Z + a2->up.Z + a2->right.Z);
    return result;
}

void RwMatrixRotate(MATRIX4X4* pMat, VECTOR* axis, float angle)
{
	((int(*)(MATRIX4X4*, VECTOR*, float, int))(SA_ADDR(0x1B9118 + 1)))(pMat, axis, angle, 1);
}



void CPlayerPed::ProcessAttach()
{
	if (!m_pPed) return;
	if (m_pPed->entity.vtable == (SA_ADDR(0x5C7358))) return;

    //int clothDesc = *(int*)(((uintptr_t)m_pPed)+0x440);
    //if(!clothDesc) return;
    //*(int*)(*(uintptr_t*)(clothDesc+4)) = 0;
    //*(int*)(*(uintptr_t*)(clothDesc+4) + 4) = 0;
    //*(int*)(*(uintptr_t*)(clothDesc+4) + 8) = 0;

	((int(*)(PED_TYPE*))(SA_ADDR(0x391968 + 1)))(m_pPed); // UpdateRPHAnim

	if (IsAdded())
	{
		ProcessHeadMatrix();
	}
	for (int i = 0; i < MAX_ATTACHED_OBJECTS; i++)
	{
		if (!m_aAttachedObjects[i].bState) continue;
		CObject* pObject = m_aAttachedObjects[i].pObject;
		CObject* pColoredObject = nullptr;
		if(m_aAttachedObjects[i].pColoredObject){
			pColoredObject = m_aAttachedObjects[i].pColoredObject;
		}
		if (IsAdded())
		{
			RpHAnimHierarchy* hierarchy = ((RpHAnimHierarchy * (*)(uintptr_t*))(SA_ADDR(0x559338 + 1)))((uintptr_t*)m_pPed->entity.m_RwObject); // GetAnimHierarchyFromSkinClump
			int iID;
			uint32_t bone = m_aAttachedObjects[i].dwBone;

			if (hierarchy)
				iID = ((int(*)(RpHAnimHierarchy*, int))(SA_ADDR(0x19A448 + 1)))(hierarchy, bone); // RpHAnimIDGetIndex
			else continue;
			if (iID == -1)
				continue;

			pObject->RemovePhysical();
			if(pColoredObject) {
				pColoredObject->RemovePhysical();
			}

			MATRIX4X4 outMat;
			memcpy(&outMat, &hierarchy->pMatrixArray[iID], sizeof(MATRIX4X4));

			VECTOR vecOut;
			RwMatrixMultiplyByVector(&vecOut, &outMat, &m_aAttachedObjects[i].vecOffset);

			outMat.pos.X = vecOut.X;
			outMat.pos.Y = vecOut.Y;
			outMat.pos.Z = vecOut.Z;

			VECTOR axis = { 1.0f, 0.0f, 0.0f };
			if (m_aAttachedObjects[i].vecRotation.X != 0.0f)
			{
				RwMatrixRotate(&outMat, &axis, m_aAttachedObjects[i].vecRotation.X);
			}
			axis = { 0.0f, 1.0f, 0.0f };
			if (m_aAttachedObjects[i].vecRotation.Y != 0.0f)
			{
				RwMatrixRotate(&outMat, &axis, m_aAttachedObjects[i].vecRotation.Y);
			}
			axis = { 0.0f, 0.0f, 1.0f };
			if (m_aAttachedObjects[i].vecRotation.Z != 0.0f)
			{
				RwMatrixRotate(&outMat, &axis, m_aAttachedObjects[i].vecRotation.Z);
			}

			RwMatrixScale(&outMat, &m_aAttachedObjects[i].vecScale);

			*(uint32_t*)((uintptr_t)pObject->m_pEntity + 28) &= 0xFFFFFFFE; // disable collision
			if(pColoredObject) {
				*(uint32_t * )((uintptr_t) pColoredObject->m_pEntity +
							   28) &= 0xFFFFFFFE; // disable collision
			}

			if (outMat.pos.X >= 10000.0f || outMat.pos.X <= -10000.0f ||
				outMat.pos.Y >= 10000.0f || outMat.pos.Y <= -10000.0f ||
				outMat.pos.Z >= 10000.0f || outMat.pos.Z <= -10000.0f)
			{
				continue;
			}

			pObject->SetMatrix(outMat); // copy to CMatrix
			if(pColoredObject) {
				pColoredObject->SetMatrix(outMat); // copy to CMatrix
			}
			if(pColoredObject){
				if (pColoredObject->m_pEntity->m_RwObject)
				{
					if (pColoredObject->m_pEntity->mat)
					{
						uintptr_t v8 = *(uintptr_t*)(pColoredObject->m_pEntity->m_RwObject + 4) + 16;
						if (v8)
						{
							((int(*)(MATRIX4X4*, uintptr_t))(SA_ADDR(0x3E862C + 1)))(pColoredObject->m_pEntity->mat, v8); // CEntity::UpdateRwFrame
						}
					}
				}
			}

			if (pObject->m_pEntity->m_RwObject)
			{
				if (pObject->m_pEntity->mat)
				{
					uintptr_t v8 = *(uintptr_t*)(pObject->m_pEntity->m_RwObject + 4) + 16;
					if (v8)
					{
						((int(*)(MATRIX4X4*, uintptr_t))(SA_ADDR(0x3E862C + 1)))(pObject->m_pEntity->mat, v8); // CEntity::UpdateRwFrame
					}
				}
			}
			//Log("pos %f %f %f", outMat.pos.X, outMat.pos.Y, outMat.pos.Z);

			if(pColoredObject) {
				((int (*)(ENTITY_TYPE *)) (SA_ADDR(0x39194C + 1)))(
						pColoredObject->m_pEntity); // CEntity::UpdateRwFrame
				pColoredObject->AddPhysical();
			}
			((int(*)(ENTITY_TYPE*))(SA_ADDR(0x39194C + 1)))(pObject->m_pEntity); // CEntity::UpdateRwFrame
			pObject->AddPhysical();
		}else {
			if(pColoredObject) {
				pColoredObject->TeleportTo(0.0f, 0.0f, 0.0f);
			}
			pObject->TeleportTo(0.0f, 0.0f, 0.0f);
		}
	}
}

void CPlayerPed::ProcessHeadMatrix()
{
	RpHAnimHierarchy* hierarchy = ((RpHAnimHierarchy * (*)(uintptr_t*))(SA_ADDR(0x559338 + 1)))((uintptr_t*)m_pPed->entity.m_RwObject); // GetAnimHierarchyFromSkinClump
	int iID;
	uint32_t bone = 4;
	if (hierarchy)
		iID = ((int(*)(RpHAnimHierarchy*, int))(SA_ADDR(0x19A448 + 1)))(hierarchy, bone); // RpHAnimIDGetIndex
	else return;

	if (iID == -1)
		return;

	memcpy(&m_HeadBoneMatrix, &hierarchy->pMatrixArray[iID], sizeof(MATRIX4X4));
}

bool IsTaskRunNamedOrSlideToCoord(void* pTask)
{
	
	uintptr_t dwVTable = *(uintptr_t*)(pTask);
	if (dwVTable == (SA_ADDR(0x5CB910)) || dwVTable == (SA_ADDR(0x5C8408))) // CTaskSimpleSlideToCoord CTaskSimpleRunNamedAnim
	{
		return true;
	}
	return false;
}

void* GetSubTaskFromTask(void* pTask)
{
	
	uintptr_t pVTableTask = *((uintptr_t*)pTask);
	return ((void* (*)(void*))(*(void**)(pVTableTask + 12)))(pTask);
}

bool CPlayerPed::IsPlayingAnim(int idx)
{
	
	if (!m_pPed || !m_dwGTAId || (idx == 0) )
	{
		return 0;
	}
	if (!GamePool_Ped_GetAt(m_dwGTAId))
	{
		return 0;
	}
	if (!m_pPed->entity.m_RwObject)
	{
		return 0;
	}

	const char* pAnim = GetAnimByIdx(idx - 1);
	if (!pAnim)
	{
		return false;
	}
	const char* pNameAnim = strchr(pAnim, ':') + 1;

	uintptr_t blendAssoc = ((uintptr_t(*)(uintptr_t clump, const char* szName))(SA_ADDR(0x340594 + 1)))
		(m_pPed->entity.m_RwObject, pNameAnim);	// RpAnimBlendClumpGetAssociation

	if (blendAssoc)
	{
		return true;
	}
	else
	{
		return false;
	}
}

int CPlayerPed::GetCurrentAnimationIndex(float& blendData)
{
	
	blendData = 4.0f;

	if (!m_pPed || !m_dwGTAId)
	{
		return 0;
	}

	if (!GamePool_Ped_GetAt(m_dwGTAId))
	{
		return 0;
	}

	if (!m_pPed->entity.m_RwObject)
	{
		return 0;
	}
	sizeof(PED_TYPE);
	CPedIntelligence* pIntelligence = m_pPed->pPedIntelligence;

	if (pIntelligence)
	{
		void* pTask = pIntelligence->m_TaskMgr.m_aPrimaryTasks[TASK_PRIMARY_PRIMARY];

		if (pTask)
		{
			while (!IsTaskRunNamedOrSlideToCoord(pTask))
			{
				pTask = GetSubTaskFromTask(pTask);
				if (!pTask)
				{
					return 0;
				}
			}

			const char* szName = (const char*)((uintptr_t)pTask + 13);
			const char* szGroupName = (const char*)((uintptr_t)pTask + 37);

			std::string szStr = std::string(szGroupName);
			szStr += ":";
			szStr += szName;

			int idx = GetAnimIdxByName(szStr.c_str());
			if (idx == -1)
			{
				return 0;
			}
			else
			{
				return idx + 1;
			}
		}
	}
	return 0;
}

void CPlayerPed::PlayAnimByIdx(int idx, float BlendData)
{
	
	if (!idx)
	{
		MATRIX4X4 mat;
		GetMatrix(&mat);
		TeleportTo(mat.pos.X, mat.pos.Y, mat.pos.Z);
		return;
	}
	std::string szAnim;
	std::string szBlock;

	char pszAnim[40];
	char pszBlock[40];

	memset(&pszAnim[0], 0, 40);
	memset(&pszBlock[0], 0, 40);

	bool bTest = false;
	const char* pBegin = GetAnimByIdx(idx - 1);
	if (!pBegin)
	{
		return;
	}
	while (*pBegin)
	{
		if (*pBegin == ':')
		{
			pBegin++;
			bTest = true;
			continue;
		}
		if (!bTest)
		{
			szBlock += *pBegin;
		}
		if (bTest)
		{
			szAnim += *pBegin;
		}
		pBegin++;
	}

	strcpy(&pszAnim[0], szAnim.c_str());
	strcpy(&pszBlock[0], szBlock.c_str());
	ApplyAnimation(&pszAnim[0], &pszBlock[0], BlendData, 0, 1, 1, 0, 0);
}

bool IsBlendAssocGroupLoaded(int iGroup)
{
	

	uintptr_t* pBlendAssocGroup = *(uintptr_t * *)(SA_ADDR(0x890350)); // CAnimManager::ms_aAnimAssocGroups
	uintptr_t blendAssoc = (uintptr_t)pBlendAssocGroup;
	blendAssoc += (iGroup * 20);
	pBlendAssocGroup = (uintptr_t*)blendAssoc;
	return *(pBlendAssocGroup) != NULL;
}

void CPlayerPed::SetMoveAnim(int iAnimGroup)
{
	Log(OBFUSCATE("SetMoveAnim %d"), iAnimGroup);
	if (iAnimGroup == 0)
		return;

	// Find which anim block to load
	const char* strBlockName = nullptr;
	switch (iAnimGroup)
	{
	case 55:
	case 58:
	case 61:
	case 64:
	case 67:
		strBlockName = OBFUSCATE("fat");
		break;

	case 56:
	case 59:
	case 62:
	case 65:
	case 68:
		strBlockName = OBFUSCATE("muscular");
		break;

	case 138:
		strBlockName = OBFUSCATE("skate");
		break;

	default:
		strBlockName = OBFUSCATE("ped");
		break;
	}

	if (!strBlockName)
		return;

	if (!m_dwGTAId || !m_pPed)
		return;

	if (!GamePool_Ped_GetAt(m_dwGTAId))
		return;

	if (!IsBlendAssocGroupLoaded(iAnimGroup))
	{
		Log(OBFUSCATE("Animgrp %d not loaded"), iAnimGroup);
		uintptr_t pAnimBlock = ((uintptr_t(*)(const char*))(SA_ADDR(0x33DB7C + 1)))(strBlockName);
		if (!pAnimBlock)
			return;

		uint8_t bLoaded = *((uint8_t*)pAnimBlock + 16);
		if (!bLoaded)
		{
			uintptr_t animBlocks = (uintptr_t)(SA_ADDR(0x89035C));
			uintptr_t idx = (pAnimBlock - animBlocks) / 32;

			uintptr_t modelId = idx + 25575;
			Log(OBFUSCATE("trying to load modelid %u"), modelId);
			if (!pGame->IsModelLoaded(modelId))
			{
				pGame->RequestModel(modelId);
				pGame->LoadRequestedModels();
				int tries = 0;
				while (!pGame->IsModelLoaded(modelId) && tries <= 10)
				{
					usleep(10);
					tries++;
				}
			}
		}
		if (!IsBlendAssocGroupLoaded(iAnimGroup))
		{
			Log(OBFUSCATE("not loaded %d"), iAnimGroup);
			return;
		}
		Log(OBFUSCATE("animgrp %d loaded"), iAnimGroup);
	}

	uintptr_t ped = (uintptr_t)m_pPed;
	*(int*)(ped + 1244) = iAnimGroup;

	((void(*)(PED_TYPE* thiz))(SA_ADDR(0x4544F4 + 1)))(m_pPed); // ReApplyMoveAnims
}


// 0.3.7
bool CPlayerPed::IsDead()
{
	
	if(!m_pPed) return true;
	if(m_pPed->fHealth > 0.0f) return false;
	return true;
}

void CPlayerPed::SetMoney(int iAmount)
{
	ScriptCommand(&set_actor_money, m_dwGTAId, 0);
	ScriptCommand(&set_actor_money, m_dwGTAId, iAmount);
}

// 0.3.7
void CPlayerPed::ShowMarker(uint32_t iMarkerColorID)
{
	if(m_dwArrow) HideMarker();
	ScriptCommand(&create_arrow_above_actor, m_dwGTAId, &m_dwArrow);
	ScriptCommand(&set_marker_color, m_dwArrow, iMarkerColorID);
	ScriptCommand(&show_on_radar2, m_dwArrow, 2);
}

// 0.3.7
void CPlayerPed::HideMarker()
{
	if(m_dwArrow) ScriptCommand(&disable_marker, m_dwArrow);
	m_dwArrow = 0;
}

// 0.3.7
#include "..//chatwindow.h"
extern CChatWindow* pChatWindow;
void CPlayerPed::SetFightingStyle(int iStyle)
{
	
	if(!m_pPed || !m_dwGTAId) return;
	if (!GamePool_Ped_GetAt(m_dwGTAId))
	{
		return;
	}
	//pChatWindow->AddDebugMessage("set fighting style %d", iStyle);
	ScriptCommand( &set_fighting_style, m_dwGTAId, iStyle, 6 );
}

// 0.3.7
void CPlayerPed::ApplyAnimation( char *szAnimName, char *szAnimFile, float fT,
								 int opt1, int opt2, int opt3, int opt4, int iUnk )
{
	
	int iWaitAnimLoad = 0;

	if(!m_pPed) return;
	if(!GamePool_Ped_GetAt(m_dwGTAId)) return;

	if(!strcasecmp(szAnimFile,OBFUSCATE("SEX"))) return;

	if(!pGame->IsAnimationLoaded(szAnimFile))
	{
		pGame->RequestAnimation(szAnimFile);
		while(!pGame->IsAnimationLoaded(szAnimFile))
		{
			usleep(1000);
			iWaitAnimLoad++;
			if(iWaitAnimLoad > 15) return;
		}
	}

	ScriptCommand(&apply_animation, m_dwGTAId, szAnimName, szAnimFile, fT, opt1, opt2, opt3, opt4, iUnk);
}

unsigned char CPlayerPed::FindDeathReasonAndResponsiblePlayer(PLAYERID *nPlayer)
{
	if(!IsValidGamePed(m_pPed) || !GamePool_Ped_GetAt(m_dwGTAId)) {
		return 0;
	}

	unsigned char byteDeathReason;
	PLAYERID bytePlayerIDWhoKilled;
	CVehiclePool *pVehiclePool;
	CPlayerPool *pPlayerPool;

	// grab the vehicle/player pool now anyway, even though we may not need it.
	if(pNetGame) {
		pVehiclePool = pNetGame->GetVehiclePool();
		pPlayerPool = pNetGame->GetPlayerPool();
	}
	else { // just leave if there's no netgame.
		*nPlayer = INVALID_PLAYER_ID;
		return WEAPON_UNKNOWN;
	}

	if(m_pPed) 
	{
		byteDeathReason = (unsigned char)m_pPed->dwWeaponUsed;
		if(byteDeathReason < WEAPON_CAMERA || byteDeathReason == WEAPON_HELIBLADES || byteDeathReason == WEAPON_EXPLOSION) { // It's a weapon of some sort.

			if(m_pPed->pdwDamageEntity) { // check for a player pointer.
				
				bytePlayerIDWhoKilled = pPlayerPool->
					FindRemotePlayerIDFromGtaPtr((PED_TYPE *)m_pPed->pdwDamageEntity);

				if(bytePlayerIDWhoKilled != INVALID_PLAYER_ID) {
					// killed by another player with a weapon, this is all easy.
					*nPlayer = bytePlayerIDWhoKilled;
					return byteDeathReason;
				} else { // could be a vehicle
					if(pVehiclePool->FindIDFromGtaPtr((VEHICLE_TYPE *)m_pPed->pdwDamageEntity) != INVALID_VEHICLE_ID) {
						VEHICLE_TYPE *pGtaVehicle = (VEHICLE_TYPE *)m_pPed->pdwDamageEntity;
						bytePlayerIDWhoKilled = pPlayerPool->
							FindRemotePlayerIDFromGtaPtr((PED_TYPE *)pGtaVehicle->pDriver);
												
						if(bytePlayerIDWhoKilled != INVALID_PLAYER_ID) {
							*nPlayer = bytePlayerIDWhoKilled;
							return byteDeathReason;
						}
					}
				}
			}
			//else { // weapon was used but who_killed is 0 (?)
			*nPlayer = INVALID_PLAYER_ID;
			return WEAPON_UNKNOWN;
			//}
		}
		else if(byteDeathReason == WEAPON_DROWN) {
			*nPlayer = INVALID_PLAYER_ID;
			return WEAPON_DROWN;
		}
		else if(byteDeathReason == WEAPON_VEHICLE) {

			if(m_pPed->pdwDamageEntity) {
				// now, if we can find the vehicle
				// we can probably derive the responsible player.
				// Look in the vehicle pool for this vehicle.
				if(pVehiclePool->FindIDFromGtaPtr((VEHICLE_TYPE *)m_pPed->pdwDamageEntity) != INVALID_VEHICLE_ID)
				{
					VEHICLE_TYPE *pGtaVehicle = (VEHICLE_TYPE *)m_pPed->pdwDamageEntity;

					bytePlayerIDWhoKilled = pPlayerPool->
						FindRemotePlayerIDFromGtaPtr((PED_TYPE *)pGtaVehicle->pDriver);
											
					if(bytePlayerIDWhoKilled != INVALID_PLAYER_ID) {
						*nPlayer = bytePlayerIDWhoKilled;
						return WEAPON_VEHICLE;
					}
				}									
			}
		}
		else if(byteDeathReason == WEAPON_COLLISION) {

			if(m_pPed->pdwDamageEntity) {
				if(pVehiclePool->FindIDFromGtaPtr((VEHICLE_TYPE *)m_pPed->pdwDamageEntity) != INVALID_VEHICLE_ID)
				{
					VEHICLE_TYPE *pGtaVehicle = (VEHICLE_TYPE *)m_pPed->pdwDamageEntity;
										
					bytePlayerIDWhoKilled = pPlayerPool->
						FindRemotePlayerIDFromGtaPtr((PED_TYPE *)pGtaVehicle->pDriver);
						
					if(bytePlayerIDWhoKilled != INVALID_PLAYER_ID) {
						*nPlayer = bytePlayerIDWhoKilled;
						return WEAPON_COLLISION;
					}
				}
				else {
					*nPlayer = INVALID_PLAYER_ID;
					return WEAPON_COLLISION;
				}
			}
		}
	}

	// Unhandled death type.
	*nPlayer = INVALID_PLAYER_ID;
	return WEAPON_UNKNOWN;
}

// 0.3.7
void CPlayerPed::GetBonePosition(int iBoneID, VECTOR* vecOut)
{
	if(!m_pPed) return;
	if(m_pEntity->vtable == SA_ADDR(0x5C7358)) return;

	(( void (*)(PED_TYPE*, VECTOR*, int, int))(SA_ADDR(0x436590 + 1)))(m_pPed, vecOut, iBoneID, 0);
}

void CPlayerPed::GetPosition(VECTOR *outPos){
	float x,y,z;
	ScriptCommand(&get_player_coordinates, m_dwGTAId, &x, &y, &z);
	outPos->X = x;
	outPos->Y = y;
	outPos->Z = z;
}


//-----------------------------------------------------------

VEHICLE_TYPE* CPlayerPed::GetGtaContactVehicle() {
    return (VEHICLE_TYPE*)m_pPed->pContactVehicle;
}

ENTITY_TYPE* CPlayerPed::GetGtaContactEntity()
{
    if (!m_pPed->pContactEntity) return NULL;
    return (ENTITY_TYPE*)m_pPed->pContactEntity;
}

void CPlayerPed::ClumpUpdateAnimations(float step, int flag)
{
	if (m_pPed)
	{
		uintptr_t pRwObj = m_pEntity->m_RwObject;
		if (pRwObj)
		{
			((void (*)(uintptr_t, float, int))(SA_ADDR(0x33D6E4 + 1)))(pRwObj, step, flag);
		}
	}
}

uint8_t CPlayerPed::GetExtendedKeys()
{
	uint8_t result = 0;
	if (LocalPlayerKeys.bKeys[ePadKeys::KEY_YES])
	{
		LocalPlayerKeys.bKeys[ePadKeys::KEY_YES] = false;
		result = 1;
	}
	if (LocalPlayerKeys.bKeys[ePadKeys::KEY_NO])
	{
		LocalPlayerKeys.bKeys[ePadKeys::KEY_NO] = false;
		result = 2;
	}
	if (LocalPlayerKeys.bKeys[ePadKeys::KEY_CTRL_BACK])
	{
		LocalPlayerKeys.bKeys[ePadKeys::KEY_CTRL_BACK] = false;
		result = 3;
	}

	return result;
}

// ��������
uint16_t CPlayerPed::GetKeys(uint16_t *lrAnalog, uint16_t *udAnalog)
{
	*lrAnalog = LocalPlayerKeys.wKeyLR;
	*udAnalog = LocalPlayerKeys.wKeyUD;
	uint16_t wRet = 0;

	// KEY_ANALOG_RIGHT
	if (LocalPlayerKeys.bKeys[ePadKeys::KEY_ANALOG_RIGHT]) wRet |= 1;
	wRet <<= 1;
	// KEY_ANALOG_LEFT
	if (LocalPlayerKeys.bKeys[ePadKeys::KEY_ANALOG_LEFT]) wRet |= 1;
	wRet <<= 1;
	// KEY_ANALOG_DOWN
	if (LocalPlayerKeys.bKeys[ePadKeys::KEY_ANALOG_DOWN]) wRet |= 1;
	wRet <<= 1;
	// KEY_ANALOG_UP
	if (LocalPlayerKeys.bKeys[ePadKeys::KEY_ANALOG_UP]) wRet |= 1;
	wRet <<= 1;
	// KEY_WALK
	if (LocalPlayerKeys.bKeys[ePadKeys::KEY_WALK]) wRet |= 1;
	wRet <<= 1;
	// KEY_SUBMISSION
	if (LocalPlayerKeys.bKeys[ePadKeys::KEY_SUBMISSION]) wRet |= 1;
	wRet <<= 1;
	// KEY_LOOK_LEFT
	if (LocalPlayerKeys.bKeys[ePadKeys::KEY_LOOK_LEFT]) wRet |= 1;
	wRet <<= 1;

	if (GetCameraMode() == 0x35)
	{
		LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = 1;
	}
	else
	{
		LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = 0;
	}

	// KEY_HANDBRAKE
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE]/*true*/) wRet |= 1;
	wRet <<= 1;
	// KEY_LOOK_RIGHT
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_LOOK_RIGHT]) wRet |= 1;
	wRet <<= 1;
	// KEY_JUMP
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_JUMP]) wRet |= 1;
	wRet <<= 1;
	// KEY_SECONDARY_ATTACK
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_SECONDARY_ATTACK]) wRet |= 1;
	wRet <<= 1;
	// KEY_SPRINT
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_SPRINT]) wRet |= 1;
	wRet <<= 1;
	// KEY_FIRE
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_FIRE]) wRet |= 1;
	wRet <<= 1;
	// KEY_CROUCH
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_CROUCH]) wRet |= 1;
	wRet <<= 1;
	// KEY_ACTION
	if(LocalPlayerKeys.bKeys[ePadKeys::KEY_ACTION]) wRet |= 1;

	memset(LocalPlayerKeys.bKeys, 0, ePadKeys::SIZE);

	return wRet;
}

WEAPON_SLOT_TYPE * CPlayerPed::GetCurrentWeaponSlot()
{
	if (m_pPed) 
	{
		return &m_pPed->WeaponSlots[m_pPed->byteCurWeaponSlot];
	}
	return NULL;
}


void CPlayerPed::SetKeys(uint16_t wKeys, uint16_t lrAnalog, uint16_t udAnalog)
{
	PAD_KEYS *pad = &RemotePlayerKeys[m_bytePlayerNumber];

	// LEFT/RIGHT
	pad->wKeyLR = lrAnalog;
	// UP/DOWN
	pad->wKeyUD = udAnalog;

	// KEY_ACTION
	pad->bKeys[ePadKeys::KEY_ACTION] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_CROUCH
	pad->bKeys[ePadKeys::KEY_CROUCH] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_FIRE
	pad->bKeys[ePadKeys::KEY_FIRE] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_SPRINT
	pad->bKeys[ePadKeys::KEY_SPRINT] = (wKeys & 1);
	if(!pad->bKeys[ePadKeys::KEY_SPRINT]) pad->bIgnoreSwim = false;
	wKeys >>= 1;
	// KEY_SECONDARY_ATTACK
	pad->bKeys[ePadKeys::KEY_SECONDARY_ATTACK] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_JUMP
	pad->bKeys[ePadKeys::KEY_JUMP] = (wKeys & 1);
	if(!pad->bKeys[ePadKeys::KEY_JUMP]) pad->bIgnoreJump = false;
	wKeys >>= 1;
	// KEY_LOOK_RIGHT
	pad->bKeys[ePadKeys::KEY_LOOK_RIGHT] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_HANDBRAKE
	pad->bKeys[ePadKeys::KEY_HANDBRAKE] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_LOOK_LEFT
	pad->bKeys[ePadKeys::KEY_LOOK_LEFT] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_SUBMISSION
	pad->bKeys[ePadKeys::KEY_SUBMISSION] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_WALK
	pad->bKeys[ePadKeys::KEY_WALK] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_ANALOG_UP
	pad->bKeys[ePadKeys::KEY_ANALOG_UP] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_ANALOG_DOWN
	pad->bKeys[ePadKeys::KEY_ANALOG_DOWN] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_ANALOG_LEFT
	pad->bKeys[ePadKeys::KEY_ANALOG_LEFT] = (wKeys & 1);
	wKeys >>= 1;
	// KEY_ANALOG_RIGHT
	pad->bKeys[ePadKeys::KEY_ANALOG_RIGHT] = (wKeys & 1);

	return;
}

void CPlayerPed::SetAimZ(float fAimZ)
{
	if (!m_pPed)
	{
		return;
	}
	*(float*)(*((uintptr_t*)m_pPed + 272) + 84) = fAimZ;
	//m_pPed + 272 - dwPlayerInfo
}

float CPlayerPed::GetAimZ()
{
	if (!m_pPed)
	{
		return 0.0f;
	}
	return *(float*)(*((uintptr_t*)m_pPed + 272) + 84);
}


void CPlayerPed::SetFloatStat(unsigned char ucStat, float fLevel)
{
	if (!m_pData) return;
	if (ucStat > MAX_FLOAT_STATS) return;
	m_pData->GetStatsTypesFloat()[ucStat] = fLevel;
}