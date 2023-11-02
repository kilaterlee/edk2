/** @file
  Memory Detection for Virtual Machines.

  Copyright (c) 2023 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/ResourcePublicationLib.h>
#include <Library/QemuFwCfgLib.h>
#include "Platform.h"

#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS  (128)
#define LOONGARCH_FW_RAM_TOP                BASE_256MB

/**
  Publish PEI core memory

  @return EFI_SUCCESS     The PEIM initialized successfully.
**/
EFI_STATUS
PublishPeiMemory (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT64      Base;
  UINT64      Size;
  UINT64      RamTop;

  //
  // Determine the range of memory to use during PEI
  //
  Base   = FixedPcdGet64 (PcdOvmfSecPeiTempRamBase) + FixedPcdGet32 (PcdOvmfSecPeiTempRamSize);
  RamTop = LOONGARCH_FW_RAM_TOP;
  Size   = RamTop - Base;

  //
  // Publish this memory to the PEI Core
  //
  Status = PublishSystemMemory (Base, Size);
  ASSERT_EFI_ERROR (Status);

  DEBUG ((DEBUG_INFO, "Publish Memory Initialize done.\n"));
  return Status;
}

/**
  Peform Memory Detection
  Publish system RAM and reserve memory regions
**/
VOID
InitializeRamRegions (
  VOID
  )
{
  EFI_STATUS            Status;
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  UINTN                 FwCfgSize;
  MEMMAP_ENTRY          MemoryMapEntry;
  MEMMAP_ENTRY          *StartEntry;
  MEMMAP_ENTRY          *pEntry;
  UINTN                 Processed;

  Status = QemuFwCfgFindFile ("etc/memmap", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a %d read etc/memmap error Status %d \n", __func__, __LINE__, Status));
    return;
  }

  if (FwCfgSize % sizeof MemoryMapEntry != 0) {
    DEBUG ((DEBUG_ERROR, "no MemoryMapEntry FwCfgSize:%d\n", FwCfgSize));
    return;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  StartEntry = AllocatePages (EFI_SIZE_TO_PAGES (FwCfgSize));
  QemuFwCfgReadBytes (FwCfgSize, StartEntry);
  for (Processed = 0; Processed < (FwCfgSize / sizeof MemoryMapEntry); Processed++) {
    pEntry = StartEntry + Processed;
    if (pEntry->Length == 0) {
      continue;
    }

    DEBUG ((DEBUG_INFO, "MemmapEntry Base %p length %p  type %d\n", pEntry->BaseAddr, pEntry->Length, pEntry->Type));
    if (pEntry->Type != EfiAcpiAddressRangeMemory) {
      continue;
    }

    AddMemoryRangeHob (pEntry->BaseAddr, pEntry->BaseAddr + pEntry->Length);
  }

  //
  // When 0 address protection is enabled,
  // 0-4k memory needs to be preallocated to prevent UEFI applications from allocating use,
  // such as grub
  //
  if (PcdGet8 (PcdNullPointerDetectionPropertyMask) & BIT0) {
    BuildMemoryAllocationHob (
      0,
      EFI_PAGE_SIZE,
      EfiBootServicesData
      );
  }
}

/**
  Gets the Virtual Memory Map of corresponding platforms

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU
  on corresponding platforms.

  @param[out]   VirtualMemoryMap    Array of MEMORY_REGION_DESCRIPTOR
                                    describing a Physical-to-Virtual Memory
                                    mapping. This array must be ended by a
                                    zero-filled entry. The allocated memory
                                    will not be freed.
  @retval MemoryTable size.         If -1 is returned, means etc/memmap not exit.
**/
UINTN
EFIAPI
GetMemoryMapPolicy (
  OUT MEMORY_REGION_DESCRIPTOR  **VirtualMemoryMap
  )
{
  EFI_STATUS                Status;
  FIRMWARE_CONFIG_ITEM      FwCfgItem;
  UINTN                     FwCfgSize;
  MEMMAP_ENTRY              MemoryMapEntry;
  MEMMAP_ENTRY              *StartEntry;
  MEMMAP_ENTRY              *pEntry;
  UINTN                     Processed;
  MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;
  UINTN                     Index = 0;

  ASSERT (VirtualMemoryMap != NULL);

  VirtualMemoryTable = AllocatePool (
                         sizeof (MEMORY_REGION_DESCRIPTOR) *
                         MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS
                         );

  //
  // Add the 0x10000000-0x20000000. In the virtual machine, this area use for CPU UART, flash, PIC etc.
  //
  VirtualMemoryTable[Index].PhysicalBase = 0x10000000;
  VirtualMemoryTable[Index].VirtualBase  = VirtualMemoryTable[Index].PhysicalBase;
  VirtualMemoryTable[Index].Length       = 0x10000000;
  VirtualMemoryTable[Index].Attributes   = PAGE_VALID | PLV_KERNEL |  CACHE_SUC | PAGE_DIRTY | PAGE_GLOBAL;
  ++Index;

  Status = QemuFwCfgFindFile ("etc/memmap", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a %d read etc/memmap error Status %d \n", __func__, __LINE__, Status));
    ZeroMem (&VirtualMemoryTable[Index], sizeof (MEMORY_REGION_DESCRIPTOR));
    *VirtualMemoryMap = VirtualMemoryTable;
    return -1;
  }

  if (FwCfgSize % sizeof MemoryMapEntry != 0) {
    DEBUG ((DEBUG_ERROR, "no MemoryMapEntry FwCfgSize:%d\n", FwCfgSize));
  }

  QemuFwCfgSelectItem (FwCfgItem);
  StartEntry = AllocatePages (EFI_SIZE_TO_PAGES (FwCfgSize));
  QemuFwCfgReadBytes (FwCfgSize, StartEntry);
  for (Processed = 0; Processed < (FwCfgSize / sizeof MemoryMapEntry); Processed++) {
    pEntry = StartEntry + Processed;
    if (pEntry->Length == 0) {
      continue;
    }

    DEBUG ((DEBUG_INFO, "MemmapEntry Base %p length %p  type %d\n", pEntry->BaseAddr, pEntry->Length, pEntry->Type));
    VirtualMemoryTable[Index].PhysicalBase = pEntry->BaseAddr;
    VirtualMemoryTable[Index].VirtualBase  = VirtualMemoryTable[Index].PhysicalBase;
    VirtualMemoryTable[Index].Length       = pEntry->Length;
    VirtualMemoryTable[Index].Attributes   = PAGE_VALID | PLV_KERNEL |  CACHE_CC | PAGE_DIRTY | PAGE_GLOBAL;
    ++Index;
  }

  FreePages (StartEntry, EFI_SIZE_TO_PAGES (FwCfgSize));
  // End of Table
  ZeroMem (&VirtualMemoryTable[Index], sizeof (MEMORY_REGION_DESCRIPTOR));
  *VirtualMemoryMap = VirtualMemoryTable;

  return Index;
}
