// CHECK-DAG: {{.*}}/Inputs/external-symbols.cpp:[[@LINE+1]]
static void duplicate_symbol() {}
inline void merged_symbol() {}

void use_external_symbols() {
  (void)duplicate_symbol;
  (void)merged_symbol;
}
