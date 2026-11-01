#pragma once

#define INVALID_OBJECT_ID	0xFFF9
#define MAX_OBJECTS			1000

class CObjectPool
{
private:
	bool m_bObjectSlotState[MAX_OBJECTS];
	CObject *m_pObjects[MAX_OBJECTS];
	std::map<ENTITY_TYPE*, CObject*> m_mapColoredObjectPool;

public:
	CObjectPool();
	~CObjectPool();

	bool New(uint16_t ObjectID, int iModel, VECTOR vecPos, VECTOR vecRot, float fDrawDistance = 0);
	bool Delete(uint16_t ObjectID);

	bool GetSlotState(uint16_t ObjectID)
	{
		if(ObjectID >= MAX_OBJECTS) return false;
		return m_bObjectSlotState[ObjectID];
	};

	CObject *GetAt(uint16_t ObjectID)
	{
		if (ObjectID >= MAX_OBJECTS || ObjectID < 0)
		{
			return nullptr;
		}
		if(!m_bObjectSlotState[ObjectID]) return nullptr;
		return m_pObjects[ObjectID];
	};

	uint16_t FindIDFromGtaPtr(ENTITY_TYPE *pGtaObject);

	CObject *GetObjectFromGtaPtr(ENTITY_TYPE *pGtaObject);
	void AddColoredObject(CObject* pObject);
	void RemoveColoredObject(CObject* pObject);
	CObject* GetColoredObjectFromGtaPtr(ENTITY_TYPE* pGtaObject);

	void ResetColoredObject();
	void Process();
};