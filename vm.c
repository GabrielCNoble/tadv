#include "vm.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#include "interactable.h"
#include "scene.h"

enum CHAR_TYPE
{
    CHAR_TYPE_BLANK = 1,
    CHAR_TYPE_LETTER = 1 << 1,
    CHAR_TYPE_NUMBER = 1 << 2,
    CHAR_TYPE_PUNCTUATOR = 1 << 3,
    CHAR_TYPE_END_LINE = 1 << 4,
    CHAR_TYPE_ILLEGAL = 1 << 5,
};


enum OPERAND_TYPES
{
    OPERAND_TYPE_INT_CONSTANT = 1,
    OPERAND_TYPE_FLOAT_CONSTANT =  1 << 1,
    OPERAND_TYPE_STRING_CONSTANT = 1 << 2,

    /*
        opcode label;

        This is mostly useful for jumps, where
        one writes:

        jmp label;
    */
    OPERAND_TYPE_LABEL = 1 << 3,

    /*
        opcode offset label;

        This is mostly useful to store the address of
        a label. For example:

        mov r0, offset label;

        Here, the offset of label 'label' has
        been stored in r0.
    */
    OPERAND_TYPE_LABEL_OFFSET = 1 << 4,
    OPERAND_TYPE_GP_REGISTER = 1 << 5,

    OPERAND_TYPE_I_REGISTER = 1 << 6,

    /*
        opcode [register]

        Here register contains an pointer, and
        it has to be dereferenced. This is useful
        for a handful of things.

        mov [r1], r0;

        Assuming r1 contains a valid memory address,
        this mov will put the contents of r0 into
        whatever r1 points to.
    */
    OPERAND_TYPE_MEMORY_REGISTER = 1 << 7,

    /*
        opcode [integer constant]

        This has the same use case as a register
        containing an pointer. The difference is
        that the value used as a pointer will be
        determined during code assembly. A constant
        address can be, for example, a global variable
    */
    OPERAND_TYPE_MEMORY_INT_CONSTANT = 1 << 8,
};




struct register_info_t
{
    char *name;
    void *address;
};

#define OPCODE(n, o, oc, a0, a1, a2)                            \
    /* designated initializers rock :) */                       \
    (struct opcode_info_t){.name = n,                           \
                           .offset = o,                         \
                           .operand_count = oc,                 \
                           .allowed_operand_types[0] = a0,      \
                           .allowed_operand_types[1] = a1,      \
                           .allowed_operand_types[2] = a2}


struct opcode_info_t opcode_info[VM_OPCODE_MAX] = {};
uint32_t custom_opcode_count = 0;
// struct custom_opcode_info_t custom_opcode_info[VM_OPCODE_MAX - VM_OPCODE_LAST] = {};
char char_map[512] = {CHAR_TYPE_BLANK};

struct scene_t *reg_scn = NULL;
void *reg_pc;
uint8_t reg_status;
char *gp_reg_names[] = {"r0", "r1", "r2", "r3"};
char *i_reg_names[] = {"ri0", "ri1"};
char *stack;

#define GP_REGS_COUNT (sizeof(gp_reg_names) / sizeof(gp_reg_names[0]))
#define I_REGS_COUNT (sizeof(i_reg_names) / sizeof(i_reg_names[0]))
#define STACK_SIZE 2048

uint64_t gp_regs[GP_REGS_COUNT];
struct interactible_t *i_regs[I_REGS_COUNT];




struct code_buffer_t current_code_buffer;

/* tokens freed through vm_free_token, so they can be reused whenever something
has to be lexed during runtime (useful for instructions that have strings as
operands) */
struct token_t *free_tokens = NULL;

struct vm_assembler_t
{
    uint32_t parsing_instruction;
    struct vm_lexer_t lexer;
};

