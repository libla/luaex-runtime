#define UTIL_CORE
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include "rc4.h"
#include "xxhash.h"
#include "md5.h"

#define N 624
#define M 397
#define twist(u, v) \
	(((((u) & 0x80000000) | ((v) & 0x7fffffff))  >>  1) ^ ((v) & 0x01 ? 0x9908b0df : 0x00))

namespace external
{
	class rc4key
	{
		static const int bestbit = 12;
		static const size_t bestlevel = 1 << bestbit;
	public:
		std::vector<unsigned int *> twices;

		struct
		{
			size_t left;
			unsigned int *next;
			unsigned int gen[N];
		} state;

		rc4key(const char *key, unsigned int len)
		{
			initgen(XXH32(key, (int)len, 0x62df340e));
		}
		~rc4key()
		{
			for (size_t i = 0, j = twices.size(); i < j; ++i)
			{
				unsigned int *sec = twices[i];
				free(sec);
			}
		}
		void encode(void *output, const void *input, size_t len)
		{
			unsigned char *output_ptr = (unsigned char *)output;
			const unsigned char *input_ptr = (const unsigned char *)input;
			size_t index = (len + sizeof(unsigned int) - 1) / sizeof(unsigned int);
			enlarge(std::min<size_t>(index, (1 << (bestbit * 2))));
			size_t i1, i2;
			i1 = i2 = 0;
			unsigned char lastbyte = 0;
			for (size_t i = 0; i < index; ++i)
			{
				unsigned int u = twices[i2][i1];
				if (++i1 == bestlevel)
				{
					i1 = 0;
					if (++i2 == bestlevel)
					{
						i2 = 0;
					}
				}
				size_t r = len - i * sizeof(unsigned int);
				if (r < sizeof(unsigned int))
				{
					for (size_t n = 0; n < r; ++n)
					{
						unsigned char byte = *input_ptr++;
						*output_ptr++ = (byte ^ lastbyte) ^ ((u >> (n * 8)) & 0xff);
						lastbyte = byte;
					}
				}
				else
				{
					unsigned int input4 = 0;
					for (size_t n = 0; n < sizeof(unsigned int); ++n)
					{
						unsigned char byte = *input_ptr++;
						input4 += (byte ^ lastbyte) << (n * 8);
						lastbyte = byte;
					}
					input4 ^= u;
					for (size_t n = 0; n < sizeof(unsigned int); ++n)
					{
						*output_ptr++ = input4 & 0xff;
						input4 >>= 8;
					}
				}
			}
		}
		void decode(void *output, const void *input, size_t len)
		{
			unsigned char *output_ptr = (unsigned char *)output;
			const unsigned char *input_ptr = (const unsigned char *)input;
			size_t index = (len + sizeof(unsigned int) - 1) / sizeof(unsigned int);
			enlarge(std::min<size_t>(index, (1 << (bestbit * 2))));
			size_t i1, i2;
			i1 = i2 = 0;
			unsigned char lastbyte = 0;
			for (size_t i = 0; i < index; ++i)
			{
				unsigned int u = twices[i2][i1];
				if (++i1 == bestlevel)
				{
					i1 = 0;
					if (++i2 == bestlevel)
					{
						i2 = 0;
					}
				}
				size_t r = len - i * sizeof(unsigned int);
				if (r < sizeof(unsigned int))
				{
					for (size_t n = 0; n < r; ++n)
					{
						unsigned char byte = (*input_ptr++ ^ ((u >> (n * 8)) & 0xff)) ^ lastbyte;
						*output_ptr++ = byte;
						lastbyte = byte;
					}
				}
				else
				{
					unsigned int input4 = 0;
					for (size_t n = 0; n < sizeof(unsigned int); ++n)
					{
						input4 += *input_ptr++ << (n * 8);
					}
					input4 ^= u;
					for (size_t n = 0; n < sizeof(unsigned int); ++n)
					{
						unsigned char byte = (input4 & 0xff) ^ lastbyte;
						*output_ptr++ = byte;
						input4 >>= 8;
						lastbyte = byte;
					}
				}
			}
		}

	protected:
		void enlarge(size_t must)
		{
			for (size_t i = twices.size(), j = (must + bestlevel - 1) >> bestbit; i < j; ++i)
			{
				unsigned int *keyt = (unsigned int *)malloc(bestlevel * sizeof(unsigned int));
				if (twices.empty())
				{
					twices.resize(bestlevel);
					twices.clear();
				}
				twices.push_back(keyt);
				for (size_t n = 0; n < bestlevel; ++n)
				{
					*keyt++ = nextgen();
				}
			}
		}
		void initgen(unsigned int seed)
		{
			state.gen[0] =  seed & 0xffffffff;
			for (int i = 1; i < N; ++i)
			{
				state.gen[i]  =  (0x6c078965 * (state.gen[i - 1] ^ (state.gen[i - 1]  >>  30)) + i);
				state.gen[i] &=  0xffffffff;
			}
			state.left = 1;
		}
		unsigned int nextgen()
		{
			unsigned int r;
			if (0 == --state.left)
			{
				unsigned int* p = state.gen;
				int i;

				for (i = N - M + 1; --i; ++p)
					*p = (p[M] ^ twist(p[0], p[1]));

				for (i = M; --i; ++p)
					*p = (p[M - N] ^ twist(p[0], p[1]));
				*p = p[M - N] ^ twist(p[0], state.gen[0]);
				state.left = N;
				state.next = state.gen;
			}
			r  = *state.next++;
			r ^= (r >> 11);
			r ^= (r << 7) & 0x9d2c5680;
			r ^= (r << 15) & 0xefc60000;
			r ^= (r >> 18);
			return r & 0xffffffff;
		}
	};
}

rc4key * ext_rc4_create(const char *key, unsigned int len)
{
	return new rc4key(key, len);
}

void ext_rc4_destroy(rc4key *key)
{
	delete key;
}

void ext_rc4_encode(rc4key *key, void *output, const void *input, unsigned int length)
{
	key->encode(output, input, (size_t)length);
}

void ext_rc4_decode(rc4key *key, void *output, const void *input, unsigned int length)
{
	key->decode(output, input, (size_t)length);
}
