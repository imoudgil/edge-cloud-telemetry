#include "http_post.h"

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct {
    char host[256];
    char path[512];
    int port;
    int use_tls; /* reserved; plain HTTP only in this demo */
} parsed_url_t;

static int parse_http_url(const char *url, parsed_url_t *out) {
    memset(out, 0, sizeof(*out));
    out->port = 80;

    const char *p = url;
    if (strncmp(p, "http://", 7) == 0) {
        p += 7;
        out->use_tls = 0;
    } else {
        return -1;
    }

    const char *slash = strchr(p, '/');
    const char *host_end = slash ? slash : (p + strlen(p));
    size_t host_len = (size_t)(host_end - p);
    if (host_len == 0 || host_len >= sizeof(out->host)) {
        return -1;
    }
    memcpy(out->host, p, host_len);
    out->host[host_len] = '\0';

    const char *colon = strchr(out->host, ':');
    if (colon) {
        *colon = '\0';
        out->port = atoi(colon + 1);
        if (out->port <= 0 || out->port > 65535) {
            return -1;
        }
    }

    if (slash) {
        snprintf(out->path, sizeof(out->path), "%s", slash);
    } else {
        snprintf(out->path, sizeof(out->path), "/");
    }
    return 0;
}

int http_post_json(const char *url, const char *json_body, long *http_status_out) {
    if (http_status_out) {
        *http_status_out = 0;
    }
    if (!url || !json_body) {
        return -1;
    }

    parsed_url_t parsed;
    if (parse_http_url(url, &parsed) != 0) {
        return -1;
    }

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", parsed.port);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *res = NULL;
    int gai = getaddrinfo(parsed.host, port_str, &hints, &res);
    if (gai != 0 || !res) {
        return -1;
    }

    int sock = -1;
    for (struct addrinfo *it = res; it; it = it->ai_next) {
        sock = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (sock < 0) {
            continue;
        }
        if (connect(sock, it->ai_addr, it->ai_addrlen) == 0) {
            break;
        }
        close(sock);
        sock = -1;
    }
    freeaddrinfo(res);
    if (sock < 0) {
        return -1;
    }

    size_t body_len = strlen(json_body);
    char req[4096];
    int req_len = snprintf(
        req,
        sizeof(req),
        "POST %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Connection: close\r\n"
        "Content-Length: %zu\r\n"
        "\r\n"
        "%s",
        parsed.path,
        parsed.host,
        body_len,
        json_body);

    if (req_len < 0 || (size_t)req_len >= sizeof(req)) {
        close(sock);
        return -1;
    }

    ssize_t sent = send(sock, req, (size_t)req_len, 0);
    if (sent != req_len) {
        close(sock);
        return -1;
    }

    char resp[1024];
    ssize_t n = recv(sock, resp, sizeof(resp) - 1, 0);
    close(sock);
    if (n <= 0) {
        return -1;
    }
    resp[n] = '\0';

    long status = 0;
    if (sscanf(resp, "HTTP/1.%*d %ld", &status) == 1 && http_status_out) {
        *http_status_out = status;
    }
    if (status >= 200 && status < 300) {
        return 0;
    }
    return -1;
}