void vm_init()
{
    opcode_info[VM_OPCODE_PRINT] = OPCODE("print", sizeof(struct opcode_1op_t), 1, OPERAND_TYPE_STRING_CONSTANT | OPERAND_TYPE_GP_REGISTER, 0, 0);
    opcode_info[VM_OPCODE_LDSC] = OPCODE("ldsc", sizeof(struct opcode_1op_t), 1, OPERAND_TYPE_STRING_CONSTANT, 0, 0);
    opcode_info[VM_OPCODE_LDSCA] = OPCODE("ldsca", sizeof(struct opcode_2op_t), 2, OPERAND_TYPE_GP_REGISTER, OPERAND_TYPE_STRING_CONSTANT, 0);
    opcode_info[VM_OPCODE_CHGSC] = OPCODE("chgsc", sizeof(struct opcode_1op_t), 1, OPERAND_TYPE_STRING_CONSTANT, 0, 0);
    opcode_info[VM_OPCODE_LDI] = OPCODE("ldi", sizeof(struct opcode_2op_t), 2, OPERAND_TYPE_I_REGISTER, OPERAND_TYPE_STRING_CONSTANT, 0);
    opcode_info[VM_OPCODE_LDIA] = OPCODE("ldia", sizeof(struct opcode_3op_t), 3, OPERAND_TYPE_GP_REGISTER, OPERAND_TYPE_I_REGISTER, OPERAND_TYPE_STRING_CONSTANT);

    opcode_info[VM_OPCODE_MOV] = OPCODE("mov", sizeof(struct opcode_2op_t), 2, OPERAND_TYPE_GP_REGISTER |
                                                                               OPERAND_TYPE_I_REGISTER |
                                                                               OPERAND_TYPE_MEMORY_REGISTER,

                                                                               OPERAND_TYPE_GP_REGISTER |
                                                                               OPERAND_TYPE_I_REGISTER |
                                                                               OPERAND_TYPE_MEMORY_REGISTER |
                                                                               OPERAND_TYPE_INT_CONSTANT |
                                                                               OPERAND_TYPE_LABEL_OFFSET, 0);


    opcode_info[VM_OPCODE_INC] = OPCODE("inc", sizeof(struct opcode_1op_t), 1, OPERAND_TYPE_GP_REGISTER |
                                                                               OPERAND_TYPE_MEMORY_REGISTER, 0, 0);


    opcode_info[VM_OPCODE_DEC] = OPCODE("dec", sizeof(struct opcode_1op_t), 1, OPERAND_TYPE_GP_REGISTER |
                                                                               OPERAND_TYPE_MEMORY_REGISTER, 0, 0);


    opcode_info[VM_OPCODE_AND] = OPCODE("and", sizeof(struct opcode_2op_t), 2, OPERAND_TYPE_GP_REGISTER |
                                                                               OPERAND_TYPE_MEMORY_REGISTER,

                                                                               OPERAND_TYPE_GP_REGISTER |
                                                                               OPERAND_TYPE_MEMORY_REGISTER |
                                                                               OPERAND_TYPE_INT_CONSTANT, 0);


    opcode_info[VM_OPCODE_OR] = OPCODE("or", sizeof(struct opcode_2op_t), 2, OPERAND_TYPE_GP_REGISTER |
                                                                             OPERAND_TYPE_MEMORY_REGISTER,

                                                                             OPERAND_TYPE_GP_REGISTER |
                                                                             OPERAND_TYPE_MEMORY_REGISTER |
                                                                             OPERAND_TYPE_INT_CONSTANT, 0);


    opcode_info[VM_OPCODE_XOR] = OPCODE("xor", sizeof(struct opcode_2op_t), 2, OPERAND_TYPE_GP_REGISTER |
                                                                               OPERAND_TYPE_MEMORY_REGISTER,

                                                                               OPERAND_TYPE_GP_REGISTER |
                                                                               OPERAND_TYPE_MEMORY_REGISTER |
                                                                               OPERAND_TYPE_INT_CONSTANT, 0);


    opcode_info[VM_OPCODE_CMP] = OPCODE("cmp", sizeof(struct opcode_2op_t), 2, OPERAND_TYPE_GP_REGISTER |
                                                                               OPERAND_TYPE_I_REGISTER |
                                                                               OPERAND_TYPE_MEMORY_REGISTER |
                                                                               OPERAND_TYPE_INT_CONSTANT,

                                                                               OPERAND_TYPE_GP_REGISTER |
                                                                               OPERAND_TYPE_I_REGISTER |
                                                                               OPERAND_TYPE_MEMORY_REGISTER |
                                                                               OPERAND_TYPE_INT_CONSTANT, 0);

    opcode_info[VM_OPCODE_CMPS] = OPCODE("cmps", sizeof(struct opcode_2op_t), 2, OPERAND_TYPE_GP_REGISTER |
                                                                                 OPERAND_TYPE_STRING_CONSTANT,

                                                                                 OPERAND_TYPE_GP_REGISTER |
                                                                                 OPERAND_TYPE_STRING_CONSTANT, 0);

    opcode_info[VM_OPCODE_CMPSLC] = OPCODE("cmpslc", sizeof(struct opcode_2op_t), 2, OPERAND_TYPE_GP_REGISTER |
                                                                                 OPERAND_TYPE_STRING_CONSTANT,

                                                                                 OPERAND_TYPE_GP_REGISTER |
                                                                                 OPERAND_TYPE_STRING_CONSTANT, 0);

    opcode_info[VM_OPCODE_CMPSSTR] = OPCODE("cmpsstr", sizeof(struct opcode_2op_t), 2, OPERAND_TYPE_GP_REGISTER |
                                                                                OPERAND_TYPE_STRING_CONSTANT,

                                                                                OPERAND_TYPE_GP_REGISTER |
                                                                                OPERAND_TYPE_STRING_CONSTANT, 0);

    opcode_info[VM_OPCODE_LCSTR] = OPCODE("lcstr", sizeof(struct opcode_1op_t), 1, OPERAND_TYPE_GP_REGISTER,0 , 0);


    opcode_info[VM_OPCODE_JMP] = OPCODE("jmp", sizeof(struct opcode_1op_t), 1, OPERAND_TYPE_GP_REGISTER |
                                                                               OPERAND_TYPE_INT_CONSTANT |
                                                                               OPERAND_TYPE_LABEL, 0, 0);


    opcode_info[VM_OPCODE_JE] = OPCODE("je", sizeof(struct opcode_1op_t), 1, OPERAND_TYPE_GP_REGISTER |
                                                                             OPERAND_TYPE_LABEL, 0, 0);


    opcode_info[VM_OPCODE_JNE] = OPCODE("jne", sizeof(struct opcode_1op_t), 1, OPERAND_TYPE_GP_REGISTER |
                                                                               OPERAND_TYPE_LABEL, 0, 0);


    opcode_info[VM_OPCODE_JG] = OPCODE("jg", sizeof(struct opcode_1op_t), 1, OPERAND_TYPE_GP_REGISTER |
                                                                             OPERAND_TYPE_LABEL, 0, 0);


    opcode_info[VM_OPCODE_JL] = OPCODE("jl", sizeof(struct opcode_1op_t), 1, OPERAND_TYPE_GP_REGISTER |
                                                                             OPERAND_TYPE_LABEL, 0, 0);


    opcode_info[VM_OPCODE_JGE] = OPCODE("jge", sizeof(struct opcode_1op_t), 1, OPERAND_TYPE_GP_REGISTER |
                                                                               OPERAND_TYPE_LABEL, 0, 0);


    opcode_info[VM_OPCODE_JLE] = OPCODE("jle", sizeof(struct opcode_1op_t), 1, OPERAND_TYPE_GP_REGISTER |
                                                                               OPERAND_TYPE_LABEL, 0, 0);

    opcode_info[VM_OPCODE_IN] = OPCODE("in", sizeof(struct opcode_1op_t), 1, OPERAND_TYPE_GP_REGISTER, 0, 0);
    opcode_info[VM_OPCODE_RET] = OPCODE("ret", sizeof(struct opcode_1op_t), 1, OPERAND_TYPE_GP_REGISTER | OPERAND_TYPE_INT_CONSTANT, 0, 0);
    opcode_info[VM_OPCODE_EXIT] = OPCODE("exit", sizeof(struct opcode_t), 0, 0, 0, 0);
    // opcode_info[VM_OPCODE_CALL] = OPCODE("call", sizeof(struct opcode_1op_t), 1, OPERAND_TYPE_LABEL | OPERAND_TYPE_GP_REGISTER, 0, 0);

    opcode_info[VM_OPCODE_FCRSH] = OPCODE("fcrsh", sizeof(struct opcode_t), 0, 0, 0, 0);



    char_map['_'] = CHAR_TYPE_LETTER;

    for(uint32_t i = 'a'; i <= 'z'; i++)
    {
        char_map[i] = CHAR_TYPE_LETTER;
    }

    for(uint32_t i = 'A'; i <= 'Z'; i++)
    {
        char_map[i] = CHAR_TYPE_LETTER;
    }

    for(uint32_t i = '0'; i <= '9'; i++)
    {
        char_map[i] = CHAR_TYPE_NUMBER;
    }

    char_map[';'] = CHAR_TYPE_PUNCTUATOR;
    char_map[','] = CHAR_TYPE_PUNCTUATOR;
    char_map[':'] = CHAR_TYPE_PUNCTUATOR;
    char_map['('] = CHAR_TYPE_PUNCTUATOR;
    char_map[')'] = CHAR_TYPE_PUNCTUATOR;
    char_map['['] = CHAR_TYPE_PUNCTUATOR;
    char_map[']'] = CHAR_TYPE_PUNCTUATOR;
    char_map['{'] = CHAR_TYPE_PUNCTUATOR;
    char_map['}'] = CHAR_TYPE_PUNCTUATOR;
    char_map['"'] = CHAR_TYPE_PUNCTUATOR;
    char_map['\''] = CHAR_TYPE_PUNCTUATOR;
    char_map['-'] = CHAR_TYPE_PUNCTUATOR;
    char_map['+'] = CHAR_TYPE_PUNCTUATOR;
    char_map['/'] = CHAR_TYPE_PUNCTUATOR;
    char_map['\\'] = CHAR_TYPE_PUNCTUATOR;
    char_map['*'] = CHAR_TYPE_PUNCTUATOR;
    char_map['='] = CHAR_TYPE_PUNCTUATOR;
    char_map['.'] = CHAR_TYPE_PUNCTUATOR;

    char_map['\n'] = CHAR_TYPE_BLANK;
    char_map['\r'] = CHAR_TYPE_BLANK;
    char_map['\t'] = CHAR_TYPE_BLANK;
    char_map['\0'] = CHAR_TYPE_BLANK;
    char_map[' '] = CHAR_TYPE_BLANK;

    // stack = calloc(1, STACK_SIZE);
}

// struct token_t *vm_lex_code(const char *src)
// {
//     uint32_t index = 0;
//     uint32_t token_str_index;
//     static char token_str[512];
//     struct token_t *tokens = NULL;
//     struct token_t *next_token = NULL;
//     struct token_t *last_token = NULL;
//     uint32_t token_class;
//     uint32_t token_type;
//     uint64_t constant;
//     // char end_char;
//     char escaped;
//     float float_constant;

//     while(src[index])
//     {
//         while(char_map[(uint32_t)src[index]] == CHAR_TYPE_BLANK && src[index] != '\0') index++;

//         if(src[index] != '\0')
//         {
//             token_class = TOKEN_CLASS_UNKNOWN;

//             if(char_map[(uint32_t)src[index]] == CHAR_TYPE_PUNCTUATOR)
//             {
//                 if(src[index] == '"')
//                 {
//                     token_class = TOKEN_CLASS_STRING_CONSTANT;
//                     token_type = 0;

//                     index++;
//                     token_str_index = 0;

//                     while(src[index] != '"')
//                     {
//                         if(src[index] == '\0')
//                         {
//                             vm_set_last_error("missing '\"' at the end of string constant");
//                             return NULL;
//                         }

//                         if(src[index] == '\\')
//                         {
//                             index++;

//                             escaped = src[index++];

//                             switch(escaped)
//                             {
//                                 case 'n':
//                                     escaped = '\n';
//                                 break;

//                                 case 't':
//                                     escaped = '\t';
//                                 break;

//                                 case '0':
//                                     escaped = '\0';
//                                 break;
//                             }
//                         }
//                         else
//                         {
//                             escaped = src[index++];
//                         }

//                         token_str[token_str_index++] = escaped;
//                     }

//                     index++;
//                     token_str[token_str_index] = '\0';
//                 }
//                 else
//                 {
//                     token_class = TOKEN_CLASS_PUNCTUATOR;

//                     switch(src[index])
//                     {
//                         case ',':
//                             index++;
//                             token_type = TOKEN_PUNCTUATOR_COMMA;
//                         break;

