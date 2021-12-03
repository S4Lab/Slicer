#include "PDG.h"

void PDG::makeGraph() {
  for (auto const &S : ddg->getBlocks()) {
    auto data_preds = BlockVec();
    std::transform(S->getPreds().begin(), S->getPreds().end(),
                   std::back_inserter(data_preds),
                   [](DDGBlock *db) { return db->getAtom(); });
    S->getAtom()->uniDataPreds.insert(data_preds.begin(), data_preds.end());
    pdg_blocks.insert(S->getAtom());
    DAM[S->getAtom()] = S;
    if (auto *DS =
            llvm::dyn_cast_or_null<clang::DeclStmt>(S->getAtom()->statement))
      realSynth.insert(std::make_pair(S->getAtom(), findDecl(DS)));
  }
  for (auto const &S : cdg->getBlocks()) {
    auto cont_preds = BlockVec();
    std::transform(S->getPreds().begin(), S->getPreds().end(),
                   std::back_inserter(cont_preds),
                   [](CDGBlock *cb) { return cb->getAtom(); });
    S->getAtom()->uniContPreds.insert(cont_preds.begin(), cont_preds.end());
    // pdg_blocks.insert(S->getAtom());
    assert(pdg_blocks.find(S->getAtom()) != pdg_blocks.end());
  }
  for (auto const &S : pdg_blocks) {
    std::set_union(S->uniDataPreds.begin(), S->uniDataPreds.end(),
                   S->uniContPreds.begin(), S->uniContPreds.end(),
                   std::inserter(S->uniPreds, S->uniPreds.begin()));
  }
}

void PDG::prettyPrint(std::stringstream& outs) {
  auto pp = clang::PrintingPolicy(Ctx->getLangOpts());
  auto jS = std::string("{\"FunctionName\":\"") +
            CurrentFunction->getNameAsString() + "\",\"Blocks\":[";
  for (auto const &S : pdg_blocks) {
    std::string stmtStream, begStream, endStream, typeStmt, nullS = "\"null\"";
    llvm::raw_string_ostream ross(stmtStream), rosb(begStream), rose(endStream);
    int64_t id = 0;
    unsigned off = 0;
    bool isD = false, notNull = (bool)S->statement;
    if (notNull) {
      auto beg = SourceManager::GetBeginOfStmt(Ctx, S->statement);
      auto end = SourceManager::GetEndOfStmt(Ctx, S->statement);
      end = clang::Lexer::getLocForEndOfToken(end, 0, Ctx->getSourceManager(), Ctx->getLangOpts());
      S->printSourceLocationAsJson(rosb, beg, Ctx->getSourceManager());
      S->printSourceLocationAsJson(rose, end, Ctx->getSourceManager());
      S->statement->printPretty(ross, nullptr, pp, 0U, "\n", Ctx);
      typeStmt = S->statement->getStmtClassName();
      id = S->statement->getID(*Ctx);
      if (realSynth.find(S) != realSynth.end()) {
        auto IdOff = realSynth[S];
        id = IdOff.first->getID(*Ctx);
        off = IdOff.second;
        isD = true;
      }
    }
    jS += "\n{\"BlkID\":" + std::to_string(S->atom_id) + ",";
    jS += "\"StmtID\":" + std::to_string(id) + ",";
    jS += "\"Offset\":" + std::to_string(off) + ",";
    jS += "\"Type\":" + S->JsonFormat(llvm::StringRef(typeStmt)) + ",";
    jS += std::string("\"isDecl\":") + (isD ? "true" : "false") + ",";
    jS += "\"Stmt\":" + S->JsonFormat(llvm::StringRef(ross.str())) + ",";
    jS += "\"Begin\":" + (rosb.str().empty() ? nullS : rosb.str()) + ",";
    jS += "\"End\":" + (rose.str().empty() ? nullS : rose.str()) + ",";
    
    jS += "\"Preds\":[";
    for (auto const &pred : S->preds)
      jS += std::to_string(pred->atom_id) + ",";
    if (!S->preds.empty())
      jS.erase(jS.end() - 1);
    jS += "],";

    jS += "\"Defs\":{";
    for (auto const &p : DAM[S]->getDefs()) {
      jS += S->JsonFormat(std::to_string(p->getID())) + ":";
      auto type = nullS;
      if (auto var = llvm::dyn_cast_or_null<clang::VarDecl>(p))
        type = S->JsonFormat(var->getType().getAsString());
      jS += type + ",";
    }
    if (!DAM[S]->getDefs().empty())
      jS.erase(jS.end() - 1);
    jS += "},";

    jS += "\"Uses\":{";
    for (auto const &p : DAM[S]->getUses()) {
      jS += S->JsonFormat(std::to_string(p->getID())) + ":";
      auto type = nullS;
      if (auto var = llvm::dyn_cast_or_null<clang::VarDecl>(p))
        type = S->JsonFormat(var->getType().getAsString());
      jS += type + ",";
    }
    if (!DAM[S]->getUses().empty())
      jS.erase(jS.end() - 1);
    jS += "},";

    jS += "\"P Edge\":[";
    for (auto const &pred : S->uniPreds)
      makeStmtStr(pred, jS);
    if (!S->uniPreds.empty())
      jS.erase(jS.end() - 1);
    jS += "],";

    jS += "\"C Edge\":[";
    for (auto const &pred : S->uniContPreds)
      jS += std::to_string(pred->atom_id) + ",";
    if (!S->uniContPreds.empty())
      jS.erase(jS.end() - 1);
    jS += "],";

    jS += "\"D Edge\":[";
    for (auto const &pred : S->uniDataPreds)
      jS += std::to_string(pred->atom_id) + ",";
    if (!S->uniDataPreds.empty())
      jS.erase(jS.end() - 1);
    jS += "]},";
  }
  if (!pdg_blocks.empty())
    jS.erase(jS.end() - 1);
  jS += "]},";
  outs << jS;
}
void PDG::prettyCDPrint(std::string base_name) {
  // auto CFile = std::ofstream(base_name + "__c.json");
  // auto DFile = std::ofstream(base_name + "__d.json");
  // auto AFile = std::ofstream(base_name + "__a.json");
  // if (!C2P)
  //   cdg->prettyPrint(CFile);
  // if (!D2P)
  //   ddg->prettyPrint(DFile);
  // if (!A2P)
  //   acfg->prettyPrint(AFile);
}

