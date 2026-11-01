// -- -- -- -- -- --
// Created by Loony-dev � 2021
// VK: https://vk.com/loonydev
// -- -- -- -

#include "CRQ_Commands.h"
#include "../util/armhook.h";
#include <GLES2/gl2.h>

int* savedMalloc = 0;
int size = 0;

void CRQ_Commands::rqVertexBufferCreate_HOOK(unsigned int **result)
{


    /*if(*(void**)(SA_ADDR(0x61722C)) != 0x0){
        free(*(void**)(SA_ADDR(0x61722C)));
    }*/
    //*(int**)(SA_ADDR(0x617234)) = (int*)((int)*result + 4); // cur
    //*(void**)(SA_ADDR(0x61722C)) = *(void**)(SA_ADDR(0x617234)); // safe
	//*(int*)(SA_ADDR(0x617230)) = *(int*)(result); // size
	/*Log("sizeee: %d", *(int*)(SA_ADDR(0x617230)));

    //indexing
    if(*(void**)(SA_ADDR(0x617224)) != 0x0){
        free(*(void**)(SA_ADDR(0x61722C)));
    }
    *(void**)(SA_ADDR(0x617224)) = malloc(14022400); // cur
    *(void**)(SA_ADDR(0x61721C)) = *(void**)(SA_ADDR(0x617224)); // safe
    *(int*)(SA_ADDR(0x617220)) = 14022400; // size*/

}

void CRQ_Commands::rqVertexBufferSelect_HOOK(unsigned int **result)
{
	unsigned int buffer = *(*result)++;

	if (!buffer)
		return glBindBuffer(0x8892, 0);

	glBindBuffer(0x8892, buffer + 8);
	*(uint32_t*)(SA_ADDR(0x617234)) = 0;
}

void CRQ_Commands::rqVertexBufferDelete_HOOK(int result)
{
	/*int v2;
	bool v3;

	int buffer = **(uint32_t**)result;
	if (buffer)
	{
		*(uint32_t *)result += 4;
		glDeleteBuffers(1, reinterpret_cast<const GLuint *>(buffer + 8));

		result = *(uint32_t *)buffer;
		*(uint32_t *)(buffer + 8) = 0;
		v3 = result == 0;

		if (result)
		{
			v3 = v2 == 0;
		}

		if (!v3)
			result = v2(buffer);
	}*/
}
