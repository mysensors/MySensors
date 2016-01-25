# arduino-heatpumpir

Original location: https://github.com/ToniA/arduino-heatpumpir

An Arduino library to control a Panasonic, Midea, Carrier, Fujitsu or Mitsubishi heat pump/split unit air conditioner.
Currently supports at least these models 
* Panasonic E9/E12-CKP (Panasonic remote control P/N A75C2295)
* Panasonic E9/E12-DKE (Panasonic remote control P/N A75C2616)
* Panasonic E9/E12-JKE and E9/E12-NKE
* Midea MSR1-12HRN1-QC2 + MOA1-12HN1-QC2, sold as Ultimate Pro Plus Basic 13FP in Finland (Midea remote control P/N RG51M1/E)
* Carrier 42NQV035G / 38NYV035H2 (Carrier remote control P/N WH-L05SE)
* Fujitsu Nocria AWYZ14 (remote control P/N AR-PZ2)
* Mitsubishi MSZ FD-25, probably also FD-35 (remote control P/N KM09D 0052376)
* Hisense AUD (remote control Y-H1-01,  Y-H1-02(E), Y-J1, Y-E4-07) probably AUC model
* Sharp AY-ZP40KR (remote control P/N CRMC-A788JBEZ), possibly also IVT
* Daikin RXS25G2V1B /FVXS25FV1B (Remote control P/N ARC452A1)


## Instructions

* Download the library, and place it under your personal Arduino 'libraries' directory, under directory 'HeatpumpIR'
* See the example sketches

![Schema](https://raw.github.com/ToniA/arduino-heatpumpir/master/arduino_irsender.png)
