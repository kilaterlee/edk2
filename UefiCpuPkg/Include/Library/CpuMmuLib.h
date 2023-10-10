/** @file

  Copyright (c) 2023 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CPU_MMU_LIB_H_
#define CPU_MMU_LIB_H_

#include <Uefi/UefiBaseType.h>

#ifdef MDE_CPU_LOONGARCH64

/* Page table property definitions  */
#define PAGE_VALID_SHIFT   0
#define PAGE_DIRTY_SHIFT   1
#define PAGE_PLV_SHIFT     2  // 2~3, two bits
#define CACHE_SHIFT        4  // 4~5, two bits
#define PAGE_GLOBAL_SHIFT  6
#define PAGE_HUGE_SHIFT    6  // HUGE is a PMD bit

#define PAGE_HGLOBAL_SHIFT  12 // HGlobal is a PMD bit
#define PAGE_PFN_SHIFT      12
#define PAGE_PFN_END_SHIFT  48
#define PAGE_NO_READ_SHIFT  61
#define PAGE_NO_EXEC_SHIFT  62
#define PAGE_RPLV_SHIFT     63

/* Used by TLB hardware (placed in EntryLo*) */
#define PAGE_VALID    ((UINTN)(1) << PAGE_VALID_SHIFT)
#define PAGE_DIRTY    ((UINTN)(1) << PAGE_DIRTY_SHIFT)
#define PAGE_PLV      ((UINTN)(3) << PAGE_PLV_SHIFT)
#define PAGE_GLOBAL   ((UINTN)(1) << PAGE_GLOBAL_SHIFT)
#define PAGE_HUGE     ((UINTN)(1) << PAGE_HUGE_SHIFT)
#define PAGE_HGLOBAL  ((UINTN)(1) << PAGE_HGLOBAL_SHIFT)
#define PAGE_NO_READ  ((UINTN)(1) << PAGE_NO_READ_SHIFT)
#define PAGE_NO_EXEC  ((UINTN)(1) << PAGE_NO_EXEC_SHIFT)
#define PAGE_RPLV     ((UINTN)(1) << PAGE_RPLV_SHIFT)
#define CACHE_MASK    ((UINTN)(3) << CACHE_SHIFT)
#define PFN_SHIFT     (EFI_PAGE_SHIFT - 12 + PAGE_PFN_SHIFT)

#define PLV_KERNEL  0
#define PLV_USER    3

#define PAGE_USER    (PLV_USER << PAGE_PLV_SHIFT)
#define PAGE_KERNEL  (PLV_KERN << PAGE_PLV_SHIFT)

#define CACHE_SUC  (0 << CACHE_SHIFT) // Strong-ordered UnCached
#define CACHE_CC   (1 << CACHE_SHIFT) // Coherent Cached
#define CACHE_WUC  (2 << CACHE_SHIFT) // Weak-ordered UnCached

#define EFI_MEMORY_CACHETYPE_MASK  (EFI_MEMORY_UC  | \
                                    EFI_MEMORY_WC  | \
                                    EFI_MEMORY_WT  | \
                                    EFI_MEMORY_WB  | \
                                    EFI_MEMORY_UCE   \
                                    )
#endif

typedef struct {
  EFI_PHYSICAL_ADDRESS    PhysicalBase;
  EFI_VIRTUAL_ADDRESS     VirtualBase;
  UINTN                   Length;
  UINTN                   Attributes;
} MEMORY_REGION_DESCRIPTOR;

/**
  Converts EFI Attributes to corresponding architecture Attributes.

  @param[in]  EfiAttributes     Efi Attributes.

  @retval  Corresponding architecture attributes.
**/
UINTN
EfiAttributeConverse (
  IN UINTN  EfiAttributes
  );

/**
  Finds the length and memory properties of the memory region corresponding to the specified base address.

  @param[in]  BaseAddress    To find the base address of the memory region.
  @param[in]  EndAddress     To find the end address of the memory region.
  @param[out]  RegionLength    The length of the memory region found.
  @param[out]  RegionAttributes    Properties of the memory region found.

  @retval  EFI_SUCCESS    The corresponding memory area was successfully found
           EFI_NOT_FOUND    No memory area found
**/
EFI_STATUS
GetMemoryRegionAttribute (
  IN     UINTN  BaseAddress,
  IN     UINTN  EndAddress,
  OUT    UINTN  *RegionLength,
  OUT    UINTN  *RegionAttributes
  );

/**
  Sets the Attributes  of the specified memory region

  @param[in]  BaseAddress  The base address of the memory region to set the Attributes.
  @param[in]  Length       The length of the memory region to set the Attributes.
  @param[in]  Attributes   The Attributes to be set.

  @retval  EFI_SUCCESS    The Attributes was set successfully
**/
EFI_STATUS
SetMemoryAttributes (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN                 Length,
  IN UINTN                 Attributes
  );

/**
  Sets the non-executable Attributes for the specified memory region

  @param[in]  BaseAddress  The base address of the memory region to set the Attributes.
  @param[in]  Length       The length of the memory region to set the Attributes.

  @retval  EFI_SUCCESS    The Attributes was set successfully
**/
EFI_STATUS
SetMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINTN                 Length
  );

/**
  Clears the non-executable Attributes for the specified memory region

  @param[in]  BaseAddress  The base address of the memory region to clear the Attributes.
  @param[in]  Length       The length of the memory region to clear the Attributes.

  @retval  EFI_SUCCESS    The Attributes was clear successfully
**/
EFI_STATUS
EFIAPI
ClearMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

/**
  Sets the read-only Attributes for the specified memory region

  @param[in]  BaseAddress  The base address of the memory region to set the Attributes.
  @param[in]  Length       The length of the memory region to set the Attributes.

  @retval  EFI_SUCCESS    The Attributes was set successfully
**/
EFI_STATUS
EFIAPI
SetMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

/**
  Clears the read-only Attributes for the specified memory region

  @param[in]  BaseAddress  The base address of the memory region to clear the Attributes.
  @param[in]  Length       The length of the memory region to clear the Attributes.

  @retval  EFI_SUCCESS    The Attributes was clear successfully
**/
EFI_STATUS
EFIAPI
ClearMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

/**
  Create a page table and initialize the memory management unit(MMU).

  @param[in]     MemoryTable           A pointer to a memory ragion table.
  @param[out]    TranslationTableBase  A pointer to a translation table base address.
  @param[in out] TranslationTableSize  A pointer to a translation table base size.

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
  );

#endif // CPU_MMU_LIB_H_
