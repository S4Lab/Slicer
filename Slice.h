#ifndef SLICE_H
#define SLICE_H

#include "Atomic.h"
#include "DDG.h"
#include "CDG.h"
#include "PDG.h"

#include <clang/Analysis/CFG.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace clang {
class ASTContext;
class ASTConsumer;
} // namespace clang

class MatchHandler : public clang::ast_matchers::MatchFinder::MatchCallback {
 public:
  using MatchResult = clang::ast_matchers::MatchFinder::MatchResult;
  MatchHandler(){}
  std::map<std::string, std::ofstream *> file_list;
  std::string prev_file;
  std::stringstream prev_str;
  void run(const MatchResult& Result);
  ~MatchHandler();
};

class Slice : public clang::ASTConsumer {

public:
  explicit Slice() : Handler() {
    using namespace clang::ast_matchers;
    const auto Matcher = functionDecl(isExpansionInMainFile(/*TODO isDefinition()*/)).bind("fn");
    Finder.addMatcher(Matcher, &Handler);
  }

  void HandleTranslationUnit(clang::ASTContext& Context) {
    Finder.matchAST(Context);
  }

 private:
  MatchHandler Handler;
  clang::ast_matchers::MatchFinder Finder;
};


#endif // ELIMINATE_H
