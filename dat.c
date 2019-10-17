#include "dat.h"
#include "vm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct dat_attrib_t *dat_get_attrib_recursive(struct dat_attrib_t *attribs, struct token_t *token)
{
    if(token->token_class == TOKEN_CLASS_IDENTIFIER)
    {
        while(attribs)
        {
            if(!strcmp((char *)token->constant.ptr_constant, attribs->name))
            {
                token = token->next;

                if(token && token->token_class == TOKEN_CLASS_PUNCTUATOR && 
                   token->token_type == TOKEN_PUNCTUATOR_DOT)
                {
                    if(attribs->type == DAT_ATTRIB_TYPE_STRUCT)
                    {
                        token = token->next;
                        return dat_get_attrib_recursive(attribs->data.attrib, token);
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
    struct token_t *tokens;
    struct token_t *token;

    tokens = vm_lex_code(name);
    attribs = dat_get_attrib_recursive(attribs, tokens);

    while(tokens)
    {
        token = tokens->next;
        vm_free_token(tokens);
        tokens = token;
    }

    return attribs;
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


 


struct dat_parser_t dat_init_parser(struct token_t *tokens)
{
    struct dat_parser_t parser;

    parser.tokens = tokens;
    parser.valid_null = 1;

    return parser;
}

uint32_t dat_next_token(struct dat_parser_t *parser)
{
    parser->tokens = parser->tokens->next;
    return !(parser->tokens || parser->valid_null);
}

struct dat_attrib_t *dat_parse_dat_string(const char *src)
{
    struct dat_attrib_t *attribs = NULL;
    struct dat_attrib_t *attrib = NULL;
    struct token_t *tokens;
    struct token_t *token;

    struct dat_parser_t parser;

    // tokens = vm_lex_code(src);
    // token = tokens;

    parser = dat_init_parser(vm_lex_code(src));


    while(parser.tokens)
    {
        attrib = calloc(1, sizeof(struct dat_attrib_t ));
        attrib->type = DAT_ATTRIB_TYPE_STRUCT;
        attrib->data.attrib = dat_parse(&parser);
        
        // printf("stuck\n");

        if(!attrib->data.attrib)
        {
            printf("%s\n", vm_get_error());
            return NULL;
        }

        attrib->next = attribs;
        attribs = attrib;
    }

    while(tokens)
    {
        token = tokens->next;
        vm_free_token(tokens);
        tokens = token;
    }

    return attribs;
}

#define UNEXPECTED_END_REACHED "unexpected end of tokens reached"
struct dat_attrib_t *dat_parse(struct dat_parser_t *parser)
{
    struct dat_attrib_t *attrib = NULL;
    struct dat_attrib_t *attribs = NULL;
    // struct token_t *token;
    struct token_t *before_code;
    struct token_t *prev_token;
    struct token_t *code;
    // char *attrib_name;
    uint32_t has_opening_brace = 0;
    

    if(!parser->tokens)
    {
        return NULL;
    }

    if(parser->tokens->token_class != TOKEN_CLASS_IDENTIFIER)
    {
        if(parser->tokens->token_class == TOKEN_CLASS_PUNCTUATOR &&
           parser->tokens->token_type == TOKEN_PUNCTUATOR_OBRACE)
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
            vm_set_last_error("error: expecting '{' or identifier, got '%s'", vm_translate_token(parser->tokens)); 
            return NULL;
        }
    }

    while(parser->tokens && (parser->tokens->token_class != TOKEN_CLASS_PUNCTUATOR || 
          parser->tokens->token_type != TOKEN_PUNCTUATOR_CBRACE))
    {
        parser->valid_null = 0;

        if(parser->tokens->token_class != TOKEN_CLASS_IDENTIFIER)
        {
            // printf("error: expected attribute identifier\n");
            vm_set_last_error("error: expecting attribute identifier, got '%s'", vm_translate_token(parser->tokens));
            goto _free_attribs;
        }

        attrib = calloc(1, sizeof(struct dat_attrib_t));
        attrib->name = strdup((char *)parser->tokens->constant.ptr_constant);
        attrib->next = attribs;
        attribs = attrib;

        if(dat_next_token(parser))
        {
            vm_set_last_error("error: unexpected end reached after attribute '%s'", attrib->name);
            goto _free_attribs;
        }

        if(parser->tokens->token_class != TOKEN_CLASS_PUNCTUATOR ||
            parser->tokens->token_type != TOKEN_PUNCTUATOR_EQUAL)
        {
            vm_set_last_error("error: expecting a '=' after attribute '%s', got '%s'", attrib->name, vm_translate_token(parser->tokens));
            goto _free_attribs;
        }

        if(dat_next_token(parser))
        {
            vm_set_last_error("error: unexpected end reached after '=' for attribute '%s'", attrib->name);
            goto _free_attribs;
        }


        switch(parser->tokens->token_class)
        {
            case TOKEN_CLASS_INTEGER_CONSTANT:
                attrib->type = DAT_ATTRIB_TYPE_INT;
                attrib->data.int_data = parser->tokens->constant.uint_constant;

                if(dat_next_token(parser))
                {
                    vm_set_last_error("error: unexpected end reached after attribute definition '%s'", attrib->name);
                    goto _free_attribs;
                }
            break;

            case TOKEN_CLASS_FLOAT_CONSTANT:
                attrib->type = DAT_ATTRIB_TYPE_FLOAT;
                attrib->data.flt_data = parser->tokens->constant.flt_constant;

                if(dat_next_token(parser))
                {
                    vm_set_last_error("error: unexpected end reached after attribute definition '%s'", attrib->name);
                    goto _free_attribs;
                }
            break;

            case TOKEN_CLASS_STRING_CONSTANT:
                attrib->type = DAT_ATTRIB_TYPE_STRING;
                attrib->data.str_data = (char *)parser->tokens->constant.ptr_constant;

                if(dat_next_token(parser))
                {
                    vm_set_last_error("error: unexpected end reached after attribute definition '%s'", attrib->name);
                    goto _free_attribs;
                }
            break;

            case TOKEN_CLASS_PUNCTUATOR:
                if(parser->tokens->token_type == TOKEN_PUNCTUATOR_OBRACE)
                {
                    attrib->type = DAT_ATTRIB_TYPE_STRUCT;
                    attrib->data.attrib = dat_parse(parser);
                    if(!attrib->data.attrib)
                    {
                        goto _free_attribs;
                    }
                    parser->valid_null = 0;
                }
                else if(parser->tokens->token_type == TOKEN_PUNCTUATOR_OPARENTHESIS)
                {
                    attrib->type = DAT_ATTRIB_TYPE_CODE;
                    before_code = parser->tokens;

                    if(dat_next_token(parser))
                    {
                        vm_set_last_error("error: unexpected end reached before code defininion for attribute '%s'", attrib->name);
                        goto _free_attribs;
                    }

                    code = parser->tokens;

                    while(parser->tokens->token_class != TOKEN_CLASS_PUNCTUATOR ||
                          parser->tokens->token_type != TOKEN_PUNCTUATOR_CPARENTHESIS)
                    {
                        prev_token = parser->tokens;

                        if(dat_next_token(parser))
                        {
                            vm_set_last_error("error: unexpected end reached while reading code for attribute '%s'", attrib->name);
                            goto _free_attribs;
                        }
                    }

                    prev_token->next = NULL;
                    before_code->next = parser->tokens;

                    if(vm_assemble_code(&attrib->data.code, code))
                    {
                        vm_set_last_error("error: error while compiling code for attribute '%s'", attrib->name);
                        goto _free_attribs;
                    }

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

        if(parser->tokens->token_class != TOKEN_CLASS_PUNCTUATOR ||
           parser->tokens->token_type != TOKEN_PUNCTUATOR_SEMICOLON)
        {
            vm_set_last_error("error: expecting token ';' after definition of attribute '%s', got '%s'\n", attrib->name, vm_translate_token(parser->tokens));
            goto _free_attribs;
        }

        parser->valid_null = 1;
        dat_next_token(parser);
    }

    /* we got here, so either token is null, or is a '}' */
    if(!parser->tokens)
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