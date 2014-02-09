#ifndef NDEBUG
#define NDEBUG
#endif
#include <iostream>
#include <fstream>
#include <queue>
#include "AStarNode.h"
#include <ctime>

#define MAX_STEPS 10000
int main(int argc, char** argv)
{

    clock_t start;
    start = std::clock();
//    omp_set_num_threads(4);
    double distToGoal;
    double bestEucDist;
    //Set up the priority queue
    std::priority_queue<AStarNode*, std::vector<AStarNode*>, AStarNodePtrCompare> openQueue;
    std::vector<AStarNode*> expandedNodes;
    map_t grid;
    grid.clear();
    StateVector_t initState;
    initState << 0.0, 0.0, 0.0, 0.0, 0.0, 0.0;
    StateVector_t goalState;
    goalState << -0.2, -0.0525, 0.1, 0.0245, 0.4115, 0.8511;
    AStarNode* currentNode; //Temporary node to hold whats in The queue
    AStarNode* bestNode;
    currentNode = new AStarNode(initState, goalState);
    std::array<int, STATE_SPACE_DIM> tmpArray = snapToGrid(currentNode);
    std::array<int, STATE_SPACE_DIM>::iterator it;
    for(it = tmpArray.begin();
	it != tmpArray.end();
	it++) {
	std::cout<<*it<<std::endl;
    }
    AStarNode* tmpNode = currentNode;
    std::cout<<"currentNode: "<<tmpNode<<std::endl;
    std::cout<<tmpNode->getNodeState()<<std::endl;
    std::cout<<"Count: ";
    std::cout<<grid.count(tmpArray)<<std::endl;
    bool insCheck = grid.insert(make_pair(tmpArray,tmpNode)).second;
    std::cout<<"map ptr: "<<grid[tmpArray]<<std::endl;
    tmpArray[0] = 2;
    grid[tmpArray] = tmpNode;
    std::cout<<"Count: "<<grid.count(tmpArray)<<std::endl;
    std::cout<<"Size: "<<grid.size()<<std::endl;
    std::cout<<insCheck<<std::endl;

    int balsdf;
    std::cin>>balsdf;
    bestNode = currentNode;
    bestEucDist = euclideanDistance(initState,goalState);
    openQueue.push(currentNode);
    int badCount = 0;
    bool solFound = 0;
    int count = 0;
    while(!openQueue.empty() && !solFound && count < MAX_STEPS)
    {
	count++;
//	if(count % 50 == 0)
//	    std::cout << "Step: " << count << std::endl;
	//Pop the highest priority node (lowest cost)
	currentNode = openQueue.top();
	openQueue.pop();
	while(!currentNode->good){ // if it was replaced
	    if(openQueue.empty()) {
		std::cout<<"Open node list is empty, nothing to search!"<<std::endl;
		break;
	    }
	    badCount++;
	    currentNode = openQueue.top();
	    openQueue.pop();
	}
	if(euclideanDistance(currentNode->getNodeState(),goalState) < bestEucDist) {
	    bestEucDist = euclideanDistance(currentNode->getNodeState(),goalState);
	    bestNode = currentNode;
	}
//	closedNodes.push_back(currentNode);
	//Check if the node is close enough
	if(bestEucDist <= GOAL_EPSILON) {
	    std::cout << "Done!" << std::endl;
	    solFound = 1;
	}
	//
	expandedNodes = currentNode->expand(grid);
	//Add to queue
	while(!expandedNodes.empty())
	{
	    openQueue.push(expandedNodes.back());
	    expandedNodes.pop_back();
	}
    }
    if(euclideanDistance(currentNode->getNodeState(),goalState) < bestEucDist) {
	bestEucDist = euclideanDistance(currentNode->getNodeState(),goalState);
	bestNode = currentNode;
    }
    currentNode = bestNode; //Get best node so far
    std::cout<<"Cost to Go: "<< currentNode->getNodeCostToGo()<<std::endl;
    //Last node
    std::cout << "Last node cost to come: " << currentNode->getNodeCostToCome() << std::endl;
    //Save the solution
    std::vector<AStarNode*> solution;
    AStarNode* solNode = currentNode;
    AStarNode* prevNode;
    while(solNode->getNodeParent() != NULL) {
	solution.push_back(solNode);
	prevNode = solNode;
	solNode = solNode->getNodeParent();
	std::cout<<"Cost To Go: "<<solNode->getNodeCostToGo()<<std::endl;
	if(solNode == prevNode) {
	    std::cout << "weird pointer behavior" << std::endl;
	    break;
	}
    }
    //Add the root node
    solution.push_back(solNode);
    std::cout<<"Solution has " << solution.size() << " nodes."<<std::endl;
    std::cout << "Outputting files" << std::endl;
    std::ofstream out_file("Solution.txt", std::ios::trunc);
    out_file <<"X Xd Y Yd Th Thd s Fn Ft"<<std::endl;
    StateVector_t tempState;
    ControlVector_t tempControl;
    while(!solution.empty() && out_file.is_open()) {
	currentNode = solution.back();
	solution.pop_back();
	tempState = currentNode->getNodeState();
	tempControl = currentNode->getNodeControl();
	out_file << tempState(0,0) << " " << tempState(1,0) << " " << tempState(2,0) << " " \
		 << tempState(3,0) << " " << tempState(4,0) << " " << tempState(5,0) << " " \
	         << tempControl(0,0) << " " << tempControl(1,0) << " " << tempControl(2,0) << std::endl;
    }
    std::cout<<"Time: " << (std::clock() - start)/(double)CLOCKS_PER_SEC<<std::endl;
    std::cout<<"Grid size is: "<<grid.size()<<std::endl;
    std::cout<<"Bad Count: "<<badCount<<std::endl;
//    printBadCount();
    return 0;
}
