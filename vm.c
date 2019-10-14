#include "vm.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

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


struct opcode_info_t
{
    char *name;
    uint32_t offset;
    uint32_t operand_count;
};

#define OPCODE(name, offset, operand_count) (struct opcode_info_t){name, offset, operand_count}

// char *opcode_names[VM_OPCODE_LAST] = {};
// uint32_t opcode_offsets[VM_OPCODE_LAST] = {};

struct opcode_info_t opcode_info[VM_OPCODE_LAST] = {};
char char_map[512] = {CHAR_TYPE_BLANK};
struct token_t *free_tokens = NULL;
char *reg_names[] = {"r0", "r1", "r2", "r3"};

#define GPR_COUNT 4

struct interactable_t *reg_i0 = NULL;
struct interactable_t *reg_i1 = NULL;
struct scene_t *reg_scn = NULL;
uint64_t reg_pc;
uint8_t reg_status;
uint64_t regs[GPR_COUNT];
struct code_buffer_t current_code_buffer;

struct vm_assembler_t
{
    uint32_t parsing_instruction;
    struct token_t *prev_token;
    struct token_t *tokens;
};

void vm_init()
{
    opcode_info[VM_OPCODE_PRINT] = OPCODE("print", sizeof(struct opcode_1op_t), 1);
    opcode_info[VM_OPCODE_LDSC] = OPCODE("ldsc", sizeof(struct opcode_1op_t), 1);
    opcode_info[VM_OPCODE_LDSCA] = OPCODE("ldisca", sizeof(struct opcode_2op_t), 2);
    opcode_info[VM_OPCODE_CHGSC] = OPCODE("chgsc", sizeof(struct opcode_1op_t), 1);
    opcode_info[VM_OPCODE_LDI] = OPCODE("ldi", sizeof(struct opcode_1op_t), 1);
    opcode_info[VM_OPCODE_LDIA] = OPCODE("ldia", sizeof(struct opcode_2op_t), 2);
    opcode_info[VM_OPCODE_MOV] = OPCODE("mov", sizeof(struct opcode_2op_t), 2);

    opcode_info[VM_OPCODE_INC] = OPCODE("inc", sizeof(struct opcode_1op_t), 1);
    opcode_info[VM_OPCODE_DEC] = OPCODE("dec", sizeof(struct opcode_1op_t), 1);
    opcode_info[VM_OPCODE_AND] = OPCODE("and", sizeof(struct opcode_2op_t), 2);
    opcode_info[VM_OPCODE_OR] = OPCODE("or", sizeof(struct opcode_2op_t), 2);
    opcode_info[VM_OPCODE_XOR] = OPCODE("xor", sizeof(struct opcode_2op_t), 2);

    opcode_info[VM_OPCODE_CMP] = OPCODE("cmp", sizeof(struct opcode_2op_t), 2);
    opcode_info[VM_OPCODE_JMP] = OPCODE("jmp", sizeof(struct opcode_1op_t), 1);
    opcode_info[VM_OPCODE_BE] = OPCODE("be", sizeof(struct opcode_1op_t), 1);
    opcode_info[VM_OPCODE_BNE] = OPCODE("bne", sizeof(struct opcode_1op_t), 1);
    opcode_info[VM_OPCODE_BG] = OPCODE("bg", sizeof(struct opcode_1op_t), 1);
    opcode_info[VM_OPCODE_BL] = OPCODE("bl", sizeof(struct opcode_1op_t), 1);
    opcode_info[VM_OPCODE_BGE] = OPCODE("bge", sizeof(struct opcode_1op_t), 1);
    opcode_info[VM_OPCODE_BLE] = OPCODE("ble", sizeof(struct opcode_1op_t), 1);

    opcode_info[VM_OPCODE_FCRSH] = OPCODE("fcrsh", sizeof(struct opcode_t), 0);

    

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
    char_map['*'] = CHAR_TYPE_PUNCTUATOR;
    char_map['='] = CHAR_TYPE_PUNCTUATOR;
    char_map['.'] = CHAR_TYPE_PUNCTUATOR;

    char_map['\n'] = CHAR_TYPE_BLANK;
    char_map['\r'] = CHAR_TYPE_BLANK;
    char_map['\t'] = CHAR_TYPE_BLANK;
    char_map['\0'] = CHAR_TYPE_BLANK;
    char_map[' '] = CHAR_TYPE_BLANK;

    // for(uint32_t i = 0; i < GPR_COUNT; i++)
    // {
    //     sprintf(reg_names + i, "r%d", i);
    // }
}

