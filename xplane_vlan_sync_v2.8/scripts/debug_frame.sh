#!/bin/bash
# 调试帧结构脚本

echo "=== VLAN帧结构调试 ==="
echo "检查系统字节序:"
echo -n "系统字节序: "
if [ $(echo -n I | od -to2 | head -n1 | cut -f2 -d" " | cut -c6) = "1" ]; then
    echo "Little Endian"
else
    echo "Big Endian"
fi

echo ""
echo "检查结构体大小:"
echo "运行以下命令编译测试程序:"
echo "gcc -o build/debug_frame src/debug_frame.c -lpcap"
echo "然后运行: ./build/debug_frame"