#include "DDG.h"

std::vector<clang::Stmt *> DDG::getAllChildren(clang::Stmt *S) {
  std::queue<clang::Stmt *> ToVisit;
  std::vector<clang::Stmt *> AllChildren;
  ToVisit.push(S);
  while (!ToVisit.empty()) {
    auto C = ToVisit.front();
    ToVisit.pop();
    AllChildren.emplace_back(C);
    for (auto const &Child : C->children()) {
      if (Child != NULL)
        ToVisit.push(Child);
    }
  }
  return AllChildren;
}

std::vector<clang::DeclRefExpr *> DDG::getDeclRefExprs(clang::Expr *E) {
  std::vector<clang::DeclRefExpr *> result;
  std::vector<clang::Stmt *> Children = getAllChildren(E);
  for (auto const &S : Children) {
    if (!S)
      continue;
    if (clang::DeclRefExpr *DRE = llvm::dyn_cast<clang::DeclRefExpr>(S))
      result.emplace_back(DRE);
  }
  return result;
}

void DDG::addDefUse(clang::DeclRefExpr *DRE, DDGBlock *DDGB, bool isdef) {
  if (clang::VarDecl *VD = llvm::dyn_cast<clang::VarDecl>(DRE->getDecl())) {
    // if (llvm::dyn_cast_or_null<clang::ConstantArrayType>(
    //         VD->getType().getTypePtr()))
    //   return;
    if (VD->isLocalVarDeclOrParm() || VD->isStaticLocal()) {
      auto decl = DRE->getDecl();
      if (isdef) {
        DDGB->Defs.insert(decl);
        if (AllDefs.find(decl) != AllDefs.end())
          AllDefs[decl].insert(DDGB);
        else
          AllDefs[decl] = {DDGB};
      } else {
        DDGB->Uses.insert(decl);
        if (AllUses.find(decl) != AllUses.end())
          AllUses[decl].insert(DDGB);
        else
          AllUses[decl] = {DDGB};
      }
    }
  }
}

void DDG::checkDependency(DDGBlock *DDGB) {
  auto S = DDGB->atom_block->statement;
  if (!S)
    return;
  if (auto *P = llvm::dyn_cast<clang::ParenExpr>(S))
    checkRealDependency(DDGB, P->getSubExpr());
  else
    checkRealDependency(DDGB, S);
}

void DDG::checkRealDependency(DDGBlock *DDGB, clang::Stmt *S) {
  if (clang::BinaryOperator *BO = llvm::dyn_cast<clang::BinaryOperator>(S)) {
    if (BO->isCompoundAssignmentOp() || BO->isShiftAssignOp()) {
      for (auto C : getDeclRefExprs(BO->getLHS())) {
        addDefUse(C, DDGB, true);
        addDefUse(C, DDGB, false);
      }
      for (auto C : getDeclRefExprs(BO->getRHS()))
        addDefUse(C, DDGB, false);
    } else if (BO->isAssignmentOp()) {
      for (auto C : getDeclRefExprs(BO->getLHS()))
        addDefUse(C, DDGB, true);
      for (auto C : getDeclRefExprs(BO->getRHS()))
        addDefUse(C, DDGB, false);
    } else {
      for (auto C : getDeclRefExprs(BO->getLHS()))
        addDefUse(C, DDGB, false);
      for (auto C : getDeclRefExprs(BO->getRHS()))
        addDefUse(C, DDGB, false);
    }
  } else if (clang::UnaryOperator *UO =
                 llvm::dyn_cast<clang::UnaryOperator>(S)) {
    switch (UO->getOpcode()) {
    case clang::UO_PostInc:
    case clang::UO_PostDec:
    case clang::UO_PreInc:
    case clang::UO_PreDec:
      for (auto C : getDeclRefExprs(UO->getSubExpr())) {
        addDefUse(C, DDGB, true);
        addDefUse(C, DDGB, false);
      }
      break;
    case clang::UO_AddrOf:
      for (auto C : getDeclRefExprs(UO->getSubExpr()))
        addDefUse(C, DDGB, true);
      break;
    case clang::UO_Plus:
    case clang::UO_Minus:
    case clang::UO_Not:
    case clang::UO_Deref:
    case clang::UO_LNot:
      for (auto C : getDeclRefExprs(UO->getSubExpr()))
        addDefUse(C, DDGB, false);
      break;
    case clang::UO_Real:
    case clang::UO_Imag:
    case clang::UO_Extension:
    case clang::UO_Coawait:
      break;
    }
  } else if (clang::DeclStmt *DS = llvm::dyn_cast<clang::DeclStmt>(S)) {
    for (auto D : DS->decls())
      if (clang::VarDecl *VD = llvm::dyn_cast<clang::VarDecl>(D)) {
        DDGB->Defs.insert(VD);
        assert((VarDecls.find(VD) == VarDecls.end()) && "Redeclare?? Shame!");
        VarDecls[VD].insert(DDGB);
        AllDefs[VD].insert(DDGB);
        if (VD->hasInit())
          for (auto C : getDeclRefExprs(VD->getInit()))
            addDefUse(C, DDGB, false);
      }
  } else if (clang::CallExpr *CE = llvm::dyn_cast<clang::CallExpr>(S)) {
    for (unsigned I = 0; I < CE->getNumArgs(); I++)
      for (auto C : getDeclRefExprs(CE->getArg(I)))
        addDefUse(C, DDGB, false);
    // } else if (clang::ReturnStmt *RS = llvm::dyn_cast<clang::ReturnStmt>(S))
    // {
    //   if (!CurrentFunction->getReturnType().getTypePtr()->isVoidType())
    //     for (auto C : getDeclRefExprs(RS->getRetValue()))
    //       addDefUse(C, DDGB, false);
  }
  // for (auto P : CurrentFunction->parameters()) {
  //   DDGB->Defs.insert(P);
  //   AllDefs.insert(P);
  // }
}

