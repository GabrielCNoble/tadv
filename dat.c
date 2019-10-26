#include "dat.h"
#include "vm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct dat_attrib_t *dat_get_attrib_recursive(struct dat_attrib_t *attribs, struct vm_lexer_t *lexer)
{
    if(lexer->token.token_class == TOKEN_CLASS_IDENTIFIER)
    {
        while(attribs)
        {
            if(!strcmp((char *)lexer->token.constant.ptr_constant, attribs->name))
            {
                vm_lex_one_token(lexer);

                if(lexer->token.token_class == TOKEN_CLASS_PUNCTUATOR && 
                   lexer->token.token_type == TOKEN_PUNCTUATOR_DOT)
                {
                    if(attribs->type == DAT_ATTRIB_TYPE_STRUCT)
                    {
                        vm_lex_one_token(lexer);
                        return dat_get_attrib_recursive(attribs->data.attrib, lexer);
                    }
                }

                return attribs;
            }

            attribs = attribs->next;    
        }
    }
    return NULL;
}

struct dat_attrib_t *dat_get_attrib(struct dat_attrib_t *attribs, const char *name)
{
    // struct token_t *tokens;
    // struct token_t *token;
    struct dat_attrib_t *ret = NULL;
    struct vm_lexer_t lexer;

    if(attribs)
    {
        vm_init_lexer(&lexer, name);
        vm_lex_one_token(&lexer);
        ret = dat_get_attrib_recursive(attribs, &lexer);
    }

    return ret;
}

void dat_del_attrib(struct dat_attrib_t *attribs, const char *name)
{

}

void dat_free_attribs(struct dat_attrib_t *attribs)
{
    struct dat_attrib_t *next_attrib;

    while(attribs)
    {
        if(attribs->type == DAT_ATTRIB_TYPE_STRUCT)
        {
            dat_free_attribs(attribs->data.attrib);
        }

        next_attrib = attribs->next;
        if(attribs->name)
        {
            free(attribs->name);
        }
        free(attribs);
        attribs = next_attrib;
    }
}


 


struct dat_parser_t dat_init_parser(const char *src)
{
    struct dat_parser_t parser;

    // parser.tokens = tokens;
    parser.valid_null = 1;
    vm_init_lexer(&parser.lexer, src);

    return parser;
}

uint32_t dat_next_token(struct dat_parser_t *parser)
{
    // parser->tokens = parser->tokens->next;
    vm_lex_one_token(&parser->lexer);
    return parser->lexer.token.token_class == TOKEN_CLASS_UNKNOWN && !parser->valid_null;
}

struct dat_attrib_t *dat_parse_dat_string(const char *src)
{
    struct dat_attrib_t *attribs = NULL;
    struct dat_attrib_t *attrib = NULL;
    // struct token_t *tokens;
    // struct token_t *token;
    const char *error;

    struct dat_parser_t parser;

    // tokens = vm_lex_code(src);
    // token = tokens;

    // parser = dat_init_parser(vm_lex_code(src));
    parser = dat_init_parser(src);
    dat_next_token(&parser);

    while(parser.lexer.token.token_class != TOKEN_CLASS_UNKNOWN)
    {
        attrib = calloc(1, sizeof(struct dat_attrib_t ));
        attrib->type = DAT_ATTRIB_TYPE_STRUCT;
        attrib->data.attrib = dat_parse(&parser);
        
        // printf("stuck\n");

        if(!attrib->data.attrib)
        {
            // printf("%s\n", vm_get_error());
            while((error = vm_get_error()))
            {
                printf("%s\n", error);
            }

            return NULL;
        }

        attrib->next = attribs;
        attribs = attrib;
    }

    // while(tokens)
    // {
    //     token = tokens->next;
    //     vm_free_token(tokens);
    //     tokens = token;
    // }

    return attribs;
}