//                         case '.':
//                             index++;
//                             token_type = TOKEN_PUNCTUATOR_DOT;
//                         break;

//                         case ':':
//                             index++;
//                             token_type = TOKEN_PUNCTUATOR_COLON;
//                         break;

//                         case ';':
//                             index++;
//                             token_type = TOKEN_PUNCTUATOR_SEMICOLON;
//                         break;

//                         case '(':
//                             index++;
//                             token_type = TOKEN_PUNCTUATOR_OPARENTHESIS;
//                         break;

//                         case ')':
//                             index++;
//                             token_type = TOKEN_PUNCTUATOR_CPARENTHESIS;
//                         break;

//                         case '[':
//                             index++;
//                             token_type = TOKEN_PUNCTUATOR_OBRACKET;
//                         break;

//                         case ']':
//                             index++;
//                             token_type = TOKEN_PUNCTUATOR_CBRACKET;
//                         break;

//                         case '=':
//                             index++;
//                             token_type = TOKEN_PUNCTUATOR_EQUAL;
//                         break;

//                         case '{':
//                             index++;
//                             token_type = TOKEN_PUNCTUATOR_OBRACE;
//                         break;

//                         case '}':
//                             index++;
//                             token_type = TOKEN_PUNCTUATOR_CBRACE;
//                         break;

//                         default:
//                             token_class = TOKEN_CLASS_UNKNOWN;
//                         break;
//                     }
//                 }
//             }
//             else
//             {
//                 if(char_map[(uint32_t)src[index]] == CHAR_TYPE_LETTER)
//                 {
//                     token_class = TOKEN_CLASS_IDENTIFIER;
//                     token_type = 0;
//                     token_str_index = 0;

//                     while(!(char_map[(uint32_t)src[index]] & (CHAR_TYPE_BLANK | CHAR_TYPE_PUNCTUATOR)))
//                     {
//                         token_str[token_str_index++] = src[index++];
//                     }

//                     token_str[token_str_index] = '\0';

//                     for(uint32_t i = 0; i < VM_OPCODE_LAST + custom_opcode_count; i++)
//                     {
//                         if(!strcmp(token_str, opcode_info[i].name))
//                         {
//                             token_class = TOKEN_CLASS_INSTRUCTION;
//                             constant = i;
//                             break;
//                         }
//                     }
//                 }
//                 else
//                 {
//                     token_class = TOKEN_CLASS_INTEGER_CONSTANT;
//                     token_type = 0;
//                     token_str_index = 0;

//                     while(char_map[(uint32_t)src[index]] == CHAR_TYPE_NUMBER)
//                     {
//                         token_str[token_str_index++] = src[index++];
//                     }

//                     if(src[index] == '.')
//                     {
//                         token_class = TOKEN_CLASS_FLOAT_CONSTANT;

//                         token_str[token_str_index++] = src[index++];

//                         if(char_map[(uint32_t)src[index]] != CHAR_TYPE_NUMBER)
//                         {
//                             vm_set_last_error("unexpected token %c", src[index]);
//                             return NULL;
//                         }

//                         while(char_map[(uint32_t)src[index]] == CHAR_TYPE_NUMBER)
//                         {
//                             token_str[token_str_index++] = src[index++];
//                         }

//                         /* TODO: allow for scientific notation? */
//                         if(char_map[(uint32_t)src[index]] != CHAR_TYPE_BLANK)
//                         {
//                             vm_set_last_error("unexpected token %c", src[index]);
//                             return NULL;
//                         }

//                         token_str[token_str_index] = '\0';
//                         float_constant = atof(token_str);

//                         constant = *(uint32_t *)&float_constant;
//                     }
//                     else
//                     {
//                         token_str[token_str_index] = '\0';
//                         constant = atoi(token_str);
//                     }
//                 }
//             }

//             if(token_class == TOKEN_CLASS_UNKNOWN)
//             {
//                 vm_set_last_error("unknown token %c", src[index]);
//                 return NULL;
//             }

//             next_token = vm_alloc_token();
//             next_token->next = NULL;

//             if(token_class == TOKEN_CLASS_STRING_CONSTANT ||
//                token_class == TOKEN_CLASS_IDENTIFIER ||
//                token_class == TOKEN_CLASS_CODE)
//             {
//                 if(next_token->token_class == TOKEN_CLASS_STRING_CONSTANT &&
//                    strlen(next_token->constant.ptr_constant) >= strlen(token_str))
//                 {
//                     /* since tokens are recycled, they keep the class they had last
//                     time they were used. If here the allocd token is a
//                     string constant token, and the space of its previous
//                     string constant is big enough, just reuse the memory */
//                     strcpy(next_token->constant.ptr_constant, token_str);
//                 }
//                 else
//                 {
//                     /* the token is either of different type or doesn't
//                     have enough space. */
//                     if(next_token->constant.ptr_constant)
//                     {
//                         /* in case of not enough space, get rid of the
//                         old buffer... */
//                         free(next_token->constant.ptr_constant);
//                     }

//                     next_token->constant.ptr_constant = strdup(token_str);
//                 }
//             }
//             else
//             {
//                 next_token->constant.uint_constant = constant;
//             }

//             next_token->token_type = token_type;
//             next_token->token_class = token_class;

//             if(!tokens)
//             {
//                 tokens = next_token;
//             }
//             else
//             {
//                 last_token->next = next_token;
//             }

//             last_token = next_token;
//         }
//     }

//     return tokens;
// }

