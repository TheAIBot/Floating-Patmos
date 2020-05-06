package patmos

import Chisel._
import hardfloat._

import Constants._

class FPUPrep() extends Module {
  val io = IO(new Bundle {
    val rs1In = UInt(width = DATA_WIDTH).asInput
    val rs2In = UInt(width = DATA_WIDTH).asInput
    val isrs1Float = Bool().asInput
    val recodeFromSigned = Bool().asInput
    val roundingMode = UInt(width = F32_ROUNDING_WIDTH).asInput
    val rs1RecF32Out = UInt(width = RECODED_F32_WIDTH).asOutput
    val rs2RecF32Out = UInt(width = RECODED_F32_WIDTH).asOutput
    val exceptionFlags = UInt(width = F32_EXCEPTION_WIDTH).asOutput
  })

  val f32Rs1AsRecF32 = recFNFromFN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH, io.rs1In)
  val f32Rs2AsRecF32 = recFNFromFN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH, io.rs2In)

  val intRs1AsRecF32 = Module(new INToRecFN(DATA_WIDTH, BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH))
  intRs1AsRecF32.io.signedIn := io.recodeFromSigned
  intRs1AsRecF32.io.in := io.rs1In
  intRs1AsRecF32.io.roundingMode := io.roundingMode
  intRs1AsRecF32.io.detectTininess := UInt(0)

  io.rs1RecF32Out := RegNext(Mux(io.isrs1Float, f32Rs1AsRecF32, intRs1AsRecF32.io.out))
  io.rs2RecF32Out := RegNext(f32Rs2AsRecF32)
  io.exceptionFlags := RegNext(intRs1AsRecF32.io.exceptionFlags)
}

class FPUrlMulAddSub() extends Module {
  val io = IO(new Bundle {
      val rs1RecF32In = UInt(width = RECODED_F32_WIDTH).asInput
      val rs2RecF32In = UInt(width = RECODED_F32_WIDTH).asInput
      val fpuFunc = UInt(width = 4).asInput
      val roundingMode = UInt(width = F32_ROUNDING_WIDTH).asInput
      val mulAddRecF32Out = UInt(width = RECODED_F32_WIDTH).asOutput
      val exceptionFlags = UInt(width = F32_EXCEPTION_WIDTH).asOutput
  })

  val isMul = io.fpuFunc === FP_FUNC_MUL
  val isAdd = io.fpuFunc === FP_FUNC_ADD
  
  val rs2RecF32RevSign = Cat(!io.rs2RecF32In(io.rs2RecF32In.getWidth() - 1), io.rs2RecF32In(io.rs2RecF32In.getWidth() - 2, 0))
  val mulAddInputA = io.rs1RecF32In
  val mulAddInputB = Mux(isMul, io.rs2RecF32In, UInt(BigInt(1)<<(BINARY32_EXP_WIDTH + BINARY32_SIG_WIDTH - 1)))
  val mulAddInputC = Mux(isMul, ((io.rs1RecF32In ^ io.rs2RecF32In) & UInt(BigInt(1)<<(BINARY32_EXP_WIDTH + BINARY32_SIG_WIDTH - 1)))<<1,
                      Mux(isAdd, io.rs2RecF32In, rs2RecF32RevSign))

  val mulAddRecF32 = Module(new MulAddRecFN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH))
  mulAddRecF32.io.op := UInt(0)
  mulAddRecF32.io.a := mulAddInputA
  mulAddRecF32.io.b := mulAddInputB
  mulAddRecF32.io.c := mulAddInputC
  mulAddRecF32.io.roundingMode := io.roundingMode
  mulAddRecF32.io.detectTininess := UInt(0)
  
  io.mulAddRecF32Out := RegNext(mulAddRecF32.io.out)
  io.exceptionFlags := RegNext(mulAddRecF32.io.exceptionFlags)
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
    val rs1RecF32In = UInt(width = RECODED_F32_WIDTH).asInput
    val rs2RecF32In = UInt(width = RECODED_F32_WIDTH).asInput
    val fpuFunc = UInt(width = 4).asInput
    val isSignaling = Bool().asInput
    val pd = Bool().asOutput
    val exceptionFlags = UInt(width = F32_EXCEPTION_WIDTH).asOutput
  })

  val cmpRecF32 = Module(new CompareRecFN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH))
  cmpRecF32.io.a := io.rs1RecF32In
  cmpRecF32.io.b := io.rs2RecF32In
  cmpRecF32.io.signaling := io.isSignaling

  io.pd := RegNext(MuxLookup(io.fpuFunc, Bool(false), Array(
      (FP_CFUNC_EQ, cmpRecF32.io.eq),
      (FP_CFUNC_LT, cmpRecF32.io.lt),
      (FP_CFUNC_LE, !cmpRecF32.io.gt)
  )))
  io.exceptionFlags := RegNext(cmpRecF32.io.exceptionFlags)
}


class FPUFinish() extends Module {
  val io = IO(new Bundle {
    val rs1F32In = UInt(width = DATA_WIDTH).asInput
    val rs1RecF32In = UInt(width = RECODED_F32_WIDTH).asInput
    val mulAddRecF32In = UInt(width = RECODED_F32_WIDTH).asInput
    val signF32In = UInt(width = DATA_WIDTH).asInput
    val divSqrtRecF32In = UInt(width = RECODED_F32_WIDTH).asInput
    val fpuRdSrc = UInt(width = FPU_RD_WIDTH).asInput
    val roundingMode = UInt(width = F32_ROUNDING_WIDTH).asInput
    val recodeToSigned = Bool().asInput
    val intToF32Exceptions = UInt(width = F32_EXCEPTION_WIDTH).asInput
    val mulAddExceptions = UInt(width = F32_EXCEPTION_WIDTH).asInput
    val divSqrtExceptions = UInt(width = F32_EXCEPTION_WIDTH).asInput
    val rdOut = UInt(width = DATA_WIDTH).asOutput
    val exceptionFlags = UInt(width = F32_EXCEPTION_WIDTH).asOutput
  })
  
  val refF32Class = classifyRecFN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH, io.rs1RecF32In)

  val recF32AsInt = Module(new RecFNToIN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH, DATA_WIDTH))
  recF32AsInt.io.in := io.rs1RecF32In
  recF32AsInt.io.roundingMode := io.roundingMode
  recF32AsInt.io.signedOut := io.recodeToSigned

  val recF32AsF32Src = MuxLookup(io.fpuRdSrc, io.rs1RecF32In, Array(
      (FPU_RD_FROM_MULADD, io.mulAddRecF32In),
      (FPU_RD_FROM_DIVSQRT, io.divSqrtRecF32In)
  ))
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
      (FPU_RD_FROM_MULADD , io.mulAddExceptions),
      (FPU_RD_FROM_DIVSQRT, io.divSqrtExceptions)
  ))
}