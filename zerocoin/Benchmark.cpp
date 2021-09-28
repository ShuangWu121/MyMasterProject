/**
 * @file       Benchmark.cpp
 *
 * @brief      Benchmarking tests for Zerocoin.
 *
 * @author     Ian Miers, Christina Garman and Matthew Green
 * @date       June 2013
 *
 * @copyright  Copyright 2013 Ian Miers, Christina Garman and Matthew Green
 * @license    This project is released under the MIT license.
 **/

using namespace std;

#include <string>
#include <iostream>
#include <fstream>
#include <curses.h>
#include <exception>
#include <cstdlib>
#include <sys/time.h>

#include "Zerocoin.h"

using namespace libzerocoin;

#define COLOR_STR_GREEN   "\033[32m"
#define COLOR_STR_NORMAL  "\033[0m"
#define COLOR_STR_RED     "\033[31m"

#define TESTS_COINS_TO_ACCUMULATE   5

// Global test counters
uint32_t    gNumTests        = 0;
uint32_t    gSuccessfulTests = 0;

// Global coin array
PrivateCoin    *gCoins[TESTS_COINS_TO_ACCUMULATE];
char iwantread[100000];
char bufferOne[100000];

// Global params
Params *g_Params;
AccumulatorWitness *witness[TESTS_COINS_TO_ACCUMULATE];
AccumulatorWitness *trywitness[TESTS_COINS_TO_ACCUMULATE];

//////////
// Utility routines
//////////
//ofstream SavePublicCredential("SavePublicCredential.txt");



class Timer
{
	timeval timer[2];

public:

	timeval start()
	{
		gettimeofday(&this->timer[0], NULL);
		return this->timer[0];
	}

	timeval stop()
	{
		gettimeofday(&this->timer[1], NULL);
		return this->timer[1];
	}

	int duration() const
	{
		int secs(this->timer[1].tv_sec - this->timer[0].tv_sec);
		int usecs(this->timer[1].tv_usec - this->timer[0].tv_usec);

		if(usecs < 0)
		{
			--secs;
			usecs += 1000000;
		}

		return static_cast<int>(secs * 1000 + usecs / 1000.0 + 0.5);
	}
};

// Global timer
Timer timer;

void
LogTestResult(string testName, bool (*testPtr)())
{
	string colorGreen(COLOR_STR_GREEN);
	string colorNormal(COLOR_STR_NORMAL);
	string colorRed(COLOR_STR_RED);

	cout << "Testing if " << testName << "..." << endl;

	bool testResult = testPtr();

	if (testResult == true) {
		cout << "\t" << colorGreen << "[PASS]"  << colorNormal << endl;
		gSuccessfulTests++;
	} else {
		cout << colorRed << "\t[FAIL]" << colorNormal << endl;
	}

	gNumTests++;
}

Bignum
GetTestModulus() //generate big number N N=p*q RSA module
{
	static Bignum testModulus(0);

	// TODO: should use a hard-coded RSA modulus for testing
	if (!testModulus) {
		Bignum p, q;
		p = Bignum::generatePrime(1024, false);
		q = Bignum::generatePrime(1024, false);
		testModulus = p * q;
	}

	return testModulus;
}

//////////
// Test routines
//////////


bool
Test_GenRSAModulus()//test the big N is true
{
	Bignum result = GetTestModulus();

	if (!result) {
		return false;
	}
	else {
		return true;
	}
}

bool
Test_CalcParamSizes()
{
	bool result = true;
#if 1

	uint32_t pLen, qLen;

	try {
		calculateGroupParamLengths(4000, 80, &pLen, &qLen);
		if (pLen < 1024 || qLen < 256) {
			result = false;
		}
		/**
		calculateGroupParamLengths(4000, 96, &pLen, &qLen);
		if (pLen < 2048 || qLen < 256) {
			result = false;
		}
		calculateGroupParamLengths(4000, 112, &pLen, &qLen);
		if (pLen < 3072 || qLen < 320) {
			result = false;
		}
		calculateGroupParamLengths(4000, 120, &pLen, &qLen);
		if (pLen < 3072 || qLen < 320) {
			result = false;
		}
		calculateGroupParamLengths(4000, 128, &pLen, &qLen);
		if (pLen < 3072 || qLen < 320) {
			result = false;
		}
		**/
	} catch (exception &e) {
		result = false;
	}
#endif

	return result;
}

