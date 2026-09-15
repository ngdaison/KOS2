#include <stdbool.h>

#include <kos/command.h>
#include <kos/console.h>
#include <kos/cpu.h>
#include <kos/driver.h>
#include <kos/e1000.h>
#include <kos/elf.h>
#include <kos/interrupts.h>
#include <kos/keyboard.h>
#include <kos/log.h>
#include <kos/net.h>
#include <kos/pci.h>
#include <kos/pic.h>
#include <kos/pmm.h>
#include <kos/serial.h>
#include <kos/system.h>
#include <kos/timer.h>
#include <kos/task.h>
#include <kos/vfs.h>

typedef void (*command_handler)(const char *arguments);

struct command_entry {
    const char *name;
    command_handler handler;
};

static void command_help(const char *arguments) {
    (void)arguments;
    log_info("help clear meminfo uptime sysinfo netinfo ifconfig arp arping ping dhcp udptest tcptest nslookup curl httpd netstat echo reboot irqinfo kbdinfo drivers lspci taskinfo tasktest taskdemo vol ls cat elfinfo");
}

static void command_clear(const char *arguments) {
    (void)arguments;
    console_clear();
    serial_write("\x1b[2J\x1b[H");
}

static void command_meminfo(const char *arguments) {
    (void)arguments;
    log_info("meminfo");
    log_info_u64("  Total RAM bytes: ", pmm_total_memory_bytes());
    log_info_u64("  Usable RAM bytes: ", pmm_usable_memory_bytes());
    log_info_u64("  PMM tracked bytes: ", pmm_tracked_memory_bytes());
    log_info_u64("  Kernel/modules reserved bytes: ", pmm_kernel_memory_bytes());
    log_info_u64("  Framebuffer reserved bytes: ", pmm_framebuffer_memory_bytes());
    log_info_u64("  Bootloader reclaimable bytes: ", pmm_bootloader_reclaimable_bytes());
    log_info_u64("  Frame size bytes: ", pmm_frame_size());
    log_info_u64("  Free frames: ", pmm_free_frame_count());
    log_info_u64("  Allocated frames: ", pmm_allocated_frame_count());
}

static void command_uptime(const char *arguments) {
    (void)arguments;
    log_info_u64_suffix("uptime: ", timer_uptime_seconds(), " seconds");
}

static void command_sysinfo(const char *arguments) {
    (void)arguments;
    struct kos_system_info info;
    system_info_snapshot(&info);
    log_info("sysinfo");
    log_info("  Kernel: KOS v0.1");
    log_info("  Architecture: x86_64");
    log_info_u64("  Uptime seconds: ", info.uptime_seconds);
    log_info_u64("  Total memory bytes: ", info.total_memory_bytes);
    log_info_u64("  Usable memory bytes: ", info.usable_memory_bytes);
    log_info_u64("  Free memory frames: ", info.free_memory_frames);
    log_info_u64("  Kernel tasks: ", info.kernel_task_count);
    log_info_u64("  Ready tasks: ", info.ready_task_count);
    log_info("  Process isolation: unavailable");
    log_info("  Scheduler: cooperative round-robin");
}

