#include "buzzer.h"
#include "cli.h"
#include "hardware/pwm.h"


static bool is_on = false;
static uint32_t on_time_cnt = 0;
static uint32_t slice_num;
static const uint32_t buzzer_gpio = 18;
static uint8_t buzzer_volume = 100;

#ifdef _USE_HW_CLI
static void cliBuzzer(cli_args_t *args);
#endif


void buzzerISR(void *arg)
{
  if (is_on && on_time_cnt > 0)
  {
    on_time_cnt--;

    if (on_time_cnt == 0)
    {
      buzzerOff();
    }
  }
}




bool buzzerInit(void)
{
  gpio_set_function(buzzer_gpio, GPIO_FUNC_PWM);
  slice_num = pwm_gpio_to_slice_num(buzzer_gpio);

  pwm_set_clkdiv(slice_num, clock_get_hz(clk_sys)/1000000);

  pwm_set_wrap(slice_num, 1000);
  pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);
  pwm_set_enabled(slice_num, true);


#ifdef _USE_HW_CLI
  cliAdd("buzzer", cliBuzzer);
#endif  
  return true;
}

void buzzerOn(uint16_t freq_hz, uint16_t time_ms)
{
  uint32_t pwm_top;
  uint32_t pwm_duty;


  if (freq_hz == 0) return;

  pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);

  pwm_top = 1000000/freq_hz;
  pwm_set_wrap(slice_num, pwm_top);
  
  pwm_duty = cmap(buzzer_volume, 0, 100, 0, (pwm_top/2));

  pwm_set_chan_level(slice_num, PWM_CHAN_A, pwm_duty);

  is_on = true;
  on_time_cnt = time_ms;
}

void buzzerBeep(uint16_t time_ms)
{
  buzzerOn(1000, time_ms);
}

void buzzerOff(void)
{  
  pwm_set_chan_level(slice_num, PWM_CHAN_A, 0);
  is_on = false;
}

void buzzerSetVolume(uint8_t volume)
{
  buzzer_volume = constrain(volume, 0, 100);
}

uint8_t buzzerGetVolume(void)
{
  return buzzer_volume;
}

#ifdef _USE_HW_CLI
void cliBuzzer(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 3 && args->isStr(0, "on"))
  {
    uint16_t freq_hz;
    uint16_t time_ms;

    freq_hz = args->getData(1);
    time_ms = args->getData(2);

    buzzerOn(freq_hz, time_ms);
    ret = true;
  }
  
  if (args->argc == 1 && args->isStr(0, "off"))
  {
    buzzerOff();
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "test"))
  {
    uint16_t freq_tbl[8] = {261, 293, 329, 349, 391, 440, 493, 523};
    uint8_t freq_i;

    for (int i=0; i<16; i++)
    {
      if (i/8 == 0)
      {
        freq_i = i%8;
      }
      else
      {
        freq_i = 7 - (i%8);
      }
      buzzerOn(freq_tbl[freq_i], 150);
      cliPrintf("%dHz, %dms\n", freq_tbl[freq_i], 100);
      delay(300);
    }
    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "melody"))
  {
    uint16_t melody[] = {NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4};
    int note_durations[] = { 4, 8, 8, 4, 4, 4, 4, 4 };

    for (int i=0; i<8; i++) 
    {
      int note_duration = 1000 / note_durations[i];

      buzzerOn(melody[i], note_duration);
      delay(note_duration * 1.30);    
    }
    ret = true;
  }
  if (ret != true)
  {
    cliPrintf("buzzer on freq(32~500000) time_ms\n");
    cliPrintf("buzzer off\n");
    cliPrintf("buzzer test\n");
    cliPrintf("buzzer melody\n");
  }
}
#endif