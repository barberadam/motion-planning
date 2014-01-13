#include "SPARSE-RRTNode.h"

//Weighting matrix for distance metric
double QDist_array[STATE_SPACE_DIM*STATE_SPACE_DIM] =			\
{1/pow((MAX_X - MIN_X),2), 0,   0, 0, 0, 0,				\
 0,      1/pow((MAXVEL_XY*2),2), 0, 0, 0, 0,				\
 0,       0,   1/pow((MAX_Y - MIN_Y),2), 0, 0, 0,			\
 0,       0,   0,      1/pow((MAXVEL_XY*2),2), 0, 0,			\
 0,       0,   0,      0,   1/pow((MAX_TH - MIN_TH),2), 0,		\
 0,       0,   0,      0,   0,      1/pow((MAXVEL_TH),2)};

Eigen::Matrix<double, STATE_SPACE_DIM, STATE_SPACE_DIM> QDist = \
    Eigen::Map<Eigen::MatrixXd>(QDist_array, STATE_SPACE_DIM, STATE_SPACE_DIM);

SPARSE_RRTNode::SPARSE_RRTNode() {
    parent = NULL;
    nodeTime = 0;
    for(int i = 0; i < STATE_SPACE_DIM; i++)
	nodeState(i,0) = 0;
    nodeControl << 0, 2.0*GRAV*MO, 0;
}

SPARSE_RRTNode::SPARSE_RRTNode(Eigen::Matrix<double, STATE_SPACE_DIM,1> setState) {
    parent = NULL;
    nodeTime = 0;
    nodeState = setState;
    nodeControl << 0, 2.0*GRAV*MO, 0;
}

SPARSE_RRTNode::SPARSE_RRTNode(Eigen::Matrix<double, STATE_SPACE_DIM,1> setState,	\
	      Eigen::Matrix<double, CONTROL_SPACE_DIM,1> setControl, \
	  SPARSE_RRTNode* setParent, double setTime){
    parent = setParent;
    nodeTime = setTime;
    nodeState = setState;
    nodeControl = setControl;
}


Eigen::Matrix<double, CONTROL_SPACE_DIM,1> const & SPARSE_RRTNode::getNodeControl() const {
    return nodeControl;
}

Eigen::Matrix<double, STATE_SPACE_DIM,1> const & SPARSE_RRTNode::getNodeState() const {
    return nodeState;
}

double SPARSE_RRTNode::getNodeTime() {
    return nodeTime;
}

SPARSE_RRTNode* SPARSE_RRTNode::getNodeParent(){
    return parent;
}

Eigen::Matrix<double, CONTROL_SPACE_DIM,1>				\
MapControlToWorld(Eigen::Matrix<double, STATE_SPACE_DIM,1> state,	\
		  Eigen::Matrix<double, CONTROL_SPACE_DIM,1> controlArray) {
    Eigen::Matrix<double, CONTROL_SPACE_DIM,1> worldControl;
    double conPoint = controlArray(0,0);
    double Fn = controlArray(1,0);
    double Ft = controlArray(2,0);
    double theta = state(4,0);
    worldControl(0,0) = Ft*cos(theta) - Fn*sin(theta);
    worldControl(1,0) = Fn*cos(theta) + Ft*sin(theta);
    worldControl(2,0) = WO*Ft + (conPoint)*Fn;
    return worldControl;
}

Eigen::Matrix<double, STATE_SPACE_DIM,1> OneStep(Eigen::Matrix<double, STATE_SPACE_DIM, 1> state, \
						 Eigen::Matrix<double, CONTROL_SPACE_DIM,1> worldControl) {
    Eigen::Matrix<double, STATE_SPACE_DIM,1> derivState;
    derivState << state(1,0),
	worldControl(0,0)/MO,
	state(3,0),
	worldControl(1,0)/MO - GRAV,
	state(5,0),
	worldControl(2,0)/JO;
    state = state + derivState*INT_TIME_STEP;
    return state;
}

Eigen::Matrix<double, STATE_SPACE_DIM,1> spawn(Eigen::Matrix<double, STATE_SPACE_DIM, 1> state, \
					       Eigen::Matrix<double, CONTROL_SPACE_DIM,1> control) {
    double time = 0;
    while(time < TIME_STEP) {
	state = OneStep(state, MapControlToWorld(state,control));
	time += INT_TIME_STEP;
    }
    return state;
}

Eigen::Matrix<double, STATE_SPACE_DIM,1> RandomSample(Eigen::Matrix<double,STATE_SPACE_DIM,1> goal) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0,1);
    if(dis(gen) < BIAS) {
	return goal;
    }
    //Not biased anymore
    Eigen::Matrix<double,STATE_SPACE_DIM,1> samp;
    samp << (MIN_X + (MAX_X - MIN_X)*(dis(gen))),	\
	MAXVEL_XY*2*(dis(gen) - 0.5),			\
	(MIN_Y + (MAX_Y - MIN_Y)*(dis(gen))),		\
	MAXVEL_XY*2*(dis(gen) - 0.5),			\
	(MIN_TH + (MAX_TH - MIN_TH)*(dis(gen))),	\
	MAXVEL_TH*2*(dis(gen) - 0.5);
    return samp;
}

double dist(Eigen::Matrix<double,STATE_SPACE_DIM,1> state1, Eigen::Matrix<double,STATE_SPACE_DIM,1> state2) {
    return (((state1 - state2).transpose())*QDist*(state1-state2))(0,0);
}


Eigen::Matrix<double, CONTROL_SPACE_DIM,1> RandomControl(Eigen::Matrix<double, CONTROL_SPACE_DIM,1> controlMean) {
    std::random_device rd;
    std::mt19937 gen(rd());
//    std::uniform_real_distribution<> dis(0,1);
    std::normal_distribution<> SDis(controlMean(0,0), S_STD_DEV);
    std::normal_distribution<> FnDis(controlMean(1,0), FN_STD_DEV);
    std::normal_distribution<> MuDis(controlMean(2,0), MU_STD_DEV);
    Eigen::Matrix<double,CONTROL_SPACE_DIM,1> samp;
    double FnSamp = FnDis(gen);
    if(FnSamp > MAX_FN)
	FnSamp = MAX_FN;
    else if(FnSamp < MIN_FN)
	FnSamp = MIN_FN;
    double muSamp = MuDis(gen);
    if(muSamp > MU)
	muSamp = MU;
    else if(muSamp < -MU)
	muSamp = -MU;
    double conPointSamp = SDis(gen);

    //Check values
    if(conPointSamp > LO)
	conPointSamp = LO;
    else if(conPointSamp < -LO)
	conPointSamp = -LO;
    samp << conPointSamp, FnSamp, muSamp*FnSamp;
    return samp;
}