struct token_t *vm_lex_code(const char *src)
{
    uint32_t index = 0;
    uint32_t token_str_index;
    static char token_str[512];
    struct token_t *tokens = NULL;
    struct token_t *next_token = NULL;
    struct token_t *last_token = NULL;
    uint32_t token_class;
    uint32_t token_type;
    uint64_t constant;
    char end_char;
    char escaped;
    float float_constant;

    while(src[index])
    {
        while(char_map[src[index]] == CHAR_TYPE_BLANK && src[index] != '\0') index++;

        if(src[index] != '\0')
        {
            token_class = TOKEN_CLASS_UNKNOWN;

            if(char_map[src[index]] == CHAR_TYPE_PUNCTUATOR)
            {
                if(src[index] == '"' || src[index] == '(')
                {
                    if(src[index] == '"')
                    {
                        token_class = TOKEN_CLASS_STRING_CONSTANT;
                        end_char = '"';
                    }
                    else
                    {
                        token_class = TOKEN_CLASS_CODE;
                        end_char = ')';
                    }
                    
                    token_type = 0;

                    index++;
                    token_str_index = 0;

                    while(src[index] != end_char)
                    {
                        if(src[index] == '\0')
                        {
                            vm_set_last_error("missing '\"' at the end of string constant");
                            return NULL;
                        }

                        if(src[index] == '\\')
                        {
                            index++;

                            escaped = src[index++];

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
                            escaped = src[index++];
                        }
                        
                        token_str[token_str_index++] = escaped;
                    }

                    index++;
                    token_str[token_str_index] = '\0';
                }
                else
                {
                    token_class = TOKEN_CLASS_PUNCTUATOR;
                    
                    switch(src[index])
                    {
                        case ',':
                            index++;
                            token_type = TOKEN_PUNCTUATOR_COMMA;
                        break;

                        case '.':
                            index++;
                            token_type = TOKEN_PUNCTUATOR_DOT;
                        break;

                        case ':':
                            index++;
                            token_type = TOKEN_PUNCTUATOR_COLON;
                        break;      

                        case ';':
                            index++;
                            token_type = TOKEN_PUNCTUATOR_SEMICOLON;
                        break;

                        case '(':
                            index++;
                            token_type = TOKEN_PUNCTUATOR_OPARENTHESIS;
                        break;

                        case ')':
                            index++;
                            token_type = TOKEN_PUNCTUATOR_CPARENTHESIS;
                        break;

                        case '[':
                            index++;
                            token_type = TOKEN_PUNCTUATOR_OBRACKET;
                        break;

                        case ']':
                            index++;
                            token_type = TOKEN_PUNCTUATOR_CBRACKET;
                        break;

                        case '=':
                            index++;
                            token_type = TOKEN_PUNCTUATOR_EQUAL;
                        break;

                        case '{':
                            index++;
                            token_type = TOKEN_PUNCTUATOR_OBRACE;
                        break;

                        case '}':
                            index++;
                            token_type = TOKEN_PUNCTUATOR_CBRACE;
                        break;

                        default:
                            token_class = TOKEN_CLASS_UNKNOWN;
                        break;
                    }
                }
            }
            else
            {
                if(char_map[src[index]] == CHAR_TYPE_LETTER)
                {
                    token_class = TOKEN_CLASS_IDENTIFIER;
                    token_type = 0;
                    token_str_index = 0;

                    while(!(char_map[src[index]] & (CHAR_TYPE_BLANK | CHAR_TYPE_PUNCTUATOR)))
                    {
                        token_str[token_str_index++] = src[index++];
                    }

                    token_str[token_str_index] = '\0';

                    for(uint32_t i = 0; opcode_info[i].name; i++)
                    {
                        if(!strcmp(token_str, opcode_info[i].name))
                        {
                            token_class = TOKEN_CLASS_INSTRUCTION;
                            constant = i;
                            break;
                        }
                    }
                }
                else
                {
                    token_class = TOKEN_CLASS_INTEGER_CONSTANT;
                    token_type = 0;
                    token_str_index = 0;

                    while(char_map[src[index]] == CHAR_TYPE_NUMBER)
                    {
                        token_str[token_str_index++] = src[index++];
                    }

                    if(src[index] == '.')
                    {
                        token_class = TOKEN_CLASS_FLOAT_CONSTANT;
            
                        token_str[token_str_index++] = src[index++];

                        if(char_map[src[index]] != CHAR_TYPE_NUMBER)
                        {
                            vm_set_last_error("unexpected token %c", src[index]);
                            return NULL;
                        }

                        while(char_map[src[index]] == CHAR_TYPE_NUMBER)
                        {
                            token_str[token_str_index++] = src[index++];
                        }

                        /* TODO: allow for scientific notation? */
                        if(char_map[src[index]] != CHAR_TYPE_BLANK)
                        {
                            vm_set_last_error("unexpected token %c", src[index]);
                            return NULL;
                        }

                        token_str[token_str_index] = '\0';
                        float_constant = atof(token_str);

                        constant = *(uint32_t *)&float_constant;
                    }
                    else
                    {
                        token_str[token_str_index] = '\0';
                        constant = atoi(token_str);
                    }
                }
            }

            if(token_class == TOKEN_CLASS_UNKNOWN)
            {
                vm_set_last_error("unknown token %c", src[index]);
                return NULL;
            }

            next_token = vm_alloc_token();
            next_token->next = NULL;
            next_token->token_type = token_type;
            next_token->token_class = token_class;

            if(token_class == TOKEN_CLASS_STRING_CONSTANT || token_class == TOKEN_CLASS_IDENTIFIER || token_class == TOKEN_CLASS_CODE)
            {
                constant = (uint64_t)strdup(token_str);
            }
            
            next_token->constant = constant;

            if(!tokens)
            {
                tokens = next_token;
            }
            else
            {
                last_token->next = next_token;
            }

            last_token = next_token;
        }
    }

