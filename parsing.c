#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

/* If compiling on windows */
#ifdef _WIN32
#include <string.h>

static char buffer[2048]

/* Fake readline function */
char* readline(char* prompt) {
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char* cpy = malloc(strlen(buffer)+1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy)-1] = '\0';
	return cpy;
}

/* Fake add_history function */
void add_history(char* unused) {}

/* Otherwise include the editline headers */
#else
#include <editline.h>
#endif

/* we are forward declaring this because lval_print and lval_expr_print both
 * call each other. since lval_expr_print is declared first, we need to make
 * sure it can use the later-called lval_print by declaring it the second time
 * up here, before lval_expr_print is called. now the functions can call each
 * other with no issue. */
void lval_print(lval* v);

/* create enumeration of possible lval types */
enum {
	LVAL_NUM,
	LVAL_ERR,
	LVAL_SYM,
	LVAL_SEXPR
};

/* create enumeration of possible error types */
/* no longer needed
enum {
	LERR_DIV_ZERO,
	LERR_BAD_OP,
	LERR_BAD_NUM
}; 
*/

/* declare new lval struct. lval stands for Lisp value.
 * a lisp value is just the possible result of any expression.
 * currently this can be a Number or an Error.
 * a struct is used to declare a new Type. all a struct
 * is is several variables bundled into one package.
 * we use the struct keyword preceded with the typedef
 * keyword. at the end, after the last bracket, type
 * the name of the struct with a semicolon. */

/* update: now that we are referencing lval in its own definition,
 * we need to change how it's defined a little bit. before we open
 * the first curly brackets, we can put hte name of the struct. this gives
 * us the ability to refer to the name inside the definition using
 * "struct lval". however, even though a struct can refer to its own type,
 * it must only contain pointers to its own type, not the type directly.
 * Otherwise, the size of the struct would refer to itself, and grow infinite
 * in size when you tried to calculate it! */
typedef struct lval{
	/* types are int so that we can easily understand what
	 * the encoded struct is; for example, if type is 0,
	 * then structure is a Number. or if type is 1, then
	 * structure is an Error. this is a simple and
	 * effective way to do this.*/
	int type;	
	long num;
	int err;
	/* error and symbol types have some string data. we're
	 * changing the representation of errors to a string.
	 * this mans we can store unique error messages
	 * rather than just a code. this makes reporting errors
	 * more flexible and better, and we can get rid of the
	 * original error enum. */
	char* err;
	char* sym;
	/* count and pointer to a list of "lval" */
	int count;
	/* s-expressions are LISTS of other values. as we learnt before, we 
	 * can't make variable length structs, so we are going to need to
	 * use a pointer. we will make a pointer field "cell" which points
	 * to a location where we store a list of lval*. more specifically,
	 * pointers to the other individual lval in the s-expression. therefore,
	 * our field should be a double pointer type lval** - a pointer to lval
	 * pointers. we'll need to keep track of the number of lval* in the list,
	 * so we add an extra field, "count", to record it. */
	struct lval** cell;
} lval;

/* create new number-type lval function */

/* we are now changing our lval construction functions to return pointers to a
 * lval, instead of returning one directly.*/
lval* lval_num(long x) {
	/* note how we assign the value "v" to a lval pointer now. We use malloc
	 * to allocate memory, in the size of the lval struct using sizeof. */
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

/* and make the error type function. */
lval* lval_err(char* m) {
	lval* = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(m)+1);
	strcpy(v->err, m);
	return v;
}

/* Construct a pointer to a new Symbol lval */
lval* lval_sym(char* s) {
	lval* = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s)+1);
	strcpy(v->sym, s);
	return v;
}

/* A pointer to a new empty Sexpr lval */
lval* lval_sexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL
	return v;
}

lval* lval_read_num(mpc_ast_t* t) {
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ?
		lval_num(x) : lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* t) {

	/* if symbol or number return conversion to that type */
	if (strstr(t->tag, "number")) {return lval_read_num(t); }
	if (strstr(t->tag, "symbol")) {return lval_sym(t->contents); }

	/* if root (>) or sexpr then create empty list */
	lval* x = NULL;
	if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
	if (strstr(t->tag, "sexpr")) { x = lval_sexpr(); }

	/* fill this list with any valid expression contained within */
	for (int i = 0; i < t->children_num; i++) {
		if (strcmp(t->children[1]->contents, "(") == 0) { continue; }
		if (strcmp(t->children[1]->contents, ")") == 0) { continue; }
		if (strcmp(t->children[1]->tag, "regex") == 0) { continue; }
		x = lval_add(x, lval_read(t->children[i]));
	}
	return x;
}

