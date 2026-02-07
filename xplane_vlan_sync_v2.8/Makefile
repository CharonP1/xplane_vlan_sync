# Makefile for X-Plane VLAN Sync Project

# 编译器设置
CC = gcc
CFLAGS = -g -D__USE_MISC=1 -Wall -Wextra -O0
LDFLAGS = -lpcap

# 目录设置
SRCDIR = src
BUILDDIR = build
SCRIPTSDIR = scripts
TARGET = $(BUILDDIR)/xplane_sync_vlan

# 源文件
SOURCES = $(SRCDIR)/main_vlan.c $(SRCDIR)/xplaneConnect.c
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

# 头文件
HEADERS = $(SRCDIR)/xplaneConnect.h $(SRCDIR)/pcap_types.h

# 网络配置
HOST_INTERFACE = enp5s0
SLAVE_INTERFACE = enp5s0
VLAN_ID = 100
HOST_IP = 192.168.2.30
SLAVE_IP = 192.168.2.40
NETMASK = 24

# 默认目标
all: build info

# 主目标
$(TARGET): $(OBJECTS)
	@echo "链接目标文件..."
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "编译完成: $(TARGET)"

# 编译对象文件
$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 构建程序
build: $(TARGET)

# 清理构建文件
clean:
	@echo "清理构建文件..."
	rm -rf $(BUILDDIR)
	@echo "清理完成"

# 完全清理（包括捕获文件）
distclean: clean
	@echo "清理捕获文件..."
	rm -f vlan_capture_*.pcap
	@echo "完全清理完成"

# 安装依赖 (Ubuntu/Debian)
install-deps:
	@echo "安装构建依赖..."
	sudo apt-get update
	sudo apt-get install -y libpcap-dev gcc make tcpdump
	@echo "依赖安装完成"

# ==================== 配置命令 ====================

# 配置主机VLAN接口
configure-host:
	@echo "配置主机VLAN接口..."
	sudo sh $(SCRIPTSDIR)/configure_vlan_host.sh

# 配置从机VLAN接口 (方案B关键步骤)
configure-slave:
	@echo "配置从机VLAN接口 (X-Plane直接接收模式)..."
	sudo sh $(SCRIPTSDIR)/configure_vlan_slave.sh

# 移除VLAN接口
remove-vlan:
	@echo "移除VLAN接口..."
	sudo ip link delete $(HOST_INTERFACE).$(VLAN_ID) 2>/dev/null || true
	sudo ip link delete $(SLAVE_INTERFACE).$(VLAN_ID) 2>/dev/null || true
	@echo "VLAN接口已移除"

# 配置主从VLAN
configure-vlan: remove-vlan configure-host configure-slave
	@echo "VLAN重新配置完成"
# ==================== 测试命令 ====================

# 测试网络连通性
test-connectivity:
	@echo "测试网络连通性..."
	@echo "从主机ping从机:"
	ping -c 3 $(SLAVE_IP) || echo "ping失败,请检查网络配置"
	@echo ""
	@echo "网络接口状态:"
	ip addr show $(HOST_INTERFACE) | grep "inet" || echo "接口未配置IP"

# 在从机捕获VLAN数据包
capture-vlan:
	@echo "在从机捕获VLAN数据包..."
	sudo sh $(SCRIPTSDIR)/capture_vlan.sh

# 运行发送程序
run-sender:
	@echo "运行VLAN发送程序..."
	sudo $(TARGET)

# ==================== 调试命令 ====================

# 显示网络信息
network-info:
	@echo "=== 网络信息 ==="
	@echo "主机接口: $(HOST_INTERFACE)"
	@echo "从机接口: $(SLAVE_INTERFACE)"
	@echo "VLAN ID: $(VLAN_ID)"
	@echo "主机IP: $(HOST_IP)"
	@echo "从机IP: $(SLAVE_IP)"
	@echo "================="
	@echo "主机MAC地址:"
	@ip link show $(HOST_INTERFACE) | grep "link/ether" || echo "无法获取MAC"
	@echo "从机MAC地址:"
	@ssh user@$(SLAVE_IP) "ip link show $(SLAVE_INTERFACE) | grep 'link/ether'" || echo "请手动获取从机MAC"

