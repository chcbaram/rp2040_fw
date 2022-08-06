#include "tea5767.h"
#include "i2c.h"
#include "cli.h"

#ifdef _USE_HW_TEA5767

typedef struct
{
  // D1~2  
  uint16_t MUTE : 1; 
  uint16_t SM   : 1;  // Serch mode
  uint16_t PLL  : 14; 

  // D3
  uint8_t  SUD  : 1;  // Serch Up/Down
  uint8_t  SSL  : 2;  // Search Stop Level
  uint8_t  HLSI : 1;  // High/Low Size Injection
  uint8_t  MS   : 1;  // Mono to Stereo
  uint8_t  MR   : 1;  // Mute Right
  uint8_t  ML   : 1;  // Mute Left
  uint8_t  SWP1 : 1;  // Software programmable port 1

  // D4
  uint8_t  SWP2 : 1;  // Software programmable port 1
  uint8_t  STBY : 1;  // Standby
  uint8_t  BL   : 1;  // Band Limit
  uint8_t  XTAL : 1;
  uint8_t  SMUTE : 1; // Soft Mute
  uint8_t  HCC  : 1;  // High Cut Control
  uint8_t  SNC  : 1;  // Stero Noise Cancelling
  uint8_t  SI   : 1;  // Search Indicatior

  // D5
  uint8_t  PLLREF : 1;
  uint8_t  DTC   : 1;

} tea5767_reg_write_t;

typedef struct
{
  // D1~2  
  uint16_t RF   : 1;  // Ready Flag 
  uint16_t BLF  : 1;  // Band Limit Flag
  uint16_t PLL  : 14; 

  // D3
  uint8_t  STEREO : 1;// Stereo indication
  uint8_t  IF   : 7;  // IF counter result

  // D4
  uint8_t  LEV  : 4;  // level ADC output
  uint8_t  CI   : 3;  // Chip Identification

} tea5767_reg_read_t;


typedef struct
{
  tea5767_reg_write_t reg_w;
  tea5767_reg_read_t  reg_r;
} tea5767_reg_t;


static bool tea5767InitSetup(void);
static bool tea5767WriteReg(tea5767_reg_write_t *p_reg);
static bool tea5767ReadReg(tea5767_reg_read_t *p_reg);

#ifdef _USE_HW_CLI
static void cliCmd(cli_args_t *args);
#endif

static const uint8_t i2c_ch   = _DEF_I2C1;
static const uint8_t i2c_addr = 0x60;
static bool is_init = false;
static tea5767_reg_t regs;





bool tea5767Init(void)
{
  bool ret;


  ret = i2cBegin(i2c_ch, 400);
  if (ret == true && i2cIsDeviceReady(i2c_ch, i2c_addr) == true)
  {
    logPrintf("tea5767 Init \t\t: OK\n");
    ret = true;
    is_init = true;
    tea5767InitSetup();
  }
  else
  {
    logPrintf("tea5767 Init \t\t: Fail\n");
    ret = false;
  }

#ifdef _USE_HW_CLI
  cliAdd("tea5767", cliCmd);
#endif

  return ret;
}

bool tea5767IsInit(void)
{
  return is_init;
}

bool tea5767InitSetup(void)
{
  bool ret;

  regs.reg_w.MUTE = 0;
  regs.reg_w.SM = 0;
  regs.reg_w.PLL = 0;
  regs.reg_w.SUD = 1;
  regs.reg_w.SSL = 1;
  regs.reg_w.HLSI = 1;
  regs.reg_w.MS = 1;
  regs.reg_w.MR = 1;
  regs.reg_w.ML = 1;
  regs.reg_w.SWP1 = 0;
  regs.reg_w.SWP2 = 0;
  regs.reg_w.STBY = 0;
  regs.reg_w.BL = 0;
  regs.reg_w.XTAL = 1;
  regs.reg_w.SMUTE = 1;
  regs.reg_w.HCC = 0;
  regs.reg_w.SNC = 0;
  regs.reg_w.SI = 0;
  regs.reg_w.PLLREF = 0;
  regs.reg_w.DTC = 0;

  ret = tea5767WriteReg(&regs.reg_w);

  return ret;
}

