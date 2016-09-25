#include "Contig.h"
#include <fstream>
#include <sstream>
#include <algorithm>    // std::reverse
#include <vector>       // std::vector

using std::stringstream;
using std::ofstream;

//Looks at all junction pairs on this contig, and prints a histogram of how many BF positives and BF negatives there are
//for pairs at different distances.
void Contig::printPairStatistics(Bloom* pair_filter){
	std::list<JuncResult> results = getJuncResults(1, 0, 3*length());
	std::cout << "Length " << length() << ", results " << results.size() << "\n";
	const int maxDist = 2000;
	const int increment = 20;
	int posNegPairCounts [2][maxDist/increment] = {};

	for(int i = 0; i < maxDist/increment; i++){
		posNegPairCounts[0][i] = 0;
		posNegPairCounts[1][i] = 0;
	}

	for(auto itL = results.begin(); itL != results.end(); itL++){
		for(auto itR = itL; itR != results.end(); itR++){
			int index = (itR->distance - itL->distance)/increment;
			if(index < maxDist/increment && index >= 0){
				if(pair_filter->containsPair(JuncPair(itL->kmer, itR->kmer))){
					posNegPairCounts[0][index] += 1;
				}
				else{
					posNegPairCounts[1][index] += 1;
				}
			}
		}
	}

	printf("Pair pos/neg char, aggregated over buckets of length %d:\n", increment);
	for(int i = 0; i < maxDist / increment; i++){
		std::cout << "Distance " << i*increment << ": ";
		std::cout << posNegPairCounts[0][i] << ",";
		std::cout << posNegPairCounts[1][i] << "\n";
	}
}

//Reverses if needed to get "canonical" concatenation of two in the same direction
//Reverses again at the end to ensure no mutation of contigs
Contig* Contig::concatenate(Contig* otherContig, int thisSide, int otherSide){
	if(thisSide == 1){
		reverse();
	}
	if(otherSide == 2){
		otherContig->reverse();
	}
	Contig* concatenation =  concatenate(otherContig);
	if(thisSide == 1){
		reverse();
	}
	if(otherSide == 2){
		otherContig->reverse();
	}
	return concatenation;
}

//utility for linking them if they're both facing "forward"
Contig* Contig::concatenate(Contig* otherContig){
	Contig* result = new Contig();
	result->setEnds(node1_p, ind1, otherContig->node2_p, otherContig->ind2);
	if(getSeq().length() < sizeKmer){
		printf("ERROR: seq less than k long in Contig::Concatenate.\n");
	}
	result->setContigJuncs(contigJuncs.concatenate(otherContig->contigJuncs));
	return result;
}

void Contig::reverse(){
	{ContigNode * temp = node1_p;
		node1_p = node2_p;
		node2_p = temp;}

	{int temp = ind1;
		ind1 = ind2;
		ind2 = temp;}

	contigJuncs.reverse();
}

void Contig::setEnds( ContigNode* n1, int i1, ContigNode* n2, int i2){
	node1_p = n1;
	node2_p = n2;
	setIndices(i1, i2);
	if(node1_p){
		node1_p->contigs[i1] = this;
	}
	if(node2_p){
		node2_p->contigs[i2] = this;
	}
}

//Gets all of the interior junctions on this contig, as a list of JuncResult objects
//Assumes this is startDist away from the real start, so increments all by startDist
//Side refers to which side of the contig to start from
std::list<JuncResult> Contig::getJuncResults(int side, int startDist, int maxDist){
	if(side == 2){
		reverse();
	}
	auto result = contigJuncs.getJuncResults(ind1 != 4, startDist, maxDist); //forward if ind1 != 4, backward if ind1 == 4
	if(side == 2){
		reverse();
	}
	return result;
}

int Contig::length(){
	return contigJuncs.length();
}

double Contig::getAvgCoverage(){
	return contigJuncs.getAvgCoverage();
}

double Contig::getAvgCoverage(std::list<JuncResult> results){
	return contigJuncs.getAvgCoverage(results);
}

double Contig::getCoverageSampleVariance(){
	return contigJuncs.getCoverageSampleVariance();
}

double Contig::getCoverageSampleVariance(std::list<JuncResult> results){
	return contigJuncs.getCoverageSampleVariance(results);
}

float Contig::getMass(){
	return getAvgCoverage()*getSeq().length();
}

void Contig::setIndices(int i1, int i2){
	ind1 = i1;
	ind2 = i2;
}

int Contig::getMinIndex(){
	return std::min(ind1, ind2);
}

