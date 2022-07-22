#include "rc100.h"
#include "cli.h"
#include "uart.h"



#ifdef _USE_HW_CLI
static void cliCmd(cli_args_t *args);
#endif



static bool is_init = false;
static uint16_t rx_data = 0;





bool rc100Init(void)
{
  is_init = true;
  rx_data = 0;

#ifdef _USE_HW_CLI
  cliAdd("rc100", cliCmd);
#endif
  return true;
}

bool rc100IsInit(void)
{
  return is_init;
}

bool rc100Update(uint8_t data)
{
  bool ret = false;
  static uint8_t save_data = 0;
  static uint8_t inv_data = 0;
  static uint32_t time_t;
  static uint8_t state = 0;


  inv_data = ~data;

  if (millis()-time_t > 100)
  {
    state = 0;
  }

  switch(state)
  {
    case 0:
      if (data == 0xFF)
      {
        state = 1;
        time_t = millis();
      }
      break;

    case 1:
      if (data == 0x55)
      {
        state    = 2;
        rx_data  = 0;
      }
      else
      {
        state = 0;
      }
      break;

    case 2:
      rx_data   = data;
      save_data = data;
      state  = 3;
      break;

    case 3:
      if (save_data == inv_data)
      {
        state = 4;
      }
      else
      {
        state = 0;
      }
      break;

    case 4:
      rx_data   |= data<<8;
      save_data  = data;
      state      = 5;
      break;

    case 5:
      if (save_data == inv_data)
      {
        ret = true;
      }
      state = 0;
      break;

    default:
      state = 0;
      break;
  }

  return ret;
}

uint16_t rc100GetData(void)
{
  return rx_data;
}

#ifdef _USE_HW_CLI
void cliCmd(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 2 && args->isStr(0, "info"))
  {
    uint8_t ch;
    bool is_received;

    ch = (uint8_t)args->getData(1);
    ch = constrain(ch, 1, UART_MAX_CH);
    ch -= 1;

    uartOpen(ch, 57600);

    while(cliKeepLoop())
    {
      is_received = false;
      if (uartAvailable(ch) > 0)
      {
        is_received = rc100Update(uartRead(ch));
      }

      if (is_received == true)
      {
        for (int i=0; i<RC100_BTN_MAX; i++)
        {
          if (rc100GetData() & (1<<i))
          {
            cliPrintf("1");
          }
          else
          {
            cliPrintf("0");
          }
        }
        cliPrintf("\n");
      }

      delay(5);
    }

    ret = true;
  }


  if (ret == false)
  {
    cliPrintf( "rc100 info 1~%d\n", UART_MAX_CH);
  }
}
#endif