void PDG::makeStmtStr(AtomBlock *S, std::string &jS) {
  int64_t id = 0;
  unsigned off = 0;
  bool isD = false;
  if (S->statement) {
    id = S->statement->getID(*Ctx);
    if (realSynth.find(S) != realSynth.end()) {
      auto IdOff = realSynth[S];
      id = IdOff.first->getID(*Ctx);
      off = IdOff.second;
      isD = true;
    }
  }
  jS += "{\"BlkID\":" + std::to_string(S->atom_id) + ",";
  jS += "\"StmtID\":" + std::to_string(id) + ",";
  jS += "\"Offset\":" + std::to_string(off) + ",";
  jS += "\"isDecl\":" + std::string(isD ? "true" : "false");
  jS += "},";
}

  // jS += "     {\n      \"BlkID\": " + std::to_string(S->atom_id) + ",\n";
  // jS += "      \"StmtID\": " + std::to_string(id) + ",\n";
  // jS += "      \"Offset\": " + std::to_string(off) + ",\n";
  // jS += "      \"isDecl\": " + ((isD ? "true" : "false") + std::string("\n"));
  // jS += "     },\n";
DeclIdx PDG::findDecl(clang::DeclStmt *DS) {
  const clang::DeclStmt *decla = nullptr;
  unsigned ind = 0;
  if (auto syn_it = llvm::find_if(
          acfg->raw_cfg->synthetic_stmts(),
          [&DS](const llvm::detail::DenseMapPair<const clang::DeclStmt *,
                                                 const clang::DeclStmt *> &it) {
            return it.getFirst() == DS;
          });
      syn_it != acfg->raw_cfg->synthetic_stmt_end()) {
    auto syn_decl = llvm::dyn_cast<clang::VarDecl>(*(DS->decl_begin()));
    for (auto child : syn_it->getSecond()->decls()) {
      auto VD = llvm::dyn_cast<clang::VarDecl>(child);
      if (VD->getNameAsString() == syn_decl->getNameAsString()) {
        decla = syn_it->getSecond();
        break;
      }
      ind++;
    }
  } else {
    decla = DS;
  }
  return DeclIdx(decla, ind);
}