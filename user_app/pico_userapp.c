#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#define CMD_LED_ON       0x01
#define CMD_LED_OFF      0x02
#define CMD_BLINK        0x03
#define CMD_WIFI_CONNECT 0x04

int main(int argc, char *argv[]) {
    int fd = open("/dev/pico_usb", O_WRONLY);
    unsigned char buf[128]; // Buffer maior para Wi-Fi
    int ret = 0;
	
    if (fd < 0) {
        perror("Erro ao abrir driver");
        return 1;
    }

    if (argc < 2) {
        printf("Uso: %s [on | off | connect <ssid> <pass>]\n", argv[0]);
        close(fd);
        return 0;
    }

    if (!strcmp(argv[1], "on")) {
        buf[0] = CMD_LED_ON;
        ret = write(fd, buf, 1);
    } 
    else if (!strcmp(argv[1], "off")) {
        buf[0] = CMD_LED_OFF;
        ret = write(fd, buf, 1);
    } 
    else if (!strcmp(argv[1], "connect")) {
        if (argc < 4) {
            printf("Uso: %s connect <ssid> <password>\n", argv[0]);
            close(fd);
            return 0;
        }

        char *ssid = argv[2];
        char *pass = argv[3];
        int ssid_len = strlen(ssid);
        int pass_len = strlen(pass);

        printf("Enviando credenciais Wi-Fi: %s\n", ssid);

        // Montagem do Pacote (Protocolo definido no main.c)
        int idx = 0;
        buf[idx++] = CMD_WIFI_CONNECT; // Byte 0: Comando
        
        buf[idx++] = (uint8_t)ssid_len; // Byte 1: Tamanho SSID
        memcpy(&buf[idx], ssid, ssid_len);
        idx += ssid_len;

        buf[idx++] = (uint8_t)pass_len; // Byte X: Tamanho Senha
        memcpy(&buf[idx], pass, pass_len);
        idx += pass_len;

        ret = write(fd, buf, idx);
    }

    if (ret < 0) perror("Falha na escrita");
    else printf("Bytes enviados: %d\n", ret);

    close(fd);
    return 0;
}