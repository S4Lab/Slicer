#ifndef SLICE_ACTION_H
#define SLICE_ACTION_H

#include "Slice.h"
#include <clang/Basic/LangOptions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>

namespace clang {
class FrontendAction;
} // namespace clang
namespace llvm {
class raw_ostream;
} // namespace llvm

class SliceAction : public clang::ASTFrontendAction {
public:
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef) override {
    return std::make_unique<Slice>();
  }
};

llvm::cl::OptionCategory SliceCategory("Slice Options");

llvm::cl::extrahelp SliceCategoryHelp(R"(
  Gives full information about PDG and other stuff
)");

//static llvm::cl::opt<std::string> StoragePath("storage", llvm::cl::desc(R"(Stotage Path for Json Files)"), llvm::cl::init(""), llvm::cl::cat(SliceCategory));

struct SliceTool : public clang::tooling::FrontendActionFactory {
  std::unique_ptr<clang::FrontendAction> create() override { return std::make_unique<SliceAction>(); }
};

#endif // SLICE_ACTION_H
