# BloomFilter
基于共享内存的 `BloomFilter` 库。

* 部分代码借鉴了<https://github.com/armon/bloomd> 项目
* 参考资料
  * <https://my.oschina.net/kiwivip/blog/133498>
  * <http://blog.csdn.net/jiaomeng/article/details/1495500>
  * <http://blog.csdn.net/hguisu/article/details/7866173>

## Require

`SWOOLE`
`PHP-X`
`PHP >= 7.0`

## 安装使用

```bash
mkdir build
cd build
cmake ..
make && make install
```
or 

```bash
make && make install
```

到`php.ini`添加`extension=BloomFilter.so`