    return tokens;
}

void vm_print_tokens(struct token_t *tokens)
{
    char *token_class;

    while(tokens)
    {
        switch(tokens->token_class)
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

        printf("%s: %s\n", token_class, vm_translate_token(tokens));

        tokens = tokens->next;
    }
}

char *vm_translate_token(struct token_t *token)
{
    char *token_class;
    char *token_type;

    static char fmt[512];

    switch(token->token_class)
    {
        case TOKEN_CLASS_STRING_CONSTANT:
            return (char *)token->constant;
        break;

        case TOKEN_CLASS_CODE:
            return (char *)token->constant;
        break;

        case TOKEN_CLASS_INTEGER_CONSTANT:
            sprintf(fmt, "%d", (int32_t )token->constant);
            return fmt;
        break;

        case TOKEN_CLASS_FLOAT_CONSTANT:
            sprintf(fmt, "%f", *(float *)&token->constant);
            return fmt;
        break;

        case TOKEN_CLASS_IDENTIFIER:
            return (char *)token->constant;
        break;

        case TOKEN_CLASS_INSTRUCTION:
            return opcode_info[token->constant].name;
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
            }

        break;
    }

    return "";
}

void vm_copy_bytes(char *buffer, uint32_t *offset, void *data, uint32_t size)
{
    uint32_t o;

    o = *offset;
    memcpy(buffer + o, data, size);
    o += size;
    *offset = o;
}


struct vm_assembler_t vm_init_assembler(struct token_t *tokens)
{
    struct vm_assembler_t assembler;

    assembler.parsing_instruction = 0;
    assembler.tokens = tokens;

    return assembler;
}

uint32_t vm_next_token(struct vm_assembler_t *assembler)
{
    assembler->prev_token = assembler->tokens;
    assembler->tokens = assembler->tokens->next;
    return assembler->parsing_instruction && (!assembler->tokens);
}

#define UNEXPECTED_END_REACHED "unxpected end of tokens reached"

