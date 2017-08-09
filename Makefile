#CXXFLAGS=-std=c++11 -DBOOST_ASIO_SEPARATE_COMPILATION
CXXFLAGS=-std=c++11 -DBOOST_ASIO_DYN_LINK
LDFLAGS=-lstdc++ -lboost_system -ltorrent-rasterbar

mag2tor: mag2tor.cpp
