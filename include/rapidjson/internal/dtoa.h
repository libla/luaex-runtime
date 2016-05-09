// Copyright (C) 2011 Milo Yip
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// This is a C++ header-only implementation of Grisu2 algorithm from the publication:
// Loitsch, Florian. "Printing floating-point numbers quickly and accurately with
// integers." ACM Sigplan Notices 45.6 (2010): 233-243.

#ifndef RAPIDJSON_DTOA_
#define RAPIDJSON_DTOA_

#if defined(_MSC_VER)
#include <intrin.h>
#if defined(_M_AMD64)
#pragma intrinsic(_BitScanReverse64)
#endif
#endif

#include <float.h>
#include <stdio.h>
#include "itoa.h" // GetDigitsLut()

RAPIDJSON_NAMESPACE_BEGIN
namespace internal {

#ifdef __GNUC__
RAPIDJSON_DIAG_PUSH
RAPIDJSON_DIAG_OFF(effc++)
#endif

inline char* WriteExponent(int K, char* buffer) {
    if (K < 0) {
        *buffer++ = '-';
        K = -K;
    }

    if (K >= 100) {
        *buffer++ = static_cast<char>('0' + static_cast<char>(K / 100));
        K %= 100;
        const char* d = GetDigitsLut() + K * 2;
        *buffer++ = d[0];
        *buffer++ = d[1];
    }
    else if (K >= 10) {
        const char* d = GetDigitsLut() + K * 2;
        *buffer++ = d[0];
        *buffer++ = d[1];
    }
    else
        *buffer++ = static_cast<char>('0' + static_cast<char>(K));

    return buffer;
}

inline char* Prettify(char* buffer, int length, int k) {
    const int kk = length + k;  // 10^(kk-1) <= v < 10^kk

    if (length <= kk && kk <= 21) {
        // 1234e7 -> 12340000000
        for (int i = length; i < kk; i++)
            buffer[i] = '0';
        buffer[kk] = '.';
        buffer[kk + 1] = '0';
        return &buffer[kk + 2];
    }
    else if (0 < kk && kk <= 21) {
        // 1234e-2 -> 12.34
        std::memmove(&buffer[kk + 1], &buffer[kk], length - kk);
        buffer[kk] = '.';
        return &buffer[length + 1];
    }
    else if (-6 < kk && kk <= 0) {
        // 1234e-6 -> 0.001234
        const int offset = 2 - kk;
        std::memmove(&buffer[offset], &buffer[0], length);
        buffer[0] = '0';
        buffer[1] = '.';
        for (int i = 2; i < offset; i++)
            buffer[i] = '0';
        return &buffer[length + offset];
    }
    else if (length == 1) {
        // 1e30
        buffer[1] = 'e';
        return WriteExponent(kk - 1, &buffer[2]);
    }
    else {
        // 1234e30 -> 1.234e33
        std::memmove(&buffer[2], &buffer[1], length - 1);
        buffer[1] = '.';
        buffer[length + 1] = 'e';
        return WriteExponent(kk - 1, &buffer[0 + length + 2]);
    }
}

inline char* dtoa(double value, char* buffer) {
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '.';
        buffer[2] = '0';
        return &buffer[3];
    }
    else {
        if (value < 0) {
            *buffer++ = '-';
            value = -value;
        }
		char mantissa_buffer[64];
		sprintf(mantissa_buffer, "%.*e", DBL_DIG, value);
        int length = 0, K = 0;
		const char *pdot = NULL, *last = NULL;
		const char *pos = mantissa_buffer;
		for (; *pos != 0 && *pos != 'e'; ++pos)
		{
			if (*pos == '.')
			{
				pdot = pos;
				last = pos;
			}
			else
			{
				if (*pos >= '1' && *pos <= '9')
				{
					last = pos;
				}
			}
		}
		if (last != NULL)
		{
			for (const char *p = mantissa_buffer; p <= last; ++p)
			{
				if (*p != '.')
				{
					buffer[length++] = *p;
				}
			}
		}
		if (*pos == 'e')
		{
			++pos;
			int minus = 1;
			for (; *pos != 0; ++pos)
			{
				if (*pos == '-')
				{
					minus = -1;
				}
				else if (*pos >= '0' && *pos <= '9')
				{
					K = K * 10 + *pos - '0';
				}
			}
			K *= minus;
		}
		if (pdot != NULL)
		{
			K -= last - pdot;
		}
        return Prettify(buffer, length, K);
    }
}

#ifdef __GNUC__
RAPIDJSON_DIAG_POP
#endif

} // namespace internal
RAPIDJSON_NAMESPACE_END

#endif // RAPIDJSON_DTOA_