#define UNEXPECTED_END_REACHED "unexpected end of tokens reached"
struct dat_attrib_t *dat_parse(struct dat_parser_t *parser)
{
    struct dat_attrib_t *attrib = NULL;
    struct dat_attrib_t *attribs = NULL;
    // struct token_t *token;
    // struct token_t *before_code;
    // struct token_t *prev_token;
    uint32_t code_start_offset;
    char *code;

    // free()
    // struct token_t *code;
    // char *attrib_name;
    uint32_t has_opening_brace = 0;
    

    if(parser->lexer.token.token_class == TOKEN_CLASS_UNKNOWN)
    {
        return NULL;
    }

    if(parser->lexer.token.token_class != TOKEN_CLASS_IDENTIFIER)
    {
        if(parser->lexer.token.token_class == TOKEN_CLASS_PUNCTUATOR &&
           parser->lexer.token.token_type == TOKEN_PUNCTUATOR_OBRACE)
        {
            // token = token->next;
            if(dat_next_token(parser))
            {
                vm_set_last_error(UNEXPECTED_END_REACHED);
                return NULL;
            }

            has_opening_brace = 1;
        }
        else
        {
            // printf("error: unexpected token at the beginning of a block attribute\n");
            vm_set_last_error("error: expecting '{' or identifier, got '%s'", vm_translate_token(&parser->lexer.token)); 
            return NULL;
        }
    }

