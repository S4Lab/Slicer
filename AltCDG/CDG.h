#ifndef CDG_H
#define CDG_H

#include "Atomic.h"

class CDGBlock {
  friend class CDG;
  AtomBlock *atom_block;
  CDGBlock *ipdom;
  CBlockSet uniPreds, allChild, pdPreds, cfgPreds;
  unsigned numChild;

public:
  CDGBlock(AtomBlock *atom_block) : atom_block(atom_block) {}
  AtomBlock *getAtom() const { return atom_block; }
  const CBlockSet &getPreds() const { return uniPreds; }
  void setCFGPreds(CBlockVec &BV) {
    cfgPreds.clear();
    cfgPreds.insert(BV.begin(), BV.end());
  }
  unsigned getNumChild() { return numChild; }
};

class CDG {
  AtomicCFG *atomic_cfg;
  clang::ASTContext *Ctx;
  const clang::FunctionDecl *CurrentFunction;
  CBlockVec cdg_blocks;
  CBlockSet cdg_blockSet;
  CDGBlock *ExBlock;

public:
  CDG(AtomicCFG *_cfg, clang::ASTContext *Ctx,
      const clang::FunctionDecl *Function)
      : atomic_cfg(_cfg), Ctx(Ctx), CurrentFunction(Function) {
    for (auto &&stmt : atomic_cfg->atom_blocks) {
      auto *b = new CDGBlock(&stmt);
      cdg_blocks.emplace_back(b);
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
  void prettyPrint();
  const CBlockVec &getBlocks() { return cdg_blocks; }

private:
  void makeGraph();
  void makePDChildren();
  void makePDTree();
  void makePDF();
  void makeBlocks();
  void traverseRemoved(CDGBlock *CB);
  void traverseRemovedDFS(CDGBlock *r, CDGBlock *&CB, CBlockSet &Visited);
  CDGBlock *getCBlock(AtomBlock *AB);
};
#endif // CDG_H
