#include "ap_def.h"


typedef struct
{
  float freq_mhz;
  const char *desc;
} radio_fm_broad_t;


const radio_fm_broad_t radio_fm_broad[] = 
{
  {89.1f, "KBS Cool FM"},
  {89.7f, "WBS 원음방송"},
  {90.7f, "경인방송"},
  {91.9f, "MBC FM4U"},
  {93.1f, "KBS ClassicFM"},
  {93.9f, "CBS 음악FM"},
  {94.5f, "YTN 라디오"},
  {95.1f, "TBS FM"},
  {95.9f, "MBC 표준FM"},
  {96.7f, "국방 FM"},
  {97.3f, "KBS1 Radio"},
  {98.1f, "CBS 표준FM"},
  {99.1f, "국악방송 FM"},
  {99.9f, "OBS 라디오"},
  {101.3f, "TBS eFM"},
  {101.9f, "BBS 불교방송"},
  {103.5f, "LOVE FM"},
  {104.5f, "EBS FM"},
  {104.9f, "KBS3 Radio"},
  {105.3f, "cpbc Radio"},
  {106.1f, "KBS HappyFM"},
  {106.9f, "febc 극동방송"},
  {107.7f, "POWER FM"},
};