#ifndef DDG_H
#define DDG_H

#include "Atomic.h"

class DDGBlock {
  friend class DDG;
  AtomBlock *atom_block;
  DeclSet Defs, Uses;
  DDeclStmtMap Gen, Kill, In, Out, Preds, Succs;
  DBlockSet uniPreds, uniSuccs;
  unsigned FN, FP;

public:
  DDGBlock(AtomBlock *atom_block) : atom_block(atom_block), FN(0), FP(0) {}
  AtomBlock *getAtom() const { return atom_block; }
  const DBlockSet& getPreds() const { return uniPreds; }
  const DeclSet& getDefs() const { return Defs; }
  const DeclSet& getUses() const { return Uses; }
};

class DDG {
  AtomicCFG *atomic_cfg;
  clang::ASTContext *Ctx;
  const clang::FunctionDecl *CurrentFunction;
  DBlockVec ddg_blocks;
  DDeclStmtMap AllDefs, AllUses, VarDecls;
  std::map<AtomBlock *, DDGBlock *> reverseMap;

public:
  DDG(AtomicCFG *_cfg, clang::ASTContext *Ctx, const clang::FunctionDecl *Function)
      : atomic_cfg(_cfg), Ctx(Ctx), CurrentFunction(Function) {
    for (auto &&stmt : atomic_cfg->atom_blocks) {
      ddg_blocks.emplace_back(new DDGBlock(&stmt));
      reverseMap[&stmt] = ddg_blocks.back();
    }
    makeGraph();
  }
  ~DDG() {
    for (auto &&b : ddg_blocks) {
      delete b;
    }
  }
  void prettyPrint(std::ostream& ofile);
  const DBlockVec& getBlocks() { return ddg_blocks; }
  void countFalse();
  
private:
  void makeGraph();
  void updateUntilConverge();
  void createUseDefChain();
  void checkDependency(DDGBlock *DDGB);
  void checkRealDependency(DDGBlock *DDGB, clang::Stmt *S);
  void addDefUse(clang::DeclRefExpr *DRE, DDGBlock *DDGB, bool isdef);
  void traverseDataDep(DDGBlock *DB, clang::Decl *const &Var, DBlockSet &Truth, BlockSet &Visited);
  std::vector<clang::DeclRefExpr *> getDeclRefExprs(clang::Expr *E);
  std::vector<clang::Stmt *> getAllChildren(clang::Stmt *S);
  DDGBlock *getDBlock(AtomBlock *AB);
};
#endif // DDG_H