static void command_netinfo(const char *arguments) {
    (void)arguments;
    log_info("netinfo");
    log_info("  IPv4 UDP socket core: loopback and IPv4 external ready");
    log_info(net_e1000_detected()
        ? "  e1000 NIC: initialized (RX/TX DMA rings ready)"
        : "  e1000 NIC: no compatible adapter");
    struct net_interface_info info;
    if (net_interface_info(&info) && info.available) {
        if (info.configuration_source == NET_CONFIGURATION_DHCP) {
            log_info("  eth0: DHCP configured; lease active");
        }
        else {
            log_info("  eth0: static IPv4 configured; ARP/Ethernet ready");
        }
    }
    else {
        log_info("  eth0: unavailable");
    }
    log_info_u64("  UDP datagrams sent: ", net_udp_datagrams_sent());
    log_info_u64("  UDP datagrams received: ", net_udp_datagrams_received());
    log_info_u64("  UDP datagrams dropped: ", net_udp_datagrams_dropped());
    log_info_u64("  Ethernet frames transmitted: ", e1000_transmitted_frame_count());
    log_info_u64("  Ethernet frames received: ", e1000_received_frame_count());
    log_info_u64("  Ethernet frames dropped: ", net_ethernet_frames_dropped());
    log_info_u64("  e1000 TX timeouts: ", e1000_transmit_timeout_count());
    log_info_u64("  ARP requests sent: ", net_arp_requests_sent());
    log_info_u64("  ARP replies received: ", net_arp_replies_received());
    log_info_u64("  ICMP echo requests sent: ", net_icmp_echo_requests_sent());
    log_info_u64("  ICMP echo replies received: ", net_icmp_echo_replies_received());
    log_info_u64("  DHCP discovers sent: ", net_dhcp_discovers_sent());
    log_info_u64("  DHCP offers received: ", net_dhcp_offers_received());
    log_info_u64("  DHCP requests sent: ", net_dhcp_requests_sent());
    log_info_u64("  DHCP acks received: ", net_dhcp_acks_received());
    log_info_u64("  TCP segments sent: ", net_tcp_segments_sent());
    log_info_u64("  TCP segments received: ", net_tcp_segments_received());
    log_info_u64("  TCP retransmissions: ", net_tcp_retransmissions());
    log_info_u64("  DNS queries sent: ", net_dns_queries_sent());
    log_info_u64("  DNS responses received: ", net_dns_responses_received());
}

static void command_dhcp(const char *arguments) {
    (void)arguments;
    if (!net_has_external_interface()) {
        log_error("dhcp: eth0 is unavailable.");
        return;
    }
    if (!net_dhcp_acquire(timer_frequency_hz() * 5)) {
        log_error("dhcp: failed to acquire DHCP lease.");
        return;
    }
    struct net_interface_info info;
    if (net_interface_info(&info) && info.available) {
        char address[16];
        char gateway[16];
        char dns[16];
        char line[64];
        net_format_ipv4(info.ipv4_address, address);
        net_format_ipv4(info.gateway, gateway);
        net_format_ipv4(info.dns_server, dns);

        uint64_t pos = 0;
        const char *prefix_addr = "DHCP: address ";
        for (uint64_t i = 0; prefix_addr[i] != '\0'; ++i) {
            line[pos++] = prefix_addr[i];
        }
        for (uint64_t i = 0; address[i] != '\0'; ++i) {
            line[pos++] = address[i];
        }
        line[pos] = '\0';
        log_info(line);

        pos = 0;
        const char *prefix_gw = "DHCP: gateway ";
        for (uint64_t i = 0; prefix_gw[i] != '\0'; ++i) {
            line[pos++] = prefix_gw[i];
        }
        for (uint64_t i = 0; gateway[i] != '\0'; ++i) {
            line[pos++] = gateway[i];
        }
        line[pos] = '\0';
        log_info(line);

        pos = 0;
        const char *prefix_dns = "DHCP: DNS ";
        for (uint64_t i = 0; prefix_dns[i] != '\0'; ++i) {
            line[pos++] = prefix_dns[i];
        }
        for (uint64_t i = 0; dns[i] != '\0'; ++i) {
            line[pos++] = dns[i];
        }
        line[pos] = '\0';
        log_info(line);
    }
}

static void command_ifconfig(const char *arguments) {
    (void)arguments;
    struct net_interface_info info;
    if (!net_interface_info(&info) || !info.available) {
        log_error("ifconfig: eth0 is unavailable.");
        return;
    }
    char mac[18];
    char address[16];
    char mask[16];
    char gateway[16];
    char dns[16];
    net_format_mac(info.mac_address, mac);
    net_format_ipv4(info.ipv4_address, address);
    net_format_ipv4(info.subnet_mask, mask);
    net_format_ipv4(info.gateway, gateway);
    net_format_ipv4(info.dns_server, dns);
    log_info("eth0");
    log_info("  MAC:"); log_info(mac);
    log_info("  IPv4:"); log_info(address);
    log_info("  Netmask:"); log_info(mask);
    log_info("  Gateway:"); log_info(gateway);
    log_info("  DNS:"); log_info(dns);
    log_info_u64("  IPv4 packets received: ", info.ipv4_packets_received);
    log_info_u64("  IPv4 packets dropped: ", info.ipv4_packets_dropped);
}

