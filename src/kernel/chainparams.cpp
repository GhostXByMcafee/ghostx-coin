// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/amount.h>
#include <consensus/merkle.h>
#include <consensus/params.h>
#include <hash.h>
#include <kernel/messagestartchars.h>
#include <logging.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <uint256.h>
#include <util/chaintype.h>
#include <util/strencodings.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <type_traits>

// Particl
#include <key/keyutil.h>
#include <chain/chainparamsimport.h>

int64_t CChainParams::GetCoinYearReward(int64_t nTime) const
{
    static const int64_t nSecondsInYear = 365 * 24 * 60 * 60;

    if (GetChainType() != ChainType::REGTEST) {
        // After HF2: 8%, 8%, 7%, 7%, 6%
        if (nTime >= consensus.exploit_fix_2_time) {
            int64_t nPeriodsSinceHF2 = (nTime - consensus.exploit_fix_2_time) / (nSecondsInYear * 2);
            if (nPeriodsSinceHF2 >= 0 && nPeriodsSinceHF2 < 2) {
                return (8 - nPeriodsSinceHF2) * CENT;
            }
            return 6 * CENT;
        }

        // Y1 5%, Y2 4%, Y3 3%, Y4 2%, ... YN 2%
        int64_t nYearsSinceGenesis = (nTime - genesis.nTime) / nSecondsInYear;
        if (nYearsSinceGenesis >= 0 && nYearsSinceGenesis < 3) {
            return (5 - nYearsSinceGenesis) * CENT;
        }
    }
    return nCoinYearReward;
};

bool CChainParams::PushTreasuryFundSettings(int64_t time_from, particl::TreasuryFundSettings &settings)
{
    if (settings.nMinTreasuryStakePercent < 0 or settings.nMinTreasuryStakePercent > 100) {
        throw std::runtime_error("minstakepercent must be in range [0, 100].");
    }
    vTreasuryFundSettings.emplace_back(time_from, settings);
    return true;
};

int64_t CChainParams::GetMaxSmsgFeeRateDelta(int64_t smsg_fee_prev, int64_t time) const
{
    int64_t max_delta = (smsg_fee_prev * consensus.smsg_fee_max_delta_percent) / 1000000;
    return std::max((int64_t)1, max_delta);
};

bool CChainParams::CheckImportCoinbase(int nHeight, uint256 &hash) const
{
    for (auto &cth : vImportedCoinbaseTxns) {
        if (cth.nHeight != (uint32_t)nHeight) {
            continue;
        }
        if (hash == cth.hash) {
            return true;
        }
        return error("%s - Hash mismatch at height %d: %s, expect %s.", __func__, nHeight, hash.ToString(), cth.hash.ToString());
    }
    return error("%s - Unknown height.", __func__);
};

const particl::TreasuryFundSettings *CChainParams::GetTreasuryFundSettings(int64_t nTime) const
{
    for (auto i = vTreasuryFundSettings.crbegin(); i != vTreasuryFundSettings.crend(); ++i) {
        if (nTime > i->first) {
            return &i->second;
        }
    }
    return nullptr;
};

bool CChainParams::IsBech32Prefix(const std::vector<unsigned char> &vchPrefixIn) const
{
    for (auto &hrp : bech32Prefixes)  {
        if (vchPrefixIn == hrp) {
            return true;
        }
    }
    return false;
};

bool CChainParams::IsBech32Prefix(const std::vector<unsigned char> &vchPrefixIn, CChainParams::Base58Type &rtype) const
{
    for (size_t k = 0; k < MAX_BASE58_TYPES; ++k) {
        auto &hrp = bech32Prefixes[k];
        if (vchPrefixIn == hrp) {
            rtype = static_cast<CChainParams::Base58Type>(k);
            return true;
        }
    }
    return false;
};

bool CChainParams::IsBech32Prefix(const char *ps, size_t slen, CChainParams::Base58Type &rtype) const
{
    for (size_t k = 0; k < MAX_BASE58_TYPES; ++k) {
        const auto &hrp = bech32Prefixes[k];
        size_t hrplen = hrp.size();
        if (hrplen > 0 &&
            slen > hrplen &&
            strncmp(ps, (const char*)&hrp[0], hrplen) == 0) {
            rtype = static_cast<CChainParams::Base58Type>(k);
            return true;
        }
    }
    return false;
};

namespace ghost {
    const std::pair<const char*, CAmount> regTestOutputs[] = {
        std::make_pair("585c2b3914d9ee51f8e710304e386531c3abcc82", 10000 * COIN),
        std::make_pair("c33f3603ce7c46b423536f0434155dad8ee2aa1f", 10000 * COIN),
        std::make_pair("72d83540ed1dcf28bfaca3fa2ed77100c2808825", 10000 * COIN),
        std::make_pair("69e4cc4c219d8971a253cd5db69a0c99c4a5659d", 10000 * COIN),
        std::make_pair("eab5ed88d97e50c87615a015771e220ab0a0991a", 10000 * COIN),
        std::make_pair("119668a93761a34a4ba1c065794b26733975904f", 10000 * COIN),
        std::make_pair("6da49762a4402d199d41d5778fcb69de19abbe9f", 10000 * COIN),
        std::make_pair("27974d10ff5ba65052be7461d89ef2185acbe411", 10000 * COIN),
        std::make_pair("89ea3129b8dbf1238b20a50211d50d462a988f61", 10000 * COIN),
        std::make_pair("3baab5b42a409b7c6848a95dfd06ff792511d561", 10000 * COIN),
    
        std::make_pair("649b801848cc0c32993fb39927654969a5af27b0", 5000 * COIN),
        std::make_pair("d669de30fa30c3e64a0303cb13df12391a2f7256", 5000 * COIN),
        std::make_pair("f0c0e3ebe4a1334ed6a5e9c1e069ef425c529934", 5000 * COIN),
        std::make_pair("27189afe71ca423856de5f17538a069f22385422", 5000 * COIN),
        std::make_pair("0e7f6fe0c4a5a6a9bfd18f7effdd5898b1f40b80", 5000 * COIN)
};

static const size_t nGenesisOutputsRegtest = sizeof(regTestOutputs) / sizeof(regTestOutputs[0]);

const std::pair<const char*, CAmount> genesisOutputs[] = {
    std::make_pair("88a8a3f770392d3ec17db9ba01b9e04e340cca26", 10000000 * COIN),
};

static const size_t nGenesisOutputs = sizeof(genesisOutputs) / sizeof(genesisOutputs[0]);

static const std::pair<const char*, CAmount> genesisOutputsTestnet[] = {
    std::make_pair("88a8a3f770392d3ec17db9ba01b9e04e340cca26", 800000 * COIN),
    std::make_pair("a8b935d5c9eaf4aa6c61d7e6a45dfef34d07a55c", 800000 * COIN),
    std::make_pair("3b5e152abb5e9fc16f4dfbf18eb7e7ec321a3405", 800000 * COIN),
    std::make_pair("da687cf68a49dafab5429fb02a03b0a1c2eb7039", 800000 * COIN),
    std::make_pair("e893ea337bca06f5da14e0452d3cebe1de79f0e1", 800000 * COIN),
    std::make_pair("bdbffa18c6c1e73f1f41ec625b5a974f8575521d", 800000 * COIN),
    std::make_pair("267ab09c4395de5f744f01ef5335b3674cb1d478", 800000 * COIN),
    std::make_pair("7ce0085b0046b884557c6ce6b1c112057cc6a401", 800000 * COIN),
    std::make_pair("ac028fa1ca4f9acfa47425ac601fc1e937121299", 800000 * COIN),
    std::make_pair("1f21e8d8edfa6ad32b23d9cc7838f91a1a3cb956", 800000 * COIN)
};

static const size_t nGenesisOutputsTestnet = sizeof(genesisOutputsTestnet) / sizeof(genesisOutputsTestnet[0]);

static CBlock CreateGenesisBlockRegTest(uint32_t nTime, uint32_t nNonce, uint32_t nBits)
{
    const char *pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";

    CMutableTransaction txNew;
    txNew.version = PARTICL_TXN_VERSION;
    txNew.SetType(TXN_COINBASE);
    txNew.vin.resize(1);
    uint32_t nHeight = 0;  // bip34
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp)) << OP_RETURN << nHeight;

    txNew.vpout.resize(nGenesisOutputsRegtest);
    for (size_t k = 0; k < nGenesisOutputsRegtest; ++k) {
        OUTPUT_PTR<CTxOutStandard> out = MAKE_OUTPUT<CTxOutStandard>();
        out->nValue = regTestOutputs[k].second;
        out->scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ParseHex(regTestOutputs[k].first) << OP_EQUALVERIFY << OP_CHECKSIG;
        txNew.vpout[k] = out;
    }

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = PARTICL_BLOCK_VERSION;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));

    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    genesis.hashWitnessMerkleRoot = BlockWitnessMerkleRoot(genesis);

    return genesis;
}

