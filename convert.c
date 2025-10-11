#include <stdio.h>
#include <stdlib.h>

static void convert_to_utf8(unsigned c)
{
  static const char *x[] = {
    "0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",
    "8",  "9",  "\n", "\b", " ",  "i",  "p",  "-",
    "+",  "|",  "#",  NULL, "A",  "B",  "C",  "D",
    "E",  "F",  "G",  "H",  "I",  "J",  "K",  "L",
    "M",  "N",  "O",  "P",  "Q",  "R",  "S",  "T",
    "U",  "V",  "W",  "X",  "Y",  "Z",  NULL, "→",
    "?",  "=",  "u",  ",",  ".",  "⊟",  "[",  "_",
    "\"", "„",  "<",  ">",  "]",  "ˣ",  ":",  "ʸ"
  };
  fputs(x[c], stdout);
}

static void convert_to_linc(const char *c)
{
  switch (*c) {
  case '0': return 000;
  case '1': return 001;
  case '2': return 002;
  case '3': return 003;
  case '4': return 004;
  case '5': return 005;
  case '6': return 006;
  case '7': return 007;
  case '8': return 010;
  case '\n': return 012;
  case '\b': return 013;
  case ' ': return 014;
  case 'i': return 015;
  case 'p': return 016;
  case '-': return 017;
  case '+': return 020;
  case '|': return 021;
  case '#': return 022;
  //CASE
  case 'A': return 024;
  case 'B': return 025;
  case 'C': return 026;
  case 'D': return 027;
  case 'E': return 030;
  case 'F': return 031;
  case 'G': return 032;
  case 'H': return 033;
  case 'I': return 034;
  case 'J': return 035;
  case 'K': return 036;
  case 'L': return 037;
  case 'M': return 040;
  case 'N': return 041;
  case 'O': return 042;
  case 'P': return 043;
  case 'Q': return 044;
  case 'R': return 045;
  case 'S': return 046;
  case 'T': return 047;
  case 'U': return 050;
  case 'V': return 051;
  case 'W': return 052;
  case 'X': return 053;
  case 'Y': return 054;
  case 'Z': return 055;
  //META           056
  //arrow          057
  case '?': return 060;
  case '=': return 061;
  case 'u': return 062;
  case ',': return 063;
  case '.': return 064;
  //⊟
  case '@': return 065;
  case '[': return 066;
  case '_': return 067;
  case '\"': return 070;
  //„              071
  case '<': return 072;
  case '>': return 073;
  case ']': return 074;
  case '*':
  case 'x': return 075;
  case ':': return 076;
  case 'y': return 077;
  }
}

static unsigned getword(void)
{
  int c1, c2;

  c1 = getchar();
  if (c1 == EOF)
    exit(0);
  c2 = getchar();
  if (c2 == EOF) {
    fprintf(stderr, "End of file inside word.\n");
    exit(1);
  }
  if (c2 & ~0xF) {
    fprintf(stderr, "Bad data in word.\n");
    return 1;
  }
  return (c1 & 0xFF) | (c2 << 4);
}

int main(void)
{
  unsigned word;

  for(;;) {
    word = getword();
    convert_to_utf8(word >> 6);
    convert_to_utf8(word & 077);
  }
}
