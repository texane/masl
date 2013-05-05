#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include "masl.h"


#if 1
#include <stdio.h>
#define MASL_PRINTF(__s, ...) printf("%u: " __s, __LINE__, ## __VA_ARGS__)
#define MASL_PERROR(__s, ...) printf("%u: " __s, __LINE__, ## __VA_ARGS__)
#else
#define MASL_PRINTF(__s, ...)
#define MASL_PERROR(__s, ...)
#endif


/* gpio sysfs based implementation */

typedef struct gpio_handle
{
  int value_fd;
  char name_str[2];
  size_t name_len;
} gpio_handle_t;

static size_t uint_to_string(unsigned int x, char* buf)
{
  /* assume 0 <= x < 100 */

  size_t pos = 0;

  if (x >= 10)
  {
    buf[0] = '0' + x / 10;
    pos = 1;
  }

  buf[pos] = '0' + (x % 10);
  
  return pos + 1;
}

static void make_sysfs_path
(gpio_handle_t* h, char* buf, const char* suffix_str, size_t suffix_len)
{
#define SYSFS_GPIO_DIR_STR "/sys/class/gpio/gpio"
#define SYSFS_GPIO_DIR_LEN (sizeof(SYSFS_GPIO_DIR_STR) - 1)

  const size_t name_pos = SYSFS_GPIO_DIR_LEN;
  const size_t suffix_pos = name_pos + h->name_len;
  const size_t zero_pos = suffix_pos + suffix_len;

  memcpy(buf, SYSFS_GPIO_DIR_STR, SYSFS_GPIO_DIR_LEN);
  memcpy(buf + name_pos, h->name_str, h->name_len);
  memcpy(buf + suffix_pos, suffix_str, suffix_len);
  buf[zero_pos] = 0;
}

static int gpio_open_common
(gpio_handle_t* h, unsigned int index, unsigned int is_in)
{
  /* echo 27 > /sys/class/gpio/export */
  /* echo {in,out} > /sys/class/gpio/gpio27/direction */

  int err = -1;
  int export_fd;
  char* dir_str;
  size_t dir_len;
  int dir_fd;
  char buf[128];

  export_fd = open("/sys/class/gpio/export", O_WRONLY);
  if (export_fd == -1)
  {
    MASL_PERROR("open export\n");
    goto on_error_0;
  }

  h->name_len = uint_to_string(index, h->name_str);

  errno = 0;
  if (write(export_fd, h->name_str, h->name_len) != (ssize_t)h->name_len)
  {
    if (errno != EBUSY)
    {
      MASL_PERROR("write export\n");
      goto on_error_1;
    }
  }

#define DIRECTION_SUFFIX_STR "/direction"
#define DIRECTION_SUFFIX_LEN (sizeof(DIRECTION_SUFFIX_STR) - 1)
  make_sysfs_path(h, buf, DIRECTION_SUFFIX_STR, DIRECTION_SUFFIX_LEN);
  dir_fd = open(buf, O_RDWR);
  if (dir_fd == -1)
  {
    MASL_PERROR("open direction");
    goto on_error_2;
  }

  dir_str = is_in ? "in" : "out";
  dir_len = is_in ? 2 : 3;
  if (write(dir_fd, dir_str, dir_len) != (ssize_t)dir_len)
  {
    MASL_PERROR("write");
    goto on_error_3;
  }

#define VALUE_SUFFIX_STR "/value"
#define VALUE_SUFFIX_LEN (sizeof(VALUE_SUFFIX_STR) - 1)
  make_sysfs_path(h, buf, VALUE_SUFFIX_STR, VALUE_SUFFIX_LEN);
  h->value_fd = open(buf, O_RDWR);
  if (h->value_fd == -1)
  {
    MASL_PERROR("open value");
    goto on_error_4;
  }

  /* success */
  err = 0;

 on_error_4:
 on_error_3:
  close(dir_fd);
 on_error_2:
 on_error_1:
  close(export_fd);
 on_error_0:
  return err;
}

__attribute__((unused))
static int gpio_open_out(gpio_handle_t* h, unsigned int n)
{
  static const unsigned int is_in = 0;
  return gpio_open_common(h, n, is_in);
}

static int gpio_open_in(gpio_handle_t* h, unsigned int n)
{
  static const unsigned int is_in = 1;
  return gpio_open_common(h, n, is_in);
}

