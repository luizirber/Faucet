CFLAGS+= -O4 -D_FILE_OFFSET_BITS=64 # needed to handle files > 2 GB on 32 bits systems
SRC=../JChecker.cpp ../Bloom.cpp ../Kmer.cpp TestUtils.cpp JunctionTests.cpp JunctionMapTests.cpp JCheckTests.cpp KmerTests.cpp RollingHashTests.cpp BloomTests.cpp RunTests.cpp
EXEC=miniaTests
OBJ= $(SRC:.cpp=.o)

all:
	$(MAKE) $(EXEC)

miniaTests: $(OBJ) RunTests.cpp
	g++ --std=c++0x $(SRC) -o RunTests 

%.o: %.cpp %.h
	g++ --std=c++0x -o $@ -c $< $(CFLAGS)

install:
	cp miniaTests /usr/local/bin
