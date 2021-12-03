#ifndef PDG_H
#define PDG_H

#include "Atomic.h"
#include "CDG.h"
#include "DDG.h"
#include "SourceManager.h"

using DeclIdx = std::pair<const clang::DeclStmt *, unsigned>;
using DataAtomMap = std::map<AtomBlock*, DDGBlock*>;

class PDG {
  AtomicCFG *acfg;
  DDG *ddg;
  CDG *cdg;
  clang::ASTContext *Ctx;
  const clang::FunctionDecl *CurrentFunction;
  BlockSet pdg_blocks;
  std::map<AtomBlock *, DeclIdx> realSynth;
  DataAtomMap DAM;

public:
  PDG(AtomicCFG *acfg, DDG *ddg, CDG *cdg, clang::ASTContext *Ctx,
      const clang::FunctionDecl *Function)
      : acfg(acfg), ddg(ddg), cdg(cdg), Ctx(Ctx), CurrentFunction(Function) {
    makeGraph();
  }
  void prettyPrint(std::stringstream& outs);
  void prettyCDPrint(std::string base_name);
  void makeStmtStr(AtomBlock *AB, std::string &jStr);
  DeclIdx findDecl(clang::DeclStmt *DS);
  const BlockSet &getBlocks() { return pdg_blocks; }

private:
  void makeGraph();
};

#endif // PDG_H