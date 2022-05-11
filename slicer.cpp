#include "SliceAction.h"

auto main(int argc, const char *argv[]) -> int {
  using namespace clang::tooling;
  auto OptionsParser = CommonOptionsParser::create(argc, argv, SliceCategory);
  ClangTool Tool(OptionsParser.get().getCompilations(),
                 OptionsParser.get().getSourcePathList());
  int stat = Tool.run(new SliceTool());
  // Here, Add another tool (e.g., Reformat, Slice, ...)
  // stat |= Tool.run(new SomethingElse());
  return stat;
}
