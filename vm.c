#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define str_cap 16
#define vm_cap 256

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

    char * input;
    long input_i;
    long input_len;
} Meta2Vm;

#if defined(__GNUC__) || defined(__clang__)
#define NORETURN __attribute__((noreturn))
#else
#define NORETURN
#endif

NORETURN
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
    if(line[0] == ' ' || line[0] == '\t') {
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
        {
            /*Remove trailing whitespace*/
            const long len = strlen(line);
            if(len > 0 && line[len - 1] == ' ') {
                line[strlen(line) - 1] = 0;
            }
        }
        {
            /*Remove quotes around string arguments*/
            const long len = strlen(line);
            if(len > 0 && line[len - 1] == '\'') {
                line[len - 1] = 0;
            }
            if(line[0] == '\'') {
                ++line;
            }

        }

        memcpy(op.str, line, strlen(line));

        out->opcodes[out->opcode_len] = op;
        ++out->opcode_len;
    } else {
        /*label*/
        memcpy(out->labels[out->labels_len].str, line, strlen(line));
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

/*vm utilities*/

char * vm_input(Meta2Vm* self) {
    assert(self);
    assert(self->input_i >= 0);
    assert(self->input_i <= self->input_len);
    return &self->input[self->input_i];
}

void vm_skip_whitespace(Meta2Vm* self) {
    assert(self);
    for(;isspace(self->input[self->input_i]); ++self->input_i);
    assert(!isspace(vm_input(self)[0]) && "skip whitespace failed");
}

void vm_advance(Meta2Vm * self, long i) {
    assert(self);
    assert(i > 0);
    assert(self->input_i + i < self->input_len && "Reached end of input");
    self->input_i += i;
}

char vm_getch(Meta2Vm * self) {
    const char ch = vm_input(self)[0];

    if(ch == 0) {
        return 0;
    } else {
        vm_advance(self, 1);
        return ch;
    }
}

char vm_peekch(Meta2Vm * self) {
    assert(self);
    return vm_input(self)[0];
}

Label vm_lookup_label(Meta2Vm * self, char * name) {
    int i = 0;
    assert(name);
    assert(self);
    for(i = 0; i < self->labels_len; ++i) {
        const Label l = self->labels[i];
        if(strncmp(l.str, name, str_cap) == 0) {
            return l;
        }
    }
    fatal_error("Undefined label name \"%s\"", name);
}


/*opcode implementations*/
void vm_tst(Meta2Vm* self, char * str) {
    assert(self);
    assert(str && "tst string is NULL");
    assert(strlen(str) > 0 && "empty string given as arg");
    assert(strcspn(str, "'") == strlen(str) && "tst string must not contain single quotes");
    {
        const long len = strlen(str);
        vm_skip_whitespace(self);
        if(strncmp(str, vm_input(self), len) == 0) {
            self->switch_flag = 1;
            vm_advance(self, len);
        } else {
            self->switch_flag = 0;
        }
    }
}

void vm_id(Meta2Vm * self) {
    assert(self);
    vm_skip_whitespace(self);
    if(!isalpha(vm_peekch(self))) {
        self->switch_flag = 0;
        return;
    } else {
        int i = 0;
        self->switch_flag = 1;

        for(;isalpha(vm_peekch(self)) || isdigit(vm_peekch(self)); ++i) {
            assert(i + 1 < (long)sizeof(self->token));
            self->token[i] = vm_getch(self);
            self->token[i + 1] = 0;
        }
    }
}


void vm_num(Meta2Vm * self) {
    assert(self);
    vm_skip_whitespace(self);
    if(!isdigit(vm_peekch(self))) {
        self->switch_flag = 0;
        return;
    } else {
        int i = 0;
        self->switch_flag = 1;

        for(;isdigit(vm_peekch(self)); ++i) {
            assert(i + 1 < (long)sizeof(self->token));
            self->token[i] = vm_getch(self);
            self->token[i + 1] = 0;
        }
    }
}

void vm_sr(Meta2Vm * self) {
    assert(self);
    vm_skip_whitespace(self);
    {
        if(vm_peekch(self) != '\'') {
            self->switch_flag = 0; 
            return;
        } else {
            int i = 0;
            self->switch_flag = 1; 
            self->token[i] = vm_getch(self);
            self->token[i + 1] = 0;
            ++i;

            for(;vm_peekch(self) != '\'' && vm_peekch(self) != 0; ++i) {
                assert(i + 1 < (long)sizeof(self->token));
                self->token[i] = vm_getch(self);
                self->token[i + 1] = 0;
            }

            if(vm_peekch(self) == 0) {
                fatal_error("Unexpected end of input");
            }

            assert(vm_peekch(self) == '\'');
            assert(i + 1 < (long)sizeof(self->token));
            self->token[i] = vm_getch(self);
            self->token[i + 1] = 0;
        }
    }
}

#define ARRAY_LEN(arr) (long)(sizeof(arr) / sizeof(arr[0]))

void vm_cll(Meta2Vm* self, char * subroutine_name) {
    const Label l = vm_lookup_label(self, subroutine_name);
    if(self->stack_len >= ARRAY_LEN(self->stack)) {
        fatal_error("Stack overflow");
    }
    self->stack[self->stack_len].return_address = self->isp;
    memset(self->stack[self->stack_len].label1, 0, str_cap);
    memset(self->stack[self->stack_len].label2, 0, str_cap);
    self->stack_len += 1;
    self->isp = l.isp;
}


void vm_r(Meta2Vm * self) {
    assert(self);
    if(self->stack_len <= 0) {
        fatal_error("Stack underflow");
    } else {
        const StackCell cell = self->stack[self->stack_len - 1];
        self->stack_len -= 1;
        self->isp = cell.return_address;
    }
}

void vm_set(Meta2Vm * self) {
    assert(self);
    self->switch_flag = 1;
}

void vm_b(Meta2Vm * self, char * label) {
    const Label l = vm_lookup_label(self, label);
    self->isp = l.isp;
}


void vm_bt(Meta2Vm * self, char * label) {
    const Label l = vm_lookup_label(self, label);
    if(self->switch_flag) {
        self->isp = l.isp;
    }
}


void vm_bf(Meta2Vm * self, char * label) {
    const Label l = vm_lookup_label(self, label);
    if(!self->switch_flag) {
        self->isp = l.isp;
    }
}

void vm_be(Meta2Vm * self) {
    if(!self->switch_flag) {
        fatal_error("An unknown error ocurred");
    }
}

void run_vm(Meta2Vm* out, char * input, long input_len) {
    out->input = input;
    out->input_len = input_len;
    assert((long)strnlen(input, input_len) == input_len && "given input len is wrong");
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
        (void)i;


    }
    return 0;
}
