# lynxbot makefile

PROGNAME=lynxbot

INC=-I./include

LIB=-lcurl -lpthread -lcrypto

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
	OFLAGS=-s
endif

_CPR=auth.o cookies.o cprtypes.o digest.o error.o multipart.o parameters.o\
    payload.o proxies.o session.o util.o
CPR=$(patsubst %,$(OBJ)/cpr_%,$(_CPR))
_CPRH=api.h auth.h body.h cookies.h cpr.h cprtypes.h curlholder.h defines.h\
     digest.h error.h multipart.h parameters.h payload.h proxies.h response.h\
     session.h timeout.h util.h
CPRH=$(patsubst %,include/cpr/%,$(_CPRH))
CPRD=$(LIBD)/cpr

JSONCPP=$(OBJ)/js_jsoncpp.o
_JSONH=json.h json-forwards.h
JSONH=$(patsubst %,include/json/%,$(_JSONH))
JSOND=$(LIBD)/json

_LYNXBOT=main.o config.o client.o TwitchBot.o Moderator.o\
	URLParser.o CommandHandler.o CustomCommandHandler.o TimerManager.o\
	GEReader.o ExpressionParser.o OpMap.o OptionParser.o SelectionWheel.o\
	EventManager.o Giveaway.o skills.o RSNList.o
LYNXBOT=$(patsubst %,$(OBJ)/%,$(_LYNXBOT))
_LBH=client.h CommandHandler.h config.h EventManager.h ExpressionParser.h\
     GEReader.h Giveaway.h Moderator.h OpMap.h OptionParser.h RSNList.h\
     SelectionWheel.h skills.h TimerManager.h TwitchBot.h URLParser.h\
     version.h
LBH=$(patsubst %,$(SRC)/%,$(_LBH))

_COMMANDS=about.o active.o addcom.o addrec.o calc.o cgrep.o cmdinfo.o cml.o\
	  count.o delcom.o delrec.o duck.o editcom.o ehp.o eightball.o ge.o\
	  help.o level.o manual.o permit.o rsn.o setgiv.o setrec.o showrec.o\
	  status.o strawpoll.o submit.o uptime.o wheel.o whitelist.o
COMMANDS=$(patsubst %, $(OBJ)/cmd/%,$(_COMMANDS))
_CMDH=command.h
CMDH=$(patsubst %, $(SRC)/cmd/%,$(_CMDH))

_LIBS=utils.o
LIBS=$(patsubst %,$(OBJ)/%,$(_LIBS))
_LIBH=utils.h
LIBH=$(patsubst %,include/%,$(_LIBH))

_TW=oauth.o base64.o authenticator.o reader.o
TW=$(patsubst %,$(OBJ)/tw_%,$(_TW))
_TWH=oauth.h base64.h authenticator.h reader.h
TWH=$(patsubst %,include/tw/%,$(_TWH))
TWD=$(LIBD)/tw

# lynxbot source
$(OBJ)/%.o: $(SRC)/%.cpp $(LBH) $(LIBH)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(OBJ)/%.o: $(LIBD)/%.cpp $(LIBH)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(OBJ)/cmd/%.o: $(SRC)/cmd/%.cpp $(LBH) $(CMDH)
	$(CXX) $(CXXFLAGS) -o $@ $<

# jsoncpp source
$(OBJ)/js_%.o: $(JSOND)/%.cpp $(JSONH)
	$(CXX) $(CXXFLAGS) -o $@ $<

# cpr source
$(OBJ)/cpr_%.o: $(CPRD)/%.cpp $(CPRH)
	$(CXX) $(CXXFLAGS) -o $@ $<

# tw source
$(OBJ)/tw_%.o: $(TWD)/%.cpp $(TWH)
	$(CXX) $(CXXFLAGS) -o $@ $<

lynxbot: odir exec

# create directory for .o files
odir:
	@mkdir -p $(OBJ)/cmd

exec: $(CPR) $(JSONCPP) $(LYNXBOT) $(LIBS) $(COMMANDS) $(TW)
	$(CXX) ${OFLAGS} -o $(PROGNAME) $(LIB) $^

.PHONY: clean

clean:
	rm -f $(PROGNAME) $(OBJ)/*.o
