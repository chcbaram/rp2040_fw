#include "ap.h"
#include "blink.pio.h"
#include "pwm.pio.h"


void cliInfo(cli_args_t *args);





void apInit(void)
{
  cliOpen(_DEF_UART1, 115200);

  cliAdd("info", cliInfo);  

   // todo get free sm
  PIO pio = pio0;
  uint offset = pio_add_program(pio, &pwm_program);
  uint8_t pwm_out = 255;

  pwm_program_init(pio, 0, offset, 25, 1000000);
  pio_sm_set_enabled(pio, 0, true);  
  pio_sm_put_blocking(pio, 0, (pwm_out) + ((255-pwm_out)<<8));
}

void apMain(void)
{
  uint32_t pre_time;
  uint8_t pwm_out = 0;

  while(1)
  {
    if (millis()-pre_time >= 10)
    {
      pre_time = millis();
      //ledToggle(_DEF_LED1);
      pwm_out = (pwm_out + 1) % 256;
      pio_sm_put_blocking(pio0, 0, (pwm_out) + ((255-pwm_out)<<8));
    }

    cliMain();
  }
}

void cliInfo(cli_args_t *args)
{
  bool ret = false;

  if (args->argc == 1 && args->isStr(0, "flash"))
  {
    extern const uint32_t __flash_binary_start;
    extern const uint32_t __flash_binary_end;

    cliPrintf("Flash FW Start : 0x%X\n", &__flash_binary_start);
    cliPrintf("Flash FW End   : 0x%X\n", &__flash_binary_end);
    cliPrintf("Flash FW Size  : %d KB\n", ((uint32_t)&__flash_binary_end - (uint32_t)&__flash_binary_start)/1024);

    ret = true;
  }


  if (ret != true)
  {
    cliPrintf("info flash\n");
  }
}