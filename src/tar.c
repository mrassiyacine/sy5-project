#include "tar.h"

/* tar Header Block, from POSIX 1003.1-1990.  */

/* POSIX header.  */

/* Note that sizeof(struct posix_header) == BLOCKSIZE */
                            /* Total: 512 */

#define TMAGIC   "ustar"        /* ustar and a null */
#define TMAGLEN  6
#define TVERSION "00"           /* 00 and no null */
#define TVERSLEN 2

/* ... */

#define OLDGNU_MAGIC "ustar  "  /* 7 chars and a null */

/* ... */


/* Compute and write the checksum of a header, by adding all
   (unsigned) bytes in it (while hd->chksum is initially all ' ').
   Then hd->chksum is set to contain the octal encoding of this
   sum (on 6 bytes), followed by '\0' and ' '.
*/

void set_checksum(struct posix_header *hd) {
  memset(hd->chksum,' ',8);
  unsigned int sum = 0;
  char *p = (char *)hd;
  for (int i=0; i < BLOCKSIZE; i++) { sum += p[i]; }
  sprintf(hd->chksum,"%06o",sum);
}

/* Check that the checksum of a header is correct */

int check_checksum(struct posix_header *hd) {
  unsigned int checksum;
  sscanf(hd->chksum,"%o ", &checksum);
  unsigned int sum = 0;
  char *p = (char *)hd;
  for (int i=0; i < BLOCKSIZE; i++) { sum += p[i]; }
  for (int i=0;i<8;i++) { sum += ' ' - hd->chksum[i]; }
  return (checksum == sum);
}
