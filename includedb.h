/*
  You need to add '#define INCLUDEDB_IMPLEMENTATION' before including this header in ONE source file.
  Like this:
      #define INCLUDEDB_IMPLEMENTATION
      #include "includedb.h"
 
  To disable compile-time unit tests:
      #define DISABLE_TESTS
 
  AUTHOR
      Professor Peanut
 
  LICENSE
      See end of file.
 
  HISTORY
      While in alpha, cross version compatibility is not guaranteed. Do not replace
      this file if you need to keep your database readable.
 
      0.1.0  Initial release
 
  PEANUTS
      BTC:  1H1RrCrEgUXDFibpaJciLjS9r7upQs6XPc
      BCH:  qzgfgd6zen70mfzasjtc4rx9m7fhz65zyg0n6v3sdh
      BSV:  15dtAGzzMf6yWF82aYuGKZYMCyP5HoWVLP
      ETH:  0x32a42d02eB021914FE8928d4A60332970F96f2cd
      DCR:  DsWY2Z1NThKqumM6x9oiyM3f2RkW28ruoyA
      LTC:  LWZ5HCcpModc1XcFpjEzz25J58eeQ8fJ7F
      DASH: XqMBmnxrgJWsvF7Hu3uBQ53TpcKLEsxsEi
      Thank you in advance!
      
*/

#ifndef NANOPULSE_DB_H
#define NANOPULSE_DB_H

#define NANOPULSE_VERSION_MAJOR 0
#define NANOPULSE_VERSION_MINOR 1
#define NANOPULSE_VERSION_PATCH 0
#define NANOPULSE_VERSION_NUM ((NANOPULSE_VERSION_MAJOR<<24) | \
                               (NANOPULSE_VERSION_MINOR<<16) | \
                               (NANOPULSE_VERSION_PATCH<< 8))

#if defined(__cplusplus)
  #define COMPTIME constexpr
  #define CTOR3(x,y,z) constexpr x,y,z
  #define CTOR4(w,x,y,z) constexpr w,x,y,z
#else
  #define COMPTIME static const
  #define constexpr
  #define nullptr ((void *)0)
  enum {false=0,true=1};
  typedef int bool;
  #define CTOR3(x,y,z)
  #define CTOR4(w,x,y,z)
  #ifndef DISABLE_TESTS
    #define DISABLE_TESTS
  #endif
#endif


struct nplse__bitvec;

typedef int (*pnplse__bitvecAlloc)(struct nplse__bitvec *bitvec, int amount);

typedef struct nplse__bitvec
{
    CTOR3(nplse__bitvec() : bitvec(nullptr), szVecIn32Chunks(0), nplse__bitvecAlloc(nullptr) {})
    unsigned *bitvec;
    int szVecIn32Chunks;
    pnplse__bitvecAlloc nplse__bitvecAlloc;
} nplse__bitvec;


typedef struct nplse__skipnode
{
    CTOR4(nplse__skipnode() : nodeid(0), filepos(0), next(0), visits(0) {})
    unsigned nodeid;
    int filepos;
    int next; // pointer into 'nodeVec'
    int visits;
} nplse__skipnode;


struct nplse__file;

bool nplse__fileOpen(struct nplse__file *file, const char *filename);
bool nplse__fileCreate(struct nplse__file *file, const char *filename);
bool nplse__fileClose(struct nplse__file *file);
int  nplse__fileWrite(struct nplse__file *file, const unsigned char *bytes, int len, int filepos);
bool nplse__fileGrow(struct nplse__file *file, int len);

#define NANOPULSE_USE_STD_FILE_OPS

#if defined(NANOPULSE_USE_STD_FILE_OPS)
  #include <stdio.h>
  typedef struct nplse__file
  {
      FILE *pFile;
  } nplse__file;
  bool nplse__fileOpen(nplse__file *file, const char *filename)
  {
      file->pFile = fopen(filename, "rb+");
      return !!file->pFile;
  }
  bool nplse__fileCreate(nplse__file *file, const char *filename)
  {
      file->pFile = fopen(filename, "wb+");
      return !!file->pFile;
  }
  bool nplse__fileClose(nplse__file *file)
  {
      return fclose(file->pFile);
  }
  int  nplse__fileWrite(nplse__file *file, const unsigned char *bytes, int len, int filepos)
  {
      fseek(file->pFile, filepos, SEEK_SET);
      return fwrite(bytes, sizeof(unsigned char), len, file->pFile);
  }
  void nplse__fileRead(nplse__file *file, unsigned char *bytes, int len, int filepos)
  {
      fseek(file->pFile, filepos, SEEK_SET);
      const int r = fread(bytes, sizeof(unsigned char), len, file->pFile);
      (void)r;
  }
  bool nplse__fileGrow(nplse__file *file, int len)
  {
      // ¯\_(ツ)_/¯
      (void)file;
      (void)len;
      return true;
  }
#else
  #if defined(_WIN32)
    #include <winbase.h>
  #else
    #include <sys/mman.h>
  #endif
#endif

#if !defined(NANOPULSE_CHUNK_SIZE)
  #define NANOPULSE_CHUNK_SIZE 256
#endif

struct nanopulseDB;

typedef int (*pnplse__write)(struct nanopulseDB *instance, int location, int amount);
typedef int (*pnplse__read)(struct nanopulseDB *instance, int location, int amount);

enum nplse__errorCodes
{
    NPLSE__OK,
    NPLSE__BITVEC_ALLOC,
    NPLSE__SLOTS_ALLOC,
    NPLSE__BUFFER_ALLOC,
    NPLSE__ALREADY_KEY,
    NPLSE__KEY_NOT_FOUND
};

COMPTIME int szBloommap = sizeof(unsigned) * 8;

typedef struct nanopulseDB
{
    // Buffer/Address:
    union
    {
        unsigned char *buffer;
        unsigned char *address;
    };
    int szBuf;
    
    // File:
    union
    {
        nplse__file file;
        unsigned char *mappedArray;
    };
    int chunkSize;
    pnplse__write nplse__write;
    pnplse__read nplse__read;
    nplse__bitvec occupied;
    
    // List:
    nplse__skipnode *nodeVec;
    int headA, headB, headC, headD;
    int nKeys;
    //int nodeVecLen = 32;
    //int addressNewNode = 0;
    int globalVisits;
    int cursor;
    
    // Hashing:
    unsigned seed;
    unsigned bloommap;
    int bloomcounters[szBloommap];
    
    // Error code
    enum nplse__errorCodes ec;
} nanopulseDB;

// Error:
static const char *nplse__errorMsg;




/*
 ------------------------------------------------------------------------------
    Public interface
 ------------------------------------------------------------------------------
*/

// Add a new record
static constexpr int nplse_put(nanopulseDB *instance, const unsigned char *key, int keylen, const unsigned char *val, int vallen);

