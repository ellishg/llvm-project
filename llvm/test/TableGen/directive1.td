// RUN: llvm-tblgen -gen-directive-decl -I %p/../../include %s | FileCheck -match-full-lines %s
// RUN: llvm-tblgen -gen-directive-impl -I %p/../../include %s | FileCheck -match-full-lines %s -check-prefix=IMPL

include "llvm/Frontend/Directive/DirectiveBase.td"

def TestDirectiveLanguage : DirectiveLanguage {
  let name = "Tdl";

  let cppNamespace = "tdl";
  let directivePrefix = "TDLD_";
  let clausePrefix = "TDLC_";
  let makeEnumAvailableInNamespace = 1;
  let enableBitmaskEnumInNamespace = 1;
  let flangClauseBaseClass = "TdlClause";
}

def TDLCV_vala : EnumVal<"vala",1,1> {}
def TDLCV_valb : EnumVal<"valb",2,1> {}
def TDLCV_valc : EnumVal<"valc",3,0> { let isDefault = 1; }

def TDLC_ClauseA : Clause<[Spelling<"clausea">]> {
  let enumClauseValue = "AKind";
  let allowedClauseValues = [
    TDLCV_vala,
    TDLCV_valb,
    TDLCV_valc
  ];
}

def TDLC_ClauseB : Clause<[Spelling<"clauseb">]> {
  let flangClass = "IntExpr";
  let isValueOptional = 1;
  let isDefault = 1;
}

def TDLC_ClauseC : Clause<[Spelling<"clausec">, Spelling<"ccccccc">]> {
  let flangClass = "IntExpr";
  let isValueList = 1;
}

def TDL_DirA : Directive<[Spelling<"dira">]> {
  let allowedClauses = [
    VersionedClause<TDLC_ClauseA>,
    VersionedClause<TDLC_ClauseB>
  ];
  let isDefault = 1;
  let association = AS_None;
  let category = CA_Executable;
}

