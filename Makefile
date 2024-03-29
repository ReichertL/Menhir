.SUFFIXES:
%:: SCCS/s.%
%:: RCS/%
%:: RCS/%,v
%:: %,v
%:: s.%


# definitions
SDIR=src
TDIR=test
HDIR=benchmark
IDIR=include
ODIR=obj
LDIR=lib
BDIR=bin

LDFLAGS=-L $(LDIR) #-Wl,--verbose #-L /usr/include/openssl #-L /usr/local/opt/openssl/lib
LDLIBS=-l boost_system -l boost_program_options -l boost_filesystem   -lstdc++fs -l pathoram -l ssl -l crypto -l pthread -l sqlite3 #-l openmp #-l rpc # libs for main code
LDTESTLIBS=-l gtest #-l benchmark # libs for tests and benchmarks
INCLUDES=-I $(IDIR) -I $(IDIR)/DOSM -I $(LDIR)/include 
CPPFLAGS=-std=c++17 
CPPFLAGS+= -Wall -Wno-unknown-pragmas -fPIC -O0 -g -DUSE_REDIS=false -DUSE_AEROSPIKE=false  -DSERVERLESS -fopenmp 
CPPFLAGS+= #-DNDEBUG #-Wfatal-errors 
CC=g++
#INCLUDES+=  -cxx-isystem /usr/include/ -cxx-isystem /usr/include/c++/4.8/ -cxx-isystem /usr/include/x86_64-linux-gnu/c++/4.8/#/usr/include/c++/4.8/x86_64-pc-linux-gnu/ #-I/usr/include/c++/4.8.5 #-I/usr/include/x86_64-linux-gnu/c++/4.8/bits
#CPPFLAGS+= -stdlib=libc++ -gfull  #-L/usr/include/c++/4.8.5 # -L/usr/include/x86_64-linux-gnu/c++/4.8/bits
RM=rm -rf

LIB_ORAM_DIR=../path-oram

LIBNAME=MENHIR

# if you follow the convention that for each class CLASS you have a header in
# $(IDIR)/CLASS.hpp, a code in $(SDIR)/CLASS.cpp and a test in $(TDIR)/test-CLASS.cpp,
# then the rest will magically work - it will compile each class and test and will run the tests.
# CLASS does not even have to be a class in C++.

ENTITIES = globals utility database_type struct_querying output_utility state_table  server_utility
ENTITIES +=  get_data_and_queries parse_args prepare_dosm  querying  
ENTITIES += dp_query_functions volume_sanitizer_utility globals_osm osm_interface avl_loadtree  avl_multiset avl_treenode  linear_db 

H_FILE_ENTITIES= definitions.h  struct_volume_sanitizer.hpp struct_error.hpp
_DEPS =  $(H_FILE_ENTITIES) $(addsuffix .hpp, $(ENTITIES))
DEPS = $(patsubst %, $(IDIR)/%, $(_DEPS))

_OBJ = $(addsuffix .o, $(ENTITIES))
OBJ = $(patsubst %, $(ODIR)/%, $(_OBJ))

TARGETS = main
TARGETBIN = $(addprefix $(BDIR)/, $(TARGETS))

TESTS = oram retrival
TESTBIN = $(addprefix $(BDIR)/test-, $(TESTS))
JUNITS= $(foreach test, $(TESTS), bin/test-$(test)?--gtest_output=xml:junit-$(test).xml)

BENCHMARKS = oram
BENCHMARKSBIN = $(addprefix $(BDIR)/benchmark-, $(BENCHMARKS))

#INTEGRATION =
#INTEGRATIONBIN = $(addprefix $(BDIR)/test-, $(INTEGRATION))

ENTRYPOINTCC = $(CC) -o $@ $^ $(CPPFLAGS) $(INCLUDES) $(LDFLAGS) $(LDLIBS)

# flags-setting commands

all: shared docs

binaries: LDLIBS += $(LDTESTLIBS)
binaries: $(TARGETBIN) $(TESTBIN) $(INTEGRATIONBIN) $(BENCHMARKSBIN)
cleandebug: clean debug

debug: CPPFLAGS += -g
debug: CPPFLAGS := $(filter-out -DNDEBUG,$(CPPFLAGS))
debug: binaries

profile: CPPFLAGS += -fprofile-arcs -ftest-coverage -fPIC -O0
profile: clean run-tests-junit

.SECONDEXPANSION:
$(info  $(OBJ) $$(subst $$(BDIR), $(SDIR), $$@).cpp)


