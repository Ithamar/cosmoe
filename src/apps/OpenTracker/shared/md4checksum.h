/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

//
//	MD4 Checksum (See RFC 1186)

#ifndef MD4CHECKSUM_H
#define MD4CHECKSUM_H

#include <string.h>

const char *kChecksumAttrName = "META:md4_checksum";
const unsigned long kChecksumAttrType = 'i128';

class MD4Checksum {
public:

	inline MD4Checksum();
	inline void	Reset();
	inline void Process(char *data, size_t size);
	inline void	GetResult(char[16]);
	inline bool Equals(char[16]);

private:
	inline void	Process16Bytes(const char[16]);	

	char partialChunk[16];
	int partialChunkOffs;
	unsigned long aa, bb, cc, dd;
};

inline MD4Checksum::MD4Checksum() 
{
	Reset();
}

inline void 
MD4Checksum::Reset()
{
	partialChunkOffs = 0;
	
	aa = 0x01UL | (0x23UL << 8) | (0x45UL << 16) | (0x67UL << 24);
	bb = 0x89UL | (0xabUL << 8) | (0xcdUL << 16) | (0xefUL << 24);
	cc = 0xfeUL | (0xdcUL << 8) | (0xbaUL << 16) | (0x98UL << 24);
	dd = 0x76UL | (0x54UL << 8) | (0x32UL << 16) | (0x10UL << 24);
}

#define F(x,y,z) (((x) & (y)) | ((~x) & (z)))
#define G(x,y,z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define H(x,y,z) ((x) ^ (y) ^ (z))
#define ROL(x, n) (((x) << (n)) | ((x) >> (32-(n)))) 
#define R1(a,b,c,d,k,s) (a) = ROL(((a) + F((b),(c),(d)) + block[k]), (s))
#define R2(a,b,c,d,k,s) (a) = ROL((((a) + G((b),(c),(d)) + block[k] + 0x5a827999)), (s))
#define R3(a,b,c,d,k,s) (a) = ROL((((a) + H((b),(c),(d)) + block[k] + 0x6ed9eba1)), (s))

inline void
MD4Checksum::Process16Bytes(const char block[16])
{
	register unsigned long a = aa;
	register unsigned long b = bb;
	register unsigned long c = cc;
	register unsigned long d = dd;

	R1(a,b,c,d,0,3);  R1(d,a,b,c,1,7);  R1(c,d,a,b,2,11);  R1(b,c,d,a,3,19);
	R1(a,b,c,d,4,3);  R1(d,a,b,c,5,7);  R1(c,d,a,b,6,11);  R1(b,c,d,a,7,19);
	R1(a,b,c,d,8,3);  R1(d,a,b,c,9,7);  R1(c,d,a,b,10,11); R1(b,c,d,a,11,19);
	R1(a,b,c,d,12,3); R1(d,a,b,c,13,7); R1(c,d,a,b,14,11); R1(b,c,d,a,15,19);

	R2(a,b,c,d,0,3);  R2(d,a,b,c,4,5);  R2(c,d,a,b,8,9);   R2(b,c,d,a,13,13);
	R2(a,b,c,d,1,3);  R2(d,a,b,c,5,5);  R2(c,d,a,b,9,9);   R2(b,c,d,a,13,13);
	R2(a,b,c,d,2,3);  R2(d,a,b,c,6,5);  R2(c,d,a,b,10,9);  R2(b,c,d,a,14,13);
	R2(a,b,c,d,3,3);  R2(d,a,b,c,7,5);  R2(c,d,a,b,11,9);  R2(b,c,d,a,15,13);

	R3(a,b,c,d,0,3);  R3(d,a,b,c,8,9);  R3(c,d,a,b,4,11);  R3(b,c,d,a,12,15);
	R3(a,b,c,d,2,3);  R3(d,a,b,c,10,9); R3(c,d,a,b,6,11);  R3(b,c,d,a,14,15);
	R3(a,b,c,d,1,3);  R3(d,a,b,c,9,9);  R3(c,d,a,b,5,11);  R3(b,c,d,a,13,15);
	R3(a,b,c,d,3,3);  R3(d,a,b,c,11,9); R3(c,d,a,b,7,11);  R3(b,c,d,a,15,15);

	aa += a;
	bb += b;
	cc += c;
	dd += d;
}

inline void 
MD4Checksum::Process(char *data, size_t size)
{
	// Fill in any remaining chunks
	if (partialChunkOffs > 0) {
		while (size > 0) {
			partialChunk[partialChunkOffs++] = *data++;
			size--;
			
			if (partialChunkOffs == 16) {
				Process16Bytes(partialChunk);
				partialChunkOffs = 0;
				break;
			}
		}		
	}
	
	// Chunk through whole chunks
	while (size >= 16) {
		Process16Bytes(data);
		data += 16;
		size -= 16;	
	}

	// Fill in remaining data
	while (size > 0) {
		partialChunk[partialChunkOffs++] = *data++;
		size--;
	}
}

#define GET_BYTE(a, b) (char)((a >> (8 * b)) & 0xff)

inline void 
MD4Checksum::GetResult(char buf[16])
{
	if (partialChunkOffs > 0) {
		while (partialChunkOffs < 16)
			partialChunk[partialChunkOffs++] = 0;
			
		Process16Bytes(partialChunk);
		partialChunkOffs = 0;
	}

	buf[0]  = GET_BYTE(aa, 0);
	buf[1]  = GET_BYTE(aa, 1);
	buf[2]  = GET_BYTE(aa, 2);
	buf[3]  = GET_BYTE(aa, 3);
	buf[4]  = GET_BYTE(bb, 0);
	buf[5]  = GET_BYTE(bb, 1);
	buf[6]  = GET_BYTE(bb, 2);
	buf[7]  = GET_BYTE(bb, 3);
	buf[8]  = GET_BYTE(cc, 0);
	buf[9]  = GET_BYTE(cc, 1);
	buf[10] = GET_BYTE(cc, 2);
	buf[11] = GET_BYTE(cc, 3);
	buf[12] = GET_BYTE(dd, 0);
	buf[13] = GET_BYTE(dd, 1);
	buf[14] = GET_BYTE(dd, 2);
	buf[15] = GET_BYTE(dd, 3);
}

inline bool 
MD4Checksum::Equals(char sum[16])
{
	char myResult[16];
	GetResult(myResult);
	return (memcmp(myResult, sum, 16) == 0);
}

#endif
