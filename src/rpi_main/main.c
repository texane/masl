#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>


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

  export_fd = open("/sys/class/gpio/export", O_RDWR);
  if (export_fd == -1)
  {
    perror("open");
    goto on_error_0;
  }

  h->name_len = uint_to_string(index, h->name_str);

  if (write(export_fd, h->name_str, h->name_len) != (ssize_t)h->name_len)
  {
    perror("write");
    goto on_error_1;
  }

#define DIRECTION_SUFFIX_STR "/direction"
#define DIRECTION_SUFFIX_LEN (sizeof(DIRECTION_SUFFIX_STR) - 1)
  make_sysfs_path(h, buf, DIRECTION_SUFFIX_STR, DIRECTION_SUFFIX_LEN);
  dir_fd = open(buf, O_RDWR);
  if (dir_fd == -1)
  {
    perror("open");
    goto on_error_2;
  }

  dir_str = is_in ? "in" : "out";
  dir_len = is_in ? 2 : 3;
  if (write(dir_fd, dir_str, dir_len) != (ssize_t)dir_len)
  {
    perror("write");
    goto on_error_3;
  }

#define VALUE_SUFFIX_STR "/value"
#define VALUE_SUFFIX_LEN (sizeof(VALUE_SUFFIX_STR) - 1)
  make_sysfs_path(h, buf, VALUE_SUFFIX_STR, VALUE_SUFFIX_LEN);
  h->value_fd = open(buf, O_RDWR);
  if (h->value_fd == -1)
  {
    perror("open");
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

  fd = open("/sys/class/gpio/unexport", O_RDWR);
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

  int edge_fd;
  ssize_t n;
  char buf[128];

#define EDGE_SUFFIX_STR "/edge"
#define EDGE_SUFFIX_LEN (sizeof(EDGE_SUFFIX_STR) - 1)
  make_sysfs_path(h, buf, EDGE_SUFFIX_STR, EDGE_SUFFIX_LEN);

  edge_fd = open(buf, O_RDWR);
  if (edge_fd == -1)
  {
    perror("open");
    return -1;
  }

#define BOTH_STR "both"
#define BOTH_LEN (sizeof(BOTH_STR) - 1)
  n = write(edge_fd, BOTH_STR, BOTH_LEN);
  close(edge_fd);

  if (n != (ssize_t)BOTH_LEN) return -1;
  return 0;
}

__attribute__((unused))
static int gpio_set_value(gpio_handle_t* h, unsigned int x)
{
  unsigned char buf[4];
  ssize_t n;

  buf[0] = '0' + x;
  n = write(h->value_fd, buf, sizeof(buf));
  if (n <= 0) return -1;
  return 0;
}

__attribute__((unused))
static int gpio_get_value(gpio_handle_t* h, unsigned int* x)
{
  unsigned char buf[4];
  ssize_t n;

  n = read(h->value_fd, buf, sizeof(buf));
  if (n <= 0) return -1;

  *x = (buf[0] == '0') ? 0 : 1;

  return 0;
}

static int gpio_get_wait_fd(gpio_handle_t* h)
{
  return h->value_fd;
}


/* masl */

#define MASL_CONFIG_GPIO_INT 27
#define MASL_CONFIG_GPIO_WAKE 17
#define MASL_CONFIG_GPIO_RESET 22

typedef struct masl_handle
{
  gpio_handle_t int_gpio;
  gpio_handle_t wake_gpio;
  gpio_handle_t reset_gpio;

} masl_handle_t;

static int masl_init(masl_handle_t* h)
{
  if (gpio_open_in(&h->int_gpio, MASL_CONFIG_GPIO_INT))
    return -1;
  gpio_set_both_edges(&h->int_gpio);
  return 0;
}

static int masl_fini(masl_handle_t* h)
{
  gpio_close(&h->int_gpio);
  return 0;
}

static int masl_loop(masl_handle_t* h)
{
  const int fd = gpio_get_wait_fd(&h->int_gpio);
  fd_set fds;

  while (1)
  {
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    select(fd + 1, NULL, NULL, &fds, NULL);
    printf("got\n");
  }

  return 0;
} 


/* main */

int main(int ac, char** av)
{
  masl_handle_t h;

  if (masl_init(&h) == -1)
    return -1;
  masl_loop(&h);
  masl_fini(&h);

  return 0;
}