static void command_arp(const char *arguments) {
    (void)arguments;
    log_info("ARP cache");
    uint64_t count = net_arp_cache_count();
    if (count == 0) {
        log_info("  (empty)");
        return;
    }
    for (uint64_t index = 0; index < count; ++index) {
        struct net_arp_cache_entry entry;
        char address[16];
        char mac[18];
        if (net_arp_cache_at(index, &entry)) {
            net_format_ipv4(entry.ipv4_address, address);
            net_format_mac(entry.mac_address, mac);
            log_info(address);
            log_info(mac);
        }
    }
}

static bool command_parse_ipv4(const char *text, uint32_t *address) {
    if (text == 0 || address == 0 || *text == '\0') {
        return false;
    }
    uint32_t result = 0;
    for (uint64_t octet = 0; octet < 4; ++octet) {
        uint32_t value = 0;
        uint64_t digits = 0;
        while (*text >= '0' && *text <= '9') {
            value = value * 10 + (uint32_t)(*text - '0');
            ++text;
            ++digits;
            if (value > 255 || digits > 3) {
                return false;
            }
        }
        if (digits == 0 || (octet < 3 && *text != '.') || (octet == 3 && *text != '\0')) {
            return false;
        }
        result = (result << 8) | value;
        if (octet < 3) {
            ++text;
        }
    }
    *address = result;
    return true;
}

static void command_arping(const char *arguments) {
    uint32_t address;
    uint8_t mac[NET_ETHERNET_ADDRESS_SIZE];
    if (!command_parse_ipv4(arguments, &address)) {
        log_error("arping: use an IPv4 address, for example arping 10.0.2.2.");
        return;
    }
    if (!net_arp_resolve(address, mac, NET_ARP_DEFAULT_TIMEOUT_TICKS)) {
        log_error("arping: no ARP reply before timeout.");
        return;
    }
    char address_text[16];
    char mac_text[18];
    net_format_ipv4(address, address_text);
    net_format_mac(mac, mac_text);
    log_info("ARP reply from:");
    log_info(address_text);
    log_info(mac_text);
}

static bool command_parse_ping_arguments(const char *arguments, uint32_t *address, uint64_t *count) {
    if (arguments == 0 || address == 0 || count == 0) {
        return false;
    }
    char address_text[16];
    uint64_t length = 0;
    while (arguments[length] != '\0' && arguments[length] != ' ' && length + 1 < sizeof(address_text)) {
        address_text[length] = arguments[length];
        ++length;
    }
    address_text[length] = '\0';
    if (!command_parse_ipv4(address_text, address)) {
        return false;
    }
    while (arguments[length] == ' ') {
        ++length;
    }
    if (arguments[length] == '\0') {
        *count = 4;
        return true;
    }
    uint64_t value = 0;
    uint64_t digits = 0;
    while (arguments[length] >= '0' && arguments[length] <= '9') {
        value = value * 10 + (uint64_t)(arguments[length] - '0');
        ++length;
        ++digits;
        if (value > 16) {
            return false;
        }
    }
    if (digits == 0 || arguments[length] != '\0' || value == 0) {
        return false;
    }
    *count = value;
    return true;
}

static void command_ping(const char *arguments) {
    uint32_t address;
    uint64_t count;
    if (!command_parse_ping_arguments(arguments, &address, &count)) {
        log_error("ping: use ping <IPv4> [count], with count from 1 to 16.");
        return;
    }
    char address_text[16];
    net_format_ipv4(address, address_text);
    log_info("PING");
    log_info(address_text);
    uint64_t received = 0;
    for (uint64_t sequence = 1; sequence <= count; ++sequence) {
        uint64_t round_trip_ticks;
        if (net_icmp_ping(address, (uint16_t)sequence, timer_frequency_hz(), &round_trip_ticks)) {
            ++received;
            log_info("64 bytes from:");
            log_info(address_text);
            log_info_u64("  seq: ", sequence);
            log_info_u64("  time ms: ", round_trip_ticks * 1000 / timer_frequency_hz());
        }
        else {
            log_info_u64("Request timed out, seq: ", sequence);
        }
    }
    log_info_u64("Packets transmitted: ", count);
    log_info_u64("Packets received: ", received);
    log_info_u64("Packet loss percent: ", (count - received) * 100 / count);
}

