#include "MasterCheat.h"
#include "gui/gui.h"
#include "main.h"
#include "game/game.h"
#include "net/netgame.h"
#include "game/RW/RenderWare.h"

extern CNetGame *pNetGame;
extern CGUI *pGUI;

MasterCheat::MasterCheat(){
    m_bShowLinesWH = false;
    Log("supported temajm");

    testTexture1 = 0;
    testTexture2 = 0;
    tessttt1 = 0;
    tessttt2 = 0;
    count = 0;

    m_showTestText = false;
    m_testVeh = -1;
}

void MasterCheat::swTest(bool act){

}

void MasterCheat::Render() {
    this->DrawLinesWH();
    /*
    if(m_showTestText){
        ImVec2 a = ImVec2(GetScaleX(0.0f), GetScaleY(0.7f));
        char buff[0xFF];

        float rx, ry, rz;

        rx = asin(2*m_testQu.y*m_testQu.z-2*m_testQu.x*m_testQu.w);
        ry = -atan2f(m_testQu.x*m_testQu.z+m_testQu.y*m_testQu.w, 0.5f-m_testQu.x*m_testQu.x-m_testQu.y*m_testQu.y);

        rz = (-atan2f(m_testQu.x*m_testQu.y+m_testQu.z*m_testQu.w, 0.5f-m_testQu.x*m_testQu.x-m_testQu.z*m_testQu.z) + 3.14159265) * 180/3.14159265;
        sprintf(buff, "w: %f (%f) x: %f (%f) y: %f (%f) z: %f (%f)", ((CQuaternion)m_testQu).w, 0, m_testQu.x, rx, m_testQu.y, ry, m_testQu.z, rz);

        ImGui::GetOverlayDrawList()->AddText(pGUI->GetFont(), 35.0f, a, 0xFFFF0000, buff);
    }else{
        ImVec2 a = ImVec2(GetScaleX(0.0f), GetScaleY(0.7f));
        ImGui::GetOverlayDrawList()->AddText(pGUI->GetFont(), 35.0f, a, 0xFFFF0000, "None");
    }


    if(testTexture1 != 0 && testTexture2 != 0){
        ImVec2 a = ImVec2(0, 0);
        ImVec2 b = ImVec2(testTexture1->raster->width,testTexture1->raster->height);
        ImGui::GetOverlayDrawList()->AddImage((ImTextureID)testTexture1->raster,a, b);
        a = ImVec2(testTexture1->raster->width + 50, testTexture1->raster->height);
        b = ImVec2(a.x+testTexture2->raster->width,a.y+testTexture2->raster->height);
        ImGui::GetOverlayDrawList()->AddImage((ImTextureID)testTexture2->raster,a, b);

    }

    for(int i = 0; i < 6250*3; i++){
        ImVec2 basePos = ImVec2(pGUI->ScaleX(10.0f), pGUI->ScaleY(600.0f));
        basePos.y += pGUI->GetFontSize() * 1.1f;
        ImVec2 basePosB = ImVec2(pGUI->ScaleX(50.0f), pGUI->ScaleY(650.0f));
        ImColor co(0.0f, 0.0f, 0.0f, 0.4f);
        ImGui::GetBackgroundDrawList()->AddRectFilled(basePos, basePosB, co);
    }*/
}

float MasterCheat::GetScaleX(float precent){
    ImGuiIO& io = ImGui::GetIO();
    return io.DisplaySize.x * precent;
}

float MasterCheat::GetScaleY(float precent){
    ImGuiIO& io = ImGui::GetIO();
    return io.DisplaySize.y * precent;
}