# 验证VLAN配置
verify-vlan:
	@echo "验证VLAN配置..."
	@echo "1. 检查VLAN接口:"
	@ip link show $(HOST_INTERFACE).$(VLAN_ID) >/dev/null 2>&1 && echo "✓ 主机VLAN接口存在" || echo "✗ 主机VLAN接口不存在"
	@echo "2. 检查IP配置:"
	@ip addr show $(HOST_INTERFACE).$(VLAN_ID) | grep "inet" >/dev/null 2>&1 && echo "✓ 主机VLAN IP已配置" || echo "✗ 主机VLAN IP未配置"
	@echo "3. 测试连通性:"
	@ping -c 1 -W 1 $(SLAVE_IP) >/dev/null 2>&1 && echo "✓ 可以ping通从机" || echo "✗ 无法ping通从机"

# 网络诊断
diagnose-network:
	@echo "运行网络诊断..."
	sudo sh $(SCRIPTSDIR)/diagnose_network.sh

# ==================== 调试帧命令 ====================

# 捕获并显示VLAN帧详情
capture-debug:
	@echo "捕获VLAN帧详情..."
	sudo tcpdump -i enp5s0 -nn -e -v -c 10 vlan 100


# 检查X-Plane绑定
check-xplane-binding:
	@echo "检查X-Plane绑定状态..."
	sudo sh $(SCRIPTSDIR)/check_xplane_binding.sh


# ==================== 显示帮助信息 ====================

# 显示项目信息
info:
	@echo "=== X-Plane VLAN同步项目信息 ==="
	@echo "编译目标: $(TARGET)"
	@echo "源文件: $(SOURCES)"
	@echo "网络配置:"
	@echo "  主机: $(HOST_IP) (接口: $(HOST_INTERFACE).$(VLAN_ID))"
	@echo "  从机: $(SLAVE_IP) (接口: $(SLAVE_INTERFACE).$(VLAN_ID))"
	@echo "VLAN ID: $(VLAN_ID)"
	@echo "================================"

# 显示帮助信息
help:
	@echo "X-Plane VLAN Sync 项目 Makefile 命令"
	@echo ""
	@echo "构建命令:"
	@echo "  all           - 编译项目并显示信息 (默认)"
	@echo "  build         - 仅编译项目"
	@echo "  clean         - 清理构建文件"
	@echo "  distclean     - 完全清理（包括捕获文件）"
	@echo "  install-deps  - 安装构建依赖"
	@echo ""
	@echo "VLAN配置命令:"
	@echo "  configure-host - 配置主机VLAN接口"
	@echo "  configure-slave - 配置从机VLAN接口"
	@echo "  configure-vlan - 配置从机VLAN接口"
	@echo "  remove-vlan    - 移除VLAN接口"
	@echo ""
	@echo "测试命令:"
	@echo "  test-connectivity - 测试网络连通性"
	@echo "  capture-vlan     - 在从机捕获VLAN数据包"
	@echo "  run-sender       - 运行发送程序"
	@echo ""
	@echo "调试命令:"
	@echo "  network-info    - 显示网络信息"
	@echo "  check-xplane    - 检查X-Plane监听状态"
	@echo "  verify-vlan     - 验证VLAN配置"
	@echo "  info            - 显示项目信息"
	@echo "  help            - 显示此帮助信息"
	@echo ""
	@echo "  1. 在两台机器上运行 'make install-deps' 安装依赖"
	@echo "  2. 在从机运行 'make configure-vlan' 配置VLAN接口"
	@echo "  3. 在从机启动X-Plane并确保插件加载"
	@echo "  4. 在主机运行 'make build' 编译程序"
	@echo "  5. 在主机运行 'make configure-vlan' 配置VLAN接口"
	@echo "  6. 在主机运行 'make run-sender' 开始发送数据"
	@echo "  7. 观察从机X-Plane是否同步显示飞行状态"

# 伪目标
.PHONY: all build clean distclean install-deps \
        configure-host configure-slave remove-vlan configure_vlan\
        test-connectivity capture-vlan run-sender \
        network-info check-xplane verify-vlan \
        info help