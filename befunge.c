#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define H 25  /* board height */
#define W 80  /* board width */

enum Direction { RIGHT = 0, DOWN = 1, LEFT = 2, UP = 3 };

static unsigned char board[H][W];
static signed int *stack;
static size_t sp, stack_size;
static int x, y, dir;

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

static int peek()
{
	return sp > 0 ? stack[sp - 1] : 0;
}

static void pop(size_t cnt, ...)
{
	va_list args;
	size_t n;

	va_start(args, cnt);
	while (cnt > sp) {
		*va_arg(args, int*) = 0;
		--cnt;
	}
	for (n = cnt; n > 0; --n) {
		*va_arg(args, int*) = stack[sp - n];
	}
	sp -= cnt;
	va_end(args);
}

static void push(int value)
{
	if (sp == stack_size) {	
		stack_size = stack_size ? 2*stack_size : 64;
		stack = realloc(stack, stack_size*sizeof *stack);
		if (!stack) fatal("stack reallocation failed");
	}
	stack[sp++] = value;
}

/* COMPAT: does not report failure if the file contains characters outside
           range of the board, but silently discards them. */
static bool load_program(const char *path)
{
	FILE *fp = fopen(path, "rt");
	int x = 0, y = 0;
	char ch;

	if (fp == NULL) return false;
	while ((ch = fgetc(fp)) != EOF) {
		if (ch == '\n') {
			x = 0;
			if (++y == H) break;
		} else {
			if (x < W) board[y][x++] = ch;
		}
	}
	fclose(fp);
	return true;
}

static void step()
{
	switch (dir) {
	case RIGHT: x = x < W - 1 ? x + 1 : 0; break;
	case DOWN:  y = y < H - 1 ? y + 1 : 0; break;
	case LEFT:  x = x > 0 ? x - 1 : W - 1; break;
	case UP:    y = y > 0 ? y - 1 : H - 1; break;
	}
}

static bool in_range(int x, int y)
{
	return x >= 0 && x < W && y >= 0 && y < H;
}

static int get(int x, int y)
{
	return in_range(x, y) ? board[y][x] : ' ';
}

static void put(int v, int x, int y)
{
	if (in_range(x, y)) board[y][x] = (char)v;
}

static int read_int()
{
	int res;
	if (scanf("%d", &res) != 1) fatal("failed to read integer");
	return res;
}

static int read_char()
{
	int res = getchar();
	if (res == EOF) fatal("failed to read integer.\n");
	return res;
}

static void zero_check(int arg)
{
	if (arg == 0) fatal("division by zero");
}

static void stringmode()
{
	step();
	while (board[y][x] != '"') {
		push((int)board[y][x]);
		step();
	}
}

static void run()
{
	int a, b, c;
	for (;;) {
		switch (board[y][x]) {
		case '+': pop(2, &a, &b); push(a + b); break;
		case '-': pop(2, &a, &b); push(a - b); break;
		case '*': pop(2, &a, &b); push(a * b); break;
		case '/': pop(2, &a, &b); zero_check(b); push(a/b); break;
		case '%': pop(2, &a, &b); zero_check(b); push(a%b); break;
		case '!': pop(1, &a); push(a); break;
		case '`': pop(2, &a, &b); push(a > b); break;
		case '>': dir = RIGHT; break;
		case '<': dir = LEFT;break;
		case '^': dir = UP; break;
		case 'v': dir = DOWN; break;
		case '?': dir = rand()%4; break;
		case '_': pop(1, &a); dir = a ? LEFT : RIGHT; break;
		case '|': pop(1, &a); dir = a ? UP : DOWN; break;
		case '"': stringmode(); break;
		case ':': push(peek()); break;
		case '\\': pop(2, &a, &b); push(b); push(a); break;
		case '$': pop(1, &a); break;
		case '.': pop(1, &a); printf("%d ", a); break;
		case ',': pop(1, &a); putchar((char)a); break;
		case '#': step(); break;
		case 'g': pop(2, &a, &b); push(get(a, b)); break;
		case 'p': pop(3, &a, &b, &c); put(a, b, c); break;
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
		case '@': exit(EXIT_SUCCESS);
		}
		step();
	}
}

int main(int argc, char *argv[])
{
	memset(board, ' ', sizeof(board));
	if (argc != 2) {
		printf("Usage: befunge <program.bf>\n");
		return EXIT_SUCCESS;
	}
	if (!load_program(argv[1])) {
		fatal("failed to load program from '%s'", argv[1]);
	}
	run();
	return EXIT_SUCCESS;
}