void DDG::updateUntilConverge() {
  unsigned flag = 1;
  while (flag != 0) {
    flag = 0;
    for (auto const &db : ddg_blocks) {
      DDeclStmtMap oldOut;
      for (auto const &D : db->Out)
        oldOut.insert(D);
      for (auto const &pred : db->atom_block->preds) {
        auto it = std::find_if(
            ddg_blocks.begin(), ddg_blocks.end(),
            [pred](const DDGBlock *DB) { return DB->atom_block == pred; });
        for (auto const &P : (*it)->Out)
          db->In[P.first].insert(P.second.begin(), P.second.end());
        // TODO:: SegFault
      }
      DeclSet ds;
      for (auto const &D : db->In)
        ds.insert(D.first);
      for (auto const &D : db->Gen)
        ds.insert(D.first);
      DDeclStmtMap minus;
      for (auto const &P : db->In) {
        auto D = P.first;
        std::set_difference(db->In[D].begin(), db->In[D].end(),
                            db->Kill[D].begin(), db->Kill[D].end(),
                            std::inserter(minus[D], minus[D].begin()));
      }
      for (auto const &D : ds) {
        db->Out[D] = {};
        std::set_union(minus[D].begin(), minus[D].end(), db->Gen[D].begin(),
                       db->Gen[D].end(),
                       std::inserter(db->Out[D], db->Out[D].begin()));
        if (db->Out[D] != oldOut[D])
          flag++;
      }
    }
  }
}

void DDG::createUseDefChain() {
  for (auto const &db : ddg_blocks) {
    for (auto const &var : db->Uses) {
      db->Preds[var].insert(VarDecls[var].begin(), VarDecls[var].end());
      db->uniPreds.insert(VarDecls[var].begin(), VarDecls[var].end());
      for (auto const &pred : db->In[var]) {
        db->Preds[var].insert(pred);
        db->uniPreds.insert(pred);
        pred->Succs[var].insert(db);
        pred->uniSuccs.insert(db);
      }
    }
    for (auto const &var :db->Defs) {
      db->Preds[var].insert(VarDecls[var].begin(), VarDecls[var].end());
      db->uniPreds.insert(VarDecls[var].begin(), VarDecls[var].end());
    }
  }
}

void DDG::makeGraph() {
  for (auto const &db : ddg_blocks) {
    checkDependency(db);
  }
  for (auto const &var : AllDefs) {
    auto D = var.first;
    auto defs = var.second;
    for (auto const &def : defs) {
      def->Kill[D] = defs;
      def->Kill[D].erase(def);
      def->Gen[D] = {def};
      def->In[D] = {};
      def->Out[D] = {def};
    }
  }
  updateUntilConverge();
  createUseDefChain();
  countFalse();
}

void DDG::prettyPrint(std::ostream& ofile) {
  auto pp = clang::PrintingPolicy(Ctx->getLangOpts());
  auto jStr = std::string("[\n");
  for (auto const &db : ddg_blocks) {
    auto AB = db->atom_block;
    std::string stmtStream;
    llvm::raw_string_ostream ross(stmtStream);
    if (AB->statement)
      AB->statement->printPretty(ross, nullptr, pp, 0U, "\n", Ctx);
    jStr += " {\n  \"BlockID\": " + std::to_string(AB->atom_id) + ",\n";
    jStr += "  \"FP\": " + std::to_string(db->FP) + ",\n";
    jStr += "  \"FN\": " + std::to_string(db->FN) + ",\n";
    jStr += "  \"Stmt\": " + AB->JsonFormat(llvm::StringRef(ross.str()), true) +
            ", \n";
    jStr += "  \"DDG Edges\": [\n";
    for (auto const &pred : db->uniPreds)
      jStr += "   " + std::to_string(pred->atom_block->atom_id) + ",\n";
    if (!db->uniPreds.empty())
      jStr.erase(jStr.end() - 2);
    jStr += "  ]\n },\n";
  }
  if (!ddg_blocks.empty())
    jStr.erase(jStr.end() - 2);
  jStr += "]\n";
  ofile << jStr;
}

void DDG::countFalse() {
  DBlockSet answer, truth, FNB, FPB;
  BlockSet visited;
  for (auto const &db : ddg_blocks) {
    for (auto const &var : db->Uses) {
      answer = db->Preds[var], FNB = {}, FPB = {}, truth = {}, visited = {};
      traverseDataDep(db, var, truth, visited);
      std::set_difference(answer.begin(), answer.end(),
                          truth.begin(), truth.end(),
                          std::inserter(FPB, FPB.begin()));
      std::set_difference(truth.begin(), truth.end(),
                          answer.begin(), answer.end(),
                          std::inserter(FNB, FNB.begin()));
      db->FP += FPB.size();
      db->FN += FNB.size();
    }
  }
}

void DDG::traverseDataDep(DDGBlock *DB, clang::Decl *const &Var, DBlockSet &Truth, BlockSet &Visited) {
  Visited.insert(DB->atom_block);
  for (auto const &b : DB->atom_block->preds)
    if (auto db = reverseMap[b]; AllDefs[Var].find(db) != AllDefs[Var].end())
      Truth.insert(db);
    else if (Visited.find(b) == Visited.end())
      traverseDataDep(db, Var, Truth, Visited);
}