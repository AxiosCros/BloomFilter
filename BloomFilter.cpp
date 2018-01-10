#include "phpx.h"

extern "C"
{
#include <php_swoole.h>
#include <swoole.h>
extern void MurmurHash3_x64_128(const void * key, const int len, const uint32_t seed, void *out);
extern void SpookyHash128(const void *key, size_t len, uint64_t seed1, uint64_t seed2, uint64_t *hash1,
        uint64_t *hash2);
}

#include <iostream>

using namespace php;
using namespace std;

#define DEFAULT_QUEUE_SIZE 64

struct BloomFilterObject
{
    size_t capacity;
    char *array;
    uint32_t k_num;
    uint64_t bit_num;
    uint64_t *hashes;
    swLock lock;
};

#define RESOURCE_NAME  "BloomFilterResource"
#define PROPERTY_NAME  "ptr"

static void compute_hashes(uint32_t k_num, char *key, size_t len, uint64_t *hashes)
{
    uint64_t out[2];
    MurmurHash3_x64_128(key, len, 0, out);

    hashes[0] = out[0];
    hashes[1] = out[1];

    uint64_t *hash1 = out;
    uint64_t *hash2 = hash1 + 1;
    SpookyHash128(key, len, 0, 0, hash1, hash2);

    hashes[2] = out[0];
    hashes[3] = out[1];

    for (uint32_t i = 4; i < k_num; i++)
    {
        hashes[i] = hashes[1] + ((i * hashes[3]) % 18446744073709551557U);
    }
}

static void BloomFilterResDtor(zend_resource *res)
{
    BloomFilterObject *bf = static_cast<BloomFilterObject *>(res->ptr);
    efree(bf->hashes);
    sw_shm_free(bf);
}

static PHPX_METHOD(BloomFilter, __construct)
{
    long capacity = args[0].toInt();
    if (capacity <= 0)
    {
        capacity = 65536;
    }

    uint32_t k_num = 2;
    if (args.exists(1))
    {
        k_num = (uint32_t) args[1].toInt();
    }

    BloomFilterObject *bf = (BloomFilterObject *) sw_shm_malloc(sizeof(BloomFilterObject) + capacity);
    bf->capacity = capacity;
    bf->array = (char *) (bf + 1);
    bzero(bf->array, bf->capacity);

    bf->hashes = (uint64_t*) ecalloc(k_num, sizeof(uint64_t));
    bf->bit_num = bf->capacity * 8;
    bf->k_num = k_num;

    swRWLock_create(&bf->lock, 1);

    _this.oSet(PROPERTY_NAME, RESOURCE_NAME, bf);
}

PHPX_METHOD(BloomFilter, has)
{
    BloomFilterObject *bf = _this.oGet<BloomFilterObject>(PROPERTY_NAME, RESOURCE_NAME);
    auto key = args[0];
    compute_hashes(bf->k_num, key.toCString(), key.length(), bf->hashes);

    uint32_t i;
    uint32_t n;
    bool miss;

    bf->lock.lock_rd(&bf->lock);
    for (i = 0; i < bf->k_num; i++)
    {
        n = bf->hashes[i] % bf->bit_num;
        miss = !(bf->array[n / 8] & (1 << (n % 8)));
        if (miss)
        {
            bf->lock.unlock(&bf->lock);
            retval = false;
            return;
        }
    }
    bf->lock.unlock(&bf->lock);
    retval = true;
}

PHPX_METHOD(BloomFilter, add)
{
    BloomFilterObject *bf = _this.oGet<BloomFilterObject>(PROPERTY_NAME, RESOURCE_NAME);
    auto key = args[0];
    compute_hashes(bf->k_num, key.toCString(), key.length(), bf->hashes);

    uint32_t i;
    uint32_t n;

    bf->lock.lock(&bf->lock);
    for (i = 0; i < bf->k_num; i++)
    {
        n = bf->hashes[i] % bf->bit_num;
        bf->array[n / 8] |= (1 << (n % 8));
    }
    bf->lock.unlock(&bf->lock);
}

PHPX_METHOD(BloomFilter, clear)
{
    BloomFilterObject *bf = _this.oGet<BloomFilterObject>(PROPERTY_NAME, RESOURCE_NAME);
    bf->lock.lock(&bf->lock);
    bzero(bf->array, bf->capacity);
    bf->lock.unlock(&bf->lock);
}

PHPX_EXTENSION()
{
    Extension *extension = new Extension("BloomFilter", "0.0.1");

    extension->onStart = [extension]() noexcept
    {
        extension->registerConstant("QUEUE_VERSION", 1001);

        Class *c = new Class("BloomFilter");
        c->addMethod(PHPX_ME(BloomFilter, __construct), CONSTRUCT);
        c->addMethod(PHPX_ME(BloomFilter, add));
        c->addMethod(PHPX_ME(BloomFilter, has));
        c->addMethod(PHPX_ME(BloomFilter, clear));

        extension->registerClass(c);
        extension->registerResource(RESOURCE_NAME, BloomFilterResDtor);
    };

    //extension->onShutdown = [extension]() noexcept {
    //};

    //extension->onBeforeRequest = [extension]() noexcept {
    //    cout << extension->name << "beforeRequest" << endl;
    //};

    //extension->onAfterRequest = [extension]() noexcept {
    //    cout << extension->name << "afterRequest" << endl;
    //};

    extension->require("swoole");

    extension->info({"BloomFilter support", "enabled"},
                    {
                        {"author", "Tianfeng Han"},
                        {"version", extension->version},
                        {"date", "2018-01-10"},
                    });

    return extension;
}