// Get a pointer to an existing record. *vallen contains the size of the returned value in bytes and can be NULL if
// you are not interested in it. Calling this is going to invalidate previously retuned pointers
static constexpr unsigned char *nplse_get(nanopulseDB *instance, const unsigned char *key, int keylen, int *vallen);

// Delete record at key (TODO)
//static constexpr void nplse_delete(nanopulseDB *instance, unsigned char *key, int keylen);

// Upon opening a database a cursor is pointing to the first record. Use this to move the cursor to the next record
static constexpr void nplse_next(nanopulseDB *instance);

// Get the key at cursor position. Optionally the size of the key can be stored in *keylen. Pass NULL if this is not needed
static constexpr unsigned char *nplse_curGet(nanopulseDB *instance, int *keylen);

// Open existing, or create new
static nanopulseDB *nplse_open(const char *filename);

// Close. Must be called to ensure all changes are written to disk
static void nplse_close(nanopulseDB *instance);

// Get error
static const char *nplse_getError(nanopulseDB *instance);








#ifdef INCLUDEDB_IMPLEMENTATION

#ifndef NANOPULSE_MALLOC
  #include <stdlib.h>
  #define nplse__malloc(sz)          malloc(sz)
  #define nplse__realloc(p,newsz)    realloc(p,newsz)
  #define nplse__zeroalloc(cnt,sz)   calloc(cnt,sz)
  #define nplse__free(p)             free(p)
#endif

#ifndef NANOPULSE_TIME
  #include <time.h>
#endif

inline constexpr void nplse__bloomPut(nanopulseDB *instance)
{
    (void)instance;
}

inline constexpr void nplse__bloomRemove(nanopulseDB *instance)
{
    (void)instance;
}

inline constexpr bool nplse__bloomMaybeHave(const nanopulseDB *instance, const unsigned hash)
{
    const unsigned h = hash & 0xffffffff;
    return true; // !((h & instance->bloommap) ^ h);
}

static constexpr unsigned nplse__xx32(const unsigned char *input, int len, unsigned seed)
{
    // https://github.com/Cyan4973/xxHash
    COMPTIME unsigned XXH_PRIME32_1 = 0x9E3779B1u;
    COMPTIME unsigned XXH_PRIME32_2 = 0x85EBCA77u;
    COMPTIME unsigned XXH_PRIME32_3 = 0xC2B2AE3Du;
    COMPTIME unsigned XXH_PRIME32_4 = 0x27D4EB2Fu;
    COMPTIME unsigned XXH_PRIME32_5 = 0x165667B1u;
    const unsigned char *bEnd = input+len;
    unsigned h=0, bytes=0;
    #define XXH_rotl32(x,r) (((x) << (r)) | ((x) >> (32 - (r))))
    if (len>=16)
    {
        const unsigned char *const limit = bEnd - 15;
        unsigned v1 = seed + XXH_PRIME32_1 + XXH_PRIME32_2;
        unsigned v2 = seed + XXH_PRIME32_2;
        unsigned v3 = seed + 0;
        unsigned v4 = seed - XXH_PRIME32_1;
        do
        {
            bytes = (input[0]+0u) | ((input[1]+0u)<< 8) | ((input[2]+0u)<<16) | ((input[3]+0u)<<24);
            v1 += bytes * XXH_PRIME32_2;
            v1  = XXH_rotl32(v1, 13);
            v1 *= XXH_PRIME32_1;
            input += 4;
            bytes = (input[0]+0u) | ((input[1]+0u)<< 8) | ((input[2]+0u)<<16) | ((input[3]+0u)<<24);
            v2 += bytes * XXH_PRIME32_2;
            v2  = XXH_rotl32(v2, 13);
            v2 *= XXH_PRIME32_1;
            input += 4;
            bytes = (input[0]+0u) | ((input[1]+0u)<< 8) | ((input[2]+0u)<<16) | ((input[3]+0u)<<24);
            v3 += bytes * XXH_PRIME32_2;
            v3  = XXH_rotl32(v3, 13);
            v3 *= XXH_PRIME32_1;
            input += 4;
            bytes = (input[0]+0u) | ((input[1]+0u)<< 8) | ((input[2]+0u)<<16) | ((input[3]+0u)<<24);
            v4 += bytes * XXH_PRIME32_2;
            v4  = XXH_rotl32(v4, 13);
            v4 *= XXH_PRIME32_1;
            input += 4;
        } while (input < limit);
        h = XXH_rotl32(v1, 1)  + XXH_rotl32(v2, 7)
          + XXH_rotl32(v3, 12) + XXH_rotl32(v4, 18);
    }
    else
    {
        h = seed + XXH_PRIME32_5;
    }
    h += len;
    len &= 15;
    while (len >= 4)
    {
        bytes = (input[0]+0u) | ((input[1]+0u)<< 8) | ((input[2]+0u)<<16) | ((input[3]+0u)<<24);
        h += bytes * XXH_PRIME32_3;
        input += 4;
        h  = XXH_rotl32(h, 17) * XXH_PRIME32_4;
        len -= 4;
    }
    while (len > 0)
    {
        h += (*input++) * XXH_PRIME32_5;
        h = XXH_rotl32(h, 11) * XXH_PRIME32_1;
        --len;
    }
    #undef XXH_rotl32
    h ^= h >> 15;
    h *= XXH_PRIME32_2;
    h ^= h >> 13;
    h *= XXH_PRIME32_3;
    h ^= h >> 16;
    return h;
}

static int nplse__bitvecAlloc(nplse__bitvec *bitvec, int amount)
{
    const int szOld = bitvec->szVecIn32Chunks;
    bitvec->szVecIn32Chunks += amount;
    unsigned *tmp = (unsigned *)nplse__zeroalloc(bitvec->szVecIn32Chunks, sizeof(unsigned));
    if (!tmp)
        return 1; // 1 is error
    for (int i=0; i<szOld; ++i)
        tmp[i] = bitvec->bitvec[i];
    nplse__free(bitvec->bitvec);
    bitvec->bitvec = tmp;
    return 0;
}

inline constexpr unsigned nplse__bitvecCheck(const nplse__bitvec *bitvec, int pos)
{
    const unsigned bit = bitvec->bitvec[pos>>5];
    return (bit >> (pos&31)) & 1;
}

inline constexpr void nplse__bitvecSet(nplse__bitvec *bitvec, int pos)
{
    const unsigned bit = 1 << (pos&31);
    bitvec->bitvec[pos>>5] |= bit;
}

