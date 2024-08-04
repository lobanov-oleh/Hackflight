/*
  C++ flight simulator support for Hackflight
 
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

#include <webots.hpp>

#include <hackflight.hpp>
#include <mixers.hpp>
#include <utils.hpp>

#include <pids/altitude.hpp>
#include <pids/climb_rate.hpp>
#include <pids/pitch_roll_angle.hpp>
#include <pids/pitch_roll_rate.hpp>
#include <pids/position.hpp>
#include <pids/yaw_angle.hpp>
#include <pids/yaw_rate.hpp>

static const float PITCH_ROLL_ANGLE_KP = 6e0;

static const float PITCH_ROLL_RATE_KP = 1.25e-2;
static const float PITCH_ROLL_RATE_KD = 0; //1.25e-4;

static const float YAW_RATE_KP = 1.20e-2;

// Motor thrust constants for climb-rate PID controller
static const float TBASE = 56;
static const float TSCALE = 0.25;
static const float TMIN = 0;

static const float INITIAL_ALTITUDE_TARGET = 0.2;

// We consider throttle inputs above this below this value to be
// positive for takeoff
static constexpr float THROTTLE_ZERO = 0.05;

static constexpr float THROTTLE_SCALE = 0.005;

// We consider altitudes below this value to be the ground
static constexpr float ZGROUND = 0.05;

// Arbitrary time constexprant
static constexpr float DT = .01;

typedef enum {

    STATUS_LANDED,
    STATUS_TAKING_OFF,
    STATUS_FLYING

} flyingStatus_e;

int main(int argc, char ** argv)
{
    hf::PositionController positionController= {};
    hf::PitchRollAngleController pitchRollAngleController= {};
    hf::PitchRollRateController pitchRollRateController= {};
    hf::AltitudeController altitudeController= {};
    hf::YawAngleController yawAngleController= {};
    hf::YawRateController yawRateController= {};
    hf::ClimbRateController climbRateController= {};

    hf::Simulator sim = {};

    sim.init();

    while (true) {

        hf::state_t state = {};

        hf::demands_t stickDemands = {};

        if (!sim.step(stickDemands, state)) {
            break;
        }

        static flyingStatus_e _status;

        static float _altitude_target;

        hf::quad_motors_t motors = {};

        _altitude_target =
            _status == STATUS_FLYING ? 
            _altitude_target + THROTTLE_SCALE * stickDemands.thrust :
            _status == STATUS_LANDED ?
            INITIAL_ALTITUDE_TARGET :
            _altitude_target;

        _status = 

            _status == STATUS_TAKING_OFF  && state.z > ZGROUND ?  
            STATUS_FLYING :

            _status == STATUS_FLYING && state.z <= ZGROUND ?  
            STATUS_LANDED :

            _status == STATUS_LANDED && 
            stickDemands.thrust > THROTTLE_ZERO ? 
            STATUS_TAKING_OFF :

            _status;

        const auto landed = _status == STATUS_LANDED;

        hf::demands_t demands = { 
            stickDemands.thrust,
            stickDemands.roll,
            stickDemands.pitch,
            stickDemands.yaw
        };

        positionController.run(state, DT, demands);

        pitchRollAngleController.run(
                PITCH_ROLL_ANGLE_KP, state, DT, demands);

        pitchRollRateController.run(PITCH_ROLL_RATE_KP,
                PITCH_ROLL_RATE_KD, state, DT, demands);

        altitudeController.run(state, DT, _altitude_target, demands);

        yawAngleController.run(state, DT, demands);

        yawRateController.run(YAW_RATE_KP, state, DT, demands);

        climbRateController.run(state, DT, TBASE, TSCALE, TMIN,
                !landed, demands);

        // Run mixer to convert demands to motor spins
        hf::Mixer::runCF(demands, motors);

        sim.setMotors(motors.m1, motors.m2, motors.m3, motors.m4);
    }

    sim.close();

    return 0;
}
