#include <stdio.h>
#include <stdlib.h>

/* Format a LAP6 manuscript using tabs for indentation. */

static int getch(void)
{
  int c = getchar();
  if (c == EOF)
    exit(0);
  return c;
}

static void skip_space(void)
{
  int c = getch();
  if (c != ' ')
    putchar(c);
}

static void tag(void)
{
  putchar('#');
  putchar(getch());
  putchar(getch());
  putchar('\t');
  skip_space();
}

static void maybe_tab(void)
{
  int c = getch();
  if (c == '#')
    tag();
  else {
    putchar('\t');
    putchar(c);
  }
}

static void newline(void)
{
  putchar('\n');
  maybe_tab();
}

int main(void)
{
  int c;

  maybe_tab();

  for (;;) {
    c = getch();
    if (c == '\n')
      newline();
    else
      putchar(c);
  }

  return 0;
}
