#ifndef _U5TK_LOGIC_H_
#define _U5TK_LOGIC_H_

#include <stdint.h>
#include <vector>
#include <bitset>
#include <chrono>

class U5TKLogic final
{
public:
    static const size_t ParticipantCount = 4U;
    void sitDown(unsigned seat);
    void standUp(unsigned seat);

    U5TKLogic();

    // 每张牌的值由如下4个信息组成，每8bit存放一个信息
    // +-----24+-----16+------8+------0+
    // | level | power |  suit |  rank |
    // +-------+-------+-------+-------+
    typedef uint32_t CARD;
    static const CARD JOKER_BLACK = 0x050E050EU;  // 小王
    static const CARD JOKER_RED = 0x050F050FU;  // 大王

#define GET_RANK(ca)    (((ca) >>  0U) & 0x0000000FU)
#define GET_SUIT(ca)    (((ca) >>  8U) & 0x0000000FU)
#define GET_POWER(ca)   (((ca) >> 16U) & 0x0000000FU)
#define GET_LEVEL(ca)   (((ca) >> 24U) & 0x0000000FU)

#define MAKE_CARD(level, power, suit, rank) (((level) << 24U) | ((power) << 16U) | ((suit) << 8U) | (rank))

    static inline const char *cardDebugString(CARD ca) {
        const char *table[5][11] = {
            { "D5", "D6", "D7", "D8", "D9", "DT", "DJ", "DQ", "DK", "DA", "D2" },
            { "C5", "C6", "C7", "C8", "C9", "CT", "CJ", "CQ", "CK", "CA", "C2" },
            { "H5", "H6", "H7", "H8", "H9", "HT", "HJ", "HQ", "HK", "HA", "H2" },
            { "S5", "S6", "S7", "S8", "S9", "ST", "SJ", "SQ", "SK", "SA", "S2" },
            { "JB", "JR" }
        };
        unsigned suit = GET_SUIT(ca), rank = GET_RANK(ca);
        return suit < 5 ? table[suit - 1][rank - 5] : table[4][rank - 14];
    }

    static inline bool isScore(CARD ca) { unsigned rank = GET_RANK(ca); return rank == 5 || rank == 10 || rank == 13; }

    enum class ErrorType : int {
        SUCCESS = 0,  // 成功
        UNKNOWN_ERROR,  // 未知错误
        STATE_ERROR,  // 状态错误
        NOT_YOUR_TURN,  // 没轮到你
        ILLEGAL_CARDS,  // 手上没对应的牌
        ILLEGAL_POS,  // 非法的座位
        UNCHARTERED_SHOW_COUNT,  // 不合规则的叫主用牌数量
        SHOW_CARDS_SHOULD_CONTAIN_JOKER,  // 叫主的牌必须包含王
        SHOW_CARDS_SHOULD_HAVE_GRAGE,  // 叫主的牌必须包含级牌
        SHOW_CARDS_SHOULD_CONTAIN_2,  // 叫主的牌必须包含2
        REBEL_NEED_TWO_GRADE,  // 反主的牌必须包含两张级牌
        REBEL_SHOULD_GREATER_THAN_ORIGIN,  // 反主的花色必须大于原花色
        NO_ASK_DEFEAT,  // 庄家没有发起投降询问
        DEFEAT_HAS_ASKED,  // 已经问过了
        UNCHARTERED_EXCHANGE_COUNT,  // 埋牌数量不对
        EXCHANGED_SHOULDNOT_CONTAIN_SCORES,  // 不能埋分
        UNCHARTERED_BRING_COUNT,  // 出牌数量不正确
        UNCHARTERED_BRING_TYPE,  // 出牌类型不正确
        FOLLOW_BRING_SHOULD_MATCH_SUIT,  // 出牌花色不正确
        FOLLOW_BRING_SHOULD_MATCH_PAIR_COUNT  // 出牌对子数不正确
    };

    bool checkSendEnd();
    ErrorType doShowTrump(int pos, const std::vector<CARD> &cards);
    ErrorType passShow(int pos);
    ErrorType askDefeat(int pos);
    ErrorType admitDefeat(int pos, bool agree);
    ErrorType setReady(int pos);
    ErrorType doExchange(int pos, const std::vector<CARD> &cards);
    ErrorType doBring(int pos, const std::vector<CARD> &cards);
    void forcedEnd();

private:
    void init();
    void setup();
    void shuffleCards();
    void _updateCards();
    void _sortCards();
    ErrorType _checkShowTrump(int pos, const std::vector<CARD> &cards) const;
    bool _checkNoTrump() const;
    bool afterSend();
    void sendUnderCards();

public:
    enum class State : int {
        WAITING,
        SENDING,
        SHOWING,
        EXCHANGING,
        BRINGING
    };

