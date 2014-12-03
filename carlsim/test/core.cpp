#include "gtest/gtest.h"
#include "carlsim_tests.h"

#include <carlsim.h>
#include <vector>
#include <periodic_spikegen.h>

/// **************************************************************************************************************** ///
/// CORE FUNCTIONALITY
/// **************************************************************************************************************** ///

TEST(CORE, getGroupGrid3D) {
	::testing::FLAGS_gtest_death_test_style = "threadsafe";

	CARLsim* sim = new CARLsim("CORE.getGroupGrid3D",CPU_MODE,SILENT,0,42);
	Grid3D grid(2,3,4);
	int g2=sim->createGroup("excit2", grid, EXCITATORY_NEURON);
	sim->setNeuronParameters(g2, 0.02f, 0.2f, -65.0f, 8.0f);
	int g1=sim->createSpikeGeneratorGroup("excit", grid, EXCITATORY_NEURON);
	sim->connect(g1,g2,"full",RangeWeight(0.1), 1.0, RangeDelay(1));
	sim->setupNetwork(); // need SETUP state for this function to work

	for (int g=g1; g<g2; g++) {
		Grid3D getGrid = sim->getGroupGrid3D(g);
		EXPECT_EQ(getGrid.x, grid.x);
		EXPECT_EQ(getGrid.y, grid.y);
		EXPECT_EQ(getGrid.z, grid.z);
		EXPECT_EQ(getGrid.N, grid.N);
	}

	delete sim;
}

TEST(CORE, getGroupIdFromString) {
	::testing::FLAGS_gtest_death_test_style = "threadsafe";

	CARLsim* sim = new CARLsim("CORE.getGroupIdFromString",CPU_MODE,SILENT,0,42);
	int g2=sim->createGroup("bananahama", Grid3D(1,2,3), INHIBITORY_NEURON);
	sim->setNeuronParameters(g2, 0.02f, 0.2f, -65.0f, 8.0f);
	int g1=sim->createSpikeGeneratorGroup("excit", Grid3D(2,3,4), EXCITATORY_NEURON);
	sim->connect(g1,g2,"full",RangeWeight(0.1), 1.0, RangeDelay(1));
	sim->setupNetwork(); // need SETUP state for this function to work

	EXPECT_EQ(sim->getGroupId("excit"), g1);
	EXPECT_EQ(sim->getGroupId("bananahama"), g2);
	EXPECT_EQ(sim->getGroupId("invalid group name"), -1); // group not found

	delete sim;
}


// This test creates a group on a grid and makes sure that the returned 3D location of each neuron is correct
TEST(CORE, getNeuronLocation3D) {
	::testing::FLAGS_gtest_death_test_style = "threadsafe";

	CARLsim* sim = new CARLsim("CORE.getNeuronLocation3D",CPU_MODE,SILENT,0,42);
	Grid3D grid(2,3,4);
	int g2=sim->createGroup("excit2", grid, EXCITATORY_NEURON);
	sim->setNeuronParameters(g2, 0.02f, 0.2f, -65.0f, 8.0f);
	int g1=sim->createSpikeGeneratorGroup("excit", grid, EXCITATORY_NEURON);
	sim->connect(g1,g2,"full",RangeWeight(0.1), 1.0, RangeDelay(1));
	sim->setupNetwork(); // need SETUP state for getNeuronLocation3D to work

	// make sure the 3D location that is returned is correct
	for (int grp=0; grp<=1; grp++) {
		// do for both spike gen and RS group

		int x=0,y=0,z=0;
		for (int neurId=grp*grid.N; neurId<(grp+1)*grid.N; neurId++) {
			Point3D loc = sim->getNeuronLocation3D(neurId);
			EXPECT_EQ(loc.x, x);
			EXPECT_EQ(loc.y, y);
			EXPECT_EQ(loc.z, z);

			x++;
			if (x==grid.x) {
				x=0;
				y++;
			}
			if (y==grid.y) {
				x=0;
				y=0;
				z++;
			}
		}
	}

	delete sim;
}

