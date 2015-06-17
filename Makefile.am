AUTOMAKE_OPTIONS = foreign

CXXFLAGS = -g -O3 -Wall -std=c++0x -gdwarf-2
#### -O0 -gdwarf-2 for debugging

####ACLOCAL_AMFLAGS = -I m4 --install

bin_PROGRAMS = simulator

simulator_SOURCES = 				 		 \
					core/main.cpp 			 \
					core/debug.cpp 			 \
					core/node.cpp 			 \
					core/queue.cpp 			 \
					core/packet.cpp 		 \
					core/event.cpp 			 \
					core/topology.cpp 		 \
					core/schedulinghost.cpp  \
					core/flow.cpp 			 \
					core/params.cpp 		 \
					core/random_variable.cpp \
					ext/factory.cpp 		 \
					ext/fountainflow.cpp 	 \
					ext/fastpassflow.cpp 	 \
					ext/fastpasshost.cpp 	 \
					ext/magichost.cpp 		 \
					ext/magicflow.cpp 		 \
					ext/capabilityflow.cpp 	 \
					ext/capabilityhost.cpp 	 \
					run/stats.cpp 			 \
					run/experiment.cpp 

