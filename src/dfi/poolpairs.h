// Copyright (c) 2020 The DeFi Foundation
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef DEFI_DFI_POOLPAIRS_H
#define DEFI_DFI_POOLPAIRS_H

#include <flushablestorage.h>

#include <amount.h>
#include <arith_uint256.h>
#include <chainparams.h>
#include <dfi/balances.h>
#include <dfi/res.h>
#include <script/script.h>
#include <serialize.h>
#include <uint256.h>

struct CFeeDir;

struct ByPairKey {
    DCT_ID idTokenA;
    DCT_ID idTokenB;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(idTokenA);
        READWRITE(idTokenB);
    }
};

struct PoolPrice {
    int64_t integer;
    int64_t fraction;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(integer);
        READWRITE(fraction);
    }

    bool operator!=(const PoolPrice &rhs) const { return integer != rhs.integer || fraction != rhs.fraction; }

    static constexpr PoolPrice getMaxValid() { return {MAX_MONEY / COIN, MAX_MONEY % COIN}; }

    bool isAboveValid() const {
        const auto maxPrice = PoolPrice::getMaxValid();
        return ((integer > maxPrice.integer) || (integer == maxPrice.integer && fraction >= maxPrice.fraction));
    }
};

struct CPoolSwapMessage {
    CScript from, to;
    DCT_ID idTokenFrom, idTokenTo;
    CAmount amountFrom;
    PoolPrice maxPrice;

    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(from);
        READWRITE(idTokenFrom);
        READWRITE(amountFrom);
        READWRITE(to);
        READWRITE(idTokenTo);
        READWRITE(maxPrice);
    }
};

struct CPoolSwapMessageV2 {
    CPoolSwapMessage swapInfo;
    std::vector<DCT_ID> poolIDs;

    ADD_SERIALIZE_METHODS

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(swapInfo);
        READWRITE(poolIDs);
    }
};

struct CPoolPairMessageBase {
    DCT_ID idTokenA, idTokenB;
    CAmount commission;  // comission %% for traders
    CScript ownerAddress;
    bool status = true;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(idTokenA);
        READWRITE(idTokenB);
        READWRITE(commission);
        READWRITE(ownerAddress);
        READWRITE(status);
    }
};

struct CCreatePoolPairMessage : public CPoolPairMessageBase {
    std::string pairSymbol;
    CBalances rewards;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITEAS(CPoolPairMessageBase, *this);
        READWRITE(pairSymbol);
        if (!s.empty()) {
            READWRITE(rewards);
        }
    }
};

struct CUpdatePoolPairMessage {
    DCT_ID poolId;
    bool status;
    CAmount commission;
    CScript ownerAddress;
    CBalances rewards;
    std::string pairSymbol;
    std::string pairName;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(poolId.v);
        READWRITE(status);
        READWRITE(commission);
        READWRITE(ownerAddress);
        if (!s.empty()) {
            READWRITE(rewards);
        }
        if (!s.empty()) {
            READWRITE(pairSymbol);
            READWRITE(pairName);
        }
    }
};

class CPoolPair : public CPoolPairMessageBase {
public:
    static const CAmount MINIMUM_LIQUIDITY = 1000;
    static const CAmount SLOPE_SWAP_RATE = 1000;
    static const uint32_t PRECISION = (uint32_t)COIN;  // or just PRECISION_BITS for "<<" and ">>"

    // temporary values, not serialized
    CAmount reserveA = 0;
    CAmount reserveB = 0;
    CAmount totalLiquidity = 0;
    CAmount blockCommissionA = 0;
    CAmount blockCommissionB = 0;

    CAmount rewardPct = 0;  // pool yield farming reward %%
    CAmount rewardLoanPct = 0;
    bool swapEvent = false;

    // serialized
    CBalances rewards;
    uint256 creationTx;
    uint32_t creationHeight = -1;

