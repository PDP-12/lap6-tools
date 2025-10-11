#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>

#define TAPE_BLOCKS   512
#define BLOCK_WORDS   256
#define BLOCK_OCTETS  (2 * BLOCK_WORDS)
#define INDEX_BLOCKS  2
#define EMPTY         05757

typedef unsigned short word_t;
typedef void (*entry_fn)(const char *name, word_t *block, word_t *blocks);

static int directory_block = 0426;
static word_t directory[INDEX_BLOCKS * BLOCK_WORDS];
static word_t input[100 * BLOCK_WORDS];
static int input_blocks, X;
static int *input_block = &X;
static const char *arg_name = NULL;
static int forward_offset;
static FILE *tape;

static void fail(const char *message)
{
  fprintf(stderr, "ERROR: %s\n", message);
  exit(1);
}

static const char *convert_to_utf8(word_t c)
{
  static const char *x[] = {
    "0",  "1",  "2",  "3",  "4",  "5",  "6",  "7",
    "8",  "9",  "\n", "\b", " ",  "i",  "p",  "-",
    "+",  "|",  "#",  "(case)", "A",  "B",  "C",  "D",
    "E",  "F",  "G",  "H",  "I",  "J",  "K",  "L",
    "M",  "N",  "O",  "P",  "Q",  "R",  "S",  "T",
    "U",  "V",  "W",  "X",  "Y",  "Z",  "(meta)", "→",
    "?",  "=",  "u",  ",",  ".",  "⊟",  "[",  "_",
    "\"", "„",  "<",  ">",  "]",  "ˣ",  ":",  "ʸ"
  };
  return x[c];
}

static unsigned char convert_to_linc(char c)
{
  switch (c) {
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
  case 'A': case 'a': return 024;
  case 'B': case 'b': return 025;
  case 'C': case 'c': return 026;
  case 'D': case 'd': return 027;
  case 'E': case 'e': return 030;
  case 'F': case 'f': return 031;
  case 'G': case 'g': return 032;
  case 'H': case 'h': return 033;
  case 'I':           return 034;
  case 'J': case 'j': return 035;
  case 'K': case 'k': return 036;
  case 'L': case 'l': return 037;
  case 'M': case 'm': return 040;
  case 'N': case 'n': return 041;
  case 'O': case 'o': return 042;
  case 'P':           return 043;
  case 'Q': case 'q': return 044;
  case 'R': case 'r': return 045;
  case 'S': case 's': return 046;
  case 'T': case 't': return 047;
  case 'U':           return 050;
  case 'V': case 'v': return 051;
  case 'W': case 'w': return 052;
  case 'X':           return 053;
  case 'Y':           return 054;
  case 'Z': case 'z': return 055;
  //META           056
  //→              057
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
  default:
    fail("Bad data in input.");
    return 077;
  }
}

static int tape_seek(int block)
{
  long offset = BLOCK_OCTETS * (block - forward_offset);
  return fseek(tape, offset, SEEK_SET);
}

static unsigned short read_uint16(void)
{
  unsigned char data[2];
  unsigned short word;
  if (fread(data, 1, 2, tape) != 2)
    fail("Reading block.");
  word = data[1];
  word <<= 8;
  word |= data[0];
  return word;
}

static signed short read_int16(void)
{
  return (signed short)read_uint16();
}

static word_t read_word(void)
{
  word_t word = read_uint16();
  if ((word & 0xF000) != 0)
    fail("Bad data in block.");
  return word;
}

static void read_block(int block, word_t *data)
{
  int i;
  if (tape_seek(block) == -1)
    fail("Seeking to block.");
  for (i = 0; i < BLOCK_WORDS; i++)
    data[i] = read_word();
}

static void write_word(word_t word)
{
  unsigned char data[2];
  data[0] = word & 0xFF;
  data[1] = word >> 8;
  if (fwrite(data, 1, 2, tape) != 2)
    fail("Writing block.");
}

