# X-Plane 多机 TSN 同步演示工程 Makefile
# 修复了 build 目录与 build 目标的循环依赖问题

# ==================== 变量定义 ====================
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

# 网络接口定义
IFACE_NAME = enp5s0
VLAN_ID = 100

.PHONY: all build clean install-deps \
        setup-host setup-slave-tsn setup-slave-std \
        run-sender remove-vlan help

# 默认目标
all: build

# ==================== 编译构建 ====================

# [修改点]：移除了 $(BUILD_DIR) 的单独规则，避免与 build 目标冲突
# 在编译 .o 文件的规则中加入 @mkdir -p $(dir $@) 自动创建目录

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build: $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo ">>> 编译完成: $(TARGET)"

clean:
	rm -rf $(BUILD_DIR)
	@echo ">>> 编译文件清理完成"

# ==================== 环境配置 (分角色) ====================

install-deps:
	@echo ">>> 安装编译依赖..."
	sudo apt-get update
	sudo apt-get install -y build-essential libpcap-dev net-tools

# 配置主机
setup-host:
	@echo ">>> 配置主机网络环境..."
	chmod +x $(SCRIPT_DIR)/configure_vlan_host.sh
	sudo $(SCRIPT_DIR)/configure_vlan_host.sh

# 配置TSN从机
setup-slave-tsn:
	@echo ">>> 配置TSN从机网络环境..."
	chmod +x $(SCRIPT_DIR)/configure_vlan_slave_tsn.sh
	sudo $(SCRIPT_DIR)/configure_vlan_slave_tsn.sh

# 配置普通从机
setup-slave-std:
	@echo ">>> 配置普通从机网络环境..."
	chmod +x $(SCRIPT_DIR)/configure_vlan_slave_std.sh
	sudo $(SCRIPT_DIR)/configure_vlan_slave_std.sh

# ==================== 网络清理 ====================

remove-vlan:
	@echo ">>> 正在移除 VLAN 接口..."
	-sudo ip link delete $(IFACE_NAME).$(VLAN_ID) 2>/dev/null || true
	@echo ">>> VLAN 接口清理完成"

# ==================== 运行程序 ====================

run-sender: build
	@echo ">>> 启动发送程序..."
	sudo $(TARGET)

# ==================== 辅助工具 ====================

capture-vlan:
	chmod +x $(SCRIPT_DIR)/capture_vlan.sh
	sudo $(SCRIPT_DIR)/capture_vlan.sh

check-xplane:
	chmod +x $(SCRIPT_DIR)/check_xplane_binding.sh
	sudo $(SCRIPT_DIR)/check_xplane_binding.sh

help:
	@echo "X-Plane TSN同步工程使用说明:"
	@echo "  make build           - 编译程序"
	@echo "  make setup-host      - 部署为主机"
	@echo "  make run-sender      - 运行程序"
	@echo "  make clean           - 清理"