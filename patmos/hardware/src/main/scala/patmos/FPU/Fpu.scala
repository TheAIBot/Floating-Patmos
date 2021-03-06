package patmos

import Chisel._
import hardfloat._

import Constants._

class FPUPrep() extends Module {
  val io = IO(new Bundle {
    val rs1In = UInt(width = DATA_WIDTH).asInput
    val rs2In = UInt(width = DATA_WIDTH).asInput
    val recodeFromSigned = Bool().asInput
    val roundingMode = UInt(width = F32_ROUNDING_WIDTH).asInput
    val intRs1RecF32Out = UInt(width = RECODED_F32_WIDTH).asOutput
    val floatRs1RawF32Out = new RawFloat(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH).asOutput
    val floatRs2RawF32Out = new RawFloat(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH).asOutput
    val exceptionFlags = UInt(width = F32_EXCEPTION_WIDTH).asOutput
  })

  val f32Rs1AsRawF32 = rawFloatFromFN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH, io.rs1In)
  val f32Rs2AsRawF32 = rawFloatFromFN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH, io.rs2In)

  val intRs1AsRecF32 = Module(new INToRecFN(DATA_WIDTH, BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH))
  intRs1AsRecF32.io.signedIn := io.recodeFromSigned
  intRs1AsRecF32.io.in := io.rs1In
  intRs1AsRecF32.io.roundingMode := io.roundingMode
  intRs1AsRecF32.io.detectTininess := UInt(0)
  
  io.floatRs1RawF32Out := RegNext(f32Rs1AsRawF32)
  io.floatRs2RawF32Out := RegNext(f32Rs2AsRawF32)
  io.intRs1RecF32Out := RegNext(intRs1AsRecF32.io.out)
  io.exceptionFlags := RegNext(intRs1AsRecF32.io.exceptionFlags)
}

class FPUrlMulAddSub() extends Module {
  val io = IO(new Bundle {
      val rs1RawF32In = new RawFloat(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH).asInput
      val rs2RawF32In = new RawFloat(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH).asInput
      val fpuFunc = UInt(width = 4).asInput
      val roundingMode = UInt(width = F32_ROUNDING_WIDTH).asInput
      val mulAddRawF32Out = new RawFloat(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH + 2).asOutput
      val invalidExc = Bool().asOutput
  })

  val isMul = io.fpuFunc === FP_FUNC_MUL
  val isAdd = io.fpuFunc === FP_FUNC_ADD
  
  val rs2RawF32RevSign = Wire(new RawFloat(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH))
  rs2RawF32RevSign.isNaN  := io.rs2RawF32In.isNaN
  rs2RawF32RevSign.isInf  := io.rs2RawF32In.isInf
  rs2RawF32RevSign.isZero := io.rs2RawF32In.isZero
  rs2RawF32RevSign.sign   := !io.rs2RawF32In.sign
  rs2RawF32RevSign.sExp   := io.rs2RawF32In.sExp
  rs2RawF32RevSign.sig    := io.rs2RawF32In.sig

  val mulAddInputA = io.rs1RawF32In
  val mulAddInputB = Mux(isMul, io.rs2RawF32In, rawFloatFromRecFN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH, UInt(BigInt(1)<<(BINARY32_EXP_WIDTH + BINARY32_SIG_WIDTH - 1))))
  val mulAddInputC = Mux(isMul, rawFloatFromIN(Bool(false), UInt(0, DATA_WIDTH)), 
                      Mux(isAdd, io.rs2RawF32In, rs2RawF32RevSign))

  val mulAddRecF32 = Module(new MulAddRecFN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH))
  mulAddRecF32.io.op := UInt(0)
  mulAddRecF32.io.a := mulAddInputA
  mulAddRecF32.io.b := mulAddInputB
  mulAddRecF32.io.c := mulAddInputC
  mulAddRecF32.io.roundingMode := io.roundingMode
  
  io.mulAddRawF32Out := RegNext(mulAddRecF32.io.out)
  io.invalidExc := RegNext(mulAddRecF32.io.invalidExc)
}

