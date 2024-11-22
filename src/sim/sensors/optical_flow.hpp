/*
 *   Simulated optical flow sensor
 *
 *
 *   For equations see
 *
 *     https://www.bitcraze.io/documentation/repository/crazyflie-firmware/
 *      master/images/flowdeck_velocity.png
 *
 *   Copyright (C) 2024 Simon D. Levy
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, in version 3.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http:--www.gnu.org/licenses/>.
 */

#include <hackflight.hpp>
#include <utils.hpp>

namespace hf {

    class OpticalFlow {

        public:

            // https://wiki.bitcraze.io/_media/projects:crazyflie2:
            //    expansionboards:pot0189-pmw3901mb-txqt-ds-r1.40-280119.pdf
            static constexpr float FIELD_OF_VIEW = 42;

            // https://github.com/bitcraze/Bitcraze_PMW3901
            static constexpr float NPIX = 35;

            static axis2_t read(const Dynamics & d, const float h)
            {
                const auto dx =   d._x2 * cos(d._x11) - d._x4 * sin(d._x11);

                const auto dy = -(d._x2 * sin(d._x11) + d._x4 * cos(d._x11));

                const auto theta = 2 * sin(Utils::DEG2RAD * FIELD_OF_VIEW / 2);

                const auto flow_dx = h > 0 ? 
                    d._dt * NPIX * (h * d._x10 + dx) / (h * theta) : 0;

                const auto flow_dy = h > 0 ? 
                    d._dt * NPIX * (h * d._x8 + dy) / (h * theta) : 0;

                return axis2_t {flow_dx, flow_dy};
            }
    };

}