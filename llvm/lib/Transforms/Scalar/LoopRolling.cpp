#include "llvm/Transforms/Scalar/LoopRolling.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/DOTGraphTraits.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Transforms/Utils/Local.h"
#include <queue>

#define DEBUG_TYPE "loop-rolling"

using namespace llvm;

static cl::opt<std::string> LoopRollingDumpAlignmentGraph(
    "loop-rolling-dump-alignment-graph",
    cl::desc("Dump DOT files for loop rolling alignment graphs"));
static cl::opt<bool>
    LoopRollingIgnoreCost("loop-rolling-ignore-cost",
                          cl::desc("Ignore cost and always roll"));
static cl::opt<int> LoopRollingCostThreshold(
    "loop-rolling-cost-threshold",
    cl::desc("Threshold for loop rolling profitability"));

STATISTIC(LoopRollingFuctions, "Number of functions processed by loop rolling");
STATISTIC(StoreInstructions, "Number of seed store instructions");
STATISTIC(CallInstructions, "Number of seed call instructions");
STATISTIC(MatchingNodes, "Number of matching nodes");
STATISTIC(MismatchingNodes, "Number of mismatching nodes");
STATISTIC(IdenticalNodes, "Number of identical nodes");
STATISTIC(IntegerSeqNodes, "Number of integer sequence nodes");
STATISTIC(NeutralPointerNodes, "Number of neutral pointer nodes");
STATISTIC(TotalCostReduced, "Total cost savings");
STATISTIC(NumSkippedConstantExprs, "Number of skipped ConstantExprs");
STATISTIC(NumGEPsWithStructsSkipped,
          "Number of GEPs skipped because they point to structs");
STATISTIC(NumAllocasInBody, "Number skipped because allocas in loop body");
STATISTIC(NumLandingPadBlocksSkipped, "Number skipped because of landing pads");
STATISTIC(NumCannotBeScheduled, "Number of unschedulable transformations");

class Node {
public:
  enum NodeType {
    Matching,
    Recurance,
    Mismatching,
    Identical,
    IntegerSeq,
    NeutralPointer,
  };

  SmallVector<Value *> Values;
  NodeType NType;

  NodeType getType() const { return NType; }

  template <typename ValueT>
  Node(ArrayRef<ValueT *> Values, NodeType NType) : NType(NType) {
    for (auto *V : Values)
      this->Values.push_back(cast<Value>(V));
  }

  size_t getIterationSize() const { return Values.size(); }

  void print(raw_ostream &OS) const;
};

static std::unique_ptr<Node> createNode(ArrayRef<Value *> Values,
                                        const BasicBlock &BB);
static std::unique_ptr<Node> createMatchingNode(ArrayRef<Value *> Values,
                                                const BasicBlock &BB);

class MatchingNode : public Node {
public:
  SmallVector<std::unique_ptr<Node>> Children;

  MatchingNode(ArrayRef<Value *> Values,
               SmallVectorImpl<std::unique_ptr<Node>> &Children,
               NodeType Type = Matching)
      : Node(Values, Type), Children(std::move(Children)) {
    ++MatchingNodes;
  }
  static bool classof(const Node *N) {
    return N->getType() == Matching || N->getType() == NeutralPointer;
  }
};

class RecuranceNode : public Node {
public:
  Value *InitialValue;
  PHINode *PhiInst = nullptr;
  RecuranceNode(ArrayRef<Value *> Values, Value *InitialValue)
      : Node(Values, Recurance), InitialValue(InitialValue) {
    // ++MatchingNodes;
  }

  static bool classof(const Node *N) { return N->getType() == Recurance; }
};

class MismatchingNode : public Node {
public:
  bool UseGlobalArray; // constant -> global array; else -> normal stack array
  Value *Array = nullptr;
  template <typename ValueT>
  MismatchingNode(ArrayRef<ValueT *> Values, bool UseGlobalArray = false)
      : Node(Values, Mismatching), UseGlobalArray(UseGlobalArray) {
    ++MismatchingNodes;
  }

  static bool classof(const Node *N) { return N->getType() == Mismatching; }
};

class IdenticalNode : public Node {
public:
  template <typename ValueT>
  IdenticalNode(ArrayRef<ValueT *> Values) : Node(Values, Identical) {
    ++IdenticalNodes;
  }

  static bool classof(const Node *N) { return N->getType() == Identical; }
};

class IntegerSeqNode : public Node {
public:
  APInt Step, Start, End;

  IntegerSeqNode(ArrayRef<Value *> Values) : Node(Values, IntegerSeq) {
    Start = cast<ConstantInt>(Values[0])->getValue();
    End = cast<ConstantInt>(Values.back())->getValue();
    Step = cast<ConstantInt>(Values[1])->getValue() - Start;
    ++IntegerSeqNodes;
  }

  static bool classof(const Node *N) { return N->getType() == IntegerSeq; }
};

class NeutralPointerNode : public MatchingNode {
public:
  Value *BaseAddress;
  GetElementPtrInst *GEPInst;
  ConstantExpr *GEPConstExpr;

  NeutralPointerNode(ArrayRef<Value *> Values,
                     SmallVectorImpl<std::unique_ptr<Node>> &Children,
                     Value *BaseAddress, GetElementPtrInst *GEPInst,
                     ConstantExpr *GEPConstExpr)
      : MatchingNode(Values, Children, NeutralPointer),
        BaseAddress(BaseAddress), GEPInst(GEPInst), GEPConstExpr(GEPConstExpr) {
    ++NeutralPointerNodes;
  }

  static bool classof(const Node *N) { return N->getType() == NeutralPointer; }
};

struct ExternalArray {
  ArrayType *ArrayTy;
  Value *ArrayAlloc;

  ExternalArray(ArrayType *ArrayTy, Value *ArrayAlloc)
      : ArrayTy(ArrayTy), ArrayAlloc(ArrayAlloc) {};
};

static ArrayRef<std::unique_ptr<Node>> getChildren(const Node *N) {
  if (auto *MN = dyn_cast<MatchingNode>(N))
    return MN->Children;
  return {};
}

// TODO: Inline
class AlignmentGraph {
public:
  std::unique_ptr<Node> Root;

