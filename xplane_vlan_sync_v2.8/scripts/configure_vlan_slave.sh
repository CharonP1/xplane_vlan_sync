#!/bin/bash
# 配置从机VLAN接口让X-Plane直接接收

INTERFACE="enp5s0"              # 物理网络接口
VLAN_ID=100                   # VLAN ID（必须与发送端一致）
VLAN_IP="192.168.2.40"        # 从机VLAN接口IP
NETMASK="24"                  # 子网掩码

echo "配置从机VLAN接口(X-Plane直接接收模式)..."
echo "网络接口: $INTERFACE"
echo "VLAN ID: $VLAN_ID"
echo "VLAN IP: $VLAN_IP/$NETMASK"

# 检查root权限
if [ "$EUID" -ne 0 ]; then
    echo "请使用root权限运行此脚本: sudo $0"
    exit 1
fi

# 加载802.1Q模块
echo "加载802.1Q内核模块..."
modprobe 8021q 2>/dev/null || true

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

# 清理冲突路由
echo "清理冲突路由..."
ip route del "192.168.2.0/24" dev "${INTERFACE}.${VLAN_ID}" 2>/dev/null || true

# 配置路由
echo "配置路由..."
ip route add "192.168.2.0/24" dev "${INTERFACE}.${VLAN_ID}"

# 显示配置结果
echo ""
echo "VLAN配置完成!"
echo "接口状态:"
ip link show "${INTERFACE}.${VLAN_ID}"
echo ""
echo "IP地址信息:"
ip addr show "${INTERFACE}.${VLAN_ID}"
echo ""
echo "路由信息:"
ip route show

echo ""
echo "重要提示:"
echo "1. 确保X-Plane插件监听在VLAN接口IP: $VLAN_IP"
echo "2. 发送端的目的IP必须设置为: $VLAN_IP"
echo "3. 发送端的VLAN ID必须设置为: $VLAN_ID"