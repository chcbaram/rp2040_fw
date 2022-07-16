#include "ws2812.h"
#include "cli.h"

#ifdef _USE_HW_WS2812
#include "ws2812.pio.h"


typedef struct 
{
  uint8_t  max_cnt;
  ws_color_t rgb[WS2812_MAX_CH];
} ws2812_t;


static ws2812_t ws2812;
static PIO pio = pio0;
static const uint32_t ws2812_gpio = 10;


#ifdef _USE_HW_CLI
static void cliCmd(cli_args_t *args);
#endif





bool ws2812Init(void)
{
  ws2812.max_cnt = WS2812_MAX_CH; 
  for (int i=0; i<WS2812_MAX_CH; i++)
  {
    ws2812.rgb[i].data = 0;
  }


  uint offset = pio_add_program(pio, &ws2812_program);
  ws2812_program_init(pio, 0, offset, ws2812_gpio, 800000, false);

  ws2812Load();

  #ifdef _USE_HW_CLI
  cliAdd("ws2812", cliCmd);
  #endif

  return true;
}

bool ws2812SetColor(uint8_t ch, uint32_t rgb)
{
  if (ch >= WS2812_MAX_CH) return false;

  ws2812.rgb[ch].data = rgb;

  return true;
}

bool ws2812Load(void)
{
  for (int i=0; i<ws2812.max_cnt; i++)
  {
    pio_sm_put_blocking(pio, 0, ws2812.rgb[i].data << 8);
  }

  return true;
}


#ifdef _USE_HW_CLI
void cliCmd(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 5 && args->isStr(0, "set"))
  {
    uint8_t ch;
    ws_color_t rgb;

    ch = constrain(args->getData(1), 0, WS2812_MAX_CH-1);
    rgb.rgb.r = args->getData(2);
    rgb.rgb.g = args->getData(3);
    rgb.rgb.b = args->getData(4);

    ws2812SetColor(ch, rgb.data);
    ws2812Load();    
    delay(1000);

    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "test1"))
  {
    uint8_t ch;
    ws_color_t rgb[3];

    ch = constrain(args->getData(1), 0, WS2812_MAX_CH-1);
    memset(&rgb, 0, sizeof(rgb));

    rgb[0].rgb.r = 255;
    rgb[1].rgb.g = 255;
    rgb[2].rgb.b = 255;

    for (int i=0; i<3; i++)
    {
      ws2812SetColor(ch, rgb[i].data);
      ws2812Load();
      delay(500);
    }
    ws2812SetColor(ch, 0);
    ws2812Load();

    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "test2"))
  {
    uint8_t ch;
    ws_color_t rgb;

    ch = constrain(args->getData(1), 0, WS2812_MAX_CH-1);
    memset(&rgb, 0, sizeof(rgb));

    
    for (int index=0; index<3; index++)
    {
      uint8_t *p_color[3] = {&rgb.rgb.r, &rgb.rgb.g, &rgb.rgb.b};

      for (int i=0; i<=255; i++)
      {
        *p_color[index] = i;
        ws2812SetColor(ch, rgb.data);
        ws2812Load();
        delay(2);
      }
      for (int i=0; i<=255; i++)
      {
        *p_color[index] = 255 - i;
        ws2812SetColor(ch, rgb.data);
        ws2812Load();
        delay(2);
      }
      ws2812SetColor(ch, 0);
      ws2812Load();
    }

    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("ws2812 set CH[0~%d] R[0~255] G[0~255] B[0~255]\n", WS2812_MAX_CH-1);
    cliPrintf("ws2812 test1 CH[0~%d]\n", WS2812_MAX_CH-1);
    cliPrintf("ws2812 test2 CH[0~%d]\n", WS2812_MAX_CH-1);
  }
}
#endif

#endif