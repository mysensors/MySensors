/*
    Samsung AQV12PSBN / AQV09ASA heatpump control (remote control P/N zzz)
*/
#ifndef SamsungHeatpumpIR_h
#define SamsungHeatpumpIR_h

#include <HeatpumpIR.h>

// Samsung timing constants
#define SAMSUNG_AIRCON1_HDR_MARK   3000
#define SAMSUNG_AIRCON1_HDR_SPACE  9000
#define SAMSUNG_AIRCON1_BIT_MARK   500
#define SAMSUNG_AIRCON1_ONE_SPACE  1500
#define SAMSUNG_AIRCON1_ZERO_SPACE 500
#define SAMSUNG_AIRCON1_MSG_SPACE  2000

// Samsung codes
#define SAMSUNG_AIRCON1_MODE_AUTO  0x00 // Operating mode
#define SAMSUNG_AIRCON1_MODE_HEAT  0x40
#define SAMSUNG_AIRCON1_MODE_COOL  0x10
#define SAMSUNG_AIRCON1_MODE_DRY   0x20
#define SAMSUNG_AIRCON1_MODE_FAN   0x30
#define SAMSUNG_AIRCON1_MODE_OFF   0xC0 // Power OFF
#define SAMSUNG_AIRCON1_MODE_ON    0xF0 // Power ON
#define SAMSUNG_AIRCON1_FAN_AUTO   0x01 // Fan speed
#define SAMSUNG_AIRCON1_FAN1       0x05 // * low
#define SAMSUNG_AIRCON1_FAN2       0x09 // * med
#define SAMSUNG_AIRCON1_FAN3       0x0B // * high
#define SAMSUNG_AIRCON1_VS_SWING   0xAE // Vertical swing
#define SAMSUNG_AIRCON1_VS_AUTO    0xFE

class SamsungHeatpumpIR : public HeatpumpIR
{
  public:
    SamsungHeatpumpIR();
    void send(IRSender& IR, uint8_t powerModeCmd, uint8_t operatingModeCmd, uint8_t fanSpeedCmd, uint8_t temperatureCmd, uint8_t swingVCmd, uint8_t swingHCmd);

  private:
    void sendSamsung(IRSender& IR, uint8_t powerMode, uint8_t operatingMode, uint8_t fanSpeed, uint8_t temperature, uint8_t swingV);
};

#endif
