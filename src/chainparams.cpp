// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Copyright (c) 2014-2017 The Dash Core developers
// Copyright (c) 2018-2019 The GeekCash developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include "arith_uint256.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
	CMutableTransaction txNew;
	txNew.nVersion = 1;
	txNew.vin.resize(1);
	txNew.vout.resize(1);
	txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
	txNew.vout[0].nValue = genesisReward;
	txNew.vout[0].scriptPubKey = genesisOutputScript;

	CBlock genesis;
	genesis.nTime    = nTime;
	genesis.nBits    = nBits;
	genesis.nNonce   = nNonce;
	genesis.nVersion = nVersion;
	genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
	genesis.hashPrevBlock.SetNull();
	genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
	return genesis;
}

static CBlock CreateDevNetGenesisBlock(const uint256 &prevBlockHash, const std::string& devNetName, uint32_t nTime, uint32_t nNonce, uint32_t nBits, const CAmount& genesisReward)
{
	assert(!devNetName.empty());

	CMutableTransaction txNew;
	txNew.nVersion = 1;
	txNew.vin.resize(1);
	txNew.vout.resize(1);
	// put height (BIP34) and devnet name into coinbase
	txNew.vin[0].scriptSig = CScript() << 1 << std::vector<unsigned char>(devNetName.begin(), devNetName.end());
	txNew.vout[0].nValue = genesisReward;
	txNew.vout[0].scriptPubKey = CScript() << OP_RETURN;

	CBlock genesis;
	genesis.nTime    = nTime;
	genesis.nBits    = nBits;
	genesis.nNonce   = nNonce;
	genesis.nVersion = 4;
	genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
	genesis.hashPrevBlock = prevBlockHash;
	genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
	return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=00000ffd590b14, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=e0028e, nTime=1390095618, nBits=1e0ffff0, nNonce=28917698, vtx=1)
 *   CTransaction(hash=e0028e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d01044c5957697265642030392f4a616e2f3230313420546865204772616e64204578706572696d656e7420476f6573204c6976653a204f76657273746f636b2e636f6d204973204e6f7720416363657074696e6720426974636f696e73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0xA9037BAC7050C479B121CF)
 *   vMerkleTree: e0028e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
	const char* pszTimestamp = "Dive in and GEEK!";
	const CScript genesisOutputScript = CScript() << ParseHex("04381ab20c1c3967d78a0e57d41499e7b1aaa31149f2767f33fb56dd76abaa1e49838f6f66e3ea41fd863169f9791a4ce6b82811739482f547c98e379aa0c1f3cb") << OP_CHECKSIG;
	return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

static CBlock FindDevNetGenesisBlock(const Consensus::Params& params, const CBlock &prevBlock, const CAmount& reward)
{
	std::string devNetName = GetDevNetName();
	assert(!devNetName.empty());

	CBlock block = CreateDevNetGenesisBlock(prevBlock.GetHash(), devNetName.c_str(), prevBlock.nTime + 1, 0, prevBlock.nBits, reward);

	arith_uint256 bnTarget;
	bnTarget.SetCompact(block.nBits);

	for (uint32_t nNonce = 0; nNonce < UINT32_MAX; nNonce++) {
		block.nNonce = nNonce;

		uint256 hash = block.GetHash();
		if (UintToArith256(hash) <= bnTarget)
			return block;
	}

	// This is very unlikely to happen as we start the devnet with a very low difficulty. In many cases even the first
	// iteration of the above loop will give a result already
	error("FindDevNetGenesisBlock: could not find devnet genesis block for %s", devNetName);
	assert(false);
}

// this one is for testing only
static Consensus::LLMQParams llmq10_60 = {
		.type = Consensus::LLMQ_10_60,
		.name = "llmq_10",
		.size = 10,
		.minSize = 6,
		.threshold = 6,

		.dkgInterval = 24, // one DKG per hour
		.dkgPhaseBlocks = 2,
		.dkgMiningWindowStart = 10, // dkgPhaseBlocks * 5 = after finalization
		.dkgMiningWindowEnd = 18,
};

