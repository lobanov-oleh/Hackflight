/*
   Webots-based flight simulator support for Hackflight

   Copyright (C) 2024 Simon D. Levy

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, in version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http:--www.gnu.org/licenses/>.
 */


#pragma once

// C
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// C++
#include <map>
#include <string>

// Webots
#include <webots/joystick.h>
#include <webots/keyboard.h>
#include <webots/motor.h>
#include <webots/robot.h>
#include <webots/supervisor.h>

// Hackflight
#include <hackflight.hpp>
#include <pids/altitude.hpp>
#include <sim/vehicles/tinyquad.hpp>

namespace hf {

    class Simulator {

        public:

            void run(const bool tryJoystick=true)
            {
                wb_robot_init();

                const auto timestep = wb_robot_get_basic_time_step();

                if (tryJoystick) {

                    wb_joystick_enable(timestep);
                }

                else {

                    printKeyboardInstructions();
                }

                wb_keyboard_enable(timestep);

                const auto copter_node = wb_supervisor_node_get_from_def("ROBOT");

                const auto translation_field =
                    wb_supervisor_node_get_field(copter_node, "translation");

                const auto rotation_field =
                    wb_supervisor_node_get_field(copter_node, "rotation");

                auto motor1 = make_motor("motor1");
                auto motor2 = make_motor("motor2");
                auto motor3 = make_motor("motor3");
                auto motor4 = make_motor("motor4");

                /*
                // Spin up the motors for a second before starting dynamics
                for (long k=0; k < SPINUP_TIME * timestep; ++k) {

                    if (wb_robot_step((int)timestep) == -1) {
                        break;
                    } 

                    const float motorvals[4] = {
                        MOTOR_MAX, MOTOR_MAX, MOTOR_MAX, MOTOR_MAX
                    };

                    spin_motors(motor1, motor2, motor3, motor4, motorvals);

                    // Keep the vehicle on the ground
                    const double pos[3] = {};
                    wb_supervisor_field_set_sf_vec3f(translation_field, pos);
                }*/

                // Start the dynamics thread
                thread_data_t thread_data = {};
                thread_data.running = true;
                pthread_t thread = {};
                pthread_create(&thread, NULL, *thread_fun, (void *)&thread_data);

                // This initial value will be ignored for traditional (non-springy)
                // throttle
                float z_target = INITIAL_ALTITUDE_TARGET;

                demands_t * demands = &thread_data.demands;

                auto posevals = thread_data.posevals;

                while (true) {

                    if (wb_robot_step((int)timestep) == -1) {
                        break;
                    } 

                    demands_t open_loop_demands = {};

                    getDemands(open_loop_demands);

                    // Throttle control begins when once takeoff is requested, either by
                    // hitting a button or key ("springy", self-centering throttle) or by
                    // raising the non-self-centering throttle stick
                    if (_requested_takeoff) {

                        // "Springy" (self-centering) throttle or keyboard: accumulate 
                        // altitude target based on stick deflection, and attempt
                        // to maintain target via PID control
                        if (isSpringy()) {

                            z_target += CLIMB_RATE_SCALE * open_loop_demands.thrust;

                            demands->thrust = z_target;
                        }

                        // Traditional (non-self-centering) throttle: 
                        //
                        //   (1) In throttle deadband (mid position), fix an altitude target
                        //       and attempt to maintain it via PID control
                        //
                        //   (2) Outside throttle deadband, get thrust from stick deflection
                        else {

                            static bool _was_in_deadband;

                            const auto in_deadband = fabs(open_loop_demands.thrust) < THROTTLE_DEADBAND;

                            z_target =
                                in_deadband && !_was_in_deadband ?
                                posevals[2] :
                                open_loop_demands.thrust;

                            _was_in_deadband = in_deadband;

                            if (in_deadband) {
                                demands->thrust = z_target;
                            }
                        }
                    }

                    auto motorvals = thread_data.motorvals;

                    const double pos[3] = {posevals[0], posevals[1], posevals[2]};
                    wb_supervisor_field_set_sf_vec3f(translation_field, pos);

                    double rot[4] = {};
                    angles_to_rotation(posevals[3], posevals[4], posevals[5], rot);
                    wb_supervisor_field_set_sf_rotation(rotation_field, rot);

                    spin_motors(motor1, motor2, motor3, motor4, motorvals);
                }

                thread_data.running = false;

                pthread_join(thread, NULL);
            }

        private:

            // For springy-throttle gamepads / keyboard
            static constexpr float INITIAL_ALTITUDE_TARGET = 0.2;
            static constexpr float CLIMB_RATE_SCALE = 0.01;

            static constexpr float THROTTLE_DEADBAND = 0.2;

            static constexpr float THRUST_BASE = 55.385;

            static constexpr float DYNAMICS_DT = 1e-4;

            static const uint32_t PID_PERIOD = 1000;

            static constexpr float MOTOR_MAX = 60;

            static constexpr float SPINUP_TIME = 2;

