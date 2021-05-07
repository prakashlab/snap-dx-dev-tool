#pragma once

#include "Subsystems.h"
#include "Util.h"

// The InstantDx class provides access to the various devices in
// the system. It also defines routines which take a long time to
// run (and thus may require certain backgrounds tasks to be run),
// such as initializing the devices, waiting for a button press,
// running the test, waiting for temperature to converge on a target, etc.
class InstantDx {
   public:
    InstantDx()
        : user_interface(51, 53),
          camera(45, 47),
          thermal_controller_1(44, A1, A0, 1000 * 10, 1000 * 20),
          thermal_controller_2(42, A2, A0, 1000 * 5, 1000 * 5),
          fan(36, true),
          motion_controller(33, 35, 31, 22, 23),
          tickler(40, true),
          door(38, 24) {}

    UserInterface user_interface;
    ESP32Camera camera;
    ThermalController
        thermal_controller_1;  // cartridge base heater (for 95 deg C)
    ThermalController
        thermal_controller_2;  // cartridge top heater (for 65 deg C)
    DigitalOutput fan;  // always on, to have good airflow exchange with system
                        // and reduce the system's heat capacity
    MotionController motion_controller;
    DigitalOutput tickler;
    Door door;

    // Initialize everything. Return value describes whether everything
    // succesfully initialized.
    bool await_initialize() {
        if (!user_interface.setup()) {
            return false;
        }
        user_interface.print_message("Initializing...");
        user_interface.label_buttons();
        /*if (!camera.setup()) {
            return false;
        }*/

        if (!thermal_controller_1.setup()) {
            return false;
        }

        if (!thermal_controller_2.setup()) {
            return false;
        }

        if (!fan.setup()) {
            return false;
        }

        fan.activate();
        /*if (!motion_controller.setup()) {
            return false;
        }*/

        if (!tickler.setup()) {
            return false;
        }

        /*if (!door.setup()) {
            return false;
        }*/

        return true;
    }

    // Wait for the specified time to elapse. Like the Arduino delay function,
    // but performs background tasks while waiting.
    void await_sleep(unsigned long timeout) {
        const unsigned long start_time = millis();
        // Background_update always runs at least once, even if timeout is 0
        while (true) {
            background_update();
            if (past_timeout(start_time, timeout)) {
                return;
            }
        }
    }

    // Wait for the specified button to be pressed and then released.
    void await_press(DebouncedSwitch &button) {
        using Event = DebouncedSwitch::State;

        // Discard previous press/release events
        button.read();
        while (true) {
            background_update();  // updates button debouncing

            if (button.read() == Event::activated) {
                SerialUSB.print("InstantDx.await_press(");
                if (user_interface.primary.pin == button.pin) {
                    SerialUSB.print("primary");
                } else if (user_interface.secondary.pin == button.pin) {
                    SerialUSB.print("secondary");
                } else {
                    SerialUSB.print(button.pin);
                }
                SerialUSB.println("): pressed!");
                return;
            }
        }
    }
    // Wait for a single button to be pressed. Return value describes which
    // button was pressed - either primary or secondary. Will not return
    // both/neither; if both were simultaneously pressed, primary button has
    // priority.
    UserInterface::ButtonsState await_press() {
        using Event = DebouncedSwitch::State;
        using ButtonsState = UserInterface::ButtonsState;

        // Discard previous press/release events
        user_interface.primary.read();
        user_interface.secondary.read();
        while (true) {
            background_update();  // updates button debouncing
            if (user_interface.primary.read() == Event::activated) {
                SerialUSB.println("InstantDx.await_press: primary pressed!");
                return ButtonsState::primary;
            }
            if (user_interface.secondary.read() == Event::activated) {
                SerialUSB.println("InstantDx.await_press: secondary pressed!");
                return ButtonsState::secondary;
            }
        }
    }

    // Wait for heater to warm up past the threshold
    bool await_thermal_warmup(
        ThermalController &thermal_controller,
        float threshold,
        unsigned long timeout) {
        thermal_controller.start_control(threshold);
        const unsigned long start_time = millis();
        while (true) {
            background_update();  // updates thermal control loop
            if (thermal_controller.thermistor.temperature() > threshold) {
                return true;
            }

            if (past_timeout(start_time, timeout)) {
                return false;
            }
        }
    }
    // Wait for both heaters to cool down past the threshold
    bool await_thermal_cooldown(float threshold, unsigned long timeout) {
        thermal_controller_1.stop_control();
        thermal_controller_2.stop_control();
        const unsigned long start_time = millis();
        while (true) {
            background_update();
            if (thermal_controller_1.thermistor.temperature() < threshold &&
                thermal_controller_2.thermistor.temperature() < threshold) {
                return true;
            }

            if (past_timeout(start_time, timeout)) {
                return false;
            }
        }
    }

    // Unlock the door, wait a while, and assume that it's unlocked
    void await_unlock(unsigned long timeout) {
        door.start_unlock();
        await_sleep(timeout);  // updates the limit switch debouncer
        // We assume door.is_open() == true without checking.
    }
    // Lock the door, wait a while, and check if it's locked
    bool await_lock(unsigned long timeout) {
        door.start_lock();
        // This approach waits a constant time, to give the solenoid time to
        // fully actuate. This approach assumes that the door is already
        // closed. The timeout would include both solenoid actuation and limit
        // switch bouncing.
        await_sleep(timeout);  // updates the limit switch debouncer
        return !door.is_open();
    }

    // Wait for the specified displacement movement to complete (successfully or
    // unsuccessfully), but give up as a failure if it's not finished within a
    // timeout, or if it was cancelled by a limit switch.
    bool await_move(
        float displacement, float target_speed, unsigned long timeout) {
        const unsigned long start_time = millis();
        motion_controller.start_move(displacement, target_speed);
        // background_update always runs at least once.
        while (true) {
            background_update();  // updates motion control loop
            if (motion_controller.moved_into_top_limit() ||
                motion_controller.moved_into_bottom_limit()) {
                return false;
            }

            if (!motion_controller.moving()) {
                return true;
            }

            if (past_timeout(start_time, timeout)) {
                return false;
            }
        }
    }

    // Wait for the specified move to a limit to complete, but give up as a
    // failure if it's not finished within a timeout.
    bool await_move(float target_velocity, unsigned long timeout) {
        const DebouncedSwitch &limit = target_velocity >= 0
                                           ? motion_controller.switch_top
                                           : motion_controller.switch_bottom;
        motion_controller.start_move(target_velocity);
        return await_limit_press(limit, timeout);
    }

    // Wait for the specified limit switch to be pressed, but give up
    // as a failure if it's not pressed within a timeout.
    bool await_limit_press(DebouncedSwitch button, unsigned long timeout) {
        using State = DebouncedSwitch::State;

        const unsigned long start_time = millis();
        // background_update always runs at least once.
        while (true) {
            background_update();  // updates motion control loop and button
            State state = button.read();
            if (state == State::active || state == State::activated) {
                return true;
            }

            if (past_timeout(start_time, timeout)) {
                return false;
            }
        }
    }

   private:
    // Run one update step for anything which needs to be updated frequently
    // e.g. the thermal controller, motion controller, etc.
    void background_update() {
        thermal_controller_1.update();
        thermal_controller_2.update();
        user_interface.update();
        motion_controller.update();
        door.update();
    }
};