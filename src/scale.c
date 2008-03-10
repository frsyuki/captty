#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>

struct context_t {
	const char* chars;
	int len;
	int pos;
};

void init_context(struct context_t* ctx, const char* chars)
{
	ctx->chars = chars;
	ctx->len = strlen(chars);
	ctx->pos = 0;
}

void print_char(struct context_t* ctx)
{
	putc(ctx->chars[ctx->pos++], stdout);
	if( ctx->pos >= ctx->len ) { ctx->pos = 0; }
}

int main(int argc, char* argv[])
{
	struct winsize ws;
	int c, r;

	if( ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) < 0 ) {
		perror("ioctl");
		return 1;
	}

	struct context_t ctx[4];
	init_context(&ctx[0], "partty! ");
	init_context(&ctx[1], "partty! ");
	init_context(&ctx[2], "partty! ");
	init_context(&ctx[3], "partty! ");

	for(c = 0; c < ws.ws_col; ++c) {
		print_char(&ctx[0]);
	}
	printf("\n");

	for(r = 1; r < ws.ws_row - 2; ++r) {
		print_char(&ctx[1]);
		for(c = 1; c < ws.ws_col - 1; ++c) {
			printf(" ");
		}
		print_char(&ctx[2]);
		printf("\n");
	}

	for(c = 0; c < ws.ws_col; ++c) {
		print_char(&ctx[3]);
	}
	printf("\n");

	return 0;
}
