/** @file

  CPU Memory Map Unit Handler Library common functions.

  Copyright (c) 2023 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Pgd or Pgd or PGD    - Page Global Directory
    - Pud or Pud or PUD    - Page Upper Directory
    - Pmd or Pmd or PMD    - Page Middle Directory
    - Pte or pte or PTE    - Page Table Entry
    - Val or VAL or val    - Value
    - Dir    - Directory
**/
#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/CpuMmuLib.h>
#include <Register/LoongArch64/Csr.h>
#include "Tlb.h"
#include "Page.h"

BOOLEAN  mMmuInited = FALSE;
#define  SWAP_PAGE_DIR  CsrRead(LOONGARCH_CSR_PGDL)

/**
  Check to see if mmu successfully initializes.

  @param  VOID.

  @retval  TRUE  Initialization has been completed.
           FALSE Initialization did not complete.
**/
BOOLEAN
MmuIsInit (
  VOID
  )
{
  if (mMmuInited || (SWAP_PAGE_DIR != 0)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Iterates through the page directory to initialize it.

  @param  Dst  A pointer to the directory of the page to initialize.
  @param  Num  The number of page directories to initialize.
  @param  Src  A pointer to the data used to initialize the page directory.

  @return VOID.
**/
VOID
PageDirInit (
  IN VOID   *Dst,
  IN UINTN  Num,
  IN VOID   *Src
  )
{
  UINTN  *Ptr;
  UINTN  *End;
  UINTN  Entry;

  Entry = (UINTN)Src;
  Ptr   = (UINTN *)Dst;
  End   = Ptr + Num;

  for ( ; Ptr < End; Ptr++) {
    *Ptr = Entry;
  }

  return;
}

/**
  Gets the virtual address corresponding to the page global directory table entry.

  @param  Address  the virtual address for the table entry.

  @retval PGD A pointer to get the table item.
**/
PGD *
PgdOffset (
  IN UINTN  Address
  )
{
  return (PGD *)(SWAP_PAGE_DIR) + PGD_INDEX (Address);
}

/**
  Gets the virtual address corresponding to the page upper directory table entry.

  @param  Pgd  A pointer to a page global directory table entry.
  @param  Address  the virtual address for the table entry.

  @retval PUD A pointer to get the table item.
**/
PUD *
PudOffset (
  IN PGD    *Pgd,
  IN UINTN  Address
  )
{
  UINTN  PgdVal;

  PgdVal = (UINTN)PGD_VAL (*Pgd);

  return (PUD *)PgdVal + PUD_INDEX (Address);
}

/**
  Gets the virtual address corresponding to the page middle directory table entry.

  @param  Pud  A pointer to a page upper directory table entry.
  @param  Address  the virtual address for the table entry.

  @retval PMD A pointer to get the table item.
**/
PMD *
PmdOffset (
  IN PUD    *Pud,
  IN UINTN  Address
  )
{
  UINTN  PudVal;

  PudVal = PUD_VAL (*Pud);

  return (PMD *)PudVal + PMD_INDEX (Address);
}

/**
  Gets the virtual address corresponding to the page table entry.

  @param  Pmd  A pointer to a page middle directory table entry.
  @param  Address  the virtual address for the table entry.

  @retval PTE A pointer to get the table item.
**/
PTE *
PteOffset (
  IN PMD    *Pmd,
  IN UINTN  Address
  )
{
  UINTN  PmdVal;

  PmdVal = (UINTN)PMD_VAL (*Pmd);

  return (PTE *)PmdVal + PTE_INDEX (Address);
}

/**
  Sets the value of the page table entry.

  @param  Pte  A pointer to a page table entry.
  @param  PteVal  The value of the page table entry to set.

**/
VOID
SetPte (
  IN PTE  *Pte,
  IN PTE  PteVal
  )
{
  *Pte = PteVal;
}

/**
  Sets the value of the page global directory.

  @param  Pgd  A pointer to a page global directory.
  @param  Pud  The value of the page global directory to set.

**/
VOID
SetPgd (
  IN PGD  *Pgd,
  IN PUD  *Pud
  )
{
  *Pgd = (PGD) {
    ((UINTN)Pud)
  };
}

/**
  Sets the value of the page upper directory.

  @param  Pud  A pointer to a page upper directory.
  @param  Pmd  The value of the page upper directory to set.

**/
VOID
SetPud (
  IN PUD  *Pud,
  IN PMD  *Pmd
  )
{
  *Pud = (PUD) {
    ((UINTN)Pmd)
  };
}

/**
  Sets the value of the page middle directory.

  @param  Pmd  A pointer to a page middle directory.
  @param  Pte  The value of the page middle directory to set.

**/
VOID
SetPmd (
  IN PMD  *Pmd,
  IN PTE  *Pte
  )
{
  *Pmd = (PMD) {
    ((UINTN)Pte)
  };
}

/**
  Free up memory space occupied by page tables.

  @param  Pte  A pointer to the page table.

**/
VOID
PteFree (
  IN PTE  *Pte
  )
{
  FreePages ((VOID *)Pte, 1);
}

/**
  Free up memory space occupied by page middle directory.

  @param  Pmd  A pointer to the page middle directory.

**/
VOID
PmdFree (
  IN PMD  *Pmd
  )
{
  FreePages ((VOID *)Pmd, 1);
}

/**
  Free up memory space occupied by page upper directory.

  @param  Pud  A pointer to the page upper directory.

**/
VOID
PudFree (
  IN PUD  *Pud
  )
{
  FreePages ((VOID *)Pud, 1);
}

/**
  Requests the memory space required for the page upper directory,
  initializes it, and places it in the specified page global directory

  @param  Pgd  A pointer to the page global directory.

  @retval  EFI_SUCCESS  Memory request successful.
  @retval  EFI_OUT_OF_RESOURCES  Resource exhaustion cannot be requested to memory.
**/
EFI_STATUS
PudAlloc (
  IN PGD  *Pgd
  )
{
  PUD  *Pud;

  Pud = (PUD *)AllocatePages (1);
  if (Pud == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PageDirInit ((VOID *)Pud, ENTRYS_PER_PUD, (VOID *)INVALID_PAGE);

  SetPgd (Pgd, Pud);

  return EFI_SUCCESS;
}

/**
  Requests the memory space required for the page middle directory,
  initializes it, and places it in the specified page upper directory

  @param  Pud  A pointer to the page upper directory.

  @retval  EFI_SUCCESS  Memory request successful.
  @retval  EFI_OUT_OF_RESOURCES  Resource exhaustion cannot be requested to memory.
**/
EFI_STATUS
PmdAlloc (
  IN PUD  *Pud
  )
{
  PMD  *Pmd;

  Pmd = (PMD *)AllocatePages (1);
  if (!Pmd) {
    return EFI_OUT_OF_RESOURCES;
  }

  PageDirInit ((VOID *)Pmd, ENTRYS_PER_PMD, (VOID *)INVALID_PAGE);

  SetPud (Pud, Pmd);

  return EFI_SUCCESS;
}

/**
  Requests the memory space required for the page table,
  initializes it, and places it in the specified page middle directory

  @param  Pmd  A pointer to the page middle directory.

  @retval  EFI_SUCCESS  Memory request successful.
  @retval  EFI_OUT_OF_RESOURCES  Resource exhaustion cannot be requested to memory.
**/
EFI_STATUS
PteAlloc (
  IN PMD  *Pmd
  )
{
  PTE  *Pte;

  Pte = (PTE *)AllocatePages (1);
  if (!Pte) {
    return EFI_OUT_OF_RESOURCES;
  }

  Pte = ZeroMem (Pte, EFI_PAGE_SIZE);

  SetPmd (Pmd, Pte);

  return EFI_SUCCESS;
}

/**
  Requests the memory space required for the page upper directory,
  initializes it, and places it in the specified page global directory,
  and get the page upper directory entry corresponding to the virtual address.

  @param  Pgd      A pointer to the page global directory.
  @param  Address  The corresponding virtual address of the page table entry.

  @retval          A pointer to the page upper directory entry. Return NULL, if
                   allocate the memory buffer is fail.
**/
PUD *
PudAllocGet (
  IN PGD    *Pgd,
  IN UINTN  Address
  )
{
  EFI_STATUS  Status;

  if (PGD_IS_EMPTY (*Pgd)) {
    Status = PudAlloc (Pgd);
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      return NULL;
    }
  }

  return PudOffset (Pgd, Address);
}

/**
  Requests the memory space required for the page middle directory,
  initializes it, and places it in the specified page upper directory,
  and get the page middle directory entry corresponding to the virtual address.

  @param  Pud      A pointer to the page upper directory.
  @param  Address  The corresponding virtual address of the page table entry.

  @retval          A pointer to the page middle directory entry. Return NULL, if
                   allocate the memory buffer is fail.
**/
PMD *
PmdAllocGet (
  IN PUD    *Pud,
  IN UINTN  Address
  )
{
  EFI_STATUS  Status;

  if (PUD_IS_EMPTY (*Pud)) {
    Status = PmdAlloc (Pud);
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      return NULL;
    }
  }

  return PmdOffset (Pud, Address);
}

/**
  Requests the memory space required for the page table,
  initializes it, and places it in the specified page middle directory,
  and get the page table entry corresponding to the virtual address.

  @param  Pmd      A pointer to the page upper directory.
  @param  Address  The corresponding virtual address of the page table entry.

  @retval          A pointer to the page table entry. Return NULL, if allocate
                   the memory buffer is fail.
**/
PTE *
PteAllocGet (
  IN PMD    *Pmd,
  IN UINTN  Address
  )
{
  EFI_STATUS  Status;

  if (PMD_IS_EMPTY (*Pmd)) {
    Status = PteAlloc (Pmd);
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      return NULL;
    }
  }

  return PteOffset (Pmd, Address);
}

