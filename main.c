#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// those are stored in Read-Only Data Segment .rodata
const char *HTTP_VERSIONS[] = {"HTTP/1.0", "HTTP/1.1", "HTTP/2", "HTTP/3"};
const char *HTTP_METHODS[] = {"GET", "HEAD", "POST", "PUT", "DELETE",
                              "CONNECT", "OPTIONS", "TRACE", "PATCH"};

typedef enum
{
    METHOD,
    VERSION
} ValidatorType;

typedef enum
{
    QUERY,
    HEADER
} ArrayType;

typedef struct
{
    char *key;
    char *val;
} Entity;

typedef struct
{
    Entity *data;
    int capacity;
    int count;
} Array;

typedef struct
{
    char *method;
    char *path;
    char *version;
    char *body;
    int body_length;
    Array *header;
    Array *query;
} HttpReq;

HttpReq *http_req_parser_init()
{
    HttpReq *http = malloc(sizeof(HttpReq));
    if (!http)
    {
        fprintf(stderr, "ERROR http_req_parser_init: http malloc failed\n");

        return NULL;
    }
    http->body_length = 0;
    http->version = NULL;
    http->method = NULL;
    http->body = NULL;
    http->path = NULL;
    http->header = malloc(sizeof(Array));
    if (!http->header)
    {
        fprintf(stderr, "ERROR http_req_parser_init: header malloc failed\n");

        return NULL;
    }
    http->header->capacity = 4;
    http->header->count = 0;
    http->header->data = malloc(http->header->capacity * sizeof(Entity));
    if (!http->header->data)
    {
        fprintf(stderr, "ERROR http_req_parser_init: header->data malloc failed\n");

        return NULL;
    }
    http->query = malloc(sizeof(Array));
    if (!http->query)
    {
        fprintf(stderr, "ERROR http_req_parser_init: query malloc failed\n");

        return NULL;
    }
    http->query->capacity = 4;
    http->query->count = 0;
    http->query->data = malloc(http->query->capacity * sizeof(Entity));
    if (!http->query->data)
    {
        fprintf(stderr, "ERROR http_req_parser_init: query->data malloc failed\n");

        return NULL;
    }
    return http;
}

void http_req_parser_free(HttpReq *http)
{
    if (http)
    {
        if (http->body)
            free(http->body);
        if (http->version)
            free(http->version);
        if (http->method)
            free(http->method);
        if (http->path)
            free(http->path);
        if (http->query->data)
        {
            int query_length = http->query->count;
            int i;
            for (i = 0; i < query_length; i++)
            {
                free(http->query->data[i].key);
                free(http->query->data[i].val);
            }
            free(http->query->data);
            free(http->query);
        }
        if (http->header->data)
        {
            int header_length = http->header->count;
            int i;
            for (i = 0; i < header_length; i++)
            {
                free(http->header->data[i].key);
                free(http->header->data[i].val);
            }
            free(http->header->data);
            free(http->header);
        }
        free(http);
    }
}

Array *http_req_parser_arr_selector(HttpReq *http, ArrayType type)
{
    switch (type)
    {
    case QUERY:
        return http->query;
    case HEADER:
        return http->header;
    default:
        return NULL;
    }
}

int http_req_parser_print_arr(HttpReq *http, ArrayType type)
{
    Array *arr = http_req_parser_arr_selector(http, type);
    if (!arr)
    {
        fprintf(stderr, "ERROR http_req_parser_print_arr: array selector returned NULL\n");
        return 0;
    }
    const char *label = (type == QUERY) ? "Query" : "Header";

    if (arr && arr->data && arr->count > 0)
    {
        for (int i = 0; i < arr->count; i++)
        {
            printf("\n\n %s (key,value): (%s,%s)", label, arr->data[i].key,
                   arr->data[i].val);
        }
    }
    return 1;
}
bool http_req_parser_validator(const char *target, ValidatorType ValidatonType)
{
    const char **lookup =
        ValidatonType == VERSION
            ? HTTP_VERSIONS
            : HTTP_METHODS; // it doesn't work as **lookup[] learn the reason why.
    int arr_length = ValidatonType == VERSION
                         ? sizeof(HTTP_VERSIONS) / sizeof(HTTP_VERSIONS[0])
                         : sizeof(HTTP_METHODS) / sizeof(HTTP_METHODS[0]);
    int i;
    for (i = 0; i < arr_length; i++)
    {
        if (strcmp(lookup[i], target) == 0)
        {
            return true;
        }
    }
    return false;
}

