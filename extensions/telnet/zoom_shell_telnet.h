/**
 * @file zoom_shell_telnet.h
 * @brief Zoom Shell Telnet 远程调试接口
 *
 * 通过 TCP socket 提供远程 shell 访问。
 * 依赖: OS + TCP/IP 网络栈 (用户需实现 socket API 回调)
 */

#ifndef ZOOM_SHELL_TELNET_H
#define ZOOM_SHELL_TELNET_H

#include "zoom_shell.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ZOOM_TELNET_PORT
#define ZOOM_TELNET_PORT    23
#endif

#ifndef ZOOM_TELNET_BUFFER_SIZE
#define ZOOM_TELNET_BUFFER_SIZE  1024
#endif

/** 平台 socket API 回调 */
typedef struct {
    int  (*socket_create)(void);
    int  (*socket_bind)(int fd, uint16_t port);
    int  (*socket_listen)(int fd, int backlog);
    int  (*socket_accept)(int fd);
    int  (*socket_recv)(int fd, char *buf, int len);
    int  (*socket_send)(int fd, const char *buf, int len);
    void (*socket_close)(int fd);
} zoom_telnet_ops_t;

typedef struct {
    zoom_shell_t shell;
    char buffer[ZOOM_TELNET_BUFFER_SIZE];
    int server_fd;
    int client_fd;
    uint8_t connected;
    const zoom_telnet_ops_t *ops;
} zoom_telnet_t;

/**
 * @brief 初始化 telnet 服务
 * @param telnet telnet 实例 (用户提供静态变量)
 * @param ops    socket 操作回调
 * @return 0 成功
 */
int zoom_telnet_init(zoom_telnet_t *telnet, const zoom_telnet_ops_t *ops);

/**
 * @brief Telnet 服务主循环 (阻塞)
 *
 * 等待客户端连接，连接后运行 shell 直到断开。
 * 适合在独立 OS 任务中调用。
 */
int zoom_telnet_run(zoom_telnet_t *telnet);

/** @brief 停止服务 */
void zoom_telnet_stop(zoom_telnet_t *telnet);

#ifdef __cplusplus
}
#endif

#endif /* ZOOM_SHELL_TELNET_H */
