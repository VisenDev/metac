/*
META II Order Codes (following figures 6.1 and 6.2 of [Schorre64])

Mnemonic        Purpose                     Actions
------------------------------------------------------------------------------------------
TST 'string'    Test for string in input    After skipping initial whitespace in the input 
                                            string, compare it to the given argument. If 
                                            the comparison is met, skip over the string in 
                                            the input and set switch. If not met, reset switch.

ID              Identifier token            After skipping initial whitespace in the input 
                                            string, test if it begins with an identifier, 
                                            i.e., a letter followed by a sequence of letters 
                                            and/or digits. If so, copy the identifier to the 
                                            token buffer; skip over it in the input; and set 
                                            switch. If not, reset switch.

NUM             Number token                After skipping initial whitespace in the input 
                                            string, test if it begins with a number, i.e., a 
                                            sequence of digits. If so, copy the number to the 
                                            token buffer; skip over it in the input; and set 
                                            switch. If not, reset switch.

SR              String token                After skipping initial whitespace in the input 
                                            string, test if it begins with a string, i.e., a 
                                            single quote followed by a sequence of any 
                                            characters other than a single quote, followed 
                                            by another single quote. If so, copy the string 
                                            (including enclosing quotes) to the token buffer; 
                                            skip over it in the input; and set switch. If not, 
                                            reset switch.

CLL AAA         Call subroutine             Enter the subroutine beginning at label AAA. 
                                            Push a stack frame of three cells onto the stack 
                                            containing:
                                            - label 1 cell, initialized to blank
                                            - label 2 cell, initialized to blank
                                            - location cell, set to the return from call location

R               Return from subroutine      Return from CLL call to the location on the top 
                                            of the stack and pop the stack frame of three cells.

SET             Set switch                  Set the switch to true.

B AAA           Unconditional branch        Branch unconditionally to the label AAA.

BT AAA          Branch if true              If the switch is true, branch to label AAA.

BF AAA          Branch if false             If the switch is false, branch to label AAA.

BE              Branch to error if false    If the switch is false, report error status and halt.

CL 'string'     Copy literal                Copy the variable-length string (without enclosing 
                                            quotes) given as an argument to the output buffer.

CI              Copy input                  Copy the token buffer to the output buffer.

GN1             Generate label 1            If the label 1 cell in the top stack frame is blank, 
                                            generate a unique label and save it in the label 1 
                                            cell. In either case, output the label.

GN2             Generate label 2            Same as GN1 except acting on the label 2 cell.

LB              Move to label field         Set the output buffer column to the first column.

OUT             Output record               Output the output buffer with a line terminator; 
                                            clear it; and set the output buffer column to the 
                                            eighth column.

META II Pseudo Operations (following figure 6.3 of [Schorre64])

Mnemonic        Purpose                     Actions
------------------------------------------------------------------------------------------
ADR AAA         Starting location           Pseudo operation that specifies the starting label 
                                            to call.

END             End of source               Pseudo operation that specifies the end of input.
*/






/* vm infracstructure */

int len;
int i;
int switch_flag;
char * input;
char token[1024];


int isWhitespace(char ch) {
    return ch == ' ' || ch == '\n';
}

void skipWhitespace() {
    for(;i < len && isWhitespace(input[i]); ++i)
    {}
}

int isDigit(char ch) {
    return ch >= '0' && ch <= '9';
}

int isAlpha(char ch) {
    return ch >= 'A' && ch <= 'Z';

}

/* vm opcodes */

void tst(char * str) {
    int start = i;
    skipWhitespace();
    for(; i < len; ++i) {
        if(str[i - start] == 0) {
            switch_flag = 1;
            return;
        } else if(str[i - start] != input[i]) {
            switch_flag = 0;
            return;
        }
    }
}

void id() {
    skipWhitespace();
    if(!isAlpha(input[i])) {
        switch_flag = 0;
        return;
    } else {
        int start = i;
        for(; i < len; ++i) {
            if(isAlpha(input[i]) || isDigit(input[i])) {
                token[i - start] = input[i];
                token[i - start + 1] = 0;
            } else if(input[i] == ' ') {
                switch_flag = 1;
                return;
            } else {
                switch_flag = 0;
                return;
            }
        }
    }
}
