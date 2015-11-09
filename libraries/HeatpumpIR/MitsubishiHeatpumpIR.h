/*
    Mitsubishi MSZ FD-25 heatpump control (remote control P/N KM09D 0052376)
*/
#ifndef MitsubishiHeatpumpIR_h
#define MitsubishiHeatpumpIR_h

#include <HeatpumpIR.h>


// Mitsubishi MSZ FD-25 timing constants (remote control P/N KM09D 0052376)
#define MITSUBISHI_AIRCON1_HDR_MARK   3500
#define MITSUBISHI_AIRCON1_HDR_SPACE  1700
#define MITSUBISHI_AIRCON1_BIT_MARK   430
#define MITSUBISHI_AIRCON1_ONE_SPACE  1250
#define MITSUBISHI_AIRCON1_ZERO_SPACE 390
#define MITSUBISHI_AIRCON1_MSG_SPACE  17500

// Mitsubishi codes
#define MITSUBISHI_AIRCON1_MODE_AUTO  0x60 // Operating mode
#define MITSUBISHI_AIRCON1_MODE_HEAT  0x48
#define MITSUBISHI_AIRCON1_MODE_COOL  0x58
#define MITSUBISHI_AIRCON1_MODE_DRY   0x50
#define MITSUBISHI_AIRCON1_MODE_FAN   0x38 // EF 'FAN' mode
#define MITSUBISHI_AIRCON1_MODE_OFF   0x00 // Power OFF
#define MITSUBISHI_AIRCON1_MODE_ON    0x20 // Power ON
#define MITSUBISHI_AIRCON1_FAN_AUTO   0x00 // Fan speed - mixed with vertical swing...
#define MITSUBISHI_AIRCON1_FAN1       0x01
#define MITSUBISHI_AIRCON1_FAN2       0x02
#define MITSUBISHI_AIRCON1_FAN3       0x03
#define MITSUBISHI_AIRCON1_FAN4       0x04

#define MITSUBISHI_AIRCON1_VS_SWING   0x78 // Vertical swing
#define MITSUBISHI_AIRCON1_VS_AUTO    0x40 // Automatic setting
#define MITSUBISHI_AIRCON1_VS_UP      0x48
#define MITSUBISHI_AIRCON1_VS_MUP     0x50
#define MITSUBISHI_AIRCON1_VS_MIDDLE  0x58
#define MITSUBISHI_AIRCON1_VS_MDOWN   0x60
#define MITSUBISHI_AIRCON1_VS_DOWN    0x68

#define MITSUBISHI_AIRCON1_VS_AUTO2   0x80 // ???
#define MITSUBISHI_AIRCON1_VS_AUTO3   0xB8 // ???

#define MITSUBISHI_AIRCON1_HS_SWING   0xC0 // Horizontal swing
#define MITSUBISHI_AIRCON1_HS_MIDDLE  0x30
#define MITSUBISHI_AIRCON1_HS_LEFT    0x10
#define MITSUBISHI_AIRCON1_HS_MLEFT   0x20
#define MITSUBISHI_AIRCON1_HS_MRIGHT  0x40
#define MITSUBISHI_AIRCON1_HS_RIGHT   0x50

// Mitsubishi model codes
#define MITSUBISHI_FD 0
#define MITSUBISHI_FE 1

class MitsubishiHeatpumpIR : public HeatpumpIR
{
  protected: // Cannot create generic Mitsubishi heatpump instances
    MitsubishiHeatpumpIR();
    uint8_t _mitsubishiModel;  // Tells whether this is FD or EF (or other supported model...)

  public:
    void send(IRSender& IR, uint8_t powerModeCmd, uint8_t operatingModeCmd, uint8_t fanSpeedCmd, uint8_t temperatureCmd, uint8_t swingVCmd, uint8_t swingHCmd);

  private:
    void sendMitsubishi(IRSender& IR, uint8_t powerMode, uint8_t operatingMode, uint8_t fanSpeed, uint8_t temperature, uint8_t swingVCmd, uint8_t swingHCmd);
};

class MitsubishiFDHeatpumpIR : public MitsubishiHeatpumpIR
{
  public:
    MitsubishiFDHeatpumpIR();
};

class MitsubishiFEHeatpumpIR : public MitsubishiHeatpumpIR
{
  public:
    MitsubishiFEHeatpumpIR();
};

#endif
