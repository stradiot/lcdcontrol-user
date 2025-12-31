/* src/main.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/sysinfo.h> // For uptime
#include <ifaddrs.h>     // For IP address
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "lcdcontrol.h"

#define DEFAULT_DEVICE "/dev/lcdcontrol"

// Global flag to handle Ctrl+C cleanly
volatile sig_atomic_t stop = 0;

void handle_sigint(int sig) {
    (void)sig;
    stop = 1;
}

// --- SYSTEM INFO HELPERS ---

void get_uptime_str(char *buffer, size_t size) {
    struct sysinfo info;
    sysinfo(&info);
    
    long total_seconds = info.uptime;
    int days = total_seconds / (3600 * 24);
    int hours = (total_seconds % (3600 * 24)) / 3600;
    int minutes = (total_seconds % 3600) / 60;
    
    if (days > 0)
        snprintf(buffer, size, "Up: %dd %dh %dm", days, hours, minutes);
    else
        snprintf(buffer, size, "Up: %02d:%02d:%02ld", hours, minutes, total_seconds % 60);
}

void get_cpu_temp_str(char *buffer, size_t size) {
    FILE *f = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!f) {
        snprintf(buffer, size, "Temp: N/A");
        return;
    }
    
    int temp_milli;
    if (fscanf(f, "%d", &temp_milli) != 1) temp_milli = 0;
    fclose(f);
    
    snprintf(buffer, size, "CPU: %.1fC", temp_milli / 1000.0);
}

void get_hostname_str(char *buffer, size_t size) {
    char hostname[64];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        snprintf(buffer, size, "%s", hostname);
    } else {
        snprintf(buffer, size, "Host: Unknown");
    }
}

void get_ip_str(char *buffer, size_t size) {
    struct ifaddrs *ifaddr, *ifa;
    int family;
    
    if (getifaddrs(&ifaddr) == -1) {
        snprintf(buffer, size, "IP: Error");
        return;
    }

    // Default if not found
    snprintf(buffer, size, "IP: No Network");

    // Loop through linked list of interfaces
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        
        family = ifa->ifa_addr->sa_family;
        
        // Look for IPv4 (AF_INET)
        if (family == AF_INET) {
            // Skip loopback (lo)
            if (strcmp(ifa->ifa_name, "lo") != 0) {
                 char host[NI_MAXHOST];
                 int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                                     host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                 if (s == 0) {
                     snprintf(buffer, size, "%s", host);
                     break; // Found one, stop looking
                 }
            }
        }
    }
    freeifaddrs(ifaddr);
}

// Helper to fill buffer based on type string
void fill_info(const char *type, char *buffer, size_t size) {
    if (strcmp(type, "uptime") == 0) get_uptime_str(buffer, size);
    else if (strcmp(type, "temp") == 0) get_cpu_temp_str(buffer, size);
    else if (strcmp(type, "host") == 0) get_hostname_str(buffer, size);
    else if (strcmp(type, "ip") == 0) get_ip_str(buffer, size);
    else snprintf(buffer, size, "%s", type); // Just print the literal string if unknown
}

void do_monitor(int fd, const char *row0_type, const char *row1_type) {
    char line0[17];
    char line1[17];
    // INCREASED SIZE: 16+1 + 16+1 + 1 (null) = 35 bytes minimum.
    // We use 64 to be safe.
    char full_msg[64];

    // Hide cursor for cleaner look
    struct lcd_config cfg = {1, 0, 0};
    ioctl(fd, LCD_IOCTL_SET_CONFIG, &cfg);

    printf("Starting monitor... (Press Ctrl+C to stop)\n");
    signal(SIGINT, handle_sigint);

    while (!stop) {
        // 1. Gather Data
        fill_info(row0_type, line0, 17);
        fill_info(row1_type, line1, 17);

        // 2. Clear Screen
        // This resets the state so the first write goes to the bottom,
        // and the second write pushes it to the top.
        ioctl(fd, LCD_IOCTL_CLEAR);

        // 3. Format Message
        // CHANGE: Added '\n' at the very end so the second line flushes too.
        snprintf(full_msg, sizeof(full_msg), "%-16s\n%-16s\n", line0, line1);

        // 4. Write
        write(fd, full_msg, strlen(full_msg));

        // 5. Wait 1 second
        sleep(1);
    }

    printf("\nExiting monitor.\n");
    // Restore cursor
    cfg.cursor_on = 1; cfg.blink_on = 1;
    ioctl(fd, LCD_IOCTL_SET_CONFIG, &cfg);
}

void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options] <command> [args...]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -d <path>   Specify device (default: " DEFAULT_DEVICE ")\n");
    fprintf(stderr, "Commands:\n");
    fprintf(stderr, "  print \"text\"       Write text to LCD\n");
    fprintf(stderr, "  read               Dump current LCD memory\n");
    fprintf(stderr, "  clear              Wipe the screen\n");
    fprintf(stderr, "  config <d> <c> <b> Set Display/Cursor/Blink (1/0)\n");
    fprintf(stderr, "  monitor <r1> <r2>  Live stats. Options: uptime, temp, ip, host\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    char *device_path = DEFAULT_DEVICE;
    int opt;

    while ((opt = getopt(argc, argv, "d:")) != -1) {
        switch (opt) {
            case 'd': device_path = optarg; break;
            default: usage(argv[0]);
        }
    }

    if (optind >= argc) usage(argv[0]);
    char *cmd = argv[optind];

    int fd = open(device_path, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Error opening %s: %s\n", device_path, strerror(errno));
        return 1;
    }

    // --- NEW MONITOR COMMAND ---
    if (strcmp(cmd, "monitor") == 0) {
        if (optind + 2 >= argc) {
            fprintf(stderr, "Error: monitor requires 2 arguments (one for each row)\n");
            fprintf(stderr, "Example: %s monitor ip temp\n", argv[0]);
            close(fd); return 1;
        }
        do_monitor(fd, argv[optind + 1], argv[optind + 2]);
    }
    // --- EXISTING COMMANDS ---
    else if (strcmp(cmd, "clear") == 0) {
        ioctl(fd, LCD_IOCTL_CLEAR);
    }
    else if (strcmp(cmd, "config") == 0) {
        if (optind + 3 >= argc) usage(argv[0]);
        struct lcd_config cfg;
        cfg.display_on = atoi(argv[optind + 1]);
        cfg.cursor_on  = atoi(argv[optind + 2]);
        cfg.blink_on   = atoi(argv[optind + 3]);
        ioctl(fd, LCD_IOCTL_SET_CONFIG, &cfg);
    }
    else if (strcmp(cmd, "print") == 0) {
        if (optind + 1 >= argc) usage(argv[0]);
        char *text = argv[optind + 1];
        size_t len = strlen(text);
        write(fd, text, len);
        if (len > 0 && text[len-1] != '\n') write(fd, "\n", 1);
    }
    else if (strcmp(cmd, "read") == 0) {
        char buf[32];
        ssize_t ret = read(fd, buf, 32);
        if (ret > 0) {
            printf("|%.16s|\n|%.16s|\n", buf, buf+16);
        }
    }
    else {
        usage(argv[0]);
    }

    close(fd);
    return 0;
}
