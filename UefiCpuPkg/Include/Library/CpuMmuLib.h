/** @file

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CPU_MMU_LIB_H_
#define CPU_MMU_LIB_H_

#include <Uefi/UefiBaseType.h>

/**
  Finds the first of the length and memory properties of the memory region corresponding
  to the specified base address.

  @param[in]       BaseAddress       To find the base address of the memory region.
  @param[in, out]  RegionLength      Pointer holding:
                                      - At entry, the length of the memory region
                                        expected to be found.
                                      - At exit, the length of the memory region found.
  @param[out]      RegionAttributes  Properties of the memory region found.

  @retval  EFI_SUCCESS    The corresponding memory area was successfully found
           EFI_NOT_FOUND    No memory area found
**/
EFI_STATUS
EFIAPI
GetMemoryRegionAttributes (
  IN     UINTN  BaseAddress,
  IN OUT UINTN  *RegionLength,
  OUT    UINTN  *RegionAttributes
  );

/**
  Sets the Attributes  of the specified memory region.

  @param[in]  BaseAddress    The base address of the memory region to set the Attributes.
  @param[in]  Length         The length of the memory region to set the Attributes.
  @param[in]  Attributes     The Attributes to be set.
  @param[in]  AttributeMask  Mask of memory attributes to take into account.

  @retval  EFI_SUCCESS    The Attributes was set successfully
**/
EFI_STATUS
EFIAPI
SetMemoryRegionAttributes (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINTN                 Length,
  IN UINTN                 Attributes,
  IN UINT64                AttributeMask
  );

#endif // CPU_MMU_LIB_H_
