# Makefile 使用教程  
  
## 阶段1：环境准备  
在主机和从机上执行： 
     
    1. 安装依赖  
    make install-deps  
  
    2. 编译程序  
    make build  
  
    3. 清理并重新配置VLAN  
    make remove-vlan  
    make reconfigure-vlan  
  
## 阶段2：从机配置  
在从机上执行：  
  
    1. 启动X-Plane  
    -  确保X-Plane Connect插件已安装  
    -  启动X-Plane并加载任意飞机  
    -  检查X-Plane网络设置允许远程连接  
  
    2. (可选)启动数据包捕获  
    make capture-vlan  
  
## 阶段3：主机配置  
在主机上执行：  
  
    运行发送程序  
    make run-sender  

## 验证功能
在从机上执行 
  
    1. 验证vlan配置  
    make verify-vlan 

    2. 检查X-Plane配置  
    make check-xplane  

在主机上执行  
  
    1. 检查网络配置  
    make network-info  

    2. 测试连通性  
    make test-connectivity  
  
    3. 运行网络诊断
    make diagnose-network  
  
## 诊断命令  
    检查数据包是否到达X-Plane进程  
    sudo tcpdump -i lo -nn udp port 49009  
  
    检查X-Plane错误日志  
    在X-Plane中查看日志文件，通常位~/Desktop/X-Plane/Log.txt  
  
    检查系统日志  
    dmesg | tail -20  