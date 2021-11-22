#------------------------------------------------------------------------------
#
# MemoryFence() for LoongArch64
#
# Copyright (c) 2021, Loongson Technology Corporation Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(MemoryFence)

#
# Memory fence for LoongArch64
#
ASM_PFX(MemoryFence):
  dbar 0
  jirl $zero, $ra, 0
  .end
