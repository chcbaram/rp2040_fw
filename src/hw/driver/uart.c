/*
 * uart.c
 *
 *  Created on: 2021. 7. 31.
 *      Author: baram
 */


#include "uart.h"
#include "cli.h"
#include "qbuffer.h"
#include "tusb.h"
#include "pico/stdio/driver.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"


#ifdef _USE_HW_UART



#define UART_RX_BUF_LENGTH      1024


typedef enum
{
  UART_HW_TYPE_MCU,
  UART_HW_TYPE_USB,
} UartHwType_t;


typedef struct
{
  bool     is_open;
  uint32_t baud;
  UartHwType_t type;


  uint8_t  rx_buf[UART_RX_BUF_LENGTH];
  qbuffer_t qbuffer;

  uart_inst_t *p_uart;

} uart_tbl_t;


static void cliUart(cli_args_t *args);


static uart_tbl_t uart_tbl[UART_MAX_CH];
extern stdio_driver_t stdio_usb;





bool uartInit(void)
{
  for (int i=0; i<UART_MAX_CH; i++)
  {
    uart_tbl[i].is_open = false;
    uart_tbl[i].baud = 57600;
  }

  cliAdd("uart", cliUart);
  return true;
}

bool uartOpen(uint8_t ch, uint32_t baud)
{
  bool ret = false;


  switch(ch)
  {
    case _DEF_UART1:
      uart_tbl[ch].type    = UART_HW_TYPE_USB;
      uart_tbl[ch].baud    = baud;      
      uart_tbl[ch].is_open = true;

      ret = true;
      break;

    case _DEF_UART2:

      uart_tbl[ch].is_open = true;
      uart_tbl[ch].baud    = baud;
      uart_tbl[ch].p_uart  = uart0;

      uart_init(uart_tbl[ch].p_uart, baud);

      gpio_set_function(0, GPIO_FUNC_UART); // TX
      gpio_set_function(1, GPIO_FUNC_UART); // RX


      // Set UART flow control CTS/RTS, we don't want these, so turn them off
      uart_set_hw_flow(uart_tbl[ch].p_uart, false, false);

      // Set our data format
      uart_set_format(uart_tbl[ch].p_uart, 8, 1, UART_PARITY_NONE);

      // Turn On FIFO      
      uart_set_fifo_enabled(uart_tbl[ch].p_uart, true);

      uartFlush(ch);
      
      ret = true;
      break;      

    case _DEF_UART3:

      uart_tbl[ch].is_open = true;
      uart_tbl[ch].baud    = baud;
      uart_tbl[ch].p_uart  = uart1;

      uart_init(uart_tbl[ch].p_uart, baud);

      gpio_set_function(8, GPIO_FUNC_UART); // TX
      gpio_set_function(9, GPIO_FUNC_UART); // RX
      gpio_pull_up(9);  

      // Set UART flow control CTS/RTS, we don't want these, so turn them off
      uart_set_hw_flow(uart_tbl[ch].p_uart, false, false);

      // Set our data format
      uart_set_format(uart_tbl[ch].p_uart, 8, 1, UART_PARITY_NONE);

      // Turn On FIFO      
      uart_set_fifo_enabled(uart_tbl[ch].p_uart, true);

      uartFlush(ch);
      
      ret = true;
      break;         
  }

  return ret;
}

bool uartIsOpen(uint8_t ch)
{
  if (ch >= UART_MAX_CH) return false;

  return uart_tbl[ch].is_open;
}

bool uartClose(uint8_t ch)
{
  return true;
}

uint32_t uartAvailable(uint8_t ch)
{
  uint32_t ret = 0;

  switch(ch)
  {
    case _DEF_UART1:
      ret = tud_cdc_available();
      break;

    case _DEF_UART2:
    case _DEF_UART3:
      ret = uart_is_readable(uart_tbl[ch].p_uart);
      break;      
  }

  return ret;
}

