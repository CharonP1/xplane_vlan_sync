// network_types.h
#ifndef NETWORK_TYPES_H
#define NETWORK_TYPES_H

#include <stdint.h>
#include <net/ethernet.h>

// 使用packed属性确保结构体正确对齐
#pragma pack(push, 1)

// VLAN头部结构
struct __attribute__((packed)) vlan_tag
{
    uint16_t tpid; // 0x8100 for VLAN
    uint16_t tci;  // PCP(3bit) + DEI(1bit) + VID(12bit)
};

// 以太网帧 + VLAN + IP + UDP + 数据
struct __attribute__((packed)) eth_vlan_frame
{
    struct ether_header eth;
    struct vlan_tag vlan;
    struct ip iphdr;
    struct udphdr udphdr;
    char payload[1500];
};

#pragma pack(pop)

#endif