ContigNode* Contig::otherEndNode(ContigNode * oneEnd){
	if(node1_p == oneEnd){
		return node2_p;
	}
	if(node2_p == oneEnd){
		return node1_p;
	}
	printf("ERROR: tried to get other end of a contig, but the given pointer didn't point to either end!.\n");
	std::cout << "node1_p: " << node1_p << " node2_p: " << node2_p << " oneEnd: " << oneEnd << "\n";
	std::cout << "This contig: " << this << "\n";
	return nullptr;
}

//Assumes the given contig node points to one end of this contig
kmer_type Contig::getNodeKmer(ContigNode * contigNode){
	if(node1_p == contigNode){
		return getSideKmer(1);
	}
	if(node2_p == contigNode){
		return getSideKmer(2);
	}
	printf("ERROR: tried to get the kmer corresponding to a node not adjacent to this contig from this contig.\n");
}

ContigNode* Contig::getNode(int side){
	if (side == 1){
		return node1_p;
	}
	if(side == 2){
		return node2_p;
	}
	printf("ERROR: called getNode on contignode with side other than 1,2\n");
}

int Contig::getIndex(int side){
	if (side == 1){
		return ind1;
	}
	if(side == 2){
		return ind2;
	}
	printf("ERROR: called getSide on contignode with side other than 1,2\n");
}

//Gets kmer for node1_p if side == 1, node2_p if side == 2
kmer_type Contig::getSideKmer(int side){
	if(side == 1){
		kmer_type kmer = getKmerFromRead(getSeq(), 0);
		if(ind1 == 4) return revcomp(kmer);
		return kmer;
	}
	if(side == 2){
		kmer_type kmer = getKmerFromRead(getSeq(), getSeq().length()-sizeKmer);
		if(ind2 == 4) return kmer;
		return revcomp(kmer);
	}
	printf("ERROR: tried to get a kmer corresponding to a side other than one or two from a contig.\n");
}

int Contig::getSide(ContigNode* node){
	if(node1_p == node){
		return 1;
	}
	if(node2_p == node){
		return 2;
	}
	printf("ERROR: tried to get the side of a contig node not adjacent to the contig.\n");
	std::cout << "Node1: " << node1_p << ", Node2: " << node2_p << " Input: " << node << "\n";
	return -1;
}

int Contig::getSide(ContigNode* node, int index){
	if((node1_p == node) && (ind1 == index)){
		return 1;
	}
	if((node2_p == node) && (ind2 == index)){
		return 2;
	}
	printf("ERROR: tried to get the side of a contig node,index pair, but didn't find it on either side.\n");
	std::cout << "Node1: " << node1_p << ", Node2: " << node2_p << " Input: " << node << "\n";
	return -1;
}

void Contig::setSide(int side, ContigNode* node){
	if(side == 1){
		node1_p = node;
	}
	else if(side == 2){
		node2_p = node;
	}
	else printf("ERROR: tried to set side for side other than 1,2.\n");	
}

bool Contig::isIsolated(){
	return ((node1_p == nullptr) && (node2_p == nullptr));
}

std::vector<std::pair<Contig*, bool>> Contig::getNeighbors(bool RC){
	if(!RC){ //forward node continuations 
	    if(node2_p){ //if node exists in forward direction 
	    	return node2_p->getFastGNeighbors(ind2);
		}
	}
	else{ //backward node continuations
		if(node1_p){ //if node exists in backward direction
			return node1_p->getFastGNeighbors(ind1);
		}
	}
	return {};
}

bool Contig::isDegenerateLoop(){
	if (node1_p && node2_p){
		return (node1_p == node2_p && ind1 == ind2);
	}
	return false;
}