static Consensus::LLMQParams llmq50_60 = {
		.type = Consensus::LLMQ_50_60,
		.name = "llmq_50_60",
		.size = 50,
		.minSize = 40,
		.threshold = 30,

		.dkgInterval = 24, // one DKG per hour
		.dkgPhaseBlocks = 2,
		.dkgMiningWindowStart = 10, // dkgPhaseBlocks * 5 = after finalization
		.dkgMiningWindowEnd = 18,
};

static Consensus::LLMQParams llmq400_60 = {
		.type = Consensus::LLMQ_400_60,
		.name = "llmq_400_51",
		.size = 400,
		.minSize = 300,
		.threshold = 240,

		.dkgInterval = 24 * 12, // one DKG every 12 hours
		.dkgPhaseBlocks = 4,
		.dkgMiningWindowStart = 20, // dkgPhaseBlocks * 5 = after finalization
		.dkgMiningWindowEnd = 28,
};

// Used for deployment and min-proto-version signalling, so it needs a higher threshold
static Consensus::LLMQParams llmq400_85 = {
		.type = Consensus::LLMQ_400_85,
		.name = "llmq_400_85",
		.size = 400,
		.minSize = 350,
		.threshold = 340,

		.dkgInterval = 24 * 24, // one DKG every 24 hours
		.dkgPhaseBlocks = 4,
		.dkgMiningWindowStart = 20, // dkgPhaseBlocks * 5 = after finalization
		.dkgMiningWindowEnd = 48, // give it a larger mining window to make sure it is mined
};


/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */


