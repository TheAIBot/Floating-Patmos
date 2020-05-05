/*
 * Execution stage of Patmos.
 *
 * Authors: Martin Schoeberl (martin@jopdesign.com)
 *          Wolfgang Puffitsch (wpuffitsch@gmail.com)
 *
 */

package patmos

import Chisel._

import Constants._
import hardfloat._


class Execute() extends Module {
  val io = IO(new ExecuteIO())

  val exReg = Reg(new DecEx())
  when(io.ena) {
    exReg := io.decex
    when(io.flush || io.brflush) {
      exReg.flush()
      exReg.relPc := io.decex.relPc
    }
  }

  def alu(func: UInt, op1: UInt, op2: UInt): UInt = {
    val result = Wire(UInt(width = DATA_WIDTH))
    val scaledOp1 = op1 << Mux(func === FUNC_SHADD2, UInt(2),
                               Mux(func === FUNC_SHADD, UInt(1),
                                   UInt(0)))
    val sum = scaledOp1 + op2
    result := sum // some default
    val shamt = op2(4, 0).toUInt
    val srOp = Mux(func === FUNC_SRA, op1(DATA_WIDTH-1), UInt(0)) ## op1
    // This kind of decoding of the ALU op in the EX stage is not efficient,
    // but we keep it for now to get something going soon.
    switch(func) {
      is(FUNC_ADD)    { result := sum }
      is(FUNC_SUB)    { result := op1 - op2 }
      is(FUNC_XOR)    { result := (op1 ^ op2).toUInt }
      is(FUNC_SL)     { result := (op1 << shamt)(DATA_WIDTH-1, 0).toUInt }
      is(FUNC_SR, FUNC_SRA) { result := (srOp.toSInt >> shamt).toUInt }
      is(FUNC_OR)     { result := (op1 | op2).toUInt }
      is(FUNC_AND)    { result := (op1 & op2).toUInt }
      is(FUNC_NOR)    { result := (~(op1 | op2)).toUInt }
      is(FUNC_SHADD)  { result := sum }
      is(FUNC_SHADD2) { result := sum }
    }
    result
  }

  def comp(func: UInt, op1: UInt, op2: UInt): Bool = {
    val op1s = op1.toSInt
    val op2s = op2.toSInt
    val bitMsk = UInt(1) << op2(4, 0).toUInt
    // Is this nicer than the switch?
    // Some of the comparison function (equ, subtract) could be shared
    val eq = op1 === op2
    val lt = op1s < op2s
    val ult = op1 < op2
    MuxLookup(func.toUInt, Bool(false), Array(
      (CFUNC_EQ,    eq),
      (CFUNC_NEQ,   !eq),
      (CFUNC_LT,    lt),
      (CFUNC_LE,    lt | eq),
      (CFUNC_ULT,   ult),
      (CFUNC_ULE,   ult | eq),
      (CFUNC_BTEST, (op1 & bitMsk) =/= UInt(0))))
  }

  def pred(func: UInt, op1: Bool, op2: Bool): Bool = {
    MuxLookup(func.toUInt, Bool(false), Array(
      (PFUNC_OR, op1 | op2),
      (PFUNC_AND, op1 & op2),
      (PFUNC_XOR, op1 ^ op2),
      (PFUNC_NOR, ~(op1 | op2))))
  }

  // data forwarding
  val fwReg  = Vec(2*PIPE_COUNT, Reg(UInt(width = 3)))
  val fwSrcReg  = Vec(2*PIPE_COUNT, Reg(UInt(width = log2Up(PIPE_COUNT))))
  val memResultDataReg = Vec(PIPE_COUNT, Reg(UInt(width = DATA_WIDTH)))
  val exResultDataReg  = Vec(PIPE_COUNT, Reg(UInt(width = DATA_WIDTH)))
  val op = Vec(2*PIPE_COUNT, UInt(width = DATA_WIDTH))

  // precompute forwarding
  for (i <- 0 until 2*PIPE_COUNT) {
    fwReg(i) := UInt("b000")
    fwSrcReg(i) := UInt(0)
    for (k <- 0 until PIPE_COUNT) {
      when(io.decex.rsAddr(i) === io.memResult(k).addr && io.memResult(k).valid) {
        fwReg(i) := UInt("b010")
        fwSrcReg(i) := UInt(k)
      }
    }
    for (k <- 0 until PIPE_COUNT) {
      when(io.decex.rsAddr(i) === io.exResult(k).addr && io.exResult(k).valid) {
        fwReg(i) := UInt("b001")
        fwSrcReg(i) := UInt(k)
      }
    }    
  }
  for (i <- 0 until PIPE_COUNT) {
    when(io.decex.immOp(i)) {
      fwReg(2*i+1) := UInt("b100")
    }
  }

