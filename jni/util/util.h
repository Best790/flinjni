#pragma once

std::string curlRequestGet(const char* url, struct curl_slist *headers);
uintptr_t FindLibrary(const char* library);
void cp1251_to_utf8(char *out, const char *in, unsigned int len = 0);

void AND_OpenLink(const char* szLink);