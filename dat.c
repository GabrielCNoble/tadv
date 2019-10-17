#include "dat.h"
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

struct dat_attrib_t *dat_parse_dat_string(const char *src)
{
    struct dat_attrib_t *attribs = NULL;
    struct dat_attrib_t *attrib = NULL;
    struct token_t *tokens;
    struct token_t *token;

    tokens = vm_lex_code(src);
    token = tokens;

    while(token)
    {
        attrib = calloc(1, sizeof(struct dat_attrib_t ));
        attrib->type = DAT_ATTRIB_TYPE_STRUCT;
        attrib->data.attrib = dat_parse(&token);

        if(!attrib->data.attrib)
        {
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

struct dat_attrib_t *dat_parse(struct token_t **tokens)
{
    struct dat_attrib_t *attrib = NULL;
    struct dat_attrib_t *attribs = NULL;
    struct token_t *token;
    struct token_t *before_code;
    struct token_t *prev_token;
    struct token_t *code;
    // char *attrib_name;
    uint32_t has_opening_brace = 0;
    
    token = *tokens;

    /* TODO: this parser is too fragile... */

    if(token->token_class != TOKEN_CLASS_IDENTIFIER)
    {
        if(token->token_class == TOKEN_CLASS_PUNCTUATOR &&
           token->token_type == TOKEN_PUNCTUATOR_OBRACE)
        {
            token = token->next;
            has_opening_brace = 1;
        }
        else
        {
            printf("error: unexpected token at the beginning of a block attribute\n");
            return NULL;
        }
    }

    while(token && (token->token_class != TOKEN_CLASS_PUNCTUATOR || 
          token->token_type != TOKEN_PUNCTUATOR_CBRACE))
    {
        if(token->token_class != TOKEN_CLASS_IDENTIFIER)
        {
            printf("error: expected attribute identifier\n");
            goto _free_attribs;
        }

        attrib = calloc(1, sizeof(struct dat_attrib_t));
        attrib->name = strdup((char *)token->constant.ptr_constant);
        attrib->next = attribs;
        attribs = attrib;

        token = token->next;

        if(token->token_class != TOKEN_CLASS_PUNCTUATOR ||
            token->token_type != TOKEN_PUNCTUATOR_EQUAL)
        {
            printf("error: expecting a '=' after attribute name %s\n", attrib->name);
            goto _free_attribs;
        }
        
        token = token->next;

        switch(token->token_class)
        {
            case TOKEN_CLASS_INTEGER_CONSTANT:
                attrib->type = DAT_ATTRIB_TYPE_INT;
                attrib->data.scalar_data = token->constant.uint_constant;
                token = token->next;
            break;

            case TOKEN_CLASS_FLOAT_CONSTANT:
                attrib->type = DAT_ATTRIB_TYPE_FLOAT;
                attrib->data.scalar_data = token->constant.flt_constant;
                token = token->next;
            break;

            case TOKEN_CLASS_STRING_CONSTANT:
                attrib->type = DAT_ATTRIB_TYPE_STRING;
                attrib->data.str_data = (char *)token->constant.ptr_constant;
                token = token->next;
            break;

            // case TOKEN_CLASS_CODE:
            //     attrib->type = DAT_ATTRIB_TYPE_CODE;
            //     attrib->data.str_data = (char *)token->constant.ptr_constant;
            //     token = token->next;
            // break;

            case TOKEN_CLASS_PUNCTUATOR:
                if(token->token_type == TOKEN_PUNCTUATOR_OBRACE)
                {
                    attrib->type = DAT_ATTRIB_TYPE_STRUCT;
                    attrib->data.attrib = dat_parse(&token);
                    if(!attrib->data.attrib)
                    {
                        goto _free_attribs;
                    }
                }
                else if(token->token_type == TOKEN_PUNCTUATOR_OPARENTHESIS)
                {
                    attrib->type = DAT_ATTRIB_TYPE_CODE;
                    before_code = token;
                    token = token->next;
                    code = token;

                    while(token->token_class != TOKEN_CLASS_PUNCTUATOR ||
                          token->token_type != TOKEN_PUNCTUATOR_CPARENTHESIS)
                    {
                        prev_token = token;
                        token = token->next;
                    }

                    prev_token->next = NULL;
                    before_code->next = token;

                    if(vm_assemble_code(&attrib->data.code, code))
                    {
                        printf("problem assembling code. Well, shit...\n");
                    }

                    token = token->next;
                }
                else
                {
                    printf("error: unexpected token after '=' at attribute %s\n", attrib->name);
                    goto _free_attribs;
                }
            break;

            default:
                printf("error: unexpected token after '=' at attribute %s\n", attrib->name);
                goto _free_attribs;
        }

        if(token->token_class != TOKEN_CLASS_PUNCTUATOR ||
           token->token_type != TOKEN_PUNCTUATOR_SEMICOLON)
        {
            // printf("error: missing ';' after definition of attribute %s\n", attrib->name);
            printf("error: expecting token ';' after definition of attribute %s\n", attrib->name);
            goto _free_attribs;
        }

        token = token->next;
    }

    if((token->token_class != TOKEN_CLASS_PUNCTUATOR || 
        token->token_type != TOKEN_PUNCTUATOR_CBRACE) && has_opening_brace)
    {
        printf("error: expecting '}' token at the end of struct attribute\n"); 
        goto _free_attribs;
    }
    else if(token->token_class == TOKEN_CLASS_PUNCTUATOR && 
            token->token_type == TOKEN_PUNCTUATOR_CBRACE && (!has_opening_brace))
    {
        printf("error: missing '{' at the start of struct attribute\n");
        goto _free_attribs;
    }

    token = token->next;
    *tokens = token;

    _return:

    return attribs;

    _free_attribs:
    dat_free_attribs(attribs);
    attribs = NULL;
    goto _return;
}