  when (!io.ena) {
    fwReg := fwReg
    fwSrcReg := fwSrcReg
  }
  when (io.ena) {
    memResultDataReg := io.memResult.map(_.data)
    exResultDataReg := io.exResult.map(_.data)
  }

  // forwarding multiplexers
  for (i <- 0 until PIPE_COUNT) {
    op(2*i) := Mux(fwReg(2*i)(0), exResultDataReg(fwSrcReg(2*i)),
                   Mux(fwReg(2*i)(1), memResultDataReg(fwSrcReg(2*i)),
                       exReg.rsData(2*i)))

    op(2*i+1) := Mux(fwReg(2*i+1)(0), exResultDataReg(fwSrcReg(2*i+1)),
                     Mux(fwReg(2*i+1)(1), memResultDataReg(fwSrcReg(2*i+1)),
                         Mux(fwReg(2*i+1)(2), exReg.immVal(i),
                             exReg.rsData(2*i+1))))
  }

  // predicates
  val predReg = Vec(PRED_COUNT, Reg(Bool()))

  val doExecute = Vec(PIPE_COUNT, Bool())
  for (i <- 0 until PIPE_COUNT) {
    doExecute(i) := Mux(io.flush, Bool(false),
                        predReg(exReg.pred(i)(PRED_BITS-1, 0)) ^ exReg.pred(i)(PRED_BITS))
  }

  // return information
  val retBaseReg = Reg(UInt(width = DATA_WIDTH))
  val retOffReg = Reg(UInt(width = DATA_WIDTH))
  val saveRetOff = Reg(Bool())
  val saveND = Reg(Bool())

  // exception return information
  val excBaseReg = Reg(UInt(width = DATA_WIDTH))
  val excOffReg = Reg(UInt(width = DATA_WIDTH))

  // MS: maybe the multiplication should be in a local component?

  // multiplication result registers
  val mulLoReg = Reg(UInt(width = DATA_WIDTH))
  val mulHiReg = Reg(UInt(width = DATA_WIDTH))

  // multiplication pipeline registers
  val mulLLReg    = Reg(UInt(width = DATA_WIDTH))
  val mulLHReg    = Reg(SInt(width = DATA_WIDTH+1))
  val mulHLReg    = Reg(SInt(width = DATA_WIDTH+1))
  val mulHHReg    = Reg(UInt(width = DATA_WIDTH))

  val mulPipeReg = Reg(Bool())

  // multiplication only in first pipeline
  when(io.ena) {
    mulPipeReg := exReg.aluOp(0).isMul && doExecute(0)

    val signed = exReg.aluOp(0).func === MFUNC_MUL

    val op1H = Cat(Mux(signed, op(0)(DATA_WIDTH-1), UInt("b0")),
                   op(0)(DATA_WIDTH-1, DATA_WIDTH/2)).toSInt
    val op1L = op(0)(DATA_WIDTH/2-1, 0)
    val op2H = Cat(Mux(signed, op(1)(DATA_WIDTH-1), UInt("b0")), 
                   op(1)(DATA_WIDTH-1, DATA_WIDTH/2)).toSInt
    val op2L = op(1)(DATA_WIDTH/2-1, 0)

    mulLLReg := op1L * op2L
    mulLHReg := op1L * op2H
    mulHLReg := op1H * op2L
    mulHHReg := op1H * op2H

    val mulResult = (Cat(mulHHReg, mulLLReg).toSInt
                     + Cat(mulHLReg, SInt(0, width = DATA_WIDTH/2)).toSInt
                     + Cat(mulLHReg, SInt(0, width = DATA_WIDTH/2)).toSInt)

    when(mulPipeReg) {
      mulHiReg := mulResult(2*DATA_WIDTH-1, DATA_WIDTH)
      mulLoReg := mulResult(DATA_WIDTH-1, 0)
    }
  }

  // interface to the stack cache
  io.exsc.op := sc_OP_NONE
  io.exsc.opData := 0.U
  io.exsc.opOff := Mux(exReg.immOp(0), exReg.immVal(0), op(0))

