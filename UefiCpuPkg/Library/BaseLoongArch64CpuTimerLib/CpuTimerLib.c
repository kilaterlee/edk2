/** @file
  CPUCFG 0x4 and 0x5 for Stable Counter frequency instance of Timer Library.

  Copyright (c) 2023, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Register/LoongArch64/Cpucfg.h>

/**
  Calculate clock frequency using CPUCFG 0x4 and 0x5 registers.

  @param  VOID.

  @return The frequency in Hz.

**/
UINT32
EFIAPI
CalcConstFreq (
  VOID
  )
{
  UINT32                 BaseFreq;
  UINT32                 ClockMultiplier;
  UINT32                 ClockDivide;
  CPUCFG_REG4_INFO_DATA  CCFreq;
  CPUCFG_REG5_INFO_DATA  CpucfgReg5Data;
  UINT32                 StableTimerFreq;

  //
  // Get the the crystal frequency corresponding to the constant
  // frequency timer and the clock used by the timer.
  //
  AsmCpucfg (CPUCFG_REG4_INFO, &CCFreq.Uint32);

  //
  // Get the multiplication factor and frequency division factor
  // corresponding to the constant frequency timer and the clock
  // used by the timer.
  //
  AsmCpucfg (CPUCFG_REG5_INFO, &CpucfgReg5Data.Uint32);

  BaseFreq        = CCFreq.Bits.CC_FREQ;
  ClockMultiplier = CpucfgReg5Data.Bits.CC_MUL & 0xFFFFULL;
  ClockDivide     = CpucfgReg5Data.Bits.CC_DIV & 0xFFFFULL;

  if (!BaseFreq || !ClockMultiplier || !ClockDivide) {
    DEBUG ((DEBUG_ERROR, "LoongArch Stable Timer is not available in the CPU, hence this library cannot be used.\n"));
    StableTimerFreq = 0;
    ASSERT (0);
  } else {
    StableTimerFreq = (BaseFreq * ClockMultiplier / ClockDivide);
  }

  return StableTimerFreq;
}

/**
  Stalls the CPU for at least the given number of microseconds.

  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return MicroSeconds

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN UINTN  MicroSeconds
  )
{
  UINTN  Count, Ticks, Start, End;

  Count = (CalcConstFreq () * MicroSeconds) / 1000000;
  Start = AsmReadStableCounter ();
  End   = Start + Count;

  do {
    Ticks = AsmReadStableCounter ();
  } while (Ticks < End);

  return MicroSeconds;
}

/**
  Stalls the CPU for at least the given number of nanoseconds.

  Stalls the CPU for the number of nanoseconds specified by NanoSeconds.

  @param  NanoSeconds The minimum number of nanoseconds to delay.

  @return NanoSeconds

**/
UINTN
EFIAPI
NanoSecondDelay (
  IN UINTN  NanoSeconds
  )
{
  UINT32  MicroSeconds;

  if (NanoSeconds % 1000 == 0) {
    MicroSeconds = NanoSeconds/1000;
  } else {
    MicroSeconds = NanoSeconds/1000 + 1;
  }

  MicroSecondDelay (MicroSeconds);

  return NanoSeconds;
}

/**
  Retrieves the current value of a 64-bit free running performance counter.

  Retrieves the current value of a 64-bit free running performance counter. The
  counter can either count up by 1 or count down by 1. If the physical
  performance counter counts by a larger increment, then the counter values
  must be translated. The properties of the counter can be retrieved from
  GetPerformanceCounterProperties().

  @return The current value of the free running performance counter.

**/
UINT64
EFIAPI
GetPerformanceCounter (
  VOID
  )
{
  return AsmReadStableCounter ();
}

/**
  Retrieves the 64-bit frequency in Hz and the range of performance counter
  values.

  If StartValue is not NULL, then the value that the performance counter starts
  with immediately after is it rolls over is returned in StartValue. If
  EndValue is not NULL, then the value that the performance counter end with
  immediately before it rolls over is returned in EndValue. The 64-bit
  frequency of the performance counter in Hz is always returned. If StartValue
  is less than EndValue, then the performance counter counts up. If StartValue
  is greater than EndValue, then the performance counter counts down. For
  example, a 64-bit free running counter that counts up would have a StartValue
  of 0 and an EndValue of 0xFFFFFFFFFFFFFFFF. A 24-bit free running counter
  that counts down would have a StartValue of 0xFFFFFF and an EndValue of 0.

  @param  StartValue  The value the performance counter starts with when it
                      rolls over.
  @param  EndValue    The value that the performance counter ends with before
                      it rolls over.

  @return The frequency in Hz.

**/
UINT64
EFIAPI
GetPerformanceCounterProperties (
  OUT UINT64  *StartValue   OPTIONAL,
  OUT UINT64  *EndValue     OPTIONAL
  )
{
  if (StartValue != NULL) {
    *StartValue = BIT2;
  }

  if (EndValue != NULL) {
    *EndValue = BIT48 - 1;
  }

  return CalcConstFreq ();
}

/**
  Converts elapsed ticks of performance counter to time in nanoseconds.

  This function converts the elapsed ticks of running performance counter to
  time value in unit of nanoseconds.

  @param  Ticks     The number of elapsed ticks of running performance counter.

  @return The elapsed time in nanoseconds.

**/
UINT64
EFIAPI
GetTimeInNanoSecond (
  IN UINT64  Ticks
  )
{
  UINT64  Frequency;
  UINT64  NanoSeconds;
  UINT64  Remainder;
  INTN    Shift;

  Frequency = GetPerformanceCounterProperties (NULL, NULL);

  //
  //          Ticks
  // Time = --------- x 1,000,000,000
  //        Frequency
  //
  NanoSeconds = MultU64x32 (DivU64x64Remainder (Ticks, Frequency, &Remainder), 1000000000u);

  //
  // Ensure (Remainder * 1,000,000,000) will not overflow 64-bit.
  // Since 2^29 < 1,000,000,000 = 0x3B9ACA00 < 2^30, Remainder should < 2^(64-30) = 2^34,
  // i.e. highest bit set in Remainder should <= 33.
  //
  Shift        = MAX (0, HighBitSet64 (Remainder) - 33);
  Remainder    = RShiftU64 (Remainder, (UINTN)Shift);
  Frequency    = RShiftU64 (Frequency, (UINTN)Shift);
  NanoSeconds += DivU64x64Remainder (MultU64x32 (Remainder, 1000000000u), Frequency, NULL);

  return NanoSeconds;
}