void MasterCheat::DrawLinesWH() {
    if(!m_bShowLinesWH){
        return;
    }

    VECTOR VecPos;
    int dwHitEntity;

    if(pNetGame) {
        CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
        for (PLAYERID playerId = 0; playerId < MAX_PLAYERS; playerId++) {
            if (pPlayerPool->GetSlotState(playerId) == true) {
                CRemotePlayer *pPlayer = pPlayerPool->GetAt(playerId);
                if(pPlayer && pPlayer->IsActive()){
                    CPlayerPed* pPlayerPed = pPlayer->GetPlayerPed();

                    if (!pPlayerPed){
                        continue;
                    }

                    if (!pPlayerPed->IsAdded()) continue;
                    VecPos.X = 0.0f;
                    VecPos.Y = 0.0f;
                    VecPos.Z = 0.0f;
                    pPlayerPed->GetBonePosition(8, &VecPos);

                    CAMERA_AIM *pCam = GameGetInternalAim();

                    dwHitEntity = ScriptCommand(&get_line_of_sight,
                                                VecPos.X, VecPos.Y, VecPos.Z,
                                                pCam->pos1x, pCam->pos1y, pCam->pos1z,
                                                1, 0, 0, 1, 0);

                    if(dwHitEntity){

                        VECTOR Out;

                        // CSprite::CalcScreenCoors
                        (( void (*)(VECTOR*, VECTOR*, float*, float*, bool, bool))(SA_ADDR(0x54EEC0 + 1)))(&VecPos, &Out, 0, 0, 0, 0);

                        ImGuiIO& io = ImGui::GetIO();
                        if (Out.Z < 1.0f){
                            continue;
                        }

                        if(Out.X >= GetScaleX(1.0f) || Out.X <= 0 || Out.Y >= GetScaleY(1.0f) || Out.Y <= 0){
                            int outScreenLineX = 0;
                            int outScreenLineY = 0;

                            int offsetCircleX = 0;
                            int offsetCircleY = 0;
                            if(Out.X <= 0){
                                outScreenLineX = 0;
                                offsetCircleX = 5.0f;
                            }else if(Out.X >= GetScaleX(1.0f)){
                                outScreenLineX = GetScaleX(1.0f);
                                offsetCircleX = -5.0f;
                            }else{
                                outScreenLineX = Out.X;
                                offsetCircleX = 0.0f;
                            }
                            if(Out.Y >= GetScaleY(1.0f)){
                                outScreenLineY = GetScaleY(1.0f);
                                offsetCircleY = -5.0f;
                            }else if(Out.Y <= 0){
                                outScreenLineY = 0;
                                offsetCircleY = 5.0f;
                            }else{
                                outScreenLineY = Out.Y;
                                offsetCircleY = 0.0f;
                            }

                                                    ImGui::GetOverlayDrawList()->AddLine(ImVec2(GetScaleX(0.5f), GetScaleY(0.8f)), ImVec2(outScreenLineX, outScreenLineY), 0xFFFF7043, 6.0f);

                            ImGui::GetOverlayDrawList()->AddCircleFilled(ImVec2(outScreenLineX + offsetCircleX, outScreenLineY + offsetCircleY), 6.0f, 0xFFFF7043);
                            ImGui::GetOverlayDrawList()->AddCircleFilled(ImVec2(outScreenLineX + offsetCircleX, outScreenLineY + offsetCircleY), 4.5f, 0xFFFFFFFF);
                            continue;
                        }else{
                             ImGui::GetOverlayDrawList()->AddLine(ImVec2(GetScaleX(0.5f), GetScaleY(0.8f)), ImVec2(Out.X, Out.Y), 0xFFFF7043, 6.0f);

                        }

                        int listSkeletBoneIds[] = {
                                ePedBones::BONE_HEAD2,
                                ePedBones::BONE_HEAD1,
                                ePedBones::BONE_HEAD,
								ePedBones::BONE_UPPERTORSO,
								ePedBones::BONE_LEFTUPPERTORSO,
                                ePedBones::BONE_LEFTSHOULDER,
                                ePedBones::BONE_LEFTELBOW,
                                ePedBones::BONE_LEFTWRIST,
                                ePedBones::BONE_LEFTHAND,
                                ePedBones::BONE_LEFTTHUMB,
								ePedBones::BONE_LEFTHAND,
								ePedBones::BONE_LEFTWRIST,
								ePedBones::BONE_LEFTELBOW,
								ePedBones::BONE_LEFTSHOULDER,
								ePedBones::BONE_LEFTUPPERTORSO,
								ePedBones::BONE_UPPERTORSO,
								ePedBones::BONE_RIGHTUPPERTORSO,
								ePedBones::BONE_RIGHTSHOULDER,
								ePedBones::BONE_RIGHTELBOW,
								ePedBones::BONE_RIGHTWRIST,
								ePedBones::BONE_RIGHTHAND,
								ePedBones::BONE_RIGHTTHUMB,
								ePedBones::BONE_RIGHTHAND,
								ePedBones::BONE_RIGHTWRIST,
								ePedBones::BONE_RIGHTELBOW,
								ePedBones::BONE_RIGHTSHOULDER,
								ePedBones::BONE_RIGHTUPPERTORSO,
								ePedBones::BONE_UPPERTORSO,
								ePedBones::BONE_NECK,
								ePedBones::BONE_PELVIS1,
                                ePedBones::BONE_LEFTHIP,
                                ePedBones::BONE_LEFTKNEE,
                                ePedBones::BONE_LEFTANKLE,
                                ePedBones::BONE_LEFTFOOT,
                                ePedBones::BONE_LEFTANKLE,
                                ePedBones::BONE_LEFTKNEE,
                                ePedBones::BONE_LEFTHIP,
                                ePedBones::BONE_PELVIS1,
                                ePedBones::BONE_RIGHTHIP,
                                ePedBones::BONE_RIGHTKNEE,
                                ePedBones::BONE_RIGHTANKLE,
                                ePedBones::BONE_RIGHTFOOT,
                        };

                        VECTOR lastBonePos;
                        for (int i = 0; i < sizeof(listSkeletBoneIds)/sizeof(int); ++i) {
                            VecPos.X = 0.0f;
                            VecPos.Y = 0.0f;
                            VecPos.Z = 0.0f;
                            pPlayerPed->GetBonePosition(listSkeletBoneIds[i], &VecPos);

                            (( void (*)(VECTOR*, VECTOR*, float*, float*, bool, bool))(SA_ADDR(0x54EEC0 + 1)))(&VecPos, &Out, 0, 0, 0, 0);

                            if(i == 0){
                                lastBonePos.X = Out.X;
                                lastBonePos.Y = Out.Y;
                                lastBonePos.Z = Out.Z;
                            }

                            ImGui::GetOverlayDrawList()->AddLine(ImVec2(lastBonePos.X, lastBonePos.Y), ImVec2(Out.X, Out.Y), 0xFF4370FF, 5.0f);
                            ImGui::GetOverlayDrawList()->AddCircleFilled(ImVec2(Out.X, Out.Y), 5.0f, 0xFFFF7043);
                            ImGui::GetOverlayDrawList()->AddCircleFilled(ImVec2(Out.X, Out.Y), 2.5f, 0xFFFFFFFF);

                            lastBonePos.X = Out.X;
                            lastBonePos.Y = Out.Y;
                            lastBonePos.Z = Out.Z;
                        }
                    }
                }
            }
        }
    }

}

void MasterCheat::ToggleShowLines(bool show) {
    m_bShowLinesWH = show;
}

MasterCheat *g_pMasterCheat = nullptr;