inline constexpr int nplse__gatherSlots(nanopulseDB *instance, int requiredSlots)
{
    bool haveAvail = false;
    int location = 0;
    nplse__bitvec *bitvec = &instance->occupied;
    for (; location<((bitvec->szVecIn32Chunks*32)-requiredSlots) && !haveAvail; ++location)
        if (nplse__bitvecCheck(bitvec, location) == 0)
        {
            haveAvail = true;
            for (int i=1; i<requiredSlots-1; ++i)
                haveAvail = haveAvail && (nplse__bitvecCheck(bitvec, location+i) == 0);
        }
    location -= 1;
    if (!haveAvail)
    {
        // make room:
        const int amount = (requiredSlots/32) + 1;
        if (bitvec->nplse__bitvecAlloc(bitvec, amount))
        {
            instance->ec = NPLSE__BITVEC_ALLOC;
            return -1; // failed
        }
        // todo : grow file 32*chunksz
        return nplse__gatherSlots(instance, requiredSlots);
    }
    return location;
}

inline constexpr void nplse__markSlots(nplse__bitvec *bitvec, int start, int len)
{
    for (int i=0; i<len; i++)
        nplse__bitvecSet(bitvec, start+i);
}

inline constexpr int nplse__getNewKeyPos(nanopulseDB *instance)
{
    return instance->nKeys++;
}

static constexpr void nplse__insertSkipnode(nanopulseDB *instance, unsigned key, int pos, int layer)
{
    // special case: key smaller than any before:
    if (key < instance->nodeVec[instance->headD].nodeid)
    {
        instance->nodeVec[pos].next = instance->headD;
        instance->headD = pos;
        return;
    }
    
    int current = instance->headD;
    int next = 0;
    for (int i=0; i<instance->nKeys-2; ++i)
    {
        next = instance->nodeVec[current].next;
        if (instance->nodeVec[next].nodeid > key)
        {
            instance->nodeVec[pos].next = next;
            break;
        }
        else if (current == next)
            i = instance->nKeys; // break
        current = next;
    }
    instance->nodeVec[current].next = pos;
}

inline constexpr void nplse__insertNewSkipnode(nanopulseDB *instance, unsigned key, int filepos)
{
    const int newNodeAddr = nplse__getNewKeyPos(instance);
    instance->nodeVec[newNodeAddr].nodeid  = key;
    instance->nodeVec[newNodeAddr].filepos = filepos;
    
    nplse__insertSkipnode(instance, key, newNodeAddr, 3);
}

static constexpr int nplse__findPrevSkipnode(nanopulseDB *instance, const unsigned key, const int start, const int layer)
{
    int current = start;
    int prev = current;
    for (int i=0; i<instance->nKeys; ++i)
    {
        if (layer<3 && (prev==current || instance->nodeVec[current].nodeid>key))
            return nplse__findPrevSkipnode(instance, key, instance->nodeVec[current].next, layer+1);
        else if (instance->nodeVec[current].nodeid == key)
        {
            instance->nodeVec[current].visits += 1;
            instance->globalVisits += 1;
            //if (((instance->nodeVec[current].visits*100) / instance->globalVisits) > 20)
                //nplse__insertSkipnode(instance, key, current, layer-1);
            return current;
        }
        else if (instance->nodeVec[current].nodeid >= key)
            break;
        prev = current;
        current = instance->nodeVec[current].next;
    }
    return prev;
}

inline constexpr nplse__skipnode *nplse__findSkipnode(nanopulseDB *instance, const unsigned key)
{
    //const int res = nplse__findPrevSkipnode(instance, key, instance->headA, 0);
    const int res = nplse__findPrevSkipnode(instance, key, instance->headD, 3);
    return instance->nodeVec[res].nodeid == key ? &instance->nodeVec[res] : nullptr;
}

static constexpr int nplse__dbBufferResize(nanopulseDB *instance, int newsize)
{
    const int sz = instance->szBuf;
    if (sz < newsize)
    {
        instance->szBuf = ((newsize/sz) * sz) + sz;
        unsigned char *tmp = (unsigned char *)nplse__realloc(instance->buffer, instance->szBuf);
        if (!tmp)
        {
            instance->ec = NPLSE__BUFFER_ALLOC;
            return 1;
        }
        instance->buffer = tmp;
    }
    return 0; // Ok
}

static int nplse__dbWrite(nanopulseDB *instance, int location, int amount)
{
    location += 128; // db header size
    const int res = nplse__fileWrite(&instance->file, instance->buffer, amount, location);
    return res != amount;
}

static int nplse__dbRead(nanopulseDB *instance, int location, int amount)
{
    location += 128;
    
    // make sure the buffer is big enough:
    if (nplse__dbBufferResize(instance, amount) == 1)
        return 1; // Error
    
    nplse__fileRead(&instance->file, instance->buffer, amount, location);
    return 0;
}

static constexpr int nplse__dbAddressResize(nanopulseDB *instance, int newsize)
{
    (void)instance;
    (void)newsize;
    return 0; // Ok
}

static int nplse__dbAddressWrite(nanopulseDB *instance, int location, int amount)
{
    //todo
    (void)instance;
    (void)location;
    (void)amount;
    return 0;
}

static int nplse__dbAddressRead(nanopulseDB *instance, int location, int amount)
{
    // todo
    (void)instance;
    (void)location;
    (void)amount;
    return 0;
}

COMPTIME int nplse__header_keyhashLen        = 4;
COMPTIME int nplse__header_keylenLen         = 4;
COMPTIME int nplse__header_vallenLen         = 4;
COMPTIME int nplse__header_recordPriorityLen = 4;












static constexpr int nplse_put(nanopulseDB *instance, const unsigned char *key, int keylen, const unsigned char *val, int vallen)
{
    const unsigned keyhash = nplse__xx32(key, keylen, instance->seed);
    if (nplse_get(instance, key, keylen, nullptr))
    {
        instance->ec = NPLSE__ALREADY_KEY;
        return 1;
    };
    const int chunkSize = instance->chunkSize;
    const int requiredSizeInByte = nplse__header_keyhashLen
                                 + nplse__header_keylenLen
                                 + nplse__header_vallenLen
                                 + nplse__header_recordPriorityLen
                                 + keylen
                                 + vallen
                                 + 1;
    const int requiredSlots = (requiredSizeInByte/chunkSize) + 1; // !!(requiredSizeInByte%chunkSize);
    const int requiredSizeOfRecord = requiredSlots*chunkSize;
    const int location = nplse__gatherSlots(instance, requiredSlots);
    if (location == -1)
    {
        instance->ec = NPLSE__SLOTS_ALLOC;
        return 1;
    }
    nplse__markSlots(&instance->occupied, location, requiredSlots);
    // Hook up new node:
    nplse__insertNewSkipnode(instance, keyhash, location*chunkSize);
    // Write data:
    if (nplse__dbBufferResize(instance, requiredSizeOfRecord))
        return 1; // Error
    unsigned char *data = instance->buffer;
    data[ 0] = (keyhash>>24) & 0xff;
    data[ 1] = (keyhash>>16) & 0xff;
    data[ 2] = (keyhash>> 8) & 0xff;
    data[ 3] = (keyhash    ) & 0xff;
    data[ 4] = (keylen>>24) & 0xff;
    data[ 5] = (keylen>>16) & 0xff;
    data[ 6] = (keylen>> 8) & 0xff;
    data[ 7] = (keylen    ) & 0xff;
    data[ 8] = (vallen>>24) & 0xff;
    data[ 9] = (vallen>>16) & 0xff;
    data[10] = (vallen>> 8) & 0xff;
    data[11] = (vallen    ) & 0xff;
    // priority is 0 for a new record:
    data[12] = 0;
    data[13] = 0;
    data[14] = 0;
    data[15] = 0;
    data += 16;
    // key first:
    for (int i=0; i<keylen; ++i, data++)
        *data = key[i];
    for (int i=0; i<vallen; ++i, data++)
        *data = val[i];
    return instance->nplse__write(instance, location*chunkSize, requiredSizeOfRecord);
}

