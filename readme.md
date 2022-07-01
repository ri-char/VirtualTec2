# Tec-2模拟机(联机终端)

由于[zhouguiheng/VirtualTEC2](https://github.com/zhouguiheng/VirtualTEC2)过于古老，使用了大量Windows API。因此对其进行简单的修改，使其能够在Linux以及Mac上运行。

## 编译

```bash
mkdir build
cd build
cmake ..
make
```

## 运行

确保三个`.ROM`文件在`pwd`中。
