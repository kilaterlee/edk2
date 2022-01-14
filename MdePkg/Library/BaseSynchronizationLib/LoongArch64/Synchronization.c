/** @file
  LoongArch synchronization functions.

  Copyright (c) 2021 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>

/**
  Performs an atomic compare exchange operation on a 16-bit
  unsigned integer.

  Performs an atomic compare exchange operation on the 16-bit
  unsigned integer specified by Value.  If Value is equal to
  CompareValue, then Value is set to ExchangeValue and
  CompareValue is returned.  If Value is not equal to
  CompareValue, then Value is returned. The compare exchange
  operation must be performed using MP safe mechanisms.

  @param  Value         A pointer to the 16-bit value for the
                        compare exchange operation.
  @param  CompareValue  16-bit value used in compare operation.
  @param  ExchangeValue 16-bit value used in exchange operation.

  @return The original *Value before exchange.

**/
UINT16
EFIAPI
InternalSyncCompareExchange16 (
  IN      volatile UINT16           *Value,
  IN      UINT16                    CompareValue,
  IN      UINT16                    ExchangeValue
  )
{
  UINT32 RetValue, Temp, Shift;
  UINT64 Mask, LocalCompareValue, LocalExchangeValue;
  volatile UINT32 *Ptr32;

  /* Check that ptr is naturally aligned */
  ASSERT(!((UINT64)Value & (sizeof(Value) - 1)));

  /* Mask inputs to the correct size. */
  Mask = (((~0UL) - (1UL << (0)) + 1) & (~0UL >> (64 - 1 - ((sizeof(UINT16) * 8) - 1))));
  LocalCompareValue = ((UINT64)CompareValue) & Mask;
  LocalExchangeValue = ((UINT64)ExchangeValue) & Mask;

  /*
   * Calculate a shift & mask that correspond to the value we wish to
   * compare & exchange within the naturally aligned 4 byte integer
   * that includes it.
   */
  Shift = (UINT64)Value & 0x3;
  Shift *= 8; /* BITS_PER_BYTE */
  LocalCompareValue <<= Shift;
  LocalExchangeValue <<= Shift;
  Mask <<= Shift;

  /*
   * Calculate a pointer to the naturally aligned 4 byte integer that
   * includes our byte of interest, and load its value.
   */
  Ptr32 = (UINT32 *)((UINT64)Value & ~0x3);

  __asm__ __volatile__ (
    "1:               \n"
    "ll.w  %0, %3     \n"
    "and   %1, %0, %4 \n"
    "bne   %1, %5, 2f \n"
    "or    %1, %1, %6 \n"
    "sc.w  %1, %2     \n"
    "beqz  %1, 1b     \n"
    "b     3f         \n"
    "2:               \n"
    "dbar  0          \n"
    "3:               \n"
    : "=&r" (RetValue), "=&r" (Temp), "=" "ZC" (*Ptr32)
    : "ZC" (*Ptr32), "Jr" (~Mask), "Jr" (LocalCompareValue), "Jr" (LocalExchangeValue)
    : "memory"
  );

  return (RetValue & Mask) >> Shift;
}

/**
  Performs an atomic compare exchange operation on a 32-bit
  unsigned integer.

  Performs an atomic compare exchange operation on the 32-bit
  unsigned integer specified by Value.  If Value is equal to
  CompareValue, then Value is set to ExchangeValue and
  CompareValue is returned.  If Value is not equal to
  CompareValue, then Value is returned. The compare exchange
  operation must be performed using MP safe mechanisms.

  @param  Value         A pointer to the 32-bit value for the
                        compare exchange operation.
  @param  CompareValue  32-bit value used in compare operation.
  @param  ExchangeValue 32-bit value used in exchange operation.

  @return The original *Value before exchange.

**/
UINT32
EFIAPI
InternalSyncCompareExchange32 (
  IN      volatile UINT32           *Value,
  IN      UINT32                    CompareValue,
  IN      UINT32                    ExchangeValue
  )
{
  UINT32 RetValue;

  __asm__ __volatile__ (
    "1:              \n"
    "ll.w %0, %2     \n"
    "bne  %0, %3, 2f \n"
    "move %0, %4     \n"
    "sc.w %0, %1     \n"
    "beqz %0, 1b     \n"
    "b    3f         \n"
    "2:              \n"
    "dbar 0          \n"
    "3:              \n"
    : "=&r" (RetValue), "=" "ZC" (*Value)
    : "ZC" (*Value), "Jr" (CompareValue), "Jr" (ExchangeValue)
    : "memory"
  );
  return RetValue;
}

/**
  Performs an atomic compare exchange operation on a 64-bit unsigned integer.

  Performs an atomic compare exchange operation on the 64-bit unsigned integer specified
  by Value.  If Value is equal to CompareValue, then Value is set to ExchangeValue and
  CompareValue is returned.  If Value is not equal to CompareValue, then Value is returned.
  The compare exchange operation must be performed using MP safe mechanisms.

  @param  Value         A pointer to the 64-bit value for the compare exchange
                        operation.
  @param  CompareValue  64-bit value used in compare operation.
  @param  ExchangeValue 64-bit value used in exchange operation.

  @return The original *Value before exchange.

**/
UINT64
EFIAPI
InternalSyncCompareExchange64 (
  IN      volatile UINT64           *Value,
  IN      UINT64                    CompareValue,
  IN      UINT64                    ExchangeValue
  )
{
  UINT64 RetValue;

  __asm__ __volatile__ (
    "1:              \n"
    "ll.d %0, %2     \n"
    "bne  %0, %3, 2f \n"
    "move %0, %4     \n"
    "sc.d %0, %1     \n"
    "beqz %0, 1b     \n"
    "b    3f         \n"
    "2:              \n"
    "dbar 0          \n"
    "3:              \n"
    : "=&r" (RetValue), "=" "ZC" (*Value)
    : "ZC" (*Value), "Jr" (CompareValue), "Jr" (ExchangeValue)
    : "memory"
  );
  return RetValue;
}

/**
  Performs an atomic increment of an 32-bit unsigned integer.

  Performs an atomic increment of the 32-bit unsigned integer specified by
  Value and returns the incremented value. The increment operation must be
  performed using MP safe mechanisms. The state of the return value is not
  guaranteed to be MP safe.

  @param  Value A pointer to the 32-bit value to increment.

  @return The incremented value.

**/
UINT32
EFIAPI
InternalSyncIncrement (
  IN      volatile UINT32           *Value
  )
{
  UINT32 Temp = *Value;

  __asm__ __volatile__(
    "dbar    0          \n"
    "amadd.w %1, %2, %0 \n"
    : "+ZB" (*Value), "=&r" (Temp)
    : "r" (1)
    : "memory"
  );
  return *Value;
}

/**
  Performs an atomic decrement of an 32-bit unsigned integer.

  Performs an atomic decrement of the 32-bit unsigned integer specified by
  Value and returns the decrement value. The decrement operation must be
  performed using MP safe mechanisms. The state of the return value is not
  guaranteed to be MP safe.

  @param  Value A pointer to the 32-bit value to decrement.

  @return The decrement value.

**/
UINT32
EFIAPI
InternalSyncDecrement (
  IN      volatile UINT32           *Value
  )
{
  UINT32 Temp = *Value;

  __asm__ __volatile__(
    "dbar    0          \n"
    "amadd.w %1, %2, %0 \n"
    : "+ZB" (*Value), "=&r" (Temp)
    : "r" (-1)
    : "memory"
  );
  return *Value;
}