class FPUDivSqrt() extends Module {
  val io = IO(new Bundle {
      val rs1RawF32In = new RawFloat(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH).asInput
      val rs2RawF32In = new RawFloat(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH).asInput
      val inValid = Bool().asInput
      val fpuFunc = UInt(width = 4).asInput
      val roundingMode = UInt(width = F32_ROUNDING_WIDTH).asInput
      val divSqrtRawF32Out = new RawFloat(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH + 2).asOutput
      val outValid = Bool().asOutput
      val invalidExc = Bool().asOutput
      val infiniteExc = Bool().asOutput
  })

  val started = RegInit(Bool(false))

  val divSqrtRawF32 = Module(new DivSqrtRecFNToRaw_small(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH, 0))
  divSqrtRawF32.io.inValid := io.inValid
  divSqrtRawF32.io.sqrtOp := io.fpuFunc === FP_RSFUNC_SQRT
  divSqrtRawF32.io.a := io.rs1RawF32In
  divSqrtRawF32.io.b := io.rs2RawF32In
  divSqrtRawF32.io.roundingMode := io.roundingMode

  started := Mux(!started, io.inValid, 
              !(divSqrtRawF32.io.rawOutValid_div || divSqrtRawF32.io.rawOutValid_sqrt))

  io.divSqrtRawF32Out := divSqrtRawF32.io.rawOut
  io.outValid := (divSqrtRawF32.io.rawOutValid_div || divSqrtRawF32.io.rawOutValid_sqrt) && started
  io.invalidExc := divSqrtRawF32.io.invalidExc
  io.infiniteExc := divSqrtRawF32.io.infiniteExc
}

class FPUrSignOps() extends Module {
  val io = IO(new Bundle {
      val rs1F32In = UInt(width = DATA_WIDTH).asInput
      val rs2F32In = UInt(width = DATA_WIDTH).asInput
      val fpuFunc = UInt(width = 4).asInput
      val signF32Out = UInt(width = DATA_WIDTH).asOutput
  })

  val rs1Sign = io.rs1F32In(DATA_WIDTH - 1)
  val rs2Sign = io.rs2F32In(DATA_WIDTH - 1)
  val rdSign = MuxLookup(io.fpuFunc, UInt(0), Array(
    (FP_FUNC_SGNJS, rs2Sign),
    (FP_FUNC_SGNJNS, !rs2Sign),
    (FP_FUNC_SGNJXS, rs1Sign ^ rs2Sign)
  ))

  io.signF32Out := Cat(rdSign, io.rs1F32In(DATA_WIDTH - 2, 0))
}

class FPUc() extends Module {
  val io = IO(new Bundle {
    val rs1RawF32In = new RawFloat(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH).asInput
    val rs2RawF32In = new RawFloat(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH).asInput
    val fpuFunc = UInt(width = 4).asInput
    val isSignaling = Bool().asInput
    val pd = Bool().asOutput
    val exceptionFlags = UInt(width = F32_EXCEPTION_WIDTH).asOutput
  })

  val cmpRecF32 = Module(new CompareRecFN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH))
  cmpRecF32.io.a := io.rs1RawF32In
  cmpRecF32.io.b := io.rs2RawF32In
  cmpRecF32.io.signaling := io.isSignaling

  io.pd := RegNext(MuxLookup(io.fpuFunc, Bool(false), Array(
      (FP_CFUNC_EQ, cmpRecF32.io.eq),
      (FP_CFUNC_LT, cmpRecF32.io.lt),
      (FP_CFUNC_LE, !cmpRecF32.io.gt)
  )))
  io.exceptionFlags := RegNext(cmpRecF32.io.exceptionFlags)
}

class FPURound() extends Module {
  val io = IO(new Bundle {
    val mulAddRawF32 = new RawFloat(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH + 2).asInput
    val mulAddInvalidExc = Bool().asInput
    val mulAddInfiniteExc = Bool().asInput
    val divSqrtRawF32 = new RawFloat(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH + 2).asInput
    val divSqrtInvalidExc = Bool().asInput
    val divSqrtInfiniteExc = Bool().asInput
    val fpuRdSrc = UInt(width = FPU_RD_WIDTH).asInput
    val roundingMode = UInt(width = F32_ROUNDING_WIDTH).asInput
    val roundedRecF32 = UInt(width = RECODED_F32_WIDTH).asOutput
    val exceptionFlags = UInt(width = F32_EXCEPTION_WIDTH).asOutput
  })

