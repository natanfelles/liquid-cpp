ODIR=obj
SDIR=src
BDIR=bin
TDIR=t
LDIR=logs
CXX=g++
CC=gcc
CFLAGS=-Wall -fexceptions -fPIC -g
CXXFLAGS=$(CFLAGS) -std=c++17
LDFLAGS=
AR=ar
SOURCES=$(wildcard $(SDIR)/*.cpp) $(wildcard $(SDIR)/*.c) $(wildcard $(TDIR)/*.cpp)
TESTSOURCES=$(wildcard $(TDIR)/*.cpp)
OBJECTS=$(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(patsubst $(SDIR)/%,$(ODIR)/%,$(SOURCES))))
EXECUTABLESOURCES=$(SDIR)/main.cpp
EXECUTABLEOBJECTS=$(ODIR)/main.o
NONLIBRARYSOURCES=$(EXECUTABLESOURCES) $(TESTSOURCES)
LIBRARYSOURCES=$(filter-out $(NONLIBRARYSOURCES),$(SOURCES))
LIBRARYOBJECTS=$(patsubst %.cpp,%.o,$(patsubst %.c,%.o,$(patsubst $(SDIR)/%,$(ODIR)/%,$(LIBRARYSOURCES))))
LIBRARY = $(BDIR)/libliquid.a
EXECUTABLE := $(BDIR)/liquid
DEPENDS=$(wildcard $(ODIR)/*.d)

all: $(EXECUTABLE)

$(BDIR)/01sanity: $(LIBRARY) $(ODIR)/01sanity.o
	$(CXX) $(ODIR)/01sanity.o -L$(BDIR) -lliquid -lgtest -lpthread -o $(BDIR)/01sanity $(LDFLAGS)

test: $(BDIR)/01sanity

$(EXECUTABLE): $(LIBRARY) $(EXECUTABLEOBJECTS)
	$(CXX) -L$(BDIR) -lliquid $(EXECUTABLEOBJECTS) -o $(EXECUTABLE) $(LDFLAGS)

$(LIBRARY): $(LIBRARYOBJECTS)
	$(AR) -r -s $(LIBRARY) $(LIBRARYOBJECTS)

$(ODIR)/%.d: $(SDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -MM $< -MMD -MP -MT '$(patsubst %.d,%.o,$@)' -MF $@

$(ODIR)/%.d: $(TDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -MM $< -MMD -MP -MT '$(patsubst %.d,%.o,$@)' -MF $@

$(ODIR)/%.o: $(SDIR)/%.cpp $(ODIR)/%.d
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(ODIR)/%.o: $(TDIR)/%.cpp $(ODIR)/%.d
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(ODIR)/%.o: $(SDIR)/%.d
	$(CC) $(CFLAGS) -M $< -MMD -MP -MF $@

$(ODIR)/%.o: $(SDIR)/%.c $(ODIR)/%.d
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(ODIR)/%.d

library: $(LIBRARY)

directories: $(ODIR) $(BDIR) $(LDIR) $(TDIR)

executable: $(EXECUTABLE)
cleanexecutable: clean
cleantest: clean

$(LDIR):
	mkdir -p $(LDIR)
$(ODIR):
	mkdir -p $(ODIR)
$(BDIR):
	mkdir -p $(BDIR)
$(TDIR):
	mkdir -p $(TDIR)

-include $(DEPENDS)

testdir:
	@echo $(DEPENDS)

clean: directories
	rm -f $(ODIR)/*.o $(ODIR)/*.d $(LIBRARY)