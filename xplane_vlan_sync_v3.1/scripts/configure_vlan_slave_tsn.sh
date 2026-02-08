#!/bin/bash
# 配置TSN从机 (Nd-40)

INTERFACE="enp5s0"            # 物理网络接口
VLAN_ID=100                   # VLAN ID（必须与发送端一致）
VLAN_IP="192.168.2.40"        # 从机VLAN接口IP
NETMASK="24"                  # 子网掩码

# [修改] 修改物理MAC地址以匹配拓扑 (Nd-40)
TARGET_MAC="54:43:41:00:00:40"

echo "=== 配置TSN从机 (Nd-40) ==="
if [ "$EUID" -ne 0 ]; then echo "请使用sudo运行"; exit 1; fi

modprobe 8021q 2>/dev/null || true

# 修改物理MAC
echo "设置物理MAC为 $TARGET_MAC ..."
ip link set dev "$INTERFACE" down
ip link set dev "$INTERFACE" address "$TARGET_MAC"
ip link set dev "$INTERFACE" up
sleep 1

# 配置VLAN接口
if ip link show "${INTERFACE}.${VLAN_ID}" > /dev/null 2>&1; then
    ip link delete "${INTERFACE}.${VLAN_ID}"
fi

ip link add link "$INTERFACE" name "${INTERFACE}.${VLAN_ID}" type vlan id "$VLAN_ID"
ip link set "${INTERFACE}.${VLAN_ID}" up
ip addr flush dev "${INTERFACE}.${VLAN_ID}"
ip addr add "${VLAN_IP}/${NETMASK}" dev "${INTERFACE}.${VLAN_ID}"

# 路由设置
ip route del "192.168.2.0/24" dev "${INTERFACE}.${VLAN_ID}" 2>/dev/null || true
ip route add "192.168.2.0/24" dev "${INTERFACE}.${VLAN_ID}"

echo "TSN从机配置完成: $VLAN_IP ($TARGET_MAC)"