  AlignmentGraph(ArrayRef<Value *> Values, BasicBlock &BB) {
    Root = createMatchingNode(Values, BB);
  }

#ifndef NDEBUG
  void debugPrint(const std::unique_ptr<Node> &CurNode, int Depth = 0) const {
    dbgs() << Depth << ":  ";
    CurNode->print(dbgs());
    dbgs() << "\n";
    for (auto &Child : getChildren(CurNode.get()))
      debugPrint(Child, Depth + 1);
  }
#endif
  void dumpDOTGraph(StringRef FileName);
};

static bool canOperandBeChanged(const Instruction *I, unsigned OpIdx) {
  // TODO: Could this be moved into canReplaceOperandWithVariable()?
  if (auto *In = dyn_cast<IntrinsicInst>(I))
    if (In->getIntrinsicID() == Intrinsic::threadlocal_address)
      if (OpIdx == 0)
        return false;
  return canReplaceOperandWithVariable(I, OpIdx);
}

static bool operandsCanBeMatched(Instruction *I1, Instruction *I2) {
  if (I1->getNumOperands() != I2->getNumOperands())
    return false;
  for (const auto &[OpIdx, Ops] :
       enumerate(zip_equal(I1->operand_values(), I2->operand_values()))) {
    if (canOperandBeChanged(I1, OpIdx) && canOperandBeChanged(I2, OpIdx))
      continue;
    // If these operands cannot be variables, make sure they are identical,
    // otherwise they will become a variable if we make this a matching node
    auto &[Op1, Op2] = Ops;
    if (Op1 != Op2) {
      if (isa<GetElementPtrInst>(I1))
        ++NumGEPsWithStructsSkipped;
      return false;
    }
  }
  return true;
}

static bool areMatchingValues(Value *V1, Value *V2) {
  if (V1->getType() != V2->getType())
    return false;

  if (auto *I1 = dyn_cast<Instruction>(V1)) {
    if (auto *I2 = dyn_cast<Instruction>(V2)) {
      if (isa<PHINode>(I1) || isa<PHINode>(I2))
        return false;

      // TODO: check return type + parameters' type
      if (auto *CB1 = dyn_cast<CallBase>(I1)) {
        if (auto *CB2 = dyn_cast<CallBase>(I2)) {
          if (!CB1->getCalledFunction())
            return false;
          if (CB1->getCalledFunction() != CB2->getCalledFunction())
            return false;
        }
      }
      if (!operandsCanBeMatched(I1, I2))
        return false;
      // TODO: There are other flags to check
      if (auto *O1 = dyn_cast<OverflowingBinaryOperator>(I1)) {
        if (auto *O2 = dyn_cast<OverflowingBinaryOperator>(I2)) {
          if (O1->hasNoSignedWrap() != O2->hasNoSignedWrap())
            return false;
          if (O1->hasNoUnsignedWrap() != O2->hasNoUnsignedWrap())
            return false;
        }
      }
      return I1->isSameOperationAs(
          I2, Instruction::OperationEquivalenceFlags::CompareIgnoringAlignment);
    }
  }

  return V1 == V2;
}

static Value *areMatchingGEPValues(ArrayRef<Value *> ValuesRef) {
  const auto *FirstGEPIt = find_if(ValuesRef, [](const Value *V) {
    // TODO: Support constant GEPs
    if (auto *CE = dyn_cast<ConstantExpr>(V))
      if (CE->getOpcode() == Instruction::GetElementPtr)
        ++NumSkippedConstantExprs;
    return isa<GetElementPtrInst>(V);
  });

  if (FirstGEPIt == ValuesRef.end())
    return nullptr;

  Value *FirstPointerOperand = nullptr;
  unsigned NumOperands = 0;
  Type *SrcType = nullptr;
  unsigned AS = 0;
  // TODO: Refactor
  if (auto *I = dyn_cast<GetElementPtrInst>(*FirstGEPIt)) {
    FirstPointerOperand = I->getPointerOperand();
    SrcType = I->getSourceElementType();
    NumOperands = I->getNumOperands();
    AS = I->getPointerAddressSpace();
  } else if (auto *CE = dyn_cast<ConstantExpr>(*FirstGEPIt)) {
    FirstPointerOperand = CE->getOperand(0);
    NumOperands = CE->getNumOperands();
    SrcType = cast<GEPOperator>(CE)->getSourceElementType();
    AS = cast<GEPOperator>(CE)->getPointerAddressSpace();
  }
  Value *FirstPointerObject = getUnderlyingObject(FirstPointerOperand);

  for (Value *V : ValuesRef) {
    if (V == *FirstGEPIt)
      continue;

    if (auto *GEP = dyn_cast<GetElementPtrInst>(V)) {
      if (GEP->getNumOperands() != NumOperands)
        return nullptr;
      if (GEP->getSourceElementType() != SrcType)
        return nullptr;
      if (GEP->getPointerAddressSpace() != AS)
        return nullptr;
      if (!areMatchingValues(GEP->getPointerOperand(), FirstPointerOperand))
        return nullptr;
      if (!operandsCanBeMatched(cast<Instruction>(*FirstGEPIt), GEP))
        return nullptr;
    } else if (auto *CE = dyn_cast<ConstantExpr>(V)) {
      if (CE->getOpcode() != Instruction::GetElementPtr)
        return nullptr;
      if (CE->getNumOperands() != NumOperands)
        return nullptr;
      if (cast<GEPOperator>(CE)->getSourceElementType() != SrcType)
        return nullptr;
      if (cast<GEPOperator>(CE)->getPointerAddressSpace() != AS)
        return nullptr;
      if (!areMatchingValues(CE->getOperand(0), FirstPointerOperand))
        return nullptr;
    } else {
      if (!areMatchingValues(V, FirstPointerObject))
        return nullptr;
    }
  }

  return FirstPointerOperand;
}

static std::unique_ptr<Node>
createRecuranceNode(ArrayRef<Value *> Values, ArrayRef<Value *> ParentValues) {
  if (Values.size() < 2 || ParentValues.size() < 2)
    return {};
  auto *InitialValue = Values.front();
  for (auto *V : Values.drop_front())
    if (InitialValue->getType() != V->getType())
      return {};
  for (const auto &[V, ParentV] : llvm::zip(Values.drop_front(), ParentValues))
    if (V != ParentV)
      return {};
  return std::make_unique<RecuranceNode>(Values, InitialValue);
}

