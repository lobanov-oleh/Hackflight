/* 
 * Custom physics plugin for Hackflight simulator using Haskell Copilot for
 * control
 *
 *  Copyright (C) 2024 Simon D. Levy
 *
 *  This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http:--www.gnu.org/licenses/>.
 */

#include "../support.hpp"

// Global data and routines shared with Haskell Copilot ----------------------

float stream_time;

float stream_dt;

float stream_throttle;
float stream_roll;
float stream_pitch;
float stream_yaw;

float stream_dx;
float stream_dy;
float stream_z;
float stream_dz;
float stream_phi;
float stream_dphi;
float stream_theta;
float stream_dtheta;
float stream_psi;
float stream_dpsi;

bool stream_requestedTakeoff;

static float motors[4];

void runMotors(float m1, float m2, float m3, float m4)
{
    motors[0] = m1;
    motors[1] = m2;
    motors[2] = m3;
    motors[3] = m4;
}

void copilot_step_core();

// ---------------------------------------------------------------------------

DLLEXPORT void webots_physics_step() 
{
    if (_robotBody == NULL) {
        return;
    }

    const auto siminfo = getSimInfo();

    // This happens at startup
    if (siminfo.framerate == 0) {
        return;
    }

    // Count elapsed time since takeoff, for climb-rate PID control
    static uint32_t _frame_count;
    stream_time = siminfo.requested_takeoff ?
        _frame_count++ / siminfo.framerate : 0;

    // Run control in outer loop
    for (uint32_t j=0; j < (1 / siminfo.framerate * PID_FREQ); ++j) {

        stream_requestedTakeoff = siminfo.requested_takeoff;

        stream_throttle = siminfo.demands.thrust;
        stream_roll = siminfo.demands.roll;
        stream_pitch = siminfo.demands.pitch;
        stream_yaw = siminfo.demands.yaw;

        // Get simulated gyro
        const auto gyro = dynamics.readGyro();

        // XXX Cheat on remaining sensors for now
        const auto pose = dynamics.getPose();
        const auto dxdy = dynamics.getGroundTruthHorizontalVelocities();

        const auto r = hf::Utils::RAD2DEG;

        stream_dx = dxdy.x;

        stream_dy = dxdy.y;

        stream_z = pose.z;

        stream_dz = dynamics.getGroundTruthVerticalVelocity();

        stream_phi = r * pose.phi;

        stream_dphi = gyro.x;

        stream_theta = r * pose.theta;

        stream_dtheta = gyro.y;

        stream_psi = r * pose.psi;

        stream_dpsi = gyro.z;

        stream_dt  = 1. / PID_FREQ;

        // Run dynamics in inner loop to update state with motors
        hf::BfQuadXMixer mixer = {};
        for (uint32_t k=0; k<DYNAMICS_FREQ / PID_FREQ; ++k) {
            dynamics.update(motors, &mixer);
        }

        copilot_step_core();
    }

    setPose(dynamics);
}
