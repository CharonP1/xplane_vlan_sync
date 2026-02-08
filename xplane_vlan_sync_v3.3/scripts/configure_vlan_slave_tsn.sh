#!/bin/bash
# TSN从机配置脚本 (VLAN 100)

INTERFACE="enp5s0"
VLAN_ID=100
SLAVE_IP="192.168.2.40"
NETMASK="24"
SLAVE_MAC="54:43:41:00:00:40"

echo "=== 配置TSN从机 (Nd-40) ==="
if [ "$EUID" -ne 0 ]; then echo "请使用sudo运行"; exit 1; fi

modprobe 8021q 2>/dev/null || true

# 1. 修改物理MAC
ip link set dev "$INTERFACE" down
ip link set dev "$INTERFACE" address "$SLAVE_MAC"
ip link set dev "$INTERFACE" up
sleep 1

# 2. 配置VLAN接口
if ip link show "${INTERFACE}.${VLAN_ID}" > /dev/null 2>&1; then
    ip link delete "${INTERFACE}.${VLAN_ID}"
fi
ip link add link "$INTERFACE" name "${INTERFACE}.${VLAN_ID}" type vlan id "$VLAN_ID"
ip link set "${INTERFACE}.${VLAN_ID}" up
ip addr flush dev "${INTERFACE}.${VLAN_ID}"
ip addr add "${SLAVE_IP}/${NETMASK}" dev "${INTERFACE}.${VLAN_ID}"

echo "TSN从机配置完成: IP=$SLAVE_IP (VLAN $VLAN_ID)"