/**
  Gets the physical address of the page table entry corresponding to the specified virtual address.

  @param  Address  The corresponding virtual address of the page table entry.

  @retval  A pointer to the page table entry.
  @retval  NULL
**/
PTE *
GetPteAddress (
  IN UINTN  Address
  )
{
  PGD  *Pgd;
  PUD  *Pud;
  PMD  *Pmd;

  Pgd = PgdOffset (Address);

  if (PGD_IS_EMPTY (*Pgd)) {
    return NULL;
  }

  Pud = PudOffset (Pgd, Address);

  if (PUD_IS_EMPTY (*Pud)) {
    return NULL;
  }

  Pmd = PmdOffset (Pud, Address);
  if (PMD_IS_EMPTY (*Pmd)) {
    return NULL;
  }

  if (IS_HUGE_PAGE (Pmd->PmdVal)) {
    return ((PTE *)Pmd);
  }

  return PteOffset (Pmd, Address);
}

/**
  Gets the Attributes of Huge Page.

  @param  Pmd  A pointer to the page middle directory.

  @retval     Value of Attributes.
**/
UINTN
GetHugePageAttributes (
  IN  PMD  *Pmd
  )
{
  UINTN  Attributes;
  UINTN  GlobalFlag;
  UINTN  HugeVal;

  HugeVal     = PMD_VAL (*Pmd);
  Attributes  = HugeVal & (~HUGEP_PAGE_MASK);
  GlobalFlag  = ((Attributes & (1 << PAGE_HGLOBAL_SHIFT)) >> PAGE_HGLOBAL_SHIFT) << PAGE_GLOBAL_SHIFT;
  Attributes &= ~(1 << PAGE_HGLOBAL_SHIFT);
  Attributes |= GlobalFlag;
  return Attributes;
}

