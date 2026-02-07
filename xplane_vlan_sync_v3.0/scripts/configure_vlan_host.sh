#!/bin/bash
# 主机VLAN配置脚本

INTERFACE="enp5s0"
VLAN_ID=100
VLAN_IP="192.168.2.30"
NETMASK="24"
# [新增] 主机MAC
HOST_MAC="54:43:41:00:00:30"

echo "=== 配置主机 (Sender) ==="
if [ "$EUID" -ne 0 ]; then echo "请使用sudo运行"; exit 1; fi

# 修改物理MAC
echo "设置物理MAC为 $HOST_MAC ..."
ip link set dev "$INTERFACE" down
ip link set dev "$INTERFACE" address "$HOST_MAC"
ip link set dev "$INTERFACE" up
sleep 1

# 加载802.1Q模块
echo "加载802.1Q内核模块..."
modprobe 8021q 2>/dev/null || true

# 检查网络接口是否存在
if ! ip link show "$INTERFACE" > /dev/null 2>&1; then
    echo "错误: 网络接口 $INTERFACE 不存在"
    echo "可用的网络接口:"
    ip link show | grep -E "^[0-9]+:" | awk -F: '{print $2}'
    exit 1
fi

# 删除已存在的VLAN接口（如果存在）
if ip link show "${INTERFACE}.${VLAN_ID}" > /dev/null 2>&1; then
    echo "删除已存在的VLAN接口 ${INTERFACE}.${VLAN_ID}..."
    ip link delete "${INTERFACE}.${VLAN_ID}" 2>/dev/null || true
    sleep 1
fi

# 创建VLAN接口
echo "创建VLAN接口 ${INTERFACE}.${VLAN_ID}..."
ip link add link "$INTERFACE" name "${INTERFACE}.${VLAN_ID}" type vlan id "$VLAN_ID"

# 启用VLAN接口
echo "启用VLAN接口..."
ip link set "${INTERFACE}.${VLAN_ID}" up

# 删除已存在的IP地址（如果存在）
echo "清理现有IP配置..."
ip addr flush dev "${INTERFACE}.${VLAN_ID}" 2>/dev/null || true

# 配置IP地址
echo "配置IP地址..."
ip addr add "${VLAN_IP}/${NETMASK}" dev "${INTERFACE}.${VLAN_ID}"

# 显示配置结果
echo ""
echo "主机VLAN配置完成!"
echo "接口状态:"
ip link show "${INTERFACE}.${VLAN_ID}"
echo ""
echo "IP地址信息:"
ip addr show "${INTERFACE}.${VLAN_ID}"