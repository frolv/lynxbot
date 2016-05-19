# lynxbot makefile

PROGNAME=lynxbot

INC=-I./include

LIB=-lcurl -lpthread

CXX=g++
CXXFLAGS=$(INC) -Wall -Wextra -c -std=c++14

CPR=auth.o cookies.o cprtypes.o digest.o error.o multipart.o parameters.o\
    payload.o proxies.o session.o util.o
_CPRH=api.h auth.h body.h cookies.h cpr.h cprtypes.h curlholder.h defines.h\
     digest.h error.h multipart.h parameters.h payload.h proxies.h response.h\
     session.h timeout.h util.h
CPRH=$(patsubst %,include/cpr/%,$(_CPRH))

all: CXXFLAGS+=-O2
all: exec

debug: CXXFLAGS+=-DDEBUG -g
debug: exec

exec: $(CPR) main.o jsoncpp.o utils.o client.o TwitchBot.o Moderator.o\
	URLParser.o CommandHandler.o CustomCommandHandler.o TimerManager.o\
	GEReader.o ExpressionParser.o OpMap.o OptionParser.o SelectionWheel.o\
	EventManager.o Giveaway.o SkillMap.o RSNList.o
	$(CXX) -o $(PROGNAME) $(LIB) $(CPR) main.o jsoncpp.o utils.o client.o\
		TwitchBot.o Moderator.o URLParser.o CommandHandler.o\
		CustomCommandHandler.o TimerManager.o GEReader.o\
		ExpressionParser.o OpMap.o OptionParser.o SelectionWheel.o\
		EventManager.o Giveaway.o SkillMap.o RSNList.o

main.o: src/main.cpp src/utils.h src/TwitchBot.h
	$(CXX) $(CXXFLAGS) src/main.cpp

utils.o: src/utils.cpp src/utils.h
	${CXX} ${CXXFLAGS} src/utils.cpp

client.o: src/client.cpp src/client.h
	${CXX} ${CXXFLAGS} src/client.cpp

TwitchBot.o: src/TwitchBot.cpp src/TwitchBot.h src/version.h
	${CXX} ${CXXFLAGS} src/TwitchBot.cpp

Moderator.o: src/Moderator.cpp src/Moderator.h
	${CXX} ${CXXFLAGS} src/Moderator.cpp

URLParser.o: src/URLParser.cpp src/URLParser.h
	${CXX} ${CXXFLAGS} src/URLParser.cpp

CommandHandler.o: src/CommandHandler.cpp src/CommandHandler.h
	${CXX} ${CXXFLAGS} src/CommandHandler.cpp

CustomCommandHandler.o: src/CustomCommandHandler.cpp src/CommandHandler.h
	${CXX} ${CXXFLAGS} src/CustomCommandHandler.cpp

TimerManager.o: src/TimerManager.cpp src/TimerManager.h
	${CXX} ${CXXFLAGS} src/TimerManager.cpp

GEReader.o: src/GEReader.cpp src/GEReader.h
	${CXX} ${CXXFLAGS} src/GEReader.cpp

ExpressionParser.o: lib/ExpressionParser.cpp include/ExpressionParser.h
	${CXX} ${CXXFLAGS} lib/ExpressionParser.cpp

OpMap.o: lib/OpMap.cpp include/OpMap.h
	${CXX} ${CXXFLAGS} lib/OpMap.cpp

OptionParser.o: src/OptionParser.cpp src/OptionParser.h
	${CXX} ${CXXFLAGS} src/OptionParser.cpp

SelectionWheel.o: src/SelectionWheel.cpp src/SelectionWheel.h
	${CXX} ${CXXFLAGS} src/SelectionWheel.cpp

EventManager.o: src/EventManager.cpp src/EventManager.h
	${CXX} ${CXXFLAGS} src/EventManager.cpp

Giveaway.o: src/Giveaway.cpp src/Giveaway.h
	${CXX} ${CXXFLAGS} src/Giveaway.cpp

SkillMap.o: src/SkillMap.cpp src/SkillMap.h
	${CXX} ${CXXFLAGS} src/SkillMap.cpp

RSNList.o: src/RSNList.cpp src/RSNList.h
	${CXX} ${CXXFLAGS} src/RSNList.cpp

jsoncpp.o: lib/jsoncpp.cpp include/json/json.h include/json/json-forwards.h
	${CXX} ${CXXFLAGS} lib/jsoncpp.cpp

# cpr library
auth.o: lib/cpr/auth.cpp $(CPRH)
	$(CXX) $(CXXFLAGS) lib/cpr/auth.cpp

cookies.o: lib/cpr/cookies.cpp $(CPRH)
	$(CXX) $(CXXFLAGS) lib/cpr/cookies.cpp

cprtypes.o: lib/cpr/cprtypes.cpp $(CPRH)
	$(CXX) $(CXXFLAGS) lib/cpr/cprtypes.cpp

digest.o: lib/cpr/digest.cpp $(CPRH)
	$(CXX) $(CXXFLAGS) lib/cpr/digest.cpp

error.o: lib/cpr/error.cpp $(CPRH)
	$(CXX) $(CXXFLAGS) lib/cpr/error.cpp

multipart.o: lib/cpr/multipart.cpp $(CPRH)
	$(CXX) $(CXXFLAGS) lib/cpr/multipart.cpp

parameters.o: lib/cpr/parameters.cpp $(CPRH)
	$(CXX) $(CXXFLAGS) lib/cpr/parameters.cpp

payload.o: lib/cpr/payload.cpp $(CPRH)
	$(CXX) $(CXXFLAGS) lib/cpr/payload.cpp

proxies.o: lib/cpr/proxies.cpp $(CPRH)
	$(CXX) $(CXXFLAGS) lib/cpr/proxies.cpp

session.o: lib/cpr/session.cpp $(CPRH)
	$(CXX) $(CXXFLAGS) lib/cpr/session.cpp

util.o: lib/cpr/util.cpp $(CPRH)
	$(CXX) $(CXXFLAGS) lib/cpr/util.cpp

clean:
	rm -f $(PROGNAME) *.o