    // 'amountA' && 'amountB' should be normalized (correspond) to actual 'tokenA' and 'tokenB' ids in the pair!!
    // otherwise, 'AddLiquidity' should be () external to 'CPairPool' (i.e. CPoolPairView::AddLiquidity(TAmount a,b etc)
    // with internal lookup of pool by TAmount a,b)
    Res AddLiquidity(CAmount amountA,
                     CAmount amountB,
                     std::function<Res(CAmount)> onMint,
                     bool slippageProtection = false);
    Res RemoveLiquidity(CAmount liqAmount, std::function<Res(CAmount, CAmount)> onReclaim);

    Res Swap(CTokenAmount in,
             CAmount dexfeeInPct,
             const PoolPrice &maxPrice,
             const std::pair<CFeeDir, CFeeDir> &asymmetricFee,
             std::function<Res(const CTokenAmount &, const CTokenAmount &)> onTransfer,
             int height = INT_MAX);

private:
    CAmount slopeSwap(CAmount unswapped, CAmount &poolFrom, CAmount &poolTo, int height);

    inline void ioProofer()
        const {  // Maybe it's more reasonable to use unsigned everywhere, but for basic CAmount compatibility
        if (reserveA < 0 || reserveB < 0 || totalLiquidity < 0 || blockCommissionA < 0 || blockCommissionB < 0 ||
            rewardPct < 0 || commission < 0 || rewardLoanPct < 0) {
            throw std::ios_base::failure("negative pool's 'CAmounts'");
        }
    }

public:
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        if (!ser_action.ForRead()) {
            ioProofer();
        }

        READWRITEAS(CPoolPairMessageBase, *this);
        READWRITE(rewards);
        READWRITE(creationTx);
        READWRITE(creationHeight);

        if (ser_action.ForRead()) {
            ioProofer();
        }
    }
};

struct PoolShareKey {
    DCT_ID poolID;
    CScript owner;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(WrapBigEndian(poolID.v));
        READWRITE(owner);
    }
};

struct TotalRewardPerShareKey {
    uint32_t height;
    uint32_t poolID;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(WrapBigEndian(height));
        READWRITE(WrapBigEndian(poolID));
    }
};

struct TotalCommissionPerShareValue {
    uint32_t tokenA;
    uint32_t tokenB;
    arith_uint256 commissionA;
    arith_uint256 commissionB;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(tokenA);
        READWRITE(tokenB);
        READWRITE(commissionA);
        READWRITE(commissionB);
    }
};

struct LoanTokenAverageLiquidityKey {
    uint32_t sourceID;
    uint32_t destID;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(sourceID);
        READWRITE(destID);
    }

    bool operator<(const LoanTokenAverageLiquidityKey &other) const {
        if (sourceID == other.sourceID) {
            return destID < other.destID;
        }
        return sourceID < other.sourceID;
    }
};

struct LoanTokenLiquidityPerBlockKey {
    uint32_t height;
    uint32_t sourceID;
    uint32_t destID;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(WrapBigEndian(height));
        READWRITE(sourceID);
        READWRITE(destID);
    }
};

struct PoolHeightKey {
    DCT_ID poolID;
    uint32_t height;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(poolID);

        if (ser_action.ForRead()) {
            READWRITE(WrapBigEndian(height));
            height = ~height;
        } else {
            uint32_t height_ = ~height;
            READWRITE(WrapBigEndian(height_));
        }
    }
};

enum RewardType {
    Commission = 127,
    Rewards = 128,
    Coinbase = Rewards | 1,
    Pool = Rewards | 2,
    LoanTokenDEXReward = Rewards | 4,
};

std::string RewardToString(RewardType type);
std::string RewardTypeToString(RewardType type);

class CPoolPairView : public virtual CStorageView {
public:
    Res SetPoolPair(const DCT_ID &poolId, uint32_t height, const CPoolPair &pool);
    Res UpdatePoolPair(DCT_ID const &poolId,
                       uint32_t height,
                       bool status,
                       const CAmount &commission,
                       const CScript &ownerAddress,
                       const CBalances &rewards);

