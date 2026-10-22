#include <stdio.h>

typedef unsigned short word_t;
typedef word_t (*second_fn_t)(FILE *, word_t, word_t, word_t);

#define UNUSED(X) (void)X

/*
7777 0000 HLT
7777 0005 ZTA
7777 0010 ENI
7777 0011 CLR
7777 0012 DIN
7777 0014 ATR
7777 0015 RTA
7777 0016 NOP
7777 0017 COM
7740 0000 MSC %b
7740 0040 SET %i %b p+1
7740 0100 SAM %i %b
7740 0140 DIS %i %b
7740 0200 XSK %i %b
7740 0240 ROL %i %b
7740 0300 ROR %i %b
7740 0340 SCR %i %b

7757 0414 SKP %i 14
7757 0415 KST %i
7757 0416 SKP %i 16
7757 0417 SKP %i 17
7757 0446 PIN %i
7757 0447 SKP %i 47
7757 0450 AZE %i
7757 0451 APO %i
7757 0452 LZE %i
7757 0453 IBZ %i
7757 0454 OVF %i
7757 0455 ZZZ %i
7740 0400 SXL %i %b
7750 0440 SNS %i %b
7750 0450 SKP %i %b

7757 0515 KBD %i
7757 0516 RSW %i
7757 0517 LSW %i
7700 0500 OPR %i %b

7707 0700 RDC %i %u
7707 0701 RCG %i %u
7707 0702 RDE %i %u
7707 0703 MTB %i %u
7707 0704 WRC %i %u
7707 0705 WCG %i %u
7707 0706 WRI %i %u
7707 0707 CHK %i %u

7740 1000 LDA %i %b p+1
7740 1040 STA %i %b p+1
7740 1100 ADA %i %b p+1
7740 1140 ADM %i %b p+1
7740 1200 LAM %i %b p+1
7740 1240 MUL %i %b p+1
7740 1300 LDH %i %b p+1
7740 1340 STH %i %b p+1
7740 1400 SHD %i %b p+1
7740 1440 SAE %i %b p+1
7740 1500 SRO %i %b p+1
7740 1540 BCL %i %b p+1
7740 1600 BSE %i %b p+1
7740 1640 BCM %i %b p+1
7740 1740 DSC %i %b p+1
6000 2000 ADD %a
6000 4000 STC %a
6000 6000 JMP %a
*/

static void keyboard_pair(FILE *f, word_t code)
{
  static const char *x[] = {
    "0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",
    "8",  "9",  " ",  " ",  " ",  "i",  "p",  "-",
    "+",  "|",  "#",  " ",  "A",  "B",  "C",  "D",
    "E",  "F",  "G",  "H",  "I",  "J",  "K",  "L",
    "M",  "N",  "O",  "P",  "Q",  "R",  "S",  "T",
    "U",  "V",  "W",  "X",  "Y",  "Z",  " ",  "→",
    "?",  "=",  "u",  ",",  ".",  "⊟",  "[",  "_",
    "\"", "„",  "<",  ">",  "]",  "x",  ":",  "y"
  };
  code &= 07777;
  fputs(x[code >>  6], f);
  fputs(x[code & 077], f);
}

static void alpha(FILE *f, word_t code)
{
  fprintf(f, " %o", code);
}

static void beta(FILE *f, word_t code)
{
  if (code != 0)
    alpha(f, code);
}

static void X(FILE *f, word_t code)
{
  fprintf(f, " %04o", code & 01777);
}

static void begin(FILE *f, word_t address, word_t insn)
{
  fprintf(f, "\n%04o  %04o ", address, insn);
  keyboard_pair(f, insn);
  fputs("   ", f);
}

static word_t second(FILE *f, word_t insn, word_t data, word_t address)
{
  UNUSED(insn);
  begin(f, address++, data);
  fprintf(f, " %04o", data);
  if (data >= 04000)
    fprintf(f, "  [-%o", data ^ 07777);
  return address;
}