static std::unique_ptr<Node> createMatchingNode(ArrayRef<Value *> Values,
                                                const BasicBlock &BB) {
  SmallVector<Instruction *> Instructions;
  for (auto *V : Values) {
    if (auto *I = dyn_cast<Instruction>(V)) {
      // TODO: Sometimes instructions outside this BB can be move into the
      // loop body safely
      if (I->getParent() != &BB)
        return {};
      Instructions.push_back(I);
      continue;
    }
    return {};
  }
  auto Instrs = ArrayRef(Instructions);
  for (auto *I : Instrs.drop_front())
    if (!areMatchingValues(Instrs.front(), I))
      return {};

  SmallVector<std::unique_ptr<Node>> Children;
  unsigned OpCount = Instrs.front()->getNumOperands();
  for (unsigned OpIdx = 0; OpIdx < OpCount; ++OpIdx) {
    SmallVector<Value *> Ops;
    for (Instruction *I : Instrs) {
      assert(I->getNumOperands() == OpCount);
      Ops.push_back(I->getOperand(OpIdx));
    }
    if (auto RN = createRecuranceNode(Ops, Values))
      Children.push_back(std::move(RN));
    else
      Children.push_back(createNode(Ops, BB));
  }
  return std::make_unique<MatchingNode>(Values, Children);
}

static std::unique_ptr<IntegerSeqNode>
createIntegerSeqNode(ArrayRef<Value *> Values) {
  // TODO: Check same type
  for (auto *V : Values)
    if (!isa<ConstantInt>(V))
      return {};

  APInt Step = cast<ConstantInt>(Values[1])->getValue() -
               cast<ConstantInt>(Values[0])->getValue();
  for (auto [A, B] : llvm::zip(Values, Values.drop_front()))
    if (Step !=
        cast<ConstantInt>(B)->getValue() - cast<ConstantInt>(A)->getValue())
      return {};
  return std::make_unique<IntegerSeqNode>(Values);
}

static std::unique_ptr<NeutralPointerNode>
createNeutralPointerNode(ArrayRef<Value *> Values, const BasicBlock &BB) {
  auto *BaseAddress = areMatchingGEPValues(Values);
  if (!BaseAddress)
    return {};

  for (auto *V : Values)
    if (auto *I = dyn_cast<GetElementPtrInst>(V))
      if (I->getParent() != &BB)
        return {};

  const auto *FirstGEPIt = llvm::find_if(Values, [](const Value *V) {
    if (auto *CE = dyn_cast<ConstantExpr>(V))
      return CE->getOpcode() == Instruction::GetElementPtr;
    return isa<GetElementPtrInst>(V);
  });

  assert(FirstGEPIt != Values.end());

  unsigned NumOperands;
  GetElementPtrInst *GEPInst;
  ConstantExpr *GEPConstExpr;
  if (auto *I = dyn_cast<GetElementPtrInst>(*FirstGEPIt)) {
    GEPInst = I;
    NumOperands = I->getNumOperands();
  } else if (auto *CE = dyn_cast<ConstantExpr>(*FirstGEPIt)) {
    GEPConstExpr = CE;
    NumOperands = CE->getNumOperands();
  }

  // process the GEP's child instructions
  SmallVector<Value *> LeftoverInstructions;
  for (Value *V : Values) {
    if (auto *GEP = dyn_cast<GetElementPtrInst>(V))
      LeftoverInstructions.push_back(GEP->getPointerOperand());
    else if (auto *CE = dyn_cast<ConstantExpr>(V))
      LeftoverInstructions.push_back(CE->getOperand(0));
    else
      LeftoverInstructions.push_back(V);
  }
  SmallVector<std::unique_ptr<Node>> Children;
  Children.push_back(createNode(LeftoverInstructions, BB));

  // process the left-over operands
  for (unsigned i = 1; i < NumOperands; ++i) {
    Type *Ty = GEPInst ? GEPInst->getOperand(i)->getType()
                       : GEPConstExpr->getOperand(i)->getType();

    SmallVector<Value *> LeftoverOperands;
    for (Value *V : Values) {
      if (auto *GEP = dyn_cast<GetElementPtrInst>(V))
        LeftoverOperands.emplace_back(GEP->getOperand(i));
      else if (auto *CE = dyn_cast<ConstantExpr>(V))
        LeftoverOperands.emplace_back(CE->getOperand(i));
      else
        LeftoverOperands.emplace_back(ConstantInt::get(Ty, 0)); // insert 0
    }
    Children.push_back(createNode(LeftoverOperands, BB));
  }

  return std::make_unique<NeutralPointerNode>(Values, Children, BaseAddress,
                                              GEPInst, GEPConstExpr);
}

static std::unique_ptr<Node> createMismatchingNode(ArrayRef<Value *> Values,
                                                   const BasicBlock &BB) {
  for (auto *V : Values.drop_front())
    if (Values.front()->getType() != V->getType())
      return std::make_unique<MismatchingNode>(Values);

  // TODO: Move to createNode? Types must be the same
  if (auto IN = createIntegerSeqNode(Values))
    return IN;

  if (auto NP = createNeutralPointerNode(Values, BB))
    return NP;

  bool IsAllConstant =
      all_of(Values, [](Value *V) { return isa<Constant>(V); });

  return std::make_unique<MismatchingNode>(Values, IsAllConstant);
}

static std::unique_ptr<Node> createNode(ArrayRef<Value *> Values,
                                        const BasicBlock &BB) {
  if (llvm::all_equal(Values))
    return std::make_unique<IdenticalNode>(Values);

  if (auto MN = createMatchingNode(Values, BB))
    return MN;

  return createMismatchingNode(Values, BB);
}

void Node::print(raw_ostream &OS) const {
  if (const auto *N = dyn_cast<MismatchingNode>(this)) {
    OS << "Mismatching node:\n";
    for (const auto *V : N->Values)
      OS << "  " << *V;
  } else if (const auto *N = dyn_cast<RecuranceNode>(this)) {
    OS << "RecuranceNode:\n";
    OS << "  Init: " << *N->InitialValue << "\n";
  } else if (const auto *N = dyn_cast<IdenticalNode>(this)) {
    OS << "Identical node:\n  " << *N->Values.front();
  } else if (const auto *N = dyn_cast<IntegerSeqNode>(this)) {
    OS << "Integer sequence node:\n  Start: " << N->Start
       << "\n  Step: " << N->Step << "\n  End: " << N->End;
  } else if (isa<NeutralPointerNode>(this)) {
    OS << "Neutral Pointer node: GEP";
  } else if (const auto *N = dyn_cast<MatchingNode>(this)) {
    OS << "Matching node:\n  " << *N->Values.front();
  } else {
    llvm_unreachable("");
  }
}

