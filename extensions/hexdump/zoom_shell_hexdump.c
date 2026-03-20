/**
 * @file zoom_shell_hexdump.c
 * @brief Zoom Shell HexDump 扩展 — 内存查看/写入
 *
 * 命令: mem read <addr> [len], mem write <addr> <val>, mem dump <addr> <len>
 */

#include "zoom_shell.h"

#if ZOOM_USING_HEXDUMP

static uintptr_t hex_str_to_addr(const char *s)
{
    uintptr_t r = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    while (*s) {
        char c = *s++;
        if (c >= '0' && c <= '9')      r = (r << 4) | (uintptr_t)(c - '0');
        else if (c >= 'a' && c <= 'f')  r = (r << 4) | (uintptr_t)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F')  r = (r << 4) | (uintptr_t)(c - 'A' + 10);
        else break;
    }
    return r;
}

static uint32_t hex_str_to_u32(const char *s)
{
    return (uint32_t)hex_str_to_addr(s);
}

static void hexdump_line(zoom_shell_t *shell, uintptr_t addr,
                         const uint8_t *data, uint16_t len)
{
    zoom_printf(shell, "  %08lx: ", (unsigned long)addr);

    for (uint16_t i = 0; i < 16; i++) {
        if (i < len)
            zoom_printf(shell, "%02x ", data[i]);
        else
            zoom_printf(shell, "   ");
        if (i == 7) zoom_printf(shell, " ");
    }

    zoom_printf(shell, " |");
    for (uint16_t i = 0; i < len; i++) {
        char c = (char)data[i];
        zoom_printf(shell, "%c", (c >= 0x20 && c < 0x7F) ? c : '.');
    }
    zoom_printf(shell, "|\r\n");
}

static int cmd_mem_read(zoom_shell_t *shell, int argc, char *argv[])
{
    if (argc < 1) {
        zoom_error(shell, "Usage: mem read <addr> [len]\r\n");
        return -1;
    }
    uintptr_t addr = hex_str_to_addr(argv[0]);
    uint32_t len = (argc >= 2) ? hex_str_to_u32(argv[1]) : 64;
    if (len > 1024) len = 1024;

    const uint8_t *ptr = (const uint8_t *)addr;
    uint32_t off = 0;
    while (off < len) {
        uint16_t chunk = (len - off > 16) ? 16 : (uint16_t)(len - off);
        hexdump_line(shell, addr + off, ptr + off, chunk);
        off += chunk;
    }
    return 0;
}

static int cmd_mem_write(zoom_shell_t *shell, int argc, char *argv[])
{
    if (argc < 2) {
        zoom_error(shell, "Usage: mem write <addr> <value>\r\n");
        return -1;
    }
    uintptr_t addr = hex_str_to_addr(argv[0]);
    uint32_t val  = hex_str_to_u32(argv[1]);

    volatile uint32_t *ptr = (volatile uint32_t *)addr;
    *ptr = val;

    zoom_ok(shell, "[%08lx] = 0x%08lx\r\n",
            (unsigned long)addr, (unsigned long)val);
    return 0;
}

static int cmd_mem_dump(zoom_shell_t *shell, int argc, char *argv[])
{
    return cmd_mem_read(shell, argc, argv);
}

ZOOM_SUBCMD_SET(sub_mem,
    ZOOM_SUBCMD(read,  cmd_mem_read,  "Read memory (hex dump)"),
    ZOOM_SUBCMD(write, cmd_mem_write, "Write 32-bit value"),
    ZOOM_SUBCMD(dump,  cmd_mem_dump,  "Dump memory region"),
);

#if ZOOM_USING_CMD_EXPORT
ZOOM_EXPORT_CMD_WITH_SUB(mem, sub_mem, "Memory read/write/dump",
                          ZOOM_ATTR_DANGEROUS, ZOOM_USER_USER);
#endif

#endif /* ZOOM_USING_HEXDUMP */
