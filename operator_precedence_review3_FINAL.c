/* BCSE307 M21 Review 3 - explicit stack-based operator-precedence parser */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#define MAX 512
#define OUT 4096

typedef struct { double data[MAX]; int top; } ValueStack;
typedef struct { char data[MAX]; int top; } OperatorStack;

static ValueStack values;
static OperatorStack ops;
static char postfix[OUT], error_msg[160];
static int step_no, detailed = 1, pushes = 0, pops = 0, operands = 0, operators = 0;

static void reset(void) {
    values.top = -1;
    ops.top = -1;
    postfix[0] = '\0';
    error_msg[0] = '\0';
    step_no = 0;
    pushes = pops = operands = operators = 0;
}

static int fail(const char *message) {
    snprintf(error_msg, sizeof(error_msg), "%s", message);
    return 0;
}

static int push_value(double x) {
    if (values.top >= MAX - 1) return fail("Operand stack overflow");
    values.data[++values.top] = x;
    pushes++;
    return 1;
}

static int pop_value(double *x) {
    if (values.top < 0) return fail("Operand stack underflow");
    *x = values.data[values.top--];
    pops++;
    return 1;
}

static int push_operator(char c) {
    if (ops.top >= MAX - 1) return fail("Operator stack overflow");
    ops.data[++ops.top] = c;
    pushes++;
    return 1;
}

static int pop_operator(char *c) {
    if (ops.top < 0) return fail("Operator stack underflow");
    *c = ops.data[ops.top--];
    pops++;
    return 1;
}

static int precedence(char op) {
    switch (op) {
        case '~': return 4;
        case '^': return 3;
        case '*': case '/': case '%': return 2;
        case '+': case '-': return 1;
        default: return 0;
    }
}

static int right_associative(char op) {
    return op == '^' || op == '~';
}

static int is_op(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^';
}

static int append_postfix(const char *s) {
    if (strlen(postfix) + strlen(s) + 2 >= OUT) return fail("Postfix output too long");
    strcat(postfix, s);
    strcat(postfix, " ");
    return 1;
}

static void stack_text(char *v, size_t vs, char *o, size_t os) {
    size_t used = 0;
    int i;
    used += (size_t)snprintf(v + used, vs - used, "[");
    for (i = 0; i <= values.top && used < vs; i++) {
        used += (size_t)snprintf(v + used, vs - used, "%s%.6g", i ? ", " : "", values.data[i]);
    }
    if (used < vs) snprintf(v + used, vs - used, "]");

    used = 0;
    used += (size_t)snprintf(o + used, os - used, "[");
    for (i = 0; i <= ops.top && used < os; i++) {
        used += (size_t)snprintf(o + used, os - used, "%s%c", i ? ", " : "", ops.data[i] == '~' ? 'u' : ops.data[i]);
    }
    if (used < os) snprintf(o + used, os - used, "]");
}

static void trace(const char *token, const char *action) {
    char v[1000], o[1000];
    if (!detailed) return;
    stack_text(v, sizeof(v), o, sizeof(o));
    printf("%-4d %-12.12s %-46.46s %-24.24s %s\n", ++step_no, token, action, o, v);
}

static int reduce_top(void) {
    char op, action[180], tok[4];
    double a, b, result;
    char num[64];

    if (!pop_operator(&op)) return 0;
    tok[0] = op == '~' ? 'u' : op;
    tok[1] = '\0';

    if (op == '~') {
        if (!pop_value(&b)) return 0;
        snprintf(action, sizeof(action), "POP unary minus; POP %.6g", b);
        trace(tok, action);
        result = -b;
        if (!push_value(result)) return 0;
        snprintf(action, sizeof(action), "PUSH %.6g (unary minus)", result);
        trace(tok, action);
        return append_postfix("u-");
    }

    if (values.top < 1) return fail("Operand expected");
    if (!pop_value(&b) || !pop_value(&a)) return 0;

    snprintf(action, sizeof(action), "POP %c; POP right %.6g; POP left %.6g", op, b, a);
    trace(tok, action);

    switch (op) {
        case '+': result = a + b; break;
        case '-': result = a - b; break;
        case '*': result = a * b; break;
        case '/':
            if (b == 0) return fail("Division by zero");
            result = a / b;
            break;
        case '%':
            if (b == 0) return fail("Division/modulus by zero");
            if (trunc(a) != a || trunc(b) != b) return fail("Modulus requires integer operands");
            result = fmod(a, b);
            break;
        case '^': result = pow(a, b); break;
        default: return fail("Unknown operator");
    }

    if (!isfinite(result)) return fail("Invalid numeric operation");
    if (!push_value(result)) return 0;

    snprintf(action, sizeof(action), "COMPUTE %.6g %c %.6g = %.6g; PUSH %.6g", a, op, b, result, result);
    trace(tok, action);
    snprintf(num, sizeof(num), "%c", op);
    return append_postfix(num);
}