            typedef enum {

                JOYSTICK_NONE,
                JOYSTICK_UNRECOGNIZED,
                JOYSTICK_RECOGNIZED

            } joystickStatus_e;

            typedef struct {

                int8_t throttle;
                int8_t roll;
                int8_t pitch;
                int8_t yaw;

                bool springy;                

            } joystick_t;

            typedef struct {

                demands_t demands;
                float posevals[6];
                float motorvals[4];
                bool running;

            } thread_data_t;

            bool _requested_takeoff;

            static WbDeviceTag make_motor(const char * name)
            {
                auto motor = wb_robot_get_device(name);

                wb_motor_set_position(motor, INFINITY);

                return motor;
            }

            static float deg2rad(const float deg)
            {
                return M_PI * deg / 180;
            }

            static float max3(const float a, const float b, const float c)
            {
                return
                    a > b && a > c ? a :
                    b > a && b > c ? b :
                    c;
            }

            static float sign(const float val)
            {
                return val < 0 ? -1 : +1;
            }

            static float scale(const float angle, const float maxang)
            {
                return sign(angle) * sqrt(fabs(angle) / maxang);
            }

            static void angles_to_rotation(
                    const float phi, const float theta, const float psi,
                    double rs[4])
            {
                const auto phirad = deg2rad(phi);
                const auto therad = deg2rad(theta);
                const auto psirad = deg2rad(psi);

                const auto maxang = max3(fabs(phirad), fabs(therad), fabs(psirad));

                if (maxang == 0) {
                    rs[0] = 0;
                    rs[1] = 0;
                    rs[2] = 1;
                    rs[3] = 0;
                }

                else {

                    rs[0] = scale(phi, maxang);
                    rs[1] = scale(theta, maxang);
                    rs[2] = scale(psi, maxang);
                    rs[3] = maxang;
                }
            }

            static float min(const float val, const float maxval)
            {
                return val > maxval ? maxval : val;
            }

            static void * thread_fun(void *ptr)
            {
                auto thread_data = (thread_data_t *)ptr;

                auto dynamics = Dynamics(tinyquad_params, DYNAMICS_DT);

                AltitudePid altitudePid = {};

                state_t state  = {};

                demands_t demands = {};

                float motor = 0;

                for (long k=0; thread_data->running; k++) {

                    if (k % PID_PERIOD == 0) {

                        // Start with open-loop demands from main thread
                        demands_t open_loop_demands = thread_data->demands;
                        demands.thrust = open_loop_demands.thrust; 
                        demands.roll = open_loop_demands.roll; 
                        demands.pitch = open_loop_demands.pitch; 
                        demands.yaw = open_loop_demands.yaw;

                        // Altitude PID controller converts target to thrust demand
                        altitudePid.run(DYNAMICS_DT, state, demands);
                    }

                    motor = min(demands.thrust + THRUST_BASE, MOTOR_MAX);

                    dynamics.setMotors(motor, motor, motor, motor);
                    state.z = dynamics.x[Dynamics::STATE_Z];
                    state.dz = dynamics.x[Dynamics::STATE_Z_DOT];

                    auto posevals = thread_data->posevals;

                    posevals[0] = dynamics.x[Dynamics::STATE_X];
                    posevals[1] = dynamics.x[Dynamics::STATE_Y];
                    posevals[2] = dynamics.x[Dynamics::STATE_Z];
                    posevals[3] = dynamics.x[Dynamics::STATE_PHI];
                    posevals[4] = dynamics.x[Dynamics::STATE_THETA];
                    posevals[5] = dynamics.x[Dynamics::STATE_PSI];

                    auto motorvals = thread_data->motorvals;

                    motorvals[0] = motor;
                    motorvals[1] = motor;
                    motorvals[2] = motor;
                    motorvals[3] = motor;

                    usleep(DYNAMICS_DT / 1e-6);
                }

                return  ptr;
            }

            static void spin_motors(
                    WbDeviceTag m1, WbDeviceTag m2, WbDeviceTag m3, WbDeviceTag m4,
                    const float motorvals[4])
            {
                // Negate expected direction to accommodate Webots
                // counterclockwise positive
                wb_motor_set_velocity(m1, -motorvals[0]);
                wb_motor_set_velocity(m2, +motorvals[1]);
                wb_motor_set_velocity(m3, +motorvals[2]);
                wb_motor_set_velocity(m4, -motorvals[3]);
            }

            std::map<std::string, joystick_t> JOYSTICK_AXIS_MAP = {

                // Springy throttle
                { "MY-POWER CO.,LTD. 2In1 USB Joystick", // PS3
                    joystick_t {-2,  3, -4, 1, true } },
                { "SHANWAN Android Gamepad",             // PS3
                    joystick_t {-2,  3, -4, 1, true } },
                { "Logitech Gamepad F310",
                    joystick_t {-2,  4, -5, 1, true } },

                // Classic throttle
                { "Logitech Logitech Extreme 3D",
                    joystick_t {-4,  1, -2, 3, false}  },
                { "OpenTX FrSky Taranis Joystick",  // USB cable
                    joystick_t { 1,  2,  3, 4, false } },
                { "FrSky FrSky Simulator",          // radio dongle
                    joystick_t { 1,  2,  3, 4, false } },
                { "Horizon Hobby SPEKTRUM RECEIVER",
                    joystick_t { 2,  -3,  4, -1, false } }
            };

