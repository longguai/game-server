#include "GameTable.h"
#include "../common-test/TimerEngine.h"

#define CMD_U5TK_SEND_CARD 4001
#define CMD_U5TK_SHOW 4002
#define CMD_U5TK_PASS 4003
#define CMD_U5TK_EXCHANGE 4004
#define CMD_U5TK_ASK_DEFEAT 4005
#define CMD_U5TK_AGREE_DEFEAT 4006
#define CMD_U5TK_DISAGREE_DEFEAT 4007
#define CMD_U5TK_BRING 4008
#define CMD_U5TK_REFRESH 4009

static inline std::vector<uint32_t> _TransformCards(const std::vector<U5TKLogic::CARD> &cards) {
    std::vector<uint32_t> ret;
    ret.reserve(cards.size());
    std::transform(cards.begin(), cards.end(), std::back_inserter(ret), std::bind(std::bit_and<U5TKLogic::CARD>(), 0x0000FFFF, std::placeholders::_1));
    return ret;
}

bool GameTable::handsUp(unsigned seat) {
    std::lock_guard<jw::QuickMutex> g(_mutex);
    (void)g;
    LOG_DEBUG(u8"玩家%d准备", seat);
    _participants[seat]->status = UserStatus::Free;
    if (_logic.setReady(seat) == U5TKLogic::ErrorType::SUCCESS) {
        if (_logic.getState() == U5TKLogic::State::SENDING) {
            LOG_DEBUG(u8"准备完毕");
            jw::TimerEngine::getInstance()->registerTimer(reinterpret_cast<uintptr_t>(this), std::chrono::seconds(1), 1, [this](int64_t) {
                jw::TimerEngine::getInstance()->unregisterTimer(reinterpret_cast<uintptr_t>(this));
                LOG_DEBUG(u8"开始发牌");
                std::lock_guard<jw::QuickMutex> g(_mutex);
                (void)g;
                _sendGameState();
                jw::TimerEngine::getInstance()->registerTimer(reinterpret_cast<uintptr_t>(this), std::chrono::milliseconds(500), jw::TimerEngine::REPEAT_FOREVER, [this](int64_t) {
                    if (_logic.checkSendEnd()) {
                        LOG_DEBUG(u8"结束发牌");
                        jw::TimerEngine::getInstance()->unregisterTimer(reinterpret_cast<uintptr_t>(this));

                        std::lock_guard<jw::QuickMutex> g(_mutex);
                        (void)g;
                        _sendGameState();
                    }
                });
            });
        }
        return true;
    }
    return false;
}

void GameTable::forcedStandUp(unsigned seat) {
    LOG_DEBUG(u8"forcedStandUp %d", seat);
    std::lock_guard<jw::QuickMutex> g(_mutex);
    (void)g;
    _participants[seat] = nullptr;

    _logic.forcedEnd();
}

void GameTable::_handleLogicResult(unsigned seat, unsigned cmd, unsigned tag, U5TKLogic::ErrorType errorType) {
    LOG_DEBUG(u8"_handleError seat = %u cmd = %u error = %d", seat, cmd, static_cast<int>(errorType));

    jw::cppJSON json(jw::cppJSON::ValueType::Object);
    json.insert(std::make_pair("result", errorType == U5TKLogic::ErrorType::SUCCESS));
    switch (errorType) {
    case U5TKLogic::ErrorType::SUCCESS:
        break;
    case U5TKLogic::ErrorType::UNKNOWN_ERROR:
        json.insert(std::make_pair("reason", u8"未知错误"));
        break;
    case U5TKLogic::ErrorType::STATE_ERROR:
        json.insert(std::make_pair("reason", u8"状态错误"));
        break;
    case U5TKLogic::ErrorType::NOT_YOUR_TURN:
        json.insert(std::make_pair("reason", u8"还没轮到你"));
        break;
    case U5TKLogic::ErrorType::ILLEGAL_CARDS:
        json.insert(std::make_pair("reason", u8"手上没对应的牌"));
        break;
    case U5TKLogic::ErrorType::ILLEGAL_POS:
        json.insert(std::make_pair("reason", u8"非法的座位"));
        break;
    case U5TKLogic::ErrorType::UNCHARTERED_SHOW_COUNT:
        json.insert(std::make_pair("reason", u8"不合规则的叫主用牌数量"));
        break;
    case U5TKLogic::ErrorType::SHOW_CARDS_SHOULD_CONTAIN_JOKER:
        json.insert(std::make_pair("reason", u8"叫主的牌必须包含王"));
        break;
    case U5TKLogic::ErrorType::SHOW_CARDS_SHOULD_HAVE_GRAGE:
        json.insert(std::make_pair("reason", u8"叫主的牌必须包含级牌"));
        break;
    case U5TKLogic::ErrorType::SHOW_CARDS_SHOULD_CONTAIN_2:
        json.insert(std::make_pair("reason", u8"叫主的牌必须包含2"));
        break;
    case U5TKLogic::ErrorType::REBEL_NEED_TWO_GRADE:
        json.insert(std::make_pair("reason", u8"反主的牌必须包含两张级牌"));
        break;
    case U5TKLogic::ErrorType::REBEL_SHOULD_GREATER_THAN_ORIGIN:
        json.insert(std::make_pair("reason", u8"反主的花色必须大于原花色"));
        break;
    case U5TKLogic::ErrorType::NO_ASK_DEFEAT:
        json.insert(std::make_pair("reason", u8"庄家没有发起投降询问"));
        break;
    case U5TKLogic::ErrorType::DEFEAT_HAS_ASKED:
        json.insert(std::make_pair("reason", u8"已经问过了"));
        break;
    case U5TKLogic::ErrorType::UNCHARTERED_EXCHANGE_COUNT:
        json.insert(std::make_pair("reason", u8"埋牌数量不对"));
        break;
    case U5TKLogic::ErrorType::EXCHANGED_SHOULDNOT_CONTAIN_SCORES:
        json.insert(std::make_pair("reason", u8"不能埋分"));
        break;
    case U5TKLogic::ErrorType::UNCHARTERED_BRING_COUNT:
        json.insert(std::make_pair("reason", u8"出牌数量不正确"));
        break;
    case U5TKLogic::ErrorType::UNCHARTERED_BRING_TYPE:
        json.insert(std::make_pair("reason", u8"出牌类型不正确"));
        break;
    case U5TKLogic::ErrorType::FOLLOW_BRING_SHOULD_MATCH_SUIT:
        json.insert(std::make_pair("reason", u8"出牌花色不正确"));
        break;
    case U5TKLogic::ErrorType::FOLLOW_BRING_SHOULD_MATCH_PAIR_COUNT:
        json.insert(std::make_pair("reason", u8"出牌对子数不正确"));
        break;
    default:
        break;
    }
    _participants[seat]->deliver(jw::JsonPacketSplitter::encodeSendPacket(cmd, tag, json));
}