uint32_t vm_assemble_code(struct code_buffer_t *code_buffer, const char *src)
{
    uint32_t code_offset = 0;
    uint32_t constant_offset = 0;
    char *code;
    struct opcode_t *opcode;
    struct opcode_1op_t *opcode_1op;
    struct opcode_2op_t *opcode_2op;
    struct vm_assembler_t assembler;

    struct code_label_t *labels = NULL;
    struct code_label_t *next_label = NULL;

    code = calloc(1, 2048);

    assembler = vm_init_assembler(vm_lex_code(src));

    while(assembler.tokens)
    {
        if(assembler.tokens->token_class == TOKEN_CLASS_INSTRUCTION)
        {
            opcode = (struct opcode_t *)(code + code_offset);
            code_offset += opcode_info[assembler.tokens->constant].offset;

            opcode->opcode = assembler.tokens->constant;
            opcode->operand_count = opcode_info[opcode->opcode].operand_count;

            if(vm_next_token(&assembler))
            {
                vm_set_last_error(UNEXPECTED_END_REACHED);
                return 1;
            }

            switch(opcode->opcode)
            {
                case VM_OPCODE_PRINT:
                case VM_OPCODE_LDSC:
                    assembler.parsing_instruction = 1;

                    {
                        opcode_1op = (struct opcode_1op_t *)opcode;

                        if(assembler.tokens->token_class != TOKEN_CLASS_STRING_CONSTANT)
                        {
                            vm_set_last_error("expecting string constant after print, got '%s'", vm_translate_token(assembler.tokens));
                            return 1;
                        } 

                        opcode_1op->operand0_type = VM_OPCODE_OPERAND_IMMEDIATE;
                        opcode_1op->operand = (void *)assembler.tokens->constant;
                        constant_offset += strlen((char *)assembler.tokens->constant) + 1;

                        if(vm_next_token(&assembler))
                        {
                            vm_set_last_error(UNEXPECTED_END_REACHED);
                            return 1;
                        }
                    }
                break;

                case VM_OPCODE_LDSCA:
                case VM_OPCODE_LDI:
                    assembler.parsing_instruction = 1;
                    {
                        opcode_2op = (struct opcode_2op_t *)opcode;

                        if(assembler.tokens->token_class == TOKEN_CLASS_IDENTIFIER)
                        {
                            opcode_2op->operand0_type = VM_OPCODE_OPERAND_REGISTER;

                            if(opcode->opcode == VM_OPCODE_LDSCA)
                            {
                                uint32_t register_index;
                                for(register_index = 0; register_index < GPR_COUNT; register_index++)
                                {
                                    if(!strcmp((char *)assembler.tokens->constant, reg_names[register_index]))
                                    {
                                        opcode_2op->operands[0] = regs + register_index;
                                        break;
                                    }
                                }

                                if(register_index == GPR_COUNT)
                                {
                                    vm_set_last_error("expecting a register name, got '%s'", vm_translate_token(assembler.tokens));
                                    return 1;
                                }
                            }
                            else
                            {
                                if(!strcmp((char *)assembler.tokens->constant, "ri0"))
                                {
                                    opcode_2op->operands[0] = &reg_i0;
                                }
                                else if(!strcmp((char *)assembler.tokens->constant, "ri1"))
                                {
                                    opcode_2op->operands[0] = &reg_i1;
                                }
                                else
                                {
                                    vm_set_last_error("expecting an interactable register, got '%s'", vm_translate_token(assembler.tokens));
                                    return 1;
                                }
                            }
                        }
                        else
                        {
                            vm_set_last_error("expecting a register name, got '%s'", vm_translate_token(assembler.tokens));
                            return 1;
                        }

                        if(vm_next_token(&assembler))
                        {
                            vm_set_last_error(UNEXPECTED_END_REACHED);
                            return 1;
                        }

                        if(assembler.tokens->token_class == TOKEN_CLASS_STRING_CONSTANT)
                        {
                            opcode_2op->operand1_type = VM_OPCODE_OPERAND_IMMEDIATE;
                            opcode_2op->operands[1] = (void *)assembler.tokens->constant;
                            constant_offset += strlen((char *)assembler.tokens->constant) + 1;
                        }
                        else
                        {
                            vm_set_last_error("expecting string constant, got '%s'", vm_translate_token(assembler.tokens));
                            return 1;
                        }

                        if(vm_next_token(&assembler))
                        {
                            vm_set_last_error(UNEXPECTED_END_REACHED);
                            return 1;
                        }
                    }
                break;

                case VM_OPCODE_MOV:
                case VM_OPCODE_CMP:
                case VM_OPCODE_AND:
                case VM_OPCODE_OR:
                case VM_OPCODE_XOR:
                    assembler.parsing_instruction = 1;

                    {
                        opcode_2op = (struct opcode_2op_t *)opcode;
                        uint32_t operand_types[2] = {VM_OPCODE_OPERAND_NONE};

                        for(uint32_t operand_index = 0; operand_index < 2; operand_index++)
                        {
                            if(assembler.tokens->token_class == TOKEN_CLASS_PUNCTUATOR &&
                               assembler.tokens->token_type == TOKEN_PUNCTUATOR_OBRACKET)
                            {
                                /* [ */

                                /* memory operand means the value will be used as a pointer */
                                operand_types[operand_index] = VM_OPCODE_OPERAND_MEMORY;

                                if(vm_next_token(&assembler))
                                {
                                    vm_set_last_error(UNEXPECTED_END_REACHED);
                                    return 1;
                                }
                            }

                            switch(assembler.tokens->token_class)
                            {
                                /* r0, r1, r2, r3 */
                                case TOKEN_CLASS_IDENTIFIER:
                                    
                                    if(operand_types[operand_index] != VM_OPCODE_OPERAND_MEMORY)
                                    {
                                        operand_types[operand_index] = VM_OPCODE_OPERAND_REGISTER;
                                    }

                                    uint32_t register_index;
                                    for(register_index = 0; register_index < GPR_COUNT; register_index++)
                                    {
                                        if(!strcmp(reg_names[register_index], (char *)assembler.tokens->constant))
                                        {
                                            opcode_2op->operands[operand_index] = (regs + register_index);
                                            break;
                                        }
                                    }

                                    if(register_index == GPR_COUNT)
                                    {
                                        vm_set_last_error("expecting a register name for the %s operand, got '%s'", operand_index ? "source" : "destination", vm_translate_token(assembler.tokens));
                                        return 1;
                                    }
                                break;

                                case TOKEN_CLASS_INTEGER_CONSTANT:
                                    /* integer constant */
                                    operand_types[operand_index] = VM_OPCODE_OPERAND_IMMEDIATE;
                                    opcode_2op->operands[operand_index] = (void *)assembler.tokens->constant;
                                    constant_offset += sizeof(uint64_t);
                                break;

                                default:
                                    vm_set_last_error("expecting %s operand", operand_index ? "source" : "destination");
                                    return 1;
                                break;
                            }

                            if(vm_next_token(&assembler))
                            {
                                vm_set_last_error(UNEXPECTED_END_REACHED);
                                return 1;
                            }

                            if(operand_types[operand_index] == VM_OPCODE_OPERAND_MEMORY)
                            {
                                /* ] */
                                if(assembler.tokens->token_class != TOKEN_CLASS_PUNCTUATOR ||
                                   assembler.tokens->token_type != TOKEN_PUNCTUATOR_CBRACKET)
                                {
                                    vm_set_last_error("expecting token ']' after register name, got '%s'", vm_translate_token(assembler.tokens));
                                    return 1;
                                }
                                
                                if(vm_next_token(&assembler) && !operand_index)
                                {
                                    vm_set_last_error(UNEXPECTED_END_REACHED);
                                    return 1;
                                }
                            }

                            if(!operand_index)
                            {
                                /* , (only if this is the first operand) */
                                if(assembler.tokens->token_class != TOKEN_CLASS_PUNCTUATOR ||
                                    assembler.tokens->token_type != TOKEN_PUNCTUATOR_COMMA)
                                {
                                    vm_set_last_error("expecting token ',' after destination operand, got '%s'", vm_translate_token(assembler.tokens));
                                    return 1;
                                }

                                if(vm_next_token(&assembler))
                                {
                                    vm_set_last_error(UNEXPECTED_END_REACHED);
                                    return 1;
                                }
                            }
                        }

                        opcode_2op->operand0_type = operand_types[0];
                        opcode_2op->operand1_type = operand_types[1];
                    }
                break;

                case VM_OPCODE_JMP:
                case VM_OPCODE_BE:
                case VM_OPCODE_BNE:
                case VM_OPCODE_BG:
                case VM_OPCODE_BL:
                case VM_OPCODE_BGE:
                case VM_OPCODE_BLE:

                    assembler.parsing_instruction = 1;
                    opcode_1op = (struct opcode_1op_t *)opcode;

                    if(assembler.tokens->token_class == TOKEN_CLASS_IDENTIFIER)
                    {
                        /* we store the name of the label for now, and set the type
                        of this operand to none. Once all the code has been assembled, 
                        and all the labels discovered, we patch the jmp instructions with 
                        the correct address */
                        opcode_1op->operand = (void *)assembler.tokens->constant;
                        opcode_1op->operand0_type = VM_OPCODE_OPERAND_NONE;

                        if(vm_next_token(&assembler))
                        {
                            vm_set_last_error(UNEXPECTED_END_REACHED);
                            return 1;
                        }
                    }
                break;

                case VM_OPCODE_INC:
                case VM_OPCODE_DEC:
                    assembler.parsing_instruction = 1;
                    opcode_1op = (struct opcode_1op_t *)opcode;

                    {
                        if(assembler.tokens->token_class == TOKEN_CLASS_PUNCTUATOR &&
                           assembler.tokens->token_type == TOKEN_PUNCTUATOR_OBRACE)
                        {
                            opcode_1op->operand0_type = VM_OPCODE_OPERAND_MEMORY;
                            
                            if(vm_next_token(&assembler))
                            {
                                vm_set_last_error(UNEXPECTED_END_REACHED);
                                return 1;
                            }
                        }

                        if(assembler.tokens->token_class != TOKEN_CLASS_IDENTIFIER)
                        {
                            vm_set_last_error("expecting register, got '%s'", vm_translate_token(assembler.tokens));
                            return 1;
                        }
                        
                        uint32_t register_index;

                        for(register_index = 0; register_index < GPR_COUNT; register_index++)
                        {
                            if(!strcmp((char *)assembler.tokens->constant, reg_names[register_index]))
                            {
                                opcode_1op->operand = regs + register_index;
                                break;
                            }
                        }

                        if(register_index == GPR_COUNT)
                        {
                            vm_set_last_error("expecting register, got '%s'", vm_translate_token(assembler.tokens));
                            return 1;
                        }

                        if(vm_next_token(&assembler))
                        {
                            vm_set_last_error(UNEXPECTED_END_REACHED);
                            return 1;
                        }

                        if(opcode_1op->operand0_type == VM_OPCODE_OPERAND_MEMORY)
                        {
                            if(assembler.tokens->token_class == TOKEN_CLASS_PUNCTUATOR ||
                               assembler.tokens->token_type == TOKEN_PUNCTUATOR_CBRACE)
                            {
                                vm_set_last_error("expected ']' after register, got '%s'", vm_translate_token(assembler.tokens));
                                return 1;
                            }

                            if(vm_next_token(&assembler))
                            {
                                vm_set_last_error(UNEXPECTED_END_REACHED);
                                return 1;
                            }
                        }
                        else
                        {
                            opcode_1op->operand0_type = VM_OPCODE_OPERAND_REGISTER;
                        }   
                    }
                break;
            }

            if(assembler.tokens->token_class != TOKEN_CLASS_PUNCTUATOR ||
               assembler.tokens->token_type != TOKEN_PUNCTUATOR_SEMICOLON)
            {
                vm_set_last_error("expecting ';' after instruction operands, got '%s'", vm_translate_token(assembler.tokens));
                return 1;   
            }

            assembler.parsing_instruction = 0;
            vm_next_token(&assembler);
        }
        else if(assembler.tokens->token_class == TOKEN_CLASS_IDENTIFIER)
        {
            assembler.parsing_instruction = 1;
            /* probably a label */

            char *label = (char *)assembler.tokens->constant;

            if(vm_next_token(&assembler))
            {
                vm_set_last_error(UNEXPECTED_END_REACHED);
                return 1;
            }

            if(assembler.tokens->token_class == TOKEN_CLASS_PUNCTUATOR)
            {
                if(assembler.tokens->token_type == TOKEN_PUNCTUATOR_COLON)
                {
                    /* label_name: */

                    next_label = calloc(1, sizeof(struct code_label_t ));
                    next_label->name = label;
                    next_label->offset = code_offset;
                    next_label->next = labels;
                    labels = next_label;
                }
                else
                {
                    vm_set_last_error("unexpected token '%s'", vm_translate_token(assembler.tokens));
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
                opcode_2op = (struct opcode_2op_t *)opcode;
                uint32_t operand_types[2];
                uint32_t operand_size;

                operand_types[0] = opcode_2op->operand0_type;
                operand_types[1] = opcode_2op->operand1_type;

                
                for(uint32_t operand_index = 0; operand_index < opcode->operand_count; operand_index++)
                {
                    if(operand_types[operand_index] == VM_OPCODE_OPERAND_NONE)
                    {
                        /* we shouldn't really get here normally, since instructions with operands of the type
                        VM_OPCODE_OPERAND_NONE actually have 0 operands. So, if we got here, it means we need
                        to patch a label name */
                        
                        next_label = labels;

                        while(next_label)
                        {
                            if(!strcmp(next_label->name, (char *)opcode_2op->operands[operand_index]))
                            {
                                /* we found the label, so patch the operand value of this instruction... */
                                opcode_2op->operands[operand_index] = code_buffer->code + code_buffer->code_start + next_label->offset;
                                break;
                            }
                            next_label = next_label->next;
                        }

                        /* we also adjust the operand type for the correct one */
                        operand_types[operand_index] = VM_OPCODE_OPERAND_IMMEDIATE;
                    }
    
                    if(operand_types[operand_index] == VM_OPCODE_OPERAND_IMMEDIATE)
                    {
                        if(opcode->opcode == VM_OPCODE_PRINT)
                        {
                            /* the print struction */
                            operand_size = strlen(opcode_2op->operands[operand_index]) + 1;
                            memcpy(code_buffer->code + constant_offset, opcode_2op->operands[operand_index], operand_size);
                            opcode_2op->operands[operand_index] = code_buffer->code + constant_offset;
                            constant_offset += operand_size;
                        }
                    }
                }

                opcode_2op->operand0_type = operand_types[0];
                opcode_2op->operand1_type = operand_types[1];
            }

            opcode = vm_next_opcode();
        }
    }

    return 0;
}

void vm_dissasemble_code(struct code_buffer_t *code_buffer)
{
    uint32_t code_offset;
    struct opcode_t *opcode;
    struct opcode_1op_t *opcode_1op;

    vm_set_code_buffer(code_buffer);
    opcode = vm_next_opcode();

    while(opcode)
    {
        // printf("%d\n", opcode->opcode);
        switch(opcode->opcode)
        {
            case VM_OPCODE_PRINT:
                opcode_1op = (struct opcode_1op_t *)opcode;
                printf("print %s\n", opcode_1op->operand);
            break;

            case VM_OPCODE_MOV:
                {
                    struct opcode_2op_t *opcode_2op;
                    uint32_t operand_types[2];

                    opcode_2op = (struct opcode_2op_t *)opcode;
                    operand_types[0] = opcode_2op->operand0_type;
                    operand_types[1] = opcode_2op->operand1_type;

                    printf("mov ");

                    for(uint32_t operand_index = 0; operand_index < 2; operand_index++)
                    {
                        switch(operand_types[operand_index])
                        {
                            case VM_OPCODE_OPERAND_MEMORY:
                                switch((uint64_t *)opcode_2op->operands[operand_index] - regs)
                                {
                                    case 0:
                                        printf("[r0]");
                                    break;

                                    case 1:
                                        printf("[r1]");
                                    break;

                                    case 2:
                                        printf("[r2]");
                                    break;

                                    case 3:
                                        printf("[r3]");
                                    break;
                                }
                            break;

                            case VM_OPCODE_OPERAND_REGISTER:
                                switch((uint64_t *)opcode_2op->operands[operand_index] - regs)
                                {
                                    case 0:
                                        printf("r0");
                                    break;

                                    case 1:
                                        printf("r1");
                                    break;

                                    case 2:
                                        printf("r2");
                                    break;

                                    case 3:
                                        printf("r3");
                                    break;
                                }
                            break;

                            case VM_OPCODE_OPERAND_IMMEDIATE:
                                printf("%d ", opcode_2op->operands[operand_index]);
                            break;
                        }

                        if(!operand_index)
                        {
                            printf(", ");
                        }
                    }

                    printf("\n");
                }
            break;

            case VM_OPCODE_JMP:
                {
                    struct opcode_1op_t *opcode_1op = (struct opcode_1op_t *)opcode;

                    printf("jmp %x\n", opcode_1op->operand);
                }
            break;

            case VM_OPCODE_BE:
                {
                    struct opcode_1op_t *opcode_1op = (struct opcode_1op_t *)opcode;

                    printf("be %x\n", opcode_1op->operand);
                }
            break;

            case VM_OPCODE_BNE:
                {
                    struct opcode_1op_t *opcode_1op = (struct opcode_1op_t *)opcode;

                    printf("bne %x\n", opcode_1op->operand);
                }
            break;

            case VM_OPCODE_BG:
                {
                    struct opcode_1op_t *opcode_1op = (struct opcode_1op_t *)opcode;

                    printf("bg %x\n", opcode_1op->operand);
                }
            break;

            case VM_OPCODE_BL:
                {
                    struct opcode_1op_t *opcode_1op = (struct opcode_1op_t *)opcode;

                    printf("bl %x\n", opcode_1op->operand);
                }
            break;

            case VM_OPCODE_BGE:
                {
                    struct opcode_1op_t *opcode_1op = (struct opcode_1op_t *)opcode;

                    printf("bge %x\n", opcode_1op->operand);
                }
            break;

            case VM_OPCODE_BLE:
                {
                    struct opcode_1op_t *opcode_1op = (struct opcode_1op_t *)opcode;

                    printf("ble %x\n", opcode_1op->operand);
                }
            break;

            case VM_OPCODE_INC:
            case VM_OPCODE_DEC:
                {
                    struct opcode_1op_t *opcode_1op = (struct opcode_1op_t *)opcode;

                    if(opcode->opcode == VM_OPCODE_INC)
                    {
                        printf("inc ");
                    }
                    else
                    {
                        printf("dec ");
                    }

                    switch(opcode_1op->operand0_type)
                    {
                        case VM_OPCODE_OPERAND_MEMORY:
                            switch((uint64_t *)opcode_1op->operand - regs)
                            {
                                case 0:
                                    printf("[r0]");
                                break;

                                case 1:
                                    printf("[r1]");
                                break;

                                case 2:
                                    printf("[r2]");
                                break;
                                
                                case 3:
                                    printf("[r3]");
                                break;
                            }  
                        break;

                        case VM_OPCODE_OPERAND_REGISTER:
                            switch((uint64_t *)opcode_1op->operand - regs)
                            {
                                case 0:
                                    printf("r0");
                                break;

                                case 1:
                                    printf("r1");
                                break;

                                case 2:
                                    printf("r2");
                                break;
                                
                                case 3:
                                    printf("r3");
                                break;
                            } 
                        break;
                    }

                    printf("\n");
                }
            break;
        }

        opcode = vm_next_opcode();
    }
}

void vm_set_code_buffer(struct code_buffer_t *code_buffer)
{
    reg_i0 = NULL;
    reg_i1 = NULL;
    reg_scn = NULL;

    reg_status = 0;

    current_code_buffer = *code_buffer;
    reg_pc = (uint64_t)(current_code_buffer.code + current_code_buffer.code_start);
}

struct opcode_t *vm_next_opcode()
{
    struct opcode_t *opcode = NULL;

    if(reg_pc < (uint64_t)(current_code_buffer.code + current_code_buffer.length))
    {
        opcode = (struct opcode_t *)reg_pc;
        reg_pc += opcode_info[opcode->opcode].offset;
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

void vm_execute_code(struct code_buffer_t *code_buffer)
{
    struct opcode_t *opcode;
    struct opcode_1op_t *opcode_1op;
    struct opcode_2op_t *opcode_2op;
    void *addresses[2];
    uint32_t operand_types[2];
    
    vm_set_code_buffer(code_buffer);
    opcode = vm_next_opcode();

    uint64_t zero_mask = 0xffffffffffffffff;
    uint64_t sign_mask = 0x8000000000000000;
    uint64_t value0;
    uint64_t value1;

    uint32_t perform_jump;
    
    while(opcode)
    {
        if(opcode->operand_count)
        {
            opcode_2op = (struct opcode_2op_t *)opcode;
            operand_types[0] = opcode_2op->operand0_type;
            operand_types[1] = opcode_2op->operand1_type;

            for(uint32_t operand_index = 0; operand_index < 2; operand_index++)
            {
                if(operand_types[operand_index] == VM_OPCODE_OPERAND_MEMORY)
                {
                    /* here we have a pointer to the address of the operand, so we dereference
                    it once to the get address of the operand */
                    addresses[operand_index] = *(void **)opcode_2op->operands[operand_index];
                }
                else if(operand_types[operand_index] == VM_OPCODE_OPERAND_REGISTER)
                {
                    /* the address of the register is in memory, so just copy it */
                    addresses[operand_index] = opcode_2op->operands[operand_index];
                }
                else
                {
                    /* here we have an immediate value in memory, so we take its address */
                    addresses[operand_index] = &opcode_2op->operands[operand_index];
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

            case VM_OPCODE_BE:
                perform_jump = reg_status & VM_STATUS_FLAG_ZERO;
                goto _test_jump;
            case VM_OPCODE_BNE:
                perform_jump = (reg_status & VM_STATUS_FLAG_ZERO) == 0;
                goto _test_jump;
            case VM_OPCODE_BG:
                perform_jump = (reg_status & (VM_STATUS_FLAG_ZERO | VM_STATUS_FLAG_NEGATIVE)) == 0;
                goto _test_jump;
            case VM_OPCODE_BL:
                perform_jump = reg_status & VM_STATUS_FLAG_NEGATIVE;
                goto _test_jump;
            case VM_OPCODE_BGE:
                perform_jump = (reg_status & VM_STATUS_FLAG_ZERO) | (!(reg_status & VM_STATUS_FLAG_NEGATIVE));
                goto _test_jump;
            case VM_OPCODE_BLE:
                perform_jump = reg_status & (VM_STATUS_FLAG_NEGATIVE | VM_STATUS_FLAG_ZERO);
                goto _test_jump;
            case VM_OPCODE_JMP:
                perform_jump = 1;
                _test_jump:
                if(perform_jump)
                {
                    memcpy(&reg_pc, addresses[0], sizeof(uint64_t));
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

            case VM_OPCODE_LDSC:
                reg_scn = get_scene(*(char **)addresses[0]);
            break;

            case VM_OPCODE_LDI:
                
            break;

            case VM_OPCODE_FCRSH:
                {
                    free(alloca(512));
                    char *die = NULL;
                    *die = 5;
                }
            break;
        }

        opcode = vm_next_opcode();
    }
}

void vm_print_registers()
{
    printf("reg_pc: 0x%x\n", reg_pc);
    printf("reg_scn: 0x%x\n", reg_scn);
    printf("reg_i0: 0x%x\n", reg_i0);
    printf("reg_i1: 0x%x\n", reg_i1);
    printf("Z: %d | N: %d\n", (reg_status & VM_STATUS_FLAG_ZERO) && 1, (reg_status & VM_STATUS_FLAG_NEGATIVE) && 1);
    for(uint32_t i = 0; i < GPR_COUNT; i++)
    {
        printf("r%d: %d\n", i, regs[i]);
    }
}



char last_error[512];

void vm_set_last_error(const char *error, ...)
{
    va_list args;
    va_start(args, error);
    vsprintf(last_error, error, args);
}

const char *vm_get_last_error()
{
    return last_error;
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

/* ========================================================== */
/* ========================================================== */
/* ========================================================== */

void vm_load_interactable_register(char *item, uint32_t reg)
{

}

void vm_load_scene_register(void *scene)
{

}