static void write_block(int block, word_t *data)
{
  int i;
  if (tape_seek(block) == -1)
    fail("Seeking to block.");
  for (i = 0; i < BLOCK_WORDS; i++)
    write_word(data[i]);
}

static int empty_entry(word_t *data)
{
  return (data[0] == EMPTY && data[1] == EMPTY &&
          data[2] == EMPTY && data[3] == EMPTY &&
          data[4] == EMPTY && data[5] == EMPTY &&
          data[6] == EMPTY && data[7] == EMPTY);
}

static int zero_entry(word_t *data)
{
  return (data[0] == 0 && data[1] == 0 &&
          data[2] == 0 && data[3] == 0 &&
          data[4] == 0 && data[5] == 0 &&
          data[6] == 0 && data[7] == 0);
}

static int unused_entry(word_t *data)
{
  return empty_entry(data) || zero_entry(data);
}

static void entry_name(word_t *data, char *name)
{
  word_t word, c;
  unsigned i;
  *name = 0;
  for (i = 0; i < 8; i++) {
    if (i & 1) 
      word <<= 6;
    else
      word = data[i >> 1];
    c = (word >> 6) & 077;
    if (c == 077)
      strcat(name, " ");
    else
      strcat(name, convert_to_utf8(c));
  }
  name += 7;
  while (*name == ' ')
    *name-- = 0;
}

static void entry(word_t *data, entry_fn manuscript, entry_fn binary)
{
  char name[20];
  if (unused_entry(data))
    return;
  entry_name(data, name);
  manuscript(name, &data[4], &data[5]);
  binary(name, &data[6], &data[7]);
}

static void traverse(entry_fn manuscript, entry_fn binary)
{
  int i;
  for (i = 8; i < 512; i += 8)
    entry(&directory[i], manuscript, binary);
}

static void print_name_and_manuscript(const char *name, word_t *block, word_t *blocks)
{
  fprintf(stdout, "%-8s", name);
  if (*block != EMPTY && *blocks != EMPTY)
    fprintf(stdout, "  M %03o %-3o", *block, *blocks);
  else
    fprintf(stdout, "           ");
}

static void print_binary(const char *name, word_t *block, word_t *blocks)
{
  if (*block != EMPTY && *blocks != EMPTY)
    fprintf(stdout, "  B %03o %-3o", *block, *blocks);
  fprintf(stdout, "\n");
}

static void vacant_name(word_t *data)
{
  const char *p = arg_name;
  word_t word = 0;
  char c;
  int i;

  for (i = 0; i < 8; i++) {
    if (*p == 0)
      c = 077;
    else
      c = convert_to_linc(*p++);
    word <<= 6;
    word |= c;
    if (i & 1) {
      *data++ = word;
      word = 0;
    }
  }
}

static void overlap(const char *name, word_t *block, word_t *blocks)
{
  int input_end = *input_block + input_blocks;
  int this_end = (*block & 0777) + (*blocks & 0777);
  if (input_end > (*block & 0777) && *input_block < this_end)
    *input_block = -1;
}

static void fit_input(void)
{
  int i;
  for (i = 0430; i < 0777 - input_blocks; i++) {
    *input_block = i;
    traverse(overlap, overlap);
    if (*input_block > 0)
      return;
  }
  for (i = 0270 - input_blocks; i >= 0; i--) {
    *input_block = i;
    traverse(overlap, overlap);
    if (*input_block > 0)
      return;
  }
  fail("Input doesn't fit in file areas.");
}

static int get_manuscript(word_t *data)
{
  int c1, c2;
  c1 = getchar();
  if (c1 == EOF)
    return 0;
  c2 = getchar();
  if (c2 == EOF)
    c2 = 'y';
  *data = convert_to_linc(c2) | (convert_to_linc(c1) << 6);
  return 1;
}

static int get_binary(word_t *data)
{
  int c1, c2;
  c1 = getchar();
  if (c1 == EOF)
    return 0;
  c2 = getchar();
  if (c2 == EOF)
    fail("Binary input must be even number of octets.");
  if (c2 & 0xF0)
    fail("Bad data in input.");
  *data = c1 | (c2 << 8);
  return 1;
}

