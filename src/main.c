/* masl library unit test */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include "masl.h"

int main(int ac, char** av)
{
  masl_handle_t* h;
  unsigned int si;

  if (masl_init(&h) != MASL_ERR_SUCCESS) return -1;

  if (strcmp(av[1], "read") == 0)
  {
    const unsigned int si = atoi(av[2]);
    size_t i;
    uint8_t data[32];
    const masl_err_t err = masl_read_slave(h, si, data, sizeof(data));
    if (err != MASL_ERR_SUCCESS) printf("masl error\n");
    for (i = 0; i < sizeof(data); ++i) printf("%02x", data[i]);
    printf("\n");
  }
  else if (strcmp(av[1], "write") == 0)
  {
    const unsigned int si = atoi(av[2]);
    const char* const data = av[3];
    const masl_err_t err = masl_write_slave(h, si, data, strlen(data));
    if (err != MASL_ERR_SUCCESS) printf("masl error\n");
  }
  else if (strcmp(av[1], "reset") == 0)
  {
    const unsigned int si = atoi(av[2]);
    const masl_err_t err = masl_reset_slave(h, si);
    if (err != MASL_ERR_SUCCESS) printf("masl error\n");
  }
  else if (strcmp(av[1], "program") == 0)
  {
    const unsigned int si = atoi(av[2]);
    const char* const filename = av[3];
    const masl_err_t err = masl_program_slave(h, si, filename);
    if (err != MASL_ERR_SUCCESS) printf("masl error\n");
  }
  else if (strcmp(av[1], "wait") == 0)
  {
    while (1)
    {
      if (masl_wait_slave(h, &si, -1) != MASL_ERR_SUCCESS)
      {
	printf("masl slave error\n");
	break ;
      }

      printf("on slave: %u\n", si);
    }
  }

  masl_fini(h);

  return 0;
}