void tea5767SetMute(bool enable)
{
  if (enable == true)
  {    
    regs.reg_w.SMUTE = 1;
    regs.reg_w.MR = 1;
    regs.reg_w.ML = 1;
  }
  else
  {
    regs.reg_w.SMUTE = 1;
    regs.reg_w.MR = 0;
    regs.reg_w.ML = 0;
  }
  tea5767WriteReg(&regs.reg_w);
}

bool tea5767GetMute(void)
{
  return regs.reg_w.SMUTE;
}

bool tea5767SetFreq(float freq_mhz)
{
  bool ret;
  uint32_t frequency;

  frequency = 4 * (freq_mhz * 1000000 + 225000) / 32768; 

  regs.reg_w.PLL = frequency;
  ret = tea5767WriteReg(&regs.reg_w);

  return ret;
}

float tea5767GetFreq(void)
{
  float freq_mhz;
  uint32_t freq_pll;

  
  tea5767ReadReg(&regs.reg_r);

  freq_pll = regs.reg_r.PLL;

  freq_mhz = ((float)freq_pll * 32768.f / 4.f - 225000.f)/1000000.f; 

  return freq_mhz;
}

bool tea5767WriteReg(tea5767_reg_write_t *p_reg)
{
  uint8_t buf[5];
  bool ret;

  if (is_init == false) return false;

  memset(buf, 0, 5);

  buf[0] |= (p_reg->MUTE << 7);
  buf[0] |= (p_reg->SM << 6);
  buf[0] |= (p_reg->PLL>>8) & 0x3F;

  buf[1] |= (p_reg->PLL>>0) & 0xFF;

  buf[2] |= (p_reg->SUD  << 7);
  buf[2] |= (p_reg->SSL  << 5);
  buf[2] |= (p_reg->HLSI << 4);
  buf[2] |= (p_reg->MS   << 3);
  buf[2] |= (p_reg->MR   << 2);
  buf[2] |= (p_reg->ML   << 1);
  buf[2] |= (p_reg->SWP1 << 0);

  buf[3] |= (p_reg->SWP2 << 7);
  buf[3] |= (p_reg->STBY << 6);
  buf[3] |= (p_reg->BL   << 5);
  buf[3] |= (p_reg->XTAL << 4);
  buf[3] |= (p_reg->SMUTE<< 3);
  buf[3] |= (p_reg->HCC  << 2);
  buf[3] |= (p_reg->SNC  << 1);
  buf[3] |= (p_reg->SI   << 0);

  buf[4] |= (p_reg->PLLREF << 7);
  buf[4] |= (p_reg->DTC    << 6);

  ret = i2cWriteData(i2c_ch, i2c_addr, buf, 5, 100);

  return ret;
}

void tea5767WriteRegInfo(tea5767_reg_write_t *p_reg)
{
  cliPrintf("MUTE \t: %d\n", p_reg->MUTE);
  cliPrintf("SM \t: %d\n", p_reg->SM);
  cliPrintf("PLL \t: %d\n", p_reg->PLL);
  cliPrintf("SUD \t: %d\n", p_reg->SUD);
  cliPrintf("SSL \t: %d\n", p_reg->SSL);
  cliPrintf("HLSI \t: %d\n", p_reg->HLSI);
  cliPrintf("MS \t: %d\n", p_reg->MS);
  cliPrintf("MR \t: %d\n", p_reg->MR);
  cliPrintf("ML \t: %d\n", p_reg->ML);
  cliPrintf("SWP1 \t: %d\n", p_reg->SWP1);
  cliPrintf("SWP2 \t: %d\n", p_reg->SWP2);
  cliPrintf("STBY \t: %d\n", p_reg->STBY);
  cliPrintf("BL \t: %d\n", p_reg->BL);
  cliPrintf("XTAL \t: %d\n", p_reg->XTAL);
  cliPrintf("SMUTE \t: %d\n", p_reg->SMUTE);
  cliPrintf("HCC \t: %d\n", p_reg->HCC);
  cliPrintf("SNC \t: %d\n", p_reg->SNC);
  cliPrintf("SI \t: %d\n", p_reg->SI);
  cliPrintf("PLLREF \t: %d\n", p_reg->PLLREF);
  cliPrintf("DTC \t: %d\n", p_reg->DTC);
}

