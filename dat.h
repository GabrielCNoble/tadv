#ifndef DAT_H
#define DAT_H

#include "vm.h"
#include <stdint.h>

enum DAT_ATTRIB_TYPE
{
    DAT_ATTRIB_TYPE_INT = 0,
    DAT_ATTRIB_TYPE_FLOAT,
    DAT_ATTRIB_TYPE_STRING,
    DAT_ATTRIB_TYPE_STRUCT,
    DAT_ATTRIB_TYPE_CODE,
};

struct dat_attrib_t
{
    struct dat_attrib_t *next;
    struct dat_attrib_t *prev;
    char *name;
    uint32_t type;

    union
    {
        uint64_t int_data;
        float flt_data;
        char *str_data;
        struct dat_attrib_t *attrib;
        struct code_buffer_t code;
    } data;

};

struct dat_parser_t
{
    uint32_t valid_null;
    struct token_t *tokens;
}; 


struct dat_attrib_t *dat_get_attrib(struct dat_attrib_t *attribs, const char *name);

void dat_del_attrib(struct dat_attrib_t *attribs, const char *name);

void dat_free_attribs(struct dat_attrib_t *attribs);

// void dat_add_attrib(struct dat_t *dat, )

struct dat_attrib_t *dat_parse_dat_string(const char *src);

struct dat_attrib_t *dat_parse(struct dat_parser_t *parser);

struct dat_parser_t dat_init_parser(struct token_t *tokens);

uint32_t dat_next_token(struct dat_parser_t *parser);

#endif