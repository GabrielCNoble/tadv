#ifndef VM_H
#define VM_H

#include <stdint.h>


enum VM_OPCODES
{
    /* 
        print: prints a string

        'print string', where string is a string constant 
    */

    VM_OPCODE_PRINT = 0,

    /* 
        ldsc: loads the reference of a scene into the scene register.

        'ldsc string', where 'string' is the scene name
    */
    VM_OPCODE_LDSC,           

    /*
        ldsca: gets the reference of a scene attribute. The scene from which
        the attribute belongs will be contained in the scene register

        'ldsca r(0-3), attribute', where 'r(0-3)' is a gpr, and will receive the
        the attribute address. 'attribute' is a string constant, and contains the 
        attribute name. Attributes can be contained inside another attributes, 
        in which case the string constant will be composed by the names of 
        all attributes until reaching the desired attribute, separated by
        a '.'. For example,

        'ldsca r0, "actions.specific.some_attribute"'

        This will look for an attribute called "some_attribute", that's inside
        an struct attribute called "specific", which is inside an struct attribute
        called "actions", and puts its address in r0.

        'ldsca r(0-3), r(0-3)', where r(0-3) is a gpr. The first register is where the
        address will be stored, the second one contains the address of the 
        string that represents the attribute name. Using the same register as source
        and destination is valid.
    */  
    VM_OPCODE_LDSCA,       

    VM_OPCODE_CHGSC,             /* changes to another scene */


    /* 
        ldi: loads an interactable into an interactable register.

        'ldi ri(0-1), interactable', where 'interactable' is a string that represents
        the name of an interactable, and ri(0-1) is a interactible register  
    */
    VM_OPCODE_LDI,

    /*
        ldia: gets the address of an interactable attribute. Attribute
        name works the same as in ldsca

        'ldia r(0-3), ri(0-1), attribute', where 'attribute' is a string that represents
        the interactible attribute, ri(0-1) is an interactible register that holds a pointer
        to a interactible, and r(0-3) is a general purpouse register, which will receive the
        pointer to the attribute
    */
    VM_OPCODE_LDIA,              


    /*
        mov: moves data from/to registers/memory. This instruction allows modifying an 
                                   writing back scene and interactable attributes

        mov r(0-3), r(0-3)
        mov [r(0-3)], r(0-3)
        mov r(0-3), [r(0-3)]
        mov [r(0-3)], [r(0-3)]
        mov r(0-3), constant
        mov [r(0-3)], constant 
    */
    VM_OPCODE_MOV,              


    VM_OPCODE_INC,
    VM_OPCODE_DEC,
    VM_OPCODE_AND,
    VM_OPCODE_OR,
    VM_OPCODE_XOR,

    
    VM_OPCODE_CMP,

    /*
        cmps: compares two strings
    */
    VM_OPCODE_CMPS,

    /*
        cmpslc: transform the strings to lowercase, then compare them 
    */
    VM_OPCODE_CMPSLC,

    /*
        lcstr: converts a string to lowercase
    */
    VM_OPCODE_LCSTR,

    /*
        cmpsstr: compares a substring to a string
    */
    VM_OPCODE_CMPSSTR,

    /*
        jmp: performs an unconditional jump.

        'jmp address', where address is the absotule address to  
        jump to
    */
    VM_OPCODE_JMP,

    /*
        be: branch when equal (Z = 1)
    */
    VM_OPCODE_BE,

    /*
        bne: branch when not equal (Z = 0)
    */
    VM_OPCODE_BNE,

    /*
        bp: branch when greater than (Z = 0 & N = 0)
    */
    VM_OPCODE_BG,

    /*
        bn: branch when lesser than (N = 1)
    */
    VM_OPCODE_BL,

    /*
        bge: branch when greater than or equal to (N = 0 | Z = 1)
    */
    VM_OPCODE_BGE,

    /* 
        ble : branch when lesser than or equal to (N = 1 | Z = 1) 
    */
    VM_OPCODE_BLE,


    VM_OPCODE_IN,

    VM_OPCODE_RET,


    VM_OPCODE_FCRSH,
    
    VM_OPCODE_LAST,
};

enum VM_OPCODE_OPERAND_CLASS
{
    VM_OPCODE_OPERAND_CLASS_NONE = 0,
    VM_OPCODE_OPERAND_CLASS_REGISTER,
    VM_OPCODE_OPERAND_CLASS_MEMORY,
    VM_OPCODE_OPERAND_CLASS_IMMEDIATE,
    VM_OPCODE_OPERAND_CLASS_STRING_CONSTANT,
};

