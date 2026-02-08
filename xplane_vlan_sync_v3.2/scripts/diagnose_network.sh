#!/bin/bash
# 网络诊断脚本

INTERFACE="enp5s0"
VLAN_ID=100
HOST_IP="192.168.2.30"
SLAVE_IP="192.168.2.40"

echo "=== 网络诊断 ==="
echo "接口: $INTERFACE, VLAN ID: $VLAN_ID"
echo "主机IP: $HOST_IP, 从机IP: $SLAVE_IP"
echo ""

# 检查物理连接
echo "1. 检查物理连接..."
ip link show "$INTERFACE"
echo ""

# 检查VLAN接口
echo "2. 检查VLAN接口..."
ip link show "${INTERFACE}.${VLAN_ID}" 2>/dev/null && echo "✓ VLAN接口存在" || echo "✗ VLAN接口不存在"
echo ""

# 检查IP配置
echo "3. 检查IP配置..."
ip addr show "${INTERFACE}.${VLAN_ID}" 2>/dev/null | grep "inet" && echo "✓ IP地址已配置" || echo "✗ IP地址未配置"
echo ""

# 检查ARP表
echo "4. 检查ARP表..."
arp -a | grep -E "($HOST_IP|$SLAVE_IP)" || echo "未找到相关ARP条目"
echo ""

# 测试连通性
echo "5. 测试连通性..."
ping -c 2 -W 1 "$SLAVE_IP" && echo "✓ 可以ping通从机" || echo "✗ 无法ping通从机"
echo ""

# 检查防火墙
echo "6. 检查防火墙状态..."
if command -v ufw >/dev/null 2>&1; then
    ufw status | grep -E "(49009|Status)"
else
    echo "未安装ufw,检查iptables..."
    iptables -L -n | grep -E "(49009|ACCEPT)" || echo "未找到相关规则"
fi
echo ""

echo "=== 诊断完成 ==="