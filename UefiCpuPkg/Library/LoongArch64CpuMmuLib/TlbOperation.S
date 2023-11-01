#------------------------------------------------------------------------------
#
# TLB operation functions
#
# Copyright (c) 2023 Loongson Technology Corporation Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#-----------------------------------------------------------------------------

#include <Register/LoongArch64/Csr.h>

ASM_GLOBAL ASM_PFX(HandleTlbRefillStart)
ASM_GLOBAL ASM_PFX(HandleTlbRefillEnd)
ASM_GLOBAL ASM_PFX(InvalidTlb)

#
#  Refill the page table.
#  @param  VOID
#  @retval  VOID
#
ASM_PFX(HandleTlbRefillStart):
  csrwr   $t0, LOONGARCH_CSR_TLBRSAVE
  csrrd   $t0, LOONGARCH_CSR_PGD
  lddir   $t0, $t0, 3   #Put pud BaseAddress into T0
  lddir   $t0, $t0, 2   #Put pmd BaseAddress into T0
  lddir   $t0, $t0, 1   #Put pte BaseAddress into T0
  ldpte   $t0, 0
  ldpte   $t0, 1
  tlbfill   // refill hi,lo0,lo1
  csrrd   $t0, LOONGARCH_CSR_TLBRSAVE
  ertn
ASM_PFX(HandleTlbRefillEnd):

#
# Invalid corresponding TLB entries are based on the address given
# @param a0 The address corresponding to the invalid page table entry
# @retval  none
#
ASM_PFX(InvalidTlb):
    invtlb  INVTLB_ADDR_GTRUE_OR_ASID, $zero, $a0
    jirl    $zero, $ra, 0

    .end