bool tea5767ReadReg(tea5767_reg_read_t *p_reg)
{
  uint8_t buf[5];
  bool ret;

  if (is_init == false) return false;

  ret = i2cReadData(i2c_ch, i2c_addr, buf, 5, 100);
  if (ret == true)
  {
    memset(p_reg, 0, sizeof(tea5767_reg_read_t));

    p_reg->RF  = (buf[0]>>7) & 0x01;
    p_reg->BLF = (buf[0]>>6) & 0x01;
    p_reg->PLL = ((buf[0] & 0x3F)<<8) | (buf[1]);
    p_reg->STEREO = (buf[2]>>7) & 0x01;
    p_reg->IF  = (buf[2] & 0x7F);
    p_reg->LEV = (buf[3]>>4) & 0x0F;
    p_reg->CI  = (buf[3]>>1) & 0x07;     
  }

  return ret;
}

void tea5767ReadRegInfo(tea5767_reg_read_t *p_reg)
{
  cliPrintf("RF \t: %d\n", p_reg->RF);
  cliPrintf("BLF \t: %d\n", p_reg->BLF);
  cliPrintf("PLL \t: %d\n", p_reg->PLL);
  cliPrintf("STEREO \t: %d\n", p_reg->STEREO);
  cliPrintf("LEV \t: %d\n", p_reg->LEV);
  cliPrintf("CI \t: %d\n", p_reg->CI);
}

bool tea5767Update(void)
{
  return tea5767ReadReg(&regs.reg_r);
}

uint8_t tea5767GetLevel(void)
{
  return regs.reg_r.LEV;
}

#ifdef _USE_HW_CLI
void cliCmd(cli_args_t *args)
{
  bool ret = false;


  if (args->argc == 1 && args->isStr(0, "info") == true)
  {
    cliPrintf("is_init : %d\n", is_init);
    if (is_init == true)
    {
      cliPrintf("Read Reg Info\n");
      tea5767ReadReg(&regs.reg_r);
      tea5767ReadRegInfo(&regs.reg_r);      

      cliPrintf("Write Reg Info\n");
      tea5767WriteRegInfo(&regs.reg_w);
    }
    ret = true;
  }

  if (args->argc == 2 && args->isStr(0, "mute") == true)
  {    
    if (args->isStr(1, "on") == true)
    {
      cliPrintf("mute on\n");
      tea5767SetMute(true);
    }
    else
    {
      cliPrintf("mute off\n");
      tea5767SetMute(false);
    }
    ret = true;
  }

  if (args->argc >= 1 && args->isStr(0, "freq") == true)
  {    
    if (args->argc == 1)
    {
      float freq;

      freq = tea5767GetFreq();
      cliPrintf("freq : %3.2f\n", freq);
    }
    else
    {
      float freq;

      freq = args->getFloat(1);
      cliPrintf("freq : %3.2f\n", freq);
      tea5767SetFreq(freq);
    }
    ret = true;
  }

  if (ret != true)
  {
    cliPrintf("tea5767 info\n");
    cliPrintf("tea5767 mute on|off\n");
    cliPrintf("tea5767 freq\n");
    cliPrintf("tea5767 freq 87.5~108\n");
  }
}
#endif


#endif


