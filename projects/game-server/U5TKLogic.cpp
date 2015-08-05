#include "U5TKLogic.h"

#include <string.h>
//#include <stdlib.h>
#include <random>
#include <algorithm>
#include <iterator>
#include <functional>

static bool _IsSubCardString(const std::vector<U5TKLogic::CARD> &ca, const std::vector<U5TKLogic::CARD> &src) {
    std::vector<U5TKLogic::CARD>::const_iterator srcFirst = src.begin(), srcLast = src.end();
    std::vector<U5TKLogic::CARD>::const_iterator dstFirst = ca.begin(), dstLast = ca.end();
    while (dstFirst < dstLast) {
        if (*dstFirst == *srcFirst) {
            if (++srcFirst == srcLast) {
                return true;
            }
        }
        ++dstFirst;
    }
    return false;
}

static bool _IsSubCardStringUnsorted(std::vector<U5TKLogic::CARD> &&ca, const std::vector<U5TKLogic::CARD> &src) {
    std::sort(ca.begin(), ca.end(), std::greater<U5TKLogic::CARD>());
    return _IsSubCardString(ca, src);
}

static void _DeleteSubCardString(std::vector<U5TKLogic::CARD> &ca, const std::vector<U5TKLogic::CARD> &src) {
    std::vector<U5TKLogic::CARD>::const_iterator srcFirst = src.begin(), srcLast = src.end();
    std::vector<U5TKLogic::CARD>::const_iterator inputIt = ca.begin(), itEnd = ca.end();
    std::vector<U5TKLogic::CARD>::iterator outputIt = ca.begin();
    for (; inputIt < itEnd; ++inputIt) {
        if (srcFirst < srcLast && *inputIt == *srcFirst) {
            ++srcFirst;
        } else {
            *outputIt++ = *inputIt;
        }
    }
    ca.erase(outputIt, ca.end());
}

U5TKLogic::U5TKLogic() {
    _handCards[0].reserve(29);
    _handCards[1].reserve(29);
    _handCards[2].reserve(29);
    _handCards[3].reserve(29);
    _shownCards.reserve(4);
    _underCards.reserve(8);
    _bringCards[0].reserve(21);
    _bringCards[1].reserve(21);
    _bringCards[2].reserve(21);
    _bringCards[3].reserve(21);
    _recordCards[0].reserve(21);
    _recordCards[1].reserve(21);
    _recordCards[2].reserve(21);
    _recordCards[3].reserve(21);
    _scoreCards.reserve(24);
    init();
}

U5TKLogic::CARD U5TKLogic::_calculateCard(CARD ca) {
    unsigned suit = GET_SUIT(ca);
    unsigned rank = GET_RANK(ca);
    if (suit == 5) {  // 大小王
        if (rank == 15) {
            return JOKER_RED;
        } else {  //if (rank == 14)
            return JOKER_BLACK;
        }
    }

    if (_trump == suit) {  // 主花色
        if (rank < _grade) {
            // 从5到级牌-1
            unsigned power = rank - 4;
            return MAKE_CARD(5, power, suit, rank);
        } else if (rank == _grade) {
            // 级牌（正的）
            return MAKE_CARD(5, 0x0D, suit, rank);
        } else if (rank <= 14) {
            // 从级牌+1到A
            unsigned power = rank - 5;
            return MAKE_CARD(5, power, suit, rank);
        } else { //if (rank == 15)
            // 2是机动主（正2）
            return MAKE_CARD(5, 0x0B, suit, 15);
        }
    } else {  // 副花色
        if (rank < _grade) {
            // 从5到级牌-1
            unsigned power = rank - 4;
            return MAKE_CARD(suit, power, suit, rank);
        } else if (rank == _grade) {
            // 级牌（副的）
            return MAKE_CARD(5, 0x0C, suit, rank);
        } else if (rank <= 14) {
            // 从级牌+1到A
            unsigned power = rank - 5;
            return MAKE_CARD(suit, power, suit, rank);
        } else { //if (rank == 15)
            // 2是机动主（副2）
            return MAKE_CARD(5, 0x0A, suit, 15);
        }
    }
}

void U5TKLogic::init() {
    _isGrabbing = true;
    _grade = 5;
    _grade2 = 5;
    _cycles = 0;
    _rounds = 0;
    _bankerPos = -1;

    _readyState.reset();  // 重置所有玩家的准备状态
}

