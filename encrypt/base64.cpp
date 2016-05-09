#define UTIL_CORE
#include <stdlib.h>
#include "base64.h"

static const char codemap[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const unsigned char bitmap[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0, 0, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0, 0, 0, 0, 0, 0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

unsigned int ext_base64_encode(char *output, const void *input, unsigned int length)
{
	unsigned int quo = length / 3;
	unsigned int res = length - quo * 3;
	if (output != NULL)
	{
		for (unsigned int i = 0; i < quo; ++i)
		{
			int stm = 0;
			for (int j = 0; j < 3; ++j)
			{
				stm = (stm << 8) + ((const unsigned char *)input)[i * 3 + j];
			}
			for (int j = 0; j < 4; ++j)
			{
				output[i * 4 + 3 - j] = codemap[stm & 0x3f];
				stm >>= 6;
			}
		}
		if (res == 1)
		{
			int stm = ((const unsigned char *)input)[quo * 3] << 4;
			for (int j = 0; j < 2; ++j)
			{
				output[quo * 4 + 1 - j] = codemap[stm & 0x3f];
				stm >>= 6;
			}
			for (int j = 2; j < 4; ++j)
			{
				output[quo * 4 + j] = '=';
			}
		}
		else if (res == 2)
		{
			int stm = 0;
			for (int j = 0; j < 2; ++j)
			{
				stm = (stm << 8) + ((const unsigned char *)input)[quo * 3 + j];
			}
			stm <<= 2;
			for (int j = 0; j < 3; ++j)
			{
				output[quo * 4 + 2 - j] = codemap[stm & 0x3f];
				stm >>= 6;
			}
			output[quo * 4 + 3] = '=';
		}
	}
	return quo * 4 + (res == 0 ? 0 : 4);
}

unsigned int ext_base64_decode(void *output, const char *input, unsigned int length)
{
	unsigned int quo = length / 4;
	unsigned int res = 0;
	while (input[length] != '=')
	{
		++res;
		--length;
	}
	if (output != NULL)
	{
		for (unsigned int i = 0; i < quo - 1; ++i)
		{
			int stm = 0;
			for (int j = 0; j < 4; ++j)
			{
				stm = (stm << 6) + bitmap[(unsigned char)input[i * 4 + j]];
			}
			for (int j = 0; j < 3; ++j)
			{
				((unsigned char *)output)[i * 3 + 2 - j] = stm & 0xff;
				stm >>= 8;
			}
		}
		int stm = 0;
		for (size_t j = 0; j < 4; ++j)
		{
			stm = (stm << 6) + bitmap[(unsigned char)input[(quo - 1) * 4 + j]];
		}
		for (size_t j = 0; j < 3; ++j)
		{
			if (j >= res)
			{
				((unsigned char *)output)[(quo - 1) * 3 + 2 - j] = stm & 0xff;
			}
			stm >>= 8;
		}
	}
	return quo * 3 - res;
}