uint32_t vm_lex_one_token(struct vm_lexer_t *lexer)
{
    // uint32_t index = *offset;
    uint32_t token_str_index;
    // static char token_str[512];
    // struct token_t *tokens = NULL;
    // struct token_t *next_token = NULL;
    // struct token_t *last_token = NULL;
    // uint32_t token_class;
    // uint32_t token_type;
    // uint64_t constant;
    // char end_char;
    char escaped;
    // float float_constant;

    // struct token_t token;

    lexer->token.token_class = TOKEN_CLASS_UNKNOWN;

    // while(src[index])
    // {
    lexer->prev_offset = lexer->offset;

    if(lexer->offset >= lexer->max_offset)
    {
        return 0;
    }

    if(!lexer->lex_blank)
    {
        while(char_map[(uint32_t)lexer->src[lexer->offset]] == CHAR_TYPE_BLANK && lexer->src[lexer->offset] != '\0') lexer->offset++;
    }

    if(lexer->src[lexer->offset] != '\0')
    {
        if(char_map[(uint32_t)lexer->src[lexer->offset]] == CHAR_TYPE_PUNCTUATOR)
        {
            if(lexer->src[lexer->offset] == '"')
            {
                lexer->token.token_class = TOKEN_CLASS_STRING_CONSTANT;
                lexer->offset++;
                token_str_index = 0;

                while(lexer->src[lexer->offset] != '"')
                {
                    if(lexer->src[lexer->offset] == '\0')
                    {
                        vm_set_last_error("missing '\"' at the end of string constant");
                        return 1;
                    }

                    if(lexer->src[lexer->offset] == '\\')
                    {
                        lexer->offset++;

                        escaped = lexer->src[lexer->offset++];

                        switch(escaped)
                        {
                            case 'n':
                                escaped = '\n';
                            break;

                            case 't':
                                escaped = '\t';
                            break;

                            case '0':
                                escaped = '\0';
                            break;
                        }
                    }
                    else
                    {
                        escaped = lexer->src[lexer->offset++];
                    }

                    lexer->token_str[token_str_index++] = escaped;
                }

                lexer->offset++;
                lexer->token_str[token_str_index] = '\0';
                lexer->token.constant.ptr_constant = lexer->token_str;

                // printf("%s\n", lexer->token_str);
            }
            else
            {
                lexer->token.token_class = TOKEN_CLASS_PUNCTUATOR;

                switch(lexer->src[lexer->offset])
                {
                    case ',':
                        lexer->offset++;
                        lexer->token.token_type = TOKEN_PUNCTUATOR_COMMA;
                    break;

                    case '.':
                        lexer->offset++;
                        lexer->token.token_type = TOKEN_PUNCTUATOR_DOT;
                    break;

                    case ':':
                        lexer->offset++;
                        lexer->token.token_type = TOKEN_PUNCTUATOR_COLON;
                    break;

                    case ';':
                        lexer->offset++;
                        lexer->token.token_type = TOKEN_PUNCTUATOR_SEMICOLON;
                    break;

                    case '(':
                        lexer->offset++;
                        lexer->token.token_type = TOKEN_PUNCTUATOR_OPARENTHESIS;
                    break;

                    case ')':
                        lexer->offset++;
                        lexer->token.token_type = TOKEN_PUNCTUATOR_CPARENTHESIS;
                    break;

                    case '[':
                        lexer->offset++;
                        lexer->token.token_type = TOKEN_PUNCTUATOR_OBRACKET;
                    break;

                    case ']':
                        lexer->offset++;
                        lexer->token.token_type = TOKEN_PUNCTUATOR_CBRACKET;
                    break;

                    case '=':
                        lexer->offset++;
                        lexer->token.token_type = TOKEN_PUNCTUATOR_EQUAL;
                    break;

                    case '{':
                        lexer->offset++;
                        lexer->token.token_type = TOKEN_PUNCTUATOR_OBRACE;
                    break;

                    case '}':
                        lexer->offset++;
                        lexer->token.token_type = TOKEN_PUNCTUATOR_CBRACE;
                    break;

					case '*':
						lexer->offset++;
						lexer->token.token_type = TOKEN_PUNCTUATOR_ASTERISK;
					break;

					case '/':
						lexer->offset++;
						lexer->token.token_type = TOKEN_PUNCTUATOR_SLASH;
					break;

                    case '\\':
                        lexer->offset++;
                        lexer->token.token_type = TOKEN_PUNCTUATOR_INV_SLASH;
                    break;

					case '+':
						lexer->offset++;
						lexer->token.token_type = TOKEN_PUNCTUATOR_PLUS;
					break;

					case '-':
						lexer->offset++;
						lexer->token.token_type = TOKEN_PUNCTUATOR_MINUS;
					break;

                    default:
                        lexer->token.token_type = TOKEN_CLASS_UNKNOWN;
                    break;
                }
            }
        }
        else if(char_map[(uint32_t)lexer->src[lexer->offset]] == CHAR_TYPE_BLANK)
        {
            lexer->token.token_class = TOKEN_CLASS_BLANK;

            /* ignore all carriage returns... */
            while(lexer->src[lexer->offset] == '\r') lexer->offset++;

            switch(lexer->src[lexer->offset])
            {
                case '\n':
                    lexer->token.token_type = TOKEN_BLANK_NEW_LINE;
                break;

                case '\t':
                    lexer->token.token_type = TOKEN_BLANK_TAB;
                break;

                case ' ':
                    lexer->token.token_type = TOKEN_BLANK_SPACE;
                break;
            }

            lexer->offset++;
        }
        else
        {
            if(char_map[(uint32_t)lexer->src[lexer->offset]] == CHAR_TYPE_LETTER)
            {
                lexer->token.token_class = TOKEN_CLASS_IDENTIFIER;
                lexer->token.token_type = 0;
                token_str_index = 0;

                while(!(char_map[(uint32_t)lexer->src[lexer->offset]] & (CHAR_TYPE_BLANK | CHAR_TYPE_PUNCTUATOR)))
                {
                    lexer->token_str[token_str_index++] = lexer->src[lexer->offset++];
                }

                lexer->token_str[token_str_index] = '\0';
                lexer->token.constant.ptr_constant = lexer->token_str;

                for(uint32_t i = 0; i < VM_OPCODE_LAST + custom_opcode_count; i++)
                {
                    if(!strcmp(lexer->token_str, opcode_info[i].name))
                    {
                        lexer->token.token_class = TOKEN_CLASS_INSTRUCTION;
                        lexer->token.constant.uint_constant = i;
                        break;
                    }
                }
            }
            else
            {
                lexer->token.token_class = TOKEN_CLASS_INTEGER_CONSTANT;
                token_str_index = 0;

                while(char_map[(uint32_t)lexer->src[lexer->offset]] == CHAR_TYPE_NUMBER)
                {
                    lexer->token_str[token_str_index++] = lexer->src[lexer->offset++];
                }

                if(lexer->src[lexer->offset] == '.')
                {
                    lexer->token.token_class = TOKEN_CLASS_FLOAT_CONSTANT;

                    lexer->token_str[token_str_index++] = lexer->src[lexer->offset++];

                    if(char_map[(uint32_t)lexer->src[lexer->offset]] != CHAR_TYPE_NUMBER)
                    {
                        vm_set_last_error("unexpected token %c", lexer->src[lexer->offset]);
                        return 1;
                    }

                    while(char_map[(uint32_t)lexer->src[lexer->offset]] == CHAR_TYPE_NUMBER)
                    {
                        lexer->token_str[token_str_index++] = lexer->src[lexer->offset++];
                    }

                    /* TODO: allow for scientific notation? */
                    if(char_map[(uint32_t)lexer->src[lexer->offset]] != CHAR_TYPE_BLANK)
                    {
                        vm_set_last_error("unexpected token %c", lexer->src[lexer->offset]);
                        return 1;
                    }

                    lexer->token_str[token_str_index] = '\0';
                    lexer->token.constant.flt_constant = atof(lexer->token_str);
                }
                else
                {
                    lexer->token_str[token_str_index] = '\0';
                    lexer->token.constant.uint_constant = atoi(lexer->token_str);
                }
            }
        }

        if(lexer->token.token_class == TOKEN_CLASS_UNKNOWN)
        {
            vm_set_last_error("unknown token %c", lexer->src[lexer->offset]);
            return 1;
        }
    }

    return 0;
}

char *vm_translate_token_verbose(struct token_t *token)
{
    char *token_class;
    // char *token_str;
    // uint32_t str_len = 0;
    static char fmt[4096];
    // char fmt[512];

    switch(token->token_class)
    {
        case TOKEN_CLASS_STRING_CONSTANT:
            token_class = "String constant";
        break;

        case TOKEN_CLASS_CODE:
            token_class = "Code";
        break;

        case TOKEN_CLASS_INTEGER_CONSTANT:
            token_class = "Integer constant";
        break;

        case TOKEN_CLASS_FLOAT_CONSTANT:
            token_class = "Float constant";
        break;

        case TOKEN_CLASS_IDENTIFIER:
            token_class = "Identifier";
        break;

        case TOKEN_CLASS_INSTRUCTION:
            token_class = "Instruction";
        break;

        case TOKEN_CLASS_PUNCTUATOR:
            token_class = "Punctuator";
        break;
    }

    sprintf(fmt, "%s: %s", token_class, vm_translate_token(token));

    return fmt;
}

char *vm_translate_token(struct token_t *token)
{
    static char fmt[512];

    switch(token->token_class)
    {
        case TOKEN_CLASS_STRING_CONSTANT:
            return (char *)token->constant.ptr_constant;
        break;

        case TOKEN_CLASS_CODE:
            return (char *)token->constant.ptr_constant;
        break;

        case TOKEN_CLASS_INTEGER_CONSTANT:
            sprintf(fmt, PRIu64, token->constant.uint_constant);
            return fmt;
        break;

        case TOKEN_CLASS_FLOAT_CONSTANT:
            sprintf(fmt, "%f", token->constant.flt_constant);
            return fmt;
        break;

        case TOKEN_CLASS_IDENTIFIER:
            return (char *)token->constant.ptr_constant;
        break;

        case TOKEN_CLASS_INSTRUCTION:
            return opcode_info[token->constant.uint_constant].name;
        break;

        case TOKEN_CLASS_PUNCTUATOR:

            switch(token->token_type)
            {
                case TOKEN_PUNCTUATOR_COMMA:
                    return ",";
                break;

                case TOKEN_PUNCTUATOR_DOT:
                    return ".";
                break;

                case TOKEN_PUNCTUATOR_COLON:
                    return ":";
                break;

                case TOKEN_PUNCTUATOR_SEMICOLON:
                    return ";";
                break;

                case TOKEN_PUNCTUATOR_OPARENTHESIS:
                    return "(";
                break;

                case TOKEN_PUNCTUATOR_CPARENTHESIS:
                    return ")";
                break;

                case TOKEN_PUNCTUATOR_OBRACKET:
                    return "[";
                break;

                case TOKEN_PUNCTUATOR_CBRACKET:
                    return "]";
                break;

                case TOKEN_PUNCTUATOR_OBRACE:
                    return "{";
                break;

                case TOKEN_PUNCTUATOR_CBRACE:
                    return "}";
                break;

                case TOKEN_PUNCTUATOR_PLUS:
                    return "+";
                break;

                case TOKEN_PUNCTUATOR_MINUS:
                    return "-";
                break;

                case TOKEN_PUNCTUATOR_EQUAL:
                    return "=";
                break;

				case TOKEN_PUNCTUATOR_ASTERISK:
					return "*";
				break;

				case TOKEN_PUNCTUATOR_SLASH:
					return "/";
				break;
            }

        break;
    }

    return "";
}