bool http_req_parser_is_str_contains_digit(const char *str)
{
    if (str == NULL || *str == '\0')
        return false;

    while (isspace((unsigned char)*str))
        str++;                                      // trim left spaces
    size_t digit_count = strspn(str, "0123456789"); // digit el lengths

    return digit_count == strlen(str);
}

void http_req_parser_print(HttpReq *http)
{

    printf("\n\n Method: %s", http->method ? http->method : "(EMPTY)");
    printf("\n\n Path: %s", http->path ? http->path : "(EMPTY)");
    printf("\n\n Version: %s", http->version ? http->version : "(EMPTY)");
    printf("\n\n Body: %s", http->body ? http->body : "(EMPTY)");
    printf("\n\n Body_Length: %d", http->body_length ? http->body_length : 0);
    http_req_parser_print_arr(http, QUERY);
    http_req_parser_print_arr(http, HEADER);
}

bool http_req_parser_parse_req_line(HttpReq *http, const char *cursor,
                                    int length)
{
    int method_length = strcspn(cursor, " ");
    char *temp_method = strndup(cursor, method_length);
    bool is_method_valid = http_req_parser_validator(temp_method, METHOD);
    if (!is_method_valid)
    {
        free(temp_method);
        return false;
    }
    http->method = temp_method;

    int path_length = strcspn(cursor + method_length + 1, " ");
    http->path = strndup(cursor + method_length + 1, path_length);

    int version_length =
        length -
        (path_length + method_length) - 2; // length = req_line total length (substr)
    // - (previously extracted length)
    char *temp_version =
        strndup(cursor + path_length + method_length + 2, version_length);
    bool is_version_valid = http_req_parser_validator(temp_version, VERSION);
    if (!is_version_valid)
    {
        free(temp_version);
        return false;
    }
    http->version = temp_version;
    return true;
}

void http_req_parser_parse_body(HttpReq *http, const char *cursor)
{
    if (http->body_length >= 1)
    {
        http->body = strndup(cursor, http->body_length);
    }
}

void http_req_parser_parse_query(HttpReq *http)
{
    char *query = strstr(http->path, "?");
    if (query != NULL)
    {
        //* STEP_1 wipe param from the path
        // example[i] = '\0' I don't implement the truncation solution because
        // after removing path it must free from the memory
        // After:   [/][s][e][a][r][c][h][\0][q][=][1][\0]   ← 12 bytes allocated, strlen=7 -> is it vulnerable what if after the first null terminator there is a JWT token?
        // learn it for wiping sensitive data along with what happens freed data in the heap? what is actual life cycle? -> explicit_bzero (consider long_live http connections like websockets)
        size_t full_len = strlen(http->path);
        size_t query_len = strlen(query);
        size_t path_len = full_len - query_len;

        // dup-then-replace pattern
        char *old_path = http->path;
        http->path = strndup(old_path, path_len);
        /*

        !BUG for learning
        *query = *(query + 1); // pass ? % e.q. ?xx=1 -> xx=1
        Before: [?][z][=][1][\0]
                 q
        After:  [z][z][=][1][\0]
                    q
        That doesn't skip ? it overwrites ? with the next character.

        */
        //* STEP_2 parse query
        query++; // skip the ?
        while (*query)
        {
            int length = strcspn(query, "&");
            if (http->query->count == http->query->capacity)
            {
                int new_capacity = http->query->capacity * 2;
                Entity *tmp = realloc(http->query->data, new_capacity * sizeof(Entity));
                if (!tmp)
                {
                    return;
                }
                http->query->data = tmp;
                http->query->capacity = new_capacity;
            }
            char *eq = memchr(query, '=', length);
            // Two shapes a query pair can take: key=value or key with no value
            if (eq)
            {
                int key_length = strcspn(query, "=");
                http->query->data[http->query->count].key = strndup(query, key_length);
                http->query->data[http->query->count].val = strndup(query + key_length + 1, length - key_length - 1);
                http->query->count++;
            }
            else
            {
                int key_length = strcspn(query, "=");
                http->query->data[http->query->count].key = strndup(query, key_length);
                http->query->data[http->query->count].val = NULL;
                http->query->count++;
            }
            query += length;
            if (*query == '\0')
            {
                break;
            }
            query += 1; // offset of &
        }
    }
}

