#ifndef ITHOECOFANRFT_H
#define ITHOECOFANRFT_H

#include "esphome.h"
#include "esphome/components/fan/fan.h"
#include <IthoCC1101.h>

class IthoEcoFanRFT : public Component, public fan::Fan {
public:
    void setup() override;
    void loop() override;

    void set_irq_pin(GPIOPin *pin) { _irq_pin = pin; }
    //void set_fan_speed(float speed);

protected:
    GPIOPin *_irq_pin;
    bool ITHOhasPacket = false;

    void control(const fan::FanCall &call) override;
    void write_state_();

private:
    IthoCC1101 rf;
    float last_speed = -1;

    #if defined (ESP82666) || defined (ESP32)
    IRAM_ATTR void ITHOCheck();
    #else
    void ITHOCheck();
    #endif
};

#endif