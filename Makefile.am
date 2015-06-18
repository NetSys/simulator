AUTOMAKE_OPTIONS = foreign

CXXFLAGS = -g -O3 -Wall -std=c++0x -gdwarf-2
#### -O0 -gdwarf-2 for debugging

####ACLOCAL_AMFLAGS = -I m4 --install

bin_PROGRAMS = simulator

simulator_SOURCES = 				 		 	 \
					coresim/main.cpp 			 \
					coresim/debug.cpp 			 \
					coresim/node.cpp 			 \
					coresim/queue.cpp 			 \
					coresim/packet.cpp 		 	 \
					coresim/event.cpp 			 \
					coresim/topology.cpp 		 \
					coresim/flow.cpp 			 \
					coresim/random_variable.cpp  \
					ext/factory.cpp 		 	 \
					ext/pfabricqueue.cpp         \
					ext/pfabricflow.cpp          \
					ext/schedulinghost.cpp	     \
					ext/fountainflow.cpp 	   	 \
					ext/magichost.cpp 		   	 \
					ext/magicflow.cpp 		   	 \
					ext/capabilityflow.cpp 	   	 \
					ext/capabilityhost.cpp 	   	 \
					ext/fastpassflow.cpp 	   	 \
					ext/fastpasshost.cpp 	   	 \
					ext/fastpassTopology.cpp	 \
					run/params.cpp 		 	 	 \
					run/stats.cpp 			   	 \
					run/flow_generator.cpp       \
					run/experiment.cpp 