static constexpr unsigned char *nplse_get(nanopulseDB *instance, const unsigned char *key, int keylen, int *vallen)
{
    instance->ec = NPLSE__KEY_NOT_FOUND;
    const unsigned keyhash = nplse__xx32(key, keylen, instance->seed);
    if (nplse__bloomMaybeHave(instance, keyhash) == false)
        return nullptr;
        
    // todo increase priority
    
    COMPTIME unsigned headerSize = nplse__header_keyhashLen
                                 + nplse__header_keylenLen
                                 + nplse__header_vallenLen
                                 + nplse__header_recordPriorityLen;
    nplse__skipnode *found = nplse__findSkipnode(instance, keyhash);
    if (found)
    {
        // read header
        instance->nplse__read(instance, found->filepos, headerSize);
        unsigned char *data = &instance->buffer[nplse__header_keyhashLen];
        const unsigned kl = (data[0]<<24) | (data[1]<<16) | (data[2]<< 8) | data[3];
        data += nplse__header_keylenLen;
        const unsigned vl = (data[0]<<24) | (data[1]<<16) | (data[2]<< 8) | data[3];
        // read rest
        instance->nplse__read(instance, found->filepos+headerSize + kl, vl);
        int unusedVallen = 0; // In case vallen == nullptr
        vallen = vallen ? : &unusedVallen;
        *vallen = vl;
        instance->ec = NPLSE__OK;
        return instance->buffer;
    }
    return nullptr;
}

//static constexpr void nplse_delete(nanopulseDB *instance, unsigned char *key, int keylen)
//{
//}

static constexpr void nplse_next(nanopulseDB *instance)
{
    instance->cursor += 1;
}

static constexpr unsigned char *nplse_curGet(nanopulseDB *instance, int *keylen)
{
    *keylen = 0;
    if (instance->cursor == instance->nKeys)
        return nullptr;
    COMPTIME int headerSize = nplse__header_keyhashLen
                            + nplse__header_keylenLen
                            + nplse__header_vallenLen
                            + nplse__header_recordPriorityLen;
    const int filepos = instance->nodeVec[instance->cursor].filepos;
    if (instance->nplse__read(instance, filepos, headerSize) != 0)
        return nullptr;
    const unsigned char *data = instance->buffer;
    const int kl = (data[4]<<24) | (data[5]<<16) | (data[6]<<8) | data[7];
    if (instance->nplse__read(instance, filepos+headerSize, kl) != 0)
        return nullptr;
    *keylen = kl;
    return instance->buffer;
}

static nanopulseDB *nplse_open(const char *filename)
{
    COMPTIME unsigned char magic[] = "npdb";
    COMPTIME unsigned v = NANOPULSE_VERSION_NUM;
    COMPTIME unsigned char versionStr[] = {(v>>24)&0xff, (v>>16)&0xff, (v>>8)&0xff, v&0xff};
    nanopulseDB *newInstance = (nanopulseDB *)nplse__malloc(sizeof(nanopulseDB));
    if (!newInstance)
    {
        nplse__errorMsg = "Couldn't alloc db. Out of mem?";
        return nullptr;
    }
    
    int nKeys = 0;
    if (!nplse__fileOpen(&newInstance->file, filename))
    {
        if (!nplse__fileCreate(&newInstance->file, filename))
        {
            nplse__errorMsg = "Couldn't open db. File corrupted?";
            // Cannot call close() on an instance that hasn't been created correctly.
            // Return NULL instead:
            nplse__free(newInstance);
            return nullptr;
        }
        // write signature
        nplse__fileWrite(&newInstance->file, magic, 4, 0);
        // write version
        nplse__fileWrite(&newInstance->file, versionStr, 4, 4);
        // write chunksize
        COMPTIME unsigned cs = NANOPULSE_CHUNK_SIZE;
        COMPTIME unsigned char chunksize[] = {(cs>>24)&0xff, (cs>>16)&0xff, (cs>>8)&0xff, cs&0xff};
        nplse__fileWrite(&newInstance->file, chunksize, 4, 8);
        newInstance->chunkSize = cs;
        // write keys & "visits"
        COMPTIME unsigned char keysNvisits[] = {0,0,0,0,0,0,0,0};
        nplse__fileWrite(&newInstance->file, keysNvisits, 8, 12);
        newInstance->globalVisits = 0;
        // create seed
        const unsigned sd = 6969; // todo
        const unsigned char seedStr[] = {(sd>>24)&0xff, (sd>>16)&0xff, (sd>>8)&0xff, sd&0xff};
        nplse__fileWrite(&newInstance->file, seedStr, 4, 20);
        newInstance->seed = sd;
    }
    else
    {
        // read npdb
        unsigned char buf[24];
        nplse__fileRead(&newInstance->file, buf, 24, 0);
        bool ok = buf[0]=='n' && buf[1]=='p' && buf[2]=='d' && buf[3]=='b';
        if (!ok)
        {
            nplse__errorMsg = "Couldn't open db. File not recognized";
            nplse__free(newInstance);
            return nullptr;
        }
        // read version vs minimum required
        ok =       buf[4] == NANOPULSE_VERSION_MAJOR;
        ok = ok && buf[5] <= NANOPULSE_VERSION_MINOR;
        ok = ok && buf[6] <= NANOPULSE_VERSION_PATCH;
        (void)     buf[7];
        if (!ok)
        {
            // todo upgrade version if possible!
            
            nplse__errorMsg = "Couldn't open db. Incompatible version";
            nplse__free(newInstance);
            return nullptr;
        }
        // read chunksise
        newInstance->chunkSize = (buf[8]<<24) | (buf[9]<<16) | (buf[10]<<8) | buf[11];
        // read # of keys
        nKeys = (buf[12]<<24) | (buf[13]<<16) | (buf[14]<<8) | buf[15];
        // read visits
          // todo: not yet
        // read randseed
        newInstance->seed = (buf[20]<<24) | (buf[21]<<16) | (buf[22]<<8) | buf[23];
        printf("seed %d  %d %d %d %d \n", newInstance->seed, buf[20],buf[21],buf[22],buf[23]);
        
        
        // chunkSize =
        // read filesz
        // read vists
        
        // if # of total acces > some 'limit', /2!!
    }
    // Create buffer:
    newInstance->buffer = (unsigned char *)nplse__malloc(newInstance->chunkSize);
    newInstance->szBuf = 1;
    // Write fn:
    newInstance->nplse__write = nplse__dbWrite;
    // Read fn:
    newInstance->nplse__read = nplse__dbRead;
    // setup bitvec:
    newInstance->occupied.bitvec = (unsigned *)nplse__zeroalloc(1, sizeof(unsigned));
    newInstance->occupied.szVecIn32Chunks = 1;
    newInstance->occupied.nplse__bitvecAlloc = nplse__bitvecAlloc;
    // setup node list:
    newInstance->nodeVec = (nplse__skipnode *)nplse__malloc(sizeof(nplse__skipnode) * 32 * 1); // todo test *8 !
    
    
    // todo: init heads
    newInstance->headD = 0;
    
    
    newInstance->nKeys = 0;
    // put cursor to the start
    newInstance->cursor = 0;
    // init bloomfilter
    newInstance->bloommap = 0;
    for (int i=0; i<szBloommap; ++i)
        newInstance->bloomcounters[i] = 0;
    // reset error
    newInstance->ec = NPLSE__OK;
    
    
    printf("num ky %d \n", nKeys);
    
    // Build index:
    COMPTIME int dbHeaderSize = 128;
    int offset = 0;
    unsigned char buf[20];
    for (int i=0; i<nKeys; ++i)
    {
        nplse__fileRead(&newInstance->file, buf, 16, offset+dbHeaderSize);
        const unsigned keyhash = (buf[0]<<24) | (buf[1]<<16) | (buf[2]<<8) | buf[3];
        nplse__insertNewSkipnode(newInstance, keyhash, offset);
        // get position for the next record:
        const unsigned keylen = (buf[ 4]<<24) | (buf[ 5]<<16) | (buf[ 6]<<8) | buf[ 7];
        const unsigned vallen = (buf[ 8]<<24) | (buf[ 9]<<16) | (buf[10]<<8) | buf[11];
        const int requiredSizeInByte = nplse__header_keyhashLen
                                     + nplse__header_keylenLen
                                     + nplse__header_vallenLen
                                     + nplse__header_recordPriorityLen
                                     + keylen
                                     + vallen
                                     + 1;
        const int requiredSlots = (requiredSizeInByte/newInstance->chunkSize) + 1;
        const int slotLocation = nplse__gatherSlots(newInstance, requiredSlots);
        nplse__markSlots(&newInstance->occupied, slotLocation, requiredSlots);
        offset += newInstance->chunkSize * requiredSlots;
    }
    
    return newInstance;
}