static word_t *blank(word_t *data, word_t word)
{
  int n = (data - input) % BLOCK_WORDS;
  int i;
  for (i = n; i < BLOCK_WORDS; i++)
    *data++ = word;
  return data;
}

static void collect_input(int (*get_data)(word_t *), word_t zero)
{
  word_t *data = input;
  for (;;) {
    if (get_data(data++))
      continue;
    data--;
    data = blank(data, zero);
    break;
  }
  input_blocks = (data - input) / BLOCK_WORDS;
}

static void save_input(const char *message,
                       int (*get_data)(word_t *),
                       word_t zero,
                       word_t *block,
                       word_t *blocks)
{
  int i;
  collect_input(get_data, zero);
  *block = *blocks = EMPTY;
  fit_input();
  fprintf(stderr, "%s %s, %03o %o.\n",
          message, arg_name, *input_block, input_blocks);
  for (i = 0; i < input_blocks; i++)
    write_block(*input_block + i, input);
  *block = *input_block;
  *blocks = input_blocks;
  write_block(directory_block, directory);
  write_block(directory_block + 1, directory + BLOCK_WORDS);
  exit(0);
}

static word_t *vacant(void)
{
  word_t *data;
  int i;
  for (i = 8; i < 512; i += 8) {
    data = &directory[i];
    if (!unused_entry(data))
      continue;
    vacant_name(data);
    data[4] = EMPTY;
    data[5] = EMPTY;
    data[6] = EMPTY;
    data[7] = EMPTY;
    return data;
  }
  fail("Directory full.");
  return NULL;
}

static int found(const char *name)
{
  return strcasecmp(name, arg_name) == 0;
}

static void ignore(const char *name, word_t *block, word_t *blocks)
{
}

static void save_manuscript_entry(const char *name, word_t *block, word_t *blocks)
{
  if (!found(name))
    return;
  save_input("Overwriting manuscript", get_manuscript, 07777, block, blocks);
}

static void add_manuscript_entry(const char *name, word_t *block, word_t *blocks)
{
  word_t data[BLOCK_WORDS];
  int i, j, n = *block, m = *blocks;
  word_t word, c;
  if (!found(name))
    return;
  if (*block == EMPTY && *blocks == EMPTY)
    fail("Entry has no manuscript.");
  for (i = 0; i < m; i++) {
    read_block(n++, data);
    for (j = 0; j < BLOCK_WORDS; j++) {
      word = data[j];
      c = (word >> 6) & 077;
      fputs(convert_to_utf8(c), stdout);
      c = word & 077;
      fputs(convert_to_utf8(c), stdout);
    }
  }
  exit(0);
}

static void save_binary_entry(const char *name, word_t *block, word_t *blocks)
{
  if (!found(name))
    return;
  save_input("Overwriting binary", get_binary, 00000, block, blocks);
}

static void load_binary_entry(const char *name, word_t *block, word_t *blocks)
{
  word_t data[BLOCK_WORDS];
  int i, j, n = *block, m = *blocks;
  if (!found(name))
    return;
  if (*block == EMPTY && *blocks == EMPTY)
    fail("Entry has no binary.");
  for (i = 0; i < m; i++) {
    read_block(n++, data);
    for (j = 0; j < BLOCK_WORDS; j++) {
      fputc(data[j] & 0xFF, stdout);
      fputc((data[j] >> 8) & 0xF, stdout);
    }
  }
  exit(0);
}

static void display_index(void)
{
  traverse(print_name_and_manuscript, print_binary);
}

static void search_index(void)
{
  word_t data[BLOCK_WORDS];
  int i;
  for (i = 0; i < TAPE_BLOCKS; i++) {
    read_block(i, data);
    if (empty_entry(data)) {
      fprintf(stdout, "Block %03o may be an index", i);
      if ((i & 7) != 6)
        fprintf(stdout, ", but doesn't end in 6");
      fprintf(stdout, ".\n");
    }
  }
}