static void command_udptest(const char *arguments) {
    (void)arguments;
    if (net_udp_loopback_self_test()) {
        log_info("udptest passed: UDP payload crossed the 127.0.0.1 socket path.");
    }
    else {
        log_error("udptest failed: UDP loopback socket path is unhealthy.");
    }
}

static bool command_parse_tcp_arguments(const char *arguments, uint32_t *address, uint16_t *port) {
    if (arguments == 0 || address == 0 || port == 0) {
        return false;
    }
    char ip_text[16];
    uint64_t length = 0;
    while (arguments[length] != '\0' && arguments[length] != ' ' && length + 1 < sizeof(ip_text)) {
        ip_text[length] = arguments[length];
        ++length;
    }
    ip_text[length] = '\0';
    if (!command_parse_ipv4(ip_text, address)) {
        return false;
    }
    while (arguments[length] == ' ') {
        ++length;
    }
    if (arguments[length] == '\0') {
        *port = 80;
        return true;
    }
    uint32_t val = 0;
    while (arguments[length] >= '0' && arguments[length] <= '9') {
        val = val * 10 + (uint32_t)(arguments[length] - '0');
        if (val > 65535) {
            return false;
        }
        ++length;
    }
    if (val == 0 || arguments[length] != '\0') {
        return false;
    }
    *port = (uint16_t)val;
    return true;
}

static void command_tcptest(const char *arguments) {
    uint32_t address;
    uint16_t port;
    if (!command_parse_tcp_arguments(arguments, &address, &port)) {
        log_error("tcptest: use tcptest <IPv4> [port], default port 80.");
        return;
    }
    char address_text[16];
    net_format_ipv4(address, address_text);
    log_info("tcptest: connecting to TCP host...");
    int sock = tcp_connect(address, port);
    if (sock < 0) {
        log_error("tcptest: TCP connection failed or timed out.");
        return;
    }
    log_info("tcptest: TCP connection ESTABLISHED!");
    log_info("tcptest: sending HTTP GET request...");
    const char *req = "GET / HTTP/1.0\r\nHost: 1.1.1.1\r\nUser-Agent: KOS\r\n\r\n";
    uint64_t req_len = 0;
    while (req[req_len] != '\0') {
        ++req_len;
    }
    int sent = tcp_send(sock, (const uint8_t *)req, req_len);
    if (sent <= 0) {
        log_error("tcptest: failed to send HTTP request.");
        tcp_close(sock);
        return;
    }
    log_info("tcptest: waiting for HTTP response...");
    uint8_t buffer[512];
    int received = tcp_receive(sock, buffer, sizeof(buffer) - 1);
    if (received > 0) {
        buffer[received] = '\0';
        log_info("tcptest: HTTP response received:");
        log_info((const char *)buffer);
    }
    else {
        log_error("tcptest: no response received or connection closed.");
    }
    tcp_close(sock);
    log_info("tcptest: TCP connection closed cleanly.");
}