bool
Test_GenerateGroupParams()
{
	uint32_t pLen = 1024, qLen = 256, count;
	IntegerGroupParams group;

	for (count = 0; count < 1; count++) {

		try {
			group = deriveIntegerGroupParams(calculateSeed(GetTestModulus(), "test", ZEROCOIN_DEFAULT_SECURITYLEVEL, "TEST GROUP"), pLen, qLen);
		} catch (std::runtime_error e) {
			cout << "Caught exception " << e.what() << endl;
			return false;
		}

		// Now perform some simple tests on the resulting parameters
		if (group.groupOrder.bitSize() < qLen || group.modulus.bitSize() < pLen) {
			return false;
		}

		Bignum c = group.g.pow_mod(group.groupOrder, group.modulus);
		//cout << "g^q mod p = " << c << endl;
		if (!(c.isOne())) return false;

		// Try at multiple parameter sizes
		pLen = pLen * 1.5;
		qLen = qLen * 1.5;
	}

	return true;
}

bool
Test_ParamGen()
{
	bool result = true;

	try {
		timer.start();
		// Instantiating testParams runs the parameter generation code
		Params testParams(GetTestModulus(),ZEROCOIN_DEFAULT_SECURITYLEVEL);
		timer.stop();

		cout << "\tPARAMGEN ELAPSED TIME: " << timer.duration() << " ms\t" << timer.duration()*0.001 << " s" << endl;
	} catch (runtime_error e) {
		cout << e.what() << endl;
		result = false;
	}

	return result;
}

bool
Test_Accumulator()
{
	// This test assumes a list of coins were generated during
	// the Test_MintCoin() test.
	if (gCoins[0] == NULL) {
		return false;
	}
	try {
		// Accumulate the coin list from first to last into one accumulator
		Accumulator accOne(&g_Params->accumulatorParams);
		Accumulator accTwo(&g_Params->accumulatorParams);
		Accumulator accThree(&g_Params->accumulatorParams);
		Accumulator accFour(&g_Params->accumulatorParams);
		AccumulatorWitness wThree(g_Params, accThree, gCoins[0]->getPublicCoin());

		for (uint32_t i = 0; i < TESTS_COINS_TO_ACCUMULATE; i++) {
			accOne += gCoins[i]->getPublicCoin();
			accTwo += gCoins[TESTS_COINS_TO_ACCUMULATE - (i+1)]->getPublicCoin();
			accThree += gCoins[i]->getPublicCoin();
			wThree += gCoins[i]->getPublicCoin();
			if(i != 0) {
				accFour += gCoins[i]->getPublicCoin();

			}
		}

		// Compare the accumulated results
		if (accOne.getValue() != accTwo.getValue() || accOne.getValue() != accThree.getValue()) {
			cout << "Accumulators don't match" << endl;
			return false;
		}

		if(accFour.getValue() != wThree.getValue()) {
			cout << "Witness math not working," << endl;
			return false;
		}

		// Verify that the witness is correct
		if (!wThree.VerifyWitness(accThree, gCoins[0]->getPublicCoin()) ) {
			cout << "Witness not valid" << endl;
			return false;
		}

	} catch (runtime_error e) {
		return false;
	}

	return true;
}

bool
Test_MintCoin()
{
	try {
		// Generate a list of coins
		timer.start();
		for (uint32_t i = 0; i < TESTS_COINS_TO_ACCUMULATE; i++) {
			gCoins[i] = new PrivateCoin(g_Params);

		}
		timer.stop();
	} catch (exception &e) {
		return false;
	}

	cout << "\tMINT ELAPSED TIME:\n\t\tTotal: " << timer.duration() << " ms\t" << timer.duration()*0.001 << " s\n\t\tPer Coin: " << timer.duration()/TESTS_COINS_TO_ACCUMULATE << " ms\t" << (timer.duration()/TESTS_COINS_TO_ACCUMULATE)*0.001 << " s" << endl;

	return true;
}