enum VM_STATUS_FLAG
{
    VM_STATUS_FLAG_CARRY = 1,
    VM_STATUS_FLAG_NEGATIVE = 1 << 1,
    VM_STATUS_FLAG_OVERFLOW = 1 << 2,
    VM_STATUS_FLAG_ZERO = 1 << 3,
};

enum VM_ALU_OPS
{
    VM_ALU_OP_ADD = 0,
    VM_ALU_OP_SUB,
    VM_ALU_OP_INC,
    VM_ALU_OP_DEC,
    VM_ALU_OP_CMP,
    VM_ALU_OP_AND,
    VM_ALU_OP_OR,
    VM_ALU_OP_XOR,
    VM_ALU_OP_MUL,
    VM_ALU_OP_DIV,
    VM_ALU_OP_SHL,
    VM_ALU_OP_SHR,
    VM_ALU_OP_PASS,
};

enum TOKEN_CLASS
{
    TOKEN_CLASS_KEYWORD,
    TOKEN_CLASS_CODE,
    TOKEN_CLASS_STRING_CONSTANT,
    TOKEN_CLASS_INTEGER_CONSTANT,
    TOKEN_CLASS_FLOAT_CONSTANT,
    TOKEN_CLASS_IDENTIFIER,
    TOKEN_CLASS_INSTRUCTION,
    TOKEN_CLASS_PUNCTUATOR,
    TOKEN_CLASS_EOF,
    TOKEN_CLASS_UNKNOWN,
};

enum TOKEN_PUNCTUATOR
{
    TOKEN_PUNCTUATOR_COMMA = 0,
    TOKEN_PUNCTUATOR_DOT,
    TOKEN_PUNCTUATOR_COLON,
    TOKEN_PUNCTUATOR_SEMICOLON,
    TOKEN_PUNCTUATOR_OPARENTHESIS,
    TOKEN_PUNCTUATOR_CPARENTHESIS,
    TOKEN_PUNCTUATOR_OBRACKET,
    TOKEN_PUNCTUATOR_CBRACKET,
    TOKEN_PUNCTUATOR_OBRACE,
    TOKEN_PUNCTUATOR_CBRACE,
    TOKEN_PUNCTUATOR_PLUS,
    TOKEN_PUNCTUATOR_MINUS,
    TOKEN_PUNCTUATOR_EQUAL
};

union token_constant_t
{
    uint64_t uint_constant;
    void *ptr_constant;
    float flt_constant;
};
struct token_t
{
    struct token_t *next;
    union token_constant_t constant;
    // uint64_t constant;
    uint32_t token_class;
    uint32_t token_type;
};

#define OPCODE_FIELDS           \
unsigned opcode : 6;            \
unsigned operand_count : 2;     \
unsigned width : 2;             \
unsigned operand0_class : 3;    \
unsigned operand1_class : 3;    \
unsigned operand2_class : 3; 

struct opcode_t
{
    OPCODE_FIELDS;
};

union operand_t
{
    uint64_t uint_operand;
    void *ptr_operand;
};

struct opcode_1op_t
{
    OPCODE_FIELDS;
    union operand_t operand;
};
struct opcode_2op_t
{
    OPCODE_FIELDS;
    union operand_t operands[2];
};

struct opcode_3op_t
{
    OPCODE_FIELDS;
    union operand_t operands[3];
};

struct code_buffer_t
{
    uint32_t length;
    uint32_t code_start;
    char *code;
};

struct code_label_t
{
    struct code_label_t *next;
    char *name;
    uint32_t offset;
};

void vm_init();

struct token_t *vm_lex_code(const char *src);

void vm_print_tokens(struct token_t *tokens);

char *vm_translate_token(struct token_t *token);

uint32_t vm_assemble_code_str(struct code_buffer_t *code_buffer, const char *src);

uint32_t vm_assemble_code(struct code_buffer_t *code_buffer, struct token_t *tokens);

void vm_dissasemble_code(struct code_buffer_t *code_buffer);

void vm_set_code_buffer(struct code_buffer_t *code_buffer);

struct opcode_t *vm_next_opcode();

uint64_t vm_alu_op(uint32_t op, uint64_t operand0, uint64_t operand1);

// void vm_update_flags(uint32_t affect_flags, uint64_t operand0, uint64_t operand1, uint64_t result);

uint64_t vm_execute_code(struct code_buffer_t *code_buffer);

void vm_print_registers();

void vm_set_last_error(const char *error, ...);

const char *vm_get_last_error();

struct token_t *vm_alloc_token();

void vm_free_token(struct token_t *token);

/* ========================================================== */
/* ========================================================== */
/* ========================================================== */

void vm_load_interactable_register(char *item, uint32_t reg);

void vm_load_scene_register(void *scene);

#endif