class CMainParams : public CChainParams {
public:
	CMainParams() {
		strNetworkID = "main";
		consensus.nSubsidyHalvingInterval = 51840; // Note: actual number of blocks per calendar year with DGW v3 is ~200700 (for example 449750 - 249050)
		consensus.nMasternodePaymentsStartBlock = 4032; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
		consensus.nMasternodePaymentsIncreaseBlock = 158000; // actual historical value
		consensus.nMasternodePaymentsIncreasePeriod = 576*30; // 17280 - actual historical value
		consensus.nInstantSendConfirmationsRequired = 6;
		consensus.nInstantSendKeepLock = 24;
		consensus.nBudgetPaymentsStartBlock = 180000; // actual historical value
		consensus.nBudgetPaymentsCycleBlocks = 16616; // ~(60*24*30)/2.6, actual number of blocks per month is 200700 / 12 = 16725
		consensus.nBudgetPaymentsWindowBlocks = 100;
		consensus.nSuperblockStartBlock = 180000; // The block at which 12.1 goes live (end of final 12.0 budget cycle)
		consensus.nSuperblockStartHash = uint256();//uint256S("0000000000020cb27c7ef164d21003d5d20cdca2f54dd9a9ca6d45f4d47f8aa3");
		consensus.nSuperblockCycle = 16616; // ~(60*24*30)/2.6, actual number of blocks per month is 200700 / 12 = 16725
		consensus.nGovernanceMinQuorum = 10;
		consensus.nGovernanceFilterElements = 20000;
		consensus.nMasternodeMinimumConfirmations = 15;
		consensus.BIP34Height = 1;
		consensus.BIP34Hash = uint256S("0x000001c800f8dd4695bc5f30711c876552f444fbaade6d344f30e05105fcc852");
		consensus.BIP65Height = 1; // 00000000000076d8fcea02ec0963de4abfd01e771fec0863f960c2c64fe6f357
		consensus.BIP66Height = 1; // 00000000000b1fa2dfa312863570e13fae9ca7b5566cb27e55422620b469aefa
		consensus.DIP0001Height = 1;
		consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 20
		consensus.nPowTargetTimespan = 24 * 60 * 60; // 1 day
		consensus.nPowTargetSpacing = 2.5 * 60; // 2.5 minutes
		consensus.fPowAllowMinDifficultyBlocks = false;
		consensus.fPowNoRetargeting = false;
		consensus.nPowKGWHeight = 1;
		consensus.nPowDGWHeight = 1;
		consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
		consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

		// Deployment of BIP68, BIP112, and BIP113.
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1547856000; // Jan 19, 2019
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1579392000; // Jan 20, 2020

		// Deployment of DIP0001
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1547856000; // Jan 19, 2019
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 1579392000; // Jan 19, 2020
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 4032;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 3226; // 80% of 4032

		// Deployment of BIP147
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1547856000; // Jan 19, 2019
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 1579392000; // Jan 19, 2020
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 4032;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThreshold = 3226; // 80% of 4032

		// Deployment of DIP0003
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = 1557360000; // May 9, 2019
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = 1588982400; // May 9, 2020
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nWindowSize = 4032;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nThreshold = 3226; // 80% of 4032

		// The best chain should have at least this much work.
		consensus.nMinimumChainWork = uint256S("0x00"); // uint256S("0x000000000000000000000000000000000000000000000000000000000c6a97b4"); // 101

		// By default assume that the signatures in ancestors of this block are valid.
		consensus.defaultAssumeValid = uint256S("0x000000ba64e036fa26c9f697f2618ddd43dd7b034cafaf6b19558b23752a9c96"); // 101

		/**
		 * The message start string is designed to be unlikely to occur in normal data.
		 * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
		 * a large 32-bit integer with any alignment.
		 */
		pchMessageStart[0] = 0xb4;
		pchMessageStart[1] = 0xbb;
		pchMessageStart[2] = 0xab;
		pchMessageStart[3] = 0xbc;
		vAlertPubKey = ParseHex("043e2024709225addadbf19d7f46a8d43e94a309e7679ccdbab85ab561c3366fccc1e3b96bb64594e9d41dc67a0f80d318ca2759d6850ee80ba52bdd8681d9f770");
		nDefaultPort = 5190;
		nPruneAfterHeight = 100000;

		genesis = CreateGenesisBlock(1710500000, 84298, 0x1e0ffff0, 1, 1000 * COIN);
		consensus.hashGenesisBlock = genesis.GetHash();
		assert(consensus.hashGenesisBlock == uint256S("0x00000a7009920012e4e7c9e05f9d28061fdcbf0f821b0787be481e1845f07aa1"));
		assert(genesis.hashMerkleRoot == uint256S("0x0a4b141461197460a86302ac1d1b17f688a4982d867b415d3fd3d30fa1b309b4"));


		vSeeds.push_back(CDNSSeedData("blazegeek.com", "explorer.blazegeek.com")); //geeknode01: 147.182.156.59
		vSeeds.push_back(CDNSSeedData("blazegeek.com", "masternode.blazegeek.com")); //geeknode02: 142.93.122.222
		vSeeds.push_back(CDNSSeedData("blazegeek.com", "dev.blazegeek.com")); //geeknode00: 99.243.184.186

	   // Blaze addresses start with 'b'
		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,85);
		// Blaze script addresses start with 'k'
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,107); //76
		// Blaze private keys start with 'S'
		base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,63);
		// Blaze BIP32 pubkeys start with 'xpub' (Bitcoin defaults)
		base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
		// Blaze BIP32 prvkeys start with 'xprv' (Bitcoin defaults)
		base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

		// GeekCash BIP44 coin type is '5'
		nExtCoinType = 5;

		vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

		// long living quorum params
		consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
		consensus.llmqs[Consensus::LLMQ_400_60] = llmq400_60;
		consensus.llmqs[Consensus::LLMQ_400_85] = llmq400_85;

		fMiningRequiresPeers = false;
		fDefaultConsistencyChecks = false;
		fRequireStandard = true;
		fRequireRoutableExternalIP = true;
		fMineBlocksOnDemand = false;
		fAllowMultipleAddressesFromGroup = false;
		fAllowMultiplePorts = false;

		nPoolMaxTransactions = 3;
		nFulfilledRequestExpireTime = 60*60; // fulfilled requests expire in 1 hour

		vSporkAddresses = {"GVsnnCjpqh7hWM9CfXNsKVrfuiJud7yViM"};
		nMinSporkKeys = 1;
		fBIP9CheckMasternodesUpgraded = false;
		consensus.fLLMQAllowDummyCommitments = false;

		checkpointData = (CCheckpointData) {
			boost::assign::map_list_of
			(  0, uint256S("0x00000a7009920012e4e7c9e05f9d28061fdcbf0f821b0787be481e1845f07aa1"))
		};

		chainTxData = ChainTxData{
			1550725301, // * UNIX timestamp of last known number of transactions
			101,    // * total number of transactions between genesis and that timestamp
						//   (the tx=... number in the SetBestChain debug.log lines)
			0.1         // * estimated number of transactions per second after that timestamp
		};
	}
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
	CTestNetParams() {
		strNetworkID = "test";
		consensus.nSubsidyHalvingInterval = 51840;
		consensus.nMasternodePaymentsStartBlock = 10; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
		consensus.nMasternodePaymentsIncreaseBlock = 4030;
		consensus.nMasternodePaymentsIncreasePeriod = 10;
		consensus.nInstantSendConfirmationsRequired = 2;
		consensus.nInstantSendKeepLock = 6;
		consensus.nBudgetPaymentsStartBlock = 500;
		consensus.nBudgetPaymentsCycleBlocks = 50;
		consensus.nBudgetPaymentsWindowBlocks = 10;
		consensus.nSuperblockStartBlock = 500; // NOTE: Should satisfy nSuperblockStartBlock > nBudgetPeymentsStartBlock
		consensus.nSuperblockStartHash = uint256(); // do not check this on testnet
		consensus.nSuperblockCycle = 24; // Superblocks can be issued hourly on testnet
		consensus.nGovernanceMinQuorum = 1;
		consensus.nGovernanceFilterElements = 500;
		consensus.nMasternodeMinimumConfirmations = 1;
		consensus.BIP34Height = 1;
		consensus.BIP34Hash = uint256S("0x");
		consensus.BIP65Height = 1; // 0000039cf01242c7f921dcb4806a5994bc003b48c1973ae0c89b67809c2bb2ab
		consensus.BIP66Height = 1; // 0000002acdd29a14583540cb72e1c5cc83783560e38fa7081495d474fe1671f7
		consensus.DIP0001Height = 10;
		consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 20
		consensus.nPowTargetTimespan = 24 * 60 * 60; // GeekCash: 1 day
		consensus.nPowTargetSpacing = 2.5 * 60; // GeekCash: 2.5 minutes
		consensus.fPowAllowMinDifficultyBlocks = true;
		consensus.fPowNoRetargeting = false;
		consensus.nPowKGWHeight = 1; // nPowKGWHeight >= nPowDGWHeight means "no KGW"
		consensus.nPowDGWHeight = 1;
		consensus.nRuleChangeActivationThreshold = 432; // 75% for testchains
		consensus.nMinerConfirmationWindow = 576; // nPowTargetTimespan / nPowTargetSpacing
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

		// Deployment of BIP68, BIP112, and BIP113.
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1549032900; // Dec 13th, 2018
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1579132800; // Dec 13th, 2019
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nWindowSize = 100;
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nThreshold = 50; // 50% of 100

		// Deployment of DIP0001
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1549119300; // Dec 13th, 2018
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 1579132800; // Dec 13th, 2019
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 100;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 50; // 50% of 100

		// Deployment of BIP147
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1549205700; // Dec 13th, 2018
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 1579132800; // Dec 13th, 2019
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 100;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThreshold = 50; // 50% of 100

		// Deployment of DIP0003
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = 1549292100; // Dec 13th, 2018
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = 1579132800; // Dec 13th, 2019
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nWindowSize = 100;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nThreshold = 50; // 50% of 100

		// The best chain should have at least this much work.
		consensus.nMinimumChainWork = uint256S("0x"); // 4000

		// By default assume that the signatures in ancestors of this block are valid.
		consensus.defaultAssumeValid = uint256S("0x"); // 4000

		pchMessageStart[0] = 0xcb;
		pchMessageStart[1] = 0xba;
		pchMessageStart[2] = 0xbb;
		pchMessageStart[3] = 0xb4;
		vAlertPubKey = ParseHex("048c9d7fb9d9cda05c9f7cb363828bcd4416fbc8c8f71375175f229931f7e2c24b36f066005c6728b28a889b18c3444a09091467ed9d438d8a3394cdf9a83736a9");
		nDefaultPort = 15190;
		nPruneAfterHeight = 1000;

		genesis = CreateGenesisBlock(1710499999, 1321194, 0x1e0ffff0, 1, 1000 * COIN);
		consensus.hashGenesisBlock = genesis.GetHash();
		assert(consensus.hashGenesisBlock == uint256S("0x00000b4e899e583809eda5c87064ce1ba3b7128c70f579e40eb36abcad5f954c"));
		assert(genesis.hashMerkleRoot == uint256S("0x0a4b141461197460a86302ac1d1b17f688a4982d867b415d3fd3d30fa1b309b4"));

		vFixedSeeds.clear();
		vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

		vSeeds.clear();
		// nodes with support for servicebits filtering should be at the top
		vSeeds.push_back(CDNSSeedData("testnet.geekcash.net",  "ns01.testnet.geekcash.net"));
		vSeeds.push_back(CDNSSeedData("testnet.geekcash.net",  "ns02.testnet.geekcash.net"));

	   // Blaze addresses start with 'b'
		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,85);
		// Blaze script addresses start with 'k'
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,107); //76
		// Blaze private keys start with 'S'
		base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,63);
		// Blaze Testnet BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
		base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
		// Blaze Testnet BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
		base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

		// Testnet GeekCash BIP44 coin type is '1' (All coin's testnet default)
		nExtCoinType = 1;

		// long living quorum params
		consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
		consensus.llmqs[Consensus::LLMQ_400_60] = llmq400_60;
		consensus.llmqs[Consensus::LLMQ_400_85] = llmq400_85;

		fMiningRequiresPeers = false;
		fDefaultConsistencyChecks = false;
		fRequireStandard = false;
		fRequireRoutableExternalIP = true;
		fMineBlocksOnDemand = false;
		fAllowMultipleAddressesFromGroup = false;
		fAllowMultiplePorts = false;

		nPoolMaxTransactions = 3;
		nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

		vSporkAddresses = {"GVsnnCjpqh7hWM9CfXNsKVrfuiJud7yViM"};
		nMinSporkKeys = 1;
		fBIP9CheckMasternodesUpgraded = false;
		consensus.fLLMQAllowDummyCommitments = true;

		// checkpointData = (CCheckpointData) {
		//     boost::assign::map_list_of
		//     (    261, uint256S("0x00000c26026d0815a7e2ce4fa270775f61403c040647ff2c3091f99e894a4618"))
		//     (   1999, uint256S("0x00000052e538d27fa53693efe6fb6892a0c1d26c0235f599171c48a3cce553b1"))
		//     (   2999, uint256S("0x0000024bc3f4f4cb30d29827c13d921ad77d2c6072e586c7f60d83c2722cdcc5"))
		// };

		// chainTxData = ChainTxData{
		//     1544707462, // * UNIX timestamp of last known number of transactions
		//     4100,       // * total number of transactions between genesis and that timestamp
		//                 //   (the tx=... number in the SetBestChain debug.log lines)
		//     0.01        // * estimated number of transactions per second after that timestamp
		// };

	}
};
static CTestNetParams testNetParams;