/*
// \FIXME deactivate for now because we don't want to instantiate CpuSNN

// tests whether a point lies on a grid
TEST(CORE, isPoint3DonGrid) {
	CpuSNN snn("CORE.isPoint3DonGrid", CPU_MODE, SILENT, 0, 42);
	EXPECT_FALSE(snn.isPoint3DonGrid(Point3D(-1,-1,-1), Grid3D(10,5,2)));
	EXPECT_FALSE(snn.isPoint3DonGrid(Point3D(0.5,0.5,0.5), Grid3D(10,5,2)));
	EXPECT_FALSE(snn.isPoint3DonGrid(Point3D(10,5,2), Grid3D(10,5,2)));

	EXPECT_TRUE(snn.isPoint3DonGrid(Point3D(0,0,0), Grid3D(10,5,2)));
	EXPECT_TRUE(snn.isPoint3DonGrid(Point3D(0.0,0.0,0.0), Grid3D(10,5,2)));
	EXPECT_TRUE(snn.isPoint3DonGrid(Point3D(1,1,1), Grid3D(10,5,2)));
	EXPECT_TRUE(snn.isPoint3DonGrid(Point3D(9,4,1), Grid3D(10,5,2)));
	EXPECT_TRUE(snn.isPoint3DonGrid(Point3D(9.0,4.0,1.0), Grid3D(10,5,2)));
}
*/

// \TODO: using external current, make sure the Izhikevich model is correctly implemented
// Run izhikevich.org MATLAB script to find number of spikes as a function of neuron type,
// input current, and time period. Build test case to reproduce the exact numbers.

TEST(CORE, setExternalCurrent) {
	::testing::FLAGS_gtest_death_test_style = "threadsafe";

	CARLsim * sim;
	int nNeur = 10;

	for (int hasCOBA=0; hasCOBA<=1; hasCOBA++) {
		for (int isGPUmode=0; isGPUmode<=1; isGPUmode++) {
			sim = new CARLsim("CORE.setExternalCurrent", isGPUmode?GPU_MODE:CPU_MODE, SILENT, 0, 42);
			int g1=sim->createGroup("excit1", nNeur, EXCITATORY_NEURON);
			sim->setNeuronParameters(g1, 0.02f, 0.2f, -65.0f, 8.0f);
			int g0=sim->createSpikeGeneratorGroup("input0", nNeur, EXCITATORY_NEURON);
			sim->connect(g0,g1,"full",RangeWeight(0.1),1.0f,RangeDelay(1));
			sim->setConductances(hasCOBA>0);
			sim->setupNetwork();
//			fprintf(stderr, "setExternalCurrent %s %s\n",hasCOBA?"COBA":"CUBA",isGPUmode?"GPU":"CPU");

			SpikeMonitor* SM = sim->setSpikeMonitor(g1,"NULL");

			// run for a bunch, observe zero spikes since ext current should be zero by default
			SM->startRecording();
			sim->runNetwork(1,0);
			SM->stopRecording();
			EXPECT_EQ(SM->getPopNumSpikes(), 0);

			// set current, observe spikes
			std::vector<float> current(nNeur,7.0f);
			sim->setExternalCurrent(g1, current);
			SM->startRecording();
			sim->runNetwork(0,500);
			SM->stopRecording();
			EXPECT_GT(SM->getPopNumSpikes(), 0); // should be >0 in all cases
			for (int i=0; i<nNeur; i++) {
				EXPECT_EQ(SM->getNeuronNumSpikes(i), 8); // but actually should be ==8
			}

			// (intentionally) forget to reset current, observe spikes
			SM->startRecording();
			sim->runNetwork(0,500);
			SM->stopRecording();
			EXPECT_GT(SM->getPopNumSpikes(), 0); // should be >0 in all cases
			for (int i=0; i<nNeur; i++) {
				EXPECT_EQ(SM->getNeuronNumSpikes(i), 8); // but actually should be ==8
			}

			// reset current to zero
			sim->setExternalCurrent(g1, 0.0f);
			SM->startRecording();
			sim->runNetwork(0,500);
			SM->stopRecording();
			EXPECT_EQ(SM->getPopNumSpikes(), 0);

			// use convenience function to achieve same result as above
			sim->setExternalCurrent(g1, 7.0f);
			SM->startRecording();
			sim->runNetwork(0,500);
			SM->stopRecording();
			EXPECT_GT(SM->getPopNumSpikes(), 0); // should be >0 in all cases
			for (int i=0; i<nNeur; i++) {
				EXPECT_EQ(SM->getNeuronNumSpikes(i), 8); // but actually should be ==8
			}

			delete sim;
		}
	}
}

