# lynxbot makefile

PROGNAME=lynxbot

INC=-I./include

LIB=-lcurl -lpthread -lssl -lcrypto

OBJ=obj
SRC=src
LIBD=lib

CXX=g++
CXXFLAGS=$(INC) -Wall -Wextra -c -std=c++14

# compile for debug or release (default: release)
DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CXXFLAGS+=-DDEBUG -g
	OBJ:=$(OBJ)/debug
	PROGNAME:=$(PROGNAME)-d
else
	CXXFLAGS+=-O2
	OBJ:=$(OBJ)/release
endif

_CPR=auth.o cookies.o cprtypes.o digest.o error.o multipart.o parameters.o\
    payload.o proxies.o session.o util.o
CPR=$(patsubst %,$(OBJ)/%,$(_CPR))
_CPRH=api.h auth.h body.h cookies.h cpr.h cprtypes.h curlholder.h defines.h\
     digest.h error.h multipart.h parameters.h payload.h proxies.h response.h\
     session.h timeout.h util.h
CPRH=$(patsubst %,include/cpr/%,$(_CPRH))
CPRD=$(LIBD)/cpr

JSONCPP=$(OBJ)/jsoncpp.o
_JSONH=json.h json-forwards.h
JSONH=$(patsubst %,include/json/%,$(_JSONH))
JSOND=$(LIBD)/json

_LYNXBOT=main.o utils.o client.o TwitchBot.o Moderator.o\
	URLParser.o CommandHandler.o CustomCommandHandler.o TimerManager.o\
	GEReader.o ExpressionParser.o OpMap.o OptionParser.o SelectionWheel.o\
	EventManager.o Giveaway.o SkillMap.o RSNList.o
LYNXBOT=$(patsubst %,$(OBJ)/%,$(_LYNXBOT))
_LBH=client.h CommandHandler.h CustomCommandHandler.h EventManager.h\
     ExpressionParser.h GEReader.h Giveaway.h Moderator.h OpMap.h\
     OptionParser.h RSNList.h SelectionWheel.h SkillMap.h TimerManager.h\
     TwitchBot.h URLParser.h utils.h version.h
LBH=$(patsubst %,$(SRC)/%,$(_LBH))

_TW=oauth.o base64.o authenticator.o reader.o
TW=$(patsubst %,$(OBJ)/%,$(_TW))
_TWH=oauth.h base64.h authenticator.h reader.h
TWH=$(patsubst %,include/tw/%,$(_TWH))
TWD=$(LIBD)/tw

# lynxbot source
$(OBJ)/%.o: $(SRC)/%.cpp $(LBH)
	$(CXX) $(CXXFLAGS) -o $@ $<

# jsoncpp source
$(OBJ)/%.o: $(JSOND)/%.cpp $(JSONH)
	$(CXX) $(CXXFLAGS) -o $@ $<

# cpr source
$(OBJ)/%.o: $(CPRD)/%.cpp $(CPRH)
	$(CXX) $(CXXFLAGS) -o $@ $<

# tw source
$(OBJ)/%.o: $(TWD)/%.cpp $(TWH)
	$(CXX) $(CXXFLAGS) -o $@ $<

lynxbot: odir exec

# create directory for .o files
odir:
	@mkdir -p $(OBJ)

exec: $(CPR) $(JSONCPP) $(LYNXBOT) ${TW}
	$(CXX) -o $(PROGNAME) $(LIB) $^

.PHONY: clean

clean:
	rm -f $(PROGNAME) $(OBJ)/*.o
