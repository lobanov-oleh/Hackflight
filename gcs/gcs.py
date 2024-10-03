#!/usr/bin/python3
'''
Hackflight Ground Control Station

This file is part of Hackflight.

Hackflight is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

Hackflight is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
Hackflight. If not, see <https://www.gnu.org/licenses/>.
'''

from inputs import get_gamepad


def main():

    axis_map = {'X': 0, 'Y': 1, 'Z': 2, 'RX': 3, 'RY': 4, 'RZ':5}

    while True:

        try:

            events = get_gamepad()

            for event in events:

                code = str(event.code)

                if 'ABS' in code:

                    print(axis_map[code[4:]], event.state)

        except KeyboardInterrupt:

            break

main()