TEST(CORE, setWeight) {
	::testing::FLAGS_gtest_death_test_style = "threadsafe";

	CARLsim* sim;
	int nNeur = 10;
	int *nSpkHighWt = new int[nNeur];
	memset(nSpkHighWt, 0, nNeur*sizeof(int));

	for (int isGPUmode=0; isGPUmode<=1; isGPUmode++) {
		sim = new CARLsim("Interface.biasWeightsDeath",isGPUmode?GPU_MODE:CPU_MODE,SILENT,0,42);
		int g1=sim->createGroup("excit", nNeur, EXCITATORY_NEURON);
		sim->setNeuronParameters(g1, 0.02f, 0.2f,-65.0f,8.0f);
		int c1=sim->connect(g1, g1, "one-to-one", RangeWeight(0.5f), 1.0f, RangeDelay(1));
		sim->setConductances(true);
		sim->setupNetwork();

		// ---- run network for a while with input current and high weight
		//      observe much spiking

		SpikeMonitor* SM = sim->setSpikeMonitor(g1,"NULL");
		sim->setExternalCurrent(g1, 7.0f);

		SM->startRecording();
		sim->runNetwork(2,0);
		SM->stopRecording();

		for (int neurId=0; neurId<nNeur; neurId++) {
			nSpkHighWt[neurId] = SM->getNeuronNumSpikes(neurId);
			sim->setWeight(c1, neurId, neurId, 0.0f, false);
		}


		// ---- run network for a while with zero weight (but still current injection)
		//      observe less spiking

		SM->startRecording();
		sim->runNetwork(2,0);
		SM->stopRecording();

		for (int neurId=0; neurId<nNeur; neurId++) {
			EXPECT_LT(SM->getNeuronNumSpikes(neurId), nSpkHighWt[neurId]);
		}

		delete sim;
	}

	delete[] nSpkHighWt;
}



// make sure bookkeeping for number of groups is correct during CONFIG
TEST(CORE, numGroups) {
	::testing::FLAGS_gtest_death_test_style = "threadsafe";

	CARLsim sim("CORE.numGroups", CPU_MODE, SILENT, 0, 42);
	EXPECT_EQ(sim.getNumGroups(), 0);

	int nLoops = 4;
	int nNeur = 10;
	for (int i=0; i<nLoops; i++) {
		sim.createGroup("regexc", nNeur, EXCITATORY_NEURON);
		EXPECT_EQ(sim.getNumGroups(), i*4+1);
		sim.createGroup("reginh", nNeur, INHIBITORY_NEURON);
		EXPECT_EQ(sim.getNumGroups(), i*4+2);
		sim.createSpikeGeneratorGroup("genexc", nNeur, EXCITATORY_NEURON);
		EXPECT_EQ(sim.getNumGroups(), i*4+3);
		sim.createSpikeGeneratorGroup("geninh", nNeur, INHIBITORY_NEURON);
		EXPECT_EQ(sim.getNumGroups(), i*4+4);
	}
}

