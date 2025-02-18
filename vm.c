#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define str_cap 16
#define vm_cap 128

typedef struct {
    enum {
        opcode_tst,
        opcode_id,
        opcode_num,
        opcode_sr,
        opcode_cll,
        opcode_r,
        opcode_set,
        opcode_b,
        opcode_bt,
        opcode_bf,
        opcode_be,
        opcode_cl,
        opcode_ci,
        opcode_gn1,
        opcode_gn2,
        opcode_lb,
        opcode_out,
        opcode_adr,
        opcode_end,
    } id;
    char str[str_cap];
} Opcode;

typedef struct {
    int isp;
    char str[str_cap];
} Label;

typedef struct {
    char label1[str_cap];  
    char label2[str_cap];  
    int return_address;
} StackCell;

typedef struct {
    Opcode opcodes[vm_cap];
    long opcode_len;
    Label labels[vm_cap];
    long labels_len;
    StackCell stack[vm_cap];
    long stack_len;
    char token[str_cap];
    long token_len;
    int switch_flag;
    int isp;
} Meta2Vm;

void fatal_error(const char * fmt, ...) {
    fprintf(stderr, "Error: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fflush(stderr);
    exit(1);
}

void read_file(const char * filename, char * buf, long buflen) {
    FILE * fp = fopen(filename, "rw");
    int i = 0;
    for(;!feof(fp) && i < buflen - 1; ++i) {
        buf[i] = fgetc(fp);         
    }
    fclose(fp);
    buf[i] = 0;
}

char * skip_whitespace(char * str) {
    for(;isspace(*str); ++str);
    return str;
}

char * skip_alpha_digit(char * str) {
    for(;isalpha(*str) || isdigit(*str); ++str);
    return str;
}

#define streql(ptr, literal) strncmp(ptr, literal, sizeof(literal) - 1) == 0

void load_line(char * line, Meta2Vm * out) {
    assert(line && "Line is NULL");
    if(line[0] == ' ') {
        Opcode op = {0};
        /*opcode*/
        line = skip_whitespace(line);

        if(strlen(line) == 0) return;

        if(streql(line, "TST")) op.id = opcode_tst;
        else if(streql(line, "ID")) op.id = opcode_id; 
        else if(streql(line, "NUM")) op.id = opcode_num; 
        else if(streql(line, "SR")) op.id = opcode_sr; 
        else if(streql(line, "CLL")) op.id = opcode_cll; 
        else if(streql(line, "R")) op.id = opcode_r; 
        else if(streql(line, "SET")) op.id = opcode_set; 
        else if(streql(line, "B")) op.id = opcode_b; 
        else if(streql(line, "BT")) op.id = opcode_bt; 
        else if(streql(line, "BF")) op.id = opcode_bf; 
        else if(streql(line, "BE")) op.id = opcode_be; 
        else if(streql(line, "CL")) op.id = opcode_cl; 
        else if(streql(line, "CI")) op.id = opcode_ci; 
        else if(streql(line, "GN1")) op.id = opcode_gn1; 
        else if(streql(line, "GN2")) op.id = opcode_gn2; 
        else if(streql(line, "LB")) op.id = opcode_lb; 
        else if(streql(line, "OUT")) op.id = opcode_out; 
        else if(streql(line, "ADR")) op.id = opcode_adr; 
        else if(streql(line, "END")) op.id = opcode_end; 
        else fatal_error("invalid opcode \"%s\"\n", line);

        line = skip_alpha_digit(line);
        line = skip_whitespace(line);
        memcpy(op.str, line, strcspn(line, "\n"));

        out->opcodes[out->opcode_len] = op;
        ++out->opcode_len;
    } else {
        /*label*/
        memcpy(out->labels[out->labels_len].str, line, strcspn(line, " \n"));
        out->labels[out->labels_len].isp = out->opcode_len;
        ++out->labels_len;
    }
}

void load_vm(char * input, Meta2Vm * out) {
    memset(out, 0, sizeof(Meta2Vm));
    char * line = strtok(input, "\n");

    for(;line != NULL; line = strtok(NULL, "\n")) {
        load_line(line, out); 
    }
}

int main(int argc, char** argv) {
    if(argc != 2) {
        fatal_error("Expected 1 cli argument, found %d\n", argc - 1); 
    } else {
        char * filename = argv[1];
        char buf[2048] = {0};
        read_file(filename, buf, sizeof(buf));
        Meta2Vm vm = {0};
        int i = 0;

        load_vm(buf, &vm);

        i = 100;
        i = 1002;


    }
    return 0;
}
