###########################
#	Makefile
###########################

#source object target
SOURCE 	:= 
OBJS 	:= process_pool.o filesystem.o mapreduce.o
TARGET	:= test  

#compile and lib parameter
CXX		:= g++
LIBS	:= 
LDFLAGS	:= -lpthread 
DEFINES	:=
CFLAGS	:= -g
CXXFLAGS:= -std=c++11
 
.PHONY: clean

#link parameter
LIB := 

#link
$(TARGET): $(OBJS) test.cpp
	$(CXX) -o $@ $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) $^

process_pool.o: process_pool.cpp process_pool.h
	$(CXX)  $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) -c $^

mapreduce.o: mapreduce.cpp mapreduce.h
	$(CXX)  $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) -c $^

filesystem.o: filesystem.h filesystem.cpp
	$(CXX)  $(CFLAGS) $(CXXFLAGS) $(LDFLAGS) -c $^

#clean
clean:
	rm -fr *.o
	rm -fr *.gch
	rm -fr test
	rm -fr file/inter*
	rm -fr file/region*
	rm -fr file/sort*
	rm -fr file/result*