void vm_init_lexer(struct vm_lexer_t *lexer, const char *src)
{
    lexer->offset = 0;
    lexer->max_offset = strlen(src);
    lexer->src = src;
    lexer->token.token_class = TOKEN_CLASS_UNKNOWN;
    lexer->lex_blank = 0;
}

// void vm_copy_bytes(char *buffer, uint32_t *offset, void *data, uint32_t size)
// {
//     uint32_t o;

//     o = *offset;
//     memcpy(buffer + o, data, size);
//     o += size;
//     *offset = o;
// }


struct vm_assembler_t vm_init_assembler(const char *src)
{
    struct vm_assembler_t assembler;

    assembler.parsing_instruction = 0;
    vm_init_lexer(&assembler.lexer, src);

    return assembler;
}

uint32_t vm_next_token(struct vm_assembler_t *assembler)
{
    vm_lex_one_token(&assembler->lexer);
    return assembler->parsing_instruction && (assembler->lexer.token.token_class == TOKEN_CLASS_UNKNOWN);
}

#define UNEXPECTED_END_REACHED "unxpected end of tokens reached"


// uint32_t vm_assemble_code_str(struct code_buffer_t *code_buffer, const char *src)
// {
//     return vm_assemble_code(code_buffer, vm_lex_code(src));
// }