static CBlock CreateGenesisBlockTestNet(uint32_t nTime, uint32_t nNonce, uint32_t nBits)
{
    const char *pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";

    CMutableTransaction txNew;
    txNew.version = PARTICL_TXN_VERSION;
    txNew.SetType(TXN_COINBASE);
    txNew.vin.resize(1);
    uint32_t nHeight = 0;  // bip34
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp)) << OP_RETURN << nHeight;

    txNew.vpout.resize(nGenesisOutputsTestnet);
    for (size_t k = 0; k < nGenesisOutputsTestnet; ++k) {
        OUTPUT_PTR<CTxOutStandard> out = MAKE_OUTPUT<CTxOutStandard>();
        out->nValue = genesisOutputsTestnet[k].second;
        out->scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ParseHex(genesisOutputsTestnet[k].first) << OP_EQUALVERIFY << OP_CHECKSIG;
        txNew.vpout[k] = out;
    }

    // Foundation Fund Raiser Funds
    // rVDQRVBKnQEfNmykMSY9DHgqv8s7XZSf5R fc118af69f63d426f61c6a4bf38b56bcdaf8d069
    OUTPUT_PTR<CTxOutStandard> out = MAKE_OUTPUT<CTxOutStandard>();
    out->nValue = 397364 * COIN;
    out->scriptPubKey = CScript() << OP_HASH160 << ParseHex("fc118af69f63d426f61c6a4bf38b56bcdaf8d069") << OP_EQUAL;
    txNew.vpout.push_back(out);

    // rVDQRVBKnQEfNmykMSY9DHgqv8s7XZSf5R fc118af69f63d426f61c6a4bf38b56bcdaf8d069
    out = MAKE_OUTPUT<CTxOutStandard>();
    out->nValue = 296138 * COIN;
    out->scriptPubKey = CScript() << OP_HASH160 << ParseHex("89ca93e03119d53fd9ad1e65ce22b6f8791f8a49") << OP_EQUAL;
    txNew.vpout.push_back(out);

    // Community Initiative
    // rAybJ7dx4t6heHy99WqGcXkoT4Bh3V9qZ8 340288104577fcc3a6a84b98f7eac1a54e5287ee
    out = MAKE_OUTPUT<CTxOutStandard>();
    out->nValue = 156675 * COIN;
    out->scriptPubKey = CScript() << OP_HASH160 << ParseHex("89ca93e03119d53fd9ad1e65ce22b6f8791f8a49") << OP_EQUAL;
    txNew.vpout.push_back(out);

    // Contributors Left Over Funds
    // rAvmLShYFZ78aAHhFfUFsrHMoBuPPyckm5 3379aa2a4379ae6c51c7777d72e8e0ffff71881b
    out = MAKE_OUTPUT<CTxOutStandard>();
    out->nValue = 216346 * COIN;
    out->scriptPubKey = CScript() << OP_HASH160 << ParseHex("89ca93e03119d53fd9ad1e65ce22b6f8791f8a49") << OP_EQUAL;
    txNew.vpout.push_back(out);

    // Reserved Particl for primary round
    // rLWLm1Hp7im3mq44Y1DgyirYgwvrmRASib 9c8c6c8c698f074180ecfdb38e8265c11f2a62cf
    out = MAKE_OUTPUT<CTxOutStandard>();
    out->nValue = 996000 * COIN;
    out->scriptPubKey = CScript() << 1512000000 << OP_CHECKLOCKTIMEVERIFY << OP_DROP << OP_HASH160<< ParseHex("9c8c6c8c698f074180ecfdb38e8265c11f2a62cf") << OP_EQUAL; // 2017-11-30
    txNew.vpout.push_back(out);


    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = PARTICL_BLOCK_VERSION;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));

    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    genesis.hashWitnessMerkleRoot = BlockWitnessMerkleRoot(genesis);

    return genesis;
}

