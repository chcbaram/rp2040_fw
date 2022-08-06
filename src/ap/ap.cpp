#include "ap.h"
#include "app/radio.h"




typedef struct
{
  image_t img[3];
  uint8_t menu_cnt;
  uint8_t menu_index;
  button_obj_t btn_left;    
  button_obj_t btn_right;    
  button_obj_t btn_enter;  
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
    menuSaveInfo();
  }
  if (buttonObjGetEvent(&menu.btn_right) & BUTTON_EVT_CLICKED)
  {
    buzzerBeep(50);
    menu.menu_index++;
    menu.menu_index %= menu.menu_cnt;
    menuSaveInfo();
  }

  if (buttonObjGetEvent(&menu.btn_enter) & BUTTON_EVT_CLICKED)
  {
    buzzerBeep(50);
    menuRunApp(menu.menu_index);
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
    cliPrintf("Flash FW Size  : %d KB\n", ((uint32_t)&__flash_binary_end - (uint32_t)&__flash_binary_start)/1024);

    ret = true;
  }


  if (ret != true)
  {
    cliPrintf("info flash\n");
  }
}