namespace llvm {

template <> struct GraphTraits<AlignmentGraph *> {
  using NodeRef = Node *;

  // iterator to traverse children of a node (required by GraphTraits)
  class ChildIteratorType {
    using InnerIterator =
        typename SmallVector<std::unique_ptr<Node>>::const_iterator;
    InnerIterator I;

  public:
    // define attributes of the iterator
    using difference_type = std::ptrdiff_t;
    using value_type = Node *;
    using pointer = Node **;
    using reference = Node *&;
    using iterator_category = std::forward_iterator_tag;

    ChildIteratorType(InnerIterator I) : I(I) {}

    NodeRef operator*() const { return I->get(); }

    ChildIteratorType &operator++() {
      ++I;
      return *this;
    }

    bool operator!=(const ChildIteratorType &X) const { return I != X.I; }
  };

  static NodeRef getNode(NodeRef N) { return N; }
  static NodeRef getEntryNode(AlignmentGraph *G) { return G->Root.get(); }

  static ChildIteratorType child_begin(NodeRef N) {
    return ChildIteratorType(getChildren(N).begin());
  }

  static ChildIteratorType child_end(NodeRef N) {
    return ChildIteratorType(getChildren(N).end());
  }

  // iterator to traverse all nodes in the graph (required by GraphTraits)
  class nodes_iterator {
    std::queue<Node *> NodeList;

  public:
    nodes_iterator(Node *N) {
      if (N) {
        NodeList.push(N);
      }
    }

    NodeRef operator*() { return NodeList.front(); }

    // BFS traversal
    nodes_iterator &operator++() {
      Node *N = NodeList.front();
      NodeList.pop();

      for (auto I = child_begin(N), E = child_end(N); I != E; ++I) {
        NodeList.push(*I);
      }

      return *this;
    }

    bool operator!=(const nodes_iterator &N2) const {
      return !(NodeList.empty() && N2.NodeList.empty());
    }
  };

  static nodes_iterator nodes_begin(AlignmentGraph *AG) {
    return nodes_iterator(AG->Root.get());
  }

  static nodes_iterator nodes_end(AlignmentGraph *AG) {
    return nodes_iterator(nullptr);
  }
};

template <>
struct DOTGraphTraits<AlignmentGraph *> : public DefaultDOTGraphTraits {
  DOTGraphTraits(bool IsSimple = true) : DefaultDOTGraphTraits(IsSimple) {}

  static std::string getGraphName(AlignmentGraph *G) {
    return "Alignment Graph";
  }

  static std::string getNodeLabel(Node *Node, AlignmentGraph *G) {
    std::string NodeLabel;
    raw_string_ostream NodeStream(NodeLabel);
    Node->print(NodeStream);
    return NodeStream.str();
  }

  static std::string getNodeAttributes(Node *Node, AlignmentGraph *G) {
    std::string Attribute = "shape=rectangle,style=filled,";
    switch (Node->getType()) {
    case Node::Matching:
    case Node::Identical:
      Attribute += "color=green4,fillcolor=green";
      break;
    case Node::Mismatching:
      Attribute += "color=red,fillcolor=pink";
      break;
    default:
      Attribute += "color=yellow4,fillcolor=yellow";
    }
    return Attribute;
  }

  static std::string
  getEdgeAttributes(Node *Source,
                    GraphTraits<AlignmentGraph *>::ChildIteratorType ChildIter,
                    AlignmentGraph *G) {
    return "color=black";
  }
};
} // namespace llvm

void AlignmentGraph::dumpDOTGraph(StringRef FileName) {
  std::error_code EC;
  raw_fd_ostream File(FileName, EC);
  WriteGraph(File, this);
}

template <typename T>
static Align getCommonAlignment(ArrayRef<const Value *> Values) {
  const auto *V = llvm::min_element(Values, [](auto *L, auto *R) {
    return cast<T>(L)->getAlign() < cast<T>(R)->getAlign();
  });
  return cast<T>(*V)->getAlign();
}

static void setCommonAlignment(Instruction *V, ArrayRef<const Value *> Values) {
  if (auto *I = dyn_cast<StoreInst>(V))
    I->setAlignment(getCommonAlignment<StoreInst>(Values));
  else if (auto *I = dyn_cast<LoadInst>(V))
    I->setAlignment(getCommonAlignment<LoadInst>(Values));
  else if (auto *I = dyn_cast<AllocaInst>(V))
    I->setAlignment(getCommonAlignment<AllocaInst>(Values));
  else if (auto *I = dyn_cast<AtomicCmpXchgInst>(V))
    I->setAlignment(getCommonAlignment<AtomicCmpXchgInst>(Values));
  else if (auto *I = dyn_cast<AtomicRMWInst>(V))
    I->setAlignment(getCommonAlignment<AtomicRMWInst>(Values));
}

static void getAllDeps(Instruction *I, SmallPtrSetImpl<Instruction *> &Deps) {
  if (!Deps.insert(I).second)
    return;
  const auto *BB = I->getParent();
  for (Value *Op : I->operands())
    if (auto *OpI = dyn_cast<Instruction>(Op))
      if (OpI->getParent() == BB)
        getAllDeps(OpI, Deps);
}

class LoopGenerator {
public:
  BasicBlock *BB, *PreBB, *BodyBB, *ExitBB;

  SmallVector<AllocaInst *> Allocas;
  SmallVector<GlobalVariable *> GlobalArrays;
  SmallVector<std::unique_ptr<ExternalArray>> ExternalArrays;
  SmallPtrSet<Value *, 8> ExternalValue;
  DenseMap<Value *, Value *> NewValues;

  // A map to match the value to the external array
  // use MapVector for the need to iterate through each elements later
  // DenseMap does not gurantee to be iterated in the insertion order
  MapVector<Value *, std::pair<unsigned, unsigned>> ValueToExternalArray;

