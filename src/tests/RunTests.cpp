#include "RunTests.h" 

//g++ ../Bloom.cpp ../Kmer.cpp ../Debloom.cpp JCheckTests.cpp KmerTests.cpp TestUtils.cpp RunTests.cpp -o RunTests

int main(int argc, char *argv[]){

    findNextJunctionTests::runFindNextJunctionTests();
    findReadJunctionsTests::runFindReadJunctionsTests();
    return 0;
}