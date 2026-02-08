// main_vlan.c - 修复版本（固定源端口）
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pcap_types.h"
#include <pcap/pcap.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include "xplaneConnect.h"

#ifdef WIN32
#include <Windows.h>
#define sleep(n) Sleep(n * 1000)
#else
#include <unistd.h>
#include <arpa/inet.h>
#endif

// 网络配置结构体
typedef struct
{
    // 网络接口
    char interface[16];

    // MAC地址
    u_char src_mac[6];
    u_char dst_mac[6];

    // IP地址
    char src_ip[16];
    char dst_ip[16];

    // VLAN配置
    uint16_t vlan_id;
    uint8_t pcp;

    // 端口配置
    unsigned short src_port;
    unsigned short dst_port;
    
    // 动态字段
    uint16_t ip_identification;
} NetworkConfig;

// 全局pcap句柄
pcap_t *pcap_handle = NULL;

// 毫秒级休眠函数
void msleep(int milliseconds)
{
#ifdef WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

// 计算IP校验和（修复字节序问题）
uint16_t checksum(uint16_t *addr, int len)
{
    int nleft = len;
    uint32_t sum = 0;
    uint16_t *w = addr;
    uint16_t answer = 0;

    while (nleft > 1)
    {
        sum += ntohs(*w++);  // 修复：使用ntohs确保正确的字节序
        nleft -= 2;
    }

    if (nleft == 1)
    {
        *(unsigned char *)(&answer) = *(unsigned char *)w;
        sum += ntohs(answer);  // 修复：使用ntohs确保正确的字节序
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return htons(answer);  // 修复：返回时转换为网络字节序
}

// 计算UDP伪头部校验和
uint16_t udp_checksum(struct ip *iph, struct udphdr *udph, const char *payload, int payload_len)
{
    char pseudogram[65536];
    int psize = 0;
    
    // 伪头部
    struct pseudohdr {
        uint32_t src;
        uint32_t dst;
        uint8_t zero;
        uint8_t protocol;
        uint16_t length;
    } ph;
    
    ph.src = iph->ip_src.s_addr;
    ph.dst = iph->ip_dst.s_addr;
    ph.zero = 0;
    ph.protocol = IPPROTO_UDP;
    ph.length = htons(sizeof(struct udphdr) + payload_len);
    
    memcpy(pseudogram, &ph, sizeof(ph));
    psize += sizeof(ph);
    
    // UDP头部
    struct udphdr udp_temp = *udph;
    udp_temp.check = 0; // 校验和计算时设为0
    memcpy(pseudogram + psize, &udp_temp, sizeof(udp_temp));
    psize += sizeof(udp_temp);
    
    // 载荷数据
    memcpy(pseudogram + psize, payload, payload_len);
    psize += payload_len;
    
    // 如果需要填充
    if (psize % 2 != 0) {
        pseudogram[psize] = 0;
        psize++;
    }
    
    return checksum((uint16_t *)pseudogram, psize);
}

// 获取一个随机的动态端口（在程序启动时调用一次）
unsigned short get_dynamic_port() {
    // 使用时间戳作为随机种子
    srand((unsigned int)time(NULL));
    // 动态端口范围：49152 到 65535
    unsigned short port = (unsigned short)(49152 + (rand() % (65535 - 49152 + 1)));
    printf("分配动态源端口: %d\n", port);
    return port;
}

// 初始化pcap发送
int init_pcap_sender(const char *interface, NetworkConfig *net_config, pcap_t **handle)
{
    char errbuf[PCAP_ERRBUF_SIZE];

    printf("打开网络接口: %s\n", interface);
    printf("配置: 源MAC %02x:%02x:%02x:%02x:%02x:%02x, 目的MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           net_config->src_mac[0], net_config->src_mac[1], net_config->src_mac[2],
           net_config->src_mac[3], net_config->src_mac[4], net_config->src_mac[5],
           net_config->dst_mac[0], net_config->dst_mac[1], net_config->dst_mac[2],
           net_config->dst_mac[3], net_config->dst_mac[4], net_config->dst_mac[5]);

    *handle = pcap_open_live(interface, BUFSIZ, 1, 1000, errbuf);
    if (*handle == NULL)
    {
        printf("无法打开接口 %s: %s\n", interface, errbuf);
        return -1;
    }

    // 初始化动态字段
    net_config->ip_identification = (uint16_t)(time(NULL) & 0xFFFF); // 基于时间的初始ID
    
    // 在程序启动时分配固定的动态源端口
    if (net_config->src_port == 49009) { // 如果还是默认端口，则分配动态端口
        net_config->src_port = get_dynamic_port();
    }

    struct bpf_program fp;
    char filter_exp[256];
    snprintf(filter_exp, sizeof(filter_exp), "vlan and dst host %s and dst port %d",
             net_config->dst_ip, net_config->dst_port);

    if (pcap_compile(*handle, &fp, filter_exp, 0, PCAP_NETMASK_UNKNOWN) == -1)
    {
        printf("无法编译过滤器: %s\n", pcap_geterr(*handle));
    }
    else
    {
        if (pcap_setfilter(*handle, &fp) == -1)
        {
            printf("无法设置过滤器: %s\n", pcap_geterr(*handle));
        }
        pcap_freecode(&fp);
    }

    printf("✓ 网络接口初始化成功\n");
    return 0;
}

// 发送带VLAN标签的帧 - 修复版本（固定源端口）
int send_vlan_frame(pcap_t *handle, NetworkConfig *net_config,
                    uint8_t pcp, const char *payload, int payload_len)
{
    unsigned char frame[1600];
    int frame_pos = 0;              //单位 byte
    
    // 清空帧
    memset(frame, 0, sizeof(frame));
    
    // 1. 目的MAC地址 (6字节)
    memcpy(frame + frame_pos, net_config->dst_mac, 6);
    frame_pos += 6;
    
    // 2. 源MAC地址 (6字节)
    memcpy(frame + frame_pos, net_config->src_mac, 6);
    frame_pos += 6;
    
    // 3. 以太网类型 - VLAN (0x8100)
    frame[frame_pos++] = 0x81;
    frame[frame_pos++] = 0x00;
    
    // 4. VLAN TCI (PCP + DEI + VID)
    uint16_t tci = ((pcp & 0x07) << 13) |         // PCP优先级 (3 bits)
                   ((0 & 0x01) << 12) |           // DEI/CFI标志 (1 bit)
                   (net_config->vlan_id & 0x0FFF); // VLAN ID (12 bits)
    frame[frame_pos++] = (tci >> 8) & 0xFF;       // 高字节
    frame[frame_pos++] = tci & 0xFF;              // 低字节
    
    // 5. 封装类型 - IP (0x0800)
    frame[frame_pos++] = 0x08;
    frame[frame_pos++] = 0x00;
    
    // 6. IP头部开始
    int ip_start = frame_pos;
    struct ip *iph = (struct ip *)(frame + ip_start);
    
    // IP版本和头部长度 (5 * 4 = 20字节)
    frame[frame_pos++] = 0x45;
    // 服务类型
    frame[frame_pos++] = 0x00;
    // 总长度 (IP头 + UDP头 + 数据)
    uint16_t total_ip_len = 20 + 8 + payload_len;
    frame[frame_pos++] = (total_ip_len >> 8) & 0xFF;
    frame[frame_pos++] = total_ip_len & 0xFF;
    // 标识 - 动态递增
    frame[frame_pos++] = (net_config->ip_identification >> 8) & 0xFF;
    frame[frame_pos++] = net_config->ip_identification & 0xFF;
    net_config->ip_identification++; // 为下一个包递增
    // 分片偏移
    frame[frame_pos++] = 0x40;
    frame[frame_pos++] = 0x00;
    // TTL
    frame[frame_pos++] = 64;
    // 协议 (UDP)
    frame[frame_pos++] = IPPROTO_UDP;
    // IP校验和 (先设为0)
    frame[frame_pos++] = 0x00;
    frame[frame_pos++] = 0x00;
    
    // 源IP地址
    struct in_addr src_addr;
    inet_pton(AF_INET, net_config->src_ip, &src_addr);
    memcpy(frame + frame_pos, &src_addr, 4);
    frame_pos += 4;
    
    // 目的IP地址
    struct in_addr dst_addr;
    inet_pton(AF_INET, net_config->dst_ip, &dst_addr);
    memcpy(frame + frame_pos, &dst_addr, 4);
    frame_pos += 4;
    
    // 计算IP校验和
    uint16_t ip_checksum = checksum((uint16_t *)(frame + ip_start), 20);
    frame[ip_start + 10] = ip_checksum & 0xFF;
    frame[ip_start + 11] = (ip_checksum >> 8) & 0xFF;
    
    // 7. UDP头部
    int udp_start = frame_pos;
    struct udphdr *udph = (struct udphdr *)(frame + udp_start);
    
    // 源端口 (固定动态端口)
    frame[frame_pos++] = (net_config->src_port >> 8) & 0xFF;
    frame[frame_pos++] = net_config->src_port & 0xFF;
    // 目的端口
    frame[frame_pos++] = (net_config->dst_port >> 8) & 0xFF;
    frame[frame_pos++] = net_config->dst_port & 0xFF;
    // UDP长度
    uint16_t udp_len = 8 + payload_len;
    frame[frame_pos++] = (udp_len >> 8) & 0xFF;
    frame[frame_pos++] = udp_len & 0xFF;
    // UDP校验和 (先设为0，稍后计算)
    frame[frame_pos++] = 0x00;
    frame[frame_pos++] = 0x00;
    
    // 8. 载荷数据
    memcpy(frame + frame_pos, payload, payload_len);
    frame_pos += payload_len;
    
    int total_len = frame_pos;
    
    // 计算UDP校验和
    udph->check = 0;
    uint16_t udp_checksum_val = udp_checksum(iph, udph, payload, payload_len);
    frame[udp_start + 6] = udp_checksum_val & 0xFF;
    frame[udp_start + 7] = (udp_checksum_val >> 8) & 0xFF;
    
    // 调试输出
    printf("  发送VLAN帧: ID=%d, 源端口=%d, 长度=%d\n", 
           net_config->ip_identification - 1, net_config->src_port, total_len);
    
    // 发送帧
    if (pcap_sendpacket(handle, frame, total_len) != 0) {
        printf("  错误: 发送VLAN帧失败: %s\n", pcap_geterr(handle));
        return -1;
    }
    printf("  ✓ VLAN帧发送成功\n");

    return 0;
}

// 使用VLAN发送POSI数据（完整XPC协议格式）
int sendPOSI_vlan(pcap_t *handle, NetworkConfig *net_config,
                  float values[], int size, char ac, uint8_t pcp)
{
    // 根据XPC协议，POSI数据包格式：
    // [0-3] "POSI"
    // [4]   保留字节 (0)
    // [5]   飞机编号 (0-20)
    // [6-33] 7个float值 (每个4字节)
    unsigned char buffer[34] = {0};

    // 填充命令头
    memcpy(buffer, "POSI", 4);
    buffer[4] = 0;  // 保留字节
    buffer[5] = ac; // 飞机编号

    // 填充位置数据
    for (int i = 0; i < 7; i++)
    {
        float val = -998.0f; // 默认值，表示不修改该字段
        if (i < size)
        {
            val = values[i];
        }
        memcpy(buffer + 6 + i * 4, &val, sizeof(float));
    }

    printf("发送POSI数据: 飞机%d -> %s, VLAN ID=%d, PCP=%d\n",
           ac, net_config->dst_ip, net_config->vlan_id, pcp);
    printf("  位置(%.5f,%.5f,%.1f) 姿态(%.1f,%.1f,%.1f) 起落架%.0f\n",
           values[0], values[1], values[2], values[3], values[4], values[5], values[6]);

    return send_vlan_frame(handle, net_config, pcp, (char *)buffer, 34);
}

// 使用VLAN发送CTRL数据（完整XPC协议格式）
int sendCTRL_vlan(pcap_t *handle, NetworkConfig *net_config,
                  float values[], int size, char ac, uint8_t pcp)
{
    // 根据XPC协议，CTRL数据包格式：
    // [0-3] "CTRL"
    // [4]   保留字节 (0)
    // [5-8]  elevator (float)
    // [9-12] aileron (float)
    // [13-16] rudder (float)
    // [17-20] throttle (float)
    // [21]   gear (byte, 0-1)
    // [22-25] flaps (float)
    // [26]   飞机编号 (0-20)
    // [27-30] speedbrakes (float)
    unsigned char buffer[31] = {0};

    // 填充命令头
    memcpy(buffer, "CTRL", 4);
    buffer[4] = 0; // 保留字节

    // 填充控制面数据
    int cur = 5;
    for (int i = 0; i < 4; i++)
    { // elevator, aileron, rudder, throttle
        float val = (i < size) ? values[i] : -998.0f;
        memcpy(buffer + cur, &val, sizeof(float));
        cur += sizeof(float);
    }

    // gear (byte)
    buffer[cur++] = (size > 4 && values[4] != -998.0f) ? (unsigned char)values[4] : 0xFF;

    // flaps (float)
    float flaps_val = (size > 5) ? values[5] : -998.0f;
    memcpy(buffer + cur, &flaps_val, sizeof(float));
    cur += sizeof(float);

    // 飞机编号
    buffer[cur++] = ac;

    // speedbrakes (float)
    float sb_val = (size > 6) ? values[6] : -998.0f;
    memcpy(buffer + cur, &sb_val, sizeof(float));

    printf("发送CTRL数据: 飞机%d -> %s, VLAN ID=%d, PCP=%d\n",
           ac, net_config->dst_ip, net_config->vlan_id, pcp);
    printf("  控制面(升降舵%.2f, 副翼%.2f, 方向舵%.2f) 油门%.2f\n",
           values[0], values[1], values[2], values[3]);

    return send_vlan_frame(handle, net_config, pcp, (char *)buffer, 31);
}

// 使用VLAN发送DREF数据（完整XPC协议格式）
int sendDREFs_vlan(pcap_t *handle, NetworkConfig *net_config,
                   const char *drefs[], float *values[], int sizes[], int count,
                   uint8_t pcp)
{
    // 根据XPC协议，DREF数据包格式：
    // [0-3] "DREF"
    // [4]   保留字节 (0)
    // 然后对于每个DREF:
    //   [n]     DREF名称长度 (byte)
    //   [n+1..] DREF名称 (变长)
    //   [m]     值数量 (byte)
    //   [m+1..] 浮点数值数组 (变长)
    unsigned char buffer[65536] = {0};

    // 填充命令头
    memcpy(buffer, "DREF", 4);
    buffer[4] = 0; // 保留字节

    int pos = 5;

    for (int i = 0; i < count; ++i)
    {
        // 检查缓冲区边界
        if (pos + 2 + strlen(drefs[i]) + sizes[i] * sizeof(float) > 65536)
        {
            printf("错误: DREF数据包超过最大长度\n");
            return -1;
        }

        // DREF名称长度和名称
        int drefLen = (int)strlen(drefs[i]);
        if (drefLen > 255)
        {
            printf("警告: DREF名称过长,将被截断: %s\n", drefs[i]);
            drefLen = 255;
        }

        buffer[pos++] = (unsigned char)drefLen;
        memcpy(buffer + pos, drefs[i], drefLen);
        pos += drefLen;

        // 值数量
        if (sizes[i] > 255)
        {
            printf("警告: DREF值数量过多,将被截断\n");
            buffer[pos++] = 255;
        }
        else
        {
            buffer[pos++] = (unsigned char)sizes[i];
        }

        // 浮点数值
        memcpy(buffer + pos, values[i], sizes[i] * sizeof(float));
        pos += sizes[i] * sizeof(float);
    }

    printf("发送DREF数据: -> %s, VLAN ID=%d, PCP=%d, %d个数据参考\n",
           net_config->dst_ip, net_config->vlan_id, pcp, count);

    return send_vlan_frame(handle, net_config, pcp, (char *)buffer, pos);
}

// 使用VLAN发送单个DREF数据
int sendDREF_vlan(pcap_t *handle, NetworkConfig *net_config,
                  const char *dref, float value[], int size,
                  uint8_t pcp)
{
    const char *drefs[1] = {dref};
    float *values[1] = {value};
    int sizes[1] = {size};

    return sendDREFs_vlan(handle, net_config, drefs, values, sizes, 1, pcp);
}

int main(int argc, char *argv[])
{
    printf("X-Plane多机同步程序(TSN对比演示版)\n");
    printf("==========================================\n");

    // ==================== 数据优先级定义 ====================
    #define PCP_POSI 5 // POSI数据优先级 (高)
    #define PCP_CTRL 5 // CTRL数据优先级 (高)
    #define PCP_DREF 3 // DREF数据优先级 (中)
    #define PCP_TEXT 1 // 文本数据优先级 (低)

    // ==================== 网络配置 ====================
    NetworkConfig config_tsn; // 从机1 (TSN路径)

    // 1. 公共配置 (接口与源地址)
    const char *IFACE_NAME = "enp5s0";
    u_char MAC_HOST[6] = {0x54, 0x43, 0x41, 0x00, 0x00, 0x30};      // 主机 MAC
    u_char MAC_SLAVE1[6] = {0x54, 0x43, 0x41, 0x00, 0x00, 0x40};    // 从机1 MAC 54:43:41:00:00:40
    char *IP_HOST = "192.168.2.30";                                 // 主机 IP
    char *IP_SLAVE1 = "192.168.2.40";                               // 从机1 IP
    char *IP_SLAVE2 = "192.168.2.10";                               // 从机2 IP
    uint16_t VLAN_ID = 100;    

    // 2. 从机1配置 (TSN - Nd-40)
    strncpy(config_tsn.interface, IFACE_NAME, 16);
    memcpy(config_tsn.src_mac, MAC_HOST, 6);
    memcpy(config_tsn.dst_mac, MAC_SLAVE1, 6);
    strncpy(config_tsn.src_ip, IP_HOST, 16);        // 源IP地址
    strncpy(config_tsn.dst_ip, IP_SLAVE1, 16);      // 目的IP地址
    config_tsn.vlan_id = VLAN_ID;                   // VLAN ID
    config_tsn.src_port = 49009;                    // 初始默认值，将在初始化时被替换
    config_tsn.dst_port = 49009;                    // X-Plane默认端口

    // ==================== 显示配置信息 ====================
    printf("\n[TSN通道] 目标: %s (MAC: ...40), PCP: 高\n", config_tsn.dst_ip);

    // ==================== 网络初始化 ====================
    pcap_t *pcap_handle = NULL;

    // 初始化 TSN 发送通道 Raw Socket)
    if (init_pcap_sender(config_tsn.interface, &config_tsn, &pcap_handle) < 0)
    {
        printf("pcap初始化失败\n");
        return EXIT_FAILURE;
    }
    // 初始化 普通 发送通道 Standard UDP)
    // 使用 XPC 库原有的 socket 连接方式连接到从机2
    // 注意：aopenUDP 的第三个参数通常设为 0
    printf("初始化普通UDP通道 -> %s:49009\n", IP_SLAVE2);
    XPCSocket sockSlaveStd = aopenUDP(IP_SLAVE2, 49009, 0);    

    // ==================== X-Plane连接 ====================
    // 连接到主机A (使用XPC库)
    XPCSocket sockHost = aopenUDP(IP_HOST, 49009, 0);

    // 验证连接
    printf("验证连接到主机A (%s)...\n", config_tsn.src_ip);
    float tVal[1];
    int tSize = 1;
    if (getDREF(sockHost, "sim/test/test_float", tVal, &tSize) < 0)
    {
        printf("错误: 无法连接到主机A %s\n", config_tsn.src_ip);
        return EXIT_FAILURE;
    }
    printf("✓ 成功连接到主机A\n");

    // ==================== 阶段1: 初始化状态 ====================
    printf("\n阶段1: 初始化两台从机状态\n");
    pauseSim(sockHost, 1);

    float initPOSI[7] = {40.191f, 116.63f, 2500.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    float initCTRL[5] = {0.0f, 0.0f, 0.0f, 0.8f, 0.0f};

    printf("设置初始位置和姿态...\n");
    // 设置主机A
    sendPOSI(sockHost, initPOSI, 7, 0);
    sendCTRL(sockHost, initCTRL, 5, 0);
    //---------------------------------------------------------------------------------------------
    // 发送给从机1 (TSN) - 高优先级
    sendPOSI_vlan(pcap_handle, &config_tsn, initPOSI, 7, 0, PCP_POSI);
    sendCTRL_vlan(pcap_handle, &config_tsn, initCTRL, 5, 0, PCP_CTRL);
    // 2. 发送给 普通 从机 (标准 UDP)
    sendPOSI(sockSlaveStd, initPOSI, 7, 0);
    sendCTRL(sockSlaveStd, initCTRL, 5, 0);

    sleep(2);
    printf("恢复模拟...\n");
    pauseSim(sockHost, 0);
    sleep(2);
    printf("✓ 初始状态设置完成\n\n");

    // ==================== 阶段2: 实时同步 ====================
    printf("阶段2: 实时数据同步\n");

    const char *DREFS[] = {
        "sim/flightmodel/position/alpha",
        "sim/flightmodel/position/beta",
        "sim/flightmodel/position/groundspeed",
        "sim/flightmodel/position/true_airspeed",
        "sim/flightmodel/position/P",
        "sim/flightmodel/position/Q",
        "sim/flightmodel/position/R"};
    const int drefCount = 7;        //dataref计数

    float *drefValues[drefCount];
    int drefSizes[drefCount];
    for (int i = 0; i < drefCount; i++)
    {
        drefValues[i] = (float *)malloc(20 * sizeof(float));
        drefSizes[i] = 20;
    }

    float POSI[7];
    float CTRL[7];

    float syncInterval = 0.1;                            // 同步间隔
    int syncDuration = 100;                              // 同步时间
    int syncCycles = (int)(syncDuration / syncInterval); // 同步次数

    printf("开始双路同步 (%d次, 间隔%.1fms)...\n", syncCycles, syncInterval * 1000);

    for (int i = 0; i < syncCycles; i++)
    {
        // 从主机A获取数据
        if (getPOSI(sockHost, POSI, 0) < 0)
            break;
        if (getDREFs(sockHost, DREFS, drefValues, drefCount, drefSizes) < 0)
            break;
        if (getCTRL(sockHost, CTRL, 0) < 0)
            break;

        // 2. 发送给 TSN 从机
        sendPOSI_vlan(pcap_handle, &config_tsn, POSI, 7, 0, PCP_POSI);
        sendCTRL_vlan(pcap_handle, &config_tsn, CTRL, 7, 0, PCP_CTRL);
        sendDREFs_vlan(pcap_handle, &config_tsn, DREFS, drefValues, drefSizes, drefCount, PCP_DREF);

        // 3. 发送给 普通 从机 (PCP 0)
        sendPOSI(sockSlaveStd, POSI, 7, 0);
        sendCTRL(sockSlaveStd, CTRL, 7, 0);
        sendDREFs(sockSlaveStd, DREFS, drefValues, drefSizes, drefCount);
        if (i % 5 == 0)
        {
            printf("[%3d/%3d] 位置: 经度=%.5f, 纬度=%.5f, 高度=%.1f\n",
                   i, syncCycles, POSI[0], POSI[1], POSI[2]);
        }

        msleep((int)(syncInterval * 1000));
    }

    // ==================== 清理资源 ====================
    for (int i = 0; i < drefCount; i++)
    {
        free(drefValues[i]);
    }

    if (pcap_handle != NULL)
    {
        pcap_close(pcap_handle);
    }
    closeUDP(sockHost);
    closeUDP(sockSlaveStd);

    printf("\n==========================================\n");
    printf("同步完成！\n");
    printf("==========================================\n");

    return 0;
}