lval* lval_add(lval* v, lval* x) {
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count-1] = x;
	return v;
}

/* now we modify our print function to print out s-expressions types.
 * using this we can doublecheck that hte reading phase is working correctly by printing out the s-expressions we read in and verifying that they match htose we input. */
void lval_expr_print(lval* v, char open, char close) {
	putchar(open);
	for (int i = 0; i < v->count; i++) {
		/* print value contained within */
		lval_print(v->cell[i]);

		/* dont print trailing space if last element */
		if (i != (v->count-1)) {
			putchar(' ');
		}
	}
	putchar(close);
}

 
void lval_print(lval v) {
	/* because our output can now be more than 1
	 * thing, simply using printf to output
	 * will no longer suffice. since we need the
	 * program to behave differently depending on
	 * the lval-type, it now makes sense to use a
	 * switch-case statement. switch will take some
	 * value (in parentheses) as input and compare
	 * it to other known values, or cases. when
	 * the values are equal it executes the code that
	 * follows until the next break statement. */
	switch (v->type) {
		case LVAL_NUM:	 printf("%li", v->num); break;
		case LVAL_ERR:	 printf("error: %s", v->err); break;
		case LVAL_SYM:	 printf("%s", v->sym); break;
		case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); }

/* print lval followed by newline */
void lval_println(lval v) { lval_print(v); putchar('\n'); }

void lval_del(lval* v) {
	switch (v->type) {
		/* do nothing special for number type */
		case LVAL_NUM: break;
		/* for err or sym, free the string data */
		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;	
		/* if sexpr then recursively delete all elements (lvals) inside */
		case LVAL_SEXPR:
			for (int i = 0; i < v->count; i++) {
				lval_del(v->cell[i]);
			}
			/* also free the memory allocated to contain the pointers */
			free(v->cell);
			break;
	}
	/* free memory for the lval struct itself */
	free(v);
}

lval eval_op(lval x, char* op, lval y) {

	/* if either value is an error, return it */
	if (x.type == LVAL_ERR) {return x;}
	if (y.type == LVAL_ERR) {return y;}

	/* use symbol string to see which operation to perform */
	if (strcmp(op, "+") == 0) {return lval_num(x.num + y.num);}
	if (strcmp(op, "-") == 0) {return lval_num(x.num - y.num);}
	if (strcmp(op, "*") == 0) {return lval_num(x.num * y.num);}
	if (strcmp(op, "/") == 0) {
		return y.num == 0
			? lval_err(LERR_DIV_ZERO)
			: lval_num(x.num / y.num);
	}
	return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {

	if (strstr(t->tag,"number")) {
		/* check if there is some error in conversion */
		errno = 0;
		/* strtol converts strings to longs. this allows
		 * us to check special variable errno to ensure conversion
		 * goes correctly. this is a more robust way
		 * to convert numbers than our previous method
		 * using atoi */
		long x = strtol(t->contents, NULL, 10);
		return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
	}

	char* op = t->children[1]->contents;
	lval x = eval(t->children[2]);

	/* iterate the remaining children and combining */
	int i = 3;
	while (strstr(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}

	return x;
}

int main(int argc, char** argv) {

	/* create some parsers */
	mpc_parser_t* Number 	= mpc_new("number");
	mpc_parser_t* Symbol  	= mpc_new("symbol");
	mpc_parser_t* Sexpr 	= mpc_new("sexpr");
	mpc_parser_t* Expr 		= mpc_new("expr");
	mpc_parser_t* Lispy 	= mpc_new("lispy");

	/* Define them with the following language */
	mpca_lang(MPCA_LANG_DEFAULT,
  "                                                   \
    number   : /-?[0-9]+/ ;                           \
    symbol   : '+' | '-' | '*' | '/' ;                \
	sexpr    : '(' <expr>* ')' ;					  \
    expr     : <number> | '(' <symbol> <expr>+ ')' ;  \
    lispy    : /^/ <symbol> <expr>+ /$/ ;             \
  ", Number, Symbol, Sexpr, Expr, Lispy);


	/* Prints version and exit info */
	puts("Lispy version 0.000000000");
	puts("Ctrl+C to exit :)\n");

	while(1) {
		char* input = readline("lispy> ");
		add_history(input);	
	
		/* attempt to parse the user input */
		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			/* on success print the ast */
			lval* x = lval_read(r.output);
			lval_println(x);
			lval_del(x)

		} else {
			/* otherwise print the error */
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(input);
	}
	/* Undefine and delete our parsers */
	mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);
	return 0;
}