static void nplse_close(nanopulseDB *instance)
{
    if (instance == nullptr)
        return;
        
    unsigned char buf[4];
    const int nk = instance->nKeys;
    buf[0]=nk>>24; buf[1]=(nk>>16)&0xff; buf[2]=(nk>>8)&0xff; buf[3]=nk&0xff;
    nplse__fileWrite(&instance->file, buf, 4, 12);
    
    // todo write inm.!!
    const unsigned sd = instance->seed;
    buf[0]=sd>>24; buf[1]=(sd>>16)&0xff; buf[2]=(sd>>8)&0xff; buf[3]=sd&0xff;
    nplse__fileWrite(&instance->file, buf, 4, 20);
    
    nplse__free(instance->buffer);
    nplse__fileClose(&instance->file);
    nplse__free(instance->occupied.bitvec);
    // todo: write all visits from nodes to file!
    nplse__free(instance->nodeVec);
    nplse__free(instance);
}

static const char *nplse_getError(nanopulseDB *instance)
{
    if (!instance)
        return nplse__errorMsg;
    switch (instance->ec)
    {
        case NPLSE__BITVEC_ALLOC:
            nplse__errorMsg = "Couldn't grow bitvec. Out of mem?";
            break;
        case NPLSE__BUFFER_ALLOC:
            nplse__errorMsg = "nplse__dbRead(): failed to realloc buffer";
            break;
        case NPLSE__ALREADY_KEY:
            nplse__errorMsg = "key already exists";
            break;
        case NPLSE__SLOTS_ALLOC:
            nplse__errorMsg = "couldn't allocate more slots. Out of mem?";
            break;
        default:
            nplse__errorMsg = "Ok";
    }
    return nplse__errorMsg;
}












#if !defined(DISABLE_TESTS)

constexpr unsigned nplse__testBitvec()
{
    unsigned bitvecBits[3] = {0};
    nplse__bitvec testBitvec;
    testBitvec.bitvec = bitvecBits;
    for (int i=0; i<64; ++i)
        nplse__bitvecSet(&testBitvec, i);
    const bool TEST_BIT_63 =  nplse__bitvecCheck(&testBitvec, 63)
                           && !nplse__bitvecCheck(&testBitvec, 64);
    nplse__bitvecSet(&testBitvec, 64);
    const bool TEST_BIT_64 =  nplse__bitvecCheck(&testBitvec, 64)
                           && !nplse__bitvecCheck(&testBitvec, 65);
    
    return  TEST_BIT_63
         | (TEST_BIT_64 << 1);
}
constexpr unsigned resBitvec = nplse__testBitvec();

static_assert(resBitvec&1, "bit 63 not correct");
static_assert(resBitvec&2, "bit 64 not correct");


constexpr unsigned nplse__testSlots()
{
    nanopulseDB testDB{ .mappedArray=nullptr,
                        .chunkSize=0,
                        .nodeVec=nullptr,
                        .seed=0
                      };
    unsigned bitvecBits[3/* *32 */] = {0};
    testDB.occupied.bitvec = bitvecBits;
    testDB.occupied.szVecIn32Chunks  = 1;
    auto testAlloc = [](nplse__bitvec *bitvec, int amount)->int
                     {
                         if ((bitvec->szVecIn32Chunks+amount) <= 3)
                         {
                             bitvec->szVecIn32Chunks += amount;
                             return 0;
                         }
                         return 1;
                     };
    testDB.occupied.nplse__bitvecAlloc = testAlloc;
    const bool TEST_HAVE_ZERO_SLOTS = nplse__gatherSlots(&testDB, 0) == 0;
    int location = nplse__gatherSlots(&testDB, 2);
    nplse__markSlots(&testDB.occupied, location, 2);
    const bool TEST_TWO_SLOTS_OCCUPIED =  (bitvecBits[0]&1)
                                       && (bitvecBits[0]&2)
                                       && !(bitvecBits[0]&4)
                                       && location==0;
    location = nplse__gatherSlots(&testDB, 1);
    nplse__markSlots(&testDB.occupied, location, 1);
    const bool TEST_LOCATION_TWO = location == 2;
    location = nplse__gatherSlots(&testDB, 7);
    nplse__markSlots(&testDB.occupied, location, 7);
    const bool TEST_LOCATION_THREE = location == 3;
    // test large allocation:
    location = nplse__gatherSlots(&testDB, 24+39);
    nplse__markSlots(&testDB.occupied, location, 24+39);
    const bool TEST_LOCATION_EIGHT = location == 10;
    location = nplse__gatherSlots(&testDB, 22);
    nplse__markSlots(&testDB.occupied, location, 22);
    const bool TEST_LOCATION_DEEP = location == 10+24+39;
    const bool TEST_ALL_FLAGS_SET =  !(bitvecBits[0]^0xffffffff)
                                  && !(bitvecBits[1]^0xffffffff)
                                  && !(bitvecBits[2]^0x7fffffff)
    //&& !(bitvecBits[2]^0b00000000000000000000000011111111)
    //&& !(bitvecBits[2]^0b1111111111111111111111111111111)
    //&& bitvecBits[2] > 0x1fffffffu
    ;
    
    // todo: test free slots!
    
    
    return  TEST_HAVE_ZERO_SLOTS
         | (TEST_TWO_SLOTS_OCCUPIED<< 1)
         | (TEST_LOCATION_TWO<< 2)
         | (TEST_LOCATION_THREE<< 3)
         | (TEST_LOCATION_EIGHT<< 4)
         | (TEST_LOCATION_DEEP<< 5)
         | (TEST_ALL_FLAGS_SET<< 6)
         ;
}
constexpr unsigned resSlots = nplse__testSlots();

