! RUN: rm -fr %t && mkdir -p %t && cd %t
! RUN: bbc -emit-fir %S/module_definition.f90
! RUN: bbc -emit-fir %s -o - | FileCheck %s

! Test use of module data not defined in this file.
! The modules are defined in module_definition.f90
! The first runs ensures the module file is generated.

! CHECK: fir.global common @__BLNK__(dense<0> : vector<4xi8>) {alignment = 4 : i64} : !fir.array<4xi8>
! CHECK-NEXT: fir.global common @named1_(dense<0> : vector<4xi8>) {alignment = 4 : i64} : !fir.array<4xi8>
! CHECK-NEXT: fir.global common @named2_(dense<0> : vector<4xi8>) {alignment = 4 : i64} : !fir.array<4xi8>

! CHECK-LABEL: func @_QPm1use()
real function m1use()
  use m1
  ! CHECK-DAG: fir.address_of(@_QMm1Ex) : !fir.ref<f32>
  ! CHECK-DAG: fir.address_of(@_QMm1Ey) : !fir.ref<!fir.array<100xi32>>
  m1use = x + y(1)
end function

! TODO: test equivalences once front-end fix in module file is pushed.
! COM: CHECK-LABEL: func @_QPmodeq1use()
!real function modEq1use()
!  use modEq1
!  COM: CHECK-DAG: fir.address_of(@_QMmodeq1Ex1) : !fir.ref<tuple<!fir.array<36xi8>, !fir.array<40xi8>>>
!  COM: CHECK-DAG: fir.address_of(@_QMmodeq1Ey1) : !fir.ref<tuple<!fir.array<16xi8>, !fir.array<24xi8>>>
!  modEq1use = x2(1) + y1
!end function
! COM: CHECK-DAG: fir.global @_QMmodeq1Ex1 : tuple<!fir.array<36xi8>, !fir.array<40xi8>>
! COM: CHECK-DAG: fir.global @_QMmodeq1Ey1 : tuple<!fir.array<16xi8>, !fir.array<24xi8>>

! CHECK-LABEL: func @_QPmodcommon1use()
real function modCommon1Use()
  use modCommonInit1
  use modCommonNoInit1
  ! CHECK-DAG: fir.address_of(@named2_) : !fir.ref<!fir.array<4xi8>>
  ! CHECK-DAG: fir.address_of(@__BLNK__) : !fir.ref<!fir.array<4xi8>>
  ! CHECK-DAG: fir.address_of(@named1_) : !fir.ref<!fir.array<4xi8>>
  modCommon1Use = x_blank + x_named1 + i_named2 
end function


! CHECK-DAG: fir.global @_QMm1Ex : f32
! CHECK-DAG: fir.global @_QMm1Ey : !fir.array<100xi32>