static int gpio_close(gpio_handle_t* h)
{
  int err = 0;
  int fd;

  fd = open("/sys/class/gpio/unexport", O_WRONLY);
  if (fd != -1)
  {
    err = -1;
    if (write(fd, h->name_str, h->name_len) != (ssize_t)h->name_len)
      err = -1;
    close(fd);
  }

  close(h->value_fd);

  return err;
}

static int gpio_set_both_edges(gpio_handle_t* h)
{
  /* echo {falling,rising,both} > /sys/class/gpio/gpio27/edge */

  int err = -1;
  int edge_fd;
  char buf[128];

#define EDGE_SUFFIX_STR "/edge"
#define EDGE_SUFFIX_LEN (sizeof(EDGE_SUFFIX_STR) - 1)
  make_sysfs_path(h, buf, EDGE_SUFFIX_STR, EDGE_SUFFIX_LEN);

  edge_fd = open(buf, O_RDWR);
  if (edge_fd == -1)
  {
    MASL_PERROR("open");
    goto on_error_0;
  }

#define BOTH_STR "both"
#define BOTH_LEN (sizeof(BOTH_STR) - 1)
  if (write(edge_fd, BOTH_STR, BOTH_LEN) != BOTH_LEN)
  {
    MASL_PERROR("write");
    goto on_error_1;
  }

  err = 0;

 on_error_1:
  close(edge_fd);
 on_error_0:
  return err;
}

__attribute__((unused))
static int gpio_set_value(gpio_handle_t* h, unsigned int x)
{
  char c;

  c = '0' + x;
  if (write(h->value_fd, &c, sizeof(c)) != (ssize_t)sizeof(c))
  {
    MASL_PERROR("write gpio value\n");
    return -1;
  }
  return 0;
}

__attribute__((unused))
static int gpio_get_value(gpio_handle_t* h, unsigned int* x)
{
  char c;

  if (read(h->value_fd, &c, sizeof(c)) != (ssize_t)sizeof(c))
  {
    MASL_PERROR("read gpio value\n");
    return -1;
  }

  *x = (c == '0') ? 0 : 1;

  return 0;
}

static int gpio_get_wait_fd(gpio_handle_t* h)
{
  return h->value_fd;
}


/* masl */

#define MASL_GPIO_INT 27
#define MASL_GPIO_WAKE 17
#define MASL_GPIO_RESET 22

#define MASL_SLAVE_COUNT 1


struct masl_handle
{
  /* gpio handles */
  gpio_handle_t int_gpio[MASL_SLAVE_COUNT];
  gpio_handle_t reset_gpio[MASL_SLAVE_COUNT];
  gpio_handle_t wake_gpio[MASL_SLAVE_COUNT];

  /* epoll related */
  int epoll_fd;
  struct epoll_event epoll_events[MASL_SLAVE_COUNT];

};


masl_err_t masl_init(masl_handle_t** hh)
{
  masl_handle_t* h;
  int int_fd;
  int err;
  struct epoll_event ev;

  h = malloc(sizeof(masl_handle_t));
  if (h == NULL)
  {
    MASL_PERROR();
    goto on_error_0;
  }

  if (gpio_open_in(&h->int_gpio[0], MASL_GPIO_INT))
    goto on_error_1;

  if (gpio_set_both_edges(&h->int_gpio[0]))
    goto on_error_2;

  /* 0 as default value resets the slave */
  if (gpio_open_out(&h->reset_gpio[0], MASL_GPIO_RESET))
    goto on_error_3;
  gpio_set_value(&h->reset_gpio[0], 1);

  /* epoll events, int gpio registration */
  h->epoll_fd = epoll_create(MASL_SLAVE_COUNT);
  if (h->epoll_fd < 0)
  {
    MASL_PERROR();
    goto on_error_4;
  }

  /* trigger event on urgent data, slave slave index as context */
  ev.events = EPOLLPRI;
  ev.data.u32 = 0;
  int_fd = gpio_get_wait_fd(&h->int_gpio[0]);
  err = epoll_ctl(h->epoll_fd, EPOLL_CTL_ADD, int_fd, &ev);
  if (err)
  {
    MASL_PERROR();
    goto on_error_5;
  }

  *hh = h;

  return MASL_ERR_SUCCESS;

 on_error_5:
  close(h->epoll_fd);
 on_error_4:
  gpio_close(&h->reset_gpio[0]);
 on_error_3:
 on_error_2:
  gpio_close(&h->int_gpio[0]);
 on_error_1:
  free(h);
 on_error_0:
  return MASL_ERR_FAILURE;
}

