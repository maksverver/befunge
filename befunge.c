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

/* Pops the top `cnt' elements off the stack and assigns them to `args'.
   The bottommost value is assigned to arg[0], et cetera.  If the stack
   contains fewer than `cnt' items, the first few elements are set to 0. */
static void pop(size_t cnt, int args[])
{
	size_t n;

	while (cnt > sp) {
		*args++ = 0;
		--cnt;
	}
	for (n = cnt; n > 0; --n) {
		*args++ = stack[sp - n];
	}
	sp -= cnt;
}

/* Reserves space for `required' additional items on the stack.  The stack is
   reallocated if necessary. A fatal error occurs if allocation fails. */
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

/* Pushes a value on the stack. */
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

static int read_int()
{
	/* COMPAT: official specification says we should discard characters
	           until we find a digit, then read digits until overflow,
	           but this does not allow negative numbers to be input! */
	/* COMPAT: aborts rather than reverses direction at EOF */
	int res;
	if (scanf("%d", &res) != 1) fatal("failed to read integer");
	return res;
}

static int read_char()
{
	/* COMPAT: aborts rather than reverses direction at EOF */
	int res = getchar();
	if (res == EOF) fatal("EOF while reading character");
	return res;
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
	int a[3];  /* arguments; popped from the stack */

	reserve(64);  /* MUST reserve some stack space (at least 2 items) */
	for (;;) {
		switch (board[y][x]) {
		case '+': pop(2, a); push_bare(a[0] + a[1]); break;
		case '-': pop(2, a); push_bare(a[0] - a[1]); break;
		case '*': pop(2, a); push_bare(a[0] * a[1]); break;
		case '/': pop(2, a); push_bare(a[1] ? a[0]/a[1] : read_int()); break;
		case '%': pop(2, a); push_bare(a[1] ? a[0]%a[1] : read_int()); break;
		case '!': pop(1, a); push_bare(!a[0]); break;
		case '`': pop(2, a); push_bare(a[0] > a[1]); break;
		case '>': dir = RIGHT; break;
		case '<': dir = LEFT;break;
		case '^': dir = UP; break;
		case 'v': dir = DOWN; break;
		case '?': dir = rand()/(RAND_MAX/4 + 1); break;
		case '_': pop(1, a); dir = a[0] ? LEFT : RIGHT; break;
		case '|': pop(1, a); dir = a[0] ? UP : DOWN; break;
		case '"': stringmode(); break;
		case ':': push(peek()); break;
		case '\\': pop(2, a); push_bare(a[1]); push_bare(a[0]); break;
		case '$': pop(1, a); break;
		case '.': pop(1, a); printf("%d ", a[0]); break;
		case ',': pop(1, a); putchar((char)a[0]); break;
		case '#': step(); break;
		case 'g': pop(2, a); push_bare(get(a[0], a[1])); break;
		case 'p': pop(3, a); put(a[0], a[1], a[2]); break;
		case '&': push(read_int()); break;
		case '~': push(read_char()); break;
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