            static float normalizeJoystickAxis(const int32_t rawval)
            {
                return 2.0f * rawval / UINT16_MAX; 
            }

            static int32_t readJoystickRaw(const int8_t index)
            {
                const auto axis = abs(index) - 1;
                const auto sign = index < 0 ? -1 : +1;
                return sign * wb_joystick_get_axis_value(axis);
            }

            static float readJoystickAxis(const int8_t index)
            {
                return normalizeJoystickAxis(readJoystickRaw(index));
            }

            joystick_t getJoystickInfo() 
            {
                return JOYSTICK_AXIS_MAP[wb_joystick_get_model()];
            }

            joystickStatus_e haveJoystick(void)
            {
                auto status = JOYSTICK_RECOGNIZED;

                auto joyname = wb_joystick_get_model();

                // No joystick
                if (joyname == NULL) {

                    static bool _didWarn;

                    if (!_didWarn) {
                        puts("Using keyboard instead:\n");
                        printKeyboardInstructions();
                    }

                    _didWarn = true;

                    status = JOYSTICK_NONE;
                }

                // Joystick unrecognized
                else if (JOYSTICK_AXIS_MAP.count(joyname) == 0) {

                    status = JOYSTICK_UNRECOGNIZED;
                }

                return status;
            }

            static void reportJoystick(void)
            {
                printf("Unrecognized joystick '%s' with axes ",
                        wb_joystick_get_model()); 

                for (uint8_t k=0; k<wb_joystick_get_number_of_axes(); ++k) {

                    printf("%2d=%+6d |", k+1, wb_joystick_get_axis_value(k));
                }
            }

            static void printKeyboardInstructions()
            {
                puts("- Use spacebar to take off\n");
                puts("- Use W and S to go up and down\n");
                puts("- Use arrow keys to move horizontally\n");
                puts("- Use Q and E to change heading\n");
            }

            void getDemands(demands_t & demands)
            {
                auto joystickStatus = haveJoystick();

                if (joystickStatus == JOYSTICK_RECOGNIZED) {

                    auto axes = getJoystickInfo();

                    demands.thrust =
                        normalizeJoystickAxis(readJoystickRaw(axes.throttle));

                    // Springy throttle stick; keep in interval [-1,+1]
                    if (axes.springy) {

                        static bool button_was_hit;

                        if (wb_joystick_get_pressed_button() == 5) {
                            button_was_hit = true;
                        }

                        _requested_takeoff = button_was_hit;

                        // Run throttle stick through deadband
                        demands.thrust =
                            fabs(demands.thrust) < 0.05 ? 0 : demands.thrust;
                    }

                    else {

                        static float throttle_prev;
                        static bool throttle_was_moved;

                        // Handle bogus throttle values on startup
                        if (throttle_prev != demands.thrust) {
                            throttle_was_moved = true;
                        }

                        _requested_takeoff = throttle_was_moved;

                        throttle_prev = demands.thrust;
                    }

                    demands.roll = readJoystickAxis(axes.roll);
                    demands.pitch = readJoystickAxis(axes.pitch); 
                    demands.yaw = readJoystickAxis(axes.yaw);
                }

                else if (joystickStatus == JOYSTICK_UNRECOGNIZED) {

                    reportJoystick();
                }

                else { 

                    getDemandsFromKeyboard(demands);

                }
            }

            void getDemandsFromKeyboard(demands_t & demands)
            {
                static bool spacebar_was_hit;

                switch (wb_keyboard_get_key()) {

                    case WB_KEYBOARD_UP:
                        demands.pitch = +1.0;
                        break;

                    case WB_KEYBOARD_DOWN:
                        demands.pitch = -1.0;
                        break;

                    case WB_KEYBOARD_RIGHT:
                        demands.roll = +1.0;
                        break;

                    case WB_KEYBOARD_LEFT:
                        demands.roll = -1.0;
                        break;

                    case 'Q':
                        demands.yaw = -1.0;
                        break;

                    case 'E':
                        demands.yaw = +1.0;
                        break;

                    case 'W':
                        demands.thrust = +1.0;
                        break;

                    case 'S':
                        demands.thrust = -1.0;
                        break;

                    case 32:
                        spacebar_was_hit = true;
                        break;
                }

                _requested_takeoff = spacebar_was_hit;
            }

            bool isSpringy()
            {
                return haveJoystick() == JOYSTICK_RECOGNIZED ?
                    getJoystickInfo().springy :
                    true; // keyboard
            }

    };

}