static_assert(resSlots&    1, "couldn't gather 0 slots");
static_assert(resSlots&    2, "slots were not correctly marked as 'occupied'");
static_assert(resSlots&    4, "incorrect slot position (2)");
static_assert(resSlots&    8, "incorrect slot position (3)");
static_assert(resSlots&   16, "incorrect slot position (8)");
static_assert(resSlots&   32, "incorrect slot position (8+24+39)");
static_assert(resSlots&   64, "flags set incorrectly");


constexpr unsigned nplse__testSkiplist()
{
    // setup:
    nplse__skipnode testNodes[32];
    nanopulseDB testDB{ .mappedArray=nullptr,
                        .chunkSize=0,
                        .nodeVec=testNodes,
                        .seed=0
                      };
    // start testing:
    nplse__insertNewSkipnode(&testDB, 111, 0);
    const bool TEST_HEAD_IS_ZERO = testDB.headD == 0;
    nplse__insertNewSkipnode(&testDB, 333, 1);
    const bool TEST_NODE_ADD =  testNodes[0].nodeid == 111
                             && testNodes[testDB.headD].nodeid == 111
                             && testDB.headD == 0
                             && testNodes[0].next == 1
                             && testNodes[1].next == 0
                             && testNodes[testNodes[0].next].nodeid == 333;
    const bool TEST_NODES_POINTING_TO_CORRECT_POSITION =  testNodes[0].filepos == 0
                                                       && testNodes[1].filepos == 1;
    // insert node between:
    nplse__insertNewSkipnode(&testDB, 222, 2);
    const bool TEST_NODE_BETWEEN =  testNodes[2].nodeid == 222
                                 && testNodes[2].next == 1
                                 && testNodes[1].next == 0
                                 && testNodes[0].next == 2;
    // find nodes:
    const nplse__skipnode *first  = nplse__findSkipnode(&testDB, 111);
    const nplse__skipnode *second = nplse__findSkipnode(&testDB, 333);
    const nplse__skipnode *third  = nplse__findSkipnode(&testDB, 222);
    const bool TEST_NODES_FOUND =  first && second && third
                                && first->filepos==0 && second->filepos==1 && third->filepos==2;
    // add node to end:
    nplse__insertNewSkipnode(&testDB, 444, 3);
    const nplse__skipnode *found = nplse__findSkipnode(&testDB, 444);
    const bool TEST_NODE_AT_END =  found && found->filepos == 3
                                && testNodes[3].nodeid == 444
                                && testNodes[1].next == 3
                                && testNodes[3].next == 0;
    // not found:
    const bool TEST_NODE_NOT_FOUND = nplse__findSkipnode(&testDB, 69) == nullptr;
    
    //todo test remove
    
    return  TEST_HEAD_IS_ZERO
         | (TEST_NODE_ADD << 1)
         | (TEST_NODES_POINTING_TO_CORRECT_POSITION << 2)
         | (TEST_NODE_BETWEEN << 3)
         | (TEST_NODES_FOUND << 4)
         | (TEST_NODE_AT_END << 5)
         | (TEST_NODE_NOT_FOUND << 6)
         ;
}
constexpr unsigned resList = nplse__testSkiplist();

static_assert(resList&  1, "head pointing to wrong place");
static_assert(resList&  2, "node was not correctly inserted");
static_assert(resList&  4, "nodes did not sit where they were supposed to");
static_assert(resList&  8, "nodes not correctly sorted");
static_assert(resList& 16, "nodes could not be recovered");
static_assert(resList& 32, "node at end not found");
static_assert(resList& 64, "nplse__findSkipnode() did not return NULL");


