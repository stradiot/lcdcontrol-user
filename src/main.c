#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "lcdcontrol.h"

#define DEFAULT_DEVICE "/dev/lcdcontrol"

void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options] <command> [args...]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -d <path>   Specify device (default: " DEFAULT_DEVICE ")\n");
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  print \"text\"       Write text to LCD\n");
    fprintf(stderr, "  read               Dump current LCD memory\n");
    fprintf(stderr, "  clear              Wipe the screen\n");
    fprintf(stderr, "  config <d> <c> <b> Set Display/Cursor/Blink (1/0)\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    char *device_path = DEFAULT_DEVICE;
    int opt;

    // 1. Simple Argument Parsing
    while ((opt = getopt(argc, argv, "d:")) != -1) {
        switch (opt) {
            case 'd': device_path = optarg; break;
            default: usage(argv[0]);
        }
    }

    if (optind >= argc) usage(argv[0]);
    char *cmd = argv[optind];

    // 2. Open Device
    int fd = open(device_path, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Error: Could not open %s: %s\n", device_path, strerror(errno));
        return 1;
    }

    // 3. Command Logic
    if (strcmp(cmd, "clear") == 0) {
        if (ioctl(fd, LCD_IOCTL_CLEAR) < 0) {
            perror("Failed to clear display");
            close(fd); return 1;
        }
        printf("Success: Display cleared.\n");
    }
    else if (strcmp(cmd, "config") == 0) {
        if (optind + 3 >= argc) {
            fprintf(stderr, "Error: Config requires 3 arguments (Display Cursor Blink)\n");
            close(fd); return 1;
        }
        struct lcd_config cfg;
        cfg.display_on = atoi(argv[optind + 1]);
        cfg.cursor_on  = atoi(argv[optind + 2]);
        cfg.blink_on   = atoi(argv[optind + 3]);

        if (ioctl(fd, LCD_IOCTL_SET_CONFIG, &cfg) < 0) {
            perror("Failed to configure display");
            close(fd); return 1;
        }
        printf("Success: Config applied.\n");
    }
    else if (strcmp(cmd, "print") == 0) {
        if (optind + 1 >= argc) {
            fprintf(stderr, "Error: Missing text argument.\n");
            close(fd); return 1;
        }
        char *text = argv[optind + 1];
        size_t len = strlen(text);
        
        // Write text
        if (write(fd, text, len) < 0) {
            perror("Write failed");
            close(fd); return 1;
        }
        // Auto-append newline if missing (for flush)
        if (len > 0 && text[len-1] != '\n') {
            write(fd, "\n", 1);
        }
        printf("Success: Sent \"%s\"\n", text);
    }
    else if (strcmp(cmd, "read") == 0) {
        char buf[32];
        ssize_t ret = read(fd, buf, 32);
        if (ret < 0) {
            perror("Read failed");
            close(fd); return 1;
        }
        printf("LCD Memory Dump:\n");
        printf("|%.16s|\n", buf);      // Top line
        printf("|%.16s|\n", buf + 16); // Bottom line
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        usage(argv[0]);
    }

    close(fd);
    return 0;
}