// CHECK:       #ifndef LLVM_Tdl_INC
// CHECK-NEXT:  #define LLVM_Tdl_INC
// CHECK-EMPTY:
// CHECK-NEXT:  #include "llvm/ADT/ArrayRef.h"
// CHECK-NEXT:  #include "llvm/ADT/BitmaskEnum.h"
// CHECK-NEXT:  #include "llvm/ADT/Sequence.h"
// CHECK-NEXT:  #include "llvm/ADT/StringRef.h"
// CHECK-NEXT:  #include "llvm/Frontend/Directive/Spelling.h"
// CHECK-NEXT:  #include "llvm/Support/Compiler.h"
// CHECK-NEXT:  #include <cstddef>
// CHECK-NEXT:  #include <utility>
// CHECK-EMPTY:
// CHECK-NEXT:  namespace llvm {
// CHECK-NEXT:  namespace tdl {
// CHECK-EMPTY:
// CHECK-NEXT:  LLVM_ENABLE_BITMASK_ENUMS_IN_NAMESPACE();
// CHECK-EMPTY:
// CHECK-NEXT:  enum class Association {
// CHECK-NEXT:    Block,
// CHECK-NEXT:    First_ = Block,
// CHECK-NEXT:    Declaration,
// CHECK-NEXT:    Delimited,
// CHECK-NEXT:    Loop,
// CHECK-NEXT:    None,
// CHECK-NEXT:    Separating,
// CHECK-NEXT:    Last_ = Separating,
// CHECK-NEXT:  };
// CHECK-EMPTY:
// CHECK-NEXT:  static constexpr std::size_t Association_enumSize = 6;
// CHECK-EMPTY:
// CHECK-NEXT:  enum class Category {
// CHECK-NEXT:    Declarative,
// CHECK-NEXT:    First_ = Declarative,
// CHECK-NEXT:    Executable,
// CHECK-NEXT:    Informational,
// CHECK-NEXT:    Meta,
// CHECK-NEXT:    Subsidiary,
// CHECK-NEXT:    Utility,
// CHECK-NEXT:    Last_ = Utility,
// CHECK-NEXT:  };
// CHECK-EMPTY:
// CHECK-NEXT:  static constexpr std::size_t Category_enumSize = 6;
// CHECK-EMPTY:
// CHECK-NEXT:  enum class SourceLanguage : uint32_t {
// CHECK-NEXT:    C = 1U,
// CHECK-NEXT:    Fortran = 2U,
// CHECK-NEXT:    LLVM_MARK_AS_BITMASK_ENUM(/*LargestValue=*/Fortran)
// CHECK-NEXT:  };
// CHECK-EMPTY:
// CHECK-NEXT:  static constexpr std::size_t SourceLanguage_enumSize = 2;
// CHECK-EMPTY:
// CHECK-NEXT:  enum class Directive {
// CHECK-NEXT:    TDLD_dira,
// CHECK-NEXT:    First_ = TDLD_dira,
// CHECK-NEXT:    Last_ = TDLD_dira,
// CHECK-NEXT:  };
// CHECK-EMPTY:
// CHECK-NEXT:  static constexpr std::size_t Directive_enumSize = 1;
// CHECK-EMPTY:
// CHECK-NEXT:  constexpr auto TDLD_dira = Directive::TDLD_dira;
// CHECK-EMPTY:
// CHECK-NEXT:  enum class Clause {
// CHECK-NEXT:    TDLC_clausea,
// CHECK-NEXT:    First_ = TDLC_clausea,
// CHECK-NEXT:    TDLC_clauseb,
// CHECK-NEXT:    TDLC_clausec,
// CHECK-NEXT:    Last_ = TDLC_clausec,
// CHECK-NEXT:  };
// CHECK-EMPTY:
// CHECK-NEXT:  static constexpr std::size_t Clause_enumSize = 3;
// CHECK-EMPTY:
// CHECK-NEXT:  constexpr auto TDLC_clausea = Clause::TDLC_clausea;
// CHECK-NEXT:  constexpr auto TDLC_clauseb = Clause::TDLC_clauseb;
// CHECK-NEXT:  constexpr auto TDLC_clausec = Clause::TDLC_clausec;
// CHECK-EMPTY:
// CHECK-NEXT:  enum class AKind {
// CHECK-NEXT:    TDLCV_vala=1,
// CHECK-NEXT:    TDLCV_valb=2,
// CHECK-NEXT:    TDLCV_valc=3,
// CHECK-NEXT:  };
// CHECK-EMPTY:
// CHECK-NEXT:  constexpr auto TDLCV_vala = AKind::TDLCV_vala;
// CHECK-NEXT:  constexpr auto TDLCV_valb = AKind::TDLCV_valb;
// CHECK-NEXT:  constexpr auto TDLCV_valc = AKind::TDLCV_valc;
// CHECK-EMPTY:
// CHECK-NEXT:  // Enumeration helper functions
// CHECK-NEXT:  LLVM_ABI std::pair<Directive, directive::VersionRange> getTdlDirectiveKindAndVersions(StringRef Str);
// CHECK-NEXT:  inline Directive getTdlDirectiveKind(StringRef Str) {
// CHECK-NEXT:   return getTdlDirectiveKindAndVersions(Str).first;
// CHECK-NEXT:  }
// CHECK-EMPTY:
// CHECK-NEXT:  LLVM_ABI StringRef getTdlDirectiveName(Directive D, unsigned Ver = 0);
// CHECK-EMPTY:
// CHECK-NEXT:  LLVM_ABI std::pair<Clause, directive::VersionRange> getTdlClauseKindAndVersions(StringRef Str);
// CHECK-EMPTY:
// CHECK-NEXT:  inline Clause getTdlClauseKind(StringRef Str) {
// CHECK-NEXT:    return getTdlClauseKindAndVersions(Str).first;
// CHECK-NEXT:  }
// CHECK-EMPTY:
// CHECK-NEXT:  LLVM_ABI StringRef getTdlClauseName(Clause C, unsigned Ver = 0);
// CHECK-EMPTY:
// CHECK-NEXT:  /// Return true if \p C is a valid clause for \p D in version \p Version.
// CHECK-NEXT:  LLVM_ABI bool isAllowedClauseForDirective(Directive D, Clause C, unsigned Version);
// CHECK-EMPTY:
// CHECK-NEXT:  constexpr std::size_t getMaxLeafCount() { return 0; }
// CHECK-NEXT:  LLVM_ABI Association getDirectiveAssociation(Directive D);
// CHECK-NEXT:  LLVM_ABI Category getDirectiveCategory(Directive D);
// CHECK-NEXT:  LLVM_ABI SourceLanguage getDirectiveLanguages(Directive D);
// CHECK-NEXT:  LLVM_ABI AKind getAKind(StringRef Str);
// CHECK-NEXT:  LLVM_ABI StringRef getTdlAKindName(AKind x);
// CHECK-EMPTY:
// CHECK-NEXT:  } // namespace tdl
// CHECK-EMPTY:
// CHECK-NEXT:  template <> struct enum_iteration_traits<tdl::Association> {
// CHECK-NEXT:    static constexpr bool is_iterable = true;
// CHECK-NEXT:  };
// CHECK-EMPTY:
// CHECK-NEXT:  template <> struct enum_iteration_traits<tdl::Category> {
// CHECK-NEXT:    static constexpr bool is_iterable = true;
// CHECK-NEXT:  };
// CHECK-EMPTY:
// CHECK-NEXT:  template <> struct enum_iteration_traits<tdl::Directive> {
// CHECK-NEXT:    static constexpr bool is_iterable = true;
// CHECK-NEXT:  };
// CHECK-EMPTY:
// CHECK-NEXT:  template <> struct enum_iteration_traits<tdl::Clause> {
// CHECK-NEXT:    static constexpr bool is_iterable = true;
// CHECK-NEXT:  };
// CHECK-NEXT:  } // namespace llvm
// CHECK-NEXT:  #endif // LLVM_Tdl_INC