static void command_nslookup(const char *arguments) {
    if (arguments == 0 || *arguments == '\0') {
        log_error("nslookup: use nslookup <hostname>");
        return;
    }
    while (*arguments == ' ') {
        ++arguments;
    }
    char hostname[64];
    uint64_t len = 0;
    while (arguments[len] != '\0' && arguments[len] != ' ' && len + 1 < sizeof(hostname)) {
        hostname[len] = arguments[len];
        ++len;
    }
    hostname[len] = '\0';
    if (len == 0) {
        log_error("nslookup: use nslookup <hostname>");
        return;
    }

    struct net_interface_info info;
    if (net_interface_info(&info) && info.available && info.dns_server != 0) {
        char dns_text[16];
        char server_line[48];
        net_format_ipv4(info.dns_server, dns_text);
        uint64_t pos = 0;
        const char *p1 = "Server: ";
        for (uint64_t i = 0; p1[i] != '\0'; ++i) server_line[pos++] = p1[i];
        for (uint64_t i = 0; dns_text[i] != '\0'; ++i) server_line[pos++] = dns_text[i];
        server_line[pos] = '\0';
        log_info(server_line);
    }

    uint32_t resolved_ip = 0;
    if (!net_dns_resolve(hostname, &resolved_ip, timer_frequency_hz() * 5)) {
        log_error("nslookup: non-existent domain or server timed out.");
        return;
    }

    char ip_text[16];
    net_format_ipv4(resolved_ip, ip_text);

    char name_line[80];
    uint64_t pos = 0;
    const char *p2 = "Name: ";
    for (uint64_t i = 0; p2[i] != '\0'; ++i) name_line[pos++] = p2[i];
    for (uint64_t i = 0; hostname[i] != '\0'; ++i) name_line[pos++] = hostname[i];
    name_line[pos] = '\0';
    log_info(name_line);

    char addr_line[48];
    pos = 0;
    const char *p3 = "Address: ";
    for (uint64_t i = 0; p3[i] != '\0'; ++i) addr_line[pos++] = p3[i];
    for (uint64_t i = 0; ip_text[i] != '\0'; ++i) addr_line[pos++] = ip_text[i];
    addr_line[pos] = '\0';
    log_info(addr_line);
}

static char curl_response_buffer[NET_HTTP_BUFFER_MAX];

static void command_curl(const char *arguments) {
    if (arguments == 0 || *arguments == '\0') {
        log_error("curl: use curl [-i] <url>");
        return;
    }
    while (*arguments == ' ') {
        ++arguments;
    }
    bool include_headers = false;
    if (arguments[0] == '-' && (arguments[1] == 'i' || arguments[1] == 'I')
        && (arguments[2] == ' ' || arguments[2] == '\0')) {
        include_headers = true;
        arguments += 2;
        while (*arguments == ' ') {
            ++arguments;
        }
    }
    if (*arguments == '\0') {
        log_error("curl: missing URL. Use curl [-i] <url>");
        return;
    }

    char url[256];
    uint64_t url_len = 0;
    while (arguments[url_len] != '\0' && arguments[url_len] != ' ' && url_len + 1 < sizeof(url)) {
        url[url_len] = arguments[url_len];
        ++url_len;
    }
    url[url_len] = '\0';

    uint64_t response_size = 0;
    if (!net_http_get(url, curl_response_buffer, sizeof(curl_response_buffer), &response_size)) {
        log_error("curl: failed to fetch URL (connection, DNS, or HTTP error).");
        return;
    }

    const char *output = curl_response_buffer;
    if (!include_headers) {
        const char *body = 0;
        for (uint64_t i = 0; curl_response_buffer[i] != '\0'; ++i) {
            if (curl_response_buffer[i] == '\r' && curl_response_buffer[i + 1] == '\n'
                && curl_response_buffer[i + 2] == '\r' && curl_response_buffer[i + 3] == '\n') {
                body = &curl_response_buffer[i + 4];
                break;
            }
            if (curl_response_buffer[i] == '\n' && curl_response_buffer[i + 1] == '\n') {
                body = &curl_response_buffer[i + 2];
                break;
            }
        }
        if (body != 0) {
            output = body;
        }
    }

    for (uint64_t index = 0; output[index] != '\0'; ++index) {
        char character = output[index];
        if (character == '\r') {
            continue;
        }
        char printable = (character >= ' ' && character <= '~') || character == '\n' || character == '\t'
            ? character : '?';
        serial_write_char(printable);
        console_write_char(printable);
    }
    uint64_t out_len = 0;
    while (output[out_len] != '\0') {
        ++out_len;
    }
    if (out_len == 0 || output[out_len - 1] != '\n') {
        serial_write_char('\n');
        console_write_char('\n');
    }
}

