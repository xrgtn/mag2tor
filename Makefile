#CXXFLAGS=-std=c++11 -DBOOST_ASIO_SEPARATE_COMPILATION
#CXXFLAGS=-std=c++11 -DBOOST_ASIO_DYN_LINK
#since 2018-09-02:
#CXXFLAGS=-std=c++11
#since 2021-12-13:
PROJECT=mag2tor
CXXFLAGS=-std=c++14 -Wall -Wfatal-errors
LDFLAGS=-lstdc++ -lboost_system -ltorrent-rasterbar

all: $(PROJECT)

$(PROJECT): $(PROJECT).cpp
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@ -s

clean:
	$(RM) $(PROJECT)
