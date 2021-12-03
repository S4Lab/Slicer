#ifndef CDG_H
#define CDG_H

#include "Atomic.h"

class CDGBlock {
  friend class CDG;
  AtomBlock *atom_block;
  unsigned FP, FN;
  CDGBlock *ipdom;
  CBlockSet uniPreds, allChild, pdPreds, cfgPreds, fathers;
  unsigned numChild;

public:
  CDGBlock(AtomBlock *atom_block) : atom_block(atom_block), FP(0), FN(0) {}
  AtomBlock *getAtom() const { return atom_block; }
  const CBlockSet &getPreds() const { return uniPreds; }
  void setCFGPreds(CBlockVec &BV) {
    cfgPreds.clear();
    cfgPreds.insert(BV.begin(), BV.end());
  }
  void dump();
  unsigned getNumChild() { return numChild; }
};

class CDG {
  AtomicCFG *atomic_cfg;
  clang::ASTContext *Ctx;
  const clang::FunctionDecl *CurrentFunction;
  CBlockVec cdg_blocks;
  CBlockSet cdg_blockSet;
  CDGBlock *ExBlock;
  std::map<AtomBlock *, CDGBlock *> reverseMap;

public:
  CDG(AtomicCFG *_cfg, clang::ASTContext *Ctx,
      const clang::FunctionDecl *Function)
      : atomic_cfg(_cfg), Ctx(Ctx), CurrentFunction(Function) {
    for (auto &&stmt : atomic_cfg->atom_blocks) {
      auto *b = new CDGBlock(&stmt);
      cdg_blocks.emplace_back(b);
      reverseMap[&stmt] = b;
      cdg_blockSet.insert(b);
      if (b->getAtom()->get_succs().empty())
        ExBlock = b;
    }
    makeGraph();
  }
  ~CDG() {
    for (auto &&b : cdg_blocks) {
      delete b;
    }
  }
  void prettyPrint(std::ostream& ofile);
  void countFalse();
  const CBlockVec &getBlocks() { return cdg_blocks; }

private:
  void makeGraph();
  void makePDChildren();
  void makePDTree();
  void makePDF();
  void makeBlocks();
  bool checkRealDep(CDGBlock *Target, CDGBlock *Src);
  void traverseRemoved(CDGBlock *CB);
  void traverseRemovedDFS(CDGBlock *r, CDGBlock *&CB, CBlockSet &Visited);
  CDGBlock *getCBlock(AtomBlock *AB);
};
#endif // CDG_H