constexpr unsigned nplse__testDBOps()
{
    // setup:
    unsigned char testBuffer[33*64] = {0};
    unsigned char fakeFile[4*32*64] = {0};
    nplse__skipnode testNodes[3*32];
    nanopulseDB testDB{ .buffer=testBuffer,
                        .szBuf=sizeof(testBuffer),
                        .mappedArray=fakeFile,
                        .chunkSize=64,
                        .nodeVec=testNodes,
                        .seed=6969
                       };
    auto testWrite = [](nanopulseDB *instance, int location, int amount)->int
                     {
                         unsigned char *data = &instance->mappedArray[location];
                         for (int i=0; i<amount; ++i)
                             data[i] = instance->buffer[i];
                         return 0; // success
                     };
    auto testRead = [](nanopulseDB *instance, int location, int amount)
                    {
                        unsigned char *data = &instance->mappedArray[location];
                        for (int i=0; i<amount; ++i)
                            instance->buffer[i] = data[i];
                        return 0; // success
                    };
    testDB.nplse__write = testWrite;
    testDB.nplse__read = testRead;
    unsigned bitvecBits[4/* *32 */] = {0};
    testDB.occupied.bitvec = bitvecBits;
    testDB.occupied.szVecIn32Chunks = 1;
    auto testAlloc = [](nplse__bitvec *bitvec, int amount)->int
                     {
                         if ((bitvec->szVecIn32Chunks + amount) <= 4)
                         {
                             bitvec->szVecIn32Chunks += amount;
                             return 0;
                         }
                         return 1;
                     };
    testDB.occupied.nplse__bitvecAlloc = testAlloc;
    // put:
    unsigned char keyA[6] = "hello";
    unsigned char valA[55] = "wurld 1111111111222222222233333333334444444444abcdef.,";
    const bool TEST_PUT_SUCCESS_A = nplse_put(&testDB, keyA, 6, valA, 55) == 0;
    const bool TEST_SHOULD_BE_W =  fakeFile[16 + sizeof(keyA)] == 'w'
                                && fakeFile[20 + sizeof(keyA)] == 'd'
                                && fakeFile[22 + sizeof(keyA)] == '1'
                                && fakeFile[68 + sizeof(keyA)] == '.';
    const bool TEST_TWO_SLOTS_TAKEN =  nplse__bitvecCheck(&testDB.occupied, 0)
                                    && nplse__bitvecCheck(&testDB.occupied, 1);
    const bool TEST_NODE_CREATED_CORRECTLY =  testDB.nodeVec[0].nodeid == nplse__xx32(keyA, 6, testDB.seed)
                                           && testDB.nodeVec[0].next == 0;
    // get:
    const unsigned char *resA = nplse_get(&testDB, keyA, 6, nullptr);
    const bool TEST_GET_SUCCESS_A = resA && (resA[0]=='w') && (resA[50]=='e') && (resA[51]=='f') && (resA[52]=='.') && (resA[53]==',');
    // put again:
    unsigned char keyB[5] = "lexi";
    unsigned char valB[5] = "paxi";
    const bool TEST_PUT_SUCCESS_B = nplse_put(&testDB, keyB, 5, valB, 5) == 0;
    const bool TEST_SHOULD_BE_P =  fakeFile[64+64+16 + sizeof(keyB)] == 'p'
                                && fakeFile[64+64+16+3 + sizeof(keyB)] == 'i';
    const bool TEST_THREE_SLOTS_TAKEN =  nplse__bitvecCheck(&testDB.occupied, 0)
                                      && nplse__bitvecCheck(&testDB.occupied, 1)
                                      && nplse__bitvecCheck(&testDB.occupied, 2)
                                      && !nplse__bitvecCheck(&testDB.occupied, 3)
                                      && !nplse__bitvecCheck(&testDB.occupied, 32);
    const bool TEST_SECOND_NODE =  testDB.nodeVec[0].nodeid == nplse__xx32(keyA, 6, testDB.seed)
                                && testDB.nodeVec[1].nodeid == nplse__xx32(keyB, 5, testDB.seed);
    // get again:
    int vallen=0;
    const unsigned char *resB = nplse_get(&testDB, keyB, 5, &vallen);
    const bool TEST_GET_SUCCESS_B = resB && resB[0]=='p' && vallen==5;
    // get s/thing that doesn't exist:
    unsigned char keyC[5] = "😕";
    const unsigned char *notFound = nplse_get(&testDB, keyC, 5, nullptr);
    const bool TEST_GET_FAILED = notFound == nullptr;
    // put many:
    bool didPut = true;
    unsigned char keyD[6] = "fixi_";
    for (int i=0; i<30; ++i)
    {
        keyD[4] = i+'0';
        const unsigned char *valD = &keyD[4];
        didPut = (nplse_put(&testDB, keyD, 6, valD, 1) == 0) && didPut;
    }
    const bool TEST_PUT_MANY_SUCCESS =  didPut
                                     && nplse__bitvecCheck(&testDB.occupied, 3)
                                     && nplse__bitvecCheck(&testDB.occupied, 32)
                                     && !nplse__bitvecCheck(&testDB.occupied, 33);
    // get last:
    const unsigned char *oldkey = nplse_get(&testDB, keyA, 6, nullptr);
    const unsigned char *last = nplse_get(&testDB, keyD, 6, nullptr);
    const bool TEST_GET_MANY_SUCCESS = last && last[0] == keyD[4];
    // get all:
    bool didGet = true;
    for (int i=0; i<30; ++i)
    {
        keyD[4] = i+'0';
        const unsigned char *valD = nplse_get(&testDB, keyD, 6, nullptr);
        didGet = didGet && valD && (valD[0] == i+'0');
    }
    const bool TEST_GET_MANY_SUCCESS_PART2 = didGet;
    // put very big value:
    unsigned char keyE[18] = "hello! I'm BIIIIG";
    unsigned char valE[32*64+1] = {0};
    valE[32*64] = 'x';
    const bool TEST_PUT_BIG_VALUE = nplse_put(&testDB, keyE, 18, valE, 32*64+1) == 0;
    // make sure all bits marked correctly:
    bool marked = true;
    for (int i=0; i<34; ++i)
        marked = nplse__bitvecCheck(&testDB.occupied, 1+1+30+i) && marked;
    const bool TEST_33_SLOTS_TAKEN = marked;
    // get 'big':
    const unsigned char *resE = nplse_get(&testDB, keyE, 18, &vallen);
    const bool TEST_BIG_VAL = resE && (vallen == 32*64+1) && resE[32*64]=='x' && resE[32*64-1]!='x';
    // check if cursor points to first record:
    int keylen = 0;
    unsigned char *curKey = nplse_curGet(&testDB, &keylen);
    const bool TEST_KEY_AT_CURSOR =  curKey
                                  && (keylen == 6)
                                  && curKey[0] == 'h'
                                  && curKey[4] == 'o';
    // shift cursor
    nplse_next(&testDB);
    curKey = nplse_curGet(&testDB, &keylen);
    const bool TEST_KEY_AFTER_CALLING_NEXT =  curKey
                                           && (keylen == 5)
                                           && curKey[0] == 'l'
                                           && curKey[3] == 'i';
    for (int i=0; i<31; ++i)
        nplse_next(&testDB);
    curKey = nplse_curGet(&testDB, &keylen);
    const bool TEST_LAST_KEY_AFTER_CALLING_NEXT =  curKey
                                                && (keylen == 18)
                                                && curKey[0] == 'h'
                                                && curKey[16] == 'G';
    // cursor end
    nplse_next(&testDB);
    curKey = nplse_curGet(&testDB, &keylen);
    const bool TEST_CURSOR_END = (keylen == 0) && (curKey == nullptr);
    // double put
    const bool TEST_DOUBLE_PUT_SHOULD_FAIL =  nplse_put(&testDB, keyB, 5, valB, 5) == 1
                                           && testDB.ec == NPLSE__ALREADY_KEY;
    
    
    return  TEST_PUT_SUCCESS_A
         | (TEST_SHOULD_BE_W << 1)
         | (TEST_TWO_SLOTS_TAKEN << 2)
         | (TEST_NODE_CREATED_CORRECTLY << 3)
         | (TEST_GET_SUCCESS_A << 4)
         | (TEST_PUT_SUCCESS_B << 5)
         | (TEST_SHOULD_BE_P << 6)
         | (TEST_THREE_SLOTS_TAKEN << 7)
         | (TEST_SECOND_NODE << 8)
         | (TEST_GET_SUCCESS_B << 9)
         | (TEST_GET_FAILED << 10)
         | (TEST_PUT_MANY_SUCCESS << 11)
         | (TEST_GET_MANY_SUCCESS << 12)
         | (TEST_GET_MANY_SUCCESS_PART2 << 13)
         | (TEST_PUT_BIG_VALUE << 14)
         | (TEST_33_SLOTS_TAKEN << 15)
         | (TEST_BIG_VAL << 16)
         | (TEST_KEY_AT_CURSOR << 17)
         | (TEST_KEY_AFTER_CALLING_NEXT << 18)
         | (TEST_LAST_KEY_AFTER_CALLING_NEXT << 19)
         | (TEST_DOUBLE_PUT_SHOULD_FAIL << 20)
         ;
}
constexpr unsigned resTestOps = nplse__testDBOps();

