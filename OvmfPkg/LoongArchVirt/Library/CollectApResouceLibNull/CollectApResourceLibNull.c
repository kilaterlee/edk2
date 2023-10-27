/** @file
  LoongArch64 CPU Collect AP resource NULL Library functions.

  Copyright (c) 2023, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Register/LoongArch64/Csr.h>
#include "../../../UefiCpuPkg/Library/LoongArch64MpInitLib/MpLib.h"

VOID
SaveProcessorResourceData (
  IN PROCESSOR_RESOURCE_DATA *
  );

VOID
EFIAPI
SaveProcessorResource (
  PROCESSOR_RESOURCE_DATA  *mProcessorResource
  )
{
  SaveProcessorResourceData (mProcessorResource);
}

VOID
EFIAPI
CollectAllProcessorResource (
  VOID
  )
{
  return;
}