uint32_t vm_assemble_code(struct code_buffer_t *code_buffer, const char *src)
{
    uint32_t code_offset = 0;
    uint32_t constant_offset = 0;
    char *code;
    struct opcode_t *opcode;
    // struct opcode_1op_t *opcode_1op;
    // struct opcode_2op_t *opcode_2op;
    struct opcode_3op_t *opcode_3op;
    struct vm_assembler_t assembler;
    uint32_t operand_classes[3];
    uint32_t operand_size;
    uint32_t register_index;

    struct code_label_t *labels = NULL;
    struct code_label_t *next_label = NULL;

    /* TODO: this is outBOUNDS to go wrong... get it? */
    code = calloc(1, 2048);

    assembler = vm_init_assembler(src);
    vm_next_token(&assembler);

    while(assembler.lexer.token.token_class != TOKEN_CLASS_UNKNOWN)
    {
        if(assembler.lexer.token.token_class == TOKEN_CLASS_INSTRUCTION)
        {
            opcode = (struct opcode_t *)(code + code_offset);
            code_offset += opcode_info[assembler.lexer.token.constant.uint_constant].offset;

            opcode->opcode = assembler.lexer.token.constant.uint_constant;
            opcode->operand_count = opcode_info[opcode->opcode].operand_count;

            if(vm_next_token(&assembler))
            {
                vm_set_last_error(UNEXPECTED_END_REACHED);
                return 1;
            }

            assembler.parsing_instruction = 1;
            opcode_3op = (struct opcode_3op_t *)opcode;

            operand_classes[0] = VM_OPCODE_OPERAND_CLASS_NONE;
            operand_classes[1] = VM_OPCODE_OPERAND_CLASS_NONE;
            operand_classes[2] = VM_OPCODE_OPERAND_CLASS_NONE;

            for(uint32_t operand_index = 0; operand_index < opcode->operand_count; operand_index++)
            {
                if(assembler.lexer.token.token_class == TOKEN_CLASS_PUNCTUATOR &&
                    assembler.lexer.token.token_type == TOKEN_PUNCTUATOR_OBRACKET)
                {
                    /* [ */

                    /* memory operand means the value will be used as a pointer */
                    operand_classes[operand_index] = VM_OPCODE_OPERAND_CLASS_MEMORY;

                    if(vm_next_token(&assembler))
                    {
                        vm_set_last_error(UNEXPECTED_END_REACHED);
                        return 1;
                    }
                }

                switch(assembler.lexer.token.token_class)
                {
                    case TOKEN_CLASS_STRING_CONSTANT:
                        if(operand_classes[operand_index] == VM_OPCODE_OPERAND_CLASS_MEMORY)
                        {
                            vm_set_last_error("cannot use string constant as pointer");
                            return 1;
                        }

                        if(!(opcode_info[opcode->opcode].allowed_operand_types[operand_index] & OPERAND_TYPE_STRING_CONSTANT))
                        {
                            vm_set_last_error("string constant not allowed for operand %d of %s", operand_index, opcode_info[opcode->opcode].name);
                            return 1;
                        }

                        operand_classes[operand_index] = VM_OPCODE_OPERAND_CLASS_STRING_CONSTANT;
                        opcode_3op->operands[operand_index].ptr_operand = strdup(assembler.lexer.token.constant.ptr_constant);
                        constant_offset += strlen((char *)assembler.lexer.token.constant.ptr_constant) + 1;

                        if(vm_next_token(&assembler))
                        {
                            vm_set_last_error(UNEXPECTED_END_REACHED);
                            return 1;
                        }
                    break;

                    /* r0, r1, r2, r3, ri0, ri1 */
                    case TOKEN_CLASS_IDENTIFIER:

                        if(operand_classes[operand_index] != VM_OPCODE_OPERAND_CLASS_MEMORY)
                        {
                            operand_classes[operand_index] = VM_OPCODE_OPERAND_CLASS_REGISTER;
                        }

                        for(register_index = 0; register_index < GP_REGS_COUNT; register_index++)
                        {
                            if(!strcmp(gp_reg_names[register_index], (char *)assembler.lexer.token.constant.ptr_constant))
                            {
                                opcode_3op->operands[operand_index].ptr_operand = (gp_regs + register_index);
                                break;
                            }
                        }

                        if(register_index == GP_REGS_COUNT)
                        {
                            /* the operand is not a gpr, so try the interactible registers */

                            for(register_index = 0; register_index < I_REGS_COUNT; register_index++)
                            {
                                if(!strcmp(i_reg_names[register_index], (char *)assembler.lexer.token.constant.ptr_constant))
                                {
                                    opcode_3op->operands[operand_index].ptr_operand = (i_regs + register_index);
                                    break;
                                }
                            }

                            if(register_index == I_REGS_COUNT)
                            {
                                /* not an interactible register, so test to see if this instruction allows labels */
                                if(opcode_info[opcode->opcode].allowed_operand_types[operand_index] & OPERAND_TYPE_LABEL)
                                {
                                    /* we store the name of the label for now, and set the type
                                    of this operand to none. Once all the code has been assembled,
                                    and all the labels discovered, we patch the jmp instructions with
                                    the correct address */
                                    // opcode_3op->operands[operand_index].ptr_operand = alloca(strlen(assembler.lexer.token.constant.ptr_constant) + 1);
                                    // strcpy(opcode_3op->operands[operand_index].ptr_operand, assembler.lexer.token.constant.ptr_constant);
                                    opcode_3op->operands[operand_index].ptr_operand = strdup(assembler.lexer.token.constant.ptr_constant);
                                    operand_classes[operand_index] = VM_OPCODE_OPERAND_CLASS_NONE;
                                }
                                else
                                {
                                    vm_set_last_error("expecting a register name for operand %d, got '%s'", operand_index, vm_translate_token(&assembler.lexer.token));
                                    return 1;
                                }
                            }
                            else
                            {
                                /* this is an interactible register, but... does this instruction allow this register for
                                this operand? */
                                if(!(opcode_info[opcode->opcode].allowed_operand_types[operand_index] & OPERAND_TYPE_I_REGISTER))
                                {
                                    vm_set_last_error("interactible register not allowed for operand %d of %s", operand_index, opcode_info[opcode->opcode].name);
                                    return 1;
                                }
                            }
                        }
                        else
                        {
                            /* this is a gpr, but... does this instruction allow a gpr for this operand? */
                            if(!(opcode_info[opcode->opcode].allowed_operand_types[operand_index] & OPERAND_TYPE_GP_REGISTER))
                            {
                                vm_set_last_error("general purpouse register not allowed for operand %d of %s", operand_index, opcode_info[opcode->opcode].name);
                                return 1;
                            }
                        }

                        if(vm_next_token(&assembler))
                        {
                            vm_set_last_error(UNEXPECTED_END_REACHED);
                            return 1;
                        }
                    break;

                    case TOKEN_CLASS_INTEGER_CONSTANT:
                        /* integer constant */

                        if(!(opcode_info[opcode->opcode].allowed_operand_types[operand_index] & OPERAND_TYPE_INT_CONSTANT))
                        {
                            vm_set_last_error("integer constant not allowed for operand %d of %s", operand_index, opcode_info[opcode->opcode].name);
                            return 1;
                        }

                        operand_classes[operand_index] = VM_OPCODE_OPERAND_CLASS_IMMEDIATE;
                        opcode_3op->operands[operand_index].uint_operand = assembler.lexer.token.constant.uint_constant;
                        constant_offset += sizeof(uint64_t);

                        if(vm_next_token(&assembler))
                        {
                            vm_set_last_error(UNEXPECTED_END_REACHED);
                            return 1;
                        }
                    break;

                    default:
                        vm_set_last_error("expecting operand %d for instruction %s", operand_index, opcode_info[opcode->opcode].name);
                        return 1;
                    break;
                }

                if(operand_classes[operand_index] == VM_OPCODE_OPERAND_CLASS_MEMORY)
                {
                    /* ] */
                    if(assembler.lexer.token.token_class != TOKEN_CLASS_PUNCTUATOR ||
                        assembler.lexer.token.token_type != TOKEN_PUNCTUATOR_CBRACKET)
                    {
                        vm_set_last_error("expecting token ']' after register name, got '%s'", vm_translate_token(&assembler.lexer.token));
                        return 1;
                    }

                    if(vm_next_token(&assembler) && !operand_index)
                    {
                        vm_set_last_error(UNEXPECTED_END_REACHED);
                        return 1;
                    }
                }

                if(operand_index + 1 < opcode->operand_count)
                {
                    /* , (only if not last operand) */
                    if(assembler.lexer.token.token_class != TOKEN_CLASS_PUNCTUATOR ||
                        assembler.lexer.token.token_type != TOKEN_PUNCTUATOR_COMMA)
                    {
                        vm_set_last_error("expecting token ',' after destination operand, got '%s'", vm_translate_token(&assembler.lexer.token));
                        return 1;
                    }

                    if(vm_next_token(&assembler))
                    {
                        vm_set_last_error(UNEXPECTED_END_REACHED);
                        return 1;
                    }
                }
            }

            opcode_3op->operand0_class = operand_classes[0];
            opcode_3op->operand1_class = operand_classes[1];
            opcode_3op->operand2_class = operand_classes[2];

            if(assembler.lexer.token.token_class != TOKEN_CLASS_PUNCTUATOR ||
                assembler.lexer.token.token_type != TOKEN_PUNCTUATOR_SEMICOLON)
            {
                vm_set_last_error("expecting ';' after instruction operands, got '%s'", vm_translate_token(&assembler.lexer.token));
                return 1;
            }

            assembler.parsing_instruction = 0;
            vm_next_token(&assembler);
        }
        else if(assembler.lexer.token.token_class == TOKEN_CLASS_IDENTIFIER)
        {
            assembler.parsing_instruction = 1;
            /* probably a label */

            char *label = (char *)assembler.lexer.token.constant.ptr_constant;

            if(vm_next_token(&assembler))
            {
                vm_set_last_error(UNEXPECTED_END_REACHED);
                return 1;
            }

            if(assembler.lexer.token.token_class == TOKEN_CLASS_PUNCTUATOR)
            {
                if(assembler.lexer.token.token_type == TOKEN_PUNCTUATOR_COLON)
                {
                    /* label_name: */
                    next_label = calloc(1, sizeof(struct code_label_t ));
                    next_label->name = strdup(label);
                    next_label->offset = code_offset;
                    next_label->next = labels;
                    labels = next_label;
                }
                else
                {
                    vm_set_last_error("unexpected token '%s'", vm_translate_token(&assembler.lexer.token));
                    return 1;
                }
            }

            assembler.parsing_instruction = 0;
            vm_next_token(&assembler);
        }
        else
        {
            printf("well, shit...\n");
        }
    }

    if(code_offset + constant_offset)
    {
        code_buffer->code = calloc(1, code_offset + constant_offset);
        code_buffer->length = code_offset + constant_offset;
        code_buffer->code_start = constant_offset;
        memcpy(code_buffer->code + constant_offset, code, code_offset);

        constant_offset = 0;

        vm_set_code_buffer(code_buffer);
        opcode = vm_next_opcode();

        while(opcode)
        {
            if(opcode->operand_count)
            {
                opcode_3op = (struct opcode_3op_t *)opcode;

                operand_classes[0] = opcode_3op->operand0_class;
                operand_classes[1] = opcode_3op->operand1_class;
                operand_classes[2] = opcode_3op->operand2_class;

                for(uint32_t operand_index = 0; operand_index < opcode->operand_count; operand_index++)
                {
                    if(operand_classes[operand_index] == VM_OPCODE_OPERAND_CLASS_NONE)
                    {
                        /* we shouldn't really get here normally, since instructions with operands of the type
                        VM_OPCODE_OPERAND_NONE actually have 0 operands. So, if we got here, it means we need
                        to patch a label name */

                        next_label = labels;

                        while(next_label)
                        {
                            if(!strcmp(next_label->name, (char *)opcode_3op->operands[operand_index].ptr_operand))
                            {
                                /* we found the label, so patch the operand value of this instruction... */


                                /* the label name was strdup'ed, so free it here... */
                                free(opcode_3op->operands[operand_index].ptr_operand);
                                opcode_3op->operands[operand_index].ptr_operand = code_buffer->code + code_buffer->code_start + next_label->offset;
                                break;
                            }

                            next_label = next_label->next;
                        }

                        if(!next_label)
                        {
                            vm_set_last_error("label %s not found", (char *)opcode_3op->operands[operand_index].ptr_operand);
                            return 1;
                        }

                        /* we also adjust the operand type for the correct one */
                        operand_classes[operand_index] = VM_OPCODE_OPERAND_CLASS_IMMEDIATE;
                    }

                    if(operand_classes[operand_index] == VM_OPCODE_OPERAND_CLASS_STRING_CONSTANT)
                    {
                        /* this instruction takes a string constant as operand, so copy the string
                        to the constant area */
                        operand_size = strlen((char *)opcode_3op->operands[operand_index].ptr_operand) + 1;
                        memcpy(code_buffer->code + constant_offset, (char *)opcode_3op->operands[operand_index].ptr_operand, operand_size);
                        free(opcode_3op->operands[operand_index].ptr_operand);
                        opcode_3op->operands[operand_index].ptr_operand = code_buffer->code + constant_offset;
                        constant_offset += operand_size;
                    }
                }

                opcode_3op->operand0_class = operand_classes[0];
                opcode_3op->operand1_class = operand_classes[1];
                opcode_3op->operand2_class = operand_classes[2];
            }

            opcode = vm_next_opcode();
        }
    }

    while(labels)
    {
        next_label = labels->next;
        free(labels->name);
        free(labels);
        labels = next_label;
    }

    return 0;
}

