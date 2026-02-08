#!/bin/bash
# 普通从机配置脚本 (Standard Ethernet)

INTERFACE="enp5s0"
SLAVE_IP="192.168.2.10"
NETMASK="24"
SLAVE_MAC="54:43:41:00:00:10"

echo "=== 配置普通从机 (Nd-10) ==="
if [ "$EUID" -ne 0 ]; then echo "请使用sudo运行"; exit 1; fi

# 1. 修改物理MAC
ip link set dev "$INTERFACE" down
ip link set dev "$INTERFACE" address "$SLAVE_MAC"
ip link set dev "$INTERFACE" up
sleep 1

# 2. 配置物理IP
echo "配置IP $SLAVE_IP..."
ip addr flush dev "$INTERFACE"
ip addr add "${SLAVE_IP}/${NETMASK}" dev "$INTERFACE"

# 3. 确保没有VLAN接口干扰
if ip link show "${INTERFACE}.100" > /dev/null 2>&1; then
    ip link delete "${INTERFACE}.100"
fi

# 4. 路由确保
ip route add "192.168.2.0/24" dev "$INTERFACE" 2>/dev/null || true

echo "普通从机配置完成。"