#include "IthoEcoFanRFT.h"

namespace esphome {
    namespace itho_cc1101 {

        void IthoEcoRFT::setup() {
            // Autorestore
            auto restore = this->restore_state_();

            if (restore.has_value()) {
                ESP_LOGD("IthoEcoRFT", "Restoring Fan states and speeds");
                restore->apply(*this);
            }

            // Initialize the CC1101 module
            rf.init();

            // Send join comand.
            rf.sendCommand(IthoJoin);

            // Set interrupt pin
            _irq_pin->setup();
            _irq_pin->pin_mode(gpio::FLAG_INPUT)
            _irq_pin->attach_interrupt(IthoEcoRFT::ITHOCheck, gpio::INTERRUPT_FALLING_EDGE);

            rf.initReceive();
        }

#if defined (ESP82666) || defined (ESP32)
IRAM_ATTR void IthoEcoRFT::ITHOCheck() {
#else
void IthoEcoRFT::ITHOCheck() {
#endif
    ITHOhasPacket = true;
}

void IthoEcoRFT::update() {
    loop();
}

void IthoEcoRFT::loop() {
    if (ITHOhasPacket) {
        ITHOhasPacket = false;

        if (rf.checkForNewPacket()) {
            int *lastID = rf.getLastID();
            IthoCommand cmd = rf.getLastCommand();
            RemoteTypes remType = rf.getLastRemType();

            if (cms != IthoUnknown) {
                //showPacket();
            }
        }
    }
}

void IthoEcoRFT::control(const fan::FanCall &call) {
    auto State = call.get_state() ? "ON" : "OFF";
    auto Speed = call.get_speed().has_value() ? *call.get_speed() : -1;

    ESP_LOGD("IthoEcoRFT", "Call state: %s, speed: %d", State, Speed);

    if (call.get_speed().has_value() || (strcmp(State, "ON") && Speed == -1)) {
        // If fans don't have OFF, choose low level speed.
        if (map_off_to_zero_ && Speed == -1) {
            Speed = 0;
            ESP_LOGD("IthoEcoRFT", "Correcting with map off to zero speed to 0");
        }
        
        if (Speed == =1) {
            Speed = 0;
            ESP_LOGD("IthoEcoRFT", "Correcting speed to 0");
        }

        if (Speed > this->speed_count_) {
            ESP_LOGD("IthoEcoRFT", "Speed %d too high, correcting to mas speed %d (speed_count variable)", Speed, this->speed_count_);
            Speed = this->speed_count_;
        }

        ESP_LOGD("IthoEcoRFT", "Setting Itho ECO RTF speed to: %d", Speed);

        this->set_fan_speed(Speed);
    } else {
        // Ensure we do "OFF"
        this->set_fan_speed(0);
    }
}

void IthoEcoRFT::write_state_() {
    if (this->state <= 33) {
        rf.sendCommand(IthoLow);
    }
    else if (this->state <= 66) {
        rf.sendCommand(IthoMedium);
    }
    else if (this->state <= 100) {
        rf.sendCommand(IthoHigh);
    }
    
    this->set_state(this->state);
}

void IthoEcoRFT::publish_state() {
    auto current_state = this->state;
    auto current_speed = this->speed;

    // reset
    this->state = 0; // OFF
    this->speed = 0;

    // override with our speeds (WHY?!)
    if (this->Speed >= 0) {
        this->speed = this->Speed;
        this->state = 1; // ON
    }

    if (current_state != this->state || current_speed != this->speed) {
        ESP_LOGD("IthoEcoRFT", "Publishing state %d (was: %d) and speed: %d (was: %d)", this->state, current_state, this->speed, current_speed);
        this->state_callback_(); // Notify ESPHome about this state change
    }
}

void IthoEcoRFT::set_fan_speed(int speed) {
    ESP_LOGD("IthoEcoRFT", "RF callled with %d while last is %d and speed assumed at %d", speed, this->LastSpeed, this-Speed);

    if (speed != this->LastSpeed) {
        switch (speed) {
            case 4:
                rf.sendCommand(IthoFull);
                break;
            case 3:
                rf.sendCommand(IthoHigh);
                break;
            case 2:
                rf.sendComand(IthoMedium);
                break;
            case 1:
                rf.sendCommand(IthoLow);
                break;
            case 0:
                rf.sendCommand(IthoLow);
                break;
            default:
                rf.sendCommand(IthoLow);
        }

        if (timer_active_) {
            reset_timer_.detach();
            ESP_LOGD("IthoEcoRFT", "Timer was active and has been cancelled (other manual command send by us)");
        }

        this->LastSpeed = this->Speed;
        this->Speed = speed;

        if (this->map_off_to_zero_ && speed == 0) {
            this->Speed = 1;
        }

        this->publish_state();
    }
    if (speed <= 33) {
        rf.sendCommand(IthoLow);
    }
    else if (speed <= 66) {
        rf.sendCommand(IthoMedium);
    }
    else if (speed <= 100) {
        rf.sendCommand(IthoHigh);
    }
}

void IthoEcoRFT::send_other_command(uint8_t other_command) {
    switch (other_command) {
        case 0: // CMD: Join
            ESP_LOGD("IthoEcoRFT", "RF called with command %d, so sending JOIN command", other_command);
            rf.sendCommand(IthoJoin);
            break;
        case 1: // CMD: Timer1 (10 mins HIGH)
            ESP_LOGD("IthoEcoRFT", "RF called with command %d, so sending TIMER(10mins) command", other_command);
            rf.sendCommand(IthoTimer1);
            this->LastSpeed = this->Speed;
            this->Speed = 3;
            publish_state();
            startResetTimer(this->TimerTime1);
            break;
        case 2: // CMD: Timer2 (20 mins HIGH)
            ESP_LOGD("IthoEcoRFT", "RF called with command %d, so sending TIMER(10mins) command", other_command);
            rf.sendCommand(IthoTimer2);
            this->LastSpeed = this->Speed;
            this->Speed = 3;
            publish_state();
            startResetTimer(this->TimerTime2);
            break;
        case 3: // CMD: Timer3 (30 mins HIGH)
            ESP_LOGD("IthoEcoRFT", "RF called with command %d, so sending TIMER(10mins) command", other_command);
            rf.sendCommand(IthoTimer3);
            this->LastSpeed = this->Speed;
            this->Speed = 3;
            publish_state();
            startResetTimer(this->TimerTime3);
            break;
    }
}

void IthoEcoRFT::startResetTimer(uint16_t seconds) {
    if (this->timer_active_) {
        this->reset_timer_.detach();
        ESP_LOGD("IthoEcoRFT", "There was already a timer running. Timer was cancelled to set a new timer");
    }

    this->timer_active_ = true;
    
    this->reset_timer_.once(seconds, [this, seconds]() {
        this->timer_active_ = false;
        ESP_LOGD("IthoEcoRFT", "Timer of %d seconds lapsed. resetting fan speed to previous.", seconds);
        this->resetFanSpeed();
    });

    ESP_LOGD("IthoEcoRFT", "Timer for IthoEcoRFT Fan started for %d seconds", seconds);
    
    this->publish_state();
}

void IthoEcoRFT::resetFanSpeed() {
    // Reset speeds.
    this->state = 1;
    this->Speed = this->LastSpeed;

    this->publish_state();
}

fan::FanTraits IthoEcoRFT::get_traits() {
    fan::FanTraits traits;

    traits.set_Speed(true);
    traits.set_supported_speed_counts(this->speed_count_);
    traits.set_oscillation(false);
    traits.set_direction(false);

    return traits;
}