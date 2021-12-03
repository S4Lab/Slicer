#include "Atomic.h"

void AtomBlock::add_succ(float block_id, bool reach) {
  TMPPC tmp(reach, block_id);
  this->tmp_succs.push_back(tmp);
}
void AtomBlock::add_pred(float block_id, bool reach) {
  TMPPC tmp(reach, block_id);
  this->tmp_preds.push_back(tmp);
}

BlockVec AtomBlock::get_preds() { return this->preds; }

BlockVec AtomBlock::get_succs() { return this->succs; }

void AtomBlock::add_all_preds() {
  for (auto &&b : this->parent->atom_blocks) {
    for (auto &&p : this->tmp_preds) {
      if (b.atom_id == p.id) {
        this->preds.push_back(&b);
      }
    }
  }
}

void AtomBlock::add_all_succs() {
  for (auto &&b : this->parent->atom_blocks) {
    for (auto &&s : this->tmp_succs) {
      if (b.atom_id == s.id) {
        this->succs.push_back(&b);
      }
    }
  }
}

void AtomicCFG::generate_atomic_cfg() {
  unsigned max = 0, scale = 10;
  for (auto *const blk : *this->raw_cfg)
    if (auto size = blk->size(); size > max)
      max = size;

  while (max /= 10)
    scale *= 10;

  for (auto *const blk : *this->raw_cfg) {
    unsigned bID = blk->getBlockID();
    unsigned size = blk->size();
    clang::Stmt *statement = nullptr;

    if (size > 1) {
      for (unsigned i = 0; i < size; i++) {
        statement = (clang::Stmt *)(*blk)[size - i - 1].castAs<clang::CFGStmt>().getStmt();
        AtomBlock tmp(this, blk, bID * scale + i, statement);
        if (i == size - 1) {
          tmp.add_succ(bID * scale + i - 1, true);
          for (auto it = blk->pred_begin(); it != blk->pred_end(); it++) {
            bool flag;
            unsigned _id;
            if (it->isReachable()) {
              _id = (*it)->getBlockID();
              flag = true;
            } else {
              _id = it->getPossiblyUnreachableBlock()->getBlockID();
              flag = false;
            }
            _id *= scale;
            tmp.add_pred(_id, flag);
          }
        } else if (i == 0) {
          tmp.add_pred(bID * scale + i + 1, true);
          for (auto it = blk->succ_begin(); it != blk->succ_end(); it++) {
            if (it->isReachable()) {
              auto size = (*it)->size(),
                   last = (size > 0) ? size - 1 : size;
              tmp.add_succ((*it)->getBlockID() * scale + last, true);
            } else {
              auto size = it->getPossiblyUnreachableBlock()->size(),
                   last = (size > 0) ? size - 1 : size,
                   bID = it->getPossiblyUnreachableBlock()->getBlockID();
              tmp.add_succ(bID * scale + last, false);
            }
          }
        } else {
          tmp.add_pred(bID * scale + i + 1, true);
          tmp.add_succ(bID * scale + i - 1, true);
        }
        this->atom_blocks.push_back(tmp);
      }
    } else {
      if (size != 0)
        statement = (clang::Stmt *)(*blk)[0].castAs<clang::CFGStmt>().getStmt();
      else
        statement = nullptr;
      AtomBlock tmp(this, blk, bID * scale, statement);
      for (auto it = blk->pred_begin(); it != blk->pred_end(); it++) {
        if (it->isReachable()) {
          tmp.add_pred((*it)->getBlockID() * scale, true);
        } else {
          auto bID = it->getPossiblyUnreachableBlock()->getBlockID();
          tmp.add_pred(bID * scale, false);
        }
      }
      for (auto it = blk->succ_begin(); it != blk->succ_end(); it++) {
        if (it->isReachable()) {
          auto size = (*it)->size(),
               last = (size > 0) ? size - 1 : size;
          tmp.add_succ((*it)->getBlockID() * scale + last, true);
        } else if (auto b = it->getPossiblyUnreachableBlock()){
          auto size = b->size(),
               last = (size > 0) ? size - 1 : size,
               bID = b->getBlockID();
          tmp.add_succ(bID * scale + last, false);
        }



      }
      this->atom_blocks.push_back(tmp);
    }
  }
  for (auto &&b : this->atom_blocks) {
    b.add_all_preds();
    b.add_all_succs();
  }
}