static void command_httpd(const char *arguments) {
    while (*arguments == ' ') {
        ++arguments;
    }
    if (*arguments == '\0' || (arguments[0] == 's' && arguments[1] == 't' && arguments[2] == 'a' && arguments[3] == 't' && arguments[4] == 'u' && arguments[5] == 's')) {
        if (httpd_is_running()) {
            log_info_u64("httpd: running on port ", httpd_listen_port());
            log_info_u64("  Requests served: ", httpd_requests_served());
        }
        else {
            log_info("httpd: stopped. Use httpd start [port] to launch.");
        }
        return;
    }
    if (arguments[0] == 's' && arguments[1] == 't' && arguments[2] == 'o' && arguments[3] == 'p') {
        httpd_stop();
        return;
    }
    if (arguments[0] == 's' && arguments[1] == 't' && arguments[2] == 'a' && arguments[3] == 'r' && arguments[4] == 't') {
        arguments += 5;
        while (*arguments == ' ') {
            ++arguments;
        }
        uint16_t port = NET_HTTP_PORT;
        if (*arguments >= '0' && *arguments <= '9') {
            uint32_t val = 0;
            while (*arguments >= '0' && *arguments <= '9') {
                val = val * 10 + (uint32_t)(*arguments++ - '0');
                if (val > 65535) {
                    log_error("httpd: invalid port.");
                    return;
                }
            }
            if (val > 0) {
                port = (uint16_t)val;
            }
        }
        if (!httpd_start(port)) {
            log_error("httpd: failed to start web server.");
        }
        return;
    }
    log_error("httpd: use httpd start [port], httpd stop, or httpd status.");
}

static const char *tcp_state_name(enum tcp_state state) {
    switch (state) {
        case TCP_STATE_LISTEN: return "LISTEN";
        case TCP_STATE_SYN_SENT: return "SYN_SENT";
        case TCP_STATE_SYN_RECEIVED: return "SYN_RECV";
        case TCP_STATE_ESTABLISHED: return "ESTABLISHED";
        case TCP_STATE_FIN_WAIT_1: return "FIN_WAIT1";
        case TCP_STATE_FIN_WAIT_2: return "FIN_WAIT2";
        case TCP_STATE_CLOSE_WAIT: return "CLOSE_WAIT";
        case TCP_STATE_LAST_ACK: return "LAST_ACK";
        case TCP_STATE_TIME_WAIT: return "TIME_WAIT";
        case TCP_STATE_CLOSED: return "CLOSED";
        default: return "UNKNOWN";
    }
}

static void format_endpoint(uint32_t ip, uint16_t port, char *buf, uint64_t max_len) {
    char ip_str[16];
    if (ip == 0) {
        ip_str[0] = '*';
        ip_str[1] = '\0';
    }
    else {
        net_format_ipv4(ip, ip_str);
    }
    uint64_t pos = 0;
    for (uint64_t i = 0; ip_str[i] != '\0' && pos + 1 < max_len; ++i) {
        buf[pos++] = ip_str[i];
    }
    if (pos + 1 < max_len) {
        buf[pos++] = ':';
    }
    if (port == 0) {
        if (pos + 1 < max_len) buf[pos++] = '*';
    }
    else {
        uint32_t p = port;
        uint32_t digits = 0;
        char tmp[8];
        do {
            tmp[digits++] = (char)('0' + (p % 10));
            p /= 10;
        } while (p > 0);
        for (uint32_t d = 0; d < digits && pos + 1 < max_len; ++d) {
            buf[pos++] = tmp[digits - 1 - d];
        }
    }
    buf[pos] = '\0';
}

static void command_netstat(const char *arguments) {
    (void)arguments;
    log_info("Active Internet connections");
    log_info("Proto  Local Address          Foreign Address        State");

    struct net_tcp_socket_info sockets[NET_TCP_SOCKET_MAX];
    uint64_t count = net_tcp_get_sockets(sockets, NET_TCP_SOCKET_MAX);
    for (uint64_t i = 0; i < count; ++i) {
        char line[80];
        char local_ep[24];
        char remote_ep[24];
        format_endpoint(sockets[i].local_ip, sockets[i].local_port, local_ep, sizeof(local_ep));
        format_endpoint(sockets[i].remote_ip, sockets[i].remote_port, remote_ep, sizeof(remote_ep));

        uint64_t pos = 0;
        const char *proto = "tcp    ";
        for (uint64_t c = 0; proto[c] != '\0'; ++c) line[pos++] = proto[c];

        for (uint64_t c = 0; local_ep[c] != '\0'; ++c) line[pos++] = local_ep[c];
        while (pos < 30) line[pos++] = ' ';

        for (uint64_t c = 0; remote_ep[c] != '\0'; ++c) line[pos++] = remote_ep[c];
        while (pos < 53) line[pos++] = ' ';

        const char *st = tcp_state_name(sockets[i].state);
        for (uint64_t c = 0; st[c] != '\0'; ++c) line[pos++] = st[c];
        line[pos] = '\0';

        log_info(line);
    }
    if (count == 0) {
        log_info("  (no active TCP sockets)");
    }
}

