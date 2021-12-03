#include "CDG.h"

void CDG::makeGraph() {
  makeBlocks();
  makePDChildren();
  makePDTree();
  makePDF();
}

void CDG::makePDChildren() {
  for (auto const &cb : cdg_blocks) {
    if (cb == ExBlock) {
      cb->allChild = cdg_blockSet;
      cb->allChild.erase(cb);
      continue;
    }
    auto children = cdg_blockSet;
    children.erase(cb);
    // CBlockSet Visited;
    // traverseRemovedDFS(ExBlock, cb, Visited);
    traverseRemoved(cb);
    auto Visited = cb->allChild;
    cb->allChild.clear();
    std::set_difference(children.begin(), children.end(), Visited.begin(),
                        Visited.end(),
                        std::inserter(cb->allChild, cb->allChild.begin()));
    cb->numChild = cb->allChild.size();
  }
}

void CDG::makePDTree() {
  auto comp = [](CDGBlock *A, CDGBlock *B) {
    return A->getNumChild() > B->getNumChild();
  };
  std::queue<CDGBlock *> ToVisit;
  CDGBlock *curr;
  CBlockSet Visited;
  ToVisit.push(ExBlock);
  Visited.insert(ExBlock);
  ExBlock->ipdom = ExBlock;
  while (!ToVisit.empty()) {
    curr = ToVisit.front();
    ToVisit.pop();
    auto children = CBlockVec(curr->allChild.begin(), curr->allChild.end());
    while (children.size() != 0) {
      CBlockVec temp;
      std::sort(children.begin(), children.end(), comp);
      auto child = children.front();
      curr->pdPreds.insert(child);
      child->ipdom = curr;
      if (Visited.find(child) == Visited.end()) {
        ToVisit.push(child);
        Visited.insert(child);
      }
      children.erase(children.begin());
      std::set_difference(children.begin(), children.end(),
                          child->allChild.begin(), child->allChild.end(),
                          std::inserter(temp, temp.begin()));
      children = temp;
    }
  } 
}

void CDG::makePDF() {
  std::queue<CDGBlock *> ToVisit;
  CBlockSet Visited;
  CDGBlock *curr;
  for (auto const &cb : cdg_blocks)
    if (cb->pdPreds.empty()) {
      ToVisit.push(cb);
      Visited.insert(cb);
    }
  while (!ToVisit.empty()) {
    curr = ToVisit.front();
    ToVisit.pop();
    assert (curr != nullptr);
    if (Visited.find(curr->ipdom) == Visited.end()) {
      ToVisit.push(curr->ipdom);
      Visited.insert(curr->ipdom);
    }
    for (auto const &pred : curr->cfgPreds)
      if (pred->ipdom != curr)
        curr->uniPreds.insert(pred);
    for (auto const &child : curr->allChild)
      for (auto const &pdf : child->uniPreds)
        if (curr->allChild.find(pdf) == curr->allChild.end())
          curr->uniPreds.insert(pdf);
  }
}

void CDG::makeBlocks() {
  for (auto const &cb : cdg_blocks) {
    CBlockVec CBV;
    auto preds = cb->getAtom()->get_preds();
    // CBV.resize(preds.size());
    std::transform(preds.begin(), preds.end(), std::back_inserter(CBV),
                   [this](AtomBlock *AB) { return getCBlock(AB); });
    cb->setCFGPreds(CBV);
  }
}

void CDG::traverseRemovedDFS(CDGBlock *r, CDGBlock *&CB, CBlockSet &Visited) {
  Visited.insert(r);
  for (auto const &b : r->cfgPreds) {
    if(b != CB && (Visited.find(b) == Visited.end()))
      traverseRemovedDFS(b, CB, Visited);
  }
  CB->allChild = Visited;
}

void CDG::traverseRemoved(CDGBlock *CB) {
  std::queue<CDGBlock *> ToVisit;
  CBlockSet Visited;
  CDGBlock *curr;
  ToVisit.push(ExBlock);
  Visited.insert(ExBlock);
  while (!ToVisit.empty()) {
    curr = ToVisit.front();
    ToVisit.pop();
    for (auto const &b : curr->cfgPreds) {
      if (b != CB && (Visited.find(b) == Visited.end())) {
        Visited.insert(b);
        ToVisit.push(b);
      }
    }
  }
  CB->allChild = Visited;
}

CDGBlock *CDG::getCBlock(AtomBlock *AB) {
  auto it = std::find_if(cdg_blocks.begin(), cdg_blocks.end(),
                         [&, AB](CDGBlock *CB) { return CB->getAtom() == AB; });
  return *it;
}
