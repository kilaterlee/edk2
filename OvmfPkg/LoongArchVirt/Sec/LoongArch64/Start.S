#------------------------------------------------------------------------------
#
# Start for Loongson LoongArch processor
#
# Copyright (c) 2023 Loongson Technology Corporation Limited. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#  @par Glossary:
#    - CSR - CPU Status Register
#    - EBASE - Exception Base Address
#------------------------------------------------------------------------------
#ifndef __ASSEMBLY__
#define __ASSEMBLY__
#endif

#include <Library/BaseMemoryLib.h>
#include <Register/LoongArch64/Csr.h>
#include <Protocol/DebugSupport.h>

#define BOOTCORE_ID 0

ASM_GLOBAL ASM_PFX(_ModuleEntryPoint)
ASM_PFX(_ModuleEntryPoint):
  /* Disable interrupt */
  li.d     $t0, (1 << 2)
  csrxchg  $zero, $t0, LOONGARCH_CSR_CRMD

  /* Read physical cpu number id */
  bl       GetApicId
  li.d     $t0, BOOTCORE_ID  //0
  bne      $a0, $t0, SlaveMain

  /* Configure BSP reset ebase */
  li.d     $a0, FixedPcdGet64(PcdCpuExceptionVectorBaseAddress)
  bl       SetExceptionBaseAddress
  move     $t1, $a0

  /* Set BSP stack */
  li.d     $t0, FixedPcdGet64(PcdOvmfSecPeiTempRamBase) + FixedPcdGet32(PcdOvmfSecPeiTempRamSize)  # stack base
  move     $sp, $t0
  addi.d   $sp, $sp, -0x8

  /* Construct SEC and PEI step exception environment */
  la.pcrel $a1, ExceptionEntryStart
  la.pcrel $t0, ExceptionEntryEnd
  sub.d    $a2, $t0, $a1
  li.w     $t0, (MAX_LOONGARCH_EXCEPTION +  MAX_LOONGARCH_INTERRUPT) * 512
  bgeu     $a2, $t0, DeadLoop
  move     $a0, $t1
  bl       CopyMem

CallEntry:
  /* Call C function make sure parameter true */
  li.d     $a0, FixedPcdGet64(PcdOvmfFdBaseAddress) # FW base
  addi.d   $a1, $sp, 0x8
  bl       SecCoreStartupWithStack
# End of _ModuleEntryPoint

ASM_PFX(ClearMailBox):
  /* Clear mailbox */
  li.d      $t1, LOONGARCH_IOCSR_MBUF3
  iocsrwr.d $zero, $t1
  li.d      $t1, LOONGARCH_IOCSR_MBUF2
  iocsrwr.d $zero, $t1
  li.d      $t1, LOONGARCH_IOCSR_MBUF1
  iocsrwr.d $zero, $t1
  li.d      $t1, LOONGARCH_IOCSR_MBUF0
  iocsrwr.d $zero, $t1
  jirl      $zero, $ra, 0
# End of ClearMailBox

ASM_PFX(EnableIPI):
  /* Enable IPI interrupt */
  li.d      $t1, (1 << 12)
  csrxchg   $t1, $t1, LOONGARCH_CSR_ECFG

  addi.d    $t2, $zero, -1
  li.d      $t1, LOONGARCH_IOCSR_IPI_EN
  iocsrwr.w $t2, $t1
  jirl      $zero, $ra, 0
# End of EeableIPI

#/**
#   Get APIC ID for every CPU.
#
#   @param   NULL
#   @return  APICID
#
#   UINTN
#   EFI_API
#   GetApicId (
#     VOID
#     )
#**/
ASM_PFX(GetApicId):
  csrrd $a0, LOONGARCH_CSR_CPUNUM
  andi  $a0, $a0, 0x3ff
  jirl  $zero, $ra, 0
# End of GetApicId

ASM_PFX(ApInitStack):
  li.d   $t1, SIZE_1KB 
  csrrd  $t0, LOONGARCH_CSR_TMID
  mul.d  $t1, $t0, $t1
  li.d   $t2, FixedPcdGet32(PcdCpuMaxLogicalProcessorNumber)
  bgeu   $t0, $t2, DeadLoop
  li.d   $t0, FixedPcdGet64(PcdOvmfSecPeiTempRamBase) + FixedPcdGet32(PcdOvmfSecPeiTempRamSize) - SIZE_64KB
  sub.d  $sp, $t0, $t1
  addi.d $sp, $sp, -0x8
  jirl   $zero, $ra, 0
# End of ApInitStack

ASM_PFX(SlaveMain):
  /* Set AP exception handle in flash */
  la.pcrel  $a0, ApException
  bl        SetExceptionBaseAddress

  /* Clean up local mail box and open INT */
  bl        ClearMailBox
  bl        EnableIPI
  bl        EnableInterrupts

WaitForWake:
  /* Wait for wakeup */
  bl        CpuSleep
  li.d      $t1, LOONGARCH_IOCSR_MBUF0
  iocsrrd.w $t2, $t1
  beqz      $t2, WaitForWake

  /* Disable global interrupt */
  bl        DisableInterrupts

  /* Disable IPI interrupt */
  li.d      $t0, (1 << 12)
  csrxchg   $zero, $t0, LOONGARCH_CSR_ECFG

  /* Read mail buf and jump to specified entry */
  li.d      $t1, LOONGARCH_IOCSR_MBUF0
  iocsrrd.d $t0, $t1
  li.d      $t1, LOONGARCH_IOCSR_MBUF3
  iocsrrd.d $a1, $t1
  bl        ClearMailBox
  beqz      $a1, NoParameterCall

  //
  // If the parameters are not NULL, then calling happened in FW ENV.
  // Set the EBASE to be the same as BSP.
  //
  li.d      $a0, FixedPcdGet64(PcdCpuExceptionVectorBaseAddress)
  bl        SetExceptionBaseAddress

  bl        ApInitStack
  bl        GetApicId
NoParameterCall:
  move      $ra, $t0
  jirl      $zero, $ra, 0x0
# End of SlaveMain

.align 12
ASM_PFX(ApException):
  csrrd     $t0, LOONGARCH_CSR_ESTAT
  srli.d    $t0, $t0, 12
  beqz      $t0, DeadLoop

  li.d      $t0, LOONGARCH_IOCSR_IPI_STATUS
  iocsrrd.w $t1, $t0
  li.d      $t0, LOONGARCH_IOCSR_IPI_CLEAR
  iocsrwr.w $t1, $t0
  ertn
# End of ApException

ASM_PFX(DeadLoop):
  b   DeadLoop
# End of DeadLoop
.end
