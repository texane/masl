#include <stdio.h>
#include <unistd.h>
#include "masl.h"

int main(int ac, char** av)
{
  masl_handle_t* h;
  unsigned int si;

  if (masl_init(&h) != MASL_ERR_SUCCESS) return -1;

  while (1)
  {
    if (masl_wait_slave(h, &si, -1) != MASL_ERR_SUCCESS)
    {
      printf("masl_wait_slave error\n");
      break ;
    }

    printf("on slave: %u\n", si);
  }

  masl_fini(h);

  return 0;
}