/**
 * Devnet
 */
class CDevNetParams : public CChainParams {
public:
	CDevNetParams() {
		strNetworkID = "dev";
		consensus.nSubsidyHalvingInterval = 51840;
		consensus.nMasternodePaymentsStartBlock = 4010; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
		consensus.nMasternodePaymentsIncreaseBlock = 4030;
		consensus.nMasternodePaymentsIncreasePeriod = 10;
		consensus.nInstantSendConfirmationsRequired = 2;
		consensus.nInstantSendKeepLock = 6;
		consensus.nBudgetPaymentsStartBlock = 4100;
		consensus.nBudgetPaymentsCycleBlocks = 50;
		consensus.nBudgetPaymentsWindowBlocks = 10;
		consensus.nSuperblockStartBlock = 4200; // NOTE: Should satisfy nSuperblockStartBlock > nBudgetPeymentsStartBlock
		consensus.nSuperblockStartHash = uint256(); // do not check this on devnet
		consensus.nSuperblockCycle = 24; // Superblocks can be issued hourly on devnet
		consensus.nGovernanceMinQuorum = 1;
		consensus.nGovernanceFilterElements = 500;
		consensus.nMasternodeMinimumConfirmations = 1;
		consensus.BIP34Height = 1; // BIP34 activated immediately on devnet
		consensus.BIP65Height = 1; // BIP65 activated immediately on devnet
		consensus.BIP66Height = 1; // BIP66 activated immediately on devnet
		consensus.DIP0001Height = 2; // DIP0001 activated immediately on devnet
		consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 1
		consensus.nPowTargetTimespan = 24 * 60 * 60; // GeekCash: 1 day
		consensus.nPowTargetSpacing = 2.5 * 60; // GeekCash: 2.5 minutes
		consensus.fPowAllowMinDifficultyBlocks = true;
		consensus.fPowNoRetargeting = false;
		consensus.nPowKGWHeight = 1; // nPowKGWHeight >= nPowDGWHeight means "no KGW"
		consensus.nPowDGWHeight = 1;
		consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
		consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

		// Deployment of BIP68, BIP112, and BIP113.
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1506556800; // September 28th, 2017
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1538092800; // September 28th, 2018

		// Deployment of DIP0001
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1505692800; // Sep 18th, 2017
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 1537228800; // Sep 18th, 2018
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 100;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 50; // 50% of 100

		// Deployment of BIP147
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1517792400; // Feb 5th, 2018
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 1549328400; // Feb 5th, 2019
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 100;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThreshold = 50; // 50% of 100

		// Deployment of DIP0003
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = 1535752800; // Sep 1st, 2018
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = 1567288800; // Sep 1st, 2019
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nWindowSize = 100;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nThreshold = 50; // 50% of 100

		// The best chain should have at least this much work.
		consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000000000000000000");

		// By default assume that the signatures in ancestors of this block are valid.
		consensus.defaultAssumeValid = uint256S("0x000000000000000000000000000000000000000000000000000000000000000");

		pchMessageStart[0] = 0xcb;
		pchMessageStart[1] = 0xba;
		pchMessageStart[2] = 0xbb;
		pchMessageStart[3] = 0xb4;
		vAlertPubKey = ParseHex("0429324e3dcbeaeddab2d377b4e29576f1c42bfe2dec7a3b5c98976fbc84f9d44091698817a522a497b6005dafb7b80a8a15c5618dd146f24dc5dcdcdf34f0fe41");
		nDefaultPort = 15190;
		nPruneAfterHeight = 1000;

		genesis = CreateGenesisBlock(1710500001, 2233306, 0x207fffff, 1, 1000 * COIN);
		consensus.hashGenesisBlock = genesis.GetHash();
		assert(consensus.hashGenesisBlock == uint256S("000004e3e3afd4a0bf2b95a181f13735d8438fe107b7121d0d61b3ba395ecba4"));
		assert(genesis.hashMerkleRoot == uint256S("0a4b141461197460a86302ac1d1b17f688a4982d867b415d3fd3d30fa1b309b4"));

		devnetGenesis = FindDevNetGenesisBlock(consensus, genesis, 50 * COIN);
		consensus.hashDevnetGenesisBlock = devnetGenesis.GetHash();

		vFixedSeeds.clear();
		vSeeds.clear();
		vSeeds.push_back(CDNSSeedData("dev.geekcash.net",  "ns01.dev.geekcash.net"));
		vSeeds.push_back(CDNSSeedData("dev.geekcash.net",  "ns02.dev.geekcash.net"));

	   // Blaze addresses start with 'b'
		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,85);
		// Blaze script addresses start with 'k'
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,107); //76
		// Blaze private keys start with 'S'
		base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,63);
		// Blaze Devnet BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
		base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
		// Blaze Devnet BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
		base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

		// Testnet GeekCash BIP44 coin type is '1' (All coin's testnet default)
		nExtCoinType = 1;

		// long living quorum params
		consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
		consensus.llmqs[Consensus::LLMQ_400_60] = llmq400_60;
		consensus.llmqs[Consensus::LLMQ_400_85] = llmq400_85;

		fMiningRequiresPeers = false;
		fDefaultConsistencyChecks = false;
		fRequireStandard = false;
		fMineBlocksOnDemand = false;
		fAllowMultipleAddressesFromGroup = true;
		fAllowMultiplePorts = true;

		nPoolMaxTransactions = 3;
		nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

		vSporkAddresses = {"GVsnnCjpqh7hWM9CfXNsKVrfuiJud7yViM"};
		nMinSporkKeys = 1;
		// devnets are started with no blocks and no MN, so we can't check for upgraded MN (as there are none)
		fBIP9CheckMasternodesUpgraded = false;
		consensus.fLLMQAllowDummyCommitments = true;

		// checkpointData = (CCheckpointData) {
		//     boost::assign::map_list_of
		//     (      0, uint256S("0x000008ca1832a4baf228eb1553c03d3a2c8e02399550dd6ea8d65cec3ef23d2e"))
		//     (      1, devnetGenesis.GetHash())
		// };

		// chainTxData = ChainTxData{
		//     devnetGenesis.GetBlockTime(), // * UNIX timestamp of devnet genesis block
		//     2,                            // * we only have 2 coinbase transactions when a devnet is started up
		//     0.01                          // * estimated number of transactions per second
		// };
	}

	void UpdateSubsidyAndDiffParams(int nMinimumDifficultyBlocks, int nHighSubsidyBlocks, int nHighSubsidyFactor)
	{
		consensus.nMinimumDifficultyBlocks = nMinimumDifficultyBlocks;
		consensus.nHighSubsidyBlocks = nHighSubsidyBlocks;
		consensus.nHighSubsidyFactor = nHighSubsidyFactor;
	}
};
static CDevNetParams *devNetParams;