static word_t indexing(FILE *f, word_t insn, word_t data, word_t address)
{
  if (insn & 017)
    return address;
  else
    return second(f, insn, data, address);
}

static void area(FILE *f, word_t block, word_t start, word_t end, const char *name)
{
  if (block < start || block >= end)
    return;
  fprintf(f, "  [ %o=%s+%o", block, name, block - start);
}

static word_t magtape(FILE *f, word_t insn, word_t data, word_t address)
{
  word_t i = insn & 020;
  word_t u = (insn & 010) >> 3;
  word_t q = (data & 07000) >> 9;
  word_t b = data & 0777;
  if (u)
    fprintf(f, " u");
  fprintf(f, "  [ %s u%o", i ? "move" : "stop", (int)u);
  begin(f, address++, data);
  fprintf(f, " %o|%03o", q, b);
  area(f, b, 0300, 00317, "LAP6");
  area(f, b, 0317, 00320, "FREE");
  area(f, b, 0320, 00340, "LAP6");
  area(f, b, 0340, 00350, "BIN");
  area(f, b, 0350, 00426, "WA");
  area(f, b, 0426, 00430, "INDEX");
  area(f, b, 0430, 01000, "FILE");
  area(f, b, 0000, 00270, "FILE");
  return address;
}

struct something {
  word_t insn_mask, insn_code;  const char *insn_name;
  word_t i_mask; const char *i_name;
  word_t beta_mask; void (*beta_fn)(FILE *, word_t);
  second_fn_t second_fn;
};