bool Contig::checkValidity(){
	std::cout << "ind1 " << ind1 << ", ind2 " << ind2 << std::endl;
	if(node1_p){
		std::cout << "there is a node 1 ptr\n";
		if(node1_p->contigs[ind1] != this){
			printf("CONTIG_ERROR: adjacent node 1 at specified index doesn't point back to this contig.\n");
			std::cout << "Expected at extension "<< ind1 << "\n";	
			std::cout << "node1_p is " << node1_p << std::endl;			
			std::cout << "contig is " << this << ", pointed to is " << node1_p->contigs[ind1] << std::endl;
			for (int i = 0; i<5; i++){
				if (node1_p->contigs[i] == this){
					std::cout << "contig is actually at extension " << i << std::endl;
					std::cout << "other ind is " << ind2 << std::endl;
				}
			}	
			// return false;
		}
		if(getSide(node1_p, ind1) != 1 && !isDegenerateLoop()){
			printf("CONTIG_ERROR: getSide incorrect on node1p, ind1.\n");
			std::cout << "Node1: " << node1_p << ", Ind1: " << ind1 << ", Side: " << getSide(node1_p, ind1) << "\n";
			std::cout << "Node2: " << node2_p << ", Ind2: " << ind2 << ", Side: " << getSide(node2_p, ind2) << "\n";
			// return false;
		}
	}
	if(node2_p){
		std::cout << "there is a node 2 ptr\n";
		if(node2_p->contigs[ind2] != this){
			printf("CONTIG_ERROR: adjacent node 2 at specified index doesn't point back to this contig.\n");
			std::cout << "Expected at extension "<< ind2 << "\n";	
			std::cout << "node2_p is " << node2_p << std::endl;			
			std::cout << "contig is " << this << ", pointed to is " << node2_p->contigs[ind2] << std::endl;
			for (int i = 0; i<5; i++){
				if (node2_p->contigs[i] == this){
					std::cout << "contig is actually at extension " << i << std::endl;
					std::cout << "other ind is " << ind1 << std::endl;
				}
			}		
			// return false;
		}
		if(getSide(node2_p, ind2) != 2 && !isDegenerateLoop()){
			printf("CONTIG_ERROR: getSide incorrect on node2p, ind2.\n");
			std::cout << "Node1: " << node1_p << ", Ind1: " << ind1 << ", Side: " << getSide(node1_p, ind1) << "\n";
			std::cout << "Node2: " << node2_p << ", Ind2: " << ind2 << ", Side: " << getSide(node2_p, ind2) << "\n";
			// return false;
		}
	}
	if (node1_p && node2_p){
		if (node1_p->contigs[ind1]->getSeq() != node2_p->contigs[ind2]->getSeq()){
			std::cout << "different contig sequences\n";
			std::cout << "node 1 contig length " << node1_p->contigs[ind1]->getSeq().length() << ", " <<
			"node 2 contig length " << node2_p->contigs[ind2]->getSeq().length() <<std::endl;		
		}
		if (node1_p->contigs[ind1]->getSeq() == revcomp_string(node2_p->contigs[ind2]->getSeq())){
			std::cout << "contig sequences are RCs\n";
		}
		if (node1_p->contigs[ind1]->getSeq() == revcomp_string(node1_p->contigs[ind1]->getSeq())){
			std::cout << "node1 contig is palindrome\n";
		}
		if (node2_p->contigs[ind2]->getSeq() == revcomp_string(node2_p->contigs[ind2]->getSeq())){
			std::cout << "node2 contig is palindrome\n";
		}
		
		if (node1_p == node2_p){
			std::cout << "nodes are equal\n";
		}else{
			std::cout << "nodes are not equal\n";
		}

		if(node1_p->contigs[ind1] != this){
			std::cout << "node 1 doesn't match\n";
		}else{
			std::cout << "node 1 does match\n";
		}
		if(node2_p->contigs[ind2] != this){
			std::cout << "node 2 doesn't match\n";
		}else{
			std::cout << "node 2 does match\n";
		}


	}
	std::cout << std::endl;
	return true;
	
}

string Contig::getFastGName(bool RC){
	stringstream stream;
    stream << "NODE_" << this << "_length_" << getSeq().length() << "_cov_" << getAvgCoverage();
    if(RC){
    	stream << "'";
    }
    return stream.str();
}

string Contig::getFastGHeader(bool RC){
	stringstream stream;
	stream << ">";
    stream << getFastGName(RC);

    //get neighbors in direction corresponding to RC value
    std::vector<std::pair<Contig*, bool>> neighbors = getNeighbors(RC);

    //if empty return now
    if(neighbors.empty()){
    	stream << ";" ;
    	return stream.str();
    }

    //not empty, add neighbors to line
    stream << ":";
    for(auto it = neighbors.begin(); it != neighbors.end(); ++it){
    	Contig* neighbor = it->first;
    	bool RC = it->second;
    	stream << neighbor->getFastGName(RC) << ",";
    }
    string result = stream.str();
    result[result.length()-1] = ';';
    return result;
}

string Contig::getStringRep(){
	stringstream stream;
    stream << node1_p << "," << ind1 << " " << node2_p << "," << ind2 << "\n";
    stream << contigJuncs.getStringRep();
    stream << "\n";
    return stream.str();
}

Contig::Contig(){
	setSeq("");
	node1_p = nullptr;
	node2_p = nullptr;
	ind1 = -1;
	ind2 = -1;
	contigJuncs = ContigJuncList();
}

Contig::~Contig(){
	node1_p = nullptr;
	node2_p = nullptr;
}