static void save_manuscript(void)
{
  word_t *data;
  traverse(save_manuscript_entry, ignore);
  data = vacant();
  save_input("New manuscript", get_manuscript, 07777, &data[4], &data[5]);
}

static void add_manuscript(void)
{
  traverse(add_manuscript_entry, ignore);
  fail("Manuscript not found.");
}

static void save_binary(void)
{
  word_t *data;
  traverse(ignore, save_binary_entry);
  data = vacant();
  save_input("New binary", get_binary, 00000, &data[6], &data[7]);
}

static void load_binary(void)
{
  traverse(ignore, load_binary_entry);
  fail("Binary not found.");
}

static char used_block[TAPE_BLOCKS];

static void used(const char *name, word_t *block, word_t *blocks)
{
  int i, j, n;
  if (*block == EMPTY && *blocks == EMPTY)
    return;
  n = *blocks & 0777;
  for (i = 0, j = (*block & 0777); i < n; i++, j++) {
    if (used_block[j])
      fprintf(stderr, "Entry %s reuses block %03o.\n", name, j);
    used_block[j] = 1;
  }
}

static void delete_manuscript_entry(const char *name, word_t *block, word_t *blocks)
{
  if (!found(name))
    return;
  if (block[2] == EMPTY && block[3] == EMPTY) {
    block[-4] = EMPTY;
    block[-3] = EMPTY;
    block[-2] = EMPTY;
    block[-1] = EMPTY;
  }
  block[0] = EMPTY;
  block[1] = EMPTY;
}

static void delete_binary_entry(const char *name, word_t *block, word_t *blocks)
{
  if (!found(name))
    return;
  if (block[-1] == EMPTY && block[-2] == EMPTY) {
    block[-6] = EMPTY;
    block[-5] = EMPTY;
    block[-4] = EMPTY;
    block[-3] = EMPTY;
  }
  block[0] = EMPTY;
  block[1] = EMPTY;
}

static void delete_manuscript(void)
{
  traverse(delete_manuscript_entry, ignore);
  write_block(directory_block, directory);
  write_block(directory_block + 1, directory + BLOCK_WORDS);
}

static void delete_binary(void)
{
  traverse(ignore, delete_binary_entry);
  write_block(directory_block, directory);
  write_block(directory_block + 1, directory + BLOCK_WORDS);
}

static void display_unused(void)
{
  int i, start, n;
  memset(used_block, 0, sizeof used_block);
  traverse(used, used);
  fprintf(stdout, "Unused blocks:\n");
  start = -1;
  n = 0;
  for (i = 0; i < TAPE_BLOCKS; i++) {
    if (used_block[i]) {
      if (start == -1)
        ;
      else if (i - start == 1)
        fprintf(stdout, "%03o  ", i-1), n += 5;
      else
        fprintf(stdout, "%03o-%03o   ", start, i-1), n += 10;
      if (n > 60)
        fputc('\n', stdout);
      start = -1;
      continue;
    }
    if (start == -1)
      start = i;
  }

  if (start == -1)
    ;
  else if (start - i == 1)
    fprintf(stdout, "%03o  ", i-1), n += 5;
  else
    fprintf(stdout, "%03o-%03o   ", start, i-1), n += 10;
  fputc('\n', stdout);
}

static void make_index(void)
{
  int i;
  for (i = 0; i < INDEX_BLOCKS * BLOCK_WORDS; i++)
    directory[i] = EMPTY;
  for (i = 0; i < INDEX_BLOCKS; i++)
    write_block(directory_block + i, directory + i * BLOCK_WORDS);
}

static void mark_tape(void)
{
  /* Which format, plain or Gesswein? */
  int reverse_offset = forward_offset = -8;
  long size;
  if (tape_seek(TAPE_BLOCKS + 10) == -1)
    fail("Seeking to block.");
  write_word(BLOCK_WORDS);
  write_word(forward_offset);
  write_word(reverse_offset);
  size = ftell(tape);
  if (size == -1 || ftruncate(fileno(tape), size) == -1)
    fail("Setting image file size.");
  exit(0);
}