bool uartFlush(uint8_t ch)
{
  uint32_t pre_time;

  pre_time = millis();
  while(uartAvailable(ch))
  {
    if (millis()-pre_time >= 10)
    {
      break;
    }
    uartRead(ch);
  }

  return true;
}

uint8_t uartRead(uint8_t ch)
{
  uint8_t ret = 0;

  switch(ch)
  {
    case _DEF_UART1:
      stdio_usb.in_chars((char *)&ret, 1);
      break;

    case _DEF_UART2:
    case _DEF_UART3:
      ret = uart_getc(uart_tbl[ch].p_uart);
      break;            
  }

  return ret;
}

uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t length)
{
  uint32_t ret = 0;
  uint32_t pre_time;
  uint32_t tx_index;

  switch(ch)
  {
    case _DEF_UART1:
      stdio_usb.out_chars((char *)p_data, length);
      break;     

    case _DEF_UART2:
    case _DEF_UART3:
      tx_index = 0;
      pre_time = millis();
      while(millis()-pre_time < 100)
      {
        if (uart_is_writable(uart_tbl[ch].p_uart))
        {
          uart_get_hw(uart_tbl[ch].p_uart)->dr = p_data[tx_index];
          tx_index++;
          if (tx_index >= length)
          {
            ret = length;
            break;
          }
        }
      }  
      break;    
  }

  return ret;
}

uint32_t uartPrintf(uint8_t ch, const char *fmt, ...)
{
  char buf[256];
  va_list args;
  int len;
  uint32_t ret;

  va_start(args, fmt);
  len = vsnprintf(buf, 256, fmt, args);

  ret = uartWrite(ch, (uint8_t *)buf, len);

  va_end(args);


  return ret;
}

uint32_t uartGetBaud(uint8_t ch)
{
  uint32_t ret;

  ret = uart_tbl[ch].baud;

  return ret;
}

void cliUart(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info"))
  {
    for (int i=0; i<UART_MAX_CH; i++)
    {
      if (uartIsOpen(i) == true)
      {
        cliPrintf("_DEF_UART%d : %d bps ", i + 1, uartGetBaud(i));
        if (cliGetPort() == i)
        {
          cliPrintf("CLI\n");
        }
        else
        {
          cliPrintf("\n");
        }
      }
      else
      {
        cliPrintf("_DEF_UART%d : closed\n", i + 1);
      }
    }
    ret = true;
  }
  
  if (args->argc == 3 && args->isStr(0, "open"))
  {
    uint8_t ch;
    uint32_t baud;

    ch = constrain((uint8_t)args->getData(1), 1, UART_MAX_CH);
    ch = ch - 1;
    baud = args->getData(2);

    if (ch != cliGetPort())
    {      
      ret = uartOpen(ch, baud);
      cliPrintf("uart open : %d, %d bps\n", ret, baud);
    }
    else
    {
      cliPrintf("This is CLI\n");
    }
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "test"))
  {
    uint8_t ch;

    ch = constrain((uint8_t)args->getData(1), 1, UART_MAX_CH);
    ch = ch - 1;

    if (ch != cliGetPort())
    { 
      if (uartIsOpen(ch) == true)
      {
        cliPrintf("Press q to exit\n");     
        while(1)
        {
          uint8_t rx_data;

          if (uartAvailable(cliGetPort()) > 0)
          {
            rx_data = uartRead(cliGetPort());
            if (rx_data == 'q')
            {
              break;
            }
            uartPrintf(ch, "Send : 0x%X from UART%d\n", rx_data, cliGetPort()+1);
          }

          if (uartAvailable(ch) > 0)
          {
            rx_data = uartRead(ch);
            uartPrintf(cliGetPort(), "Send : 0x%X from UART%d\n", rx_data, ch+1);
          }          
        }
      }
      else
      {  
        cliPrintf("uart is not open\n");
      }
    }
    else
    {
      cliPrintf("This is CLI\n");
    }
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("uart info\n");
    cliPrintf("uart open 1~%d [baud]\n", UART_MAX_CH);
    cliPrintf("uart test 1~%d\n", UART_MAX_CH);
  }
}


#endif