// IMPL:       #ifdef GEN_FLANG_DIRECTIVE_CLAUSE_SETS
// IMPL-NEXT:  #undef GEN_FLANG_DIRECTIVE_CLAUSE_SETS
// IMPL-EMPTY:
// IMPL-NEXT:  namespace llvm {
// IMPL-NEXT:  namespace tdl {
// IMPL-EMPTY:
// IMPL-NEXT:    // Sets for dira
// IMPL-EMPTY:
// IMPL-NEXT:    static  allowedClauses_TDLD_dira {
// IMPL-NEXT:      Clause::TDLC_clausea,
// IMPL-NEXT:      Clause::TDLC_clauseb,
// IMPL-NEXT:    };
// IMPL-EMPTY:
// IMPL-NEXT:    static  allowedOnceClauses_TDLD_dira {
// IMPL-NEXT:    };
// IMPL-EMPTY:
// IMPL-NEXT:    static  allowedExclusiveClauses_TDLD_dira {
// IMPL-NEXT:    };
// IMPL-EMPTY:
// IMPL-NEXT:    static  requiredClauses_TDLD_dira {
// IMPL-NEXT:    };
// IMPL-NEXT:  } // namespace tdl
// IMPL-NEXT:  } // namespace llvm
// IMPL-EMPTY:
// IMPL-NEXT:  #endif // GEN_FLANG_DIRECTIVE_CLAUSE_SETS
// IMPL-EMPTY:
// IMPL-NEXT:  #ifdef GEN_FLANG_DIRECTIVE_CLAUSE_MAP
// IMPL-NEXT:  #undef GEN_FLANG_DIRECTIVE_CLAUSE_MAP
// IMPL-EMPTY:
// IMPL-NEXT:  {
// IMPL-NEXT:    {llvm::tdl::Directive::TDLD_dira,
// IMPL-NEXT:      {
// IMPL-NEXT:        llvm::tdl::allowedClauses_TDLD_dira,
// IMPL-NEXT:        llvm::tdl::allowedOnceClauses_TDLD_dira,
// IMPL-NEXT:        llvm::tdl::allowedExclusiveClauses_TDLD_dira,
// IMPL-NEXT:        llvm::tdl::requiredClauses_TDLD_dira,
// IMPL-NEXT:      }
// IMPL-NEXT:    },
// IMPL-NEXT:  }
// IMPL-EMPTY:
// IMPL-NEXT:  #endif // GEN_FLANG_DIRECTIVE_CLAUSE_MAP
// IMPL-EMPTY:
// IMPL-NEXT:  #ifdef GEN_FLANG_CLAUSE_PARSER_CLASSES
// IMPL-NEXT:  #undef GEN_FLANG_CLAUSE_PARSER_CLASSES
// IMPL-EMPTY:
// IMPL-NEXT:  EMPTY_CLASS(Clausea);
// IMPL-NEXT:  WRAPPER_CLASS(Clauseb, std::optional<IntExpr>);
// IMPL-NEXT:  WRAPPER_CLASS(Clausec, std::list<IntExpr>);
// IMPL-EMPTY:
// IMPL-NEXT:  #endif // GEN_FLANG_CLAUSE_PARSER_CLASSES
// IMPL-EMPTY:
// IMPL-NEXT:  #ifdef GEN_FLANG_CLAUSE_PARSER_CLASSES_LIST
// IMPL-NEXT:  #undef GEN_FLANG_CLAUSE_PARSER_CLASSES_LIST
// IMPL-EMPTY:
// IMPL-NEXT:  Clausea
// IMPL-NEXT:  , Clauseb
// IMPL-NEXT:  , Clausec
// IMPL-EMPTY:
// IMPL-NEXT:  #endif // GEN_FLANG_CLAUSE_PARSER_CLASSES_LIST
// IMPL-EMPTY:
// IMPL-NEXT:  #ifdef GEN_FLANG_DUMP_PARSE_TREE_CLAUSES
// IMPL-NEXT:  #undef GEN_FLANG_DUMP_PARSE_TREE_CLAUSES
// IMPL-EMPTY:
// IMPL-NEXT:  NODE(TdlClause, Clausea)
// IMPL-NEXT:  NODE(TdlClause, Clauseb)
// IMPL-NEXT:  NODE(TdlClause, Clausec)
// IMPL-EMPTY:
// IMPL-NEXT:  #endif // GEN_FLANG_DUMP_PARSE_TREE_CLAUSES
// IMPL-EMPTY:
// IMPL-NEXT:  #ifdef GEN_FLANG_CLAUSE_UNPARSE
// IMPL-NEXT:  #undef GEN_FLANG_CLAUSE_UNPARSE
// IMPL-EMPTY:
// IMPL-NEXT:  void Before(const TdlClause::Clausea &) { Word("CLAUSEA"); }
// IMPL-NEXT:  void Unparse(const TdlClause::Clauseb &x) {
// IMPL-NEXT:    Word("CLAUSEB");
// IMPL-NEXT:    Walk("(", x.v, ")");
// IMPL-NEXT:  }
// IMPL-NEXT:  void Unparse(const TdlClause::Clausec &x) {
// IMPL-NEXT:    Word("CLAUSEC");
// IMPL-NEXT:    Put("(");
// IMPL-NEXT:    Walk(x.v, ",");
// IMPL-NEXT:    Put(")");
// IMPL-NEXT:  }
// IMPL-EMPTY:
// IMPL-NEXT:  #endif // GEN_FLANG_CLAUSE_UNPARSE
// IMPL-EMPTY:
// IMPL-NEXT:  #ifdef GEN_FLANG_CLAUSE_CHECK_ENTER
// IMPL-NEXT:  #undef GEN_FLANG_CLAUSE_CHECK_ENTER
// IMPL-EMPTY:
// IMPL-NEXT:  void Enter(const parser::TdlClause::Clausea &);
// IMPL-NEXT:  void Enter(const parser::TdlClause::Clauseb &);
// IMPL-NEXT:  void Enter(const parser::TdlClause::Clausec &);
// IMPL-EMPTY:
// IMPL-NEXT:  #endif // GEN_FLANG_CLAUSE_CHECK_ENTER
// IMPL-EMPTY:
// IMPL-NEXT:  #ifdef GEN_FLANG_CLAUSE_PARSER_KIND_MAP
// IMPL-NEXT:  #undef GEN_FLANG_CLAUSE_PARSER_KIND_MAP
// IMPL-EMPTY:
// IMPL-NEXT:  if constexpr (std::is_same_v<A, parser::TdlClause::Clausea>)
// IMPL-NEXT:    return llvm::tdl::Clause::TDLC_clausea;
// IMPL-NEXT:  if constexpr (std::is_same_v<A, parser::TdlClause::Clauseb>)
// IMPL-NEXT:    return llvm::tdl::Clause::TDLC_clauseb;
// IMPL-NEXT:  if constexpr (std::is_same_v<A, parser::TdlClause::Clausec>)
// IMPL-NEXT:    return llvm::tdl::Clause::TDLC_clausec;
// IMPL-NEXT:  llvm_unreachable("Invalid Tdl Parser clause");
// IMPL-EMPTY:
// IMPL-NEXT:  #endif // GEN_FLANG_CLAUSE_PARSER_KIND_MAP
// IMPL-EMPTY:
// IMPL-NEXT: #ifdef GEN_FLANG_CLAUSES_PARSER
// IMPL-NEXT: #undef GEN_FLANG_CLAUSES_PARSER
// IMPL-EMPTY:
// IMPL-NEXT:  TYPE_PARSER(
// IMPL-NEXT:    "clausec" >> construct<TdlClause>(construct<TdlClause::Clausec>(parenthesized(nonemptyList(Parser<IntExpr>{})))) || 
// IMPL-NEXT:    "clauseb" >> construct<TdlClause>(construct<TdlClause::Clauseb>(maybe(parenthesized(Parser<IntExpr>{})))) ||
// IMPL-NEXT:    "clausea" >> construct<TdlClause>(construct<TdlClause::Clausea>()) ||
// IMPL-NEXT:    "ccccccc" >> construct<TdlClause>(construct<TdlClause::Clausec>(parenthesized(nonemptyList(Parser<IntExpr>{}))))
// IMPL-NEXT:  )
// IMPL-EMPTY:
// IMPL-NEXT:  #endif // GEN_FLANG_CLAUSES_PARSER
// IMPL-EMPTY:
// IMPL-NEXT:  #ifdef GEN_CLANG_CLAUSE_CLASS
// IMPL-NEXT:  #undef GEN_CLANG_CLAUSE_CLASS
// IMPL-EMPTY:
// IMPL-NEXT:  #ifndef CLAUSE
// IMPL-NEXT:  #define CLAUSE(Enum, Str, Implicit)
// IMPL-NEXT:  #endif
// IMPL-NEXT:  #ifndef CLAUSE_CLASS
// IMPL-NEXT:  #define CLAUSE_CLASS(Enum, Str, Class)
// IMPL-NEXT:  #endif
// IMPL-NEXT:  #ifndef CLAUSE_NO_CLASS
// IMPL-NEXT:  #define CLAUSE_NO_CLASS(Enum, Str)
// IMPL-NEXT:  #endif
// IMPL-EMPTY:
// IMPL-NEXT:  #define __CLAUSE(Name, Class)                      \
// IMPL-NEXT:    CLAUSE(TDLC_##Name, #Name, /* Implicit */ false) \
// IMPL-NEXT:    CLAUSE_CLASS(TDLC_##Name, #Name, Class)
// IMPL-NEXT:  #define __CLAUSE_NO_CLASS(Name)                    \
// IMPL-NEXT:    CLAUSE(TDLC_##Name, #Name, /* Implicit */ false) \
// IMPL-NEXT:    CLAUSE_NO_CLASS(TDLC_##Name, #Name)
// IMPL-NEXT:  #define __IMPLICIT_CLAUSE_CLASS(Name, Str, Class)  \
// IMPL-NEXT:    CLAUSE(TDLC_##Name, Str, /* Implicit */ true)    \
// IMPL-NEXT:    CLAUSE_CLASS(TDLC_##Name, Str, Class)
// IMPL-NEXT:  #define __IMPLICIT_CLAUSE_NO_CLASS(Name, Str)      \
// IMPL-NEXT:    CLAUSE(TDLC_##Name, Str, /* Implicit */ true)    \
// IMPL-NEXT:    CLAUSE_NO_CLASS(TDLC_##Name, Str)
// IMPL-EMPTY:
// IMPL-NEXT:  __CLAUSE_NO_CLASS(clausea)
// IMPL-NEXT:  __CLAUSE_NO_CLASS(clauseb)
// IMPL-NEXT:  __CLAUSE_NO_CLASS(clausec)
// IMPL-EMPTY:
// IMPL-NEXT:  #undef __IMPLICIT_CLAUSE_NO_CLASS
// IMPL-NEXT:  #undef __IMPLICIT_CLAUSE_CLASS
// IMPL-NEXT:  #undef __CLAUSE_NO_CLASS
// IMPL-NEXT:  #undef __CLAUSE
// IMPL-NEXT:  #undef CLAUSE_NO_CLASS
// IMPL-NEXT:  #undef CLAUSE_CLASS
// IMPL-NEXT:  #undef CLAUSE
// IMPL-EMPTY:
// IMPL-NEXT:  #endif // GEN_CLANG_CLAUSE_CLASS
// IMPL-EMPTY:

// IMPL:       #ifdef GEN_DIRECTIVES_IMPL
// IMPL-NEXT:  #undef GEN_DIRECTIVES_IMPL
// IMPL-EMPTY:
// IMPL-NEXT:  #include "llvm/Frontend/Directive/Spelling.h"
// IMPL-NEXT:  #include "llvm/Support/ErrorHandling.h"
// IMPL-NEXT:  #include <utility>
// IMPL-EMPTY:
// IMPL-NEXT: std::pair<llvm::tdl::Directive, llvm::directive::VersionRange> llvm::tdl::getTdlDirectiveKindAndVersions(llvm::StringRef Str) {
// IMPL-NEXT:   directive::VersionRange All; // Default-initialized to "all versions"
// IMPL-NEXT:   return StringSwitch<std::pair<Directive, directive::VersionRange>>(Str)
// IMPL-NEXT:     .Case("dira", {TDLD_dira, All})
// IMPL-NEXT:     .Default({TDLD_dira, All});
// IMPL-NEXT: }
// IMPL-EMPTY:
// IMPL-NEXT:  llvm::StringRef llvm::tdl::getTdlDirectiveName(llvm::tdl::Directive Kind, unsigned Version) {
// IMPL-NEXT:    switch (Kind) {
// IMPL-NEXT:      case TDLD_dira:
// IMPL-NEXT:        return "dira";
// IMPL-NEXT:    }
// IMPL-NEXT:    llvm_unreachable("Invalid Tdl Directive kind");
// IMPL-NEXT:  }
// IMPL-EMPTY:
// IMPL-NEXT: std::pair<llvm::tdl::Clause, llvm::directive::VersionRange> llvm::tdl::getTdlClauseKindAndVersions(llvm::StringRef Str) {
// IMPL-NEXT:   directive::VersionRange All; // Default-initialized to "all versions"
// IMPL-NEXT:   return StringSwitch<std::pair<Clause, directive::VersionRange>>(Str)
// IMPL-NEXT:     .Case("clausea", {TDLC_clausea, All})
// IMPL-NEXT:     .Case("clauseb", {TDLC_clauseb, All})
// IMPL-NEXT:     .Case("clausec", {TDLC_clausec, All})
// IMPL-NEXT:     .Case("ccccccc", {TDLC_clausec, All})
// IMPL-NEXT:     .Default({TDLC_clauseb, All});
// IMPL-NEXT: }
// IMPL-EMPTY:
// IMPL-NEXT:  llvm::StringRef llvm::tdl::getTdlClauseName(llvm::tdl::Clause Kind, unsigned Version) {
// IMPL-NEXT:    switch (Kind) {
// IMPL-NEXT:      case TDLC_clausea:
// IMPL-NEXT:        return "clausea";
// IMPL-NEXT:      case TDLC_clauseb:
// IMPL-NEXT:        return "clauseb";
// IMPL-NEXT:      case TDLC_clausec: {
// IMPL-NEXT:        static constexpr llvm::directive::Spelling TDLC_clausec_spellings[] = {
// IMPL-NEXT:            {"clausec", {0, 2147483647}},
// IMPL-NEXT:            {"ccccccc", {0, 2147483647}},
// IMPL-NEXT:        };
// IMPL-NEXT:        return llvm::directive::FindName(TDLC_clausec_spellings, Version);
// IMPL-NEXT:      }
// IMPL-NEXT:    }
// IMPL-NEXT:    llvm_unreachable("Invalid Tdl Clause kind");
// IMPL-NEXT:  }
// IMPL-EMPTY:
// IMPL-NEXT:  llvm::tdl::AKind llvm::tdl::getAKind(llvm::StringRef Str) {
// IMPL-NEXT:    return StringSwitch<AKind>(Str)
// IMPL-NEXT:      .Case("vala",TDLCV_vala)
// IMPL-NEXT:      .Case("valb",TDLCV_valb)
// IMPL-NEXT:      .Case("valc",TDLCV_valc)
// IMPL-NEXT:      .Default(TDLCV_valc);
// IMPL-NEXT:  }
// IMPL-EMPTY:
// IMPL-NEXT:  llvm::StringRef llvm::tdl::getTdlAKindName(llvm::tdl::AKind x) {
// IMPL-NEXT:    switch (x) {
// IMPL-NEXT:      case TDLCV_vala:
// IMPL-NEXT:        return "vala";
// IMPL-NEXT:      case TDLCV_valb:
// IMPL-NEXT:        return "valb";
// IMPL-NEXT:      case TDLCV_valc:
// IMPL-NEXT:        return "valc";
// IMPL-NEXT:    }
// IMPL-NEXT:    llvm_unreachable("Invalid Tdl AKind kind");
// IMPL-NEXT:  }
// IMPL-EMPTY:
// IMPL-NEXT:  bool llvm::tdl::isAllowedClauseForDirective(llvm::tdl::Directive D, llvm::tdl::Clause C, unsigned Version) {
// IMPL-NEXT:    assert(unsigned(D) <= Directive_enumSize);
// IMPL-NEXT:    assert(unsigned(C) <= Clause_enumSize);
// IMPL-NEXT:    switch (D) {
// IMPL-NEXT:      case TDLD_dira:
// IMPL-NEXT:        switch (C) {
// IMPL-NEXT:          case TDLC_clausea:
// IMPL-NEXT:            return 1 <= Version && 2147483647 >= Version;
// IMPL-NEXT:          case TDLC_clauseb:
// IMPL-NEXT:            return 1 <= Version && 2147483647 >= Version;
// IMPL-NEXT:          default:
// IMPL-NEXT:            return false;
// IMPL-NEXT:        }
// IMPL-NEXT:        break;
// IMPL-NEXT:    }
// IMPL-NEXT:    llvm_unreachable("Invalid Tdl Directive kind");
// IMPL-NEXT:  }
// IMPL-EMPTY:
// IMPL-NEXT:  llvm::tdl::Association llvm::tdl::getDirectiveAssociation(llvm::tdl::Directive Dir) {
// IMPL-NEXT:    switch (Dir) {
// IMPL-NEXT:    case TDLD_dira:
// IMPL-NEXT:      return Association::None;
// IMPL-NEXT:    } // switch (Dir)
// IMPL-NEXT:    llvm_unreachable("Unexpected directive");
// IMPL-NEXT:  }
// IMPL-EMPTY:
// IMPL-NEXT:  llvm::tdl::Category llvm::tdl::getDirectiveCategory(llvm::tdl::Directive Dir) {
// IMPL-NEXT:    switch (Dir) {
// IMPL-NEXT:    case TDLD_dira:
// IMPL-NEXT:      return Category::Executable;
// IMPL-NEXT:    } // switch (Dir)
// IMPL-NEXT:    llvm_unreachable("Unexpected directive");
// IMPL-NEXT:  }
// IMPL-EMPTY:
// IMPL-NEXT:  llvm::tdl::SourceLanguage llvm::tdl::getDirectiveLanguages(llvm::tdl::Directive D) {
// IMPL-NEXT:    switch (D) {
// IMPL-NEXT:    case TDLD_dira:
// IMPL-NEXT:      return SourceLanguage::C | SourceLanguage::Fortran;
// IMPL-NEXT:    } // switch(D)
// IMPL-NEXT:    llvm_unreachable("Unexpected directive");
// IMPL-NEXT:  }
// IMPL-EMPTY:
// IMPL-NEXT:  static_assert(sizeof(llvm::tdl::Directive) == sizeof(int));
// IMPL-NEXT:  {{.*}} static const llvm::tdl::Directive LeafConstructTable[][2] = {
// IMPL-NEXT:    {llvm::tdl::TDLD_dira, static_cast<llvm::tdl::Directive>(0),},
// IMPL-NEXT:  };
// IMPL-EMPTY:
// IMPL-NEXT:  {{.*}} static auto LeafConstructTableEndDirective = LeafConstructTable + 1;
// IMPL-EMPTY:
// IMPL-NEXT:  {{.*}} static const int LeafConstructTableOrdering[] = {
// IMPL-NEXT:    0,
// IMPL-NEXT:  };
// IMPL-EMPTY:
// IMPL-NEXT:  #endif // GEN_DIRECTIVES_IMPL