void vm_dissasemble_code(struct code_buffer_t *code_buffer)
{
    /* TODO: implement this shit correctly */

    // uint32_t code_offset;
    // struct opcode_t *opcode;
    // struct opcode_1op_t *opcode_1op;
    // struct opcode_3op_t *opcode_3op;

    // vm_set_code_buffer(code_buffer);
    // opcode = vm_next_opcode();

    // while(opcode)
    // {
    //     // printf("%d\n", opcode->opcode);
    //     switch(opcode->opcode)
    //     {
    //         case VM_OPCODE_PRINT:
    //             opcode_1op = (struct opcode_1op_t *)opcode;
    //             printf("print %s\n", opcode_1op->operand);
    //         break;

    //         case VM_OPCODE_MOV:
    //             {
    //                 struct opcode_2op_t *opcode_2op;
    //                 uint32_t operand_types[2];

    //                 opcode_2op = (struct opcode_2op_t *)opcode;
    //                 operand_types[0] = opcode_2op->operand0_type;
    //                 operand_types[1] = opcode_2op->operand1_type;

    //                 printf("mov ");

    //                 for(uint32_t operand_index = 0; operand_index < 2; operand_index++)
    //                 {
    //                     switch(operand_types[operand_index])
    //                     {
    //                         case VM_OPCODE_OPERAND_MEMORY:
    //                             switch((uint64_t *)opcode_2op->operands[operand_index] - regs)
    //                             {
    //                                 case 0:
    //                                     printf("[r0]");
    //                                 break;

    //                                 case 1:
    //                                     printf("[r1]");
    //                                 break;

    //                                 case 2:
    //                                     printf("[r2]");
    //                                 break;

    //                                 case 3:
    //                                     printf("[r3]");
    //                                 break;
    //                             }
    //                         break;

    //                         case VM_OPCODE_OPERAND_REGISTER:
    //                             switch((uint64_t *)opcode_2op->operands[operand_index] - regs)
    //                             {
    //                                 case 0:
    //                                     printf("r0");
    //                                 break;

    //                                 case 1:
    //                                     printf("r1");
    //                                 break;

    //                                 case 2:
    //                                     printf("r2");
    //                                 break;

    //                                 case 3:
    //                                     printf("r3");
    //                                 break;
    //                             }
    //                         break;

    //                         case VM_OPCODE_OPERAND_IMMEDIATE:
    //                             printf("%d ", opcode_2op->operands[operand_index]);
    //                         break;
    //                     }

    //                     if(!operand_index)
    //                     {
    //                         printf(", ");
    //                     }
    //                 }

    //                 printf("\n");
    //             }
    //         break;

    //         case VM_OPCODE_JMP:
    //             {
    //                 struct opcode_1op_t *opcode_1op = (struct opcode_1op_t *)opcode;

    //                 printf("jmp %x\n", opcode_1op->operand);
    //             }
    //         break;

    //         case VM_OPCODE_BE:
    //             {
    //                 struct opcode_1op_t *opcode_1op = (struct opcode_1op_t *)opcode;

    //                 printf("be %x\n", opcode_1op->operand);
    //             }
    //         break;

    //         case VM_OPCODE_BNE:
    //             {
    //                 struct opcode_1op_t *opcode_1op = (struct opcode_1op_t *)opcode;

    //                 printf("bne %x\n", opcode_1op->operand);
    //             }
    //         break;

    //         case VM_OPCODE_BG:
    //             {
    //                 struct opcode_1op_t *opcode_1op = (struct opcode_1op_t *)opcode;

    //                 printf("bg %x\n", opcode_1op->operand);
    //             }
    //         break;

    //         case VM_OPCODE_BL:
    //             {
    //                 struct opcode_1op_t *opcode_1op = (struct opcode_1op_t *)opcode;

    //                 printf("bl %x\n", opcode_1op->operand);
    //             }
    //         break;

    //         case VM_OPCODE_BGE:
    //             {
    //                 struct opcode_1op_t *opcode_1op = (struct opcode_1op_t *)opcode;

    //                 printf("bge %x\n", opcode_1op->operand);
    //             }
    //         break;

    //         case VM_OPCODE_BLE:
    //             {
    //                 struct opcode_1op_t *opcode_1op = (struct opcode_1op_t *)opcode;

    //                 printf("ble %x\n", opcode_1op->operand);
    //             }
    //         break;

    //         case VM_OPCODE_INC:
    //         case VM_OPCODE_DEC:
    //             {
    //                 struct opcode_1op_t *opcode_1op = (struct opcode_1op_t *)opcode;

    //                 if(opcode->opcode == VM_OPCODE_INC)
    //                 {
    //                     printf("inc ");
    //                 }
    //                 else
    //                 {
    //                     printf("dec ");
    //                 }

    //                 switch(opcode_1op->operand0_type)
    //                 {
    //                     case VM_OPCODE_OPERAND_MEMORY:
    //                         switch((uint64_t *)opcode_1op->operand - regs)
    //                         {
    //                             case 0:
    //                                 printf("[r0]");
    //                             break;

    //                             case 1:
    //                                 printf("[r1]");
    //                             break;

    //                             case 2:
    //                                 printf("[r2]");
    //                             break;

    //                             case 3:
    //                                 printf("[r3]");
    //                             break;
    //                         }
    //                     break;

    //                     case VM_OPCODE_OPERAND_REGISTER:
    //                         switch((uint64_t *)opcode_1op->operand - regs)
    //                         {
    //                             case 0:
    //                                 printf("r0");
    //                             break;

    //                             case 1:
    //                                 printf("r1");
    //                             break;

    //                             case 2:
    //                                 printf("r2");
    //                             break;

    //                             case 3:
    //                                 printf("r3");
    //                             break;
    //                         }
    //                     break;
    //                 }

    //                 printf("\n");
    //             }
    //         break;
    //     }

    //     opcode = vm_next_opcode();
    // }
}

void vm_set_code_buffer(struct code_buffer_t *code_buffer)
{
    // reg_i0 = NULL;
    // reg_i1 = NULL;
    reg_scn = NULL;

    reg_status = 0;

    current_code_buffer = *code_buffer;
    reg_pc = current_code_buffer.code + current_code_buffer.code_start;
}

struct opcode_t *vm_next_opcode()
{
    struct opcode_t *opcode = NULL;

    if((char *)reg_pc < current_code_buffer.code + current_code_buffer.length)
    {
        opcode = (struct opcode_t *)reg_pc;
        reg_pc =  (char *)reg_pc + opcode_info[opcode->opcode].offset;
    }

    return opcode;
}

uint64_t vm_alu_op(uint32_t op, uint64_t operand0, uint64_t operand1)
{
    uint64_t result;
    uint64_t zero_mask = 0xffffffffffffffff;
    uint64_t sign_mask = 0x8000000000000000;

    switch(op)
    {
        case VM_ALU_OP_INC:
            operand1 = 1;
        case VM_ALU_OP_ADD:
            result = operand0 + operand1;
        break;

        case VM_ALU_OP_DEC:
            operand1 = 1;
        case VM_ALU_OP_SUB:
            result = operand0 - operand1;
        break;

        case VM_ALU_OP_AND:
            result = operand0 & operand1;
        break;

        case VM_ALU_OP_OR:
            result = operand0 | operand1;
        break;

        case VM_ALU_OP_XOR:
            result = operand0 ^ operand1;
        break;

        case VM_ALU_OP_PASS:
            result = operand0;
        break;
    }

    (result & zero_mask) ? (reg_status &= ~VM_STATUS_FLAG_ZERO) : (reg_status |= VM_STATUS_FLAG_ZERO);
    (result & sign_mask) ? (reg_status |= VM_STATUS_FLAG_NEGATIVE) : (reg_status &= ~VM_STATUS_FLAG_NEGATIVE);

    return result;
}