# executables

.SECONDEXPANSION:
$(TARGETBIN): $(OBJ) $$(subst $$(BDIR), $(SDIR), $$@).cpp
	$(ENTRYPOINTCC)

.SECONDEXPANSION:
$(TESTBIN): $(OBJ) $$(subst $$(BDIR), $(TDIR), $$@).cpp
	$(ENTRYPOINTCC)

.SECONDEXPANSION:
$(BENCHMARKSBIN): $(OBJ) $$(subst $$(BDIR), $(HDIR), $$@).cpp
	$(ENTRYPOINTCC)

.SECONDEXPANSION:
$(INTEGRATIONBIN): $(OBJ) $$(subst $$(BDIR), $(TDIR), $$@).cpp
	$(ENTRYPOINTCC)

shared: CPPFLAGS += -DSHARED
shared: $(OBJ)
	$(CC) -shared $(LDLIBS) $(LDFLAGS) -o $(BDIR)/lib$(LIBNAME).so $(OBJ)

# programs

main: LDLIBS += -l rpc
main: bin/main

# objects
$(info $(SDIR)/%.cpp $(DEPS))

$(ODIR)/%.o: $(SDIR)/%.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CPPFLAGS) $(INCLUDES)

# commands

# some Linuxes do not respect -L flag during execution, so we can modify the system LD path
ldconfig:
	@echo $(shell pwd)/bin > /etc/ld.so.conf.d/$(LIBNAME)-dev.conf
	@ldconfig

# will compile the test against the library (not objects and sources) and will run it
#run-shared-lib: shared#
#	$(CC) -o $(BDIR)/test-shared-lib $(TDIR)/test-shared-lib.cpp -I $(IDIR) -L $(BDIR) -l $(LIBNAME) $(CPPFLAGS)
#	$(BDIR)/test-shared-lib

#run-tests-junit: CPPFLAGS += -DTESTING
#run-tests-junit: $(TESTBIN)
#	$(subst ?, ,$(addsuffix &&, $(JUNITS))) echo Tests passed!

tests: CPPFLAGS += -DTESTING -DSERVERLESS -DGTEST_USES_POSIX_RE=1
tests: LDLIBS += $(LDTESTLIBS)
tests: $(TESTBIN)

run-tests: CPPFLAGS += -DTESTING -DSERVERLESS -DGTEST_USES_POSIX_RE=1
run-tests: LDLIBS += $(LDTESTLIBS)
run-tests: $(TESTBIN)
	$(addsuffix &&, $(TESTBIN)) echo Tests passed!

run-benchmarks: CPPFLAGS += -DSERVERLESS
run-benchmarks: $(BENCHMARKSBIN)
	$(addsuffix &&, $(BENCHMARKSBIN)) echo Benchmarks completed!

#run-integration: CPPFLAGS += -DTESTING
#run-integration: $(INTEGRATIONBIN)
#	$(addsuffix &&, $(INTEGRATIONBIN)) echo Tests passed!

coverage: profile
	gcovr -r . $(addprefix -f $(SDIR)/, $(addsuffix .cpp, $(ENTITIES))) --exclude-unreachable-branches
	mkdir -p coverage-html/
	gcovr -r . --html --html-details -o coverage-html/index.html
	gcovr -r . -x -o cobertura.xml

copy-libs-dev: LIB_ORAM_DIR=./pathoram
copy-libs-dev: copy-libs

copy-libs:
	@rm -rf $(LDIR)/*
	@mkdir -p $(LDIR)/include/path-oram
	@cp $(LIB_ORAM_DIR)/bin/libpathoram.so $(LDIR)
	@cp $(LIB_ORAM_DIR)/include/* $(LDIR)/include/path-oram
	@cp $(LDIR)/lib*.so $(BDIR)

install-libs:
	cp $(LDIR)/lib*.so /usr/lib/

docs: clean-docs
	doxygen ../Doxyfile

clean: clean-docs clean-binaries
	$(RM) $(ODIR)/* *~ *.dSYM *.gcov *.gcda *.gcno *.bin coverage-html junit-*.xml cobertura.xml

clean-docs:
	$(RM) ../docs

clean-binaries:
	$(RM) $(BDIR)/*

# phony

.PHONY: docs clean clean-docs clean-binaries coverage
.PHONY: profile debug cleandebug
.PHONY: binaries all shared
.PHONY: run-tests run-integration run-benchmarks run-shared-lib run-tests-junit
