#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"

// If we are compiling on windows, compile these functions
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

// Fake readline function
char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* cpy = malloc(strlen(buffer) + 1);
    srcpy(cpy, buffer);
    cpy[strlen(cpy) - 1] = '\0';
    return cpy;
}

// Fake add_history function
void add_history(char* unused) {}

// Otherwise include the editline headers
#else
#include <editline/history.h>
#include <editline/readline.h>
#endif

typedef struct {
    int type;
    long num;
    int err;
} lval;

enum { LVAL_NUM, LVAL_ERR };

enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;

    return v;
}

lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;

    return v;
}

long power(long base, long exp) {
    long result = 1;
    while (exp > 0) {
        result *= base;
        exp--;
    }
    return result;
}

lval eval_op(lval x, char* op, lval y) {
    if (x.type == LVAL_ERR) return x;
    if (y.type == LVAL_ERR) return y;

    if (strcmp(op, "+") == 0) {
        return lval_num(x.num + y.num);
    }

    if (strcmp(op, "-") == 0) {
        return lval_num(x.num - y.num);
    }

    if (strcmp(op, "*") == 0) {
        return lval_num(x.num * y.num);
    }

    if (strcmp(op, "/") == 0) {
        return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
    }

    if (strcmp(op, "%") == 0) {
        return lval_num(x.num % y.num);
    }

    if (strcmp(op, "^") == 0) {
        return lval_num(power(x.num, y.num));
    }

    if (strcmp(op, "min") == 0) {
        return (x.num < y.num) ? lval_num(x.num) : lval_num(y.num);
    }

    if (strcmp(op, "max") == 0) {
        return (x.num > y.num) ? lval_num(x.num) : lval_num(y.num);
    }

    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
    // if tagged as number, return it directly
    if (strstr(t->tag, "number")) {
        // printf("%f\n", strtof(t->contents, NULL));
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    // node is an expr
    // first child is always '(', second child is the op
    char* op = t->children[1]->contents;

    lval x = eval(t->children[2]);

    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

void lval_print(lval v) {
    switch (v.type) {
        case LVAL_NUM:
            printf("%li", v.num);
            break;
        case LVAL_ERR:
            if (v.err == LERR_BAD_NUM) {
                printf("Error: Invalid Number!");
            }

            if (v.err == LERR_DIV_ZERO) {
                printf("Error: Division By Zero!");
            }

            if (v.err == LERR_BAD_OP) {
                printf("Error: Invalid Operator!");
            }
            break;
    }
}

/* Print an "lval" followed by a newline */
void lval_println(lval v) {
    lval_print(v);
    putchar('\n');
}

int main(int argc, char** argv) {
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Keyword = mpc_new("keyword");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Lispy = mpc_new("lispy");

    mpca_lang(MPCA_LANG_DEFAULT,
              "\
			number: /-?[0-9]+(\\.?[0-9]+)?/ ; \
			keyword: /(min|max)/ ; \
			operator: '+' | '-' | '*' | '/' | '%' | '^' | <keyword> ; \
			expr: <number> | '(' <operator> <expr>+ ')' ; \
			lispy: /^/ <operator> <expr>+ /$/ ; \
		",
              Number, Keyword, Operator, Expr, Lispy);

    puts("Lispy Version 0.0.1");
    puts("Press Ctrl+c to exit\n");

    while (1) {
        char* input = readline("lispy> ");

        add_history(input);

        // printf("No you're a %s\n", input);
        mpc_result_t r;

        if (mpc_parse("<stdin>", input, Lispy, &r)) {
            lval result = eval(r.output);
            // printf("%li\n", result);
            lval_println(result);
            mpc_ast_delete(r.output);
        } else {
            puts("Parser Error");
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        // Free received input
        free(input);
    }

    mpc_cleanup(4, Number, Operator, Expr, Lispy);

    return 0;
}