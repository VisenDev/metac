#include <stdarg.h>
#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define str_cap 16
#define vm_cap 256

void gensym(char * out, int len) {
    static int counter = 0;
    snprintf(out, len, "A%d", counter);
    ++counter;
}

const char * opcode_names[] = {
        "opcode_tst",
        "opcode_id",
        "opcode_num",
        "opcode_sr",
        "opcode_cll",
        "opcode_r",
        "opcode_set",
        "opcode_b",
        "opcode_bt",
        "opcode_bf",
        "opcode_be",
        "opcode_cl",
        "opcode_ci",
        "opcode_gn1",
        "opcode_gn2",
        "opcode_lb",
        "opcode_out",
};

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

    char starting_label[str_cap];
    int isp;

    char * input;
    long input_i;
    long input_len;

    char output[str_cap];
    int output_i;
    int output_column;
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
    abort();
}

void read_file(const char * filename, char * buf, long buflen) {
    FILE * fp = fopen(filename, "rw");
    int i = 0;
    for(;!feof(fp); ++i) {
        assert(i < buflen && "buffer overflow");
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

#define streql(ptr, literal) strncmp(ptr, literal, strlen(literal)) == 0

int load_line(char * line, Meta2Vm * out) {
    assert(line && "Line is NULL");
    assert(strlen(line) > 0 && "empty line");
    if(line[0] == ' ' || line[0] == '\t') {
        Opcode op = {0};
        char opstr[str_cap] = {0};
        /*opcode*/
        line = skip_whitespace(line);

        memcpy(opstr, line, strcspn(line, " \n"));

        if(streql(opstr, "ADR")) {
            line = skip_alpha_digit(line);
            line = skip_whitespace(line);
            memcpy(out->starting_label, line, strnlen(line, str_cap));
            return 1;
        }


        if(streql(opstr, "TST")) op.id = opcode_tst;
        else if(streql(opstr, "ID")) op.id = opcode_id; 
        else if(streql(opstr, "NUM")) op.id = opcode_num; 
        else if(streql(opstr, "SR")) op.id = opcode_sr; 
        else if(streql(opstr, "CLL")) op.id = opcode_cll; 
        else if(streql(opstr, "R")) op.id = opcode_r; 
        else if(streql(opstr, "SET")) op.id = opcode_set; 
        else if(streql(opstr, "BT")) op.id = opcode_bt; 
        else if(streql(opstr, "BF")) op.id = opcode_bf; 
        else if(streql(opstr, "BE")) op.id = opcode_be; 
        else if(streql(opstr, "B")) op.id = opcode_b; 
        else if(streql(opstr, "CL")) op.id = opcode_cl; 
        else if(streql(opstr, "CI")) op.id = opcode_ci; 
        else if(streql(opstr, "GN1")) op.id = opcode_gn1; 
        else if(streql(opstr, "GN2")) op.id = opcode_gn2; 
        else if(streql(opstr, "LB")) op.id = opcode_lb; 
        else if(streql(opstr, "OUT")) op.id = opcode_out; 
        else if(streql(opstr, "END")) {
            printf("END REACHED\n");
            return 0;
        }
        else fatal_error("invalid opcode \"%s\"\n", opstr);

        printf("Parsed \"%s\" into %s\n", opstr, opcode_names[op.id]);

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
        /*printf("Found label: %s\n", line);*/
        /*label*/
        memcpy(out->labels[out->labels_len].str, line, strlen(line));
        out->labels[out->labels_len].isp = out->opcode_len;
        ++out->labels_len;
        if(strncmp(out->starting_label, line, strlen(line)) == 0) {
            printf("STARTING LABEL FOUND\n");
            out->isp = out->opcode_len;
        }
    }
    return 1;
}

void load_vm(char * input, Meta2Vm * out) {
    memset(out, 0, sizeof(Meta2Vm));
    char * line = strtok(input, "\n");

    for(;line != NULL && load_line(line, out); line = strtok(NULL, "\n")) {
    }
    out->stack_len = 1;
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

Label vm_lookup_label(Meta2Vm * self, const char * name) {
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
void vm_tst(Meta2Vm* self, const char * str) {
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

void vm_cll(Meta2Vm* self, const char * subroutine_name) {
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

void vm_b(Meta2Vm * self, const char * label) {
    const Label l = vm_lookup_label(self, label);
    self->isp = l.isp;
}


void vm_bt(Meta2Vm * self, const char * label) {
    const Label l = vm_lookup_label(self, label);
    if(self->switch_flag) {
        self->isp = l.isp;
    }
}


void vm_bf(Meta2Vm * self, const char * label) {
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

void vm_ci(Meta2Vm* self) {
    memcpy(&(self->output[self->output_i]), self->token, strlen(self->token));
    memset(self->token, 0, sizeof(self->token));
}

void vm_cl(Meta2Vm * self, const char * literal) {
    const long len = strnlen(literal, str_cap - 1);
    memcpy(&(self->output[self->output_i]), literal, len + 1);
    self->output_i = len;
    self->output[len] = 0;
}

void vm_gn1(Meta2Vm * self) {
    assert(self->stack_len > 0);
    {
        StackCell * top = &self->stack[self->stack_len - 1];
        if(top->label1[0] == 0) {
            gensym(top->label1, sizeof(top->label1));
        } 
        printf("%s\n", top->label1);
    }
}


void vm_gn2(Meta2Vm * self) {
    assert(self->stack_len > 0);
    {
        StackCell * top = &self->stack[self->stack_len - 1];
        if(top->label2[0] == 0) {
            gensym(top->label2, sizeof(top->label2));
        } 
        printf("%s\n", top->label2);
    }
}

void vm_lb(Meta2Vm * self) {
    self->output_column = 0;
}

void vm_out(Meta2Vm * self) {
    int i = 0;
    for(i = 0; i < self->output_column; ++i) {
        printf(" ");
    }
    printf("%s\n", self->output);
    memset(self->output, 0, sizeof(self->output));
}

void run_vm(Meta2Vm* self, char * input, long input_len) {
    self->input = input;
    self->input_len = input_len;
    assert((long)strnlen(input, input_len) == input_len && "given input len is wrong");
    
    while(self->input_i < self->input_len) {
        const Opcode op = self->opcodes[self->isp];
        printf("Running: %s %s\n", opcode_names[op.id], op.str);
        ++self->isp;
        switch(op.id) {
            case opcode_tst: vm_tst(self, op.str); break;
            case opcode_id: vm_id(self); break;
            case opcode_num: vm_num(self); break;
            case opcode_sr: vm_sr(self); break;
            case opcode_cll: vm_cll(self, op.str); break;
            case opcode_r: vm_r(self); break;
            case opcode_set: vm_set(self); break;
            case opcode_b: vm_b(self, op.str); break;
            case opcode_bt: vm_bt(self, op.str); break;
            case opcode_bf: vm_bf(self, op.str); break;
            case opcode_be: vm_be(self); break;
            case opcode_cl: vm_cl(self, op.str); break;
            case opcode_ci: vm_ci(self); break;
            case opcode_gn1: vm_gn1(self); break;
            case opcode_gn2: vm_gn2(self); break;
            case opcode_lb: vm_lb(self); break;
            case opcode_out: vm_out(self); break;
            default:
                /*invalid opcode*/
                abort();
        }
    }
}


int main(int argc, char** argv) {
    if(argc != 3) {
        fatal_error("Expected 2 cli argument, found %d\n", argc - 1); 
    } else {
        char * code_file= argv[1];
        char * input_file = argv[2];
        char code[9000] = {0};
        char input[9000] = {0};
        read_file(code_file, code, sizeof(code));
        read_file(input_file, input, sizeof(input));
        Meta2Vm vm = {0};

        load_vm(code, &vm);
        run_vm(&vm, input, strnlen(input, sizeof(input)));
    }
    return 0;
}