// make sure bookkeeping for number of neurons is correct during CONFIG
TEST(CORE, numNeurons) {
	::testing::FLAGS_gtest_death_test_style = "threadsafe";

	CARLsim sim("CORE.numNeurons", CPU_MODE, SILENT, 0, 42);
	EXPECT_EQ(sim.getNumNeurons(), 0);
	EXPECT_EQ(sim.getNumNeuronsRegExc(), 0);
	EXPECT_EQ(sim.getNumNeuronsRegInh(), 0);
	EXPECT_EQ(sim.getNumNeuronsGenExc(), 0);
	EXPECT_EQ(sim.getNumNeuronsGenInh(), 0);

	int nLoops = 4;
	int nNeur = 10;

	for (int i=0; i<nLoops; i++) {
		sim.createGroup("regexc", nNeur, EXCITATORY_NEURON);
		EXPECT_EQ(sim.getNumNeurons(), i*4*nNeur + nNeur);
		EXPECT_EQ(sim.getNumNeuronsRegExc(), i*nNeur + nNeur);
		EXPECT_EQ(sim.getNumNeuronsRegInh(), i*nNeur);
		EXPECT_EQ(sim.getNumNeuronsGenExc(), i*nNeur);
		EXPECT_EQ(sim.getNumNeuronsGenInh(), i*nNeur);
		EXPECT_EQ(sim.getNumNeurons(), sim.getNumNeuronsRegExc() + sim.getNumNeuronsRegInh()
			+ sim.getNumNeuronsGenExc() + sim.getNumNeuronsGenInh());
		EXPECT_EQ(sim.getNumNeuronsReg(), sim.getNumNeuronsRegExc() + sim.getNumNeuronsRegInh());
		EXPECT_EQ(sim.getNumNeuronsGen(), sim.getNumNeuronsGenExc() + sim.getNumNeuronsGenInh());

		sim.createGroup("reginh", nNeur, INHIBITORY_NEURON);
		EXPECT_EQ(sim.getNumNeurons(), i*4*nNeur + 2*nNeur);
		EXPECT_EQ(sim.getNumNeuronsRegExc(), i*nNeur + nNeur);
		EXPECT_EQ(sim.getNumNeuronsRegInh(), i*nNeur + nNeur);
		EXPECT_EQ(sim.getNumNeuronsGenExc(), i*nNeur);
		EXPECT_EQ(sim.getNumNeuronsGenInh(), i*nNeur);
		EXPECT_EQ(sim.getNumNeurons(), sim.getNumNeuronsRegExc() + sim.getNumNeuronsRegInh()
			+ sim.getNumNeuronsGenExc() + sim.getNumNeuronsGenInh());
		EXPECT_EQ(sim.getNumNeuronsReg(), sim.getNumNeuronsRegExc() + sim.getNumNeuronsRegInh());
		EXPECT_EQ(sim.getNumNeuronsGen(), sim.getNumNeuronsGenExc() + sim.getNumNeuronsGenInh());

		sim.createSpikeGeneratorGroup("genexc", nNeur, EXCITATORY_NEURON);
		EXPECT_EQ(sim.getNumNeurons(), i*4*nNeur + 3*nNeur);
		EXPECT_EQ(sim.getNumNeuronsRegExc(), i*nNeur + nNeur);
		EXPECT_EQ(sim.getNumNeuronsRegInh(), i*nNeur + nNeur);
		EXPECT_EQ(sim.getNumNeuronsGenExc(), i*nNeur + nNeur);
		EXPECT_EQ(sim.getNumNeuronsGenInh(), i*nNeur);
		EXPECT_EQ(sim.getNumNeurons(), sim.getNumNeuronsRegExc() + sim.getNumNeuronsRegInh()
			+ sim.getNumNeuronsGenExc() + sim.getNumNeuronsGenInh());
		EXPECT_EQ(sim.getNumNeuronsReg(), sim.getNumNeuronsRegExc() + sim.getNumNeuronsRegInh());
		EXPECT_EQ(sim.getNumNeuronsGen(), sim.getNumNeuronsGenExc() + sim.getNumNeuronsGenInh());

		sim.createSpikeGeneratorGroup("geninh", nNeur, INHIBITORY_NEURON);
		EXPECT_EQ(sim.getNumNeurons(), i*4*nNeur + 4*nNeur);
		EXPECT_EQ(sim.getNumNeuronsRegExc(), i*nNeur + nNeur);
		EXPECT_EQ(sim.getNumNeuronsRegInh(), i*nNeur + nNeur);
		EXPECT_EQ(sim.getNumNeuronsGenExc(), i*nNeur + nNeur);
		EXPECT_EQ(sim.getNumNeuronsGenInh(), i*nNeur + nNeur);
		EXPECT_EQ(sim.getNumNeurons(), sim.getNumNeuronsRegExc() + sim.getNumNeuronsRegInh()
			+ sim.getNumNeuronsGenExc() + sim.getNumNeuronsGenInh());
		EXPECT_EQ(sim.getNumNeuronsReg(), sim.getNumNeuronsRegExc() + sim.getNumNeuronsRegInh());
		EXPECT_EQ(sim.getNumNeuronsGen(), sim.getNumNeuronsGenExc() + sim.getNumNeuronsGenInh());
	}
}
