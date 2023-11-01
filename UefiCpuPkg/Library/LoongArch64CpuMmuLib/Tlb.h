/** @file

  Copyright (c) 2023 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TLB_H_
#define TLB_H_

/**
  Invalid corresponding TLB entries are based on the address given

  @param Address The address corresponding to the invalid page table entry

  @retval  none
**/
VOID
InvalidTlb (
  UINTN  Address
  );

/**
  TLB refill handler start.

  @param  none

  @retval none
**/
VOID
HandleTlbRefillStart (
  VOID
  );

/**
  TLB refill handler end.

  @param  none

  @retval none
**/
VOID
HandleTlbRefillEnd (
  VOID
  );

#endif // TLB_H_
