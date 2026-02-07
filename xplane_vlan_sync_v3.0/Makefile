# X-Plane TSN同步演示工程 Makefile
# 支持角色：主机(Sender), TSN从机(Receiver-TSN), 普通从机(Receiver-STD)

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

.PHONY: all build clean install-deps run-sender setup-host setup-slave-tsn setup-slave-std help

# 默认目标
all: build

# ==================== 编译构建 ====================

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

build: $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "编译完成: $(TARGET)"

clean:
	rm -rf $(BUILD_DIR)
	@echo "清理完成"

# ==================== 环境配置 (分角色) ====================

# 1. 安装系统依赖 (所有机器需执行一次)
install-deps:
	@echo "安装编译依赖..."
	sudo apt-get update
	sudo apt-get install -y build-essential libpcap-dev net-tools

# 2. 配置主机 (Sender - Nd-30)
setup-host:
	@echo "配置主机网络环境 (VLAN 100, IP .30)..."
	chmod +x $(SCRIPT_DIR)/configure_vlan_host.sh
	sudo $(SCRIPT_DIR)/configure_vlan_host.sh

# 3. 配置TSN从机 (Receiver 1 - Nd-40)
setup-slave-tsn:
	@echo "配置TSN从机网络环境 (VLAN 100, IP .40)..."
	chmod +x $(SCRIPT_DIR)/configure_vlan_slave_tsn.sh
	sudo $(SCRIPT_DIR)/configure_vlan_slave_tsn.sh

# 4. 配置普通从机 (Receiver 2 - Nd-10)
setup-slave-std:
	@echo "配置普通从机网络环境 (VLAN 100, IP .10)..."
	chmod +x $(SCRIPT_DIR)/configure_vlan_slave_std.sh
	sudo $(SCRIPT_DIR)/configure_vlan_slave_std.sh

# ==================== 运行程序 ====================

# 在主机上运行发送程序
run-sender: build
	@echo "启动多播同步发送程序..."
	sudo $(TARGET)

# ==================== 辅助工具 ====================

# 抓取VLAN数据包 (用于调试)
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
	@echo "  make install-deps    - 安装系统依赖 (所有机器)"
	@echo "  make build           - 编译程序"
	@echo ""
	@echo "  --- 角色配置 ---"
	@echo "  make setup-host      - 配置主机 (Sender)"
	@echo "  make setup-slave-tsn - 配置TSN从机 (Receiver 1)"
	@echo "  make setup-slave-std - 配置普通从机 (Receiver 2)"
	@echo ""
	@echo "  --- 运行 ---"
	@echo "  make run-sender      - 启动发送程序 (仅主机)"
	@echo ""
	@echo "  --- 调试 ---"
	@echo "  make capture-vlan    - 抓取VLAN数据包"
	@echo "  make clean           - 清理编译文件"