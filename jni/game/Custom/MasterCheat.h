#pragma once

#include "main.h";
#include "../quaternion.h"

class MasterCheat{
public:
    MasterCheat();
    ~MasterCheat();

    void Render();
    void DrawLinesWH();

    void ToggleShowLines(bool show);
    void swTest(bool act);


    bool m_bShowLinesWH;
    bool m_showTestText;
    VECTOR m_testVector;
    CQuaternion m_testQu;
    int m_testVeh;

    RwTexture* testTexture1;
    RwTexture* testTexture2;
    int count;

    uintptr_t tessttt1;
    uintptr_t tessttt2;

private:
    float GetScaleX(float precent);
    float GetScaleY(float precent);
};

extern MasterCheat *g_pMasterCheat;