    std::optional<CPoolPair> GetPoolPair(const DCT_ID &poolId) const;
    std::optional<std::pair<DCT_ID, CPoolPair> > GetPoolPair(DCT_ID const &tokenA, DCT_ID const &tokenB) const;

    void ForEachPoolId(std::function<bool(DCT_ID const &)> callback, DCT_ID const &start = DCT_ID{0});
    void ForEachPoolPair(std::function<bool(DCT_ID const &, CPoolPair)> callback, DCT_ID const &start = DCT_ID{0});
    void ForEachPoolShare(std::function<bool(DCT_ID const &, const CScript &, uint32_t)> callback,
                          const PoolShareKey &startKey = {});

    Res SetShare(DCT_ID const &poolId, const CScript &provider, uint32_t height);
    Res DelShare(DCT_ID const &poolId, const CScript &provider);

    std::optional<uint32_t> GetShare(DCT_ID const &poolId, const CScript &provider);

    void CalculatePoolRewards(DCT_ID const &poolId,
                              std::function<CAmount()> onLiquidity,
                              uint32_t begin,
                              uint32_t end,
                              std::function<void(RewardType, CTokenAmount, uint32_t)> onReward);

    void CalculateStaticPoolRewards(std::function<CAmount()> onLiquidity,
                                    std::function<void(RewardType, CTokenAmount, uint32_t)> onReward,
                                    const uint32_t poolID,
                                    const uint32_t beginHeight,
                                    const uint32_t endHeight);

    Res SetLoanDailyReward(const uint32_t height, const CAmount reward);
    Res SetDailyReward(uint32_t height, CAmount reward);
    Res SetRewardPct(DCT_ID const &poolId, uint32_t height, CAmount rewardPct);
    Res SetRewardLoanPct(DCT_ID const &poolId, uint32_t height, CAmount rewardLoanPct);
    bool HasPoolPair(DCT_ID const &poolId) const;

    Res SetDexFeePct(DCT_ID poolId, DCT_ID tokenId, CAmount feePct);
    Res EraseDexFeePct(DCT_ID poolId, DCT_ID tokenId);
    CAmount GetDexFeeInPct(DCT_ID poolId, DCT_ID tokenId) const;
    CAmount GetDexFeeOutPct(DCT_ID poolId, DCT_ID tokenId) const;

    std::pair<CAmount, CAmount> UpdatePoolRewards(
        std::function<CTokenAmount(const CScript &, DCT_ID)> onGetBalance,
        std::function<Res(const CScript &, const CScript &, CTokenAmount)> onTransfer,
        int nHeight = 0);

    bool SetLoanTokenLiquidityPerBlock(const LoanTokenLiquidityPerBlockKey &key, const CAmount liquidityPerBlock);
    bool EraseTokenLiquidityPerBlock(const LoanTokenLiquidityPerBlockKey &key);
    void ForEachTokenLiquidityPerBlock(
        std::function<bool(const LoanTokenLiquidityPerBlockKey &key, const CAmount liquidityPerBlock)> callback,
        const LoanTokenLiquidityPerBlockKey &start = LoanTokenLiquidityPerBlockKey{});

    bool SetLoanTokenAverageLiquidity(const LoanTokenAverageLiquidityKey &key, const uint64_t liquidity);
    std::optional<uint64_t> GetLoanTokenAverageLiquidity(const LoanTokenAverageLiquidityKey &key);
    bool EraseTokenAverageLiquidity(const LoanTokenAverageLiquidityKey key);
    void ForEachTokenAverageLiquidity(
        std::function<bool(const LoanTokenAverageLiquidityKey &key, const uint64_t liquidity)> callback,
        const LoanTokenAverageLiquidityKey start = LoanTokenAverageLiquidityKey{});