int metadata(int *block_size, int *forward_offset, int *reverse_offset)
{
  long size;
  if (fseek(tape, 0L, SEEK_END) == -1)
    fail("Getting file size");
  size = ftell(tape);
  if (size == -1)
    fail("Getting file size");
  if (size == TAPE_BLOCKS * BLOCK_WORDS * 2) {
    /* Plain image. */
    *block_size = BLOCK_WORDS;
    *forward_offset = 0;
    *reverse_offset = 0;
  } else if ((size % BLOCK_OCTETS) == 6) {
    /* Extended image with additional meta data. */
    word_t data[3];
    if (tape_seek(size / BLOCK_OCTETS) == -1)
      fail("Seeking to metadata.");
    data[0] = read_int16();
    data[1] = read_int16();
    data[2] = read_int16();
    *block_size = data[0];
    *forward_offset = (signed short)data[1];
    *reverse_offset = (signed short)data[2];
  } else
    return -1;
  return 0;
}

static void get_metadata(void)
{
  int block_size, reverse_offset;
  metadata(&block_size, &forward_offset, &reverse_offset);
  if (block_size != BLOCK_WORDS)
    fail("Tape has bad block size");
  if (forward_offset != reverse_offset)
    fail("Forward offset differs from reverse offset");
}

static void get_index(void)
{
  get_metadata();
  read_block(directory_block, directory);
  if (!empty_entry(&directory[0])) {
    fprintf(stderr, "Block %03o has no index, ", directory_block);
    directory_block = 0326;
    fprintf(stderr, "trying %03o.\n", directory_block);
    read_block(directory_block, directory);
    if (!empty_entry(&directory[0]))
      fail("Directory block has wrong format.");
  }
  read_block(directory_block + 1, directory + BLOCK_WORDS);
}

static void read_index(const char *file)
{
  tape = fopen(file, "rb");
  if (tape == NULL)
    fail("Open tape.");
  get_index();
}

static void write_index(const char *file)
{
  tape = fopen(file, "r+b");
  if (tape == NULL)
    fail("Open tape.");
  get_index();
}

static void read_noindex(const char *file)
{
  tape = fopen(file, "rb");
  if (tape == NULL)
    fail("Open tape.");
  get_metadata();
}

static void write_noindex(const char *file)
{
  tape = fopen(file, "r+b");
  if (tape == NULL)
    fail("Open tape.");
  get_metadata();
}

static void create_noindex(const char *file)
{
  tape = fopen(file, "wb");
  if (tape == NULL)
    fail("Open tape.");
}

static const struct {
  const char *name;
  void (*open)(const char *);
  void (*fn)(void);
} command[] = {
  { "dx", read_index,     display_index },  
  { "sx", read_noindex,   search_index },   
  { "sm", write_index,    save_manuscript },
  { "am", read_index,     add_manuscript }, 
  { "sb", write_index,    save_binary },    
  { "lo", read_index,     load_binary },    
  { "dm", write_index,    delete_manuscript },         
  { "db", write_index,    delete_binary },         
  { "du", read_index,     display_unused }, 
  { "mx", write_noindex,  make_index },     
  { "mk", create_noindex, mark_tape }       
};

static void usage(const char *name)
{
  int i;
  fprintf(stderr, "Usage: %s command file [name]\n", name);
  fprintf(stderr, "Commands:");
  for (i = 0; i < sizeof command / sizeof command[0]; i++)
    fprintf(stderr, " %s", command[i].name);
  fprintf(stderr, "\nFile: tape image file name.\n");
  fprintf(stderr, "Name: LAP6 entry name.\n");
  exit(1);
}

int main(int argc, char **argv)
{
  int i;

  arg_name = argv[3];
  if (argv[1] == NULL)
    usage(argv[0]);

  for (i = 0; i < sizeof command / sizeof command[0]; i++) {
    if (strcmp(argv[1], command[i].name) == 0) {
      command[i].open(argv[2]);
      command[i].fn();
      return 0;
    }
  }

  fail("Unknonwn command.");
  return 1;
}