/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
	CRegTestParams() {
		strNetworkID = "regtest";
		consensus.nSubsidyHalvingInterval = 150;
		consensus.nMasternodePaymentsStartBlock = 240;
		consensus.nMasternodePaymentsIncreaseBlock = 350;
		consensus.nMasternodePaymentsIncreasePeriod = 10;
		consensus.nInstantSendConfirmationsRequired = 2;
		consensus.nInstantSendKeepLock = 6;
		consensus.nBudgetPaymentsStartBlock = 1000;
		consensus.nBudgetPaymentsCycleBlocks = 50;
		consensus.nBudgetPaymentsWindowBlocks = 10;
		consensus.nSuperblockStartBlock = 1500;
		consensus.nSuperblockStartHash = uint256(); // do not check this on regtest
		consensus.nSuperblockCycle = 10;
		consensus.nGovernanceMinQuorum = 1;
		consensus.nGovernanceFilterElements = 100;
		consensus.nMasternodeMinimumConfirmations = 1;
		consensus.BIP34Height = 1; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
		consensus.BIP34Hash = uint256();
		consensus.BIP65Height = 1; // BIP65 activated on regtest (Used in rpc activation tests)
		consensus.BIP66Height = 1; // BIP66 activated on regtest (Used in rpc activation tests)
		consensus.DIP0001Height = 1;
		consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 1
		consensus.nPowTargetTimespan = 24 * 60 * 60; // GeekCash: 1 day
		consensus.nPowTargetSpacing = 2.5 * 60; // GeekCash: 2.5 minutes
		consensus.fPowAllowMinDifficultyBlocks = true;
		consensus.fPowNoRetargeting = true;
		consensus.nPowKGWHeight = 1; // same as mainnet
		consensus.nPowDGWHeight = 1; // same as mainnet
		consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
		consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 999999999999ULL;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 999999999999ULL;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = 999999999999ULL;

		// The best chain should have at least this much work.
		consensus.nMinimumChainWork = uint256S("0x00");

		// By default assume that the signatures in ancestors of this block are valid.
		consensus.defaultAssumeValid = uint256S("0x00");

		pchMessageStart[0] = 0xff;
		pchMessageStart[1] = 0xbb;
		pchMessageStart[2] = 0xbb;
		pchMessageStart[3] = 0xcb;
		nDefaultPort = 25190;
		nPruneAfterHeight = 1000;

		// genesis = CreateGenesisBlock(1549022900, 871116, 0x207fffff, 1, 50 * COIN);
		// consensus.hashGenesisBlock = genesis.GetHash();

		// assert(consensus.hashGenesisBlock == uint256S("0x00000599f7218e76bbb3c21a8c5046a555b1e50b7753ca185f79177c48744973"));
		// assert(genesis.hashMerkleRoot == uint256S("0x6d87016979d2f369dcb5fc3a5c284be1a316790cbaabfcce403d24da4b49b210"));

		genesis = CreateGenesisBlock(1710499998, 774698, 0x207fffff, 1, 1000 * COIN);
		consensus.hashGenesisBlock = genesis.GetHash();
		// printf("-----regtest\n");
		// printf("genesis hash: %s\n", consensus.hashGenesisBlock.ToString().c_str());
		// printf("root hash: %s\n", genesis.hashMerkleRoot.ToString().c_str());
		// printf("%s\n", genesis.ToString().c_str());
		assert(consensus.hashGenesisBlock == uint256S("0x00000a36e7e6eb73ea99238bb4442a6551703ab25b8dd81b2b73e5bdf6a8d526"));
		assert(genesis.hashMerkleRoot == uint256S("0x0a4b141461197460a86302ac1d1b17f688a4982d867b415d3fd3d30fa1b309b4"));

		vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
		vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

		fMiningRequiresPeers = false;
		fDefaultConsistencyChecks = true;
		fRequireStandard = false;
		fRequireRoutableExternalIP = false;
		fMineBlocksOnDemand = true;
		fAllowMultipleAddressesFromGroup = true;
		fAllowMultiplePorts = true;

		nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

		// privKey: cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK
		vSporkAddresses = {"GVsnnCjpqh7hWM9CfXNsKVrfuiJud7yViM"};
		nMinSporkKeys = 1;
		// regtest usually has no masternodes in most tests, so don't check for upgraged MNs
		fBIP9CheckMasternodesUpgraded = false;
		consensus.fLLMQAllowDummyCommitments = true;

		// checkpointData = (CCheckpointData){
		//     boost::assign::map_list_of
		//     ( 0, uint256S("0x000008ca1832a4baf228eb1553c03d3a2c8e02399550dd6ea8d65cec3ef23d2e"))
		// };

		// chainTxData = ChainTxData{
		//     0,
		//     0,
		//     0
		// };

	   // Blaze addresses start with 'b'
		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,85);
		// Blaze script addresses start with 'k'
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,107); //76
		// Blaze private keys start with 'S'
		base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,63);
		// Blaze RegTest BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
		base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
		// Blaze RegTest BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
		base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

		// Regtest GeekCash BIP44 coin type is '1' (All coin's testnet default)
		nExtCoinType = 1;

		// long living quorum params
		consensus.llmqs[Consensus::LLMQ_10_60] = llmq10_60;
		consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
	}

	void UpdateBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
	{
		consensus.vDeployments[d].nStartTime = nStartTime;
		consensus.vDeployments[d].nTimeout = nTimeout;
	}

	void UpdateBudgetParameters(int nMasternodePaymentsStartBlock, int nBudgetPaymentsStartBlock, int nSuperblockStartBlock)
	{
		consensus.nMasternodePaymentsStartBlock = nMasternodePaymentsStartBlock;
		consensus.nBudgetPaymentsStartBlock = nBudgetPaymentsStartBlock;
		consensus.nSuperblockStartBlock = nSuperblockStartBlock;
	}
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
	assert(pCurrentParams);
	return *pCurrentParams;
}

