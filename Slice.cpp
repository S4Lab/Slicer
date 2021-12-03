#include "Slice.h"

void MatchHandler::run(const MatchResult &Result) {
  const auto *Function = Result.Nodes.getNodeAs<clang::FunctionDecl>("fn");
  auto CFG = clang::CFG::buildCFG(Function, Function->getBody(), Result.Context,
                                  clang::CFG::BuildOptions());
  if (!CFG)
    return;
  auto file_name = std::string(Result.SourceManager->getFilename(Function->getEndLoc()));
  auto function_name = Function->getNameAsString();
  if (file_list.find(file_name) == file_list.end()) {
    file_list[file_name] = new std::ofstream(file_name + ".json");
    
    *file_list[file_name] << "[";
    if (!prev_file.empty()) {
      auto temp = prev_str.str();
      temp.erase(temp.end() - 1);
      *file_list[prev_file] << temp << "]\n";
    }
  } else {
    *file_list[file_name] << prev_str.str();
  }
  prev_file = file_name;
  auto atomic = new AtomicCFG(CFG, Result.Context, Function);
  auto cdg = new CDG(atomic, Result.Context, Function);
  auto ddg = new DDG(atomic, Result.Context, Function);
  auto pdg = new PDG(atomic, ddg, cdg, Result.Context, Function);
  prev_str.str(std::string());
  pdg->prettyPrint(prev_str);
  pdg->prettyCDPrint(file_name + "__" + function_name);
  delete pdg;
  delete ddg;
  delete cdg;
  delete atomic;
}

MatchHandler::~MatchHandler() {
  if (!prev_str.str().empty()) {
    auto temp = prev_str.str();
    temp.erase(temp.end() - 1);
    *file_list[prev_file] << temp << "]\n";
  }
  for (auto const &f : file_list)
    f.second->close();
}