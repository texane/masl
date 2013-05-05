#ifndef MASL_H_INCLUDED
#define MASL_H_INCLUDED

#include <sys/types.h>

typedef enum
{
  MASL_ERR_SUCCESS = 0,
  MASL_ERR_UNIMPL,
  MASL_ERR_FAILURE
} masl_err_t;

typedef struct masl_handle masl_handle_t;
typedef masl_err_t (*masl_slavefn_t)(masl_handle_t*, unsigned int, void*);

masl_err_t masl_init(masl_handle_t**);
masl_err_t masl_fini(masl_handle_t*);
masl_err_t masl_loop(masl_handle_t*, masl_slavefn_t, void*);
masl_err_t masl_reset_slave(masl_handle_t*, unsigned int);
masl_err_t masl_program_slave(masl_handle_t*, unsigned int, const char*);
masl_err_t masl_write_slave(masl_handle_t*, unsigned int, const void*, size_t);
masl_err_t masl_read_slave(masl_handle_t*, unsigned int, void*, size_t);

#endif /* ! MASL_H_INCLUDED */