static void print_table(void) {
    puts("\nPRECEDENCE AND ASSOCIATIVITY TABLE");
    puts("----------------------------------------------");
    puts("Operator     Precedence     Associativity");
    puts("u-           4              Right (unary)");
    puts("^            3              Right");
    puts("* / %        2              Left");
    puts("+ -          1              Left");
    puts("( )          grouping       N/A");
    puts("----------------------------------------------");
}

static int parse(const char *expr) {
    size_t i = 0;
    int expect_value = 1, had_token = 0;
    char action[180], token[80];

    reset();
    if (detailed) print_table();
    if (detailed) {
        puts("\nPARSING TRACE (u means unary minus in operator stack)");
        printf("%-4s %-12s %-46s %-24s %s\n", "Step", "Token", "Push / Pop / Reduction action", "Operator stack", "Operand stack");
        puts("-----------------------------------------------------------------------------------------------------------------------");
    }

    while (expr[i] && expr[i] != '\n') {
        char c = expr[i];
        if (isspace((unsigned char)c)) {
            i++;
            continue;
        }

        had_token = 1;

        if (isdigit((unsigned char)c) || c == '.') {
            char *end;
            double value;
            size_t n;

            if (!expect_value) return fail("Missing operator before number");
            value = strtod(expr + i, &end);
            if (end == expr + i || !isfinite(value)) return fail("Invalid number");

            n = (size_t)(end - (expr + i));
            if (n >= sizeof(token)) return fail("Numeric token too long");

            memcpy(token, expr + i, n);
            token[n] = '\0';

            if (!push_value(value) || !append_postfix(token)) return 0;
            operands++;

            snprintf(action, sizeof(action), "PUSH operand %.6g", value);
            trace(token, action);
            i += n;
            expect_value = 0;
            continue;
        }

        if (c == '(') {
            if (!expect_value) return fail("Missing operator before '('");
            if (!push_operator(c)) return 0;
            trace("(", "PUSH '(' onto operator stack");
            i++;
            continue;
        }

        if (c == ')') {
            char ignored;
            if (expect_value) return fail("Missing operand before ')'");
            while (ops.top >= 0 && ops.data[ops.top] != '(') {
                if (!reduce_top()) return 0;
            }
            if (ops.top < 0) return fail("Unmatched closing parenthesis");
            if (!pop_operator(&ignored)) return 0;
            trace(")", "POP matching '('");
            i++;
            expect_value = 0;
            continue;
        }

        if (is_op(c)) {
            char incoming = c;
            token[0] = c;
            token[1] = '\0';

            if (expect_value) {
                if (c == '-') {
                    incoming = '~';
                    strcpy(token, "u-");
                } else if (c == '+') {
                    trace("u+", "Ignore unary plus");
                    i++;
                    continue;
                } else {
                    return fail("Operand expected");
                }
            }

            if (incoming != '~') {
                while (ops.top >= 0 && ops.data[ops.top] != '(' &&
                       (precedence(ops.data[ops.top]) > precedence(incoming) ||
                       (precedence(ops.data[ops.top]) == precedence(incoming) && !right_associative(incoming)))) {
                    if (!reduce_top()) return 0;
                }
            }

            if (!push_operator(incoming)) return 0;
            operators++;

            snprintf(action, sizeof(action), "PUSH operator %s (precedence %d)", token, precedence(incoming));
            trace(token, action);
            i++;
            expect_value = 1;
            continue;
        }

        return fail("Invalid character token");
    }

    if (!had_token) return fail("Empty expression");
    if (expect_value) return fail("Operand expected: incomplete expression");

    while (ops.top >= 0) {
        if (ops.data[ops.top] == '(') return fail("Unmatched opening parenthesis");
        if (!reduce_top()) return 0;
    }

    if (values.top != 0) return fail("Invalid final operand stack");
    return 1;
}

typedef struct {
    const char *expr;
    int should_pass;
    double expected;
    const char *error_fragment;
} TestCase;

