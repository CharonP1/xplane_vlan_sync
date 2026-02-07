#!/bin/bash
# 检查X-Plane绑定状态

echo "检查X-Plane网络绑定状态..."
echo "============================"

# 检查X-Plane进程
echo "1. 检查X-Plane进程..."
ps aux | grep -i x-plane | grep -v grep

# 检查网络监听状态
echo ""
echo "2. 检查网络监听状态..."
netstat -tulpn | grep -E "(49009|X-Plane)"

# 检查所有接口的IP地址
echo ""
echo "3. 检查网络接口..."
ip addr show | grep -E "(enp5s0|192.168.2)"

# 检查路由表
echo ""
echo "4. 检查路由表..."
ip route show | grep 192.168.2

# 建议
echo ""
echo "5. 建议的X-Plane网络设置:"
echo "   - 在X-Plane中: Settings → Network"
echo "   - 确保'Receive on IP'设置为: 0.0.0.0 (所有接口)"
echo "   - 或设置为VLAN接口IP: 192.168.2.40"
echo "   - 端口: 49009"
echo ""
echo "6. 如果X-Plane绑定到特定IP，可能需要重启X-Plane"