    inline State getState() const { return _state; }
    inline bool isGrabbing() const { return _isGrabbing; }
    inline unsigned getGrade() const { return _grade; }
    inline unsigned getTrump() const { return _trump; }
    inline unsigned getGrade2() const { return _grade2; }
    inline int getBankerPos() const { return _bankerPos; }
    inline int getShownPos() const { return _shownPos; }
    inline int getTurnPos() const { return _turnPos; }
    inline unsigned getScores() const { return _scores; }
    inline unsigned getCycles() const { return _cycles; }
    inline unsigned getRounds() const { return _rounds; }

private:
    State _state = State::WAITING;

    bool _isGrabbing;  // 争庄与否
    unsigned _grade;  // 级别
    unsigned _trump;  // 主花色 0:无，1234分别代表方板-梅花-红桃-黑桃
    unsigned _grade2;  // 闲家级别
    int _bankerPos;  // 庄家
    int _shownPos;  // 叫主玩家
    int _turnPos;  // 轮到出牌的玩家
    int _leaderPos;  // 一轮中最先出牌的玩家
    unsigned _scores;  // 闲家得分
    //unsigned _trumpTable[4][10];  // 各家机动主表
    unsigned _cycles = 0;  // 第几局
    unsigned _rounds = 0;  // 第几回合

private:
    enum class DefeatState : int {
        NOT_ASK,
        ASKING,
        ASKED
    };
    DefeatState _defeatState = DefeatState::NOT_ASK;
    std::bitset<4> _readyState;  // 准备状态
    std::bitset<4> _passFlag;  // 反主过的标志

public:
    CARD _calculateCard(CARD ca);

    inline const std::vector<CARD> &getHandCards(int pos) const { return _handCards[pos]; }
    inline const std::vector<CARD> &getShownCards() const { return _shownCards; }
    inline const std::vector<CARD> &getUnderCards() const { return _underCards; }
    inline const std::vector<CARD> &getBringCards(int pos) const { return _bringCards[pos]; }
    inline const std::vector<CARD> &getRecordCards(int pos) const { return _recordCards[pos]; }
    inline const std::vector<CARD> &getScoreCards() const { return _scoreCards; }

private:
    std::vector<CARD> _handCards[4];  // 手上的牌
    std::vector<CARD> _shownCards;  // 叫主的牌
    std::vector<CARD> _underCards;  // 底牌
    std::vector<CARD> _bringCards[4];  // 当前一轮出的牌
    std::vector<CARD> _recordCards[4];  // 已出的牌
    std::vector<CARD> _scoreCards;  // 分牌

    std::chrono::system_clock::time_point _sendTime;

    // 当前出牌信息，每8bit存放一个信息
    // +-----24+-----16+------8+------0+
    // | count | level |  type | power |
    // +-------+-------+-------+-------+
    typedef uint32_t BRING_INFO;
    BRING_INFO _bringInfo[4];

#define BRING_INFO_GET_COUNT(info)      (((info) >> 24U) & 0x0000000FU)
#define BRING_INFO_GET_LEVEL(info)      (((info) >> 16U) & 0x0000000FU)
#define BRING_INFO_GET_TYPE(info)       (((info) >>  8U) & 0x0000000FU)
#define BRING_INFO_GET_POWER(info)      (((info) >>  0U) & 0x0000000FU)

#define MAKE_BRING_INFO(count, level, type, power) (((count) << 24U) | ((level) << 16U) | ((type) << 8U) | (power))
    static const uint32_t BRING_TYPE_NONE = 0;
    static const uint32_t BRING_TYPE_SINGLE = 1;
    static const uint32_t BRING_TYPE_PAIR = 2;
    static const uint32_t BRING_TYPE_TRACTOR = 3;

    BRING_INFO _analyze(const std::vector<CARD> &cards);
    ErrorType _doLeaderBring(int pos, const std::vector<CARD> &cards);

    static unsigned _countSingles(const std::vector<CARD> &cards, unsigned level);
    static unsigned _countPairs(const std::vector<CARD> &cards, unsigned level);
    ErrorType _doFollowBring(int pos, const std::vector<CARD> &cards);

    void _nextBring();
    static bool _greatThan(BRING_INFO info1, BRING_INFO info2);
    void _gameOver();

//    // 当前出牌信息，每8bit存放一个信息
//    // +-----24+-----16+------8+------0+
//    // | scores| banker| grade | grade2|
//    // +-------+-------+-------+-------+
//    typedef uint32_t RESULT_STATISTICS;
//
//#define MAKE_RESULT_STATISTICS(scores, banker, grade, grade2) (((scores) << 24U) | ((banker) << 16U) | ((grade) << 8U) | (grade2))
//    static const uint32_t RESULT_NO_SHOWN = 0;
//    static const uint32_t RESULT_PURE_EMPTY = 1;
//    static const uint32_t RESULT_HAIR_EMPTY = 2;
//    static const uint32_t RESULT_PASS = 3;
//    static const uint32_t RESULT_FAIL = 4;
//    static const uint32_t RESULT_CAPSIZED = 5;
//    static const uint32_t RESULT_BIG_CAPSIZED = 6;
//
//    std::vector<RESULT_STATISTICS> _statistics;

};

#endif