CChainParams& Params(const std::string& chain)
{
	if (chain == CBaseChainParams::MAIN)
			return mainParams;
	else if (chain == CBaseChainParams::TESTNET)
			return testNetParams;
	else if (chain == CBaseChainParams::DEVNET) {
			assert(devNetParams);
			return *devNetParams;
	} else if (chain == CBaseChainParams::REGTEST)
			return regTestParams;
	else
		throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
	if (network == CBaseChainParams::DEVNET) {
		devNetParams = (CDevNetParams*)new uint8_t[sizeof(CDevNetParams)];
		memset(devNetParams, 0, sizeof(CDevNetParams));
		new (devNetParams) CDevNetParams();
	}

	SelectBaseParams(network);
	pCurrentParams = &Params(network);
}

void UpdateRegtestBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
	regTestParams.UpdateBIP9Parameters(d, nStartTime, nTimeout);
}

void UpdateRegtestBudgetParameters(int nMasternodePaymentsStartBlock, int nBudgetPaymentsStartBlock, int nSuperblockStartBlock)
{
	regTestParams.UpdateBudgetParameters(nMasternodePaymentsStartBlock, nBudgetPaymentsStartBlock, nSuperblockStartBlock);
}

void UpdateDevnetSubsidyAndDiffParams(int nMinimumDifficultyBlocks, int nHighSubsidyBlocks, int nHighSubsidyFactor)
{
	assert(devNetParams);
	devNetParams->UpdateSubsidyAndDiffParams(nMinimumDifficultyBlocks, nHighSubsidyBlocks, nHighSubsidyFactor);
}