  // stack control instructions
  when(!io.brflush && doExecute(0)) {
    io.exsc.op := exReg.stackOp
  }

  // dual-issue operations
  for (i <- 0 until PIPE_COUNT) {

    val aluResult = alu(exReg.aluOp(i).func, op(2*i), op(2*i+1))
    val compResult = comp(exReg.aluOp(i).func, op(2*i), op(2*i+1))

    val bcpyPs = predReg(exReg.aluOp(i).func(PRED_BITS-1, 0)) ^ exReg.aluOp(i).func(PRED_BITS);
    val shiftedPs = ((UInt(0, DATA_WIDTH-1) ## bcpyPs) << op(2*i+1)(4, 0))(DATA_WIDTH-1, 0)
    val maskedOp = op(2*i) & ~(UInt(1, width = DATA_WIDTH) << op(2*i+1)(4, 0))(DATA_WIDTH-1, 0)
    val bcpyResult = maskedOp | shiftedPs

    // predicate operations
    val ps1 = predReg(exReg.predOp(i).s1Addr(PRED_BITS-1,0)) ^ exReg.predOp(i).s1Addr(PRED_BITS)
    val ps2 = predReg(exReg.predOp(i).s2Addr(PRED_BITS-1,0)) ^ exReg.predOp(i).s2Addr(PRED_BITS)
    val predResult = pred(exReg.predOp(i).func, ps1, ps2)

    when((exReg.aluOp(i).isCmp || exReg.aluOp(i).isPred) && doExecute(i)) {
      predReg(exReg.predOp(i).dest) := Mux(exReg.aluOp(i).isCmp, compResult, predResult)
    }
    predReg(0) := Bool(true)

    // special registers
    when(exReg.aluOp(i).isMTS && doExecute(i)) {
      io.exsc.opData := op(2*i)

      switch(exReg.aluOp(i).func) {
        is(SPEC_FL) {
          predReg := op(2*i)(PRED_COUNT-1, 0)
          predReg(0) := Bool(true)
        }
        is(SPEC_SL) {
          mulLoReg := op(2*i)
        }
        is(SPEC_SH) {
          mulHiReg := op(2*i)
        }
        is(SPEC_ST) {
          io.exsc.op := sc_OP_SET_ST
        }
        is(SPEC_SS) {
          io.exsc.op := sc_OP_SET_MT
        }
        is(SPEC_SRB) {
          retBaseReg := op(2*i)
        }
        is(SPEC_SRO) {
          retOffReg := op(2*i)
        }
        is(SPEC_SXB) {
          excBaseReg := op(2*i)
        }
        is(SPEC_SXO) {
          excOffReg := op(2*i)
        }
      }
    }
    val mfsResult = UInt();
    mfsResult := UInt(0, DATA_WIDTH)
    switch(exReg.aluOp(i).func) {
      is(SPEC_FL) {
        mfsResult := Cat(UInt(0, DATA_WIDTH-PRED_COUNT), predReg.asUInt()).asUInt()
      }
      is(SPEC_SL) {
        mfsResult := mulLoReg
      }
      is(SPEC_SH) {
        mfsResult := mulHiReg
      }
      is(SPEC_ST) {
        mfsResult := io.scex.stackTop
      }
      is(SPEC_SS) {
        mfsResult := io.scex.memTop
      }
      is(SPEC_SRB) {
        mfsResult := retBaseReg
      }
      is(SPEC_SRO) {
        mfsResult := retOffReg
      }
      is(SPEC_SXB) {
        mfsResult := excBaseReg
      }
      is(SPEC_SXO) {
        mfsResult := excOffReg
      }
    }

    // result
    io.exmem.rd(i).addr := exReg.rdAddr(i)
    io.exmem.rd(i).valid := exReg.wrRd(i) && doExecute(i)
    io.exmem.rd(i).data := Mux(exReg.aluOp(i).isMFS, mfsResult,
                               Mux(exReg.aluOp(i).isBCpy, bcpyResult,
                                   aluResult))
  }


  //##################
  //#### FPU land ####
  //##################

  val isFpuRd = Wire(Bool())
  val isFpuPd = Wire(Bool())
  val recodeFromSigned = Wire(Bool())
  val recodeToSigned = Wire(Bool())
  val noCast = Wire(Bool())
  val floatToIntCast = Wire(Bool())
  val rs1IsFloat = Wire(Bool())

  val resFromFloat = Wire(Bool())
  val resFromRs1 = Wire(Bool())
  val resFromInt = Wire(Bool())
  val resFromClassify = Wire(Bool())
  val roundingMode = Wire(UInt(width = 3))
  val isSignaling = Wire(Bool())
  val modifySign = Wire(Bool())
  val isMul = Wire(Bool())
  val isAdd = Wire(Bool())
  val useMulAdd = Wire(Bool())
  


  resFromFloat := Bool(false)
  resFromRs1 := Bool(false)
  resFromInt := Bool(false)
  resFromClassify := Bool(false)
  roundingMode := consts.round_near_even
  isSignaling := Bool(false)
  modifySign := Bool(false)
  isMul := Bool(false)
  isAdd := Bool(false)
  useMulAdd := Bool(false)



  

  isFpuRd := Bool(false)
  isFpuPd := Bool(false)
  recodeFromSigned := Bool(false)
  recodeToSigned := Bool(false)
  noCast := Bool(false)
  floatToIntCast := Bool(false)

  when(exReg.fpuOp.isTR) {
    io.exmem.rd(0).addr := exReg.rdAddr(0)
    io.exmem.rd(0).valid := exReg.wrRd(0) && doExecute(0)

    isFpuRd := Bool(true)
    switch(exReg.fpuOp.func) {
      is(FP_FUNC_ADD) {
        isAdd := Bool(true)
        useMulAdd := Bool(true)
      }
      is(FP_FUNC_SUB) {
        useMulAdd := Bool(true)
      }
      is(FP_FUNC_MUL) {
        isMul := Bool(true)
        useMulAdd := Bool(true)
      }
      is(FP_FUNC_DIV) {
        
      }
      is(FP_FUNC_SGNJS) {
        modifySign := Bool(true)
      }
      is(FP_FUNC_SGNJNS) {
        modifySign := Bool(true)
      }
      is(FP_FUNC_SGNJXS) {
        modifySign := Bool(true)
      }
    }
  }

  when(exReg.fpuOp.isMTF) {
    io.exmem.rd(0).addr := exReg.rdAddr(0)
    io.exmem.rd(0).valid := exReg.wrRd(0) && doExecute(0)

    isFpuRd := Bool(true)
    floatToIntCast := Bool(false)
    switch(exReg.fpuOp.func) {
      is(FP_FPCTFUNC_CVTIS) {
        recodeFromSigned := Bool(true)
        resFromFloat := Bool(true)
      }
      is(FP_FPCTFUNC_CVTUS) {
        recodeFromSigned := Bool(false)
        resFromFloat := Bool(true)
      }
      is(FP_FPCTFUNC_MVIS) {
        resFromRs1 := Bool(true)
      }
    }   
  }

  when(exReg.fpuOp.isMFF) {
    io.exmem.rd(0).addr := exReg.rdAddr(0)
    io.exmem.rd(0).valid := exReg.wrRd(0) && doExecute(0)

    isFpuRd := Bool(true)
    floatToIntCast := Bool(true)
    switch(exReg.fpuOp.func) {
      is(FP_FPCFFUNC_CVTSI) {
        recodeToSigned := Bool(true)
        resFromInt := Bool(true)
        roundingMode := consts.round_minMag
      }
      is(FP_FPCFFUNC_CVTSU) {
        recodeToSigned := Bool(false)
        resFromInt := Bool(true)
        roundingMode := consts.round_minMag
      }
      is(FP_FPCFFUNC_MVSI) {
        resFromRs1 := Bool(true)
      }
      is(FP_FPCFFUNC_CLASS) {
        resFromClassify := Bool(true)
      }
    }
  }

  when(exReg.fpuOp.isCmp) {
    isFpuPd := Bool(true)
    switch(exReg.fpuOp.func) {
      is(FP_CFUNC_EQ) {
        isSignaling := Bool(false)
      }
      is(FP_CFUNC_LT) {
        isSignaling := Bool(true)
      }
      is(FP_CFUNC_LE) {
        isSignaling := Bool(true)
      }
    }
  }

  val fpexceptions1 = Reg(UInt(width = 32))
  val fpexceptions2 = Reg(UInt(width = 32))
  val fpexceptions3 = Reg(UInt(width = 32))
  val fpexceptions4 = Reg(UInt(width = 32))

  val fpuRs1 = op(0)
  val fpuRs2 = op(1)

  //
  //Convert to recoded format
  //

  val f32Rs1AsRecF32 = recFNFromFN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH, fpuRs1)
  val f32Rs2AsRecF32 = recFNFromFN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH, fpuRs2)

