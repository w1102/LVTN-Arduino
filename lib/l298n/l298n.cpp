#include "l298n.h"

L298N::L298N(const uint8_t enb, const uint8_t in1, const uint8_t in2)
    : enb(enb),
      in1(in1),
      in2(in2),
      pwmChan(nullptr)
{
    pinMode(in1, OUTPUT);
    pinMode(in2, OUTPUT);
    stop();

    pwmChan = new ESP32PWM();
    pwmChan->attachPin(enb, L298N_PWM_FREQ, L298N_PWM_RESO);    
    setSpeed(0);
}

// ============ =============== =========================== ===========
// ============ =============== =========================== ===========
void L298N::setSpeed(uint16_t speed)
{
    if (!pwmChan || !pwmChan->attached())
    {
        return;
    }

    pwmChan->write(constrain(speed, 0, l298N_PWM_LIMIT_SPD));
}

void L298N::forward()
{
    if (!in1 || !in2)
    {
        return;
    }

    digitalWrite(in1, 0);
    digitalWrite(in2, 1);
}

void L298N::reward()
{
    if (!in1 || !in2)
    {
        return;
    }

    digitalWrite(in1, 1);
    digitalWrite(in2, 0);
}

void L298N::stop()
{
    if (!in1 || !in2)
    {
        return;
    }

    digitalWrite(in1, 0);
    digitalWrite(in2, 0);
    setSpeed(0);
}

// ============ =============== =========================== ===========
// ============ =============== =========================== ===========
L298N::~L298N()
{
    if (pwmChan)
    {
        pwmChan->detachPin(enb);
        delete pwmChan;
    }
}
