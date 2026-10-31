/* Each octet in the image is one tape line with two mark tracks and
 * six data tracks.  There is no timing track.
 *
 * The tracks are laid out like this in a tape line:
 * Bit 7 - Mark track A.
 * Bit 6 - Data track 0A
 * Bit 5 - Data track 1A.
 * Bit 4 - Data track 2A.
 * Bit 3 - Data track 0B.
 * Bit 2 - Data track 1B.
 * Bit 1 - Data track 2B.
 * Bit 0 - Mark track B.
 *
 * The code below implements the LINCtape 12-bit format.
 */

#include <stdio.h>
#include <stdlib.h>

#define DATA_WORDS       256
#define START_BLOCKS      10
#define DATA_BLOCKS      512
#define END_BLOCKS        15

#define BINARY(B3,B2,B1,B0) (((B3)<<3)|((B2)<<2)|((B1)<<1)|((B0)<<0))
#define MARK_EM   BINARY(0,0,0,0)
#define MARK_IM   BINARY(1,1,1,1)
#define MARK_FBM  BINARY(1,1,1,0)
#define MARK_GM   BINARY(0,0,1,0)
#define MARK_DM   BINARY(1,0,0,1)
#define MARK_FM   BINARY(1,0,1,1)
#define MARK_CM   BINARY(0,0,0,1)
#define MARK_RBM  BINARY(0,1,1,1)

static FILE *f;
static int checksum;

static void mark_bits(int mark1, int data1, int data2, int data3,
                      int data4, int data5, int data6, int mark2)
{
  int frame;
  frame =
    ((mark1 & 1) << 7) |
    ((data1 & 1) << 6) |
    ((data2 & 1) << 5) |
    ((data3 & 1) << 4) |
    ((data4 & 1) << 3) |
    ((data5 & 1) << 2) |
    ((data6 & 1) << 1) |
    ((mark2 & 1) << 0);
  fputc(frame, f);
}

static void mark_12bit_word(int mark, int data)
{
  int i;
  for (i = 0; i < 4; i++) {
    mark_bits(mark >> 3, data >> 0, data >> 4, data >> 8,
              data >> 0, data >> 4, data >> 8, mark >> 3);
    mark <<= 1;
    data >>= 1;
  }
}

static void mark_em(int n)
{
  fprintf(stderr, "Mark end zone.\n");
  while (n-- > 0)
    mark_12bit_word(MARK_EM, 0);
}

static void mark_im(int n)
{
  while (n-- > 0)
    mark_12bit_word(MARK_IM, 0);
}

static void mark_fbm(int bn)
{
  fprintf(stderr, "Mark block %o.\n", bn);
  mark_12bit_word(MARK_FBM, bn);
}

static void mark_gm(void)
{
  mark_12bit_word(MARK_GM, 0);
}

static void mark_dm(int data)
{
  mark_12bit_word(MARK_DM, data);
}

static void mark_fm(int data)
{
  mark_12bit_word(MARK_FM, data);
}

static int sum(int data)
{
  data &= 07777;
  checksum += data;
  checksum &= 07777;
  return data;
}

static void mark_random_data(void)
{
  int i;
  checksum = 0;
  for (i = 0; i < DATA_WORDS - 1; i++)
    mark_dm(sum(rand()));
  mark_fm(sum(rand()));
}

static void mark_cm(int data)
{
  mark_12bit_word(MARK_CM, data);
}

static void mark_rbm(int bn)
{
  mark_12bit_word(MARK_RBM, bn);
}

static void mark_block(int bn)
{
  if (bn < 0)
    bn = -bn ^ 07777;
  mark_fbm(bn);
  mark_gm();
  mark_random_data();
  mark_cm(checksum ^ 07777);
  mark_cm(checksum ^ 07777);
  mark_cm(checksum ^ 07777);
  mark_rbm(bn);
}

static void mark_tape(void)
{
  int i;
  mark_em(1000);
  mark_im(5);
  for (i = -START_BLOCKS; i < DATA_BLOCKS + END_BLOCKS; i++) {
    mark_block(i);
    mark_im(5);
  }
  mark_em(1000);
}

static int read_line_forward(void)
{
  int data = fgetc(f);
  if (data == EOF) {
    fprintf(stderr, "Physical end of tape.\n");
    exit(0);
  }
  return data;
}

static int read_line_backward(void)
{
  int data;
  if (fseek(f, -2, SEEK_CUR) != 0) {
    fprintf(stderr, "Error or physical start of tape.\n");
    exit(1);
  }
  data = fgetc(f);
  if (data == EOF) {
    fprintf(stderr, "Physical start of tape.\n");
    exit(0);
  }
  return data;
}

static void redundancy(int data)
{
  if ((data & 0x80) >> 7 != (data & 0x01)) {
    fprintf(stderr, "Inconsistent mark bits.\n");
    exit(1);
  }
  if ((data & 0x40) >> 3 != (data & 0x08)) {
    fprintf(stderr, "Inconsistent data bit 0.\n");
    exit(1);
  }
  if ((data & 0x20) >> 3 != (data & 0x04)) {
    fprintf(stderr, "Inconsistent data bit 1.\n");
    exit(1);
  }
  if ((data & 0x10) >> 3 != (data & 0x02)) {
    fprintf(stderr, "Inconsistent data bit 2.\n");
    exit(1);
  }
}

