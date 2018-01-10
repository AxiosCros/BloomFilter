PHP_INCLUDE = `php-config --includes`
PHP_LIBS = `php-config --libs`
PHP_LDFLAGS = `php-config --ldflags`
PHP_INCLUDE_DIR = `php-config --include-dir`
PHP_EXTENSION_DIR = `php-config --extension-dir`

bitmap.so: bitmap.cpp
	c++ -DHAVE_CONFIG_H -g -o bitmap.so -O0 -fPIC -shared bitmap.cpp -std=c++11 -lphpx ${PHP_INCLUDE} -I${PHP_INCLUDE_DIR} -I${PHP_INCLUDE_DIR}/ext/swoole -I${PHP_INCLUDE_DIR}/ext/swoole/include ${PHP_LIBS} ${PHP_LDFLAGS}
install: bitmap.so
	cp bitmap.so ${PHP_EXTENSION_DIR}/
clean:
	rm *.so
