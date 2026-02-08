#!/bin/bash
# 主机配置脚本 (混合模式)

INTERFACE="enp5s0"
HOST_IP="192.168.2.30"
NETMASK="24"
HOST_MAC="54:43:41:00:00:30"

echo "=== 配置主机 (Hybrid Sender) ==="
if [ "$EUID" -ne 0 ]; then echo "请使用sudo运行"; exit 1; fi

# 加载802.1Q模块
echo "加载802.1Q内核模块..."
modprobe 8021q 2>/dev/null || true

# 1. 修改物理MAC
echo "设置物理MAC为 $HOST_MAC ..."
ip link set dev "$INTERFACE" down
ip link set dev "$INTERFACE" address "$HOST_MAC"
ip link set dev "$INTERFACE" up
sleep 1

# 2. 配置物理接口IP (用于普通UDP通信)
echo "配置物理IP $HOST_IP..."
ip addr flush dev "$INTERFACE"
ip addr add "${HOST_IP}/${NETMASK}" dev "$INTERFACE"

# 3. 清理旧的VLAN接口 (如果存在)
# 混合模式下，主机不需要操作系统层面的VLAN接口，程序会直接通过pcap发包
if ip link show "${INTERFACE}.100" > /dev/null 2>&1; then
    echo "清理旧的 VLAN 接口..."
    ip link delete "${INTERFACE}.100"
fi

echo "主机配置完成: 物理IP=$HOST_IP, MAC=$HOST_MAC"
echo "TSN数据将通过 Raw Socket 直接注入，普通数据通过物理口路由。"

# 显示配置结果
echo ""
echo "主机VLAN配置完成!"
echo "接口状态:"
ip link show "${INTERFACE}.${VLAN_ID}"
echo ""
echo "IP地址信息:"
ip addr show "${INTERFACE}.${VLAN_ID}"