void U5TKLogic::setup() {
    _state = State::WAITING;
    _trump = 0;
    _scores = 0;
    _defeatState = DefeatState::NOT_ASK;
    _scoreCards.resize(0);
    _shownCards.resize(0);

    if (!_isGrabbing) {
        _turnPos = _bankerPos;
    }
    else {
        _bankerPos = -1;
        _turnPos = -1;
    }
    _shownPos = -1;
    _leaderPos = -1;

    _handCards[0].clear();
    _handCards[1].clear();
    _handCards[2].clear();
    _handCards[3].clear();
    _shownCards.clear();
    _underCards.clear();
    _bringCards[0].clear();
    _bringCards[1].clear();
    _bringCards[2].clear();
    _bringCards[3].clear();
    _recordCards[0].clear();
    _recordCards[1].clear();
    _recordCards[2].clear();
    _recordCards[3].clear();
    _scoreCards.clear();

    _bringInfo[0] = 0;
    _bringInfo[1] = 0;
    _bringInfo[2] = 0;
    _bringInfo[3] = 0;
}

void U5TKLogic::sitDown(unsigned seat) {
    _readyState.reset(seat);
}

void U5TKLogic::standUp(unsigned seat) {
    _readyState.reset(seat);
}

void U5TKLogic::forcedEnd() {
    init();
    setup();
}

U5TKLogic::ErrorType U5TKLogic::setReady(int pos) {
    if (_readyState.none()) {
        setup();
    }
    if (_state != State::WAITING) {
        return ErrorType::STATE_ERROR;
    }

    if (_readyState.test(pos)) {
        return ErrorType::UNKNOWN_ERROR;
    }
    _readyState.set(pos);
    if (_readyState.all()) {
        setup();
        shuffleCards();
        _state = State::SENDING;
    }
    return ErrorType::SUCCESS;
}

void U5TKLogic::shuffleCards() {
    CARD totalCards[92];
    div_t ret;
    for (int i = 0; i < 88; ++i) {
        ret = div(i, 11);
        totalCards[i] = _calculateCard(MAKE_CARD(0, 0, (ret.quot >> 1) + 1, ret.rem + 5));
    }
    totalCards[88] = JOKER_BLACK;
    totalCards[89] = JOKER_BLACK;
    totalCards[90] = JOKER_RED;
    totalCards[91] = JOKER_RED;

    std::uniform_int_distribution<int> dist(0, 92);
    std::random_device seedGen;
    std::mt19937 engine(seedGen());

    bool index[92] = { false };
    for (int i = 0; i < 84; ++i) {
        do {
            //int k = rand() % 92;
            int k = dist(engine);
            if (!index[k]) {
                _handCards[i & 3].push_back(totalCards[k]);
                index[k] = true;
                break;
            }
        } while (1);
    }
    for (int k = 0; k < 92; ++k) {
        if (!index[k]) {
            _underCards.push_back(totalCards[k]);
        }
    }

    _updateCards();
    _sendTime = std::chrono::system_clock::now();
}

void U5TKLogic::_updateCards() {
    std::transform(_handCards[0].begin(), _handCards[0].end(), _handCards[0].begin(), std::bind(&U5TKLogic::_calculateCard, this, std::placeholders::_1));
    std::transform(_handCards[1].begin(), _handCards[1].end(), _handCards[1].begin(), std::bind(&U5TKLogic::_calculateCard, this, std::placeholders::_1));
    std::transform(_handCards[2].begin(), _handCards[2].end(), _handCards[2].begin(), std::bind(&U5TKLogic::_calculateCard, this, std::placeholders::_1));
    std::transform(_handCards[3].begin(), _handCards[3].end(), _handCards[3].begin(), std::bind(&U5TKLogic::_calculateCard, this, std::placeholders::_1));
    std::transform(_underCards.begin(), _underCards.end(), _underCards.begin(), std::bind(&U5TKLogic::_calculateCard, this, std::placeholders::_1));
}

bool U5TKLogic::checkSendEnd() {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    int64_t dt = std::chrono::duration_cast<std::chrono::seconds>(now - _sendTime).count();
    if (dt > 21) {
        afterSend();
        return true;
    }
    return false;
}

