#include "mobile.h"
#include "dxl/dxl_uart.h"


enum 
{
  MOTOR_LEFT,
  MOTOR_RIGHT,
  MOTOR_MAX,
};

typedef struct
{
  bool    torque[MOTOR_MAX];
  int32_t position[MOTOR_MAX];
  int32_t velocity[MOTOR_MAX];
} motor_info_t;


static bool motorInit(void);
static bool motorIsConnected(void);
static bool motorSetTorque(bool enable);
static bool motorGetInfo(motor_info_t *p_info);
static bool motorSetVelocity(int32_t left_vel, int32_t right_vel);

static bool remoteInit(void);
static bool remoteUpdate(uint16_t *p_data);



static bool is_init = false;
static bool is_connected = false;
static dxl_t dxl;
static dxl_inst_t dxl_inst;

const uint8_t remote_ch = _DEF_UART2;
const uint8_t id_tbl[2] = { 1, 2 };




bool mobileInit(void)
{
  bool ret = false;


  motorInit();

  is_connected = motorIsConnected();
  is_init = true;

  return ret;
}

void mobileMain(void)
{
  button_obj_t btn_exit;
  button_obj_t btn_torque;
  uint32_t pre_time_info;
  uint32_t pre_time_motor;
  uint32_t pre_time_remote;
  motor_info_t motor_info;
  bool is_remote_connected = false;
  int32_t goal_velocity[MOTOR_MAX] = {0, };
  uint16_t remote_data;


  buttonObjCreate(&btn_torque,  0, 50, 1000, 100);      
  buttonObjCreate(&btn_exit,  3, 50, 1000, 100);      
  


  if (is_connected == false)
  {
    lcdClearBuffer(black);
    lcdPrintf(0, 0, white, "DXL 하드웨어 없음");
    lcdRequestDraw();
    delay(2000);
    return;
  }

  remoteInit();
  motorSetTorque(true);
  memset(&motor_info, 0, sizeof(motor_info));

  pre_time_info = millis();
  pre_time_motor = millis();
  pre_time_remote = millis();
  while(1)
  {
    buttonObjClearAndUpdate(&btn_torque);
    buttonObjClearAndUpdate(&btn_exit);

    if (buttonObjGetEvent(&btn_exit) & BUTTON_EVT_CLICKED)
    {
      buzzerBeep(50);
      break;
    }
    if (buttonObjGetEvent(&btn_torque) & BUTTON_EVT_CLICKED)
    {
      buzzerBeep(50);
      if (motor_info.torque[0] == true)
      {
        motorSetTorque(false);
      }
      else
      {
        motorSetTorque(true);
      }
    }


    if (lcdDrawAvailable() > 0)
    {
      lcdClearBuffer(black);

      lcdDrawFillRect(0, 0, LCD_WIDTH, 16, white);
      lcdPrintf(32, 0, black, "Mobile Robot");      

      lcdPrintf(0,   16, white, "TOQ :");
      lcdPrintf(48,  16, white, "%s", motor_info.torque[0] ? "On" : "Off");
      lcdPrintf(100, 16, white, "%s", motor_info.torque[1] ? "On" : "Off");

      lcdPrintf(0,   32, white, "POS :");
      lcdPrintf(48,  32, white, "%d", motor_info.position[0]);
      lcdPrintf(100, 32, white, "%d", motor_info.position[1]);

      lcdPrintf(0,   48, white, "VEL :");
      lcdPrintf(48,  48, white, "%d", motor_info.velocity[0]);
      lcdPrintf(100, 48, white, "%d", motor_info.velocity[1]);

      if (is_remote_connected == true)
        lcdPrintf(0,   64, green, "REM : Connected");
      else
        lcdPrintf(0,   64, red, "REM : Disconnected");

      lcdRequestDraw();
    }    

    cliMain();


    if (remoteUpdate(&remote_data) == true)
    {
      is_remote_connected = true;      
      pre_time_remote = millis();

      int32_t speed_ref = 100;
      int32_t speed_l = 0;
      int32_t speed_r = 0;

      if (remote_data & RC100_BTN_1) speed_ref *= 1.5;
      if (remote_data & RC100_BTN_2) speed_ref *= 2.0;
      if (remote_data & RC100_BTN_3) speed_ref *= 2.5;
      
      if (remote_data & RC100_BTN_U)
      {
        speed_l = speed_ref;
        speed_r = speed_ref;
      }
      if (remote_data & RC100_BTN_D)
      {
        speed_l = -speed_ref;
        speed_r = -speed_ref;
      } 
      if (remote_data & RC100_BTN_L)
      {
        speed_l = -speed_ref;
        speed_r = speed_ref;
      } 
      if (remote_data & RC100_BTN_R)
      {
        speed_l = speed_ref;
        speed_r = -speed_ref;
      } 

      goal_velocity[MOTOR_LEFT]  = speed_l;
      goal_velocity[MOTOR_RIGHT] = speed_r;
      
    }
    if (millis()-pre_time_remote >= 500)
    {
      is_remote_connected = false;
      goal_velocity[MOTOR_LEFT]  = 0;
      goal_velocity[MOTOR_RIGHT] = 0;
    }

    if (millis()-pre_time_info >= 100)
    {
      pre_time_info = millis();
      motorGetInfo(&motor_info);
    }

    if (millis()-pre_time_motor >= 50)
    {
      pre_time_motor = millis();
      motorSetVelocity(goal_velocity[MOTOR_LEFT], goal_velocity[MOTOR_RIGHT]);
    }
  }  


  motorSetTorque(false);
}

