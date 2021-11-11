#------------------------------------------------------------------------------
#
# CpuSleep() for LoongArch64
#
# Copyright (c) 2021, Loongson Technology Corporation Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------
ASM_GLOBAL ASM_PFX(CpuSleep)

ASM_PFX(CpuSleep):
  idle 0
  jirl $zero, $ra, 0
  .end
