/*
 * pwm.c
 *
 *  Created on: 2021. 2. 14.
 *      Author: baram
 */


#include "pwm.h"
#include "cli.h"

#ifdef _USE_HW_PWM
#include "hardware/pwm.h"


static bool is_init = false;


typedef struct
{
  uint32_t slice_num;  
  uint32_t channel;
  uint16_t max;
  uint16_t data;
  uint32_t freq_khz;
} pwm_tbl_t;


pwm_tbl_t  pwm_tbl[HW_PWM_MAX_CH];


#ifdef _USE_HW_CLI
static void cliPwm(cli_args_t *args);
#endif

bool pwmInit(void)
{
  bool ret = true;
  pwm_tbl_t *p_pwm;



  p_pwm = &pwm_tbl[0];


  gpio_set_function(7, GPIO_FUNC_PWM);
  p_pwm->slice_num = pwm_gpio_to_slice_num(7);
  p_pwm->channel = PWM_CHAN_B;
  p_pwm->max = 255;
  p_pwm->freq_khz = 10;
  p_pwm->data = 0;

  pwm_set_clkdiv(p_pwm->slice_num, clock_get_hz(clk_sys)/(p_pwm->freq_khz * 1000 * p_pwm->max));

  pwm_set_wrap(p_pwm->slice_num, p_pwm->max);
  pwm_set_chan_level(p_pwm->slice_num, p_pwm->channel, p_pwm->data);
  pwm_set_enabled(p_pwm->slice_num, true);


  is_init = ret;


#ifdef _USE_HW_CLI
  cliAdd("pwm", cliPwm);
#endif
  return ret;
}

bool pwmIsInit(void)
{
  return is_init;
}

void pwmWrite(uint8_t ch, uint16_t pwm_data)
{
  if (ch >= PWM_MAX_CH) return;

  switch(ch)
  {
    case _DEF_PWM1:
      pwm_tbl[ch].data = constrain(pwm_data, 0, pwm_tbl[ch].max);
      pwm_set_chan_level(pwm_tbl[ch].slice_num, pwm_tbl[ch].channel, pwm_tbl[ch].data);
      break;
  }
}

uint16_t pwmRead(uint8_t ch)
{
  uint16_t ret = 0;

  if (ch >= HW_PWM_MAX_CH) return 0;


  switch(ch)
  {
    case _DEF_PWM1:
      ret = pwm_tbl[ch].data;
      break;
  }

  return ret;
}

uint16_t pwmGetMax(uint8_t ch)
{
  uint16_t ret = 255;

  if (ch >= HW_PWM_MAX_CH) return 255;


  switch(ch)
  {
    case _DEF_PWM1:
      ret = pwm_tbl[ch].max;
      break;
  }

  return ret;
}



#ifdef _USE_HW_CLI
void cliPwm(cli_args_t *args)
{
  bool ret = true;
  uint8_t  ch;
  uint32_t pwm;


  if (args->argc == 3)
  {
    ch  = (uint8_t)args->getData(1);
    pwm = (uint8_t)args->getData(2);

    ch = constrain(ch, 0, PWM_MAX_CH);

    if(args->isStr(0, "set"))
    {
      pwmWrite(ch, pwm);
      cliPrintf("pwm ch%d %d\n", ch, pwm);
    }
    else
    {
      ret = false;
    }
  }
  else if (args->argc == 2)
  {
    ch = (uint8_t)args->getData(1);

    if(args->isStr(0, "get"))
    {
      cliPrintf("pwm ch%d %d\n", ch, pwmRead(ch));
    }
    else
    {
      ret = false;
    }
  }
  else
  {
    ret = false;
  }


  if (ret == false)
  {
    cliPrintf( "pwm set 0~%d 0~%d \n", PWM_MAX_CH-1, pwm_tbl[0].max);
    cliPrintf( "pwm get 0~%d \n", PWM_MAX_CH-1);
  }

}
#endif

#endif