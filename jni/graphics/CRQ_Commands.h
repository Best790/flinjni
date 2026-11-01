// -- -- -- -- -- --
// Created by Loony-dev � 2021
// VK: https://vk.com/loonydev
// -- -- -- -

#pragma once

#include "../main.h"

class CRQ_Commands {
public:
    static void rqVertexBufferCreate_HOOK(unsigned int **result);
	static void rqVertexBufferSelect_HOOK(unsigned int **result);
	static void rqVertexBufferDelete_HOOK(int result);
};