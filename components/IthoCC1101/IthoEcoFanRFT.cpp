#include "IthoEcoFanRFT.h"

void IthoEcoFanRFT::setup() {
    // Initialize the CC1101 module
    rf.init();

    // Send join comand.
    rf.sendCommand(IthoJoin);

    // Set interrupt pin
    _irq_pin.attach_interrupt(ITHOCheck, null, gpio::INTERRUPT_FALLING_EDGE);
}

#if defined (ESP82666) || defined (ESP32)
IRAM_ATTR void IthoEcoFanRFT::ITHOCheck() {
#else
void IthoEcoFanRFT::ITHOCheck() {
#endif
    ITHOhasPacket = true;
}

void IthoEcoFanRFT::loop() {
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

void IthoEcoFanRFT::control(const fan::FanCall &call) {
  if (call.get_state().has_value())
    this->state = *call.get_state();

  this->write_state_();
  this->publish_state();
}

void IthoEcoFanRFT::write_state_() {
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

void set_fan_speed(float speed) {
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