static CBlock CreateGenesisBlockMainNet(uint32_t nTime, uint32_t nNonce, uint32_t nBits)
{
    const char *pszTimestamp = "BTC 000000000000000000c679bc2209676d05129834627c7b1c02d1018b224c6f37";

    CMutableTransaction txNew;
    txNew.version = PARTICL_TXN_VERSION;
    txNew.SetType(TXN_COINBASE);

    txNew.vin.resize(1);
    uint32_t nHeight = 0;  // bip34
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp)) << OP_RETURN << nHeight;

    txNew.vpout.resize(nGenesisOutputs);
    for (size_t k = 0; k < nGenesisOutputs; ++k) {
        OUTPUT_PTR<CTxOutStandard> out = MAKE_OUTPUT<CTxOutStandard>();
        out->nValue = genesisOutputs[k].second;
        out->scriptPubKey = CScript() << OP_DUP << OP_HASH160 << ParseHex(genesisOutputs[k].first) << OP_EQUALVERIFY << OP_CHECKSIG;
        txNew.vpout[k] = out;
    }

    // Foundation Fund Raiser Funds
    // RHFKJkrB4H38APUDVckr7TDwrK11N7V7mx
    OUTPUT_PTR<CTxOutStandard> out = MAKE_OUTPUT<CTxOutStandard>();
    out->nValue = 397364 * COIN;
    out->scriptPubKey = CScript() << OP_HASH160 << ParseHex("5766354dcb13caff682ed9451b9fe5bbb786996c") << OP_EQUAL;
    txNew.vpout.push_back(out);

    out = MAKE_OUTPUT<CTxOutStandard>();
    out->nValue = 296138 * COIN;
    out->scriptPubKey = CScript() << OP_HASH160 << ParseHex("5766354dcb13caff682ed9451b9fe5bbb786996c") << OP_EQUAL;
    txNew.vpout.push_back(out);

    // Community Initative
    // RKKgSiQcMjbC8TABRoyyny1gTU4fAEiQz9
    out = MAKE_OUTPUT<CTxOutStandard>();
    out->nValue = 156675 * COIN;
    out->scriptPubKey = CScript() << OP_HASH160 << ParseHex("6e29c4a11fd54916d024af16ca913cdf8f89cb31") << OP_EQUAL;
    txNew.vpout.push_back(out);

    // Contributors Left Over Funds
    // RKiaVeyLUp7EmwHtCP92j8Vc1AodhpWi2U
    out = MAKE_OUTPUT<CTxOutStandard>();
    out->nValue = 216346 * COIN;
    out->scriptPubKey = CScript() << OP_HASH160 << ParseHex("727e5e75929bbf26912dd7833971d77e7450a33e") << OP_EQUAL;
    txNew.vpout.push_back(out);

    // Reserved Particl for primary round
    // RNnoeeqBTkpPQH8d29Gf45dszVj9RtbmCu
    out = MAKE_OUTPUT<CTxOutStandard>();
    out->nValue = 996000 * COIN;
    out->scriptPubKey = CScript() << 1512000000 << OP_CHECKLOCKTIMEVERIFY << OP_DROP << OP_HASH160<< ParseHex("9433643b4fd5de3ebd7fdd68675f978f34585af1") << OP_EQUAL; // 2017-11-30
    txNew.vpout.push_back(out);


    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = PARTICL_BLOCK_VERSION;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));

    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    genesis.hashWitnessMerkleRoot = BlockWitnessMerkleRoot(genesis);

    return genesis;
}
} // namespace particl
using namespace util::hex_literals;

// Workaround MSVC bug triggering C7595 when calling consteval constructors in
// initializer lists.
// A fix may be on the way:
// https://developercommunity.visualstudio.com/t/consteval-conversion-function-fails/1579014
#if defined(_MSC_VER)
auto consteval_ctor(auto&& input) { return input; }
#else
#define consteval_ctor(input) (input)
#endif

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.version = 1;
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

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    const CScript genesisOutputScript = CScript() << "04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f"_hex << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

