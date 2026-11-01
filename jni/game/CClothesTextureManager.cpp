/*
#include "../main.h"
#include "game.h";

char* ped_parts_name[MAX_PED_PARTS] = {
        "torso", "head", "legs", "feet"
};

CClothesTextureManager::CClothesTextureManager(){

}

CClothesTextureManager::~CClothesTextureManager(){

}

void CClothesTextureManager::LoadAllTextures() {
    for (int i = 0; i < MAX_PED_PARTS; ++i) {
        for (int j = 0; j < COUNT_PED_TEXTURES; ++j) {
            char bufName[0xFF];
            sprintf(bufName, "%s_%d", ped_parts_name[i], j);
            Log("lloooooadddd %s", bufName);
            RwTexture* texture = (RwTexture*) LoadTextureFromDB("player", bufName);
            texture = ((RwTexture*(*)(RwTexture*))(SA_ADDR(0x3F4340+1)))(texture);
            m_listTexturesPed[i][j] = texture;
        }
    }
}

RwTexture* CClothesTextureManager::GetTexture(int part, int pedId){
    return m_listTexturesPed[part][pedId];
}

char* CClothesTextureManager::GetPartName(int part){
    return ped_parts_name[part];
}
 */