void GameTable::_sendGameState() {
    jw::cppJSON json(jw::cppJSON::ValueType::Object);

    json.insert(std::make_pair("state", _logic.getState()));
    json.insert(std::make_pair("isGrabbing", _logic.isGrabbing()));
    json.insert(std::make_pair("trump", _logic.getTrump()));
    json.insert(std::make_pair("grade", _logic.getGrade()));
    json.insert(std::make_pair("grade2", _logic.getGrade2()));
    json.insert(std::make_pair("banker", _logic.getBankerPos()));
    json.insert(std::make_pair("shown", _logic.getShownPos()));
    json.insert(std::make_pair("turn", _logic.getTurnPos()));
    json.insert(std::make_pair("scores", _logic.getScores()));

    json.insert(std::make_pair("showCards", _TransformCards(_logic.getShownCards())));
    std::vector<std::vector<uint32_t> > bringingCards({ _TransformCards(_logic.getBringCards(0)), _TransformCards(_logic.getBringCards(1)), _TransformCards(_logic.getBringCards(2)), _TransformCards(_logic.getBringCards(3)) });
    json.insert(std::make_pair("bringingCards", bringingCards));
    std::vector<size_t> broughtCounts({ _logic.getRecordCards(0).size(), _logic.getRecordCards(1).size(), _logic.getRecordCards(2).size(), _logic.getRecordCards(3).size() });
    json.insert(std::make_pair("broughtCounts", broughtCounts));
    json.insert(std::make_pair("scoreCards", _TransformCards(_logic.getScoreCards())));

    for (size_t i = 0; i < ParticipantCount; ++i) {
        json.erase("handCards");
        json.insert(std::make_pair("handCards", _TransformCards(_logic.getHandCards(i))));

        if (_logic.getState() == U5TKLogic::State::BRINGING && i == _logic.getBankerPos()) {
            json.insert(std::make_pair("underCards", _TransformCards(_logic.getUnderCards())));
        }
        else {
            json.erase("underCards");
        }
        std::vector<char> buf = jw::JsonPacketSplitter::encodeSendPacket(CMD_U5TK_REFRESH, PUSH_SERVICE_TAG, json);
        LOG_DEBUG(u8"_sendGameState: %.*s", (int)buf.size() - 4, &buf[4]);
        _participants[i]->deliver(buf);
    }
}