void U5TKLogic::_sortCards() {
    std::sort(_handCards[0].begin(), _handCards[0].end(), std::greater<CARD>());
    std::sort(_handCards[1].begin(), _handCards[1].end(), std::greater<CARD>());
    std::sort(_handCards[2].begin(), _handCards[2].end(), std::greater<CARD>());
    std::sort(_handCards[3].begin(), _handCards[3].end(), std::greater<CARD>());
    std::sort(_underCards.begin(), _underCards.end(), std::greater<CARD>());
}

U5TKLogic::ErrorType U5TKLogic::_checkShowTrump(int pos, const std::vector<CARD> &cards) const {
    if (cards.size() < 3) {
        return ErrorType::UNCHARTERED_SHOW_COUNT;
    }
    if (JOKER_BLACK != cards[0] && JOKER_RED != cards[0]) {
        return ErrorType::SHOW_CARDS_SHOULD_CONTAIN_JOKER;
    }
    if (GET_RANK(cards[1]) != _grade || cards[1] != cards[cards.size() - 2]) {
        return ErrorType::SHOW_CARDS_SHOULD_HAVE_GRAGE;
    }
    if (GET_RANK(cards.back()) != 15) {
        return ErrorType::SHOW_CARDS_SHOULD_CONTAIN_2;
    }

    auto shownCnt = _shownCards.size();
    if (shownCnt == 0) {  // 无人叫主
        return ErrorType::SUCCESS;
    }

    // 有人叫主了，必须要4张才能反
    if (cards.size() < 4) {
        return ErrorType::REBEL_NEED_TWO_GRADE;
    }

    if (shownCnt == 3) {
        return ErrorType::SUCCESS;
    }

    if (shownCnt == 4) {
        if (GET_SUIT(cards[1]) > GET_SUIT(_shownCards[1])) {
            return ErrorType::SUCCESS;
        }
        return ErrorType::REBEL_SHOULD_GREATER_THAN_ORIGIN;
    }

    return ErrorType::UNKNOWN_ERROR;
}

U5TKLogic::ErrorType U5TKLogic::doShowTrump(int pos, const std::vector<CARD> &cards) {
    if (_state != U5TKLogic::State::SENDING && _state != U5TKLogic::State::SHOWING) {
        return ErrorType::STATE_ERROR;
    }

    if (_state == U5TKLogic::State::SENDING) {
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        int64_t dt = std::chrono::duration_cast<std::chrono::seconds>(now - _sendTime).count();
        ptrdiff_t offset = std::max(static_cast<ptrdiff_t>(dt), static_cast<ptrdiff_t>(_handCards[pos].size()));
        if (_IsSubCardStringUnsorted(std::vector<CARD>(_handCards[pos].begin(), _handCards[pos].begin() + offset), cards)) {
            return ErrorType::ILLEGAL_CARDS;
        }
    }
    else {
        if (pos != _turnPos) {
            return ErrorType::NOT_YOUR_TURN;
        }
        if (!_IsSubCardString(_handCards[pos], cards)) {
            return ErrorType::ILLEGAL_CARDS;
        }
    }

    U5TKLogic::ErrorType ret = _checkShowTrump(pos, cards);
    if (ret == ErrorType::SUCCESS) {
        _shownCards.assign(cards.begin(), cards.end());
        _trump = GET_SUIT(cards[1]);
        _shownPos = pos;
        _passFlag.reset();  // 重置所有玩家的不反状态
        _passFlag.set(_shownPos);  // 自己不能反自己
        if (_isGrabbing) {
            _bankerPos = pos;
        }

        _updateCards();
        if (_state == U5TKLogic::State::SHOWING) {
            _sortCards();
            _turnPos = (_turnPos + 1) & 3;
        }
    }
    return ret;
}

bool U5TKLogic::_checkNoTrump() const {
    for (unsigned pos = 0; pos < 4; ++pos) {
        // 理好牌后，手上第一张牌小于0x050A010FU（副2），可以断定没有机动主
        if (_handCards[pos][0] < 0x050A010FU) {
            return true;
        }
    }
    return false;
}

