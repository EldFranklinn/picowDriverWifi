#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "pico/cyw43_arch.h"

// Comandos suportados
#define CMD_LED_ON       0x01
#define CMD_LED_OFF      0x02
#define CMD_WIFI_CONNECT 0x04 

// Estrutura da fila de comandos
typedef struct {
    uint8_t cmd;
    uint8_t len;
    uint8_t data[128]; // Aumentado levemente para garantir espaço para SSIDs longos
} cmd_t;

static volatile int cmd_head = 0;
static volatile int cmd_tail = 0;
static cmd_t cmd_queue[4];

// --- Gerenciamento da Fila (Queue) ---

static int push_cmd(const uint8_t *buf, uint32_t len) {
    int next = (cmd_head + 1) % 4;
    if (next == cmd_tail) return -1; // Fila cheia
    
    cmd_queue[cmd_head].cmd = buf[0];
    
    // O tamanho dos dados é o total recebido menos 1 byte (o comando)
    uint32_t data_len = (len > 0) ? len - 1 : 0;
    if (data_len > 128) data_len = 128;
    
    cmd_queue[cmd_head].len = data_len;
    
    if (data_len > 0) {
        memcpy(cmd_queue[cmd_head].data, &buf[1], data_len);
    }
    
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    cmd_head = next;
    return 0;
}

static int pop_cmd(cmd_t *out) {
    if (cmd_tail == cmd_head) return -1; // Fila vazia
    *out = cmd_queue[cmd_tail];
    cmd_tail = (cmd_tail + 1) % 4;
    return 0;
}

// --- Callbacks USB TinyUSB ---

// Chamado quando dados chegam do PC para a Pico
void tud_vendor_rx_cb(uint8_t itf, uint8_t const* buf, uint16_t count)
{
    // Empilha diretamente para processar no loop principal
    push_cmd(buf, count);
}

// --- Lógica do Wi-Fi ---

void handle_wifi_connect(uint8_t *data, uint8_t len) {
    // Protocolo: [Len_SSID] [SSID...] [Len_PASS] [PASS...]
    if (len < 2) return;

    // 1. Extrair SSID
    uint8_t ssid_len = data[0];
    if (ssid_len > 32 || ssid_len >= len) return; // Validação básica

    char ssid[33];
    memcpy(ssid, &data[1], ssid_len);
    ssid[ssid_len] = '\0';

    // 2. Extrair Senha
    uint8_t pass_offset = 1 + ssid_len;
    if (pass_offset >= len) return; // Sem senha?

    uint8_t pass_len = data[pass_offset];
    char pass[65];
    
    // Garante que não estoure o buffer lido
    if (pass_offset + 1 + pass_len > len) {
        pass_len = len - (pass_offset + 1);
    }
    
    memcpy(pass, &data[pass_offset + 1], pass_len);
    pass[pass_len] = '\0';

    printf("Pico W: Conectando a '%s'...\n", ssid);
    
    // Tenta conectar (timeout 15s)
    int err = cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_AES_PSK, 15000);
    
    if (err == 0) {
        printf("Pico W: Conectado com sucesso!\n");
        // Liga o LED para indicar sucesso
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1); 
    } else {
        printf("Pico W: Falha ao conectar (%d)\n", err);
        // Desliga o LED para indicar falha
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    }
}

// --- Processador de Comandos ---

void handle_pending_commands(void) {
    static cmd_t c;
    
    // Se houver comando na fila, processa
    if (pop_cmd(&c) == 0) {
         switch (c.cmd) {
            case CMD_LED_ON:
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                break;
                
            case CMD_LED_OFF:
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                break;
                
            case CMD_WIFI_CONNECT:
                handle_wifi_connect(c.data, c.len);
                break;
                
            default:
                break;
        }
    }
}

// --- Main ---

int main() {
    stdio_init_all();

    // Inicializa o chip Wi-Fi (necessário para LED e Conexão)
    if (cyw43_arch_init()) {
        printf("Falha ao iniciar cyw43\n");
        return -1;
    }
    
    // Habilita modo Station (Cliente Wi-Fi)
    cyw43_arch_enable_sta_mode();
    
    // Pisca LED inicial para mostrar que bootou
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(200);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    // Inicializa USB
    tusb_init();

    while (true) {
        tud_task(); // Mantém a pilha USB rodando
        handle_pending_commands(); // Processa lógica da aplicação
    }
}