# X-Plane 多机 TSN 同步演示工程 Makefile
# 支持角色：主机(Sender), TSN从机(Receiver-TSN), 普通从机(Receiver-STD)

# ==================== 变量定义 ====================
# 编译器设置
CC = gcc
CFLAGS = -Wall -O2 -I./src
LDFLAGS = -lpcap -lpthread

# 目录设置
BUILD_DIR = build
SRC_DIR = src
SCRIPT_DIR = scripts

# 目标文件
TARGET = $(BUILD_DIR)/xplane_sync_vlan
OBJS = $(BUILD_DIR)/main_vlan.o $(BUILD_DIR)/xplaneConnect.o

# 网络接口定义 (用于清理命令)
IFACE_NAME = enp5s0
VLAN_ID = 100

.PHONY: all build clean install-deps \
		setup-host setup-slave-tsn \
		setup-slave-std run-sender help

# 默认目标
all: build

# ==================== 编译构建 ====================

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

build: $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo ">>> 编译完成: $(TARGET)"

clean:
	rm -rf $(BUILD_DIR)
	@echo ">>> 编译文件清理完成"

# ==================== 环境配置 (分角色) ====================

# 1. 安装系统依赖 (所有机器需执行一次)
install-deps:
	@echo ">>> 安装编译依赖..."
	sudo apt-get update
	sudo apt-get install -y build-essential libpcap-dev net-tools

# 2. 配置主机 (Sender - Nd-30)
setup-host:
	@echo "配置主机 (Hybrid Sender)..."
	chmod +x $(SCRIPT_DIR)/configure_vlan_host.sh
	sudo $(SCRIPT_DIR)/configure_vlan_host.sh

setup-slave-tsn:
	@echo "配置 TSN 从机 (VLAN 100)..."
	chmod +x $(SCRIPT_DIR)/configure_vlan_slave_tsn.sh
	sudo $(SCRIPT_DIR)/configure_vlan_slave_tsn.sh

setup-slave-std:
	@echo "配置 普通 从机 (Standard Ethernet)..."
	chmod +x $(SCRIPT_DIR)/configure_slave_std.sh
	sudo $(SCRIPT_DIR)/configure_slave_std.sh

# ==================== 网络清理 (新增) ====================

# 移除VLAN接口配置
remove-vlan:
	@echo ">>> 正在移除 VLAN 接口..."
	# 使用 - 前缀忽略错误（例如接口不存在时）
	-sudo ip link delete $(IFACE_NAME).$(VLAN_ID) 2>/dev/null || true
	@echo ">>> VLAN 接口 $(IFACE_NAME).$(VLAN_ID) 已移除 (物理接口保留)"

# ==================== 运行程序 ====================

# 在主机上运行发送程序
run-sender: build
	@echo ">>> 启动多播同步发送程序..."
	sudo $(TARGET)

# ==================== 辅助工具 ====================

# 抓取VLAN数据包
capture-vlan:
	chmod +x $(SCRIPT_DIR)/capture_vlan.sh
	sudo $(SCRIPT_DIR)/capture_vlan.sh

# 检查X-Plane绑定状态
check-xplane:
	chmod +x $(SCRIPT_DIR)/check_xplane_binding.sh
	sudo $(SCRIPT_DIR)/check_xplane_binding.sh

# 帮助信息
help:
	@echo "X-Plane TSN同步工程使用说明:"
	@echo "  make install-deps    - 安装系统依赖"
	@echo "  make build           - 编译程序"
	@echo ""
	@echo "  --- 环境部署 (选一个执行) ---"
	@echo "  make setup-host      - 部署为主机 (Sender)"
	@echo "  make setup-slave-tsn - 部署为TSN从机 (Nd-40)"
	@echo "  make setup-slave-std - 部署为普通从机 (Nd-10)"
	@echo ""
	@echo "  --- 清理与运行 ---"
	@echo "  make remove-vlan     - 删除 VLAN 接口配置"
	@echo "  make run-sender      - 运行发送程序 (主机)"
	@echo "  make clean           - 清理编译文件"
	@echo "  --- 调试 ---"
	@echo "  make capture-vlan    - 抓取VLAN数据包"
	@echo "  make clean           - 清理编译文件"