uint64_t vm_execute_code(struct code_buffer_t *code_buffer)
{
    struct opcode_t *opcode;
    struct opcode_3op_t *opcode_3op;
    // struct opcode_1op_t *opcode_1op;
    // struct opcode_2op_t *opcode_2op;
    void *addresses[3];
    uint32_t operand_classes[3];

    static char input_str[512];
    static char lc_str0[512];
    static char lc_str1[512];
    char *lc_str0ptr = lc_str0;
    char *lc_str1ptr = lc_str1;
    struct interactable_t *interactable;
    struct dat_attrib_t *attrib;
    // struct scene_t *scene;

    vm_set_code_buffer(code_buffer);
    opcode = vm_next_opcode();

    // uint64_t zero_mask = 0xffffffffffffffff;
    // uint64_t sign_mask = 0x8000000000000000;
    uint64_t value0;
    // uint64_t value1;
    char *str0;
    char *str1;

    union operand_t punny = {};

    uint32_t perform_jump;

    while(opcode)
    {
        if(opcode->operand_count)
        {
            opcode_3op = (struct opcode_3op_t *)opcode;
            operand_classes[0] = opcode_3op->operand0_class;
            operand_classes[1] = opcode_3op->operand1_class;
            operand_classes[2] = opcode_3op->operand2_class;

            for(uint32_t operand_index = 0; operand_index < 3; operand_index++)
            {
                if(operand_classes[operand_index] == VM_OPCODE_OPERAND_CLASS_MEMORY)
                {
                    /* here we have a pointer to the address of the operand, so we dereference
                    it once to the get address of the operand */
                    addresses[operand_index] = *(void **)opcode_3op->operands[operand_index].ptr_operand;
                }
                else if(operand_classes[operand_index] == VM_OPCODE_OPERAND_CLASS_REGISTER)
                {
                    /* the address of the register is in memory, so just copy it */
                    addresses[operand_index] = opcode_3op->operands[operand_index].ptr_operand;
                }
                else
                {
                    /* here we have an immediate value in memory, so we take its address */
                    addresses[operand_index] = &opcode_3op->operands[operand_index];
                }
            }
        }

        switch(opcode->opcode)
        {
            case VM_OPCODE_PRINT:
                printf("%s", *(char **)addresses[0]);
            break;

            case VM_OPCODE_MOV:
                value0 = vm_alu_op(VM_ALU_OP_PASS, *(uint64_t *)addresses[1], 0);
                memcpy(addresses[0], &value0, sizeof(uint64_t));
            break;

            case VM_OPCODE_AND:
                value0 = vm_alu_op(VM_ALU_OP_AND, *(uint64_t *)addresses[0], *(uint64_t *)addresses[1]);
                memcpy(addresses[0], &value0, sizeof(uint64_t));
            break;

            case VM_OPCODE_OR:
                value0 = vm_alu_op(VM_ALU_OP_OR, *(uint64_t *)addresses[0], *(uint64_t *)addresses[1]);
                memcpy(addresses[0], &value0, sizeof(uint64_t));
            break;

            case VM_OPCODE_XOR:
                value0 = vm_alu_op(VM_ALU_OP_XOR, *(uint64_t *)addresses[0], *(uint64_t *)addresses[1]);
                memcpy(addresses[0], &value0, sizeof(uint64_t));
            break;

            case VM_OPCODE_JE:
                perform_jump = reg_status & VM_STATUS_FLAG_ZERO;
                goto _test_jump;
            case VM_OPCODE_JNE:
                perform_jump = (reg_status & VM_STATUS_FLAG_ZERO) == 0;
                goto _test_jump;
            case VM_OPCODE_JG:
                perform_jump = (reg_status & (VM_STATUS_FLAG_ZERO | VM_STATUS_FLAG_NEGATIVE)) == 0;
                goto _test_jump;
            case VM_OPCODE_JL:
                perform_jump = reg_status & VM_STATUS_FLAG_NEGATIVE;
                goto _test_jump;
            case VM_OPCODE_JGE:
                perform_jump = (reg_status & VM_STATUS_FLAG_ZERO) | (!(reg_status & VM_STATUS_FLAG_NEGATIVE));
                goto _test_jump;
            case VM_OPCODE_JLE:
                perform_jump = reg_status & (VM_STATUS_FLAG_NEGATIVE | VM_STATUS_FLAG_ZERO);
                goto _test_jump;
            case VM_OPCODE_JMP:
                perform_jump = 1;
                _test_jump:
                if(perform_jump)
                {
                    memcpy(&reg_pc, addresses[0], sizeof(void *));
                }
            break;

            case VM_OPCODE_INC:
                value0 = vm_alu_op(VM_ALU_OP_INC, *(uint64_t *)addresses[0], 0);
                memcpy(addresses[0], &value0, sizeof(uint64_t));
            break;

            case VM_OPCODE_DEC:
                value0 = vm_alu_op(VM_ALU_OP_DEC, *(uint64_t *)addresses[0], 0);
                memcpy(addresses[0], &value0, sizeof(uint64_t));
            break;

            case VM_OPCODE_CMP:
                vm_alu_op(VM_ALU_OP_SUB, *(uint64_t *)addresses[0], *(uint64_t *)addresses[1]);
            break;

            case VM_OPCODE_CMPSLC:
                str0 = *(char **)addresses[0];
                str1 = *(char **)addresses[1];

                value0 = 0;
                while(str0[value0])
                {
                    lc_str0ptr[value0] = tolower(str0[value0]);
                    value0++;
                }
                lc_str0ptr[value0] = '\0';

                value0 = 0;
                while(str1[value0])
                {
                    lc_str1ptr[value0] = tolower(str1[value0]);
                    value0++;
                }
                lc_str1ptr[value0] = '\0';

                addresses[0] = &lc_str0ptr;
                addresses[1] = &lc_str1ptr;

            case VM_OPCODE_CMPS:
                value0 = strcmp(*(char **)addresses[0], *(char **)addresses[1]);
                vm_alu_op(VM_ALU_OP_SUB, value0, 0);
            break;

            case VM_OPCODE_CMPSSTR:
                str0 = *(char **)addresses[0];
            break;

            case VM_OPCODE_LCSTR:
                str0 = *(char **)addresses[0];

                value0 = 0;
                while(str0[value0])
                {
                    str0[value0] = tolower(str0[value0]);
                    value0++;
                }
            break;

            case VM_OPCODE_LDSC:
                reg_scn = get_scene(*(char **)addresses[0]);
            break;

            case VM_OPCODE_LDI:
                interactable = get_interactable(reg_scn, *(char **)addresses[1]);
                memcpy(addresses[0], &interactable, sizeof(struct interactable_t *));
            break;

            case VM_OPCODE_LDIA:
                interactable = *(struct interactable_t **)addresses[1];
                attrib = dat_get_attrib(interactable->attribs, *(char **)addresses[2]);
                value0 = vm_alu_op(VM_ALU_OP_PASS, (uint64_t)&attrib->data, 0);
                memcpy(addresses[0], &value0, sizeof(uint64_t));
            break;

            case VM_OPCODE_CHGSC:
                set_scene(get_scene(*(char **)addresses[0]));
            break;

            case VM_OPCODE_IN:
                fgets(input_str, 512, stdin);
                input_str[strlen(input_str) - 1] = '\0';
                punny.ptr_operand = input_str;
                value0 = vm_alu_op(VM_ALU_OP_PASS, punny.uint_operand, 0);
                memcpy(addresses[0], &value0, sizeof(uint64_t));
            break;

            case VM_OPCODE_RET:
                return *(uint64_t *)addresses[0];
            break;

            case VM_OPCODE_EXIT:
                exit(0);
            break;

            case VM_OPCODE_FCRSH:
                {
                    // free(alloca(512));
                    char *die = NULL;
                    *die = 5;
                }
            break;
        }

        opcode = vm_next_opcode();
    }

    return 0;
}

void vm_print_registers()
{
    printf("reg_pc: 0x%p\n", (void *)reg_pc);
    printf("reg_scn: 0x%p\n", (void *)reg_scn);
    printf("Z: %d | N: %d\n", (reg_status & VM_STATUS_FLAG_ZERO) && 1, (reg_status & VM_STATUS_FLAG_NEGATIVE) && 1);
    for(uint32_t i = 0; i < I_REGS_COUNT; i++)
    {
        printf("%s: 0x%p\n", i_reg_names[i], i_regs[i]);
    }
    for(uint32_t i = 0; i < GP_REGS_COUNT; i++)
    {
        printf("%s: %" PRIu64 "\n", gp_reg_names[i], gp_regs[i]);
    }
}




#define ERROR_STACK_DEPTH 16
#define ERROR_MESSAGE_LEN 128
int32_t error_index = -1;
char last_error[ERROR_STACK_DEPTH][ERROR_MESSAGE_LEN];

void vm_set_last_error(const char *error, ...)
{
    if(error_index < ERROR_STACK_DEPTH)
    {
        error_index++;
        va_list args;
        va_start(args, error);
        vsnprintf(last_error[error_index], ERROR_MESSAGE_LEN, error, args);
    }
}

const char *vm_get_error()
{
    char *error = NULL;

    if(error_index >= 0)
    {
        error = &last_error[error_index][0];
        error_index--;
    }

    return error;
}


struct token_t *vm_alloc_token()
{
    struct token_t *token;

    if(free_tokens)
    {
        token = free_tokens;
        free_tokens = free_tokens->next;
        token->next = NULL;
    }
    else
    {
        token = calloc(1, sizeof(struct token_t));
        token->token_class = TOKEN_CLASS_UNKNOWN;
    }

    return token;
}

void vm_free_token(struct token_t *token)
{
    if(token)
    {
        token->next = free_tokens;
        free_tokens = token;
    }
}

void vm_free_tokens(struct token_t *tokens)
{
    struct token_t *next_token;
    while(tokens)
    {
        next_token = tokens->next;
        vm_free_token(tokens);
        tokens = next_token;
    }
}