  SmallVector<Instruction *> InstsInPre, InstsInExit;
  SmallVector<std::pair<Node *, SmallVector<Instruction *>>> BodyInstructions;
  SmallVector<MismatchingNode *> MismatchingNodes;
  SmallVector<RecuranceNode *> RecuranceNodes;

  // TODO: private
  LoopGenerator(BasicBlock *BB) : BB(BB) {}

  bool canBeScheduled(const AlignmentGraph &AG) {
    // TODO: Const
    SmallPtrSet<Instruction *, 4> HeaderInstructions;
    std::function<void(Node *)> Recurse;
    Recurse = [&](Node *N) {
      for (auto &Child : getChildren(N))
        Recurse(Child.get());

      switch (N->getType()) {
      case Node::Matching:
      case Node::IntegerSeq:
      case Node::NeutralPointer: {
        SmallVector<Instruction *> Instrs;
        for (auto *V : N->Values) {
          auto *I = dyn_cast<Instruction>(V);
          if (N->getType() == Node::NeutralPointer &&
              !isa<GetElementPtrInst>(V))
            I = nullptr;
          assert(!I || I->getParent() == BB);
          Instrs.push_back(I);
        }
        BodyInstructions.emplace_back(N, Instrs);
        break;
      }
      case Node::Mismatching:
        MismatchingNodes.push_back(cast<MismatchingNode>(N));
        [[fallthrough]];
      case Node::Identical:
        // TODO: Don't insert identical instructions multiple times
        for (auto *V : N->Values)
          if (auto *I = dyn_cast<Instruction>(V))
            if (I->getParent() == BB)
              HeaderInstructions.insert(I);
        break;
      case Node::Recurance: {
        auto *RN = cast<RecuranceNode>(N);
        RecuranceNodes.push_back(RN);
        if (auto *I = dyn_cast<Instruction>(RN->InitialValue))
          if (I->getParent() == BB)
            HeaderInstructions.insert(I);
        break;
      }
      }
    };

    Recurse(AG.Root.get());

    for (Instruction &I : *BB)
      if (isa<PHINode>(I))
        HeaderInstructions.insert(&I);

    SmallPtrSet<Instruction *, 4> HeaderAndDeps;
    for (auto *I : HeaderInstructions)
      getAllDeps(I, HeaderAndDeps);

    SmallPtrSet<Instruction *, 4> BodyAndDeps;
    SmallPtrSet<Instruction *, 4> BodySet; // todo
    SmallVector<Instruction *> InstsInBody;
    for (size_t J = 0; J < AG.Root->getIterationSize(); ++J) {
      for (auto &[N, Instrs] : BodyInstructions) {
        assert(AG.Root->getIterationSize() == Instrs.size());
        auto *I = Instrs[J];
        if (!I)
          continue;
        getAllDeps(I, BodyAndDeps);
        InstsInBody.push_back(I);
        BodySet.insert(I);
      }
    }

    for (Instruction &I : *BB) {
      if (HeaderAndDeps.contains(&I)) {
        if (BodySet.contains(&I)) {
          LLVM_DEBUG(dbgs() << "Failed to roll: header cannot depend on loop "
                               "body instructions\n");
          return false;
        }
        InstsInPre.push_back(&I);
      } else if (BodySet.contains(&I)) {
        if (isa<AllocaInst>(I)) {
          ++NumAllocasInBody;
          LLVM_DEBUG(dbgs()
                     << "Failed to roll: alloca instructions cannot move "
                        "to the loop body\n");
          return false;
        }
      } else {
        InstsInExit.push_back(&I);
      }
    }

    // Phi instructions must be first in the block
    std::stable_partition(InstsInPre.begin(), InstsInPre.end(),
                          [](const auto *I) { return isa<PHINode>(I); });
    std::stable_partition(InstsInExit.begin(), InstsInExit.end(),
                          [](const auto *I) { return isa<PHINode>(I); });

    LLVM_DEBUG({
      dbgs() << "Instruction Schedule:\nHeader:\n";
      for (auto *I : InstsInPre)
        dbgs() << *I << "\n";
      dbgs() << "Body:\n";
      for (auto *I : InstsInBody)
        dbgs() << *I << "\n";
      dbgs() << "Exit:\n";
      for (auto *I : InstsInExit)
        dbgs() << *I << "\n";
    });

    LLVM_DEBUG(dbgs() << "Side Effect Order:\n");
    std::queue<Instruction *> SideEffectQueue;
    for (auto &I : *BB) {
      if (I.mayHaveSideEffects() || I.mayReadOrWriteMemory()) {
        LLVM_DEBUG(dbgs() << I << "\n");
        SideEffectQueue.push(&I);
      }
    }
    for (auto &Block : {InstsInPre, InstsInBody, InstsInExit}) {
      for (auto *I : Block) {
        if (I->mayHaveSideEffects() || I->mayReadOrWriteMemory()) {
          if (SideEffectQueue.empty() || I != SideEffectQueue.front()) {
            LLVM_DEBUG(
                dbgs()
                << "Failed to roll: Cannot reorder side effect instructions\n");
            return false;
          }
          SideEffectQueue.pop();
        }
      }
    }
    if (!SideEffectQueue.empty()) {
      LLVM_DEBUG(
          dbgs()
          << "Failed to roll: Cannot reorder side effect instructions\n");
      return false;
    }
    assert(!any_of(InstsInBody, [](auto *I) { return isa<AllocaInst>(I); }));
    return true;
  }

