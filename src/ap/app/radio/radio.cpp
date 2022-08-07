#include "radio.h"
#include "radio_fm.h"


void radioSetMute(bool enable);
bool radioSetFreq(float freq_mhz);
float radioGetFreq(void);
void radioSaveInfo(void);
void radioLoadInfo(void);


static bool is_init = false;
static uint16_t fm_ch_index = 0;
static uint16_t fm_ch_count = 0;


bool radioInit(void)
{
  bool ret = false;


  fm_ch_count = sizeof(radio_fm_broad) / sizeof(radio_fm_broad_t);

  radioLoadInfo();

  is_init = tea5767IsInit();

  return ret;
}

void radioMain(void)
{
  bool is_mute = true;
  bool is_save = false;
  uint32_t pre_time;
  button_obj_t btn_exit;
  button_obj_t btn_mute;
  button_obj_t btn_left;
  button_obj_t btn_right;

  buttonObjCreate(&btn_left,  0, 50, 1000, 100);  
  buttonObjCreate(&btn_right, 1, 50, 1000, 100);  
  buttonObjCreate(&btn_exit,  3, 50, 1000, 100);      
  buttonObjCreate(&btn_mute,  2, 50, 1000, 100);      

  if (is_init == false)
  {
    lcdClearBuffer(black);
    lcdPrintf(0, 0, white, "Radio 하드웨어 없음");
    lcdRequestDraw();
    delay(2000);
    return;
  }


  radioSetMute(is_mute);
  radioSetFreq(radio_fm_broad[fm_ch_index].freq_mhz);
  while(1)
  {
    buttonObjClearAndUpdate(&btn_left);
    buttonObjClearAndUpdate(&btn_right);
    buttonObjClearAndUpdate(&btn_mute);
    buttonObjClearAndUpdate(&btn_exit);

    if (buttonObjGetEvent(&btn_exit) & BUTTON_EVT_CLICKED)
    {
      buzzerBeep(50);
      break;
    }
    if (buttonObjGetEvent(&btn_mute) & BUTTON_EVT_CLICKED)
    {
      buzzerBeep(50);
      is_mute = !is_mute;
      radioSetMute(is_mute);
    }
    if (buttonObjGetEvent(&btn_left) & BUTTON_EVT_CLICKED)
    {
      buzzerBeep(50);
      if (fm_ch_index > 0)
      {
        fm_ch_index--;
      }
      else
      {
        fm_ch_index = fm_ch_count - 1;
      }
      is_save = true;
      pre_time = millis();
      radioSetFreq(radio_fm_broad[fm_ch_index].freq_mhz);
    }

    if (buttonObjGetEvent(&btn_right) & BUTTON_EVT_CLICKED)
    {
      buzzerBeep(50);
      fm_ch_index++;
      fm_ch_index = (fm_ch_index % fm_ch_count);
      is_save = true;
      pre_time = millis();      
      radioSetFreq(radio_fm_broad[fm_ch_index].freq_mhz);
    }

    if (is_save == true && millis()-pre_time >= 100)
    {
      is_save = false;
      radioSaveInfo();
    }

    if (lcdDrawAvailable() > 0)
    {
      lcdClearBuffer(black);

      lcdPrintf(120, 0, white, "%d/%d", fm_ch_index+1, fm_ch_count);
      lcdPrintfResize(16, 24, white, 32, "%3.1fMhz", radioGetFreq());
      lcdPrintf(16, 64, white, "%s", radio_fm_broad[fm_ch_index].desc);
      
      if (is_mute == true)
      {
        lcdDrawFillRect(60, 0, 32,16, white);
        lcdPrintf(60, 0, red, "MUTE");
      }

      tea5767Update();
      lcdPrintf(0, 0, white, "LV %d", tea5767GetLevel());

      lcdRequestDraw();
    }    

    cliMain();
  }

  radioSetMute(true);
}

void radioSetMute(bool enable)
{
  if (enable == true)
  {
    gpioPinWrite(_PIN_GPIO_RADIO_SPK, 0);
    tea5767SetMute(true);
  }
  else
  {
    gpioPinWrite(_PIN_GPIO_RADIO_SPK, 1);
    tea5767SetMute(false);    
  }
}

bool radioSetFreq(float freq_mhz)
{
  bool ret;


  ret = tea5767SetFreq(freq_mhz);

  return ret;
}

float radioGetFreq(void)
{
  return tea5767GetFreq();
}

void radioSaveInfo(void)
{
  fs_t fs;

  if (fsFileOpen(&fs, "radio_ch_index") == true)
  {
    fsFileWrite(&fs, &fm_ch_index, sizeof(fm_ch_index));
    fsFileClose(&fs);
  }  
}

void radioLoadInfo(void)
{
  fs_t fs;

  if (fsFileOpen(&fs, "radio_ch_index") == true)
  {
    fsFileRead(&fs, &fm_ch_index, sizeof(fm_ch_index));
    fsFileClose(&fs);
  }
}