masl_err_t masl_fini(masl_handle_t* h)
{
  close(h->epoll_fd);
  gpio_close(&h->reset_gpio[0]);
  gpio_close(&h->int_gpio[0]);
  return MASL_ERR_FAILURE;
}

masl_err_t masl_reset_slave(masl_handle_t* h, unsigned int si)
{
  /* si the slave index */

  if (gpio_set_value(&h->reset_gpio[0], 0)) return MASL_ERR_FAILURE;
  usleep(1000);
  gpio_set_value(&h->reset_gpio[0], 1);
  return MASL_ERR_SUCCESS;
}

extern int load_linux_only_main(int, char**);

masl_err_t masl_program_slave
(masl_handle_t* h, unsigned int si, const char* filename)
{
  const char* av[] = { "fubar", "-mmcu=mk20dx128", filename, NULL };
  const int ac = 3;

  /* TODO: use return code */
  load_linux_only_main(ac, (char**)av);

  return MASL_ERR_SUCCESS;
}

static inline int timeval_to_ms(const struct timeval* tv)
{
  return 1 + tv->tv_sec * 1000 + tv->tv_usec / 1000;
}

static inline void ms_to_timeval(int ms, struct timeval* tv)
{
  const unsigned int us = (unsigned int)ms * 1000;
  tv->tv_sec = us / 1000000;
  tv->tv_usec = us % 1000000;
}

masl_err_t masl_loop
(masl_handle_t* h, masl_slavefn_t onslave, void* udata, int timeout)
{
  const unsigned int has_timeout = (timeout <= 0) ? 0 : 1;
  struct timeval tv_pre = { 0, };
  struct timeval tv_now = { 0, };
  struct timeval tv_rem = { 0, };
  unsigned char buf[32];
  unsigned int i;
  int nev;

  if (has_timeout) ms_to_timeval(timeout, &tv_rem);
  else timeout = -1;

  while (1)
  {
  redo_epoll:
    if (has_timeout)
    {
      timeout = timeval_to_ms(&tv_rem);
      gettimeofday(&tv_pre, NULL);
    }

    errno = 0;
    nev = epoll_wait(h->epoll_fd, h->epoll_events, MASL_SLAVE_COUNT, timeout);

    if (nev < 0)
    {
      if (errno == EINTR)
      {
	if (has_timeout)
	{
	  gettimeofday(&tv_now, NULL);
	  timersub(&tv_now, &tv_pre, &tv_rem);
	}

	goto redo_epoll;
      }

      MASL_PERROR();
      return MASL_ERR_FAILURE;
    }
    else if (nev == 0)
    {
      /* timeout, not an error */
      return MASL_ERR_TIMEOUT;
    }

    if (has_timeout)
    {
      gettimeofday(&tv_now, NULL);
      timersub(&tv_now, &tv_pre, &tv_rem);
    }

    for (i = 0; i < (unsigned int)nev; ++i)
    {
      /* read the corresponding data */

      const unsigned int si = (unsigned int)h->epoll_events[i].data.u32;
      gpio_handle_t* const gpio_handle = &h->int_gpio[si];
      const int gpio_fd = gpio_get_wait_fd(gpio_handle);

      /* ack the event */
      lseek(gpio_fd, 0, SEEK_SET);
      read(gpio_fd, buf, sizeof(buf));

      if (onslave(h, si, udata) != MASL_ERR_CONTINUE)
      {
	/* this is not an error, return 0 */
	return MASL_ERR_SUCCESS;
      }
    }
  }

  return MASL_ERR_SUCCESS;
}

static masl_err_t onslave(masl_handle_t* h, unsigned int si, void* p)
{
  *(unsigned int*)p = si;
  return MASL_ERR_BREAK;
}

masl_err_t masl_wait_slave
(masl_handle_t* h, unsigned int* si, int timeout)
{
  return masl_loop(h, onslave, si, timeout);
}

masl_err_t masl_write_slave
(masl_handle_t* h, unsigned int si, const void* buf, size_t size)
{
  /* TODO: spi_write */
  return MASL_ERR_UNIMPL;
}

masl_err_t masl_read_slave
(masl_handle_t* h, unsigned int si, void* buf, size_t size)
{
  /* TODO: spi_read */
  return MASL_ERR_UNIMPL;
}