bool http_req_parser_parse(HttpReq *http, char *src)
{
    char *cursor = src;
    int state = 0;
    char *is_header_valid = strstr(src, "\r\n\r\n");
    if (!is_header_valid)
    {
        fprintf(stderr, "ERROR http_req_parser_parse: invalid header (missing \\r\\n\\r\\n terminator)\n");

        return false;
    }

    while (*cursor)
    {
        int length = strcspn(cursor, "\r\n");
        // req_line segment
        if (state == 0)
        {
            bool is_req_line_successed =
                http_req_parser_parse_req_line(http, cursor, length);
            if (is_req_line_successed == false)
            {
                fprintf(stderr, "ERROR http_req_parser_parse: invalid header (missing \\r\\n\\r\\n terminator)\n");

                return false;
            }
        }
        if (state >= 1 && length == 0)
        {
            cursor += strspn(cursor, "\r\n");
            break;
        }
        // header segment
        if (state >= 1 && length > 0)
        {
            int header_length = strcspn(cursor, "\r\n");
            char *str_p = strndup(cursor, header_length);
            int key_length = strcspn(str_p, ":");
            if (http->header->capacity == http->header->count)
            {
                http->header->capacity *= 2;
                http->header->data = realloc(http->header->data,
                                             http->header->capacity * sizeof(Entity));
                if (!http->header->data)
                {
                    free(str_p);
                    fprintf(stderr, "ERROR http_req_parser_parse: header->data realloc failed\n");
                    return false;
                }
            }
            int index = http->header->count;
            http->header->data[index].key = strndup(str_p, key_length);
            char *val_start = str_p + key_length + 1;
            while (isspace((unsigned char)*val_start))
                val_start++;
            http->header->data[index].val =
                strdup(val_start);

            if (strcmp(http->header->data[index].key, "Content-Length") == 0)
            {
                bool is_digit = http_req_parser_is_str_contains_digit(
                    http->header->data[index].val);
                if (is_digit == 0)
                {
                    fprintf(stderr, "http_req_parser_parse: Content-Length value '%s' is not numeric\n",
                            http->header->data[index].val);
                }
                http->body_length = atoi(http->header->data[index].val);
            }
            /*          printf("KEY=[%s] VAL=[%s] VAL_LEN=%zu\n",
                            http->header->data[index].key,
                            http->header->data[index].val,
                            strlen(http->header->data[index].val)); */
            http->header->count++;
            free(str_p);
        }
        state++;
        cursor += length;
        cursor += 2; // jump to next segment
    }
    http_req_parser_parse_body(http, cursor);
    if (http->path)
    {
        http_req_parser_parse_query(http);
    }

    return true;
}

