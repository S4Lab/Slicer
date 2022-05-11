SLICE = slicer
HEADERS = -isystem `llvm-config-14 --includedir`
PROJ_HEADERS = Atomic.h PDG.h DDG.h CDG.h Slice.h SliceAction.h SourceManager.h
PROJ_SRC = slicer.cpp Slice.cpp PDG.cpp DDG.cpp CDG.cpp Atomic.cpp SourceManager.cpp
WARNINGS = -Wall -Wextra -pedantic -Wno-unused-parameter
CXXFLAGS = $(WARNINGS) -std=c++17 -fno-rtti -O3 -Os -fno-exceptions
LDFLAGS = `llvm-config-14 --ldflags`

CLANG_LIBS = \
	-lclangFrontendTool \
	-lclangRewriteFrontend \
	-lclangDynamicASTMatchers \
	-lclangToolingCore \
	-lclangTooling \
	-lclangFrontend \
	-lclangASTMatchers \
	-lclangParse \
	-lclangDriver \
	-lclangSerialization \
	-lclangRewrite \
	-lclangSema \
	-lclangEdit \
	-lclangAnalysis \
	-lclangAST \
	-lclangLex \
	-lclangBasic

LIBS = $(CLANG_LIBS) `llvm-config-14 --libs --system-libs`

all: slicer

.phony: clean
.phony: run

clean:
	rm $(SLICE) || echo -n ""

slicer:$(PROJ_SRC) $(PROJ_HEADERS)
	$(CXX) $(HEADERS) $(LDFLAGS) $(CXXFLAGS) $(PROJ_SRC) $(LIBS) -o $(SLICE)
