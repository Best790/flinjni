#pragma once
#include "../game/game.h"

class CCrossHair : private CSprite2d
{
private:
    bool m_bNeedFix;
public:
	CCrossHair(/* args */);
	~CCrossHair();

	void ChangeAim(const char* szName);
	void Render();
	
	bool m_UsedCrossHair;

	    bool IsNeededFix() { return m_bNeedFix; }
};