  val intRs1AsRecF32 = Module(new INToRecFN(DATA_WIDTH, BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH))
  intRs1AsRecF32.io.signedIn := recodeFromSigned
  intRs1AsRecF32.io.in := fpuRs1
  intRs1AsRecF32.io.roundingMode := roundingMode // todo: add rounding support here
  intRs1AsRecF32.io.detectTininess := UInt(0)
  fpexceptions1 := intRs1AsRecF32.io.exceptionFlags

  val rs1AsRecF32 = Mux(exReg.isFloatSrc1, f32Rs1AsRecF32, intRs1AsRecF32.io.out)
  val rs2AsRecF32 = f32Rs2AsRecF32

  //
  // Do computations
  //

  val rs2RecF32RevSign = Cat(!rs2AsRecF32(rs2AsRecF32.getWidth() - 1), rs2AsRecF32(rs2AsRecF32.getWidth() - 2, 0))
  val mulAddInputA = rs1AsRecF32
  val mulAddInputB = Mux(isMul, rs2AsRecF32, UInt(BigInt(1)<<(BINARY32_EXP_WIDTH + BINARY32_SIG_WIDTH - 1)))
  val mulAddInputC = Mux(isMul, ((rs1AsRecF32 ^ rs2AsRecF32) & UInt(BigInt(1)<<(BINARY32_EXP_WIDTH + BINARY32_SIG_WIDTH - 1)))<<1,
                      Mux(isAdd, rs2AsRecF32, rs2RecF32RevSign))

