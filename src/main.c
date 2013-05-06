/* masl library unit test */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include "masl.h"

static void print_data(const uint8_t* data, size_t len)
{
  for (; len; --len, ++data) printf("%02x", *data);
  printf("\n");
}

int main(int ac, char** av)
{
  masl_handle_t* h;
  unsigned int si;
  uint8_t data[32];
  masl_err_t err = MASL_ERR_SUCCESS;

  if (masl_init(&h) != MASL_ERR_SUCCESS) return -1;

  if (strcmp(av[1], "read") == 0)
  {
    si = atoi(av[2]);

    err = masl_read_slave(h, si, data, sizeof(data));
    if (err != MASL_ERR_SUCCESS) goto on_error;

    print_data(data, sizeof(data));
  }
  else if (strcmp(av[1], "write") == 0)
  {
    size_t len = strlen(av[3]);
    if (len > sizeof(data)) len = sizeof(data);
    memcpy(data, av[3], len);

    si = atoi(av[2]);

    err = masl_write_slave(h, si, data, sizeof(data));
    if (err != MASL_ERR_SUCCESS) goto on_error;
  }
  else if (strcmp(av[1], "reset") == 0)
  {
    si = atoi(av[2]);
    err = masl_reset_slave(h, si);
    if (err != MASL_ERR_SUCCESS) goto on_error;
  }
  else if (strcmp(av[1], "program") == 0)
  {
    const char* const filename = av[3];
    si = atoi(av[2]);
    err = masl_program_slave(h, si, filename);
    if (err != MASL_ERR_SUCCESS) goto on_error;
  }
  else if (strcmp(av[1], "alert") == 0)
  {
    while (1)
    {
      err = masl_wait_slave(h, &si, -1);
      if (err != MASL_ERR_SUCCESS) goto on_error;

      printf("alert on slave: %u\n", si);

      err = masl_read_slave(h, si, data, sizeof(data));
      if (err != MASL_ERR_SUCCESS) goto on_error;

      print_data(data, sizeof(data));
    }
  }

 on_error:
  if (err != MASL_ERR_SUCCESS) printf("masl_error: %u\n", err);

  masl_fini(h);

  return 0;
}
