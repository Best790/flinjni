// -- -- -- -- -- --
// Created by Loony-dev © 2021
// VK: https://vk.com/loonydev
// -- -- -- -

#pragma once

#include <vector>
#include <string>
#include <sstream>

struct BUFFERED_BIND_BINDER
{
	int type;
	char buff[300];
};

class CBinder {
public:
	static void Initialise();

	static void Render();

	static void Toggle();

	static void NewBindInputHandler(const char* str);

	static bool IsRender();

private:
	static bool m_bIsRender;

	static int m_iValuesCount;
	static int m_iSelectedBindID;

	static std::vector<const char *> m_BinderValues;

	static char szNewBindInputBuffer[512];

	static DataStructures::SingleProducerConsumer<BUFFERED_BIND_BINDER> bufferedChat;
};