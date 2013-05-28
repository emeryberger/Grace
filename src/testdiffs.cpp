#include <iostream>
using namespace std;

#include <string.h>
#include <sys/mman.h>

//////////////////////////////////////////////////////////////////////

#include <xmmintrin.h>

// @note Uses SSE3, so you must compile with -msse3.

// For every byte in the page,
//   dest[i] unchanged if local[i] == twin[i]
//   dest[i] = local[i] if local[i] != twin[i]

inline void writeDiffs (const void * local,
			const void * twin,
			void * dest)
{
  // IMPORTANT: make sure long long = 64bits!
  enum { VERIFY_LONGLONG_64 = 1 / (sizeof(long long) == 8) };

  // Each page is 4096 bytes.
  enum { PAGE_SIZE = 4096 };

  // A string of one bits.
  __m128i allones = _mm_setzero_si128();
  allones = _mm_cmpeq_epi32(allones, allones); 

  __m128i * localbuf = (__m128i *) local;
  __m128i * twinbuf  = (__m128i *) twin;
  __m128i * destbuf  = (__m128i *) dest;
  
  // Some vectorizing pragamata here; not sure if gcc implements them.

#pragma vector always
  for (int i = 0; i < PAGE_SIZE / sizeof(__m128i); i++) {

    __m128i localChunk, twinChunk;

    localChunk = _mm_load_si128 (&localbuf[i]);
    twinChunk  = _mm_load_si128 (&twinbuf[i]);

    // Compare the local and twin byte-wise.
    __m128i eqChunk = _mm_cmpeq_epi8 (localChunk, twinChunk);

    // Invert the bits by XORing them with ones.
    __m128i neqChunk = _mm_xor_si128 (allones, eqChunk);

    // Write local pieces into destbuf everywhere there are diffs.
    _mm_maskmoveu_si128 (localChunk, neqChunk, (char *) &destbuf[i]);
  }
}

/////////////////////////////////

enum { RANGE = 4096 };
enum { NUMPAGES = 100 };

inline void naiveWriteDiffs (const void * local,
			     const void * twin,
			     void * dest)
{
  char * localBuf = (char *) local;
  char * twinBuf  = (char *) twin;
  char * destBuf  = (char *) dest;

#pragma vector always
  for (int i = 0; i < 4096; i++) {
    if (localBuf[i] != twinBuf[i]) {
      destBuf[i] = localBuf[i];
    }
  }
}

main()
{
  char * local
    = (char *) mmap(0, RANGE * NUMPAGES, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  char * twin
    = (char *) mmap(0, RANGE * NUMPAGES, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  char * dest
    = (char *) mmap(0, RANGE * NUMPAGES, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

  memset ((void *) local, ' ', RANGE * NUMPAGES);
  memset ((void *) twin,  ' ', RANGE * NUMPAGES);
  memset ((void *) dest, '_', RANGE * NUMPAGES);

  local[4] = 'Z';
  local[4095] = 'Q';
  dest[4] = 'A';
  dest[4095] = 'A';

  for (int i = 0; i < 10000; i++) {
    for (int j = 0; j < NUMPAGES; j++) {
      writeDiffs ((const void *) &local[j * RANGE], (const void *) &twin[j * RANGE], (void *) &dest[j * RANGE]);
      // naiveWriteDiffs ((const void *) &local[j * RANGE], (const void *) &twin[j * RANGE], (void *) &dest[j * RANGE]);
    }
  }

  for (int i = 0; i < 4096; i++) {
    cout << dest[i];
  }
  cout << endl;
    
}
