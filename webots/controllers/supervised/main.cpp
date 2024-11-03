/* 
   C++ flight simulator kinematic program 

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

#include <sim/sim2.hpp>
#include <pids/altitude.hpp>
#include <pids/yaw_rate.hpp>

static const float YAW_PRESCALE = 160; // deg/sec

static const float THRUST_BASE = 55.385;

static const float THROTTLE_DOWN = 0.06;

static const float THROTTLE_DEADBAND = 0.2;

static const float PITCH_ROLL_POST_SCALE = 50;

// For springy-throttle gamepads / keyboard
static const float INITIAL_ALTITUDE_TARGET = 0.2;
static const float CLIMB_RATE_SCALE = 0.01;

static bool _run_altitude_pid;

static bool _reset_pids;

namespace hf {

    static AltitudePid _altitudePid;

    static YawRatePid _yawRatePid;

    void run_closed_loop_controllers(
            const float dt, const state_t & state, demands_t & demands)
    {
        if (_run_altitude_pid) {
            _altitudePid.run(dt, state, demands);
        }

        _yawRatePid.run(dt, _reset_pids, state, demands);
    }
}

int main(int argc, char ** argv)
{
    auto * logfp = fopen("log.csv", "w");

    (void)argc;
    (void)argv;

    hf::Simulator sim = {};

    sim.init();

    // This initial value will be ignored for traditional (non-springy)
    // throttle
    float z_target = INITIAL_ALTITUDE_TARGET;

    _run_altitude_pid = true;

    hf::demands_t demands = {};

    while (true) {

        if (!sim.step(demands)) {
            break;
        }

        auto open_loop_demands = sim.getDemands();

        const auto state = sim.getState();

        demands.yaw = open_loop_demands.yaw * YAW_PRESCALE;

        _reset_pids = demands.thrust < THROTTLE_DOWN;

        // Throttle control begins when once takeoff is requested, either by
        // hitting a button or key ("springy", self-centering throttle) or by
        // raising the non-self-centering throttle stick
        if (sim.requestedTakeoff()) {

            // "Springy" (self-centering) throttle or keyboard: accumulate 
            // altitude target based on stick deflection, and attempt
            // to maintain target via PID control
            if (sim.isSpringy()) {

                z_target += CLIMB_RATE_SCALE * open_loop_demands.thrust;

                demands.thrust = z_target;
            }

            // Traditional (non-self-centering) throttle: 
            //
            //   (1) In throttle deadband (mid position), fix an altitude target
            //       and attempt to maintain it via PID control
            //
            //   (2) Outside throttle deadband, get thrust from stick deflection
            else {

                if (fabs(open_loop_demands.thrust) < THROTTLE_DEADBAND) {

                    demands.thrust = state.z;

                    _run_altitude_pid = true;
                }

                else {

                    demands.thrust = open_loop_demands.thrust;

                    _run_altitude_pid = false;
                }
            }

            const auto motors = sim.getMotors();

            fprintf(logfp, "%f,%f,%f,%f,%+f\n",
                    motors.m1, motors.m2,motors.m3, motors.m4, state.psi);

        }    
    }

    sim.close();

    return 0;
}