bool AtomBlock::operator==(AtomBlock &a) { return a.atom_id == this->atom_id; }

void AtomicCFG::prettyPrint(std::ostream& ofile) {
  auto pp = clang::PrintingPolicy(Ctx->getLangOpts());
  auto jStr = std::string("[\n");
  for (auto const &AB : atom_blocks) {
    // auto AB = db.atom_block;
    std::string stmtStream;
    llvm::raw_string_ostream ross(stmtStream);
    if (AB.statement)
      AB.statement->printPretty(ross, nullptr, pp, 0U, "\n", Ctx);
    jStr += " {\"BlockID\":" + std::to_string(AB.atom_id) + ",";
    jStr += "\"Stmt\":" + AB.JsonFormat(llvm::StringRef(ross.str()), true) +
            ",";
    jStr += "\"CFG Preds\":[";
    for (auto const &pred : AB.preds)
      jStr += std::to_string(pred->atom_id) + ",";
    if (!AB.preds.empty())
      jStr.erase(jStr.end() - 1);
    jStr += "]},";
  }
  if (!atom_blocks.empty())
    jStr.erase(jStr.end() - 1);
  jStr += "]";
  ofile << jStr;
}

std::string AtomBlock::JsonFormat(clang::StringRef RawSR, bool AddQuotes) {
  if (RawSR.empty())
    return "\"null\"";
  // Trim special characters.
  std::string Str = RawSR.trim().str();
  size_t Pos = 0;
  // Escape backslashes.
  while (true) {
    Pos = Str.find('\\', Pos);
    if (Pos == std::string::npos)
      break;
    // Prevent bad conversions.
    size_t TempPos = (Pos != 0) ? Pos - 1 : 0;
    // See whether the current backslash is not escaped.
    if (TempPos != Str.find("\\\\", Pos)) {
      Str.insert(Pos, "\\");
      ++Pos; // As we insert the backslash move plus one.
    }
    ++Pos;
  }
  // Escape double quotes.
  Pos = 0;
  while (true) {
    Pos = Str.find('\"', Pos);
    if (Pos == std::string::npos)
      break;
    // Prevent bad conversions.
    size_t TempPos = (Pos != 0) ? Pos - 1 : 0;
    // See whether the current double quote is not escaped.
    if (TempPos != Str.find("\\\"", Pos)) {
      Str.insert(Pos, "\\");
      ++Pos; // As we insert the escape-character move plus one.
    }
    ++Pos;
  }
  // Remove new-lines.
  Str.erase(std::remove(Str.begin(), Str.end(), '\n'), Str.end());
  if (!AddQuotes)
    return Str;
  return '\"' + Str + '\"';
}

void AtomBlock::printSourceLocationAsJson(llvm::raw_ostream &Out, clang::SourceLocation Loc, const clang::SourceManager &SM, bool AddBraces) {
  // Mostly copy-pasted from SourceLocation::print.
  if (!Loc.isValid()) {
    Out << "null";
    return;
  }
  if (Loc.isFileID()) {
    clang::PresumedLoc PLoc = SM.getPresumedLoc(Loc);
    if (PLoc.isInvalid()) {
      Out << "null";
      return;
    }
    // The macro expansion and spelling pos is identical for file locs.
    if (AddBraces)
      Out << "{ ";
    Out << "\"line\": " << PLoc.getLine()
        << ", \"column\": " << PLoc.getColumn()
        << ", \"ID\": " << Loc.getRawEncoding();
    // Loc.dump(SM);
    if (AddBraces)
      Out << " }";
    return;
  }
  // We want 'location: { ..., spelling: { ... }}' but not
  // 'location: { ... }, spelling: { ... }', hence the dance
  // with braces.
  Out << "{ ";
  printSourceLocationAsJson(Out, SM.getExpansionLoc(Loc), SM, false);
  Out << ", \"spelling\": ";
  printSourceLocationAsJson(Out, SM.getSpellingLoc(Loc), SM, true);
  Out << " }";
}