  void generateLoop(size_t IterationSize) {
    LLVMContext &Context = BB->getContext();
    PreBB = BasicBlock::Create(Context, "loop_rolling_pre");
    BodyBB = BasicBlock::Create(Context, "loop_rolling_body");
    ExitBB = BasicBlock::Create(Context, "loop_rolling_exit");

    ExitBB->insertInto(BB->getParent(), BB);
    BodyBB->insertInto(BB->getParent(), ExitBB);
    PreBB->insertInto(BB->getParent(), BodyBB);

    Type *Ty = Type::getInt32Ty(Context);

    // insert code before the loop
    IRBuilder<> PreBuilder(PreBB);
    generatePreHeaderCode(PreBuilder);

    // insert code inside the loop
    IRBuilder<> BodyBuilder(BodyBB);
    // TODO: Use the smallest possible type for iv
    PHINode *IV = BodyBuilder.CreatePHI(Ty, 2, "iv");
    IV->addIncoming(BodyBuilder.getInt32(0), PreBB);

    markExternalUseValue();

    for (auto *RN : RecuranceNodes) {
      RN->PhiInst = BodyBuilder.CreatePHI(RN->InitialValue->getType(),
                                          IV->getNumIncomingValues());
      RN->PhiInst->addIncoming(RN->InitialValue, PreBB);
    }

    generateBodyCode(BodyBuilder, IV);

    PreBuilder.CreateBr(BodyBB);

    // Create the induction variable increment
    Value *IVNext =
        BodyBuilder.CreateAdd(IV, BodyBuilder.getInt32(1), "iv_next");
    IV->addIncoming(IVNext, BodyBB);

    // use unsigned integer compare since our counter variable is only
    // non-negative number
    Value *ExitCond = BodyBuilder.CreateICmpULT(
        IVNext, BodyBuilder.getInt32(IterationSize), "exit");
    BodyBuilder.CreateCondBr(ExitCond, BodyBB, ExitBB);

    // insert code after the loop
    IRBuilder<> ExitBuilder(ExitBB);
    generateExitCode(ExitBuilder);
  }

  static std::unique_ptr<LoopGenerator> rollBlock(const AlignmentGraph &AG,
                                                  BasicBlock &BB) {
    auto LG = std::make_unique<LoopGenerator>(&BB);
    if (!LG->canBeScheduled(AG)) {
      ++NumCannotBeScheduled;
      return {};
    }
    LG->generateLoop(AG.Root->getIterationSize());
    return LG;
  }

  void emitMismatchingArray(MismatchingNode *N, IRBuilder<> &Builder) {
    auto *F = Builder.GetInsertBlock()->getParent();
    auto *Ty = N->Values.front()->getType();
    auto *ArrayTy = ArrayType::get(Ty, N->getIterationSize());
    if (N->UseGlobalArray) {
      // convert values from Value* to Constant*
      SmallVector<Constant *> Elements;
      for (Value *V : N->Values)
        Elements.push_back(cast<Constant>(V));

      // Use InternalLinkage to permit access from only this module
      auto *M = F->getParent();
      auto *GlobalArray = new GlobalVariable(
          *M, ArrayTy, true, GlobalValue::InternalLinkage,
          ConstantArray::get(ArrayTy, Elements), "integer_sequence_array");
      GlobalArrays.push_back(GlobalArray);
      N->Array = GlobalArray;
      return;
    }

    auto &Entry = F->getEntryBlock();
    const DataLayout &DL = Entry.getDataLayout();

    // create the mismatching stack array
    auto *NewMismatchingArray =
        new AllocaInst(ArrayTy, DL.getProgramAddressSpace(),
                       /*ArraySize=*/nullptr, DL.getPrefTypeAlign(ArrayTy),
                       "mismatching_array", /*InsertBefore=*/nullptr);
    Allocas.push_back(NewMismatchingArray);

    for (auto [Index, V] : llvm::enumerate(N->Values)) {
      auto *Ptr =
          Builder.CreateGEP(ArrayTy, NewMismatchingArray,
                            {Builder.getInt32(0), Builder.getInt32(Index)});
      Builder.CreateStore(V, Ptr);
    }

    N->Array = NewMismatchingArray;
  }

  void generatePreHeaderCode(IRBuilder<> &Builder) {
    for (Instruction *I : InstsInPre)
      NewValues[I] = Builder.Insert(I->clone());

    for (auto *MN : MismatchingNodes)
      emitMismatchingArray(MN, Builder);
  }

  bool shouldSaveValue(const Node *N, const Value *V) const {
    if (NewValues.contains(V))
      return false;
    if (isa<NeutralPointerNode>(N) && !isa<GetElementPtrInst>(V))
      return false;
    auto *I = cast<Instruction>(V);
    auto *BB = I->getParent();
    if (I->isUsedOutsideOfBlock(BB))
      return true;

    // check for phi instruction in succeeding BBs if they use V
    // (isUsedOutsideOfBlock cannot detect that)
    // since V is guaranteed to be in the BB -> V-users() can only be
    // instructions in succeeding BBs
    for (const User *U : I->users())
      if (auto *PN = dyn_cast<PHINode>(U))
        if (PN->getParent() != BB)
          return true;

    // TODO: What is left to handle above? Do I need this?
    return ExternalValue.contains(V);
  }

  bool shouldSaveValues(const Node *N) {
    if (isa<IntegerSeqNode>(N))
      return false;
    bool ShouldSave = false;
    for (auto [Index, V] : llvm::enumerate(N->Values)) {
      if (shouldSaveValue(N, V)) {
        ValueToExternalArray[V] = {ExternalArrays.size(), Index};
        ShouldSave = true;
      }
    }
    return ShouldSave;
  }

