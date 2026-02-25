#include "base/logging.h"
#include "base/app_common_defs.h"

#include <cstdint>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <termios.h>
#include <unistd.h>


static int g_fd_uart1 = -1;
static int g_fd_uart2 = -1;
volatile bool g_running = false;


#define WIFI_CONNECT_SUCCESS "success"
#define WIFI_CONNECT_FAILED "fail"
#define UART1_DEV_PATH "/dev/ttyS1"
#define UART2_DEV_PATH "/dev/ttyS2"


#define ALIGN_DOWN(size, alignment) (size & ~(alignment - 1))
#define ALIGN_UP(size, alignment) ((size + alignment - 1) & ~(alignment - 1))

static int uart_configure(int fd);


int mcu_control_start(void) {
    const char *uart1 = UART1_DEV_PATH;
    const char *uart2 = UART2_DEV_PATH;
    g_fd_uart1 = open(uart1, O_RDWR | O_NOCTTY | O_NDELAY);
    g_fd_uart2 = open(uart2, O_RDWR | O_NOCTTY | O_NDELAY);

    if (g_fd_uart1 < 0) {
        printf("open %s is failed", uart1);
    } else {
      uart_configure(g_fd_uart1);
    }

    if (g_fd_uart2 < 0) {
        printf("open %s is failed", uart2);
    } else {
      uart_configure(g_fd_uart2);
    }

    return g_fd_uart2;
}


static int uart_configure(int fd)
{
    struct termios newtio, oldtio;
    if (tcgetattr(fd, &oldtio) != 0) {
        perror("SetupSerial 1");
        return -1;
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag |= CLOCAL | CREAD;  // 设置本地连接和接收使能标志位
    newtio.c_cflag &= ~CSIZE;  // 清除数据位设置标志位
    newtio.c_cflag |= CS8;  // 设置数据位为8位
    newtio.c_cflag &= ~PARENB;  // 清除校验使能标志位

    cfsetispeed(&newtio, B9600);
    cfsetospeed(&newtio, B9600);

    newtio.c_cflag &= ~CSTOPB;  // 设置停止位为1位
    newtio.c_cc[VTIME] = 50;  // 设置超时时间为2秒
    newtio.c_cc[VMIN] = 0;
    tcflush(fd, TCIFLUSH);

    if ((tcsetattr(fd, TCSANOW, &newtio)) != 0) {
        perror("com set error");
        return -1;
    }

    fcntl(fd, F_SETFL, 0);

    return 0;
}


static int uart_write(int fd, const uint8_t *data, int data_length) {
  const int buffer_size = 2;
  char buffer[buffer_size];

  int bytes_write = 0;
  while (bytes_write < data_length) {
    int sending_size = (data_length - bytes_write);
    memset(buffer, 0, buffer_size);
    for (int i = 0; i < sending_size; i++) {
      printf("0x%x ", data[bytes_write + i]);
    }
    printf("\n");
    memcpy(buffer, data + bytes_write, sending_size);
    int n = HANDLE_EINTR(write(fd, buffer, buffer_size));
    if (n <= 0) {
      return n;
    }

    if (sending_size < buffer_size) {
      n = sending_size;
    }

    bytes_write += n;
  }

  LOG_I("uart_write %d", bytes_write);
  return bytes_write;
}

static int uart_read(int fd, char *buffer, int length) {
  int bytes_read = HANDLE_EINTR(read(fd, buffer, length));
  if (bytes_read > 0) {
      LOG_I("uart_read %s", buffer);
  } else {
    LOGE("uart_read nothing");
  }
  return bytes_read;
}

int mcu_control_stop(void) {
  if (g_fd_uart1 > 0) {
    close(g_fd_uart1);
    g_fd_uart1 = -1;
  }

  if (g_fd_uart2 > 0) {

    close(g_fd_uart2);
    g_fd_uart2 = -1;
  }

  return 0;
}

int read_mic_data() {
  while (g_running) {
    char buffer[1024] = {0};
    int ret = uart_read(g_fd_uart1, buffer, 1024);
    if (ret > 0) {
      buffer[ret] = '\0';
      LOG_I("uart1_read fd:%d, data size:%d ,data:%s--", g_fd_uart1, ret, buffer);
      for (int i = 0; i < ret; ++i) {
        printf("0x%x ", buffer[i]);
      }
      printf("\n ");
    } else {
      LOG_I("uart1_read fd:%d, data size:%d", g_fd_uart1, ret);
    }

    // usleep(1000 * 200);
    break;
  }

  return 0;
}

int send_mic_ready() {
    uint16_t hello_flag = htons(MAIN_CONTROL_READY);
    int ret = uart_write(g_fd_uart1, (const uint8_t *)&hello_flag, sizeof(hello_flag));
    if (ret > 0) {
      LOG_I("uart1_write fd:%d, data size:%d ,data:0x%x--", g_fd_uart1, ret, hello_flag);
    } else {
      LOG_E("uart1_write fd:%d, data size:%d", g_fd_uart1, ret);
    }
  return 0;
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    LOGE("Invalid params");
    return -1;
  }
  LOG_I("main000: %s", argv[2]);

  mcu_control_start();
  g_running = true;


  // if (strcmp(argv[1], "-rs") ==0) {
      while (g_running) {
        send_mic_ready();
        read_mic_data();
        sleep(2);
      }
  // }

  g_running = false;

  mcu_control_stop();
  return 0;
}