void CChainParams::SetOld()
{
    consensus.m_particl_mode = false;
    if (GetChainType() == ChainType::MAIN) {
        consensus.script_flag_exceptions.clear();
        consensus.script_flag_exceptions.emplace( // BIP16 exception
            uint256{"00000000000002dc756eebf4f49723ed8d30cc28a5f108eb94b1ba88ac4f9c22"}, SCRIPT_VERIFY_NONE);
        consensus.script_flag_exceptions.emplace( // Taproot exception
            uint256{"0000000000000000000f14c35b2d841e986ab5441de8c585d5ffe55ea1e395ad"}, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS);
        consensus.BIP34Height = 227931;
        consensus.BIP34Hash = uint256{"000000000000024b89b42a942fe0d9fea3bb44ab7bd1b19115dd6a759c0808b8"};
        consensus.BIP65Height = 388381; // 000000000000000004c2b624ed5d7756c508d90fd0da2c7c679febfa6c4735f0
        consensus.BIP66Height = 363725; // 00000000000000000379eaa19dce8c9b722d46ae6a57c2f1a988119488b50931
        consensus.CSVHeight = 419328; // 000000000000000004a1b34462cb8aeebd5799177f7a29cf28f2d1961716b5b5
        consensus.SegwitHeight = 481824; // 0000000000000000001c8018d9cb3b742ef25114f27563e3fc4a1902167f9893
        consensus.MinBIP9WarningHeight = consensus.SegwitHeight + consensus.nMinerConfirmationWindow;
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

        genesis = CreateGenesisBlock(1231006505, 2083236893, 0x1d00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();

        pchMessageStart[0] = 0xf9;
        pchMessageStart[1] = 0xbe;
        pchMessageStart[2] = 0xb4;
        pchMessageStart[3] = 0xd9;

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,0);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,5);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,128);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "bc";
    } else
    if (GetChainType() == ChainType::TESTNET) {
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

        genesis = CreateGenesisBlock(1296688602, 414098458, 0x1d00ffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tb";
    } else
    if (GetChainType() == ChainType::REGTEST) {
        genesis = CreateGenesisBlock(1296688602, 2, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        /*
        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        */

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "bcrt";
    }
};

/**
 * Main network on which people trade goods and services.
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        m_chain_type = ChainType::MAIN;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP34Height = 0;
        consensus.BIP65Height = 0;
        consensus.BIP66Height = 0;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 0;
        consensus.MinBIP9WarningHeight = 0;

        consensus.OpIsCoinstakeTime = 0x5A04EC00;       // 2017-11-10 00:00:00 UTC
        consensus.fAllowOpIsCoinstakeWithP2PKH = false;
        consensus.nPaidSmsgTime = 0x5C791EC0;           // 2019-03-01 12:00:00 UTC
        consensus.smsg_fee_time = 0x5D2DBC40;           // 2019-07-16 12:00:00 UTC
        consensus.bulletproof_time = 0x5D2DBC40;        // 2019-07-16 12:00:00 UTC
        consensus.rct_time = 0x5D2DBC40;                // 2019-07-16 12:00:00 UTC
        consensus.smsg_difficulty_time = 0x5D2DBC40;    // 2019-07-16 12:00:00 UTC
        consensus.exploit_fix_1_time = 1614268800;      // 2021-02-25 16:00:00 UTC
        consensus.exploit_fix_2_time = 1626109200;      // 2021-07-12 17:00:00 UTC
        consensus.exploit_fix_2_height = 976263;

        consensus.clamp_tx_version_time = 1643734800;   // 2022-02-01 17:00:00 UTC
        consensus.exploit_fix_3_time = 1643734800;      // 2022-02-01 17:00:00 UTC
        consensus.m_taproot_time = 1643734800;          // 2022-02-01 17:00:00 UTC

        consensus.m_frozen_anon_index = 27340;
        consensus.m_frozen_blinded_height = 884433;

        consensus.smsg_fee_period = 5040;
        consensus.smsg_fee_funding_tx_per_k = 200000;
        consensus.smsg_fee_msg_per_day_per_k = 50000;
        consensus.smsg_fee_max_delta_percent = 43;
        consensus.smsg_min_difficulty = 0x1effffff;
        consensus.smsg_difficulty_max_delta = 0xffff;

        consensus.powLimit = uint256S("000000000000bfffffffffffffffffffffffffffffffffffffffffffffffffff");

        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.enforce_BIP94 = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1815; // 90% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Deployment of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = 1619222400; // April 24th, 2021
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = 1628640000; // August 11th, 2021
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 709632; // Approximately November 12th, 2021

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000001748b2094c246d2cd8d");
        consensus.defaultAssumeValid = uint256S("0xcdf136797304d6e8eb2b0ab8b4e3805261048d247ea31aaeefbd6d10a56e35bb"); // 1558245

        consensus.nMinRCTOutputDepth = 12;

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xfb;
        pchMessageStart[1] = 0xf2;
        pchMessageStart[2] = 0xef;
        pchMessageStart[3] = 0xb4;
        nDefaultPort = 51738;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 4;
        m_assumed_chain_state_size = 3;

        nBIP44ID = (int)WithHardenedBit(44);
        assert(nBIP44ID == (int)0x8000002C);
        nModifierInterval = 10 * 60;    // 10 minutes
        nStakeMinConfirmations = 225;   // 225 * 2 minutes
        nTargetSpacing = 120;           // 2 minutes
        nTargetTimespan = 24 * 60;      // 24 mins

        particl::AddImportHashesMain(vImportedCoinbaseTxns);
        SetLastImportHeight();

        genesis = particl::CreateGenesisBlockMainNet(1500296400, 31429, 0x1f00ffff); // 2017-07-17 13:00:00
        consensus.hashGenesisBlock = genesis.GetHash();

        assert(consensus.hashGenesisBlock == uint256S("0x0000ee0784c195317ac95623e22fddb8c7b8825dc3998e0bb924d66866eccf4c"));
        assert(genesis.hashMerkleRoot == uint256S("0xc95fb023cf4bc02ddfed1a59e2b2f53edd1a726683209e2780332edf554f1e3e"));
        assert(genesis.hashWitnessMerkleRoot == uint256S("0x619e94a7f9f04c8a1d018eb8bcd9c42d3c23171ebed8f351872256e36959d66c"));

        // Note that of those which support the service bits prefix, most only support a subset of
        // possible options.
        // This is fine at runtime as we'll fall back to using them as an addrfetch if they don't support the
        // service bits we want, but we should get them updated to support all service bits wanted by any
        // release ASAP to avoid it where possible.
        vSeeds.emplace_back("mainnet-seed.particl.io");
        vSeeds.emplace_back("dnsseed-mainnet.particl.io");
        vSeeds.emplace_back("mainnet.particl.io");
        vSeeds.emplace_back("dnsseed.tecnovert.net");


        vTreasuryFundSettings.emplace_back(0,
            particl::TreasuryFundSettings("RJAPhgckEgRGVPZa9WoGSWW24spskSfLTQ", 10, 60));
        vTreasuryFundSettings.emplace_back(consensus.OpIsCoinstakeTime,
            particl::TreasuryFundSettings("RBiiQBnQsVPPQkUaJVQTjsZM9K2xMKozST", 10, 60));
        vTreasuryFundSettings.emplace_back(consensus.exploit_fix_2_time,
            particl::TreasuryFundSettings("RQYUDd3EJohpjq62So4ftcV5XZfxZxJPe9", 50, 650));


        base58Prefixes[PUBKEY_ADDRESS]     = {0x38}; // P
        base58Prefixes[SCRIPT_ADDRESS]     = {0x3c};
        base58Prefixes[PUBKEY_ADDRESS_256] = {0x39};
        base58Prefixes[SCRIPT_ADDRESS_256] = {0x3d};
        base58Prefixes[SECRET_KEY]         = {0x6c};
        base58Prefixes[EXT_PUBLIC_KEY]     = {0x69, 0x6e, 0x82, 0xd1}; // PPAR
        base58Prefixes[EXT_SECRET_KEY]     = {0x8f, 0x1d, 0xae, 0xb8}; // XPAR
        base58Prefixes[STEALTH_ADDRESS]    = {0x14};
        base58Prefixes[EXT_KEY_HASH]       = {0x4b}; // X
        base58Prefixes[EXT_ACC_HASH]       = {0x17}; // A
        base58Prefixes[EXT_PUBLIC_KEY_BTC] = {0x04, 0x88, 0xB2, 0x1E}; // xpub
        base58Prefixes[EXT_SECRET_KEY_BTC] = {0x04, 0x88, 0xAD, 0xE4}; // xprv

        bech32Prefixes[PUBKEY_ADDRESS].assign       ("ph",(const char*)"ph"+2);
        bech32Prefixes[SCRIPT_ADDRESS].assign       ("pr",(const char*)"pr"+2);
        bech32Prefixes[PUBKEY_ADDRESS_256].assign   ("pl",(const char*)"pl"+2);
        bech32Prefixes[SCRIPT_ADDRESS_256].assign   ("pj",(const char*)"pj"+2);
        bech32Prefixes[SECRET_KEY].assign           ("px",(const char*)"px"+2);
        bech32Prefixes[EXT_PUBLIC_KEY].assign       ("pep",(const char*)"pep"+3);
        bech32Prefixes[EXT_SECRET_KEY].assign       ("pex",(const char*)"pex"+3);
        bech32Prefixes[STEALTH_ADDRESS].assign      ("ps",(const char*)"ps"+2);
        bech32Prefixes[EXT_KEY_HASH].assign         ("pek",(const char*)"pek"+3);
        bech32Prefixes[EXT_ACC_HASH].assign         ("pea",(const char*)"pea"+3);
        bech32Prefixes[STAKE_ONLY_PKADDR].assign    ("pcs",(const char*)"pcs"+3);

        bech32_hrp = "pw";
        vFixedSeeds = std::vector<uint8_t>(std::begin(chainparams_seed_main), std::end(chainparams_seed_main));

        fDefaultConsistencyChecks = false;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                { 5000,     uint256S("0xe786020ab94bc5461a07d744f3631a811b4ebf424fceda12274f2321883713f4")},
                { 15000,    uint256S("0xafc73ac299f2e6dd309077d230fccef547b9fc24379c1bf324dd3683b13c61c3")},
                { 30000,    uint256S("0x35d95c12799323d7b418fd64df9d88ef67ef27f057d54033b5b2f38a5ecaacbf")},
                { 91000,    uint256S("0x4d1ffaa5b51431918a0c74345e2672035c743511359ac8b1be67467b02ff884c")},
                { 112250,   uint256S("0x89e4b23471aea7a875df835d6f89613fd87ba649e7a1d8cb892917d0080ef337")},
                { 213800,   uint256S("0xfd6c0e5f7444a9e09a5fa1652db73d5b8628aeabe162529a5356be700509aa80")},
                { 303640,   uint256S("0x7cc035d7888ee6d824cec8ff01a6287a71873d874f72a5fd3706d227b88f8e99")},
                { 443228,   uint256S("0x1e2ae3edb2fa5b398c2f719d2bbb44b3089fb96170b6676c0c963f12bceba489")},
                { 583322,   uint256S("0x2be0224e40ddf4763f61ff6db088806f3ad5c6530ea7a6801b067ecbbd13fec9")},
                { 634566,   uint256S("0xc330a61e218b06d3d567c459b54e83ab682a366fc00b77d69dd78c6ed9655a2e")},
                { 777625,   uint256S("0x75b2b1412610c1ff54e49fc38222f3f45fe934b0e485ccae7b5d461b94510734")},
                { 856749,   uint256S("0x6b705dbf87345594314152841212a532753f11ec711ac81afc64f31eb048df19")},
                { 887180,   uint256S("0xf9f1e91f82e73d4781052e42c8b814b8265e0929d4c16284db3feb354bfc317c")},
                { 962370,   uint256S("0x43c3d5568f3b3467e5142f86445d5b12b923e3e5c4a1e6566d90a7fad807799c")},
                { 992790,   uint256S("0xea97054e60558199d5c94627116fbfa9f3be1c63d45510963d1a308fe152974b")},
                { 1026710,  uint256S("0xdcbe7be05060974cca33a52790e047a73358812102a97cd5b3089f2469bd6601")},
                { 1075660,  uint256S("0x1357966bfddceb8ea97f15da76e61087be6254d0b62ce9d4959ceee9b75c89f3")},
                { 1102310,  uint256S("0x4e43167170e4639dbb1fdb84cb5735853f9595ff42713c143b70fd0625a382cd")},
                { 1117588,  uint256S("0x6b13071bc3f5689a3f8a29808f726654283714461076ea890f4272574ff2659f")},
                { 1159409,  uint256S("0x4e0328ad2e2cb4fe6cb9a46c675a00673ff0e2e3c8185d7055e404adefbc7a96")},
                { 1236270,  uint256S("0xb35f0e0cde108c282380d6850bae83c1fd2e961781e2289e2b2cd3d17ff039ab")},
                { 1303280,  uint256S("0x6d3abd8a80371e78957cc30af3e84c45c44b2eb26175b50c5ba7ebc9e990189c")},
                { 1412800,  uint256S("0xf47bc3f695a6e16196babdc8a1e21974622c288bba808ce30efcaf86a28099bd")},
                { 1503450,  uint256S("0x3e1f03d1f294a04ffc3aa8e8a2ed6bc150c66d7806c79c10808637a12ae74690")},
                { 1558245,  uint256S("0xcdf136797304d6e8eb2b0ab8b4e3805261048d247ea31aaeefbd6d10a56e35bb")},
            }
        };

        m_assumeutxo_data = {
            {
                .height = 840'000,
                .hash_serialized = AssumeutxoHash{uint256{"a2a5521b1b5ab65f67818e5e8eccabb7171a517f9e2382208f77687310768f96"}},
                .m_chain_tx_count = 991032194,
                .blockhash = consteval_ctor(uint256{"0000000000000000000320283a032748cef8227873ff4872689bf23f1cda83a5"}),
            },
            {
                .height = 880'000,
                .hash_serialized = AssumeutxoHash{uint256{"dbd190983eaf433ef7c15f78a278ae42c00ef52e0fd2a54953782175fbadcea9"}},
                .m_chain_tx_count = 1145604538,
                .blockhash = consteval_ctor(uint256{"000000000000000000010b17283c3c400507969a9c2afd1dcf2082ec5cca2880"}),
            }
        };

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 4096 00000000000000000001b658dd1120e82e66d2790811f89ede9742ada3ed6d77
            .nTime    = 1741017141,
            .tx_count = 1161875261,
            .dTxRate  = 4.620728156243148,
        };
    }
};

/**
 * Testnet (v3): public test network which is reset from time to time.
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        m_chain_type = ChainType::TESTNET;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP34Height = 0;
        consensus.BIP65Height = 0;
        consensus.BIP66Height = 0;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 0;
        consensus.MinBIP9WarningHeight = 0;

        consensus.OpIsCoinstakeTime = 0;
        consensus.fAllowOpIsCoinstakeWithP2PKH = true; // TODO: clear for next testnet
        consensus.nPaidSmsgTime = 0;
        consensus.smsg_fee_time = 0x5C67FB40;           // 2019-02-16 12:00:00
        consensus.bulletproof_time = 0x5C67FB40;        // 2019-02-16 12:00:00
        consensus.rct_time = 0;
        consensus.smsg_difficulty_time = 0x5D19F5C0;    // 2019-07-01 12:00:00
        consensus.exploit_fix_1_time = 1614268800;      // 2021-02-25 16:00:00

        consensus.clamp_tx_version_time = 1641056400;   // 2022-01-01 17:00:00 UTC
        consensus.m_taproot_time = 1641056400;          // 2022-01-01 17:00:00 UTC

        consensus.smsg_fee_period = 5040;
        consensus.smsg_fee_funding_tx_per_k = 200000;
        consensus.smsg_fee_msg_per_day_per_k = 50000;
        consensus.smsg_fee_max_delta_percent = 43;
        consensus.smsg_min_difficulty = 0x1effffff;
        consensus.smsg_difficulty_max_delta = 0xffff;

        consensus.powLimit = uint256S("000000000005ffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.enforce_BIP94 = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Deployment of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = 1619222400; // April 24th, 2021
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = 1628640000; // August 11th, 2021
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000000002828c6d82d9552c531");
        consensus.defaultAssumeValid = uint256S("0x71693391c9e109328919e7641128a935734701d4e055a6c10aaa3179e6296965"); // 1485020

        consensus.nMinRCTOutputDepth = 12;

        pchMessageStart[0] = 0x08;
        pchMessageStart[1] = 0x11;
        pchMessageStart[2] = 0x05;
        pchMessageStart[3] = 0x0b;
        nDefaultPort = 51938;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 2;
        m_assumed_chain_state_size = 1;

        nBIP44ID = (int)WithHardenedBit(1);
        nModifierInterval = 10 * 60;    // 10 minutes
        nStakeMinConfirmations = 225;   // 225 * 2 minutes
        nTargetSpacing = 120;           // 2 minutes
        nTargetTimespan = 24 * 60;      // 24 mins

        particl::AddImportHashesTest(vImportedCoinbaseTxns);
        SetLastImportHeight();

        genesis = particl::CreateGenesisBlockTestNet(1502309248, 5924, 0x1f00ffff);
        consensus.hashGenesisBlock = genesis.GetHash();

        assert(consensus.hashGenesisBlock == uint256S("0x0000594ada5310b367443ee0afd4fa3d0bbd5850ea4e33cdc7d6a904a7ec7c90"));
        assert(genesis.hashMerkleRoot == uint256S("0x2c7f4d88345994e3849502061f6303d9666172e4dff3641d3472a72908eec002"));
        assert(genesis.hashWitnessMerkleRoot == uint256S("0xf9e2235c9531d5a19263ece36e82c4d5b71910d73cd0b677b81c5e50d17b6cda"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("testnet-seed.particl.io");
        vSeeds.emplace_back("dnsseed-testnet.particl.io");
        vSeeds.emplace_back("dnsseed-testnet.tecnovert.net");

        vTreasuryFundSettings.push_back(std::make_pair(0, particl::TreasuryFundSettings("rTvv9vsbu269mjYYEecPYinDG8Bt7D86qD", 10, 60)));

        base58Prefixes[PUBKEY_ADDRESS]     = {0x76}; // p
        base58Prefixes[SCRIPT_ADDRESS]     = {0x7a};
        base58Prefixes[PUBKEY_ADDRESS_256] = {0x77};
        base58Prefixes[SCRIPT_ADDRESS_256] = {0x7b};
        base58Prefixes[SECRET_KEY]         = {0x2e};
        base58Prefixes[EXT_PUBLIC_KEY]     = {0xe1, 0x42, 0x78, 0x00}; // ppar
        base58Prefixes[EXT_SECRET_KEY]     = {0x04, 0x88, 0x94, 0x78}; // xpar
        base58Prefixes[STEALTH_ADDRESS]    = {0x15}; // T
        base58Prefixes[EXT_KEY_HASH]       = {0x89}; // x
        base58Prefixes[EXT_ACC_HASH]       = {0x53}; // a
        base58Prefixes[EXT_PUBLIC_KEY_BTC] = {0x04, 0x35, 0x87, 0xCF}; // tpub
        base58Prefixes[EXT_SECRET_KEY_BTC] = {0x04, 0x35, 0x83, 0x94}; // tprv

        bech32Prefixes[PUBKEY_ADDRESS].assign       ("tph",(const char*)"tph"+3);
        bech32Prefixes[SCRIPT_ADDRESS].assign       ("tpr",(const char*)"tpr"+3);
        bech32Prefixes[PUBKEY_ADDRESS_256].assign   ("tpl",(const char*)"tpl"+3);
        bech32Prefixes[SCRIPT_ADDRESS_256].assign   ("tpj",(const char*)"tpj"+3);
        bech32Prefixes[SECRET_KEY].assign           ("tpx",(const char*)"tpx"+3);
        bech32Prefixes[EXT_PUBLIC_KEY].assign       ("tpep",(const char*)"tpep"+4);
        bech32Prefixes[EXT_SECRET_KEY].assign       ("tpex",(const char*)"tpex"+4);
        bech32Prefixes[STEALTH_ADDRESS].assign      ("tps",(const char*)"tps"+3);
        bech32Prefixes[EXT_KEY_HASH].assign         ("tpek",(const char*)"tpek"+4);
        bech32Prefixes[EXT_ACC_HASH].assign         ("tpea",(const char*)"tpea"+4);
        bech32Prefixes[STAKE_ONLY_PKADDR].assign    ("tpcs",(const char*)"tpcs"+4);

        bech32_hrp = "tpw";

        vFixedSeeds = std::vector<uint8_t>(std::begin(chainparams_seed_test), std::end(chainparams_seed_test));

        fDefaultConsistencyChecks = false;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                {127620, uint256S("0xe5ab909fc029b253bad300ccf859eb509e03897e7853e8bfdde2710dbf248dd1")},
                {210920, uint256S("0x5534f546c3b5a264ca034703b9694fabf36d749d66e0659eef5f0734479b9802")},
                {312860, uint256S("0xaba2e3b2dcf1970b53b67c869325c5eefd3a107e62518fa4640ddcfadf88760d")},
                {428386, uint256S("0x08bbc92c831b864c809b575901e37aaa9aa2b2e38212594aedf2712a87267da9")},
                {534422, uint256S("0xbf0ae4652ff8d2b2479cf828e2e4ec408cf29223c2ec8a96485b1bf424e096c6")},
                {728858, uint256S("0xd71157e5a929a2aba06b23566932ffaba05d1a063b2ab71d2807b8e2efcf765c")},
                {808059, uint256S("0x89de981a2cca262ae52ff5e69a0915c9083fb7cd4aba44e39f83c12a6b6602a9")},
                {909640, uint256S("0xe2e1880d525c93e24ca2d0d494fe78624ad28c4ce778f987504582b7404bcb71")},
                {1010348, uint256S("0x12e6a081d1874b3dfff99e120b8e22599e15730c23c88805740c507c11c91809")},
                {1029738, uint256S("0x62ca4edbeb88e371d36052f7bbb52a598b1ac13d9269b5205ba7ed228d8b742a")},
                {1044185, uint256S("0xc8b34db63fb6ec97b557a969065e56167030c36c12b7c988561736ad3a5488c0")},
                {1086076, uint256S("0x1e74a808cc907a48cdaf6f2cf5488a54d64a186fa7d65f5f717b11a8dc37d55e")},
                {1162920, uint256S("0x01bbfe87e90af03e6daa171611fe342403a7ebb7f4703342f34fdeb613b3ecbc")},
                {1230000, uint256S("0xab3724205826d2e38f0a12e1938d4825e3ba6f983ee74a08f2af33918bb53122")},
                {1339440, uint256S("0x894e4e80612341dca2e90f259fe7b006c982175833c4229c3d1782b37176a727")},
                {1430095, uint256S("0x8178aa5369953e3fd3c9f0e23db18c631909c7037eb9f5a151ef6b95b2396722")},
                {1558245, uint256S("0x71693391c9e109328919e7641128a935734701d4e055a6c10aaa3179e6296965")},
            }
        };

        m_assumeutxo_data = {
            {
                .height = 2'500'000,
                .hash_serialized = AssumeutxoHash{uint256{"f841584909f68e47897952345234e37fcd9128cd818f41ee6c3ca68db8071be7"}},
                .m_chain_tx_count = 66484552,
                .blockhash = consteval_ctor(uint256{"0000000000000093bcb68c03a9a168ae252572d348a2eaeba2cdf9231d73206f"}),
            }
        };

        chainTxData = ChainTxData{
            // Data from rpc: getchaintxstats 4096 71693391c9e109328919e7641128a935734701d4e055a6c10aaa3179e6296965
            .nTime    = 1701109632,
            .nTxCount = 1550896,
            .dTxRate  = 0.006
        };
    }
};

/**
 * Signet: test network with an additional consensus parameter (see BIP325).
 */
class SigNetParams : public CChainParams {
public:
    explicit SigNetParams(const SigNetOptions& options)
    {
        std::vector<uint8_t> bin;
        vFixedSeeds.clear();
        vSeeds.clear();

        if (!options.challenge) {
            bin = "512103ad5e0edad18cb1f0fc0d28a3d4f1f3e445640337489abb10404f2d1e086be430210359ef5021964fe22d6f8e05b2463c9540ce96883fe3b278760f048f5189f2e6c452ae"_hex_v_u8;
            vFixedSeeds = std::vector<uint8_t>(std::begin(chainparams_seed_signet), std::end(chainparams_seed_signet));
            vSeeds.emplace_back("seed.signet.bitcoin.sprovoost.nl.");
            vSeeds.emplace_back("seed.signet.achownodes.xyz."); // Ava Chow, only supports x1, x5, x9, x49, x809, x849, xd, x400, x404, x408, x448, xc08, xc48, x40c

            consensus.nMinimumChainWork = uint256{"000000000000000000000000000000000000000000000000000002b517f3d1a1"};
            consensus.defaultAssumeValid = uint256{"000000895a110f46e59eb82bbc5bfb67fa314656009c295509c21b4999f5180a"}; // 237722
            m_assumed_blockchain_size = 9;
            m_assumed_chain_state_size = 1;
            chainTxData = ChainTxData{
                // Data from RPC: getchaintxstats 4096 000000895a110f46e59eb82bbc5bfb67fa314656009c295509c21b4999f5180a
                .nTime    = 1741019645,
                .tx_count = 16540736,
                .dTxRate  = 1.064918879911595,
            };
        } else {
            bin = *options.challenge;
            consensus.nMinimumChainWork = uint256{};
            consensus.defaultAssumeValid = uint256{};
            m_assumed_blockchain_size = 0;
            m_assumed_chain_state_size = 0;
            chainTxData = ChainTxData{
                0,
                0,
                0,
            };
            LogPrintf("Signet with challenge %s\n", HexStr(bin));
        }

        if (options.seeds) {
            vSeeds = *options.seeds;
        }

        m_chain_type = ChainType::SIGNET;
        consensus.signet_blocks = true;
        consensus.signet_challenge.assign(bin.begin(), bin.end());
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256{};
        consensus.BIP65Height = 1;
        consensus.BIP66Height = 1;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 1;
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.enforce_BIP94 = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1815; // 90% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256{"00000377ae000000000000000000000000000000000000000000000000000000"};
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Activation of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // message start is defined as the first 4 bytes of the sha256d of the block script
        HashWriter h{};
        h << consensus.signet_challenge;
        uint256 hash = h.GetHash();
        std::copy_n(hash.begin(), 4, pchMessageStart.begin());

        nDefaultPort = 38333;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1598918400, 52613770, 0x1e0377ae, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256{"00000008819873e925422c1ff0f99f7cc9bbb232af63a077a480a3633bee1ef6"});
        assert(genesis.hashMerkleRoot == uint256{"4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"});

        m_assumeutxo_data = {
            {
                .height = 160'000,
                .hash_serialized = AssumeutxoHash{uint256{"fe0a44309b74d6b5883d246cb419c6221bcccf0b308c9b59b7d70783dbdf928a"}},
                .m_chain_tx_count = 2289496,
                .blockhash = consteval_ctor(uint256{"0000003ca3c99aff040f2563c2ad8f8ec88bd0fd6b8f0895cfaf1ef90353a62c"}),
            }
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tb";

        fDefaultConsistencyChecks = false;
        m_is_mockable_chain = false;
    }
};

/**
 * Regression test: intended for private networks only. Has minimal difficulty to ensure that
 * blocks can be found instantly.
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const RegTestOptions& opts)
    {
        m_chain_type = ChainType::REGTEST;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 150;
        consensus.BIP34Height = 1; // Always active unless overridden
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1;  // Always active unless overridden
        consensus.BIP66Height = 1;  // Always active unless overridden
        consensus.CSVHeight = 1;    // Always active unless overridden
        consensus.SegwitHeight = 0; // Always active unless overridden
        consensus.MinBIP9WarningHeight = 0;

        consensus.OpIsCoinstakeTime = 0;
        consensus.fAllowOpIsCoinstakeWithP2PKH = false;
        consensus.nPaidSmsgTime = 0;
        consensus.smsg_fee_time = 0;
        consensus.bulletproof_time = 0;
        consensus.rct_time = 0;
        consensus.smsg_difficulty_time = 0;

        consensus.clamp_tx_version_time = 0;

        consensus.smsg_fee_period = 50;
        consensus.smsg_fee_funding_tx_per_k = 200000;
        consensus.smsg_fee_msg_per_day_per_k = 50000;
        consensus.smsg_fee_max_delta_percent = 4300;
        consensus.smsg_min_difficulty = 0x1f0fffff;
        consensus.smsg_difficulty_max_delta = 0xffff;
        consensus.m_taproot_time = 0;

        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.enforce_BIP94 = opts.enforce_bip94;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        consensus.nMinimumChainWork = uint256{};
        consensus.defaultAssumeValid = uint256{};

        consensus.nMinRCTOutputDepth = 2;

        pchMessageStart[0] = 0x09;
        pchMessageStart[1] = 0x12;
        pchMessageStart[2] = 0x06;
        pchMessageStart[3] = 0x0c;
        nDefaultPort = 11938;
        nBIP44ID = (int)WithHardenedBit(1);


        nModifierInterval = 2 * 60;     // 2 minutes
        nStakeMinConfirmations = 12;
        nTargetSpacing = 5;             // 5 seconds
        nTargetTimespan = 16 * 60;      // 16 mins
        nStakeTimestampMask = 0;

        SetLastImportHeight();

        nPruneAfterHeight = opts.fastprune ? 100 : 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        for (const auto& [dep, height] : opts.activation_heights) {
            switch (dep) {
            case Consensus::BuriedDeployment::DEPLOYMENT_SEGWIT:
                consensus.SegwitHeight = int{height};
                break;
            case Consensus::BuriedDeployment::DEPLOYMENT_HEIGHTINCB:
                consensus.BIP34Height = int{height};
                break;
            case Consensus::BuriedDeployment::DEPLOYMENT_DERSIG:
                consensus.BIP66Height = int{height};
                break;
            case Consensus::BuriedDeployment::DEPLOYMENT_CLTV:
                consensus.BIP65Height = int{height};
                break;
            case Consensus::BuriedDeployment::DEPLOYMENT_CSV:
                consensus.CSVHeight = int{height};
                break;
            }
        }

        for (const auto& [deployment_pos, version_bits_params] : opts.version_bits_parameters) {
            consensus.vDeployments[deployment_pos].nStartTime = version_bits_params.start_time;
            consensus.vDeployments[deployment_pos].nTimeout = version_bits_params.timeout;
            consensus.vDeployments[deployment_pos].min_activation_height = version_bits_params.min_activation_height;
        }

        genesis = particl::CreateGenesisBlockRegTest(1487714923, 0, 0x207fffff);

        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x6cd174536c0ada5bfa3b8fde16b98ae508fff6586f2ee24cf866867098f25907"));
        assert(genesis.hashMerkleRoot == uint256S("0xf89653c7208af2c76a3070d436229fb782acbd065bd5810307995b9982423ce7"));
        assert(genesis.hashWitnessMerkleRoot == uint256S("0x36b66a1aff91f34ab794da710d007777ef5e612a320e1979ac96e5f292399639"));


        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();
        vSeeds.emplace_back("dummySeed.invalid.");

        fDefaultConsistencyChecks = true;
        m_is_mockable_chain = true;

        checkpointData = {
            {
                {0, uint256{"0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"}},
            }
        };

        m_assumeutxo_data = {
            {   // For use by unit tests
                .height = 110,
                .hash_serialized = AssumeutxoHash{uint256S("0x3e8de6bf3e4420c18612dd8b59be64bb155996fa7d08b450c6279fe83c09fa2c")},
                .nChainTx = 111,
                .blockhash = uint256S("0x696e92821f65549c7ee134edceeeeaaa4105647a3c4fd9f298c0aec0ab50425c")
            },
            {
                // For use by test/functional/feature_assumeutxo.py
                .height = 299,
                .hash_serialized = AssumeutxoHash{uint256S("0x2a8a4471ac1888c47365d15bde12b93e700e5aea3c9736a2a1527bf40afbefd7")},
                .nChainTx = 334,
                .blockhash = uint256S("0x3bb7ce5eba0be48939b7a521ac1ba9316afee2c7bada3a0cca24188e6d7d96c0")
            },
        };

        base58Prefixes[PUBKEY_ADDRESS]     = {0x76}; // p
        base58Prefixes[SCRIPT_ADDRESS]     = {0x7a};
        base58Prefixes[PUBKEY_ADDRESS_256] = {0x77};
        base58Prefixes[SCRIPT_ADDRESS_256] = {0x7b};
        base58Prefixes[SECRET_KEY]         = {0x2e};
        base58Prefixes[EXT_PUBLIC_KEY]     = {0xe1, 0x42, 0x78, 0x00}; // ppar
        base58Prefixes[EXT_SECRET_KEY]     = {0x04, 0x88, 0x94, 0x78}; // xpar
        base58Prefixes[STEALTH_ADDRESS]    = {0x15}; // T
        base58Prefixes[EXT_KEY_HASH]       = {0x89}; // x
        base58Prefixes[EXT_ACC_HASH]       = {0x53}; // a
        base58Prefixes[EXT_PUBLIC_KEY_BTC] = {0x04, 0x35, 0x87, 0xCF}; // tpub
        base58Prefixes[EXT_SECRET_KEY_BTC] = {0x04, 0x35, 0x83, 0x94}; // tprv

        bech32Prefixes[PUBKEY_ADDRESS].assign       ("tph",(const char*)"tph"+3);
        bech32Prefixes[SCRIPT_ADDRESS].assign       ("tpr",(const char*)"tpr"+3);
        bech32Prefixes[PUBKEY_ADDRESS_256].assign   ("tpl",(const char*)"tpl"+3);
        bech32Prefixes[SCRIPT_ADDRESS_256].assign   ("tpj",(const char*)"tpj"+3);
        bech32Prefixes[SECRET_KEY].assign           ("tpx",(const char*)"tpx"+3);
        bech32Prefixes[EXT_PUBLIC_KEY].assign       ("tpep",(const char*)"tpep"+4);
        bech32Prefixes[EXT_SECRET_KEY].assign       ("tpex",(const char*)"tpex"+4);
        bech32Prefixes[STEALTH_ADDRESS].assign      ("tps",(const char*)"tps"+3);
        bech32Prefixes[EXT_KEY_HASH].assign         ("tpek",(const char*)"tpek"+4);
        bech32Prefixes[EXT_ACC_HASH].assign         ("tpea",(const char*)"tpea"+4);
        bech32Prefixes[STAKE_ONLY_PKADDR].assign    ("tpcs",(const char*)"tpcs"+4);

        bech32_hrp = "rtpw";

        chainTxData = ChainTxData{
            0,
            0,
            0
        };
    }
};

std::unique_ptr<CChainParams> CChainParams::SigNet(const SigNetOptions& options)
{
    return std::make_unique<SigNetParams>(options);
}

std::unique_ptr<CChainParams> CChainParams::RegTest(const RegTestOptions& options)
{
    return std::make_unique<CRegTestParams>(options);
}

std::unique_ptr<CChainParams> CChainParams::Main()
{
    return std::make_unique<CMainParams>();
}

std::unique_ptr<CChainParams> CChainParams::TestNet()
{
    return std::make_unique<CTestNetParams>();
}

std::unique_ptr<const CChainParams> CChainParams::TestNet4()
{
    return std::make_unique<const CTestNet4Params>();
}

std::vector<int> CChainParams::GetAvailableSnapshotHeights() const
{
    std::vector<int> heights;
    heights.reserve(m_assumeutxo_data.size());

    for (const auto& data : m_assumeutxo_data) {
        heights.emplace_back(data.height);
    }
    return heights;
}

std::optional<ChainType> GetNetworkForMagic(const MessageStartChars& message)
{
    const auto mainnet_msg = CChainParams::Main()->MessageStart();
    const auto testnet_msg = CChainParams::TestNet()->MessageStart();
    const auto testnet4_msg = CChainParams::TestNet4()->MessageStart();
    const auto regtest_msg = CChainParams::RegTest({})->MessageStart();
    const auto signet_msg = CChainParams::SigNet({})->MessageStart();

    if (std::ranges::equal(message, mainnet_msg)) {
        return ChainType::MAIN;
    } else if (std::ranges::equal(message, testnet_msg)) {
        return ChainType::TESTNET;
    } else if (std::ranges::equal(message, testnet4_msg)) {
        return ChainType::TESTNET4;
    } else if (std::ranges::equal(message, regtest_msg)) {
        return ChainType::REGTEST;
    } else if (std::ranges::equal(message, signet_msg)) {
        return ChainType::SIGNET;
    }
    return std::nullopt;
}