bool motorInit(void)
{
  bool ret;

  dxlLoadDriver(&dxl, dxlUartDriver);

  ret = dxlOpen(&dxl, _DEF_DXL1, 1000000);

  return ret;
}

bool motorIsConnected(void)
{
  bool ret = true;
  dxl_ping_t *p_ping = &dxl_inst.ping;

    
  ret = dxlInstPing(&dxl, id_tbl[0], p_ping, 100);
  if (ret == false) return false;

  ret = dxlInstPing(&dxl, id_tbl[1], p_ping, 100);
  if (ret == false) return false;

  return true;
}

bool motorSetTorque(bool enable)
{
  bool ret;
  dxl_sync_write_t *p_sysnc_write = &dxl_inst.sync_write;


  p_sysnc_write->param.id_cnt = 2;
  p_sysnc_write->param.addr   = 64;
  p_sysnc_write->param.length = 1;
  p_sysnc_write->param.node[0].id = id_tbl[MOTOR_LEFT];
  p_sysnc_write->param.node[0].data[0] = enable;
  p_sysnc_write->param.node[1].id = id_tbl[MOTOR_RIGHT];
  p_sysnc_write->param.node[1].data[0] = enable;

  ret = dxlInstSyncWrite(&dxl, p_sysnc_write, 100);

  return ret;
}

bool motorGetInfo(motor_info_t *p_info)
{
  bool ret;
  dxl_sync_read_t *p_sync_read = &dxl_inst.sync_read;


  p_sync_read->param.id_cnt = MOTOR_MAX;
  p_sync_read->param.addr   = 128;
  p_sync_read->param.length = 8;
  for (int i=0; i<MOTOR_MAX; i++)
  {
    p_sync_read->param.id[i] = id_tbl[i];
  }

  ret = dxlInstSyncRead(&dxl, p_sync_read, 100);
  if (ret == true)
  {
    for (int i=0; i<p_sync_read->resp.id_cnt; i++)
    {      
      p_info->velocity[i]  = p_sync_read->resp.node[i].data[0]<<0;
      p_info->velocity[i] |= p_sync_read->resp.node[i].data[1]<<8;
      p_info->velocity[i] |= p_sync_read->resp.node[i].data[2]<<16;
      p_info->velocity[i] |= p_sync_read->resp.node[i].data[3]<<24;

      p_info->position[i]  = p_sync_read->resp.node[i].data[4]<<0;
      p_info->position[i] |= p_sync_read->resp.node[i].data[5]<<8;
      p_info->position[i] |= p_sync_read->resp.node[i].data[6]<<16;
      p_info->position[i] |= p_sync_read->resp.node[i].data[7]<<24;
    }
  }

  p_sync_read->param.id_cnt = MOTOR_MAX;
  p_sync_read->param.addr   = 64;
  p_sync_read->param.length = 1;
  for (int i=0; i<MOTOR_MAX; i++)
  {
    p_sync_read->param.id[i] = id_tbl[i];
  }

  ret = dxlInstSyncRead(&dxl, p_sync_read, 100);
  if (ret == true)
  {
    for (int i=0; i<p_sync_read->resp.id_cnt; i++)
    {      
      p_info->torque[i]  = p_sync_read->resp.node[i].data[0];
    }
  }

  return ret;
}

bool motorSetVelocity(int32_t left_vel, int32_t right_vel)
{
  bool ret;
  dxl_sync_write_t *p_sysnc_write = &dxl_inst.sync_write;


  p_sysnc_write->param.id_cnt = 2;
  p_sysnc_write->param.addr   = 104;
  p_sysnc_write->param.length = 4;
  p_sysnc_write->param.node[0].id = id_tbl[MOTOR_LEFT];
  p_sysnc_write->param.node[0].data[0] = (left_vel >>  0) & 0xFF;
  p_sysnc_write->param.node[0].data[1] = (left_vel >>  8) & 0xFF;
  p_sysnc_write->param.node[0].data[2] = (left_vel >> 16) & 0xFF;
  p_sysnc_write->param.node[0].data[3] = (left_vel >> 24) & 0xFF;
  p_sysnc_write->param.node[1].id = id_tbl[MOTOR_RIGHT];
  p_sysnc_write->param.node[1].data[0] = (right_vel >>  0) & 0xFF;
  p_sysnc_write->param.node[1].data[1] = (right_vel >>  8) & 0xFF;
  p_sysnc_write->param.node[1].data[2] = (right_vel >> 16) & 0xFF;
  p_sysnc_write->param.node[1].data[3] = (right_vel >> 24) & 0xFF;

  ret = dxlInstSyncWrite(&dxl, p_sysnc_write, 100);

  return ret;
}

bool remoteInit(void)
{
  uartOpen(remote_ch, 57600);
  return true;
}

bool remoteUpdate(uint16_t *p_data)
{
  bool ret = false;


  if (uartAvailable(remote_ch) > 0)
  {
    while(uartAvailable(remote_ch) > 0)
    {
      if (rc100Update(uartRead(remote_ch)) == true)
      {
        *p_data = rc100GetData();
        ret = true;
        break;
      }
    }
  }

  return ret;
}