void GameTable::deliver(unsigned seat, unsigned cmd, unsigned tag, const jw::cppJSON &json) {
    try {
        std::lock_guard<jw::QuickMutex> g(_mutex);
        (void)g;

        switch (cmd) {
        case CMD_U5TK_SHOW: {
            std::vector<uint32_t> cards = json.getValueByKey<std::vector<uint32_t> >("cards");
            std::transform(cards.begin(), cards.end(), cards.begin(), std::bind(&U5TKLogic::_calculateCard, std::ref(_logic), std::placeholders::_1));
            U5TKLogic::ErrorType errorType = _logic.doShowTrump(seat, cards);
            if (errorType == U5TKLogic::ErrorType::SUCCESS) {
                _sendGameState();
            }
            _handleLogicResult(seat, cmd, tag, errorType);
            break;
        }
        case CMD_U5TK_PASS: {
            U5TKLogic::ErrorType errorType = _logic.passShow(seat);
            if (errorType == U5TKLogic::ErrorType::SUCCESS) {
                _sendGameState();
            }
            _handleLogicResult(seat, cmd, tag, errorType);
            if (_logic.getState() == U5TKLogic::State::EXCHANGING) {
                jw::TimerEngine::getInstance()->registerTimer(reinterpret_cast<uintptr_t>(this), std::chrono::seconds(1), 1, [this](int64_t) {
                    jw::TimerEngine::getInstance()->unregisterTimer(reinterpret_cast<uintptr_t>(this));
                    LOG_DEBUG(u8"给庄家发底牌");
                    std::lock_guard<jw::QuickMutex> g(_mutex);
                    (void)g;
                    _sendGameState();
                });
            }
            break;
        }
        case CMD_U5TK_EXCHANGE: {
            std::vector<uint32_t> cards = json.getValueByKey<std::vector<uint32_t> >("cards");
            std::transform(cards.begin(), cards.end(), cards.begin(), std::bind(&U5TKLogic::_calculateCard, std::ref(_logic), std::placeholders::_1));
            U5TKLogic::ErrorType errorType = _logic.doExchange(seat, cards);
            if (errorType == U5TKLogic::ErrorType::SUCCESS) {
                _sendGameState();
            }
            _handleLogicResult(seat, cmd, tag, errorType);
            break;
        }
        case CMD_U5TK_ASK_DEFEAT: {
            U5TKLogic::ErrorType errorType = _logic.askDefeat(seat);
            if (errorType == U5TKLogic::ErrorType::SUCCESS) {
                _sendGameState();
            }
            _handleLogicResult(seat, cmd, tag, errorType);
            break;
        }
        case CMD_U5TK_AGREE_DEFEAT: {
            U5TKLogic::ErrorType errorType = _logic.admitDefeat(seat, true);
            if (errorType == U5TKLogic::ErrorType::SUCCESS) {
                _sendGameState();
            }
            _handleLogicResult(seat, cmd, tag, errorType);
            break;
        }
        case CMD_U5TK_DISAGREE_DEFEAT: {
            U5TKLogic::ErrorType errorType = _logic.admitDefeat(seat, false);
            if (errorType == U5TKLogic::ErrorType::SUCCESS) {
                _sendGameState();
            }
            _handleLogicResult(seat, cmd, tag, errorType);
            break;
        }
        case CMD_U5TK_BRING: {
            std::vector<uint32_t> cards = json.getValueByKey<std::vector<uint32_t> >("cards");
            std::transform(cards.begin(), cards.end(), cards.begin(), std::bind(&U5TKLogic::_calculateCard, std::ref(_logic), std::placeholders::_1));
            U5TKLogic::ErrorType errorType = _logic.doBring(seat, cards);
            if (errorType == U5TKLogic::ErrorType::SUCCESS) {
                _sendGameState();
            }
            _handleLogicResult(seat, cmd, tag, errorType);
            break;
        }
        default:
            break;
        }
    }
    catch (std::exception &e) {
        LOG_ERROR("%s", e.what());
    }

    //_logic.operateWithCards(static_cast<U5TKLogic::Operation>(cmd - CMD_U5TK_SHOW + 2), seat, nullptr, nullptr);
    //U5TKLogic logic;
    //logic._grade = 10;
    //logic._trump = 2;
    //logic.shuffleCards();
    //logic._sortCards();
    //for (int i = 0; i < 4; ++i)
    //{
    //    //std::for_each(logic._handCards[i].begin(), logic._handCards[i].end(), std::bind(&printf, "%.8X  ", std::placeholders::_1));
    //    std::for_each(logic._handCards[i].begin(), logic._handCards[i].end(), std::bind(&printf, "%s ", std::bind(&U5TKLogic::cardDebugString, std::placeholders::_1)));
    //    puts("");
    //}
    ////std::for_each(logic._underCards.begin(), logic._underCards.end(), std::bind(&printf, "%.8X  ", std::placeholders::_1));
    //std::for_each(logic._underCards.begin(), logic._underCards.end(), std::bind(&printf, "%s ", std::bind(&U5TKLogic::cardDebugString, std::placeholders::_1)));

    //jw::cppJSON json(jw::cppJSON::ValueType::Object);
    //json.insert(std::make_pair("seat", 0));
    //json.insert(std::make_pair("operation", 0));
    //json.insert(std::make_pair("cards", std::vector<unsigned>({ 0, 1, 2, 3, 4, 5 })));
    //std::cout << json.PrintUnformatted() << std::endl;

    //jw::cppJSON::const_iterator it = json.find("cards");
    //std::vector<unsigned> vec = it->as<std::vector<unsigned> >();
    //return 0
}
