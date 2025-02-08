#include <stdint.h>
#include <sstream>
#include <iomanip>
#include "sha256.h"

typedef uint8_t		u8;
typedef uint32_t	u32;
typedef uint64_t	u64;

// first 32 bits of the fractional parts of the cube roots of the first 64 primes 2..311
const u32 K[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,
        0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,
        0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,
        0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,
        0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,
        0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,
        0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,
        0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,
        0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2,
};

struct SHA256
{
        u32 state[8];
        u32 blockLen;
        u64 bitLen;
        u8 data[64];
        
        SHA256() : blockLen(0), bitLen(0)
        {
                //first 32 bits of the fractional parts of the square roots of the first 8 primes 2..19)
                state[0] = 0x6a09e667;
                state[1] = 0xbb67ae85;
                state[2] = 0x3c6ef372;
                state[3] = 0xa54ff53a;
                state[4] = 0x510e527f;
                state[5] = 0x9b05688c;
                state[6] = 0x1f83d9ab;
                state[7] = 0x5be0cd19;
        }
};

static u32 rotr(u32 x, u32 n) {
	return (x >> n) | (x << (32 - n));
}

static u32 choose(u32 e, u32 f, u32 g) {
	return (e & f) ^ (~e & g);
}

static u32 majority(u32 a, u32 b, u32 c) {
	return (a & (b | c)) | (b & c);
}

static u32 sig0(u32 x) {
	return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

static u32 sig1(u32 x) {
	return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

static void transform(struct SHA256 &sha) {
        u32 maj, xorA, ch, xorE, sum, newA, newE, m[64];
        u32 state[8];

        for (u8 i = 0, j = 0; i < 16; i++, j += 4) { // Split data in 32 bit blocks for the 16 first words
                m[i] = (sha.data[j] << 24) | (sha.data[j + 1] << 16) | (sha.data[j + 2] << 8) | (sha.data[j + 3]);
        }

        for (u8 k = 16 ; k < 64; k++) { // Remaining 48 blocks
                m[k] = sig1(m[k - 2]) + m[k - 7] + sig0(m[k - 15]) + m[k - 16];
        }

        for(u8 i = 0 ; i < 8 ; i++) {
                state[i] = sha.state[i];
        }

        for (u8 i = 0; i < 64; i++) {
                maj   = majority(state[0], state[1], state[2]);
                xorA  = rotr(state[0], 2) ^ rotr(state[0], 13) ^ rotr(state[0], 22);

                ch = choose(state[4], state[5], state[6]);

                xorE  = rotr(state[4], 6) ^ rotr(state[4], 11) ^ rotr(state[4], 25);

                sum  = m[i] + K[i] + state[7] + ch + xorE;
                newA = xorA + maj + sum;
                newE = state[3] + sum;

                state[7] = state[6];
                state[6] = state[5];
                state[5] = state[4];
                state[4] = newE;
                state[3] = state[2];
                state[2] = state[1];
                state[1] = state[0];
                state[0] = newA;
        }

        for(u8 i = 0 ; i < 8 ; i++) {
                sha.state[i] += state[i];
        }
}

static void pad(struct SHA256 &sha) {
        u64 i = sha.blockLen;
        u8 end = sha.blockLen < 56 ? 56 : 64;

        sha.data[i++] = 0x80; // Append a bit 1
        while (i < end) {
                sha.data[i++] = 0x00; // Pad with zeros
        }

        if(sha.blockLen >= 56) {
                transform(sha);
                std::fill(sha.data, sha.data + 56, 0);
        }

        // Append to the padding the total message's length in bits and transform.
        sha.bitLen += sha.blockLen * 8;
        sha.data[63] = sha.bitLen;
        sha.data[62] = sha.bitLen >> 8;
        sha.data[61] = sha.bitLen >> 16;
        sha.data[60] = sha.bitLen >> 24;
        sha.data[59] = sha.bitLen >> 32;
        sha.data[58] = sha.bitLen >> 40;
        sha.data[57] = sha.bitLen >> 48;
        sha.data[56] = sha.bitLen >> 56;
        transform(sha);
}

void sha256(std::string msg, u32 hash[8])
{
        const u8		*data = reinterpret_cast<const u8 *>(msg.c_str());
        u64				len;
        struct SHA256	sha;
        u8 hash_[32];

        len = msg.length();
        sha = SHA256();
        for (u64 i = 0; i < len; i++)
        {
                sha.data[sha.blockLen++] = data[i];
                if(sha.blockLen == 64) {
                        transform(sha);
                        sha.bitLen += 512;
                        sha.blockLen = 0;
                }
        }
        pad(sha);
        for (uint8_t i = 0 ; i < 4 ; i++) for(uint8_t j = 0 ; j < 8 ; j++) hash_[i + (j * 4)] = (sha.state[j] >> (24 - i * 8)) & 0x000000ff;
        for(int i = 0; i < 8; i++)
                hash[i] = hash_[8 * i + 0] << (8 * 0)
                        | hash_[8 * i + 1] << (8 * 1)
                        | hash_[8 * i + 2] << (8 * 2)
                        | hash_[8 * i + 3] << (8 * 3);
}
