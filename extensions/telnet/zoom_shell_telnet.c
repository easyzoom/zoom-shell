/**
 * @file zoom_shell_telnet.c
 * @brief Zoom Shell Telnet 远程调试接口实现
 *
 * 通过用户提供的 socket API 回调实现 TCP 服务端。
 * 每次只支持一个客户端连接。
 */

#include "zoom_shell_telnet.h"

#include <string.h>

static zoom_telnet_t *s_current_telnet = NULL;

static int16_t telnet_read(char *data, uint16_t len)
{
    if (!s_current_telnet || !s_current_telnet->connected) return 0;
    int ret = s_current_telnet->ops->socket_recv(
        s_current_telnet->client_fd, data, (int)len);
    if (ret <= 0) {
        s_current_telnet->connected = 0;
        return 0;
    }

    /* Filter telnet negotiation bytes (0xFF prefix) */
    if ((uint8_t)data[0] == 0xFF && ret >= 3) return 0;

    return (int16_t)ret;
}

static int16_t telnet_write(const char *data, uint16_t len)
{
    if (!s_current_telnet || !s_current_telnet->connected) return 0;

    /* Convert \n to \r\n for telnet protocol */
    for (uint16_t i = 0; i < len; i++) {
        if (data[i] == '\n') {
            char cr = '\r';
            s_current_telnet->ops->socket_send(
                s_current_telnet->client_fd, &cr, 1);
        }
        s_current_telnet->ops->socket_send(
            s_current_telnet->client_fd, &data[i], 1);
    }

    return (int16_t)len;
}

int zoom_telnet_init(zoom_telnet_t *telnet, const zoom_telnet_ops_t *ops)
{
    if (!telnet || !ops) return -1;

    memset(telnet, 0, sizeof(*telnet));
    telnet->ops = ops;
    telnet->server_fd = -1;
    telnet->client_fd = -1;
    telnet->connected = 0;

    telnet->shell.read  = telnet_read;
    telnet->shell.write = telnet_write;

    return 0;
}

int zoom_telnet_run(zoom_telnet_t *telnet)
{
    if (!telnet || !telnet->ops) return -1;

    telnet->server_fd = telnet->ops->socket_create();
    if (telnet->server_fd < 0) return -1;

    if (telnet->ops->socket_bind(telnet->server_fd, ZOOM_TELNET_PORT) < 0) {
        telnet->ops->socket_close(telnet->server_fd);
        return -1;
    }

    if (telnet->ops->socket_listen(telnet->server_fd, 1) < 0) {
        telnet->ops->socket_close(telnet->server_fd);
        return -1;
    }

    while (1) {
        telnet->client_fd = telnet->ops->socket_accept(telnet->server_fd);
        if (telnet->client_fd < 0) continue;

        telnet->connected = 1;
        s_current_telnet = telnet;

        /* Send telnet negotiation: will echo, will suppress go-ahead */
        const char nego[] = {
            (char)0xFF, (char)0xFB, 0x01,  /* WILL ECHO */
            (char)0xFF, (char)0xFB, 0x03,  /* WILL SGA */
        };
        telnet->ops->socket_send(telnet->client_fd, nego, sizeof(nego));

        zoom_shell_init(&telnet->shell,
                        telnet->buffer, ZOOM_TELNET_BUFFER_SIZE);
        zoom_shell_run(&telnet->shell);
        zoom_shell_deinit(&telnet->shell);

        telnet->connected = 0;
        s_current_telnet = NULL;
        telnet->ops->socket_close(telnet->client_fd);
        telnet->client_fd = -1;
    }
}

void zoom_telnet_stop(zoom_telnet_t *telnet)
{
    if (!telnet) return;
    if (telnet->client_fd >= 0) {
        telnet->connected = 0;
        telnet->shell.state = ZOOM_STATE_IDLE;
        telnet->ops->socket_close(telnet->client_fd);
        telnet->client_fd = -1;
    }
    if (telnet->server_fd >= 0) {
        telnet->ops->socket_close(telnet->server_fd);
        telnet->server_fd = -1;
    }
}
