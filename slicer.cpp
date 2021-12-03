#include "SliceAction.h"

auto main(int argc, const char *argv[]) -> int {
  using namespace clang::tooling;
  CommonOptionsParser OptionsParser(argc, argv, SliceCategory);
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());
  int stat = Tool.run(new SliceTool());
  // Here, Add another tool (e.g., Reformat, Slice, ...)
  // stat |= Tool.run(new SomethingElse());
  return stat;
}