    bool SetTotalRewardPerShare(const TotalRewardPerShareKey &key, const arith_uint256 &totalReward);
    arith_uint256 GetTotalRewardPerShare(const TotalRewardPerShareKey &totalReward) const;
    bool SetTotalLoanRewardPerShare(const TotalRewardPerShareKey &key, const arith_uint256 &totalReward);
    arith_uint256 GetTotalLoanRewardPerShare(const TotalRewardPerShareKey &totalReward) const;
    bool SetTotalCustomRewardPerShare(const TotalRewardPerShareKey &key,
                                      const std::map<uint32_t, arith_uint256> &customRewards);
    std::map<uint32_t, arith_uint256> GetTotalCustomRewardPerShare(const TotalRewardPerShareKey &key) const;
    bool SetTotalCommissionPerShare(const TotalRewardPerShareKey &key,
                                    const TotalCommissionPerShareValue &totalCommission);
    TotalCommissionPerShareValue GetTotalCommissionPerShare(const TotalRewardPerShareKey &key) const;

    // tags
    struct ByID {
        static constexpr uint8_t prefix() { return 'i'; }
    };
    struct ByPair {
        static constexpr uint8_t prefix() { return 'j'; }
    };
    struct ByShare {
        static constexpr uint8_t prefix() { return 'k'; }
    };
    struct ByIDPair {
        static constexpr uint8_t prefix() { return 'C'; }
    };
    struct ByPoolSwap {
        static constexpr uint8_t prefix() { return 'P'; }
    };
    struct ByReserves {
        static constexpr uint8_t prefix() { return 'R'; }
    };
    struct ByRewardPct {
        static constexpr uint8_t prefix() { return 'Q'; }
    };
    struct ByPoolReward {
        static constexpr uint8_t prefix() { return 'I'; }
    };
    struct ByDailyReward {
        static constexpr uint8_t prefix() { return 'B'; }
    };
    struct ByCustomReward {
        static constexpr uint8_t prefix() { return 'A'; }
    };
    struct ByTotalLiquidity {
        static constexpr uint8_t prefix() { return 'f'; }
    };
    struct ByDailyLoanReward {
        static constexpr uint8_t prefix() { return 'q'; }
    };
    struct ByRewardLoanPct {
        static constexpr uint8_t prefix() { return 'U'; }
    };
    struct ByPoolLoanReward {
        static constexpr uint8_t prefix() { return 'W'; }
    };
    struct ByTokenDexFeePct {
        static constexpr uint8_t prefix() { return 'l'; }
    };
    struct ByLoanTokenLiquidityPerBlock {
        static constexpr uint8_t prefix() { return 'p'; }
    };
    struct ByLoanTokenLiquidityAverage {
        static constexpr uint8_t prefix() { return 0x27; }
    };
    struct ByTotalRewardPerShare {
        static constexpr uint8_t prefix() { return 0x28; }
    };

    struct ByTotalLoanRewardPerShare {
        static constexpr uint8_t prefix() { return 0x29; }
    };

    struct ByTotalCustomRewardPerShare {
        static constexpr uint8_t prefix() { return 0x2A; }
    };

    struct ByTotalCommissionPerShare {
        static constexpr uint8_t prefix() { return 0x7B; }
    };
};

struct CLiquidityMessage {
    CAccounts from;  // from -> balances
    CScript shareAddress;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(from);
        READWRITE(shareAddress);
    }
};

struct CRemoveLiquidityMessage {
    CScript from;
    CTokenAmount amount;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream &s, Operation ser_action) {
        READWRITE(from);
        READWRITE(amount);
    }
};

bool poolInFee(const bool forward, const std::pair<CFeeDir, CFeeDir> &asymmetricFee);
bool poolOutFee(const bool forward, const std::pair<CFeeDir, CFeeDir> &asymmetricFee);

#endif  // DEFI_DFI_POOLPAIRS_H
