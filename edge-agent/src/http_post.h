#ifndef HTTP_POST_H
#define HTTP_POST_H

#include <stddef.h>

/* Returns 0 on success, non-zero on failure. */
int http_post_json(const char *url, const char *json_body, long *http_status_out);

#endif