  Value *emitMatchingNode(const MatchingNode *N, IRBuilder<> &Builder,
                          PHINode *IV, DenseMap<Node *, Value *> &NodeToValue) {
    auto *F = Builder.GetInsertBlock()->getParent();
    auto &Entry = F->getEntryBlock();
    const DataLayout &DL = Entry.getDataLayout();
    // copy operands first then instruction
    SmallVector<Value *> NewOperands;
    for (const auto &Child : N->Children) {
      Value *Op;
      if (auto *MN = dyn_cast<MismatchingNode>(Child.get())) {
        Op = emitMismatchingNode(MN, Builder, IV);
      } else if (auto *RN = dyn_cast<RecuranceNode>(Child.get())) {
        Op = RN->PhiInst;
      } else if (isa<IdenticalNode>(Child.get())) {
        Op = Child->Values.front();
      } else {
        Op = NodeToValue.at(Child.get());
      }
      NewOperands.push_back(Op);
    }

    // TODO: Merge debug info
    Instruction *ClonedInst = nullptr;
    if (auto *NN = dyn_cast<NeutralPointerNode>(N)) {
      // clone the GEP instruction or GEP constant expr
      if (NN->GEPInst)
        ClonedInst = NN->GEPInst->clone();
      else
        ClonedInst = NN->GEPConstExpr->getAsInstruction();
    } else {
      ClonedInst = cast<Instruction>(N->Values.front())->clone();
      setCommonAlignment(ClonedInst, N->Values);
    }

    // replace old operands with new operands
    for (auto [Index, NewOperand] : llvm::enumerate(NewOperands))
      ClonedInst->setOperand(Index, NewOperand);
    Builder.Insert(ClonedInst);

    for (const auto &Child : N->Children) {
      if (auto *RN = dyn_cast<RecuranceNode>(Child.get())) {
        RN->PhiInst->addIncoming(ClonedInst, Builder.GetInsertBlock());
      }
    }

    // check if there is a need to create a external array to store output
    if (!shouldSaveValues(N))
      return ClonedInst;

    // use preheader builder to create array
    auto *ArrayTy =
        ArrayType::get(N->Values.front()->getType(), N->getIterationSize());
    auto *NewMismatchingArray =
        new AllocaInst(ArrayTy, DL.getProgramAddressSpace(),
                       /*ArraySize=*/nullptr, DL.getPrefTypeAlign(ArrayTy),
                       "external_array", /*InsertBefore=*/nullptr);
    Allocas.push_back(NewMismatchingArray);

    // keep track of how many external arrays are created
    ExternalArrays.push_back(
        std::make_unique<ExternalArray>(ArrayTy, NewMismatchingArray));

    // use the new operand to store the value
    // TODO: Use CreateInBoundsGEP()
    Value *NewPtr = Builder.CreateGEP(ArrayTy, NewMismatchingArray,
                                      {Builder.getInt32(0), IV});
    Builder.CreateStore(ClonedInst, NewPtr);
    return ClonedInst;
  }

  Value *emitMismatchingNode(const MismatchingNode *N, IRBuilder<> &Builder,
                             PHINode *IV) {
    auto *Ty = N->Values.front()->getType();
    auto *ArrayTy = ArrayType::get(Ty, N->getIterationSize());
    auto *Ptr = Builder.CreateGEP(ArrayTy, N->Array, {Builder.getInt32(0), IV});
    return Builder.CreateLoad(Ty, Ptr);
  }

  Value *emitIntegerSeqNode(const IntegerSeqNode *N, IRBuilder<> &Builder,
                            PHINode *IV) const {
    auto *CorrecTypeIV = Builder.CreateIntCast(
        IV, N->Values.front()->getType(), /*isSigned=*/true, "corrected_iv");
    auto *Mul = Builder.CreateMul(
        CorrecTypeIV,
        ConstantInt::get(CorrecTypeIV->getType(), N->Step.getSExtValue()));
    return Builder.CreateAdd(Mul, ConstantInt::get(CorrecTypeIV->getType(),
                                                   N->Start.getSExtValue()));
  }

  void generateBodyCode(IRBuilder<> &Builder, PHINode *IV) {
    // TODO: Add NewValue field in node
    DenseMap<Node *, Value *> NodeToValue;
    for (auto &[N, Instrs] : BodyInstructions) {
      if (auto *IN = dyn_cast<IntegerSeqNode>(N)) {
        NodeToValue[N] = emitIntegerSeqNode(IN, Builder, IV);
      } else if (auto *MN = dyn_cast<MatchingNode>(N)) {
        NodeToValue[N] = emitMatchingNode(MN, Builder, IV, NodeToValue);
      } else {
        llvm_unreachable("");
      }
    }
  }

  void generateExitCode(IRBuilder<> &Builder) {
    // create all the GEP and load output mismatching arrays first
    // TODO: If only the last value in the external array is used, we might be
    // able to remove the array
    for (auto [V, ExternalArray] : ValueToExternalArray) {
      auto [ArrayNumber, OperandIndex] = ExternalArray;
      ArrayType *ArrayTy = ExternalArrays[ArrayNumber]->ArrayTy;
      Value *ArrayAlloc = ExternalArrays[ArrayNumber]->ArrayAlloc;

      Value *Ptr = Builder.CreateGEP(
          ArrayTy, ArrayAlloc,
          {Builder.getInt32(0), Builder.getInt32(OperandIndex)});

      NewValues[V] = Builder.CreateLoad(ArrayTy->getElementType(), Ptr);
    }

    for (Instruction *I : InstsInExit)
      NewValues[I] = Builder.Insert(I->clone());
  }

  void markExternalUseValue() {
    for (Instruction *I : InstsInExit)
      for (Value *Operand : I->operands())
        if (isa<Instruction>(Operand))
          ExternalValue.insert(Operand);
  }

  InstructionCost getRollCost(TargetTransformInfo &TTI) {
    InstructionCost NewCost = 0;
    // TODO: Also GlobalArrays
    // for (auto *AI : Allocas)
    //   NewCost += TTI.getInstructionCost(AI,
    //   TargetTransformInfo::TCK_CodeSize);
    NewCost += calculateCodeSize(PreBB, TTI);
    NewCost += calculateCodeSize(BodyBB, TTI);
    NewCost += calculateCodeSize(ExitBB, TTI);

    auto OldCost = calculateCodeSize(BB, TTI);
    return NewCost - OldCost;
  }

  InstructionCost calculateCodeSize(BasicBlock *BB, TargetTransformInfo &TTI) {
    InstructionCost TotalCost = 0;
    for (auto &I : *BB)
      TotalCost +=
          TTI.getInstructionCost(&I, TargetTransformInfo::TCK_CodeSize);
    return TotalCost;
  }

  void replaceBlockWithRolled() {
    LLVM_DEBUG({
      dbgs() << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
      BB->dump();
      dbgs() << "==================================================\n";
      for (auto *GA : GlobalArrays)
        GA->dump();
      PreBB->dump();
      BodyBB->dump();
      ExitBB->dump();
      dbgs() << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n";
    });
    for (auto *AI : Allocas)
      AI->insertBefore(BB->getParent()->getEntryBlock().getFirstInsertionPt());
    for (auto [OldVal, NewVal] : NewValues)
      OldVal->replaceAllUsesWith(NewVal);

    BB->replaceAllUsesWith(PreBB);
    BB->replaceSuccessorsPhiUsesWith(PreBB, ExitBB);
    BB->eraseFromParent();
  }

  void destroyRolled() {
    PreBB->getTerminator()->eraseFromParent();
    BodyBB->getTerminator()->eraseFromParent();
    ExitBB->eraseFromParent();
    BodyBB->eraseFromParent();
    PreBB->eraseFromParent();
    for (auto *GA : GlobalArrays)
      GA->eraseFromParent();
    for (auto *AI : Allocas)
      AI->deleteValue();
  }
};

