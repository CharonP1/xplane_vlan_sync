#!/bin/bash
# build.sh

echo "X-Plane VLAN同步程序编译脚本"
echo "=============================="

# 创建构建目录
mkdir -p build

# 检查依赖
echo "检查依赖..."
if pkg-config --exists libpcap; then
    echo "✓ libpcap 已安装"
else
    echo "✗ libpcap 未安装"
    echo "请安装libpcap-dev: sudo apt-get install libpcap-dev"
    exit 1
fi

if which gcc > /dev/null; then
    echo "✓ gcc 已安装"
else
    echo "✗ gcc 未安装,请先安装gcc"
    exit 1
fi

echo ""

# 编译完整版本
echo "编译完整版本..."
gcc -g -D__USE_MISC=1 -Wall -Wextra -O0 -o build/xplane_sync_vlan src/main_vlan.c src/xplaneConnect.c -lpcap

if [ $? -eq 0 ]; then
    echo "✓ 编译成功: build/xplane_sync_vlan"
    echo ""
    echo "使用说明:"
    echo "  运行程序: sudo ./build/xplane_sync_vlan"
    echo "  确保网络接口支持VLAN和原始套接字"
else
    echo "✗ 编译失败"
    exit 1
fi

echo ""
echo "编译完成!"