// pcap_types.h
#ifndef PCAP_TYPES_H
#define PCAP_TYPES_H

#include <stdint.h>

// 定义 pcap 需要的类型
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef int bpf_int32;
typedef u_int bpf_u_int32;

// pcap 常量定义
#define PCAP_ERRBUF_SIZE 256
#define PCAP_VERSION_MAJOR 2
#define PCAP_VERSION_MINOR 4

#define __USE_MISC 1

#endif