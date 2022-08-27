#include "ap.h"
#include "app/radio/radio.h"
#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"

#include <math.h>
#include "pico/audio_i2s.h"
#include "hardware/dma.h"


#define SINE_WAVE_TABLE_LEN 2048
#define SAMPLES_PER_BUFFER 256

static int16_t sine_wave_table[SINE_WAVE_TABLE_LEN];

struct audio_buffer_pool *init_audio() {

    static audio_format_t audio_format = {
            .sample_freq = 16000,
            .format = AUDIO_BUFFER_FORMAT_PCM_S16,
            .channel_count = 2,
    };

    static struct audio_buffer_format producer_format = {
            .format = &audio_format,
            .sample_stride = 4
    };

    cliPrintf("i1\n");
    struct audio_buffer_pool *producer_pool = audio_new_producer_pool(&producer_format, 3,
                                                                      SAMPLES_PER_BUFFER); // todo correct size
    bool __unused ok;
    const struct audio_format *output_format;

    struct audio_i2s_config config = {
            .data_pin = 19,
            .clock_pin_base = 16,
            .dma_channel = 2,
            .pio_sm = 0,
    };    

    cliPrintf("i2\n");
    output_format = audio_i2s_setup(&audio_format, &config);
    if (!output_format) 
    {
      //panic("PicoAudio: Unable to open audio device.\n");
    }
    cliPrintf("i3\n");
    ok = audio_i2s_connect(producer_pool);
    cliPrintf("i4\n");
    assert(ok);
    audio_i2s_set_enabled(true);

    return producer_pool;
}




typedef struct
{
  image_t img[3];
  uint8_t menu_cnt;
  uint8_t menu_index;
  button_obj_t btn_left;    
  button_obj_t btn_right;    
  button_obj_t btn_enter;  

  bool is_save;
  uint32_t pre_time;
} menu_t;


void cliInfo(cli_args_t *args);
void menuInit(void);
void menuUpdate(void);
void menuSaveInfo(void);
void menuLoadInfo(void);
void menuRunApp(uint8_t index);


LVGL_IMG_DEF(menu_img);
menu_t menu;





void apInit(void)
{
  cliOpen(_DEF_UART1, 115200);

  cliAdd("info", cliInfo);  

  menuInit();
}

void apMain(void)
{
  uint32_t pre_time;
  bool is_menu = false;


  while(1)
  {
    if (millis()-pre_time >= 500)
    {
      pre_time = millis();
      ledToggle(_DEF_LED1);      
    }
    if (buttonGetData() > 0 && lcdLogoIsOn() == true)
    {
      buzzerBeep(100);
      lcdLogoOff();
      is_menu = true;
    }

    if (is_menu == true)
    {
      menuUpdate();
    }
    cliMain();            
  }
}

void menuInit(void)
{
  menu.menu_cnt = 3;
  menu.menu_index = 0;
  buttonObjCreate(&menu.btn_left,  0, 50, 1000, 100);      
  buttonObjCreate(&menu.btn_right, 1, 50, 1000, 100);      
  buttonObjCreate(&menu.btn_enter, 3, 50, 1000, 100);      

  menu.img[0] = lcdCreateImage(&menu_img,  0, 0, 64, 80);
  menu.img[1] = lcdCreateImage(&menu_img, 64, 0, 64, 80);
  menu.img[2] = lcdCreateImage(&menu_img,128, 0, 64, 80);

  menu.is_save = false;
  buzzerSetVolume(1);
  menuLoadInfo();
}

