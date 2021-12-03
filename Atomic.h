#ifndef ATOMIC_H
#define ATOMIC_H

// Clang includes
#include <clang/AST/ASTContext.h>
#include <clang/Analysis/CFG.h>
#include <clang/Tooling/CommonOptionsParser.h>

// LLVM includes
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <queue>
#include <stack>
#include <sstream>
#include <fstream>

class AtomicCFG;
class AtomBlock;
class DDGBlock;
class CDGBlock;

using BlockSet = std::set<AtomBlock *>;
using BlockVec = std::vector<AtomBlock *>;
using DBlockSet = std::set<DDGBlock *>;
using DBlockVec = std::vector<DDGBlock *>;
using CBlockSet = std::set<CDGBlock *>;
using CBlockVec = std::vector<CDGBlock *>;
using DeclSet = std::set<clang::Decl *>;
using DDeclStmtMap = std::map<clang::Decl *, DBlockSet>;


// static llvm::cl::opt<bool> A2P("no-atomic", llvm::cl::desc(R"(Do not print atomic CFG data indivisually)"), llvm::cl::init(false), llvm::cl::cat(SliceCategory));

// static llvm::cl::opt<bool> C2P("no-control", llvm::cl::desc(R"(Do not print control dependency analysis indivisually)"), llvm::cl::init(false), llvm::cl::cat(SliceCategory));

// static llvm::cl::opt<bool> D2P("no-data", llvm::cl::desc(R"(Do not print date dependency analysis indivisually)"), llvm::cl::init(false), llvm::cl::cat(SliceCategory));

class TMPPC {
public:
  bool isReachable;
  unsigned id;
  TMPPC() {}
  TMPPC(bool isReachable, unsigned id) : isReachable(isReachable), id(id) {}
};

class AtomBlock {
  friend class PDG;
public:
  AtomicCFG *parent;
  clang::CFGBlock *parent_cfg_block;
  unsigned atom_id;
  clang::Stmt *statement;
  std::vector<TMPPC> tmp_preds;
  std::vector<TMPPC> tmp_succs;
  BlockVec preds, succs;
  BlockSet uniPreds, uniDataPreds, uniContPreds;
  bool isReachable;

public:
  AtomBlock() {}
  AtomBlock(float block_id) {this->atom_id = -1;}
  AtomBlock(AtomicCFG *atomic, clang::CFGBlock *parent, unsigned block_id,
            clang::Stmt *stmt)
      : parent(atomic), parent_cfg_block(parent), atom_id(block_id), statement(stmt) {}

  static std::string JsonFormat(clang::StringRef RawSR, bool AddQuotes = true);
  void printSourceLocationAsJson(llvm::raw_ostream &Out, clang::SourceLocation Loc, const clang::SourceManager &SM, bool AddBraces = true);
  bool operator==(AtomBlock &a);
  void add_succ(float block_id, bool reach);
  void add_pred(float block_id, bool reach);
  void add_all_succs();
  void add_all_preds();
  BlockVec get_preds();
  BlockVec get_succs();
  const BlockSet& getProgPreds() { return uniPreds; }
  const BlockSet& getDataPreds() { return uniDataPreds; }
  const BlockSet& getContPreds() { return uniContPreds; }
};

class AtomicCFG {
public:
  clang::ASTContext *Ctx;
  std::unique_ptr<clang::CFG> raw_cfg;
  std::vector<AtomBlock> atom_blocks;
  const clang::FunctionDecl *CurrentFunction;

  AtomicCFG() {}
  AtomicCFG(std::unique_ptr<clang::CFG>& raw_cfg, clang::ASTContext *Ctx, const clang::FunctionDecl *Function) : Ctx(Ctx), CurrentFunction(Function) {
    this->raw_cfg = std::move(raw_cfg);
    generate_atomic_cfg();
  }
  void generate_atomic_cfg();
  void prettyPrint(std::ostream& ofile);
};

#endif // ATOMIC_H