bool
Test_MintAndSpend()
{
	try {
		// This test assumes a list of coins were generated in Test_MintCoin()
		if (gCoins[0] == NULL)
		{
			// No coins: mint some.
			Test_MintCoin();
			if (gCoins[0] == NULL) {
				return false;
			}
		}

		// Accumulate the list of generated coins into a fresh accumulator.
		// The first one gets marked as accumulated for a witness, the
		// others just get accumulated normally.
		Accumulator acc(&g_Params->accumulatorParams);
		AccumulatorWitness wAcc(g_Params, acc, gCoins[0]->getPublicCoin());


		ofstream SavePublicCredential("SavePublicCredential.txt");

        //cout <<"the accululater size is"<<sizeof(acc)<<endl<<endl;

		//cout << "\tACCUMULATOR ELAPSED TIME:\n\t\tTotal: " << timer.duration() << " ms\t" << timer.duration()*0.001 << " s\n\t\tPer Element: " << timer.duration()/TESTS_COINS_TO_ACCUMULATE << " ms\t" << (timer.duration()/TESTS_COINS_TO_ACCUMULATE)*0.001 << " s" << endl;

		timer.start();
		for (uint32_t i = 0; i < TESTS_COINS_TO_ACCUMULATE; i++) {
			wAcc +=gCoins[i]->getPublicCoin();
		}

		for (uint32_t j=0; j < TESTS_COINS_TO_ACCUMULATE; j++){
            witness[j]= new AccumulatorWitness(g_Params, acc, gCoins[j]->getPublicCoin());
            for (uint32_t i = 0; i < TESTS_COINS_TO_ACCUMULATE; i++) {
                *witness[j] +=gCoins[i]->getPublicCoin();
            }


            Accumulator accu= (*witness[j]).getAccumulator();

            CDataStream witnessStream(SER_NETWORK, PROTOCOL_VERSION);
            witnessStream << accu;
            witnessStream.read(iwantread,witnessStream.GetSerializeSize(accu));

            Accumulator accone(&g_Params->accumulatorParams);
            //witnessStream>>accu;

            CDataStream getwitnessStream(SER_NETWORK, PROTOCOL_VERSION);
            getwitnessStream.write(iwantread,sizeof(iwantread));
            getwitnessStream>>accone;

            //Accumulator acccc(g_Params,witnessStream);
            AccumulatorWitness iamwitness(g_Params, accone, gCoins[j]->getPublicCoin());

			SavePublicCredential <<"\n\nVoter's credential:\n\t"<< gCoins[j]->getPublicCoin().getValue().GetHex()<<"\nThe witness is:\n\t"<<witness[j]->getValue().GetHex()<<endl<<endl;
            SavePublicCredential <<"\n\nVoter's credential:\n\t"<< gCoins[j]->getPublicCoin().getValue().GetHex()<<"\nThe witness is:\n\t"<<iamwitness.getValue().GetHex()<<endl<<endl;


        }

        for (uint32_t i = 0; i < TESTS_COINS_TO_ACCUMULATE; i++) {
			acc += gCoins[i]->getPublicCoin();
		}

		SavePublicCredential <<"the accumulator of this group of voters is  " << acc.getValue()<<endl<<endl;
        timer.stop();
        SavePublicCredential << "\tGenerate witnesses for "<< TESTS_COINS_TO_ACCUMULATE<<" voters costs "<<timer.duration()*0.001<< " s\t"<< " s\n\t\tPer voter: " << (timer.duration()/TESTS_COINS_TO_ACCUMULATE)*0.001 << " s\t"<< endl;

		SavePublicCredential.close();
		ofstream SavePrivatekey("SavePrivatekey.txt");
        SavePrivatekey<<"the publiccoin 3 serial number is   "<<gCoins[3]->getSerialNumber()<<endl<<endl;
        SavePrivatekey<<"the publiccoin 3 randomness number is   "<<gCoins[3]->getRandomness()<<endl<<endl;
        SavePrivatekey.close();



       // CDataStream witnessStream(SER_NETWORK, PROTOCOL_VERSION);
        //witnessStream << acc;
        //witnessStream.read(iwantread,witnessStream.GetSerializeSize(acc));
        //Accumulator accone(&g_Params->accumulatorParams);
        //Accumulator acccc(g_Params,witnessStream);

       // CDataStream getwitnessStream(SER_NETWORK, PROTOCOL_VERSION);
        //getwitnessStream.write(iwantread,sizeof(iwantread));



       // cout<<"see if it can show :"<<endl<<endl;
        //char buffer[100000];
        //int i;
        //for(i=0;i<sizeof(iwantread)-1;i++){
          //  buffer[i]=iwantread[i];
       // }

        //getwitnessStream.write(buffer,sizeof(buffer));
        //getwitnessStream >> accone;

        cout<<"the accumulator is  " << acc.getValue()<<endl<<endl;
        //cout <<"the witness size is"<<sizeof(wAcc)<<endl<<endl;
       // cout<<"the accumulator  1 is  " << accone.getValue()<<endl<<endl;
      // cout<<"the accumulator  1 is  "  <<accone.getValue()<<endl<<endl;
      // cout<<"the SIZE  1 is  "  <<sizeof(witnessStream)<<endl<<endl;


		//cout << "\tWITNESS ELAPSED TIME: \n\t\tTotal: " << timer.duration() << " ms\t" << timer.duration()*0.001 << " s\n\t\tPer Element: " << timer.duration()/TESTS_COINS_TO_ACCUMULATE << " ms\t" << (timer.duration()/TESTS_COINS_TO_ACCUMULATE)*0.001 << " s" << endl;
        if(wAcc.VerifyWitness(acc,gCoins[0]->getPublicCoin())){
			cout<<"it is doing pow"<<endl<<endl;
        }else{
			cout<<"it is not doing pow"<<endl<<endl;
        }
        cout<<"witness 0  is  " << wAcc.getValue()<<endl<<endl;
		// Now spend the coin
		SpendMetaData m(1,1);
		//cout <<"the signature size is"<<sizeof(m)<<endl<<endl;

		timer.start();
		CoinSpend spend(g_Params, *(gCoins[0]), acc, wAcc, m);
		timer.stop();

		//cout << "\tSPEND ELAPSED TIME: " << timer.duration() << " ms\t" << timer.duration()*0.001 << " s" << endl;

		// Serialize the proof and deserialize into newSpend
		CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);

		timer.start();
		ss << spend;
		timer.stop();

		CoinSpend newSpend(g_Params, ss);

		//cout << "\tSERIALIZE ELAPSED TIME: " << timer.duration() << " ms\t" << timer.duration()*0.001 << " s" << endl;

		// Finally, see if we can verify the deserialized proof (return our result)
		timer.start();
		bool ret = newSpend.Verify(acc, m);
		timer.stop();

		//cout << "\tSPEND VERIFY ELAPSED TIME: " << timer.duration() << " ms\t" << timer.duration()*0.001 << " s" << endl;

		return ret;
	} catch (runtime_error &e) {
		cout << e.what() << endl;
		return false;
	}

	return false;
}