static bool runOnBlock(BasicBlock &BB, TargetTransformInfo &TTI) {
  LLVM_DEBUG(dbgs() << "Attempting to roll block " << BB.getName()
                    << " of function " << BB.getParent()->getName() << "\n");
  // TODO: Support landing pads
  if (BB.isLandingPad()) {
    ++NumLandingPadBlocksSkipped;
    return false;
  }

  DenseMap<std::pair<Type *, Value *>, SmallVector<Value *>> StoreSeeds;
  DenseMap<std::pair<Function *, unsigned>, SmallVector<Value *>> CallSeeds;
  DenseMap<std::pair<Type *, unsigned>, SetVector<Value *>> ReductionNodes;
  for (Instruction &I : BB) {
    if (auto *SI = dyn_cast<StoreInst>(&I)) {
      Type *DataType = SI->getValueOperand()->getType();
      Value *Addr = getUnderlyingObject(SI->getPointerOperand());
      StoreSeeds[std::make_pair(DataType, Addr)].push_back(SI);
      ++StoreInstructions;
    } else if (auto *CI = dyn_cast<CallBase>(&I)) {
      // CI->getCalledFunction() can be null for indirect or virtual
      // calls, handle later
      auto *CalledFunction = CI->getCalledFunction();
      if (CalledFunction == nullptr || CalledFunction->isIntrinsic())
        continue;
      CallSeeds[std::make_pair(CalledFunction, CI->getNumOperands())].push_back(
          CI);
      ++CallInstructions;
    } else if (I.isBinaryOp() &&
               ConstantExpr::getBinOpIdentity(I.getOpcode(), I.getType())) {
      // ReductionNodes[std::make_pair(I.getType(), I.getOpcode())].insert(&I);
    }
  }

  DenseSet<Value *> Visited;
  DenseMap<Value *, DenseSet<Value *>> ConnectedComponents;
  DenseSet<Value *> Connected;
  DenseSet<Value *> NewSeed;
  // Root to seed
  DenseMap<Value *, SmallVector<Value *>> ReductionSeeds;
  for (auto &Node : ReductionNodes) {
    auto &Values = Node.second;
    Visited.clear();
    ConnectedComponents.clear();

    std::function<void(Value *, DenseSet<Value *> &)> FindComponents;
    FindComponents = [&](Value *V, DenseSet<Value *> &ConnectedValues) {
      for (auto *Op : cast<Instruction>(V)->operand_values()) {
        if (Values.contains(Op)) {
          ConnectedValues.insert(Op);
          Visited.insert(Op);
          FindComponents(Op, ConnectedValues);
        }
      }
    };
    for (auto *V : llvm::reverse(Values)) {
      if (Visited.contains(V))
        continue;
      Connected.clear();
      FindComponents(V, Connected);
      if (Connected.empty())
        continue;
      auto AllMatching = llvm::all_of(
          Connected, [&](auto *V2) { return areMatchingValues(V, V2); });
      if (!AllMatching)
        continue;
      Connected.insert(V);
      ConnectedComponents[V] = Connected;
    }

    LLVM_DEBUG(for (auto &[Root, Values] : ConnectedComponents) {
      dbgs() << "CC: " << *Root;
      for (auto *V : Values)
        dbgs() << *V;
      dbgs() << "\n";
    });
    for (auto &[Root, Values] : ConnectedComponents) {
      NewSeed.clear();
      for (auto *V : Values)
        for (auto *Op : cast<Instruction>(V)->operand_values())
          if (!Values.contains(Op))
            NewSeed.insert(Op);
      // Seed instructions should be ordered the same as they are in the block
      for (Instruction &I : BB)
        if (NewSeed.contains(&I))
          ReductionSeeds[Root].push_back(&I);
    }
  }

  auto BestCost = LoopRollingIgnoreCost
                      ? InstructionCost::getMax()
                      : InstructionCost(LoopRollingCostThreshold);
  std::unique_ptr<LoopGenerator> BestLG;
  auto RollSeed = [&](ArrayRef<Value *> Values,
                      Value *ReductionNode = nullptr) {
    // TODO: Use ReductionNode
    if (Values.size() <= 1)
      return;
    AlignmentGraph AG(Values, BB);
    if (!AG.Root)
      return;
    LLVM_DEBUG(AG.debugPrint(AG.Root));

    if (!LoopRollingDumpAlignmentGraph.empty())
      AG.dumpDOTGraph((LoopRollingDumpAlignmentGraph + "/AlignmentGraph-" +
                       BB.getParent()->getName() + "-" + BB.getName() + ".dot")
                          .str());

    auto LG = LoopGenerator::rollBlock(AG, BB);
    if (!LG)
      return;

    auto Cost = LG->getRollCost(TTI);
    if (Cost >= BestCost) {
      LG->destroyRolled();
      return;
    }
    BestCost = Cost;
    if (BestLG)
      BestLG->destroyRolled();
    BestLG = std::move(LG);
  };

  for (auto &[_, Values] : StoreSeeds)
    RollSeed(Values);
  for (auto &[_, Values] : CallSeeds)
    RollSeed(Values);
  for (auto &[Root, Values] : ReductionSeeds)
    RollSeed(Values, Root);

  if (!BestLG)
    return false;
  ++LoopRollingFuctions;
  // Cost is usually negative
  TotalCostReduced -= BestCost.getValue();
  BestLG->replaceBlockWithRolled();
  return true;
}

PreservedAnalyses LoopRollingPass::run(Function &F,
                                       FunctionAnalysisManager &AM) {
  if (F.isDeclaration() || F.isIntrinsic())
    return PreservedAnalyses::all();
  // TODO: Correctly handle debug info

  auto &TTI = AM.getResult<TargetIRAnalysis>(F);

  bool HasChanged = false;
  for (BasicBlock &BB : make_early_inc_range(F))
    HasChanged |= runOnBlock(BB, TTI);

  if (!HasChanged)
    return PreservedAnalyses::all();

#ifndef NDEBUG
  if (verifyFunction(F, &errs())) {
    F.dump();
    assert(false && "Failed to verify rolled function");
  }
#endif
  return PreservedAnalyses::none();
}
