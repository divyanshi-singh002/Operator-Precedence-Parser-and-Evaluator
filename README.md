### Project Overview
A stack-based arithmetic expression parser and evaluator
implemented in C.

### Features
- Operator precedence and associativity
- Operand and operator stacks
- PUSH and POP operations
- Step-by-step parsing trace
- Infix-to-postfix conversion
- Arithmetic expression evaluation
- Error handling
- 22 automated test cases
- Fast mode for longer expressions

### Compilation

gcc operator_precedence_review3_FINAL.c -o parser_final.exe -lm

### Execution

Detailed mode:
./parser_final.exe

Automated testing:
./parser_final.exe --self-test

Fast mode:
./parser_final.exe --fast "3+4*2"

### Example

Input: (3+4)*2

Postfix: 3 4 + 2 *

Result: 14

### Testing

22 out of 22 automated test cases passed.

### Language
C
