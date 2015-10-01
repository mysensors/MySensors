/*
    Carrier 42NQV035G / 38NYV035H2 heatpump control (remote control P/N WH-L05SE)
*/
#ifndef CarrierHeatpumpIR_h
#define CarrierHeatpumpIR_h

#include <HeatpumpIR.h>

// Carrier (42NQV035G / 38NYV035H2) timing constants (remote control P/N WH-L05SE)
#define CARRIER_AIRCON1_HDR_MARK   4320
#define CARRIER_AIRCON1_HDR_SPACE  4350
#define CARRIER_AIRCON1_BIT_MARK   500
#define CARRIER_AIRCON1_ONE_SPACE  1650
#define CARRIER_AIRCON1_ZERO_SPACE 550
#define CARRIER_AIRCON1_MSG_SPACE  7400

// Carrier codes
#define CARRIER_AIRCON1_MODE_AUTO  0x00 // Operating mode
#define CARRIER_AIRCON1_MODE_HEAT  0xC0
#define CARRIER_AIRCON1_MODE_COOL  0x80
#define CARRIER_AIRCON1_MODE_DRY   0x40
#define CARRIER_AIRCON1_MODE_FAN   0x20
#define CARRIER_AIRCON1_MODE_OFF   0xE0 // Power OFF
#define CARRIER_AIRCON1_FAN_AUTO   0x00 // Fan speed
#define CARRIER_AIRCON1_FAN1       0x02
#define CARRIER_AIRCON1_FAN2       0x06
#define CARRIER_AIRCON1_FAN3       0x01
#define CARRIER_AIRCON1_FAN4       0x05
#define CARRIER_AIRCON1_FAN5       0x03


class CarrierHeatpumpIR : public HeatpumpIR
{
  public:
    CarrierHeatpumpIR();
    void send(IRSender& IR, uint8_t powerModeCmd, uint8_t operatingModeCmd, uint8_t fanSpeedCmd, uint8_t temperatureCmd, uint8_t swingVCmd, uint8_t swingHCmd);

  private:
    void sendCarrier(IRSender& IR, uint8_t operatingMode, uint8_t fanSpeed, uint8_t temperature);
};

#endif
