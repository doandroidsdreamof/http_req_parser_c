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
        printf("\n\n ERR: http_req_parser_init http allocation failed");
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
        printf("\n\n ERR: http_req_parser_init header allocation failed");
        return NULL;
    }
    http->header->capacity = 4;
    http->header->count = 0;
    http->header->data = malloc(http->header->capacity * sizeof(Entity));
    if (!http->header->data)
    {
        printf("\n\n ERR: http_req_parser_init header->data allocation failed");
        return NULL;
    }
    http->query = malloc(sizeof(Array));
    if (!http->query)
    {
        printf("\n\n ERR: http_req_parser_init query allocation failed");
        return NULL;
    }
    http->query->capacity = 4;
    http->query->count = 0;
    http->query->data = malloc(http->query->capacity * sizeof(Entity));
    if (!http->query->data)
    {
        printf("\n\n ERR: http_req_parser_init query->data allocation failed");
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
        printf("\n\n http_req_parser_print_arr arr failed");
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
    /*     if (ValidatonType == VERSION)
        {
          printf("\n\n ---------> target [%s]", target);
        } */
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
    http->method = temp_method; // GET -> 4

    int path_length = strcspn(cursor + method_length + 1, " "); // /index.html -> 11
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

        return false;
    }
    http->version = temp_version;
    return true;
}

int http_req_parser_parse_body(HttpReq *http, const char *cursor)
{
    if (http->body_length < 1)
    {
        return 0;
    }
    http->body = strndup(cursor, http->body_length);
    return 1;
}

void http_req_parser_parse_query(HttpReq *http)
{
}

int http_req_parser_parse(HttpReq *http, char *src)
{
    char *cursor = src;
    int state = 0;
    char *is_header_valid = strstr(src, "\r\n\r\n");
    if (!is_header_valid)
    {
        printf("ERR: http_req_parser_parse ivalid header \n\n");
        return 0;
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
                printf("ERR: http_req_parser_parse req line parsing failed \n\n");
                return 0;
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
                    printf("\n\n ERR: http_req_parser_parse header->data allocation failed");
                    return 0;
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
                    printf("\n\n http_req_parser_parse body_length is not digit");
                }
                http->body_length = atoi(http->header->data[index].val);
            }
            printf("KEY=[%s] VAL=[%s] VAL_LEN=%zu\n",
                   http->header->data[index].key,
                   http->header->data[index].val,
                   strlen(http->header->data[index].val));
            http->header->count++;
            free(str_p);
        }
        state++;
        cursor += length;
        cursor += 2; // jump to next segment
    }
    http_req_parser_parse_body(http, cursor);
    return 1;
}

int main()
{

    HttpReq *http_0 = http_req_parser_init();
    char req4[] = "GET /search?q=hello&page=2 HTTP/1.1\r\nHost:search.com\r\n\r\n";

    printf("\n\n --- Test 0 SAFE --- \n");
    int test_0 = http_req_parser_parse(http_0, req4);
    if (test_0 == 1)
    {
        http_req_parser_print(http_0);
    }

    // Method: GET
    // Path: /search
    // Query[0]: q=hello
    // Query[1]: page=2
    // Version: HTTP/1.1
    // Header[0]: Host: search.com
    // Body: (empty)

    HttpReq *http = http_req_parser_init();

    // Test 2 — GET with multiple headers
    char req2[] = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nAccept: "
                  "text/html\r\nConnection: close\r\nConnection: close\r\n\r\n";
    printf("\n\n --- Test 2 SAFE --- \n");
    int test_1 = http_req_parser_parse(http, req2);
    if (test_1 == 1)
    {
        http_req_parser_print(http);
    }

    HttpReq *http_2 = http_req_parser_init();
    char req3[] =
        "POST /api/users HTTP/1.1\r\nHost: api.example.com\r\nContent-Type: "
        "application/json\r\nContent-Length: 27\r\nHeader: "
        "last\r\n\r\n{\"name\":\"x\",\"active\":true}";
    printf("\n\n --- Test 3 SAFE ---\n");
    int test_2 = http_req_parser_parse(http_2, req3);
    if (test_2 == 1)
    {
        http_req_parser_print(http_2);
    }

    HttpReq *http_3 = http_req_parser_init();

    char req7[] = "GET /broken\r\nHost: bad.com\r\n\r\n";
    printf("\n\n --- Test 7 BROKEN --- \n\n");
    int test_3 = http_req_parser_parse(http_3, req7);
    if (test_3 == 1)
    {
        http_req_parser_print(http_3);
    }
    // Error: malformed request line

    HttpReq *http_4 = http_req_parser_init();
    // Test 8 — Malformed: no \r\n
    char req8[] = "GET / HTTP/1.1\nHost: bad.com\n\n";
    printf("\n\n --- Test 8 BROKEN ---\n\n");
    int test_4 = http_req_parser_parse(http_4, req8);
    if (test_4 == 1)
    {
        http_req_parser_print(http_4);
    }
    // Error: expected \r\n, got \n

    http_req_parser_free(http_0);
    http_req_parser_free(http);
    http_req_parser_free(http_2);
    http_req_parser_free(http_3);
    http_req_parser_free(http_4);

    return 0;
}