/**
  Establishes a page table entry based on the specified memory region.

  @param  Pmd  A pointer to the page middle directory.
  @param  Address  The memory space start address.
  @param  End  The end address of the memory space.
  @param  Attributes  Memory space Attributes.

  @retval     EFI_SUCCESS   The page table entry was created successfully.
  @retval     EFI_OUT_OF_RESOURCES  Page table entry establishment failed due to resource exhaustion.
**/
EFI_STATUS
MemoryMapPteRange (
  IN PMD    *Pmd,
  IN UINTN  Address,
  IN UINTN  End,
  IN UINTN  Attributes
  )
{
  PTE      *Pte;
  PTE      PteVal;
  BOOLEAN  UpDate;

  Pte = PteAllocGet (Pmd, Address);
  if (!Pte) {
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((
    DEBUG_INFO,
    "%a %d Address %p End %p  Attributes %llx\n",
    __func__,
    __LINE__,
    Address,
    End,
    Attributes
    ));

  do {
    UpDate = FALSE;
    PteVal = MAKE_PTE (Address, Attributes);

    if ((!PTE_IS_EMPTY (*Pte)) &&
        (PTE_VAL (*Pte) != PTE_VAL (PteVal)))
    {
      UpDate = TRUE;
    }

    SetPte (Pte, PteVal);
    if (UpDate) {
      InvalidTlb (Address);
    }
  } while (Pte++, Address += EFI_PAGE_SIZE, Address != End);

  return EFI_SUCCESS;
}

/**
  Convert Huge Page to Page.

  @param  Pmd  A pointer to the page middle directory.
  @param  Address  The memory space start address.
  @param  End  The end address of the memory space.
  @param  Attributes  Memory space Attributes.

  @retval  EFI_SUCCESS   The page table entry was created successfully.
  @retval  EFI_OUT_OF_RESOURCES  Page table entry establishment failed due to resource exhaustion.
**/
EFI_STATUS
ConvertHugePageToPage (
  IN  PMD   *Pmd,
  IN UINTN  Address,
  IN UINTN  End,
  IN UINTN  Attributes
  )
{
  UINTN       OldAttributes;
  UINTN       HugePageEnd;
  UINTN       HugePageStart;
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  if ((PMD_IS_EMPTY (*Pmd)) ||
      (!IS_HUGE_PAGE (Pmd->PmdVal)))
  {
    Status |= MemoryMapPteRange (Pmd, Address, End, Attributes);
  } else {
    OldAttributes = GetHugePageAttributes (Pmd);
    if (Attributes == OldAttributes) {
      return Status;
    }

    SetPmd (Pmd, (PTE *)(INVALID_PAGE));
    HugePageStart = Address & PMD_MASK;
    HugePageEnd   = HugePageStart + HUGE_PAGE_SIZE;
    ASSERT (HugePageEnd >= End);

    if (Address > HugePageStart) {
      Status |= MemoryMapPteRange (Pmd, HugePageStart, Address, OldAttributes);
    }

    Status |= MemoryMapPteRange (Pmd, Address, End, Attributes);

    if (End < HugePageEnd) {
      Status |= MemoryMapPteRange (Pmd, End, HugePageEnd, OldAttributes);
    }
  }

  return Status;
}

/**
  Establishes a page middle directory based on the specified memory region.

  @param  Pud  A pointer to the page upper directory.
  @param  Address  The memory space start address.
  @param  End  The end address of the memory space.
  @param  Attributes  Memory space Attributes.

  @retval     EFI_SUCCESS   The page middle directory was created successfully.
  @retval     EFI_OUT_OF_RESOURCES  Page middle directory establishment failed due to resource exhaustion.
**/
EFI_STATUS
MemoryMapPmdRange (
  IN PUD    *Pud,
  IN UINTN  Address,
  IN UINTN  End,
  IN UINTN  Attributes
  )
{
  PMD      *Pmd;
  UINTN    Next;
  PTE      PteVal;
  BOOLEAN  UpDate;

  Pmd = PmdAllocGet (Pud, Address);
  if (!Pmd) {
    return EFI_OUT_OF_RESOURCES;
  }

  do {
    Next = PMD_ADDRESS_END (Address, End);
    if (((Address & (~PMD_MASK)) == 0) &&
        ((Next &  (~PMD_MASK)) == 0) &&
        (PMD_IS_EMPTY (*Pmd) || IS_HUGE_PAGE (Pmd->PmdVal)))
    {
      UpDate = FALSE;
      PteVal = MAKE_HUGE_PTE (Address, Attributes);

      if ((!PMD_IS_EMPTY (*Pmd)) &&
          (PMD_VAL (*Pmd) != PTE_VAL (PteVal)))
      {
        UpDate = TRUE;
      }

      SetPmd (Pmd, (PTE *)PteVal.PteVal);
      if (UpDate) {
        InvalidTlb (Address);
      }
    } else {
      ConvertHugePageToPage (Pmd, Address, Next, Attributes);
    }
  } while (Pmd++, Address = Next, Address != End);

  return EFI_SUCCESS;
}

/**
  Establishes a page upper directory based on the specified memory region.

  @param  Pgd  A pointer to the page global directory.
  @param  Address  The memory space start address.
  @param  End  The end address of the memory space.
  @param  Attributes  Memory space Attributes.

  @retval     EFI_SUCCESS   The page upper directory was created successfully.
  @retval     EFI_OUT_OF_RESOURCES  Page upper directory establishment failed due to resource exhaustion.
**/
EFI_STATUS
MemoryMapPudRange (
  IN PGD    *Pgd,
  IN UINTN  Address,
  IN UINTN  End,
  IN UINTN  Attributes
  )
{
  PUD    *Pud;
  UINTN  Next;

  Pud = PudAllocGet (Pgd, Address);
  if (!Pud) {
    return EFI_OUT_OF_RESOURCES;
  }

  do {
    Next = PUD_ADDRESS_END (Address, End);
    if (EFI_ERROR (MemoryMapPmdRange (Pud, Address, Next, Attributes))) {
      return EFI_OUT_OF_RESOURCES;
    }
  } while (Pud++, Address = Next, Address != End);

  return EFI_SUCCESS;
}

/**
  Establishes a page global directory based on the specified memory region.

  @param  Start  The memory space start address.
  @param  End  The end address of the memory space.
  @param  Attributes  Memory space Attributes.

  @retval     EFI_SUCCESS   The page global directory was created successfully.
  @retval     EFI_OUT_OF_RESOURCES  Page global directory establishment failed due to resource exhaustion.
**/
EFI_STATUS
MemoryMapPageRange (
  IN UINTN  Start,
  IN UINTN  End,
  IN UINTN  Attributes
  )
{
  PGD         *Pgd;
  UINTN       Next;
  UINTN       Address;
  EFI_STATUS  Err;

  Address = Start;

  /* Get PGD(PTE PMD PUD PGD) in PageTables */
  Pgd = PgdOffset (Address);
  do {
    Next = PGD_ADDRESS_END (Address, End);
    /* Get Next Align Page to Map */
    Err = MemoryMapPudRange (Pgd, Address, Next, Attributes);
    if (Err) {
      return Err;
    }
  } while (Pgd++, Address = Next, Address != End);

  return EFI_SUCCESS;
}

/**
  Page tables are established from memory-mapped tables.

  @param  MemoryRegion   A pointer to a memory-mapped table entry.

  @retval     EFI_SUCCESS   The page table was created successfully.
  @retval     EFI_OUT_OF_RESOURCES  Page table  establishment failed due to resource exhaustion.
**/
EFI_STATUS
FillTranslationTable (
  IN  MEMORY_REGION_DESCRIPTOR  *MemoryRegion
  )
{
  return MemoryMapPageRange (
           MemoryRegion->VirtualBase,
           (MemoryRegion->Length + MemoryRegion->VirtualBase),
           MemoryRegion->Attributes
           );
}

/**
  Convert EFI Attributes to Loongarch Attributes.

  @param[in]  EfiAttributes     Efi Attributes.

  @retval  Corresponding architecture attributes.
**/
UINTN
EfiAttributeConverse (
  IN UINTN  EfiAttributes
  )
{
  UINTN  LoongArchAttributes;

  LoongArchAttributes = PAGE_VALID | PAGE_DIRTY | PLV_KERNEL | PAGE_GLOBAL;

  switch (EfiAttributes & EFI_MEMORY_CACHETYPE_MASK) {
    case EFI_MEMORY_UC:
      LoongArchAttributes |= CACHE_SUC;
      break;
    case EFI_MEMORY_WC:
    case EFI_MEMORY_WT:
    case EFI_MEMORY_WB:
      LoongArchAttributes |= CACHE_CC;
      break;
    default:
      LoongArchAttributes |= CACHE_CC;
      break;
  }

  // Write protection attributes
  if (((EfiAttributes & EFI_MEMORY_RO) != 0) ||
      ((EfiAttributes & EFI_MEMORY_WP) != 0))
  {
    LoongArchAttributes &= ~PAGE_DIRTY;
  }

  if ((EfiAttributes & EFI_MEMORY_RP) != 0) {
    LoongArchAttributes |= PAGE_NO_READ;
  }

  // eXecute protection attribute
  if ((EfiAttributes & EFI_MEMORY_XP) != 0) {
    LoongArchAttributes |= PAGE_NO_EXEC;
  }

  return LoongArchAttributes;
}

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
  )
{
  PTE    *Pte;
  UINTN  Attributes;
  UINTN  AttributesTmp;
  UINTN  MaxAddress;

  if (!MmuIsInit ()) {
    return EFI_UNSUPPORTED;
  }

  MaxAddress = LShiftU64 (1ULL, MAX_VA_BITS) - 1;
  Pte        = GetPteAddress (BaseAddress);

  if (Pte == NULL) {
    return EFI_NOT_FOUND;
  }

  Attributes = GET_PAGE_ATTRIBUTES (*Pte);
  if (IS_HUGE_PAGE (Pte->PteVal)) {
    *RegionAttributes = Attributes & (~(PAGE_HUGE));
    *RegionLength    += HUGE_PAGE_SIZE;
  } else {
    *RegionLength    += EFI_PAGE_SIZE;
    *RegionAttributes = Attributes;
  }

  while (BaseAddress <= MaxAddress) {
    Pte = GetPteAddress (BaseAddress);
    if (Pte == NULL) {
      return EFI_SUCCESS;
    }

    AttributesTmp = GET_PAGE_ATTRIBUTES (*Pte);
    if (IS_HUGE_PAGE (Pte->PteVal)) {
      if (AttributesTmp == Attributes) {
        *RegionLength += HUGE_PAGE_SIZE;
      }

      BaseAddress += HUGE_PAGE_SIZE;
    } else {
      if (AttributesTmp == Attributes) {
        *RegionLength += EFI_PAGE_SIZE;
      }

      BaseAddress += EFI_PAGE_SIZE;
    }

    if (BaseAddress > EndAddress) {
      break;
    }
  }

  return EFI_SUCCESS;
}

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
  )
{
  if (!MmuIsInit ()) {
    return EFI_UNSUPPORTED;
  }

  Attributes = EfiAttributeConverse (Attributes);
  MemoryMapPageRange (BaseAddress, BaseAddress + Length, Attributes);

  return EFI_SUCCESS;
}

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
  )
{
  if (MmuIsInit ()) {
    Length = EFI_PAGES_TO_SIZE (EFI_SIZE_TO_PAGES (Length));
    SetMemoryAttributes (BaseAddress, Length, EFI_MEMORY_XP);
  }

  return EFI_SUCCESS;
}

/**
  Check to see if mmu successfully initializes and saves the result.

  @param[in]  ImageHandle  The firmware allocated handle for the EFI image.
  @param[in]  SystemTable  A pointer to the EFI System Table.

  @retval  RETURN_SUCCESS    Initialization succeeded.
**/
RETURN_STATUS
EFIAPI
MmuInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (SWAP_PAGE_DIR != 0) {
    mMmuInited = TRUE;
  }

  return RETURN_SUCCESS;
}