    while(parser->lexer.token.token_class != TOKEN_CLASS_PUNCTUATOR || 
          parser->lexer.token.token_type != TOKEN_PUNCTUATOR_CBRACE)
    {
        parser->valid_null = 0;

        if(parser->lexer.token.token_class != TOKEN_CLASS_IDENTIFIER)
        {
            // printf("error: expected attribute identifier\n");
            vm_set_last_error("error: expecting attribute identifier, got '%s'", vm_translate_token(&parser->lexer.token));
            goto _free_attribs;
        }

        attrib = calloc(1, sizeof(struct dat_attrib_t));
        attrib->name = strdup((char *)parser->lexer.token.constant.ptr_constant);
        attrib->next = attribs;
        attribs = attrib;

        if(dat_next_token(parser))
        {
            vm_set_last_error("error: unexpected end reached after attribute '%s'", attrib->name);
            goto _free_attribs;
        }

        if(parser->lexer.token.token_class != TOKEN_CLASS_PUNCTUATOR ||
           parser->lexer.token.token_type != TOKEN_PUNCTUATOR_EQUAL)
        {
            vm_set_last_error("error: expecting a '=' after attribute '%s', got '%s'", attrib->name, vm_translate_token(&parser->lexer.token));
            goto _free_attribs;
        }

        if(dat_next_token(parser))
        {
            vm_set_last_error("error: unexpected end reached after '=' for attribute '%s'", attrib->name);
            goto _free_attribs;
        }


        switch(parser->lexer.token.token_class)
        {
            case TOKEN_CLASS_INTEGER_CONSTANT:
                attrib->type = DAT_ATTRIB_TYPE_INT;
                attrib->data.int_data = parser->lexer.token.constant.uint_constant;

                if(dat_next_token(parser))
                {
                    vm_set_last_error("error: unexpected end reached after attribute definition '%s'", attrib->name);
                    goto _free_attribs;
                }
            break;

            case TOKEN_CLASS_FLOAT_CONSTANT:
                attrib->type = DAT_ATTRIB_TYPE_FLOAT;
                attrib->data.flt_data = parser->lexer.token.constant.flt_constant;

                if(dat_next_token(parser))
                {
                    vm_set_last_error("error: unexpected end reached after attribute definition '%s'", attrib->name);
                    goto _free_attribs;
                }
            break;

            case TOKEN_CLASS_STRING_CONSTANT:
                attrib->type = DAT_ATTRIB_TYPE_STRING;
                attrib->data.str_data = strdup((char *)parser->lexer.token.constant.ptr_constant);

                if(dat_next_token(parser))
                {
                    vm_set_last_error("error: unexpected end reached after attribute definition '%s'", attrib->name);
                    goto _free_attribs;
                }
            break;

            case TOKEN_CLASS_PUNCTUATOR:
                if(parser->lexer.token.token_type == TOKEN_PUNCTUATOR_OBRACE)
                {
                    attrib->type = DAT_ATTRIB_TYPE_STRUCT;
                    attrib->data.attrib = dat_parse(parser);
                    if(!attrib->data.attrib)
                    {
                        goto _free_attribs;
                    }
                    parser->valid_null = 0;
                }
                else if(parser->lexer.token.token_type == TOKEN_PUNCTUATOR_OPARENTHESIS)
                {
                    attrib->type = DAT_ATTRIB_TYPE_CODE;

                    if(dat_next_token(parser))
                    {
                        vm_set_last_error("error: unexpected end reached before code defininion for attribute '%s'", attrib->name);
                        goto _free_attribs;
                    }

                    code_start_offset = parser->lexer.prev_offset;

                    while(parser->lexer.token.token_class != TOKEN_CLASS_PUNCTUATOR ||
                          parser->lexer.token.token_type != TOKEN_PUNCTUATOR_CPARENTHESIS)
                    {
                        // prev_token = parser->lexer.token;

                        if(dat_next_token(parser))
                        {
                            vm_set_last_error("error: unexpected end reached while reading code for attribute '%s'", attrib->name);
                            goto _free_attribs;
                        }
                    }

                    code = calloc(1, parser->lexer.prev_offset - code_start_offset + 1);
                    strncpy(code, parser->lexer.src + code_start_offset, parser->lexer.prev_offset - code_start_offset);
                    code[parser->lexer.prev_offset - code_start_offset] = '\0';

                    // printf("(%s)\n", code);

                    if(vm_assemble_code(&attrib->data.code, code))
                    {
                        vm_set_last_error("error: error while compiling code for attribute '%s'", attrib->name);
                        goto _free_attribs;
                    }

                    free(code);

                    if(dat_next_token(parser))
                    {
                        vm_set_last_error("error: unexpected end reached after code attribute '%s'", attrib->name);
                        goto _free_attribs;
                    }
                }
                else
                {
                    vm_set_last_error("error: unexpected token after '=' at attribute %s\n", attrib->name);
                    goto _free_attribs;
                }
            break;

            default:
                vm_set_last_error("error: unexpected token after '=' at attribute %s\n", attrib->name);
                goto _free_attribs;
        }

        // if(dat_next_token(parser))
        // {
        //     vm_set_last_error("error: unexpected end reached after attribute definition '%s'", attrib->name);
        //     goto _free_attribs;
        // }

        if(parser->lexer.token.token_class != TOKEN_CLASS_PUNCTUATOR ||
           parser->lexer.token.token_type != TOKEN_PUNCTUATOR_SEMICOLON)
        {
            vm_set_last_error("error: expecting token ';' after definition of attribute '%s', got '%s'\n", attrib->name, vm_translate_token(&parser->lexer.token));
            goto _free_attribs;
        }

        parser->valid_null = 1;
        dat_next_token(parser);
    }

    /* we got here, so either token is null, or is a '}' */
    if(parser->lexer.token.token_class == TOKEN_CLASS_UNKNOWN)
    {
        if(has_opening_brace)
        {
            /* there as a '{' at the beginning, but we ain't got a '}' here */

            vm_set_last_error("error: expecting '}' token at the end of struct attribute\n"); 
            goto _free_attribs;
        }    
    }
    else
    {
        if(!has_opening_brace)
        {
            /* there wasn't a '{', but we got one '}' here */
            vm_set_last_error("error: missing '{' at the start of struct attribute\n");
            goto _free_attribs;
        }
        else
        {
            /* all is fine :) */
            dat_next_token(parser);   
        }
    }

    _return:

    return attribs;

    _free_attribs:
    dat_free_attribs(attribs);
    attribs = NULL;
    goto _return;
}