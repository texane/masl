#include <stdio.h>
#include <unistd.h>
#include "masl.h"

static masl_err_t onslave(masl_handle_t* h, unsigned int si, void* p)
{
  printf("on slave: %u\n", si);
  return MASL_ERR_SUCCESS;
}

int main(int ac, char** av)
{
  masl_handle_t* h;

  if (masl_init(&h) != MASL_ERR_SUCCESS) return -1;

  masl_loop(h, onslave, NULL);
  masl_fini(h);

  return 0;
}