  val mulAddRecF32 = Module(new MulAddRecFN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH))
  mulAddRecF32.io.op := UInt(0)
  mulAddRecF32.io.a := mulAddInputA
  mulAddRecF32.io.b := mulAddInputB
  mulAddRecF32.io.c := mulAddInputC
  mulAddRecF32.io.roundingMode := roundingMode
  mulAddRecF32.io.detectTininess := UInt(0)
  fpexceptions4 := mulAddRecF32.io.exceptionFlags

  val rs1Sign = fpuRs1(DATA_WIDTH - 1)
  val rs2Sign = fpuRs2(DATA_WIDTH - 1)
  val rdSign = MuxLookup(exReg.fpuOp.func, UInt(0), Array(
    (FP_FUNC_SGNJS, rs2Sign),
    (FP_FUNC_SGNJNS, !rs2Sign),
    (FP_FUNC_SGNJXS, rs1Sign ^ rs2Sign)
  ))

  val cmpRecF32 = Module(new CompareRecFN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH))
  cmpRecF32.io.a := rs1AsRecF32
  cmpRecF32.io.b := rs2AsRecF32
  cmpRecF32.io.signaling := isSignaling
  fpexceptions3 := cmpRecF32.io.exceptionFlags

  val refF32Class = classifyRecFN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH, rs1AsRecF32)

  //
  // Convert from recoded format
  //

  val recF32AsInt = Module(new RecFNToIN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH, DATA_WIDTH))
  recF32AsInt.io.in := rs1AsRecF32
  recF32AsInt.io.roundingMode := roundingMode // todo: add rounding support here
  recF32AsInt.io.signedOut := recodeToSigned
  fpexceptions2 := recF32AsInt.io.intExceptionFlags

  val recF32AsF32Src = Mux(useMulAdd, mulAddRecF32.io.out, rs1AsRecF32)
  val recF32AsF32 = fNFromRecFN(BINARY32_EXP_WIDTH, BINARY32_SIG_WIDTH, recF32AsF32Src)

  when(isFpuRd) {
    io.exmem.rd(0).data := Mux(resFromRs1, fpuRs1, 
                            Mux(resFromFloat, recF32AsF32, 
                              Mux(resFromClassify, refF32Class, 
                                Mux(modifySign, Cat(rdSign, fpuRs1(DATA_WIDTH - 2, 0)),
                                  Mux(useMulAdd, recF32AsF32, recF32AsInt.io.out)))))
  }

  when(isFpuPd) {
    switch(exReg.fpuOp.func) {
      is(FP_CFUNC_EQ) {
        predReg(exReg.predOp(0).dest) := cmpRecF32.io.eq
      }
      is(FP_CFUNC_LT) {
        predReg(exReg.predOp(0).dest) := cmpRecF32.io.lt
      }
      is(FP_CFUNC_LE) {
        predReg(exReg.predOp(0).dest) := !cmpRecF32.io.gt
      }
    }
  }

  debug(fpexceptions1)
  debug(fpexceptions2)
  debug(fpexceptions3)
  debug(fpexceptions4)

  // load/store
  io.exmem.mem.load := exReg.memOp.load && doExecute(0)
  io.exmem.mem.store := exReg.memOp.store && doExecute(0)
  io.exmem.mem.hword := exReg.memOp.hword
  io.exmem.mem.byte := exReg.memOp.byte
  io.exmem.mem.zext := exReg.memOp.zext
  io.exmem.mem.typ := exReg.memOp.typ
  io.exmem.mem.addr := op(0) + exReg.immVal(0)
  io.exmem.mem.data := op(1)

  // call/return
  io.exmem.mem.call := exReg.call && doExecute(0)
  io.exmem.mem.ret  := exReg.ret && doExecute(0)
  io.exmem.mem.brcf := exReg.brcf && doExecute(0)
  io.exmem.mem.trap := exReg.trap && doExecute(0)
  io.exmem.mem.xcall := exReg.xcall && doExecute(0)
  io.exmem.mem.xret := exReg.xret && doExecute(0)
  io.exmem.mem.xsrc := exReg.xsrc
  io.exmem.mem.nonDelayed := exReg.nonDelayed
  io.exmem.mem.illOp := exReg.illOp

  val doCallRet = (exReg.call || exReg.ret || exReg.brcf ||
                   exReg.xcall || exReg.xret) && doExecute(0)

  val brcfOff = Mux(exReg.immOp(0), UInt(0), op(1).toUInt)
  val callRetAddr = Mux(exReg.call || exReg.xcall, UInt(0),
                        Mux(exReg.brcf, brcfOff,
                            Mux(exReg.xret, excOffReg, retOffReg)))

  val callBase = Mux(exReg.immOp(0), exReg.callAddr, op(0).toUInt)
  val callRetBase = Mux(exReg.call || exReg.xcall || exReg.brcf, callBase,
                        Mux(exReg.xret, excBaseReg, retBaseReg))

  io.exmem.mem.callRetBase := callRetBase
  io.exmem.mem.callRetAddr := callRetAddr

  // return information
  when(exReg.call && doExecute(0)) {
    retBaseReg := Cat(exReg.base, UInt("b00").toUInt)
  }
  // the offset is saved when the call is already in the MEM statge
  saveRetOff := exReg.call && doExecute(0) && io.ena
  saveND := exReg.nonDelayed

  // exception return information
  when(exReg.xcall && doExecute(0)) {
    excBaseReg := Cat(exReg.base, UInt("b00").toUInt)
    excOffReg := Cat(exReg.relPc, UInt("b00").toUInt)
  }

  // branch
  io.exfe.doBranch := exReg.jmpOp.branch && doExecute(0)
  val target = Mux(exReg.immOp(0),
                   exReg.jmpOp.target,
                   op(0)(DATA_WIDTH-1, 2).toUInt - exReg.jmpOp.reloc)
  io.exfe.branchPc := target
  io.brflush := exReg.nonDelayed && exReg.jmpOp.branch && doExecute(0)

  // pass on PC
  io.exmem.pc := exReg.pc
  io.exmem.base := exReg.base
  io.exmem.relPc := exReg.relPc

  //call/return for icache
  io.exicache.doCallRet := doCallRet
  io.exicache.callRetBase := callRetBase(31,2)
  io.exicache.callRetAddr := callRetAddr(31,2)

  // suppress writes to special registers
  when(!io.ena) {
    predReg := predReg
    mulLoReg := mulLoReg
    mulHiReg := mulHiReg
    retBaseReg := retBaseReg
    retOffReg := retOffReg
    excBaseReg := excBaseReg
    excOffReg := excOffReg
  }

  // saveRetOff overrides io.ena for writes to retOffReg
  when(saveRetOff) {
    retOffReg := Cat(Mux(saveND, exReg.relPc, io.feex.pc), UInt("b00").toUInt)
  }

  // reset at end to override any computations
  when(reset) {
    exReg.flush()
    predReg(0) := Bool(true)
  }
}
