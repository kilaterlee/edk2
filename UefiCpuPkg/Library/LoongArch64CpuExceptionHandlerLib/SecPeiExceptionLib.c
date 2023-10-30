/** @file SecPeiExceptionLib.c

  LoongArch exception library implemenation for PEI and SEC modules.

  Copyright (c) 2023, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/CpuLib.h>
#include <Library/DebugLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/BaseLib.h>
#include <Library/SerialPortLib.h>
#include <Library/BaseLib.h>
#include <Register/LoongArch64/Csr.h>

#include <Protocol/DebugSupport.h>

#include "ExceptionCommon.h"

/**
  Registers a function to be called from the processor interrupt or exception handler.

  Always return EFI_UNSUPPORTED in the SEC exception initialization module.

  @param  InterruptType    A pointer to the processor's current interrupt state. Set to TRUE if interrupts
                           are enabled and FALSE if interrupts are disabled.
  @param  InterruptHandler A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER that is called
                           when a processor interrupt occurs. If this parameter is NULL, then the handler
                           will be uninstalled.

  @retval EFI_UNSUPPORTED  The interrupt specified by InterruptType is not supported.

**/
EFI_STATUS
RegisterCpuInterruptHandler (
  IN EFI_EXCEPTION_TYPE         InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER  InterruptHandler
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Common exception handler.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

**/
VOID
EFIAPI
CommonExceptionHandler (
  IN     EFI_EXCEPTION_TYPE  ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  //
  // APs may wake up by IPI IRQ during the SEC or PEI phase, clear the IPI interrupt and return.
  //
  if (GetInterruptType (SystemContext) == EXCEPT_LOONGARCH_INT_IPI) {
    DisableLocalInterrupts (1 << EXCEPT_LOONGARCH_INT_IPI);
    IoCsrWrite32 (LOONGARCH_IOCSR_IPI_CLEAR, IoCsrRead32 (LOONGARCH_IOCSR_IPI_STATUS));
    return;
  } else {
    ExceptionType >>= EXCEPT_LOONGARCH_ECODE_SHIFT;
    DefaultExceptionHandler (ExceptionType, SystemContext);
  }
}

/**
  Initializes all CPU exceptions entries and provides the default exception handlers.

  Always return EFI_SUCCESS in the SEC exception initialization module.

  @param[in]  VectorInfo    Pointer to reserved vector list.

  @retval EFI_SUCCESS       CPU Exception Entries have been successfully initialized
                            with default exception handlers.

**/
EFI_STATUS
EFIAPI
InitializeCpuExceptionHandlers (
  IN EFI_VECTOR_HANDOFF_INFO  *VectorInfo OPTIONAL
  )
{
  return EFI_SUCCESS;
}
