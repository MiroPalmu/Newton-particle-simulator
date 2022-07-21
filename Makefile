
CXX=g++-11
WARNINGS=-Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wpedantic -Wconversion
CPP_STANDARD = -std=c++20
CXXFLAGS=$(WARNINGS) $(CPP_STANDARD)
DB_CXXFLAGS=-pg $(WARNINGS) $(CPP_STANDARD)
LDFLAGS=-lfmt -lvulkan -ldl 

TARGET=nps
OBJECTFILES=main.o NPS.o
# External objectfiles are from libraries which are not compiled by this makefile
EXTERNAL_OBJECTFILES=/home/pamiro/koodaus/cpp/libs/kompute/build/src/libkompute.a

SHADERS=

all: target

#Implicit rule for compute shaders:
%.comp.spv: %.comp
	glslangValidator -V $^ -o $@


#Dependecies for implicit rules:
##Examples:
#vkcom.o : vkcom.hpp
#vecProd.o : vecProd.hpp
main.o : timer.hpp
#End of Dependecies for implicit rules 

0: CXXFLAGS+= -O0
0: target
0r: 0
	./$(TARGET)
1: CXXFLAGS+= -O1
1: target
1r: 1
	./$(TARGET)
2: CXXFLAGS+= -O2
2: target
2r: 2
	./$(TARGET)
3: CXXFLAGS+= -O3
3: target
3r: 3
	./$(TARGET)

debug: CXXFLAGS=$(DB_CXXFLAGS)
debug: target

target: $(OBJECTFILES) $(SHADERS)
	$(CXX) $(CXXFLAGS) $(OBJECTFILES) $(EXTERNAL_OBJECTFILES) -o $(TARGET) $(LDFLAGS)

test: target
	./$(TARGET)

.phony: clean init


init_gpu:
	sudo sysctl dev.i915.perf_stream_paranoid=0


clean:
	rm -f $(TARGET) $(OBJECTFILES) $(SHADERS) gmon.out