bool U5TKLogic::afterSend() {
    _sortCards();
    if (_checkNoTrump()) {
        _scores = 205;
        _gameOver();
        return false;
    }

    if (!_shownCards.empty()) {  // 有人叫主了
        _turnPos = (_shownPos + 1) & 3;  // 从叫主玩家下家开始
    }
    else {  // 没人叫主
        if (_isGrabbing) {  // 争庄局
            // 随机选一家开始
            _turnPos = rand() & 3;
        } else {  // 定庄局
            _turnPos = _bankerPos;  // 从庄家开始
        }
    }
    _state = U5TKLogic::State::SHOWING;
    return true;
}

U5TKLogic::ErrorType U5TKLogic::passShow(int pos) {
    if (_state == U5TKLogic::State::SHOWING) {
        if (pos == _turnPos) {
            _passFlag.set(pos);
            if (_passFlag.all()) {  // 都过了
                if (_shownPos > 0) {  // 有人叫主了
                    sendUnderCards();
                }
                else {  // 憋庄
                    _scores = _isGrabbing ? 205 : 80;
                    _gameOver();
                }
            }
            else {
                _turnPos = (_turnPos + 1) & 3;
            }
            return ErrorType::SUCCESS;
        }
        return ErrorType::NOT_YOUR_TURN;
    }
    return ErrorType::STATE_ERROR;
}

void U5TKLogic::sendUnderCards() {
    std::vector<CARD> &bankerHandCards = _handCards[_bankerPos];
    bankerHandCards.reserve(29);
    std::copy(_underCards.begin(), _underCards.end(), std::back_inserter(bankerHandCards));  // 给庄家
    std::sort(bankerHandCards.begin(), bankerHandCards.end(), std::greater<CARD>());  // 给庄家手上的牌排序
    _turnPos = _bankerPos;  // 轮到庄家埋底牌
    _state = State::EXCHANGING;
}

U5TKLogic::ErrorType U5TKLogic::askDefeat(int pos) {
    if (_state == U5TKLogic::State::EXCHANGING) {
        if (pos == _bankerPos) {
            _defeatState = DefeatState::ASKING;
            return ErrorType::SUCCESS;
        }
        return ErrorType::NOT_YOUR_TURN;
    }
    return ErrorType::STATE_ERROR;
}

U5TKLogic::ErrorType U5TKLogic::admitDefeat(int pos, bool argee) {
    if (_state != U5TKLogic::State::EXCHANGING) {
        return ErrorType::STATE_ERROR;
    }

    if (pos != (_bankerPos + 2) % 4) {
        return ErrorType::NOT_YOUR_TURN;
    }

    switch (_defeatState) {
    case DefeatState::NOT_ASK:
        return ErrorType::NO_ASK_DEFEAT;
    case DefeatState::ASKING:
        _defeatState = DefeatState::ASKED;
        if (argee) {
            _scores = 80;
            _gameOver();
        }
        return ErrorType::SUCCESS;
    case DefeatState::ASKED:
        return ErrorType::DEFEAT_HAS_ASKED;
    }
    return ErrorType::UNKNOWN_ERROR;
}

U5TKLogic::ErrorType U5TKLogic::doExchange(int pos, const std::vector<CARD> &cards) {
    if (_state != U5TKLogic::State::EXCHANGING) {
        return ErrorType::STATE_ERROR;
    }
    if (pos != _bankerPos) {
        return ErrorType::NOT_YOUR_TURN;
    }
    if (cards.size() != 8) {
        return ErrorType::UNCHARTERED_EXCHANGE_COUNT;
    }

    if (cards.end() != std::find_if(cards.begin(), cards.end(), &U5TKLogic::isScore)) {
        return ErrorType::EXCHANGED_SHOULDNOT_CONTAIN_SCORES;
    }

    std::vector<CARD> &bankerHandCards = _handCards[_bankerPos];
    if (!_IsSubCardString(bankerHandCards, cards)) {
        return ErrorType::ILLEGAL_CARDS;
    }

    _DeleteSubCardString(bankerHandCards, cards);
    _state = State::BRINGING;
    _turnPos = pos;
    _leaderPos = pos;
    return ErrorType::SUCCESS;
}

