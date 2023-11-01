/** @file
  CPU Memory Map Unit PEI phase driver.

  Copyright (c) 2023 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Tlb      - Translation Lookaside Buffer
**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/CpuMmuLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/CpuMmuLib.h>
#include <Register/LoongArch64/Csr.h>

#include "Page.h"
#include "Tlb.h"
#include "CommonMmuLib.h"

/**
  Create a page table and initialize the memory management unit(MMU).

  @param[in]      MemoryTable           A pointer to a memory ragion table.
  @param[out]     TranslationTableBase  A pointer to a translation table base address.
  @param[in, out] TranslationTableSize  A pointer to a translation table base size.

  @retval  EFI_SUCCESS                Configure MMU successfully.
           EFI_INVALID_PARAMETER      MemoryTable is NULL.
           EFI_UNSUPPORTED            Out of memory space or size not aligned.
**/
EFI_STATUS
EFIAPI
ConfigureMemoryManagementUint (
  IN     MEMORY_REGION_DESCRIPTOR  *MemoryTable,
  OUT    VOID                      **TranslationTableBase OPTIONAL,
  IN OUT UINTN                     *TranslationTableSize  OPTIONAL
  )
{
  PGD            *SwapperPageDir;
  UINTN          PgdShift;
  UINTN          PgdWide;
  UINTN          PudShift;
  UINTN          PudWide;
  UINTN          PmdShift;
  UINTN          PmdWide;
  UINTN          PteShift;
  UINTN          PteWide;
  UINTN          Length;
  UINTN          TlbReEntry;
  UINTN          TlbReEntryOffset;
  RETURN_STATUS  PcdStatus;

  SwapperPageDir = NULL;
  PgdShift       = PGD_SHIFT;
  PgdWide        = PGD_WIDE;
  PudShift       = PUD_SHIFT;
  PudWide        = PUD_WIDE;
  PmdShift       = PMD_SHIFT;
  PmdWide        = PMD_WIDE;
  PteShift       = PTE_SHIFT;
  PteWide        = PTE_WIDE;

  if (MemoryTable == NULL) {
    ASSERT (MemoryTable != NULL);
    return EFI_INVALID_PARAMETER;
  }

  SwapperPageDir = AllocatePages (EFI_SIZE_TO_PAGES (PGD_TABLE_SIZE));
  ZeroMem (SwapperPageDir, PGD_TABLE_SIZE);

  if (SwapperPageDir == NULL) {
    goto FreeTranslationTable;
  }

  CsrWrite (LOONGARCH_CSR_PGDL, (UINTN)SwapperPageDir);

  while (((*TranslationTableSize)--) != 0) {
    DEBUG ((
      DEBUG_INFO,
      "%a %d VirtualBase %p VirtualEnd %p Attributes %p .\n",
      __func__,
      __LINE__,
      MemoryTable->VirtualBase,
      (MemoryTable->Length + MemoryTable->VirtualBase),
      MemoryTable->Attributes
      ));

    PcdStatus = FillTranslationTable (MemoryTable);
    if (EFI_ERROR (PcdStatus)) {
      goto FreeTranslationTable;
    }

    MemoryTable++;
  }

  //
  // TLB Re-entry address at the end of exception vector, a vector is up to 512 bytes,
  // so the starting address is: total exception vector size + total interrupt vector size + base.
  // The total size of TLB handler and exception vector size and interrupt vector size should not
  // be lager than 64KB.
  //
  Length           = (UINTN)HandleTlbRefillEnd - (UINTN)HandleTlbRefillStart;
  TlbReEntryOffset = (MAX_LOONGARCH_EXCEPTION + MAX_LOONGARCH_INTERRUPT) * 512;
  TlbReEntry       = PcdGet64 (PcdCpuExceptionVectorBaseAddress) + TlbReEntryOffset;
  if ((TlbReEntryOffset + Length) > SIZE_64KB) {
    goto FreeTranslationTable;
  }

  //
  // Make sure TLB refill exception base address alignment is greater than or equal to 4KB and valid
  //
  if (TlbReEntry & (SIZE_4KB - 1)) {
    goto FreeTranslationTable;
  }

  CopyMem ((VOID *)TlbReEntry, HandleTlbRefillStart, Length);
  InvalidateInstructionCacheRange ((VOID *)(UINTN)HandleTlbRefillStart, Length);

  DEBUG ((
    DEBUG_INFO,
    "%a  %d PteShift %d PteWide %d PmdShift %d PmdWide %d PudShift %d PudWide %d PgdShift %d PgdWide %d.\n",
    __func__,
    __LINE__,
    PteShift,
    PteWide,
    PmdShift,
    PmdWide,
    PudShift,
    PudWide,
    PgdShift,
    PgdWide
    ));

  //
  // Set the address of TLB refill exception handler
  //
  SetTlbRebaseAddress ((UINTN)TlbReEntry);

  //
  // Set page size
  //
  CsrXChg (LOONGARCH_CSR_TLBIDX, (DEFAULT_PAGE_SIZE << CSR_TLBIDX_SIZE), CSR_TLBIDX_SIZE_MASK);
  CsrWrite (LOONGARCH_CSR_STLBPGSIZE, DEFAULT_PAGE_SIZE);
  CsrXChg (LOONGARCH_CSR_TLBREHI, (DEFAULT_PAGE_SIZE << CSR_TLBREHI_PS_SHIFT), CSR_TLBREHI_PS);

  CsrWrite (LOONGARCH_CSR_PWCTL0, (PteShift | PteWide << 5 | PmdShift << 10 | PmdWide << 15 | PudShift << 20 | PudWide << 25));
  CsrWrite (LOONGARCH_CSR_PWCTL1, (PgdShift | PgdWide << 6));

  DEBUG ((DEBUG_INFO, "%a %d Enable Mmu Start PageBassAddress %p.\n", __func__, __LINE__, SwapperPageDir));

  return EFI_SUCCESS;

FreeTranslationTable:
  if (SwapperPageDir != NULL) {
    FreePages (SwapperPageDir, EFI_SIZE_TO_PAGES (PGD_TABLE_SIZE));
  }

  return EFI_UNSUPPORTED;
}