static void command_echo(const char *arguments) {
    log_info(arguments);
}

static void command_reboot(const char *arguments) {
    (void)arguments;
    log_info("Rebooting through the PS/2 controller.");
    cpu_reboot();
}

static void command_irqinfo(const char *arguments) {
    (void)arguments;
    log_info("irqinfo");
    log_info_u64("  IRQ0 timer dispatches: ", irq_dispatch_count(KOS_PIC_IRQ_TIMER));
    log_info_u64("  IRQ0 timer unhandled: ", irq_unhandled_count(KOS_PIC_IRQ_TIMER));
    log_info_u64("  IRQ1 keyboard dispatches: ", irq_dispatch_count(KOS_PIC_IRQ_KEYBOARD));
    log_info_u64("  IRQ1 keyboard unhandled: ", irq_unhandled_count(KOS_PIC_IRQ_KEYBOARD));
}

static void command_kbdinfo(const char *arguments) {
    (void)arguments;
    log_info("kbdinfo");
    log_info_u64("  Pending characters: ", keyboard_pending_count());
    log_info_u64("  Dropped characters: ", keyboard_dropped_char_count());
}

static void command_drivers(const char *arguments) {
    (void)arguments;
    log_info("drivers");
    for (uint64_t index = 0; index < driver_count(); ++index) {
        const struct driver *driver = driver_at(index);
        if (driver != 0) {
            log_info(driver->name);
        }
    }
}

static void command_lspci(const char *arguments) {
    (void)arguments;
    log_info("lspci");
    for (uint64_t index = 0; index < pci_device_count(); ++index) {
        const struct pci_device *device = pci_device_at(index);
        if (device == 0) {
            continue;
        }
        uint64_t bdf = ((uint64_t)device->bus << 16) | ((uint64_t)device->device << 8)
            | device->function;
        uint64_t class_info = ((uint64_t)device->class_code << 16)
            | ((uint64_t)device->subclass << 8) | device->programming_interface;
        log_info_hex("  BDF: ", bdf);
        log_info_hex("    Vendor ID: ", device->vendor_id);
        log_info_hex("    Device ID: ", device->device_id);
        log_info_hex("    Class/Subclass/ProgIF: ", class_info);
        log_info_u64("    IRQ line: ", device->interrupt_line);
    }
}

static void command_taskinfo(const char *arguments) {
    (void)arguments;
    log_info("taskinfo");
    log_info_u64("  Known tasks: ", task_count());
    log_info_u64("  Ready tasks: ", task_ready_count());
    log_info("  Scheduler: cooperative round-robin; timer requests reschedule.");
}

static void command_tasktest(const char *arguments) {
    (void)arguments;
    if (task_run_self_test()) {
        log_info("tasktest passed: two kernel tasks each yielded 32 times.");
    }
    else {
        log_error("tasktest failed: scheduler is busy or context switch failed.");
    }
}

static void command_taskdemo(const char *arguments) {
    (void)arguments;
    if (task_run_log_demo()) {
        log_info("taskdemo passed: keyboard IRQ remained enabled while tasks alternated.");
    }
    else {
        log_error("taskdemo failed: scheduler is busy or task creation failed.");
    }
}

static bool command_parse_volume(const char *arguments, char *letter) {
    if (arguments == 0 || letter == 0) {
        return false;
    }
    if (*arguments == '\0') {
        *letter = 'C';
        return true;
    }
    if (arguments[1] != ':' || arguments[2] != '\0') {
        return false;
    }
    *letter = arguments[0] >= 'a' && arguments[0] <= 'z'
        ? (char)(arguments[0] - ('a' - 'A')) : arguments[0];
    return vfs_volume_is_mounted(*letter);
}