U5TKLogic::ErrorType U5TKLogic::doBring(int pos, const std::vector<CARD> &cards) {
    if (_state != U5TKLogic::State::BRINGING) {
        return ErrorType::STATE_ERROR;
    }
    if (pos != _turnPos) {
        return ErrorType::NOT_YOUR_TURN;
    }

    if (cards.empty() || !_IsSubCardString(_handCards[pos], cards)) {
        return ErrorType::ILLEGAL_CARDS;
    }

    ErrorType ret = (pos == _leaderPos) ? _doLeaderBring(pos, cards) : _doFollowBring(pos, cards);
    if (ret == ErrorType::SUCCESS) {
        _DeleteSubCardString(_handCards[pos], cards);
        std::copy(cards.begin(), cards.end(), std::back_inserter(_bringCards[pos]));
        std::copy(cards.begin(), cards.end(), std::back_inserter(_recordCards[pos]));

        _turnPos = (_turnPos + 1) & 3;  // 轮转
        if (_turnPos == _leaderPos) {  // 已经出玩一轮牌
            _nextBring();
        }
    }
    return ret;
}

U5TKLogic::BRING_INFO U5TKLogic::_analyze(const std::vector<CARD> &cards) {
    auto cnt = cards.size();
    unsigned level0 = GET_LEVEL(cards[0]);
    unsigned power0 = GET_POWER(cards[0]);
    if (cnt == 1) {  // 单张
        return MAKE_BRING_INFO(1, level0, BRING_TYPE_SINGLE, power0);
    }

    unsigned leveln = GET_LEVEL(cards.back());
    unsigned powern = GET_LEVEL(cards.back());

    if (level0 != leveln) {  // 不同等级多张
        return MAKE_BRING_INFO(cnt, 0, 0, 0);
    }

    // 同等级多张
    if (cnt == 2) {  // 2张
        if (cards[0] == cards[1]) {  // 对子
            return MAKE_BRING_INFO(2, level0, BRING_TYPE_PAIR, power0);
        } else {  // 2张单牌
            return MAKE_BRING_INFO(2, level0, BRING_TYPE_NONE, powern);
        }
    }

    // 更多张
    std::vector<CARD> pairs;
    pairs.reserve(21);
    for (const CARD *ca = &cards.back(); ca >= &cards.front(); --ca) {  // 从后往前检测
        if (ca > &cards.front() && ca[0] == ca[-1]) {
            pairs.push_back(ca[0]);  // 选出对子
            --ca;
        }
        else {
            return MAKE_BRING_INFO(cnt, level0, BRING_TYPE_SINGLE, GET_POWER(ca[0]));// 发现单张
        }
    }
    for (size_t i = 0, cnt = pairs.size(); i < cnt - 1; ++i) {
        unsigned power0 = GET_POWER(pairs[i]);
        unsigned power1 = GET_POWER(pairs[i + 1]);
        if (power0 != power1 - 1) {
            return MAKE_BRING_INFO(cnt, level0, BRING_TYPE_PAIR, power0);
        }
    }
    return MAKE_BRING_INFO(cnt, level0, BRING_TYPE_TRACTOR, GET_POWER(pairs[0]));
}

U5TKLogic::ErrorType U5TKLogic::_doLeaderBring(int pos, const std::vector<CARD> &cards) {
    BRING_INFO info = _analyze(cards);
    unsigned level = BRING_INFO_GET_LEVEL(info);
    unsigned type = BRING_INFO_GET_TYPE(info);

    if (level == 0) {
        return ErrorType::UNCHARTERED_BRING_COUNT;
    }

    if (type == BRING_TYPE_NONE) {
        return ErrorType::UNCHARTERED_BRING_TYPE;
    }
    _bringInfo[pos] = info;
    return ErrorType::SUCCESS;
}

unsigned U5TKLogic::_countSingles(const std::vector<CARD> &cards, unsigned level) {
    unsigned cnt = 0;
    for (std::vector<CARD>::const_iterator it = cards.begin(); it != cards.end(); ++it) {
        unsigned l = GET_LEVEL(*it);
        if (l == level) {
            ++cnt;
        }
        else if (l < level) {  // 由于牌是排好序的，到这里说明已找完
            return cnt;
        }
    }
    return cnt;
}

unsigned U5TKLogic::_countPairs(const std::vector<CARD> &cards, unsigned level) {
    unsigned cnt = 0;
    for (std::vector<CARD>::const_iterator it = cards.begin(); it != cards.end(); ++it) {
        unsigned l = GET_LEVEL(*it);
        if (l == level) {
            if (it + 1 != cards.end() && *it == *(it + 1)) {
                ++cnt;
                ++it;
            }
        }
        else if (l < level) {  // 由于牌是排好序的，到这里说明已找完
            return cnt;
        }
    }
    return cnt;
}

