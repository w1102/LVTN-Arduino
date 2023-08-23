#ifndef _L298N_H_
#define _L298N_H_

#define L298N_PWM_FREQ 50000
#define L298N_PWM_RESO 10
#define l298N_PWM_LIMIT_SPD 1023

#include "Arduino.h"
#include "ESP32PWM.h"

class L298N
{
  public:
    L298N(const uint8_t enb, const uint8_t in1, const uint8_t in2);

    void setSpeed(uint16_t speed);
    void forward();
    void reward();
    void stop();
    ~L298N();

  private:
    uint8_t enb, in1, in2;
    ESP32PWM *pwmChan;

    uint8_t lhsEn, rhsEn;
    ESP32PWM *lhsEnPwmChan;
    ESP32PWM *rhsEnPwmChan;
    uint8_t lhsIN1, lhsIN2, rhsIN1, rhsIN2;
};

#endif // _L298N_H_