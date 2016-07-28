#include <stdio.h>
#include "option.h"

int optind;
static int optopt;
static char *next;

void opt_init()
{
	optind = 0;
	optopt = '\0';
	next = NULL;
}