static int read_12bit_word(void)
{
  int i, mark, data;
  int line[4];
  line[0] = read_line_forward();
  line[1] = read_line_forward();
  line[2] = read_line_forward();
  line[3] = read_line_forward();
  redundancy(line[0]);
  redundancy(line[1]);
  redundancy(line[2]);
  redundancy(line[3]);
  mark =
    ((line[0] & 0x80) >> 4) |
    ((line[1] & 0x80) >> 5) |
    ((line[2] & 0x80) >> 6) |
    ((line[3] & 0x80) >> 7);
  for (data = i = 0; i < 4; i++) {
    data |=
      ((line[i] & 0x40) >> 2) |
      ((line[i] & 0x20) << 3) |
      ((line[i] & 0x10) << 8) ;
    data >>= 1;
  }
  /*
summing 2547 => 2547
summing 1706 => 4455
summing 4151 => 0626
summing 4163 => 5011

          .x.. ....
.... .... ...x ....

          ..x. ....
.... ...x .... ....

          ...x ....
...x .... .... ....
  */

  return (mark << 12) | data;
}

static void find_im(void)
{
  int mark;
  mark = read_12bit_word() >> 12;
  while (mark != MARK_IM) {
    mark = (mark << 1) & BINARY(1,1,1,0);
    mark |= read_line_forward() & 1;
  }
  fprintf(stderr, "Interblock zone.\n");
}

static int find_fbm(void)
{
  int mark = MARK_IM;
  int data, bn = 0;
  do {
    find_im();
    do {
      mark = (mark << 1) & BINARY(1,1,1,0);
      data = read_line_forward();
      mark |= data & 1;
      bn &= ~0x1110;
      bn |=
        ((data & 0x40) >> 2) |
        ((data & 0x20) << 3) |
        ((data & 0x10) << 8) ;
      bn >>= 1;
    } while (mark == MARK_IM);
    if (mark != MARK_FBM)
      fprintf(stderr, "Error in format, expected BM.\n");
  } while (mark != MARK_FBM);
  return bn;
}

static int find_gm(void)
{
  int data = read_12bit_word();
  int mark = data >> 12;
  if (mark != MARK_GM)
    fprintf(stderr, "Error in format, expected GM.\n");
  return mark == MARK_GM;
}

static int find_data(void)
{
  int n, data, mark;
  checksum = 0;
  for (n = 0;;) {
    data = read_12bit_word();
    mark = data >> 12;
    n++;
    switch (mark) {
    case MARK_DM:
      if (n > DATA_WORDS)
        goto oversize;
      sum(data);
      break;
    case MARK_FM:
      sum(data);
    oversize:
      fprintf(stderr, "Found %d words of block data.\n", n);
      return n == DATA_WORDS;
    default:
      fprintf(stderr, "Error in format, expected FM or DM.\n");
      return 0;
    }
  }
}

static int find_cm(void)
{
  int data = read_12bit_word();
  int mark = data >> 12;
  if (mark != MARK_CM)
    fprintf(stderr, "Error in format, expected CM.\n");
  return data & 07777;
}

static int find_rbm(void)
{
  int data = read_12bit_word();
  int mark = data >> 12;
  if (mark != MARK_RBM)
    fprintf(stderr, "Error in format, expected RBM.\n");
  return data & 07777;
}

static void check_tape(void)
{
  int bn, data;
  for (;;) {
    bn = find_fbm();
    fprintf(stderr, "Found forward block number %04o ", bn);
    if (bn & 04000)
      bn = -(bn ^ 07777);
    fprintf(stderr, "(%d).\n", bn);
    if (!find_gm())
      continue;
    if (!find_data())
      continue;
    data = find_cm();
    sum(data);
    fprintf(stderr, "Found %scorrect checksum %04o.\n",
            checksum == 07777 ? "" : "in", data);
  }
}

static void check_tape_by_line(void)
{
  int mark = MARK_IM;
  int data = 0;
  int line;

  for (;;) {
    line = read_line_forward();
    mark = (mark << 1) & BINARY(1,1,1,0);
    mark |= line & 1;
    data &= ~0x1110;
    data |=
      ((line & 0x40) >> 2) |
      ((line & 0x20) << 3) |
      ((line & 0x10) << 8) ;
    data >>= 1;

    switch (mark) {
    case MARK_EM:
      break;
    case MARK_IM:
      break;
    case MARK_FBM:
      fprintf(stderr, "Found forward block number %04o ", data);
      if (data & 04000)
        data = -(data ^ 07777);
      fprintf(stderr, "(%d).\n", data);
      break;
    case MARK_GM:
      checksum = 0;
      break;
    case MARK_DM:
      sum(data);
      break;
    case MARK_FM:
      sum(data);
      break;
    case MARK_CM:
      sum(data);
      fprintf(stderr, "Found %scorrect checksum %04o.\n",
              checksum == 07777 ? "" : "in", data);
      break;
    }
  }
}

int main(void)
{
  f = fopen("output.tape", "wb");
  mark_tape();
  fclose(f);
  f = fopen("output.tape", "rb");
  check_tape();
  fclose(f);
}