static struct something table[] = {
  { 07777, 00000, "HLT", 00000, NULL, 00000, NULL,  NULL },
  { 07777, 00005, "ZTA", 00000, NULL, 00000, NULL,  NULL },
  { 07777, 00010, "ENI", 00000, NULL, 00000, NULL,  NULL },
  { 07777, 00011, "CLR", 00000, NULL, 00000, NULL,  NULL },
  { 07777, 00012, "DIN", 00000, NULL, 00000, NULL,  NULL },
  { 07777, 00014, "ATR", 00000, NULL, 00000, NULL,  NULL },
  { 07777, 00015, "RTA", 00000, NULL, 00000, NULL,  NULL },
  { 07777, 00016, "NOP", 00000, NULL, 00000, NULL,  NULL },
  { 07777, 00017, "COM", 00000, NULL, 00000, NULL,  NULL },
  { 07740, 00000, "MSC", 00000, NULL, 00017, alpha, NULL },
  { 07740, 00040, "SET", 00020, "i",  00017, alpha, second },
  { 07740, 00100, "SAM", 00020, "i",  00017, alpha, NULL },
  { 07740, 00140, "DIS", 00020, "i",  00017, alpha, NULL },
  { 07740, 00200, "XSK", 00020, "i",  00017, alpha, NULL },
  { 07740, 00240, "ROL", 00020, "i",  00017, alpha, NULL },
  { 07740, 00300, "ROR", 00020, "i",  00017, alpha, NULL },
  { 07740, 00340, "SCR", 00020, "i",  00017, alpha, NULL },
  { 07757, 00415, "KST", 00020, "i",  00000, NULL,  NULL },
  { 07757, 00446, "PIN", 00020, "i",  00000, NULL,  NULL },
  { 07757, 00450, "AZE", 00020, "i",  00000, NULL,  NULL },
  { 07757, 00451, "APO", 00020, "i",  00000, NULL,  NULL },
  { 07757, 00452, "LZE", 00020, "i",  00000, NULL,  NULL },
  { 07757, 00453, "IBZ", 00020, "i",  00000, NULL,  NULL },
  { 07757, 00454, "OVF", 00020, "i",  00000, NULL,  NULL },
  { 07757, 00455, "ZZZ", 00020, "i",  00000, NULL,  NULL },
  { 07740, 00400, "SXL", 00020, "i",  00017, alpha, NULL },
  { 07750, 00440, "SNS", 00020, "i",  00017, alpha, NULL },
  { 07740, 00440, "SKP", 00020, "i",  00017, alpha, NULL },
//{ 07757, 00513, "PDP", 00020, "i",  00000, NULL,  NULL }, // LINC-8
//{ 07757, 00514, "TYP", 00020, "i",  00000, NULL,  NULL }, // LINC-8
  { 07757, 00515, "KBD", 00020, "i",  00000, NULL,  NULL },
  { 07757, 00516, "RSW", 00020, "i",  00000, NULL,  NULL },
  { 07757, 00517, "LSW", 00020, "i",  00000, NULL,  NULL },
  { 07700, 00500, "OPR", 00020, "i",  00017, alpha, NULL },
  { 07707, 00700, "RDC", 00020, "i",  00000, NULL,  magtape },
  { 07707, 00701, "RCG", 00020, "i",  00000, NULL,  magtape },
  { 07707, 00702, "RDE", 00020, "i",  00000, NULL,  magtape },
  { 07707, 00703, "MTB", 00020, "i",  00000, NULL,  magtape },
  { 07707, 00704, "WRC", 00020, "i",  00000, NULL,  magtape },
  { 07707, 00705, "WCG", 00020, "i",  00000, NULL,  magtape },
  { 07707, 00706, "WRI", 00020, "i",  00000, NULL,  magtape },
  { 07707, 00707, "CHK", 00020, "i",  00000, NULL,  magtape },
//{ 07777, 00740, "EXC", 00000, NULL, 00000, NULL,  NULL }, // LINC-8
  { 07740, 01000, "LDA", 00020, "i",  00017, beta,  indexing },
  { 07740, 01040, "STA", 00020, "i",  00017, beta,  indexing },
  { 07740, 01100, "ADA", 00020, "i",  00017, beta,  indexing },
  { 07740, 01140, "ADM", 00020, "i",  00017, beta,  indexing },
  { 07740, 01200, "LAM", 00020, "i",  00017, beta,  indexing },
  { 07740, 01240, "MUL", 00020, "i",  00017, beta,  indexing },
  { 07740, 01300, "LDH", 00020, "i",  00017, beta,  indexing },
  { 07740, 01340, "STH", 00020, "i",  00017, beta,  indexing },
  { 07740, 01400, "SHD", 00020, "i",  00017, beta,  indexing },
  { 07740, 01440, "SAE", 00020, "i",  00017, beta,  indexing },
  { 07740, 01500, "SRO", 00020, "i",  00017, beta,  indexing },
  { 07740, 01540, "BCL", 00020, "i",  00017, beta,  indexing },
  { 07740, 01600, "BSE", 00020, "i",  00017, beta,  indexing },
  { 07740, 01640, "BCO", 00020, "i",  00017, beta,  indexing },
  { 07740, 01740, "DSC", 00020, "i",  00017, beta,  indexing },
  { 06000, 02000, "ADD", 00000, NULL, 01777, X,     NULL },
  { 06000, 04000, "STC", 00000, NULL, 01777, X,     NULL },
  { 06000, 06000, "JMP", 00000, NULL, 01777, X,     NULL }
};

static second_fn_t mnemonic(FILE *f, word_t insn)
{
  unsigned i;
  for (i = 0; i < sizeof table / sizeof table[0]; i++) {
    if ((insn & table[i].insn_mask) != table[i].insn_code)
      continue;
    fprintf(f, "%s", table[i].insn_name);
    if ((insn & table[i].i_mask) != 0)
      fprintf(f, " %s", table[i].i_name);
    if (table[i].beta_mask)
      table[i].beta_fn(f, insn & table[i].beta_mask);
    return table[i].second_fn;
  }
  fprintf(f, "%04o", insn);
  return (second_fn_t)NULL;
}

word_t disassemble(FILE *f, word_t address, word_t *data)
{
  word_t insn = data[address];
  second_fn_t fn;
  begin(f, address++, insn);
  fn = mnemonic(f, insn);
  if (fn)
    address = fn(f, insn, data[address], address);
  return address;
}
