#ifndef JUNCTION_MAP
#define JUNCTION_MAP

// #include <unordered_map>
#include <unordered_set>
#include <string>
#include "Kmer.h"
#include "Junction.h"
#include "Cap.h"
#include "ReadKmer.h"
#include "Bloom.h"
#include "JChecker.h"
#include "Kmer.h"
#include "JuncPairs.h"

#include "../src/Contig.h"
#include "../src/ContigNode.h"
#include "../src/ContigGraph.h"
#include <fstream>
#include "../src/BfSearchResult.h"
using std::ofstream;
// using std::unordered_map;
using std::string;
using std::unordered_set;
// #include "sparsepp.h"
// using spp::sparse_hash_map;


class JunctionMap{

private: 
    Bloom* bloom;
    JChecker* jchecker; 
    int maxReadLength; //needed for finding sinks properly- tells you when to stop scanning   

    
public:
    void printDistAndExtension(int dist, int maxDist, int index, kmer_type kmer);
 
    void buildLinearRegions(ContigGraph* contigGraph); //Builds node graph for any connected component that has branching
    void buildBranchingPaths(ContigGraph* contigGraph); //For connected components that have no branching at all- builds contig graph
    void destroyComplexJunctions(); //destroys all complex junctions. used after building branching paths for contig graph
    void destroyJunctionSet(std::set<kmer_type> dead_juncs); // periodically destroy complex junctions - aimed to reduce memory use


    //Builds a contig graph from this junction map, destroying the non-complex junctions as it goes
    ContigGraph* buildContigGraph();

    //Gets the contig from this junction to the next complex junction or sink
    //Has all fields except the ContigNode pointers filled out- indices, juncDistances, and seq are all there
    Contig* getContig(Junction junc, kmer_type startKmer, int index);

    //Scans forward from junction junc at index i with bloom filter
    //If it hits another junction at or before the distance specified by the given junction, returns a "node" result with that junction
    //If it does not, it keeps scanning until it hits another junction or an actual sink
    //If it hits a sink, it returns it.  If it hits a junction, it tests how far that junction points along the path.
    //Based on the indicated overlap, it either decides the entire intermediate sequence is real or the connection is a 
    //false positive connection.  Then returns either a sink or a node result.
    BfSearchResult findNeighbor(Junction junc, kmer_type startKmer, int index);
    
    std::unordered_map<kmer_type,Junction> junctionMap;  //stores the junctions themselves
    // sparse_hash_map<kmer_type,Junction> junctionMap;  //stores the junctions themselves

    //Returns true if multiple extensions of the given kmer jcheck
    //Assumes the given kmer is in the BF
    bool isBloomJunction(kmer_type kmer);

    //Gets the valid extension of the given kmer based on the bloom filter and cFPs.  Uses JChecking! so this cuts off tips
    //Assume the given kmer is not a junction
    //Returns -1 if there is no valid extension
    //Returns -2 if there are multiple
    //ASSUMES NO CFP SET- since this is only done in findSinks, BEFORE the cFPs are found
    int getValidJExtension(DoubleKmer kmer);

    //File format:
    //One line for each junction.  On each line, the kmer is printed as a string, then the junction is printed.  
    //See Junction.h for junction print documentation.
    void writeToFile(string filename); 

    void buildFromFile(string junction_file);
    
    //Finds the junction associated with the given kmer and returns how far we can skip in the given direction from that junction
    int getSkipDist(ReadKmer* readKmer, bool direction);

    //Directly links two adjacent junctions from the same read
    void directLinkJunctions(ReadKmer* kmer1, ReadKmer* kmer2, Junction* junc1, Junction* junc2);

    int getNumComplexJunctions(); //Gets the number of junctions with more than one valid extension
    int getNumSolidJunctions(int i); //Gets the number of solid complex junctions, multiple valid extensions of coverage at least i
    int getNumJunctions();

    void createJunction(kmer_type kmer);
    void createJunction(ReadKmer* readKmer);
    bool isJunction(kmer_type kmer); //returns true if there is a junction at the given kmer
    bool isJunction(ReadKmer* readKmer); //same as above
    Junction* getJunction(ReadKmer kmer); //returns the junction located at the given kmer, or NULL if there is none
    Junction* getJunction(kmer_type kmer); //same as above
    void killJunction(kmer_type kmer); //removes the junction at the specified kmer, if there is one

    JunctionMap(Bloom* bloo, JChecker* jchecker, int maxReadLength);
};
#endif