void
Test_RunAllTests()
{
	// Make a new set of parameters from a random RSA modulus
	g_Params = new Params(GetTestModulus());

	gNumTests = gSuccessfulTests = 0;
	for (uint32_t i = 0; i < TESTS_COINS_TO_ACCUMULATE; i++) {
		gCoins[i] = NULL;
	}

	// Run through all of the Zerocoin tests
	LogTestResult("an RSA modulus can be generated", Test_GenRSAModulus);
	LogTestResult("parameter sizes are correct", Test_CalcParamSizes);
	LogTestResult("group/field parameters can be generated", Test_GenerateGroupParams);
	LogTestResult("parameter generation is correct", Test_ParamGen);
	LogTestResult("coins can be minted", Test_MintCoin);
	LogTestResult("the accumulator works", Test_Accumulator);
	LogTestResult("a minted coin can be spent", Test_MintAndSpend);

	// Summarize test results
	if (gSuccessfulTests < gNumTests) {
		cout << endl << "ERROR: SOME TESTS FAILED" << endl;
	}



	// Clear any generated coins
	for (uint32_t i = 0; i < TESTS_COINS_TO_ACCUMULATE; i++) {
		delete gCoins[i];
	}

	cout << gSuccessfulTests << " out of " << gNumTests << " tests passed." << endl << endl;
	delete g_Params;
}






int main(int argc, char **argv)
{

	cout << "libzerocoin v" << ZEROCOIN_VERSION_STRING << " benchmark utility." << endl << endl;


	Test_RunAllTests();

}