static_assert(resTestOps&     1 , "nplse_put() failed");
static_assert(resTestOps&(1<< 1), "the fakeFile did not contain correct data after nplse_put()");
static_assert(resTestOps&(1<< 2), "last two bits were not marked 'occupied' after nplse_put()");
static_assert(resTestOps&(1<< 3), "node incorrect");
static_assert(resTestOps&(1<< 4), "nplse_get() did not return 'wurld'");
static_assert(resTestOps&(1<< 5), "nplse_put() failed with 'lexi'");
static_assert(resTestOps&(1<< 6), "the fakeFile did not contain correct data after another nplse_put()");
static_assert(resTestOps&(1<< 7), "three slots should now be occupied");
static_assert(resTestOps&(1<< 8), "second node not correct");
static_assert(resTestOps&(1<< 9), "second key/val incorrect");
static_assert(resTestOps&(1<<10), "should be NULL, instead some value was returned!");
static_assert(resTestOps&(1<<11), "could not nplse_put() all the keys");
static_assert(resTestOps&(1<<12), "nplse_get() failed to return desired value");
static_assert(resTestOps&(1<<13), "at least one value returned did not match the original data");
static_assert(resTestOps&(1<<14), "could not nplse_put() big value");
static_assert(resTestOps&(1<<15), "not all slots marked 'occupied'");
static_assert(resTestOps&(1<<16), "nplse_get() did not return a large value correctly");
static_assert(resTestOps&(1<<17), "the key at the cursor position should be the first key");
static_assert(resTestOps&(1<<18), "incorrect key returned after calling nplse_next()");
static_assert(resTestOps&(1<<19), "incorrect key at last cursor position");
static_assert(resTestOps&(1<<20), "nplse_put()ing the same key twice should produce a NPLSE__ALREADY_KEY error code");

constexpr unsigned nplse__testDBput()
{
    // 25*256 == 6400
    unsigned char testBuffer[25*256] = {0};
    unsigned char fakeFile[(25*256)+(25*256)+(25*256)] = {0};
    nplse__skipnode testNodes[3];
    nanopulseDB anotherTestDB{ .buffer=testBuffer,
                               .szBuf=sizeof(testBuffer),
                               .mappedArray=fakeFile,
                               .chunkSize=256,
                               .nodeVec=testNodes,
                               .seed=4242
                             };
    auto testWrite = [](nanopulseDB *instance, int location, int amount)->int
                     {
                         unsigned char *data = &instance->mappedArray[location];
                         for (int i=0; i<amount; ++i)
                             data[i] = instance->buffer[i];
                         return 0; // success
                     };
    auto testRead = [](nanopulseDB *instance, int location, int amount)
                    {
                        unsigned char *data = &instance->mappedArray[location];
                        for (int i=0; i<amount; ++i)
                            instance->buffer[i] = data[i];
                        return 0; // success
                    };
    anotherTestDB.nplse__write = testWrite;
    anotherTestDB.nplse__read = testRead;
    unsigned bitvecBits[3/* *32 */] = {0};
    anotherTestDB.occupied.bitvec = bitvecBits;
    anotherTestDB.occupied.szVecIn32Chunks = 1;
    auto testAlloc = [](nplse__bitvec *bitvec, int amount)->int
                     {
                         if ((bitvec->szVecIn32Chunks + amount) <= 3)
                         {
                             bitvec->szVecIn32Chunks += amount;
                             return 0;
                         }
                         return 1;
                     };
    anotherTestDB.occupied.nplse__bitvecAlloc = testAlloc;
    
    auto makeKey = [](unsigned char *key, const unsigned src)
    {
        key[0] = (src>>24)&0xff;
        key[1] = (src>>16)&0xff;
        key[2] = (src>> 8)&0xff;
        key[3] =  src     &0xff;
        key[4] = '.';
        key[5] = '.';
    };
    
    unsigned char key[6] = {0};
    makeKey(key, 12345678);
    unsigned char val[6144] = {0};
    for (int i=0; i<6144; ++i)
        val[i] = '.';
    
    const bool TEST_PUT_SUCCESS_01 = nplse_put(&anotherTestDB, key, 6, val, 6144) == 0;
    unsigned char *res01 = nplse_get(&anotherTestDB, key, 6, nullptr);
    const bool TEST_GET_SUCCESS_01 = res01 != nullptr;
    
    makeKey(key, 87654321);
    
    const bool TEST_PUT_SUCCESS_02 = nplse_put(&anotherTestDB, key, 6, val, 6144) == 0;
    unsigned char *res02 = nplse_get(&anotherTestDB, key, 6, nullptr);
    const bool TEST_GET_SUCCESS_02 = res02 != nullptr;
    
    makeKey(key, 12343210);
    val[0] = 'a'; val[6143] = 'z';
 
    const bool TEST_PUT_SUCCESS_03 = nplse_put(&anotherTestDB, key, 6, val, 6144) == 0;
    unsigned char *res03 = nplse_get(&anotherTestDB, key, 6, nullptr);
    const bool TEST_GET_SUCCESS_03 = res03 != nullptr;
    
    const bool TEST_VALUES_ARE_CORRECT =  res03
                                       && res03[0] == 'a'
                                       && res03[6143] == 'z';
    
    return  TEST_PUT_SUCCESS_01
         | (TEST_GET_SUCCESS_01 << 1)
         | (TEST_PUT_SUCCESS_02 << 2)
         | (TEST_GET_SUCCESS_02 << 3)
         | (TEST_PUT_SUCCESS_03 << 4)
         | (TEST_PUT_SUCCESS_03 << 5)
         | (TEST_VALUES_ARE_CORRECT << 6)
         ;
}
constexpr unsigned resTestPut = nplse__testDBput();

static_assert(resTestPut&  1, "did not put key (1)");
static_assert(resTestPut&  2, "did not get key (1)");
static_assert(resTestPut&  4, "did not put key (2)");
static_assert(resTestPut&  8, "did not get key (2)");
static_assert(resTestPut& 16, "did not put key (3)");
static_assert(resTestPut& 32, "did not get key (3)");
static_assert(resTestPut& 64, "res03 values incorrect");

#endif // !defined(DISABLE_TESTS)

#endif // INCLUDEDB_IMPLEMENTATION


#endif // NANOPULSE_DB_H

/*
 ------------------------------------------------------------------------------
 Copyright (c) 2020 Professor Peanut
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 ------------------------------------------------------------------------------
*/
