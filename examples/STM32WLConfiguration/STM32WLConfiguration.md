/*
 * DESCRIPTION
 * This Document provides some #defines needed to configure common STM32WL modules
 *
 * STM32WL series processors use SX126x radios internally, but are more complicated to
 * configure because they can use either or both of the Highe Power Amplifier and the Low Power Amplifier.
 * In addition, they use additional pins to configure the RF output of the module
 */

/*
 * SEEED WIO-E5
 * This module has only the High Power Amplifier
 * It uses the additional RF switch pin, but does not use the enable pin
 * It supports 868 or 915 MHz
*/
#define MY_SX126x_TCXO_VOLTAGE (SX126x_TCXO_1V7)
#define MY_SX126x_ANT_SWITCH_PIN (PA5)  // P5
#define MY_SX126x_RF_SWITCH_PIN (PA4) // Enable low or high power for WIO E5 family
#define MY_SX126x_VARIANT (3)  // It's an STM32WL
#define MY_SX126x_MAX_POWER_LEVEL_DBM (22) // Max chip capibility.  Check local regulations
#define MY_SX126x_MIN_POWER_LEVEL_DBM (-9)
#define MY_SX126x_RF_SWITCH_IDLE LOW
#define MY_SX126x_RF_SWITCH_RX HIGH
#define MY_SX126x_RF_SWITCH_HPTX LOW


/*
 * SEEED WIO-E5-LE
 * This module has only the Low Power Amplifier
 * It uses the additional RF switch pin, but does not use the enable pin
 * It only supports 868 or 915 MHz
 * The lower power consumption of this module makes it a good candidate
 * for battery powered nodes
*/
#define MY_SX126x_TCXO_VOLTAGE (SX126x_TCXO_1V7)
#define MY_SX126x_ANT_SWITCH_PIN (PA5)  // P5
#define MY_SX126x_RF_SWITCH_PIN (PA4) // Enable low or high power for WIO E5 family
#define MY_SX126x_VARIANT (3)  // it's an STM32WL
#define MY_SX126x_MAX_POWER_LEVEL_DBM (14)
#define MY_SX126x_MIN_POWER_LEVEL_DBM (-17)
#define MY_SX126x_RF_SWITCH_IDLE LOW
#define MY_SX126x_RF_SWITCH_RX HIGH
#define MY_SX126x_RF_SWITCH_LPTX HIGH


/*
 * STM NUCLEO-WL55JC
 * This is the original dev board for the STM32WL55
 * This module has both the Low Power Amplifier and the High Power Amplifier
 * It uses the additional RF switch pin and the RF enable pin
 * NUCLEO-WL55JC1 supports high-frequency band
 * NUCLEO-WL55JC2 supports low-frequency band
*/
#define MY_SX126x_TCXO_VOLTAGE (SX126x_TCXO_1V7)
#define MY_SX126x_ANT_SWITCH_PIN (PC5)  // FE_CTRL2 on schematic
#define MY_SX126x_RF_SWITCH_PIN (PC4) // FE_CTRL1 on schematic
#define MY_SX126x_RF_ENABLE_PIN (PC3) // FE_CTRL3 on schematic
#define MY_SX126x_VARIANT (3)  // it's an STM32WL
#define MY_SX126x_MAX_POWER_LEVEL_DBM (22) // Max chip capibility. Check local regulations
#define MY_SX126x_MIN_POWER_LEVEL_DBM (-17)
#define MY_SX126x_RF_SWITCH_IDLE LOW
#define MY_SX126x_RF_SWITCH_RX HIGH
#define MY_SX126x_RF_SWITCH_LPTX HIGH
#define MY_SX126x_RF_SWITCH_HPTX LOW
