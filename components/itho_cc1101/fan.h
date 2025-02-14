#ifndef ITHOECOFANRFT_H
#define ITHOECOFANRFT_H

#include "esphome.h"
#include "esphome/components/fan/fan.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/gpio.h"
#include <IthoCC1101.h>

namespace esphome {
    namespace IthoEcoRFTFan {
        class IthoEcoRFT : public PollingComponent, public fan::Fan {
            public:
                IthoEcoRFT(int speed_count, bool map_off_to_zero) : speed_count_(speed_count), map_off_to_zero_(map_off_to_zero) {}

                void setup() override;
                void update() override;
                void loop() override;

                void set_irq_pin(GPIOPin *pin) { _irq_pin = pin; }
                void set_fan_speed(int speed);
                void send_other_command(uint8_t other_command);

            protected:
                GPIOPin *_irq_pin;
                bool ITHOhasPacket = false;
                int speed_count_{};
                bool map_off_to_zero_{};

                void startResetTimer(uint16_t seconds);
                void resetFanSpeed();

                void control(const fan::FanCall &call) override;
                void write_state_();
                void publish_state();

            private:
                IthoCC1101 rf;
                
                int LastSpeed = -1;
                int Speed = -1;

                Ticker reset_Timer_;
                bool timer_active_ = false;

                uint16_t TimerTime1 = 10*60;
                uint16_t TimerTime2 = 20*60;
                uint16_t TimerTime3 = 30*60;

                #if defined (ESP82666) || defined (ESP32)
                IRAM_ATTR void ITHOCheck();
                #else
                void ITHOCheck();
                #endif
        };
    } // namespace: IthoEcoRFTFan
} // namespace: esphome

#endif