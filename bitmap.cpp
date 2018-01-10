#include "phpx.h"

extern "C"
{
#include <php_swoole.h>
}

#include <iostream>

using namespace php;
using namespace std;

#define DEFAULT_QUEUE_SIZE 64

struct BitmapObject
{
    size_t size;
    size_t num;
    char *map;
};

#define RESOURCE_NAME  "BitmapResource"
#define PROPERTY_NAME  "ptr"

//destructor
static void bitmapResDtor(zend_resource *res)
{
    BitmapObject *bitmap = static_cast<BitmapObject *>(res->ptr);
    sw_shm_free(bitmap->map);
    efree(bitmap);
}

static PHPX_METHOD(Bitmap, __construct)
{
    BitmapObject *bitmap = (BitmapObject *) emalloc(sizeof(BitmapObject));
    bitmap->num = args[0].toInt();
    bitmap->size = bitmap->num / 8 + (bitmap->num % 8 > 0 ? 1 : 0);
    bitmap->map = (char *) sw_shm_malloc(bitmap->size);
    bzero(bitmap->map, bitmap->size);
    _this.oSet(PROPERTY_NAME, RESOURCE_NAME, bitmap);
}

static PHPX_METHOD(Bitmap, get)
{
    BitmapObject *bitmap = _this.oGet<BitmapObject>(PROPERTY_NAME, RESOURCE_NAME);
    long n = args[0].toInt();
    if (n > bitmap->num)
    {
        retval = false;
        return;
    }

    retval = (bitmap->map[n / 8] & (1 << (n % 8))) > 0;
}

PHPX_METHOD(Bitmap, set)
{
    BitmapObject *bitmap = _this.oGet<BitmapObject>(PROPERTY_NAME, RESOURCE_NAME);
    long n = args[0].toInt();
    if (n > bitmap->num)
    {
        retval = false;
        return;
    }

    if (args[1].toBool())
    {
        bitmap->map[n / 8] |= (1 << (n % 8));
    }
    else
    {
        bitmap->map[n / 8] &= ~(1 << (n % 8));
    }
}

PHPX_METHOD(Bitmap, getEx)
{
    BitmapObject *bitmap = _this.oGet<BitmapObject>(PROPERTY_NAME, RESOURCE_NAME);
    auto key = args[0];
    String s = key;
    long n = s.hashCode() % bitmap->num;
    int bit_offset = n % 8;
    long index = (n / 8) + (bit_offset > 0 ? 1 : 0);

    retval = (bitmap->map[index] & (2 << bit_offset));
}

PHPX_METHOD(Bitmap, setEx)
{
    BitmapObject *bitmap = _this.oGet<BitmapObject>(PROPERTY_NAME, RESOURCE_NAME);
    auto key = args[0];
    String s = key;
    long n = s.hashCode() % bitmap->num;
    int bit_offset = n % 8;
    long index = (n / 8) + (bit_offset > 0 ? 1 : 0);

    retval = (bitmap->map[index] & (2 << bit_offset));
}


PHPX_EXTENSION()
{
    Extension *extension = new Extension("bitmap", "0.0.1");

    extension->onStart = [extension]() noexcept
    {
        extension->registerConstant("QUEUE_VERSION", 1001);

        Class *c = new Class("Bitmap");
        c->addMethod(PHPX_ME(Bitmap, __construct), CONSTRUCT);
        c->addMethod(PHPX_ME(Bitmap, get));
        c->addMethod(PHPX_ME(Bitmap, getEx));
        c->addMethod(PHPX_ME(Bitmap, set));
        c->addMethod(PHPX_ME(Bitmap, setEx));

        extension->registerClass(c);
        extension->registerResource(RESOURCE_NAME, bitmapResDtor);
    };

    //extension->onShutdown = [extension]() noexcept {
    //};

    //extension->onBeforeRequest = [extension]() noexcept {
    //    cout << extension->name << "beforeRequest" << endl;
    //};

    //extension->onAfterRequest = [extension]() noexcept {
    //    cout << extension->name << "afterRequest" << endl;
    //};

    extension->info({"bitmap support", "enabled"},
                    {
                        {"author", "Tianfeng Han"},
                        {"version", extension->version},
                        {"date", "2018-01-10"},
                    });

    return extension;
}