int main()
{

    // Test Q1 — path > query
    // path: /api/v2/users/profile (21 chars), query: id=1 (4 chars)
    HttpReq *http_q1 = http_req_parser_init();
    char req_q1[] = "GET /api/v2/users/profile?id=1 HTTP/1.1\r\nHost:api.com\r\n\r\n";
    printf("\n\n --- Test Q1 path>query --- \n");
    bool test_q1 = http_req_parser_parse(http_q1, req_q1);
    if (test_q1)
    {
        http_req_parser_print(http_q1);
    }
    // Expected:
    // Method: GET
    // Path: /api/v2/users/profile
    // Version: HTTP/1.1
    // Query (key,value): (id,1)
    // Header (key,value): (Host,api.com)
    // Body: (EMPTY)

    // Test Q2 — query > path
    // path: /s (2 chars), query: q=looooong&filter=active&sort=date&page=1 (40 chars)
    HttpReq *http_q2 = http_req_parser_init();
    char req_q2[] = "GET /s?q=looooong&filter=active&sort=date&page=1 HTTP/1.1\r\nHost:s.com\r\n\r\n";
    printf("\n\n --- Test Q2 query>path --- \n");
    bool test_q2 = http_req_parser_parse(http_q2, req_q2);
    if (test_q2)
    {
        http_req_parser_print(http_q2);
    }

    // Expected:
    // Method: GET
    // Path: /s
    // Version: HTTP/1.1
    // Query (key,value): (q,looooong)
    // Query (key,value): (filter,active)
    // Query (key,value): (sort,date)
    // Query (key,value): (page,1)
    // Header (key,value): (Host,s.com)
    // Body: (EMPTY)

    // Test Q3 — path == query (both 7 chars)
    // path: /search (7 chars), query: q=hello (7 chars)
    HttpReq *http_q3 = http_req_parser_init();
    char req_q3[] = "GET /search?q=hello HTTP/1.1\r\nHost:s.com\r\n\r\n";
    printf("\n\n --- Test Q3 path==query --- \n");
    bool test_q3 = http_req_parser_parse(http_q3, req_q3);
    if (test_q3)
    {
        http_req_parser_print(http_q3);
    }
    // Expected:
    // Method: GET
    // Path: /search
    // Version: HTTP/1.1
    // Query (key,value): (q,hello)
    // Header (key,value): (Host,s.com)
    // Body: (EMPTY)

    // Test Q4 — single-char path, single-char query
    // path: /a (2 chars), query: x=1 (3 chars) — minimum non-trivial case
    HttpReq *http_q4 = http_req_parser_init();
    char req_q4[] = "GET /a?x=1 HTTP/1.1\r\nHost:s.com\r\n\r\n";
    printf("\n\n --- Test Q4 minimal --- \n");
    bool test_q4 = http_req_parser_parse(http_q4, req_q4);
    if (test_q4)
    {
        http_req_parser_print(http_q4);
    }
    // Expected:
    // Method: GET
    // Path: /a
    // Version: HTTP/1.1
    // Query (key,value): (x,1)
    // Header (key,value): (Host,s.com)
    // Body: (EMPTY)

    // Test Q5 — extreme imbalance, very long path, tiny query
    // Forces both that path length math is independent of query, and that you don't truncate
    HttpReq *http_q5 = http_req_parser_init();
    char req_q5[] = "GET /a/b/c/d/e/f/g/h/i/j/k/l/m/n?z=1 HTTP/1.1\r\nHost:s.com\r\n\r\n";
    printf("\n\n --- Test Q5 long-path/short-query --- \n");
    bool test_q5 = http_req_parser_parse(http_q5, req_q5);
    if (test_q5)
    {
        http_req_parser_print(http_q5);
    }

    // Expected:
    // Method: GET
    // Path: /a/b/c/d/e/f/g/h/i/j/k/l/m/n
    // Version: HTTP/1.1
    // Query (key,value): (z,1)
    // Header (key,value): (Host,s.com)
    // Body: (EMPTY)
    http_req_parser_free(http_q1);
    http_req_parser_free(http_q2);
    http_req_parser_free(http_q3);
    http_req_parser_free(http_q4);
    http_req_parser_free(http_q5);
    return 0;
}
