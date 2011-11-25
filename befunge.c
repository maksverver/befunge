#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define H 25  /* board height */
#define W 80  /* board width */

enum Direction { RIGHT = 0, DOWN = 1, LEFT = 2, UP = 3 };

static unsigned char board[H][W];
static signed int *stack;
static size_t sp, stack_size;
static int x, y, dir;

/* Prints a fatal error message and then exits the program. */
static void fatal(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	fprintf(stderr, "Fatal error: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "!\n");
	va_end(args);
	exit(EXIT_FAILURE);
}

/* Returns the value on top of stack (or 0) but does not remove it. */
static int peek()
{
	return sp > 0 ? stack[sp - 1] : 0;
}

/* The following three functions pop one, two or three items off the stack and
   assign them to the values pointed to by the arguments from right to left.
   If the stack does not contain enough values, then zeros are assigned to the
   remaining arguments.

   The definitions of pop2() and pop3() may look redundant, but are written to
   take advantage of inlining compiler optimizations. */

static void pop1(int *a)
{
	if (sp >= 1) *a = stack[--sp]; else *a = 0;
}

static void pop2(int *a, int *b)
{
	if (sp >= 2) pop1(b), *a = stack[--sp]; else pop1(b), *a = 0;
}

static void pop3(int *a, int *b, int *c)
{
	if (sp >= 3) pop2(b, c), *a = stack[--sp]; else pop2(b, c), *a = 0;
}

/* Reserves space for `required' additional items on the stack.  The stack is
   reallocated if necessary.  A fatal error occurs if allocation fails. */
static void reserve(size_t required)
{
	required += sp;
	if (stack_size < required) {
		stack_size += stack_size;
		if (stack_size < required) stack_size = required;
		stack = realloc(stack, stack_size*sizeof *stack);
		if (!stack) fatal("stack reallocation failed");
	}
}

/* Pushes a value on the stack WITHOUT reserve space first.  This UNSAFE unless
   it is known that there is free space in the stack array (for example,
   immediately after some values have been popped).  */
static void push_bare(int value)
{
	stack[sp++] = value;
}

/* Pushes a value onto the stack. */
static void push(int value)
{
	reserve(1);
	push_bare(value);
}

/* Loads a program from a file onto the board.
   COMPAT: silently discards characters outside the board. */
static void load_program(FILE *fp)
{
	int x = 0, y = 0;
	char ch;

	while ((ch = getc(fp)) != EOF) {
		if (ch == '\n') {
			x = 0;
			if (y < H) ++y;
		} else {
			if (x < W && y < H) board[y][x++] = ch;
		}
	}
}

/* Moves the instruction pointer by one step. */
static void step()
{
	switch (dir) {
	case RIGHT: x = x < W - 1 ? x + 1 : 0; break;
	case DOWN:  y = y < H - 1 ? y + 1 : 0; break;
	case LEFT:  x = x > 0 ? x - 1 : W - 1; break;
	case UP:    y = y > 0 ? y - 1 : H - 1; break;
	}
}

/* Returns whether the given coordinates are in range of the board. */
static int in_range(int x, int y)
{
	return x >= 0 && x < W && y >= 0 && y < H;
}

/* Returns value on the board at (x,y) (or space if not in range). */
static int get(int x, int y)
{
	return in_range(x, y) ? board[y][x] : ' ';
}

/* Writes `v' to the board at (x,y) if in range. */
static void put(int v, int x, int y)
{
	if (in_range(x, y)) board[y][x] = (char)v;
}

/* Reverses the direction of the instruction pointer. */
static void reflect()
{
	dir ^= 2;
}

/* Executes the readint command: discards characters until reading a digit, then
   reads digits until overflow/EOF, and pushes the result.  If no digits occur
   before EOF, it reflects instead. */
static void readint()
{
	char ch;
	int i;

	while (!isdigit(ch = getc(stdin))) {
		if (ch == EOF) {
			reflect();
			return;
		}
	}
	i = ch - '0';
	while (isdigit(ch = getc(stdin))) {
		if (INT_MAX/i < 10 || INT_MAX - 10*i < (ch - '0')) break;
		i = 10*i + (ch - '0');
	}
	if (ch != EOF) ungetc(ch, stdin);
	push(i);
}

/* Executes the readchar command: reads a character and pushes it, or reflects
   instead if no character can be read. */
static void readchar()
{
	int ch = getc(stdin);
	if (ch != EOF) push(ch); else reflect();
}

/* Executes stringmode: steps over the board until a double-quote character is
   found, pushing all visited characters onto the stack in the process. */
static void stringmode()
{
	step();
	while (board[y][x] != '"') {
		push((int)board[y][x]);
		step();
	}
}

/* Executes the main interpreter loop. */
static void run()
{
	reserve(64);  /* MUST reserve some stack space (at least 2 items) */
	for (;;) {
		int a, b, c;  /* arguments; popped from the stack */

		switch (board[y][x]) {
		case '+': pop2(&a, &b); push_bare(a + b); break;
		case '-': pop2(&a, &b); push_bare(a - b); break;
		case '*': pop2(&a, &b); push_bare(a * b); break;
		case '/': pop2(&a, &b);
		          if (b) push_bare(a/b); else readint(); break;
		case '%': pop2(&a, &b);
		          if (b) push_bare(a%b); else readint(); break;
		case '!': pop1(&a); push_bare(!a); break;
		case '`': pop2(&a, &b); push_bare(a > b); break;
		case '>': dir = RIGHT; break;
		case '<': dir = LEFT;break;
		case '^': dir = UP; break;
		case 'v': dir = DOWN; break;
		case '?': dir = rand()/(RAND_MAX/4 + 1); break;
		case '_': pop1(&a); dir = a ? LEFT : RIGHT; break;
		case '|': pop1(&a); dir = a ? UP : DOWN; break;
		case '"': stringmode(); break;
		case ':': push(peek()); break;
		case '\\': pop2(&a, &b); push_bare(b); push_bare(a); break;
		case '$': pop1(&a); break;
		case '.': pop1(&a); printf("%d ", a); break;
		case ',': pop1(&a); putchar((char)a); break;
		case '#': step(); break;
		case 'g': pop2(&a, &b); push_bare(get(a, b)); break;
		case 'p': pop3(&a, &b, &c); put(a, b, c); break;
		case '&': readint(); break;
		case '~': readchar(); break;
		case '0': push(0); break;
		case '1': push(1); break;
		case '2': push(2); break;
		case '3': push(3); break;
		case '4': push(4); break;
		case '5': push(5); break;
		case '6': push(6); break;
		case '7': push(7); break;
		case '8': push(8); break;
		case '9': push(9); break;
		case '@': return;
		}
		step();
	}
}

/* Application entry point. */
int main(int argc, char *argv[])
{
	srand(time(NULL) + 31337*getpid());  /* seed RNG */
	memset(board, ' ', sizeof(board));   /* initialize board with spaces */
	setvbuf(stdout, NULL, _IONBF, 0);    /* make output unbuffered */
	if (argc != 2) {
		printf("Usage: befunge <program.bf>\n");
		return EXIT_SUCCESS;
	}
	if (strcmp(argv[1], "-") == 0) {
		load_program(stdin);
	} else {
		FILE *fp = fopen(argv[1], "rt");
		if (!fp) fatal("could not open program file '%s'", argv[1]);
		load_program(fp);
		fclose(fp);
	}
	run();
	return EXIT_SUCCESS;
}
