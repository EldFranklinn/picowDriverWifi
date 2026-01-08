#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// --- Configurações Gerais do lwIP ---
// NO_SYS = 1 para rodar sem um sistema operacional complexo (Bare Metal)
// O SDK da Pico cuida da sincronização para nós no modo background
#define NO_SYS                      1
#define LWIP_SOCKET                 0
#define LWIP_NETCONN                0

// --- Configurações de Memória ---
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    4000
#define MEMP_NUM_TCP_SEG            32
#define MEMP_NUM_ARP_QUEUE          10
#define PBUF_POOL_SIZE              24

// --- Protocolos ---
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define LWIP_IPV4                   1
#define LWIP_TCP                    1
#define LWIP_UDP                    1
#define LWIP_DNS                    1
#define LWIP_DHCP                   1  // Importante para pegar IP do roteador

// --- Checksums (Hardware da Pico ajuda nisso) ---
#define LWIP_CHKSUM_ALGORITHM       3

// --- Configurações de TCP ---
#define TCP_MSS                     1460
#define TCP_WND                     (8 * TCP_MSS)

// --- Debug (Opcional) ---
#define LWIP_STATS                  0
#define LWIP_DEBUG                  0

#endif