U5TKLogic::ErrorType U5TKLogic::_doFollowBring(int pos, const std::vector<CARD> &cards) {
    BRING_INFO leaderBringInfo = _bringInfo[_leaderPos];

    int cnt = cards.size();
    if (cnt != BRING_INFO_GET_COUNT(leaderBringInfo)) {
        return ErrorType::UNCHARTERED_BRING_COUNT;
    }

    BRING_INFO followBringInfo = _analyze(cards);
    unsigned leaderType = BRING_INFO_GET_TYPE(leaderBringInfo);
    if (leaderType == BRING_TYPE_NONE) {
        _bringInfo[pos] = followBringInfo;
        return ErrorType::SUCCESS;
    }

    unsigned leaderLevel = BRING_INFO_GET_LEVEL(leaderBringInfo);
    unsigned followLevel = BRING_INFO_GET_LEVEL(followBringInfo);
    unsigned handSinglesCnt = _countSingles(_handCards[pos], leaderLevel);

    if (leaderType == BRING_TYPE_SINGLE) {  // 首家出单张
        // 没有同等级单张，可以随便出。有，则必须出同等级单张
        if (handSinglesCnt == 0 || followLevel == leaderLevel) {  // 必须出同等级单张
            _bringInfo[pos] = followBringInfo;
            return ErrorType::SUCCESS;
        }
        return ErrorType::FOLLOW_BRING_SHOULD_MATCH_SUIT;
    }

    // 首家出对子或者拖拉机
    unsigned leaderCnt = BRING_INFO_GET_COUNT(leaderBringInfo);
    if (handSinglesCnt <= leaderCnt) {  // 手上单张数小于等于首家出牌张数
        unsigned perpareSinglesCnt = _countSingles(cards, leaderLevel);
        if (perpareSinglesCnt == handSinglesCnt) {  // 必须将单张出完
            _bringInfo[pos] = followBringInfo;
            return ErrorType::SUCCESS;
        }
        return ErrorType::FOLLOW_BRING_SHOULD_MATCH_SUIT;
    }

    unsigned leaderPairsCnt = leaderCnt >> 1;
    unsigned handPairsCnt = _countPairs(_handCards[pos], leaderLevel);
    unsigned perparePairsCnt = _countPairs(cards, leaderLevel);
    if (handPairsCnt <= leaderPairsCnt) {  // 手上对子数小于等于首家出牌对子数
        if (perparePairsCnt == handPairsCnt) {  // 必须将对子出完
            _bringInfo[pos] = followBringInfo;
            return ErrorType::SUCCESS;
        }
        return ErrorType::FOLLOW_BRING_SHOULD_MATCH_PAIR_COUNT;
    }

    // 手上对子数大于首家出牌对子数的情况
    if (perparePairsCnt == leaderPairsCnt) {  // 必须出与首家等数量的对子数
        _bringInfo[pos] = followBringInfo;
        return ErrorType::SUCCESS;
    }
    return ErrorType::FOLLOW_BRING_SHOULD_MATCH_PAIR_COUNT;
}

bool U5TKLogic::_greatThan(BRING_INFO info1, BRING_INFO info2) {
    unsigned level1 = BRING_INFO_GET_LEVEL(info1);
    unsigned level2 = BRING_INFO_GET_LEVEL(info2);

    unsigned type1 = BRING_INFO_GET_TYPE(info1);
    unsigned type2 = BRING_INFO_GET_TYPE(info2);

    if (level1 == level2) {  // 相同等级
        if (type2 == BRING_TYPE_SINGLE || type2 == BRING_TYPE_PAIR || type2 == BRING_TYPE_TRACTOR) {
            unsigned power1 = BRING_INFO_GET_POWER(info1);
            unsigned power2 = BRING_INFO_GET_POWER(info2);
            return (type1 == type2 && power1 > power2);
        }
        return false;  // 混合牌先出为大
    }
    else {
        if (level1 == 0x05U) {  // 用主牌毙或者消主
            return (type1 == type2);
        }

        // 不同等级副牌，先出为大；主牌大于副牌
        return false;
    }
}

