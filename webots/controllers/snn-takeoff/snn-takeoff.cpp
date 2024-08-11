/*
  C++ flight simulator takeoff example for Hackflight
 
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

#include <sim.hpp>
#include <snn.hpp>
//#include <socket_server.hpp>

static const int VIZ_PORT = 8100;

int main(int argc, char ** argv)
{
    // Create a simulator object for Webots functionality 

    hf::Simulator sim = {};

    sim.init();

    SNN * snn = NULL;

    // Load up the network specified in the command line

    if (argc < 2) {
        fprintf(stderr, "Usage: %s RISP_NETWORK\n", argv[0]);
        exit(1);
    }

    try {
        snn = new SNN(argv[1], "risp");
    } catch (const SRE &e) {
        fprintf(stderr, "Couldn't set up SNN:\n%s\n", e.what());
        exit(1);
    }

    // Serve up a socket for the visualizer
    //printf("Listening for viz client on port %d ...", VIZ_PORT);
    //fflush(stdout);

    //const auto viz_client = socket_serve(VIZ_PORT);

    //printf("\nClient connected\n");

    while (true) {

        hf::state_t state = {};

        if (!sim.step(state)) {
            break;
        }

        vector<double> observations = {state.z, state.dz};
        vector <double> actions;
        snn->step(observations, actions);
        const auto motor = actions[0];

        vector<int> counts;
        snn->get_counts(counts);

        char message[100];

        sprintf(message,
            "{\"Event Counts\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d],"
            "\"Neuron Alias\":[0,1,2,5,6,9,10,15,18,53,66]}\n",
            counts[0], 
            counts[1], 
            counts[2], 
            counts[3], 
            counts[4], 
            counts[5], 
            counts[6], 
            counts[7], 
            counts[8], 
            counts[9], 
            counts[10]);

        socket_write(viz_client, message);

        sim.setMotors(motor, motor, motor, motor);
    }

    wb_robot_cleanup();

    return 0;
}
