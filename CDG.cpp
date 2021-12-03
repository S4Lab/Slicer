#include "CDG.h"

void CDG::makeGraph() {
  makeBlocks();
  makePDChildren();
  makePDTree();
  makePDF();
  countFalse();
}

void CDG::makeBlocks() {
  for (auto const &cb : cdg_blocks) {
    CBlockVec CBV;
    auto preds = cb->getAtom()->get_preds();
    // CBV.resize(preds.size());
    std::transform(preds.begin(), preds.end(), std::back_inserter(CBV),
                   [this](AtomBlock *AB) { return getCBlock(AB); });
    cb->setCFGPreds(CBV);
    cb->fathers.insert(ExBlock);
  }
}

void CDG::makePDChildren() {
  for (auto const &cb : cdg_blocks) {
    if (cb == ExBlock) {
      cb->allChild = cdg_blockSet;
      cb->allChild.erase(cb);
      cb->numChild = cb->allChild.size();
      continue;
    }
    auto children = cdg_blockSet;
    children.erase(cb);
    // CBlockSet Visited;
    // traverseRemovedDFS(ExBlock, cb, Visited);
    traverseRemoved(cb);
    auto Visited = cb->allChild;
    cb->allChild.clear();
    std::set_difference(children.begin(), children.end(),
                        Visited.begin(), Visited.end(),
                        std::inserter(cb->allChild, cb->allChild.begin()));
    std::for_each(cb->allChild.begin(), cb->allChild.end(),
                  [cb](CDGBlock *Child) { Child->fathers.insert(cb); });
    cb->numChild = cb->allChild.size();
  }
}

void CDG::makePDTree() {
  auto comp = [](CDGBlock *A, CDGBlock *B) {
    return A->getNumChild() > B->getNumChild();
  };
  ExBlock->ipdom = ExBlock; // TODO
  for (auto const &cb : cdg_blocks) {
    cb->ipdom = *cb->fathers.begin();
    for (auto const &father : cb->fathers)
      if (comp(cb->ipdom, father))
        cb->ipdom = father;
    cb->ipdom->pdPreds.insert(cb);
  }
  ExBlock->pdPreds.erase(ExBlock);
}

void CDG::makePDF() {
  std::queue<CDGBlock *> ToQ;
  std::stack<CDGBlock *> ToS;
  CDGBlock *curr;
  ToQ.push(ExBlock);
  while (!ToQ.empty()) {
    curr = ToQ.front();
    ToQ.pop();
    ToS.push(curr);
    for (auto const &pd : curr->pdPreds)
      ToQ.push(pd); 
  }
  while (!ToS.empty()) {
    curr = ToS.top();
    ToS.pop();
    assert (curr != nullptr);
    for (auto const &pred : curr->cfgPreds)
      if (pred->ipdom != curr)
        curr->uniPreds.insert(pred);
    for (auto const &child : curr->pdPreds)
      for (auto const &pdf : child->uniPreds)
        if (pdf == curr)
          continue;
        else if (curr->allChild.find(pdf) == curr->allChild.end())
          curr->uniPreds.insert(pdf);
  }
}

void CDG::countFalse() {
  CBlockSet truth, FNB, FPB;
  for (auto const &cb : cdg_blocks) {
    truth = {}, FNB = {}, FPB = {};
    for (auto const &pdf : cdg_blocks) {
      if (cb == pdf)
        continue;
      if (checkRealDep(cb, pdf))
        truth.insert(pdf);
    }
    std::set_difference(cb->uniPreds.begin(), cb->uniPreds.end(),
                        truth.begin(), truth.end(),
                        std::inserter(FPB, FPB.begin()));
    std::set_difference(truth.begin(), truth.end(),
                        cb->uniPreds.begin(), cb->uniPreds.end(),
                        std::inserter(FNB, FNB.begin()));
    cb->FP += FPB.size();
    cb->FN += FNB.size();
  }
}

bool CDG::checkRealDep(CDGBlock *Target, CDGBlock *Src) {
  AtomBlock *curr, *Mark = Target->atom_block;
  unsigned depend = 0, not_depend = 0;
  for (auto const &R : Src->atom_block->succs) {
    BlockSet Visited;
    std::queue<AtomBlock *> ToVisit;
    ToVisit.push(R);
    Visited.insert(R);
    while (!ToVisit.empty()) {
      curr = ToVisit.front();
      ToVisit.pop();
      if (Mark == curr)
        continue;
      for (auto const &b : curr->succs) {
        if (Visited.find(b) == Visited.end()) {
          Visited.insert(b);
          ToVisit.push(b);
        }
      }
    }
    if (Visited.find(ExBlock->atom_block) == Visited.end())
      depend++;
    else
      not_depend++;
  }
  if (depend != 0 && not_depend != 0)
    return true;
  else
    return false;
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

void CDG::prettyPrint(std::ostream& ofile) {
  auto pp = clang::PrintingPolicy(Ctx->getLangOpts());
  auto jStr = std::string("[\n");
  for (auto const &cb : cdg_blocks) {
    auto AB = cb->atom_block;
    std::string stmtStream;
    llvm::raw_string_ostream ross(stmtStream);
    if (AB->statement)
      AB->statement->printPretty(ross, nullptr, pp, 0U, "\n", Ctx);
    jStr += " {\n  \"BlockID\": " + std::to_string(AB->atom_id) + ",\n";
    jStr += "  \"FP\": " + std::to_string(cb->FP) + ",\n";
    jStr += "  \"FN\": " + std::to_string(cb->FN) + ",\n";
    jStr += "  \"Stmt\": " + AB->JsonFormat(llvm::StringRef(ross.str()), true) +
            ", \n";
    jStr += "  \"CDG Edges\": [\n";
    for (auto const &pred : cb->uniPreds)
      jStr += "   " + std::to_string(pred->atom_block->atom_id) + ",\n";
    if (!cb->uniPreds.empty())
      jStr.erase(jStr.end() - 2);
    jStr += "  ]\n },\n";
  }
  if (!cdg_blocks.empty())
    jStr.erase(jStr.end() - 2);
  jStr += "]\n";
  ofile << jStr;
}

CDGBlock *CDG::getCBlock(AtomBlock *AB) {
  auto it = std::find_if(cdg_blocks.begin(), cdg_blocks.end(),
                         [&, AB](CDGBlock *CB) { return CB->getAtom() == AB; });
  return *it;
}

void CDGBlock::dump() {
  llvm::errs() << atom_block->atom_id << "\t";
}