  val rawF32Src      = Mux(io.fpuRdSrc === FPU_RD_FROM_MULADD, io.mulAddRawF32, io.divSqrtRawF32)
  val invalidExcSrc  = Mux(io.fpuRdSrc === FPU_RD_FROM_MULADD, io.mulAddInvalidExc, io.divSqrtInvalidExc)
  val infiniteExcSrc = Mux(io.fpuRdSrc === FPU_RD_FROM_MULADD, io.mulAddInfiniteExc, io.divSqrtInfiniteExc)

    val roundAnyRawFNToRecFN =
      Module(new RoundAnyRawFNToRecFN(
        io.mulAddRawF32.expWidth, io.mulAddRawF32.sigWidth, BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH, 0))
    roundAnyRawFNToRecFN.io.invalidExc     := invalidExcSrc
    roundAnyRawFNToRecFN.io.infiniteExc    := infiniteExcSrc
    roundAnyRawFNToRecFN.io.in             := rawF32Src
    roundAnyRawFNToRecFN.io.roundingMode   := io.roundingMode
    roundAnyRawFNToRecFN.io.detectTininess := UInt(0)

    io.roundedRecF32  := RegNext(roundAnyRawFNToRecFN.io.out)
    io.exceptionFlags := RegNext(roundAnyRawFNToRecFN.io.exceptionFlags)
}


class FPUFinish() extends Module {
  val io = IO(new Bundle {
    val rs1F32In = UInt(width = DATA_WIDTH).asInput
    val floatRs1RawF32In = new RawFloat(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH).asInput
    val intRs1RecF32In = UInt(width = RECODED_F32_WIDTH).asInput
    val roundedRecF32In = UInt(width = RECODED_F32_WIDTH).asInput
    val signF32In = UInt(width = DATA_WIDTH).asInput
    val fpuRdSrc = UInt(width = FPU_RD_WIDTH).asInput
    val roundingMode = UInt(width = F32_ROUNDING_WIDTH).asInput
    val recodeToSigned = Bool().asInput
    val intToF32Exceptions = UInt(width = F32_EXCEPTION_WIDTH).asInput
    val roundedRecF32Exceptions = UInt(width = F32_EXCEPTION_WIDTH).asInput
    val rdOut = UInt(width = DATA_WIDTH).asOutput
    val exceptionFlags = UInt(width = F32_EXCEPTION_WIDTH).asOutput
  })
  
  val refF32Class = classifyRecFN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH, io.floatRs1RawF32In)

  val recF32AsInt = Module(new RecFNToIN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH, DATA_WIDTH))
  recF32AsInt.io.in := io.floatRs1RawF32In
  recF32AsInt.io.roundingMode := io.roundingMode
  recF32AsInt.io.signedOut := io.recodeToSigned

  val recF32AsF32Src = Mux(io.fpuRdSrc === FPU_RD_FROM_FLOAT, io.intRs1RecF32In, io.roundedRecF32In)
  val recF32AsF32 = fNFromRecFN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH, recF32AsF32Src)

  io.rdOut := MuxLookup(io.fpuRdSrc, UInt(0), Array(
      (FPU_RD_FROM_FLOAT  , recF32AsF32),
      (FPU_RD_FROM_RS1    , io.rs1F32In),
      (FPU_RD_FROM_INT    , recF32AsInt.io.out),
      (FPU_RD_FROM_CLASS  , refF32Class),
      (FPU_RD_FROM_SIGN   , io.signF32In),
      (FPU_RD_FROM_MULADD , recF32AsF32),
      (FPU_RD_FROM_DIVSQRT, recF32AsF32)
  ))

  io.exceptionFlags := MuxLookup(io.fpuRdSrc, UInt(0), Array(
      (FPU_RD_FROM_FLOAT  , io.intToF32Exceptions),
      (FPU_RD_FROM_RS1    , UInt(0)),
      (FPU_RD_FROM_INT    , recF32AsInt.io.intExceptionFlags),
      (FPU_RD_FROM_CLASS  , UInt(0)),
      (FPU_RD_FROM_SIGN   , UInt(0)),
      (FPU_RD_FROM_MULADD , io.roundedRecF32Exceptions),
      (FPU_RD_FROM_DIVSQRT, io.roundedRecF32Exceptions)
  ))
}