static int self_test(void) {
    static const TestCase tests[] = {
        {"3+4*2", 1, 11, NULL}, {"(3+4)*2", 1, 14, NULL}, {"2^3^2", 1, 512, NULL},
        {"10%4+6", 1, 8, NULL}, {"10-2-3", 1, 5, NULL}, {"2*3+4^2", 1, 22, NULL},
        {"3.5+2.5*2", 1, 8.5, NULL}, {"10.5/2.5", 1, 4.2, NULL}, {"-5+3", 1, -2, NULL},
        {"-(3+4)", 1, -7, NULL}, {"2*-3", 1, -6, NULL}, {"((2+3)*(4-1))^2", 1, 225, NULL},
        {"((5))", 1, 5, NULL}, {"10+2*6/3-4", 1, 10, NULL}, {"2^2^3", 1, 256, NULL},
        {"8/0", 0, 0, "zero"}, {"(3+4", 0, 0, "parenthesis"},
        {"3+*4", 0, 0, "Operand expected"}, {"5$3", 0, 0, "Invalid character"},
        {"10%2.5", 0, 0, "integer operands"}, {"2^", 0, 0, "Operand expected"},
        {"", 0, 0, "Empty expression"}
    };
    int i, pass = 0, total = (int)(sizeof(tests) / sizeof(tests[0]));

    detailed = 0;
    puts("AUTOMATED TEST SUITE - OPERATOR PRECEDENCE PARSER");
    puts("No. | Input Expression          | Expected      | Actual / Error                  | Status");
    puts("--------------------------------------------------------------------------------------------");

    for (i = 0; i < total; i++) {
        int ok = parse(tests[i].expr);
        int correct = tests[i].should_pass
            ? (ok && fabs(values.data[0] - tests[i].expected) < 1e-8)
            : (!ok && (!tests[i].error_fragment || strstr(error_msg, tests[i].error_fragment) != NULL));
        char actual[100], expected[40];

        if (ok) snprintf(actual, sizeof(actual), "%.10g", values.data[0]);
        else snprintf(actual, sizeof(actual), "ERROR: %.78s", error_msg);

        if (tests[i].should_pass) snprintf(expected, sizeof(expected), "%.10g", tests[i].expected);
        else snprintf(expected, sizeof(expected), "ERROR");

        printf("%3d | %-25.25s | %-13s | %-31.31s | %s\n",
               i + 1, tests[i].expr[0] ? tests[i].expr : "<empty>", expected, actual, correct ? "PASS" : "FAIL");
        if (correct) pass++;
    }

    printf("\nSUMMARY: %d/%d tests passed\n", pass, total);
    return pass == total ? 0 : 1;
}

int main(int argc, char **argv) {
    char input[2048];
    int ok;
    clock_t start, end;
    size_t input_len;
    int k, first_arg = 1;

    if (argc == 2 && strcmp(argv[1], "--self-test") == 0) return self_test();

    if (argc > 1 && strcmp(argv[1], "--fast") == 0) {
        detailed = 0;
        first_arg = 2;
    }

    if (argc == first_arg && !detailed) {
        puts("Usage: parser.exe --fast EXPR");
        return 1;
    }

    if (argc > first_arg) {
        size_t n = 0;
        input[0] = '\0';
        for (k = first_arg; k < argc; k++) {
            size_t len = strlen(argv[k]);
            if (n + len + 2 >= sizeof(input)) {
                puts("Input too long");
                return 1;
            }
            if (k > first_arg) {
                strcat(input, " ");
                n++;
            }
            strcat(input, argv[k]);
            n += len;
        }
    } else {
        puts("OPERATOR-PRECEDENCE PARSER AND EVALUATOR | BCSE307 M21");
        printf("Enter arithmetic expression: ");
        if (!fgets(input, sizeof(input), stdin)) return 1;
        if (!strchr(input, '\n') && !feof(stdin)) {
            puts("ERROR: Input exceeds 2047 characters");
            return 1;
        }
    }

    input_len = strlen(input);
    if (input_len && input[input_len - 1] == '\n') input_len--;

    printf("\nINPUT LENGTH: %zu characters | MODE: %s\n", input_len, detailed ? "DETAILED" : "FAST");

    start = clock();
    ok = parse(input);
    end = clock();

    if (!ok) {
        printf("\nERROR: %s\n", error_msg);
        return 1;
    }

    printf("\nFINAL OPERATOR STACK: []\nFINAL OPERAND STACK: [%.10g]\n", values.data[0]);
    printf("POSTFIX: %s\nRESULT: %.10g\n", postfix, values.data[0]);
    printf("\nEXECUTION SUMMARY\nInput characters: %zu\nOperands: %d\nOperators: %d\nPush operations: %d\nPop operations: %d\nCPU time (parsing%s): %.6f seconds\n",
           input_len, operands, operators, pushes, pops,
           detailed ? " + printing trace" : "",
           (double)(end - start) / CLOCKS_PER_SEC);

    return 0;
}