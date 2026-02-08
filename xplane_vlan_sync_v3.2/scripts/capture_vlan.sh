#!/bin/bash
# 抓包脚本 - 在接收端运行

INTERFACE="enp5s0"        # 监听的网络接口
VLAN_ID=100             # 要捕获的VLAN ID
OUTPUT_FILE="vlan_capture_$(date +%Y%m%d_%H%M%S).pcap"

echo "VLAN数据包捕获脚本"
echo "=================="
echo "接口: $INTERFACE"
echo "VLAN ID: $VLAN_ID"
echo "输出文件: $OUTPUT_FILE"
echo ""

# 检查tcpdump是否安装
if ! command -v tcpdump &> /dev/null; then
    echo "错误: 未找到tcpdump"
    echo "安装tcpdump: sudo apt-get install tcpdump"
    exit 1
fi

# 检查root权限
if [ "$EUID" -ne 0 ]; then
    echo "请使用root权限运行此脚本: sudo $0"
    exit 1
fi

echo "开始捕获VLAN $VLAN_ID 的数据包..."
echo "按Ctrl+C停止捕获"
echo ""

# 捕获VLAN数据包
tcpdump -i $INTERFACE -nn -e -v -w $OUTPUT_FILE vlan $VLAN_ID

echo ""
echo "捕获完成! 数据包已保存到: $OUTPUT_FILE"
echo "可以使用以下命令分析数据包:"
echo "  tcpdump -nn -e -v -r $OUTPUT_FILE"
echo "  或"
echo "  wireshark $OUTPUT_FILE"