static void command_vol(const char *arguments) {
    (void)arguments;
    log_info("vol");
    for (uint64_t index = 0; index < vfs_volume_count(); ++index) {
        struct vfs_volume_info info;
        if (!vfs_volume_info_at(index, &info)) {
            continue;
        }
        char name[] = { info.letter, ':', '\0' };
        log_info(name);
        log_info("  Label:");
        log_info(info.label);
        log_info_u64("  Files: ", info.file_count);
    }
}

static void command_ls(const char *arguments) {
    char letter;
    if (!command_parse_volume(arguments, &letter)) {
        log_error("ls: use a mounted volume, for example C: or D:.");
        return;
    }
    char name[] = { letter, ':', '\\', '\0' };
    log_info(name);
    for (uint64_t index = 0; index < vfs_file_count_on_volume(letter); ++index) {
        const struct vfs_file *file = vfs_file_at_on_volume(letter, index);
        if (file != 0) {
            log_info(file->path);
        }
    }
}

static void command_cat(const char *arguments) {
    const uint8_t *data;
    uint64_t size;
    if (*arguments == '\0' || !vfs_read_file(arguments, &data, &size)) {
        log_error("cat: file not found.");
        return;
    }
    for (uint64_t index = 0; index < size; ++index) {
        char character = data[index] >= ' ' && data[index] <= '~' ? (char)data[index]
            : data[index] == '\n' ? '\n' : '?';
        serial_write_char(character);
        console_write_char(character);
    }
    if (size == 0 || data[size - 1] != '\n') {
        serial_write_char('\n');
        console_write_char('\n');
    }
}

static void command_elfinfo(const char *arguments) {
    const uint8_t *data;
    uint64_t size;
    struct elf_image_info info;
    if (*arguments == '\0' || !vfs_read_file(arguments, &data, &size)) {
        log_error("elfinfo: file not found.");
        return;
    }
    if (!elf_inspect_image(data, size, &info)) {
        log_error("elfinfo: not a supported x86_64 ELF image.");
        return;
    }
    log_info_hex("ELF entry: ", info.entry_point);
    log_info_u64("ELF program headers: ", info.program_header_count);
    log_info_u64("ELF loadable segments: ", info.loadable_segment_count);
}

static const struct command_entry command_registry[] = {
    { "help", command_help },
    { "clear", command_clear },
    { "meminfo", command_meminfo },
    { "uptime", command_uptime },
    { "sysinfo", command_sysinfo },
    { "netinfo", command_netinfo },
    { "ifconfig", command_ifconfig },
    { "arp", command_arp },
    { "arping", command_arping },
    { "ping", command_ping },
    { "dhcp", command_dhcp },
    { "udptest", command_udptest },
    { "tcptest", command_tcptest },
    { "nslookup", command_nslookup },
    { "curl", command_curl },
    { "httpd", command_httpd },
    { "netstat", command_netstat },
    { "echo", command_echo },
    { "reboot", command_reboot },
    { "irqinfo", command_irqinfo },
    { "kbdinfo", command_kbdinfo },
    { "drivers", command_drivers },
    { "lspci", command_lspci },
    { "taskinfo", command_taskinfo },
    { "tasktest", command_tasktest },
    { "taskdemo", command_taskdemo },
    { "vol", command_vol },
    { "ls", command_ls },
    { "cat", command_cat },
    { "elfinfo", command_elfinfo },
};

bool kernel_command_execute(const char *command) {
    if (command == 0) {
        return false;
    }
    while (*command == ' ') {
        ++command;
    }
    const char *name = command;
    while (*command != '\0' && *command != ' ') {
        ++command;
    }
    uint64_t name_length = (uint64_t)(command - name);
    while (*command == ' ') {
        ++command;
    }

    for (uint64_t index = 0; index < sizeof(command_registry) / sizeof(command_registry[0]); ++index) {
        const char *registered_name = command_registry[index].name;
        uint64_t registered_length = 0;
        while (registered_name[registered_length] != '\0') {
            ++registered_length;
        }
        if (registered_length == name_length) {
            bool matches = true;
            for (uint64_t character = 0; character < name_length; ++character) {
                if (name[character] != registered_name[character]) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
                command_registry[index].handler(command);
                return true;
            }
        }
    }
    return false;
}
