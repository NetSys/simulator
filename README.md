YAPS = Yet Another Packet Simulator
==================================

Core stuff is in `coresim/` 
---------------------------
Normally these files shouldn't change. This directory includes implementations of the following:
* Main event loop and related helper functions, global variables, main() function to determine which experiment to run: `main.cpp`
    * Note: deciding which experiment to run will eventually be moved to the `run/` directory, probably to `experiment.cpp`.
* Core event implementations (`Event`, `FlowArrivalEvent`, `FlowFinishedEvent`, etc): `event.cpp`.
* Representation of the topology: `node.cpp`, `topology.cpp`
* Queueing behavior. This is a basis for extension; the default implementation is FIFO-dropTail: `queue.cpp`.
* Flows and packets. This is also a basis for extension; default is TCP: `packet.cpp` and `flow.cpp`.
* Random variables used in flow generation. Used as a library by the flow generation code: `random_variable.cpp`.

Extensions are in `ext/`
-----------------------
This is where you implement your favorite protocol.
* Generally extensions are created by subclassing one or more aspects of classes defined in `coresim/`.
* Once an extension is defined, it should be added to `factory.cpp` so it can be run. 
    * Currently, `factory.cpp` supports changing the flow, queue, and host-scheduling implementations.
* Methods in `coresim/` call the `get_...` methods in `factory.cpp` to initialize the simulation with the correct implementation.
* Which implementation to use from `factory.cpp` is determined by the config file, parsed by `run/params.cpp`.
    * You should give your extension an identifier in `factory.h` so it can be uniquely identified in the config file.

Stuff related to actually running the simulator is in `run/`
------------------------------------------------------------
* Experiment setup, running, and post-analysis: `experiment.cpp`
* Flow generation models: `flow_generator.cpp`
* Parsing of config file: `params.cpp`
    * Configuration parameters for your extension should be added to `params.h` and `params.cpp`.
    * These can then be accessed with `params.<your_parameter>`

Helper scripts to run experiments are in `py/`
---------------------------------------------
This can be useful if:
* You are running many experiments in parallel.
* You want to easily generate configuration files.

To compile, the Automake and Autoconf files are included: `configure.ac` and `Makefile.am`. The makefile will produce two targets: `simulator` and `simdebug`. 
`simdebug` is equivalent to `simulator`, except compiler optimzations are turned off to make debugging easier.

Authors
-------

* Gautam Kumar
* Akshay Narayan
* Peter Gao

![Our Project Mascot](yaps-mascot.png)