void menuUpdate(void)
{
  buttonObjClearAndUpdate(&menu.btn_left);
  buttonObjClearAndUpdate(&menu.btn_right);
  buttonObjClearAndUpdate(&menu.btn_enter);


  if (buttonObjGetEvent(&menu.btn_left) & BUTTON_EVT_CLICKED)
  {
    buzzerBeep(50);
    if (menu.menu_index > 0)
    {
      menu.menu_index--;
    } 
    else
    {
      menu.menu_index = menu.menu_cnt - 1;
    }
    menu.is_save = true;
    menu.pre_time = millis();    
  }
  if (buttonObjGetEvent(&menu.btn_right) & BUTTON_EVT_CLICKED)
  {
    buzzerBeep(50);
    menu.menu_index++;
    menu.menu_index %= menu.menu_cnt;

    menu.is_save = true;
    menu.pre_time = millis();    
  }

  if (buttonObjGetEvent(&menu.btn_enter) & BUTTON_EVT_CLICKED)
  {
    buzzerBeep(50);
    menuRunApp(menu.menu_index);
  }

  if (menu.is_save == true && millis()-menu.pre_time >= 100)
  {
    menu.is_save = false;
    menuSaveInfo();
  }


  if (lcdDrawAvailable() > 0)
  {
    int16_t x;


    lcdClearBuffer(black);
    for (int i=0; i<menu.menu_cnt; i++)
    {
      x = (lcdGetWidth()/2) - (64/2);

      if (menu.menu_index > 0)
      {
        x -= (70*menu.menu_index);  
      }
      lcdDrawImage(&menu.img[i], x + i*70, 0);

      if (menu.menu_index == i)
      {
        lcdDrawRoundRect(x + i*70-4+0, 0, 64+8-0, 80-0, 5, blue);
        lcdDrawRoundRect(x + i*70-4+1, 1, 64+8-2, 80-2, 5, blue);
      }
    }

    lcdRequestDraw();
  }
}

void menuSaveInfo(void)
{
  fs_t fs;

  if (fsFileOpen(&fs, "menu_index") == true)
  {
    fsFileWrite(&fs, &menu.menu_index, sizeof(menu.menu_index));
    fsFileClose(&fs);
  }  
}

void menuLoadInfo(void)
{
  fs_t fs;

  if (fsFileOpen(&fs, "menu_index") == true)
  {
    fsFileRead(&fs, &menu.menu_index, sizeof(menu.menu_index));
    fsFileClose(&fs);
  }
}

void menuRunApp(uint8_t index)
{
  bool is_run = true;

  switch(index)
  {
    case 0:
      lcdUpdateDraw();
      radioInit();
      radioMain();
      break;

    default:
      is_run = false;
      break;
  }

  if (is_run == true)
  {
    buttonObjInit(&menu.btn_left);
    buttonObjInit(&menu.btn_right);
    buttonObjInit(&menu.btn_enter);      
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
    cliPrintf("Flash FW Size  : %d Bytes\n", ((uint32_t)&__flash_binary_end - (uint32_t)&__flash_binary_start));

    ret = true;
  }

  if (args->argc == 1 && args->isStr(0, "i2s"))
  {
    for (int i = 0; i < SINE_WAVE_TABLE_LEN; i++) 
    {
        sine_wave_table[i] = 32767 * cosf(i * 2 * (float) (M_PI / SINE_WAVE_TABLE_LEN));
    }

    cliPrintf("1\n");
    struct audio_buffer_pool *ap = init_audio();
    uint32_t step = 0x100000;
    uint32_t pos = 0;
    uint32_t pos_max = 0x10000 * SINE_WAVE_TABLE_LEN;
    uint vol = 128;

    cliPrintf("2\n");
    while (cliKeepLoop()) 
    {

      cliPrintf("3\n");  
      struct audio_buffer *buffer = take_audio_buffer(ap, true);
      cliPrintf("4\n");
      int16_t *samples = (int16_t *) buffer->buffer->bytes;
      for (uint i = 0; i < buffer->max_sample_count; i++) 
      {
          samples[i] = (vol * sine_wave_table[pos >> 16u]) >> 8u;
          pos += step;
          if (pos >= pos_max) pos -= pos_max;
      }
      buffer->sample_count = buffer->max_sample_count;
      give_audio_buffer(ap, buffer);
    }
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("info flash\n");
    cliPrintf("info i2s\n");
  }
}