void U5TKLogic::_nextBring() {
    BRING_INFO maxInfo = _bringInfo[_leaderPos];
    int maxPos = _leaderPos;
    for (int pos = (_leaderPos + 1) & 3; pos != _leaderPos; pos = (pos + 1) & 3) {
        if (_greatThan(_bringInfo[pos], maxInfo)) {
            maxInfo = _bringInfo[pos];
            maxPos = pos;
        }
    }

    // 闲家大
    if (maxPos == ((_bankerPos + 1) & 3) || maxPos == ((_bankerPos + 3) & 3)) {
        // 捡分
        int pos = _leaderPos;
        do {
            for (CARD ca : _bringCards[pos]) {
                unsigned suit = GET_SUIT(ca);
                unsigned rank = GET_RANK(ca);
                if (suit <= 4) {
                    switch (rank) {
                    case 5: _scores += 5; _scoreCards.push_back(ca); break;
                    case 10: case 13: _scores += 10; _scoreCards.push_back(ca); break;
                    default: break;
                    }
                }
            }
            pos = (pos + 1) & 3;
        } while (pos != _leaderPos);
    }

    // 大牌玩家下轮先出
    _leaderPos = maxPos;
    _turnPos = maxPos;

    if (_handCards[_turnPos].empty()) {  // 手上没有牌了，结束游戏
        _gameOver();
    }
    else {  // 手上还有牌，清空，准备下轮
        _bringInfo[0] = 0;
        _bringInfo[1] = 0;
        _bringInfo[2] = 0;
        _bringInfo[3] = 0;
        _bringCards[0].clear();
        _bringCards[1].clear();
        _bringCards[2].clear();
        _bringCards[3].clear();
    }
}

void U5TKLogic::_gameOver() {

#define CASE_FOR_PURE_EMPTY()               \
    do {                                    \
        _isGrabbing = true;             \
        _bankerPos = -1;                    \
        _grade = 5;                         \
        _grade2 = 5;                        \
        ++_cycles;                          \
        _rounds = 0;                        \
    } while (0)

#define CASE_FOR_PASS(newGrade)             \
    do {                                    \
        _isGrabbing = false;            \
        _bankerPos = (_bankerPos + 2) & 3;  \
        _grade = newGrade;                  \
        ++_rounds;                          \
    } while (0)

#define CASE_FOR_LOSE(newGrade)             \
    do {                                    \
        _isGrabbing = false;            \
        _bankerPos = (_bankerPos + 1) & 3;  \
        _grade2 = _grade;                   \
        _grade = newGrade;                  \
        ++_rounds;                          \
    } while (0)

    if (_scores == 0) {  // 清光
        CASE_FOR_PURE_EMPTY();
    }
    else if (_scores < 25) {  // 毛光
        if (_grade == 5) {  // 原来打5，则升到K
            CASE_FOR_PASS(13);
        }
        else {  // 原来打10或K，则争庄打5
            CASE_FOR_PURE_EMPTY();
        }
    }
    else if (_scores < 80) {  // 过庄
        if (_grade == 13) {  // 原来打K，则争庄打5
            CASE_FOR_PURE_EMPTY();
        }
        else {  // 原来打5，则升到打10；原来打10，则升到打K
            if (_grade == 5) CASE_FOR_PASS(10);
            else CASE_FOR_PASS(13);
        }
    }
    else if (_scores < 125) {  // 败庄
        int grade2 = _grade2;  // 否则_grade2的值会在宏里面被覆盖
        CASE_FOR_LOSE(grade2);
    }
    else if (_scores < 165) {  // 翻
        if (_grade2 == 13) {  // 原来闲家打K，则争庄打5
            CASE_FOR_PURE_EMPTY();
        } else {  // 原来闲家打5，则升到打10；原来闲家打10，则升到打K
            if (_grade2 == 5) CASE_FOR_LOSE(10);
            else CASE_FOR_LOSE(13);
        }
    }
    else if (_scores <= 200) {  // 翻两级
        if (_grade2 == 5) {  // 原来闲家打5，则升到打K
            CASE_FOR_LOSE(13);
        } else {  // 原来闲家打10或打K，则争庄打5
            CASE_FOR_PURE_EMPTY();
        }
    }
    else {  // 有玩家无机动主或争庄局憋庄，信息不变
        ++_rounds;
    }

    _state = State::WAITING;
    _readyState.reset();  // 重置所有玩家的准备状态
}
