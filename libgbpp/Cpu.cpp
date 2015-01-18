/*
 *   Copyright (C) 2010, 2011 by Claudemiro Alves Feitosa Neto
 *   <dimiro1@gmail.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licences>
 */

#include "Cpu.h"


namespace gbpp {

  Cpu::Cpu() : in_bios(true), AF(A, F), HL(H, L), BC(B, C), DE(D, E) {
    reset(0x100);
  }

  void Cpu::reset(const word start_pc) {
    // If using Bios, it is not necessary set this registers
    // Because Bios does this.
    AF = 0x01B0;
    BC = 0x0013;
    DE = 0x00D8;
    HL = 0x014D;
    SP = 0xFFFE;
    PC = start_pc;
    ime = true;
    pending_interupt_enabled = false;
    pending_interupt_disabled = false;
    halt = false;
    is_cbop = false;
    executed_instructions = 0;
    timer_counter   = 0;
    divider_counter = 0;
    clock_speed = clock_speed_available[0];
    speed_mode = NORMAL_SPEED;
    cpu_time = 0;
  }

  // Verify if it is executing Bios.
  bool Cpu::is_in_bios() {
    if(in_bios && (PC == 0x100)) {
      debug(PC);
      in_bios = false;
    }
    return in_bios;
  }

  int Cpu::max_cycles() const {
    return speed_mode * MAX_CYCLES;
  }

  int Cpu::get_cpu_time() const {
    return cpu_time;
  }

  bool Cpu::can_execute() {
    if(cpu_time < max_cycles()) {
      return true;
    } else if(cpu_time > max_cycles()) {
      cpu_time -= max_cycles(); // adjust frame_cycles, to be very accurate
    } else {
      cpu_time = 0;
    }
    return false;
  }

  /**
   * Do SRL
   * @param r Register
   */
  inline void Cpu::SRL(Register &v) {
    F.c = test_bit(v, 0);
    v = v >> 1;
    F.n = false;
    F.h = false;
    F.z = (v == 0);
  }

  /**
   * Push w to the stack
   * @param w value to be put in the stack
   */
  inline void Cpu::PUSH(const word w) {
    SP = SP - 2;
    memory.write_word(SP, w);
  }

  /**
   * Pop value from stack
   * @return w Value from stack
   */
  inline void Cpu::POP(Register &w) {
    w = memory.read_word(SP);
    SP = SP + 2;
  }

  /**
   * Do Ret
   */
  inline void Cpu::RET() {
    POP(PC);
  }

  /**
   * Do RET
   * @param Flag
   */
  inline void Cpu::RET(const bool ff) {
    if (ff) {
      RET();
    }
  }

  /**
   * Do RETI
   */
  inline void Cpu::RETI() {
    RET();
    ime = true;
    pending_interupt_enabled = false;
    pending_interupt_disabled = false;
  }

  /**
   * Do JR
   */
  inline void Cpu::JR() {
    sbyte T1 = static_cast<sbyte>(memory.read_byte(PC));
    PC = PC + T1 + 1;
  }


  /**
   * Do JR
   * @param f Flag
   */
  inline void Cpu::JR(const bool f) {
    if (f) {
      JR();
    } else {
      PC = PC + 1;
    }
  }

  /**
   * Do JP
   */
  inline void Cpu::JP() {
    PC = memory.read_word(PC);
  }

  /**
   * Do JP
   * @param f Flag
   */
  inline void Cpu::JP(const bool f) {
    if (f) {
      JP();
    } else {
      PC = PC + 2;
    }
  }

  /**
   * Do SWAP
   * @param r Register
   */
  inline void Cpu::SWAP(Register &v) {
    v = (((v & 0xF0) >> 4) | ((v & 0x0F) << 4));
    F.h = false;
    F.n = false;
    F.c = false;
    F.z = (v == 0);
  }

  /**
   * Do AND
   */
  inline void Cpu::AND(const byte n) {
    A = A & n;
    F.z = (A == 0);
    F.h = true;
    F.n = false;
    F.c = false;
  }

  /**
   * Do XOR
   */
  inline void Cpu::XOR(const byte n) {
    A = A ^ n;
    F.z = (A == 0);
    F.h = false;
    F.n = false;
    F.c = false;
  }

  /**
   * Do OR
   */
  inline void Cpu::OR(const byte n) {
    A = A | n;
    F.z = (A == 0);
    F.h = false;
    F.n = false;
    F.c = false;
  }

  /**
   * Do CP
   */
  inline void Cpu::CP(const byte n) {
    F.z = (A == n);
    F.n = true;
    F.c = (A < n);
    F.h = ((A & 0x0F) < (n & 0x0F));
  }

  /**
   * Do DEC 8 bits
   * @param r Register
   */
  inline void Cpu::DEC(Register &v) {
    v = v - 1;
    F.h = ((v & 0x0F) == 0xF);
    F.z = (v == 0);
    F.n = true;
  }

  /**
   * Do INC 8 bits
   * @param r Register
   */
  inline void Cpu::INC(Register &v) {
    v = v + 1;
    F.h = ((v & 0xF) == 0);
    F.z = (v == 0);
    F.n = false;
  }

  /**
   * Do RLC
   * @param r Register
   */
  inline void Cpu::RLC(Register &v) {
    F.c = test_bit(v, 7);
    v = ((v << 1) & 0xFF) | F.c;
    F.n = false;
    F.h = false;
    F.z = (v == 0);
  }

  /**
   * Do DAA
   * Stolen from VisualBoyAdvance
   */
  inline void Cpu::DAA() {
    word T1 = A;

    if(F.c) {
      T1 |= 256;
    }
    if(F.h) {
      T1 |= 512;
    }
    if(F.n) {
      T1 |= 1024;
    }
    T1 = daa_table[T1];
    A = T1 >> 8;
    F = T1 & 0xFF;
  }

  /**
   * ADD RA RB
   * @param ra register to be incremented
   * @param rb increment
   */
  template<typename T>
  inline void Cpu::ADDW(Register &ra, const T rb) {
    F.n = false;
    F.h = (((ra & 0xFFF) + (rb & 0xFFF)) > 0xFFF);
    F.c = ((ra + rb) > 0xFFFF);
    ra = ra + rb;
  }

  /**
   * Do RRC
   * @param r Register
   */
  inline void Cpu::RRC(Register &v) {
    F.c = test_bit(v, 0);
    v = (v >> 1) | (F.c << 7);
    F.n = false;
    F.h = false;
    F.z = (v == 0);
  }

  /**
   * Do RR
   * @param r Register
   */
  inline void Cpu::RR(Register &v) {
    int T1 = F.c;
    F.c = test_bit(v, 0);
    v = (v >> 1) | (T1 << 7);
    F.n = false;
    F.h = false;
    F.z = (v == 0);
  }

  /**
   * Do RL
   * @param r Register
   */
  inline void Cpu::RL(Register &v) {
    int T1 = F.c;
    F.c = test_bit(v, 7);
    v = ((v << 1) & 0xFF) | T1;
    F.n = false;
    F.h = false;
    F.z = (v == 0);
  }

  /**
   * Do Call
   */
  inline void Cpu::CALL() {
    PUSH(PC + 2);
    JP();
  }

  /**
   * Do Call
   * @param f Flag
   */
  inline void Cpu::CALL(const bool f) {
    if (f) {
      CALL();
    } else {
      PC = PC + 2;
    }
  }

  /**
   * ADD A RB
   * @param r increment
   */
  inline void Cpu::ADD(const byte v) {
    F.n = false;
    F.h = (((A & 0xF) + (v & 0xF)) > 0xF);
    F.c = (((A & 0xFF) + (v & 0xFF)) > 0xFF);
    A = A + v;
    F.z = (A == 0);
  }

  /**
   * ADC A RB
   * @param rb increment
   */
  inline void Cpu::ADC(const byte v) {
    int T1 = F.c;
    F.n = false;
    F.h = ((((A & 0xF) + (v & 0xF)) + T1) > 0xF);
    F.c = ((((A & 0xFF) + (v & 0xFF)) + T1) > 0xFF);
    A = A + v + T1;
    F.z = (A == 0);
  }

  /**
   * SUB RB
   * @param rb vaue to be decremented
   */
  inline void Cpu::SUB(const byte v) {
    F.h = ((A & 0xF) < (v & 0xF));
    F.c = ((A & 0xFF) < (v & 0xFF));
    F.n = true;
    A = A - v;
    F.z = (A == 0);
  }

  /**
   * SBC A,RB
   * @param rb vaue to be decremented
   */
  inline void Cpu::SBC(const byte v) {
    int T1 = F.c;
    F.h = ((A & 0xF) < ((v & 0xF) + T1));
    F.c = ((A & 0xFF) < ((v & 0xFF) + T1));
    F.n = true;
    A = (A - v - T1);
    F.z = (A == 0);
  }

  /**
   * Do SLA
   * @param r Register
   */
  inline void Cpu::SLA(Register &v) {
    F.c = test_bit(v, 7);
    v = v << 1;
    F.z = (v == 0);
    F.n = false;
    F.h = false;
  }

  /**
   * Do SRA
   * @param r Register
   */
  inline void Cpu::SRA(Register &v) {
    F.c = (v & 1) ? true : false;
    v = ((v >> 1) | (v & 0x80));
    F.z = (v == 0);
    F.n = false;
    F.h = false;
  }

  inline void Cpu::BIT(const byte v, const int bit) {
    F.z = !test_bit(v, bit);
    F.n = false;
    F.h = true;
  }

  inline void Cpu::RES(Register &v, const int bit) {
    clear_bit(v, bit);
  }

  inline void Cpu::SET(Register &v, const int bit) {
    set_bit(v, bit);
  }

  inline void Cpu::LD_HL_SP_n() {
    sbyte T1 = memory.read_byte(PC);
    PC = PC + 1;
    HL = SP + T1;
    F.z = false;
    F.n = false;
    F.h = (((SP & 0xF) + (T1 & 0xF)) > 0xF);
    F.c = (((SP & 0xFF) + (T1 & 0xFF)) > 0xFF);
  }

  /* Process instructions */
  int Cpu::execute() {
    Register16 T1; // temp
    sbyte T2; // temp
    byte cbop;
    byte op;
    int total_cycles = 0;
    int interrupts_cycles = 0;

    // TODO: Halt with interrupts
    if(halt) {
      // Will wait for an interrupt forever :(
      // Because the interrupt is executed in the instruction before this.
      //return 4;
    }

    // Interrupts are disabled after instruction after DI is executed.
    if(pending_interupt_disabled) {
      if(memory.read_byte(PC - 1) != 0xF3) {
        pending_interupt_disabled = false;
        ime = false;
      }
    }

    // Interrupts are enabled after instruction after EI is executed.
    if(pending_interupt_enabled) {
      if(memory.read_byte(PC - 1) != 0xFB) {
        pending_interupt_enabled = false;
        ime = true;
      }
    }

    op = memory.read_byte(PC);
    PC = PC + 1;
    //debug(PC-1);
    //printf("OP: 0x%x\n", PC - 1);

    switch(op) {
    case 0x00: /* NOP */
      break;
    case 0x01: /* LD BC,n */
      BC = memory.read_word(PC);
      PC = PC + 2;
      break;
    case 0x02: /* LD (BC),A */
      memory.write_byte(BC, A);
      break;
    case 0x03: /* INC BC */
      BC = BC + 1;
      break;
    case 0x04: /* INC B */
      INC(B);
      break;
    case 0x05: /* DEC B */
      DEC(B);
      break;
    case 0x06: /* LD B,n */
      B = memory.read_byte(PC);
      PC = PC + 1;
      break;
    case 0x07: /* RLCA */
      RLC(A);
      F.z = false;
      break;
    case 0x08: /* LD (n),SP */
      memory.write_word(memory.read_word(PC), SP);
      PC = PC + 2;
      break;
    case 0x09: /* ADD HL,BC */
      ADDW(HL, BC);
      break;
    case 0x0A: /* LD A,(BC) */
      A = memory.read_byte(BC);
      break;
    case 0x0B: /* DEC BC */
      BC = BC - 1;
      break;
    case 0x0C: /* INC C */
      INC(C);
      break;
    case 0x0D: /* DEC C */
      DEC(C);
      break;
    case 0x0E: /* LD C,n */
      C = memory.read_byte(PC);
      PC = PC + 1;
      break;
    case 0x0F: /* RRCA */
      RRC(A);
      F.z = false;
      break;
    case 0x10: /* STOP */
      PC = PC + 1;
      break;
    case 0x11: /* LD DE,n */
      DE = memory.read_word(PC);
      PC = PC + 2;
      break;
    case 0x12: /* LD (DE),A */
      memory.write_byte(DE, A);
      break;
    case 0x13: /* INC DE */
      DE = DE + 1;
      break;
    case 0x14: /* INC D */
      INC(D);
      break;
    case 0x15: /* DEC D */
      DEC(D);
      break;
    case 0x16: /* LD D,n */
      D = memory.read_byte(PC);
      PC = PC + 1;
      break;
    case 0x17: /* RLA */
      RL(A);
      F.z = false;
      break;
    case 0x18: /* JR */
      JR();
      break;
    case 0x19: /* ADD HL,DE */
      ADDW(HL, DE);
      break;
    case 0x1A: /* LD A,(DE) */
      A = memory.read_byte(DE);
      break;
    case 0x1B: /* DEC DE */
      DE = DE - 1;
      break;
    case 0x1C: /* INC E */
      INC(E);
      break;
    case 0x1D: /* DEC E */
      DEC(E);
      break;
    case 0x1E: /* LD E,n */
      E = memory.read_byte(PC);
      PC = PC + 1;
      break;
    case 0x1F: /* RRA */
      RR(A);
      F.z = false;
      break;
    case 0x20: /* JR NZ */
      JR(!F.z);
      break;
    case 0x21: /* LD HL,n */
      HL = memory.read_word(PC);
      PC = PC + 2;
      break;
    case 0x22: /* LDI (HL),A */
      memory.write_byte(HL, A);
      HL = HL + 1;
      break;
    case 0x23: /* INC HL */
      HL = HL + 1;
      break;
    case 0x24: /* INC H */
      INC(H);
      break;
    case 0x25: /* DEC H */
      DEC(H);
      break;
    case 0x26: /* LD H,n */
      H = memory.read_byte(PC);
      PC = PC + 1;
      break;
    case 0x27: /* DAA */ // Decimal Adjust Accumulator
      DAA();
      break;
    case 0x28: /* JR Z */
      JR(F.z);
      break;
    case 0x29: /* ADD HL,HL */
      ADDW(HL, HL);
      break;
    case 0x2A: /* LDI A,(HL) */
      A = memory.read_byte(HL);
      HL = HL + 1;
      break;
    case 0x2B: /* DEC HL */
      HL = HL - 1;
      break;
    case 0x2C: /* INC L */
      INC(L);
      break;
    case 0x2D: /* DEC L */
      DEC(L);
      break;
    case 0x2E: /* LD L,n */
      L = memory.read_byte(PC);
      PC = PC + 1;
      break;
    case 0x2F: /* CPL */
      A = ~A;
      F.n = true;
      F.h = true;
      break;
    case 0x30: /* JR NC */
      JR(!F.c);
      break;
    case 0x31: /* LD SP,n */
      SP = memory.read_word(PC);
      PC = PC + 2;
      break;
    case 0x32: /* LDD (HL),A */
      memory.write_byte(HL, A);
      HL = HL - 1;
      break;
    case 0x33: /* INC SP */
      SP = SP + 1;
      break;
    case 0x34: /* INC (HL) */
      T1 = memory.read_byte(HL);
      INC(T1);
      memory.write_byte(HL, T1);
      break;
    case 0x35: /* DEC (HL) */
      T1 = memory.read_byte(HL);
      DEC(T1);
      memory.write_byte(HL, T1);
      break;
    case 0x36: /* LD (HL),n */
      memory.write_byte(HL, memory.read_byte(PC));
      PC = PC + 1;
      break;
    case 0x37: /* SCF */
      F.c = true;
      F.n = false;
      F.h = false;
      break;
    case 0x38: /* JR C */
      JR(F.c);
      break;
    case 0x39: /* ADD HL,SP */
      ADDW(HL, SP);
      break;
    case 0x3A: /* LDD A,(HL) */
      A = memory.read_byte(HL);
      HL = HL - 1;
      break;
    case 0x3B: /* DEC SP */
      SP = SP - 1;
      break;
    case 0x3C: /* INC A */
      INC(A);
      break;
    case 0x3D: /* DEC A */
      DEC(A);
      break;
    case 0x3E: /* LD A,n */
      A = memory.read_byte(PC);
      PC = PC + 1;
      break;
    case 0x3F: /* CCF */
      F.c = F.c ? false : true;
      F.n = false;
      F.h = false;
      break;
    case 0x40: /* LD B,B */
      break;
    case 0x41: /* LD B,C */
      B = C;
      break;
    case 0x42: /* LD B,D */
      B = D;
      break;
    case 0x43: /* LD B,E */
      B = E;
      break;
    case 0x44: /* LD B,H */
      B = H;
      break;
    case 0x45: /* LD B,L */
      B = L;
      break;
    case 0x46: /* LD B,(HL) */
      B = memory.read_byte(HL);
      break;
    case 0x47: /* LD B,A */
      B = A;
      break;
    case 0x48: /* LD C,B */
      C = B;
      break;
    case 0x49: /* LD C,C */
      break;
    case 0x4A: /* LD C,D */
      C = D;
      break;
    case 0x4B: /* LD C,E */
      C = E;
      break;
    case 0x4C: /* LD C,H */
      C = H;
      break;
    case 0x4D: /* LD C,L */
      C = L;
      break;
    case 0x4E: /* LD C,(HL) */
      C = memory.read_byte(HL);
      break;
    case 0x4F: /* LD C,A */
      C = A;
      break;
    case 0x50: /* LD D,B */
      D = B;
      break;
    case 0x51: /* LD D,C */
      D = C;
      break;
    case 0x52: /* LD D,D */
      break;
    case 0x53: /* LD D,E */
      D = E;
      break;
    case 0x54: /* LD D,H */
      D = H;
      break;
    case 0x55: /* LD D,L */
      D = L;
      break;
    case 0x56: /* LD D,(HL) */
      D = memory.read_byte(HL);
      break;
    case 0x57: /* LD D,A */
      D = A;
      break;
    case 0x58: /* LD E,B */
      E = B;
      break;
    case 0x59: /* LD E,C */
      E = C;
      break;
    case 0x5A: /* LD E,D */
      E = D;
      break;
    case 0x5B: /* LD E,E */
      break;
    case 0x5C: /* LD E,H */
      E = H;
      break;
    case 0x5D: /* LD E,L */
      E = L;
      break;
    case 0x5E: /* LD E,(HL) */
      E = memory.read_byte(HL);
      break;
    case 0x5F: /* LD E,A */
      E = A;
      break;
    case 0x60: /* LD H,B */
      H = B;
      break;
    case 0x61: /* LD H,C */
      H = C;
      break;
    case 0x62: /* LD H,D */
      H = D;
      break;
    case 0x63: /* LD H,E */
      H = E;
      break;
    case 0x64: /* LD H,H */
      break;
    case 0x65: /* LD H,L */
      H = L;
      break;
    case 0x66: /* LD H,(HL) */
      H = memory.read_byte(HL);
      break;
    case 0x67: /* LD H,A */
      H = A;
      break;
    case 0x68: /* LD L,B */
      L = B;
      break;
    case 0x69: /* LD L,C */
      L = C;
      break;
    case 0x6A: /* LD L,D */
      L = D;
      break;
    case 0x6B: /* LD L,E */
      L = E;
      break;
    case 0x6C: /* LD L,H */
      L = H;
      break;
    case 0x6D: /* LD L,L */
      break;
    case 0x6E: /* LD L,(HL) */
      L = memory.read_byte(HL);
      break;
    case 0x6F: /* LD L,A */
      L = A;
      break;
    case 0x70: /* LD (HL),B */
      memory.write_byte(HL, B);
      break;
    case 0x71: /* LD (HL),C */
      memory.write_byte(HL, C);
      break;
    case 0x72: /* LD (HL),D */
      memory.write_byte(HL, D);
      break;
    case 0x73: /* LD (HL),E */
      memory.write_byte(HL, E);
      break;
    case 0x74: /* LD (HL),H */
      memory.write_byte(HL, H);
      break;
    case 0x75: /* LD (HL),L */
      memory.write_byte(HL, L);
      break;
    case 0x76: /* HALT */
      if(ime) {
        halt = true;
      }
      break;
    case 0x77: /* LD (HL),A */
      memory.write_byte(HL, A);
      break;
    case 0x78: /* LD A,B */
      A = B;
      break;
    case 0x79: /* LD A,C */
      A = C;
      break;
    case 0x7A: /* LD A,D */
      A = D;
      break;
    case 0x7B: /* LD A,E */
      A = E;
      break;
    case 0x7C: /* LD A,H */
      A = H;
      break;
    case 0x7D: /* LD A,L */
      A = L;
      break;
    case 0x7E: /* LD A,(HL) */
      A = memory.read_byte(HL);
      break;
    case 0x7F: /* LD A,A */
      break;
    case 0x80: /* ADD A,B */
      ADD(B);
      break;
    case 0x81: /* ADD A,C */
      ADD(C);
      break;
    case 0x82: /* ADD A,D */
      ADD(D);
      break;
    case 0x83: /* ADD A,E */
      ADD(E);
      break;
    case 0x84: /* ADD A,H */
      ADD(H);
      break;
    case 0x85: /* ADD A,L */
      ADD(L);
      break;
    case 0x86: /* ADD A,(HL) */
      ADD(memory.read_byte(HL));
      break;
    case 0x87: /* ADD A,A */
      ADD(A);
      break;
    case 0x88: /* ADC A,B */
      ADC(B);
      break;
    case 0x89: /* ADC A,C */
      ADC(C);
      break;
    case 0x8A: /* ADC A,D */
      ADC(D);
      break;
    case 0x8B: /* ADC A,E */
      ADC(E);
      break;
    case 0x8C: /* ADC A,H */
      ADC(H);
      break;
    case 0x8D: /* ADC A,L */
      ADC(L);
      break;
    case 0x8E: /* ADC A,(HL) */
      ADC(memory.read_byte(HL));
      break;
    case 0x8F: /* ADC A,A */
      ADC(A);
      break;
    case 0x90: /* SUB B */
      SUB(B);
      break;
    case 0x91: /* SUB C */
      SUB(C);
      break;
    case 0x92: /* SUB D */
      SUB(D);
      break;
    case 0x93: /* SUB E */
      SUB(E);
      break;
    case 0x94: /* SUB H */
      SUB(H);
      break;
    case 0x95: /* SUB L */
      SUB(L);
      break;
    case 0x96: /* SUB (HL) */
      SUB(memory.read_byte(HL));
      break;
    case 0x97: /* SUB A */
      SUB(A);
      break;
    case 0x98: /* SBC A,B */
      SBC(B);
      break;
    case 0x99: /* SBC A,C */
      SBC(C);
      break;
    case 0x9A: /* SBC A,D */
      SBC(D);
      break;
    case 0x9B: /* SBC A,E */
      SBC(E);
      break;
    case 0x9C: /* SBC A,H */
      SBC(H);
      break;
    case 0x9D: /* SBC A,L */
      SBC(L);
      break;
    case 0x9E: /* SBC A,(HL) */
      SBC(memory.read_byte(HL));
      break;
    case 0x9F: /* SBC A,A */
      SBC(A);
      break;
    case 0xA0: /* AND B */
      AND(B);
      break;
    case 0xA1: /* AND C */
      AND(C);
      break;
    case 0xA2: /* AND D */
      AND(D);
      break;
    case 0xA3: /* AND E */
      AND(E);
      break;
    case 0xA4: /* AND H */
      AND(H);
      break;
    case 0xA5: /* AND L */
      AND(L);
      break;
    case 0xA6: /* AND (HL) */
      AND(memory.read_byte(HL));
      break;
    case 0xA7: /* AND A */
      AND(A);
      break;
    case 0xA8: /* XOR B */
      XOR(B);
      break;
    case 0xA9: /* XOR C */
      XOR(C);
      break;
    case 0xAA: /* XOR D */
      XOR(D);
      break;
    case 0xAB: /* XOR E */
      XOR(E);
      break;
    case 0xAC: /* XOR H */
      XOR(H);
      break;
    case 0xAD: /* XOR L */
      XOR(L);
      break;
    case 0xAE: /* XOR (HL) */
      XOR(memory.read_byte(HL));
      break;
    case 0xAF: /* XOR A */
      XOR(A);
      break;
    case 0xB0: /* OR B */
      OR(B);
      break;
    case 0xB1: /* OR C */
      OR(C);
      break;
    case 0xB2: /* OR D */
      OR(D);
      break;
    case 0xB3: /* OR E */
      OR(E);
      break;
    case 0xB4: /* OR H */
      OR(H);
      break;
    case 0xB5: /* OR L */
      OR(L);
      break;
    case 0xB6: /* OR (HL) */
      OR(memory.read_byte(HL));
      break;
    case 0xB7: /* OR A */
      OR(A);
      break;
    case 0xB8: /* CP B */
      CP(B);
      break;
    case 0xB9: /* CP C */
      CP(C);
      break;
    case 0xBA: /* CP D */
      CP(D);
      break;
    case 0xBB: /* CP E */
      CP(E);
      break;
    case 0xBC: /* CP H */
      CP(H);
      break;
    case 0xBD: /* CP L */
      CP(L);
      break;
    case 0xBE: /* CP (HL) */
      CP(memory.read_byte(HL));
      break;
    case 0xBF: /* CP A */
      CP(A);
      break;
    case 0xC0: /* RET NZ */
      RET(!F.z);
      break;
    case 0xC1: /* POP BC */
      POP(BC);
      break;
    case 0xC2: /* JP NZ */
      JP(!F.z);
      break;
    case 0xC3: /* JP */
      JP();
      break;
    case 0xC4: /* CALL NZ */
      CALL(!F.z);
      break;
    case 0xC5: /* PUSH BC */
      PUSH(BC);
      break;
    case 0xC6: /* ADD A,n */
      ADD(memory.read_byte(PC));
      PC = PC + 1;
      break;
    case 0xC7: /* RST 0 */
      PUSH(PC);
      PC = 0x00;
      break;
    case 0xC8: /* RET Z */
      RET(F.z);
      break;
    case 0xC9: /* RET */
      RET();
      break;
    case 0xCA: /* JP Z */
      JP(F.z);
      break;
    case 0xCC: /* CALL Z */
      CALL(F.z);
      break;
    case 0xCD: /* CALL */
      CALL();
      break;
    case 0xCE:  /* ADC A,n */
      ADC(memory.read_byte(PC));
      PC = PC + 1;
      break;
    case 0xCF: /* RST 8 */
      PUSH(PC);
      PC = 0x08;
      break;
    case 0xD0: /* RET NC */
      RET(!F.c);
      break;
    case 0xD1: /* POP DE */
      POP(DE);
      break;
    case 0xD2: /* JP NC */
      JP(!F.c);
      break;
    case 0xD3: // unknown
      abort(op, PC, false);
      break;
    case 0xD4: /* CALL NC */
      CALL(!F.c);
      break;
    case 0xD5: /* PUSH DE */
      PUSH(DE);
      break;
    case 0xD6: /* SUB u8 */
      SUB(memory.read_byte(PC));
      PC = PC + 1;
      break;
    case 0xD7: /* RST 10 */
      PUSH(PC);
      PC = 0x10;
      break;
    case 0xD8: /* RET C */
      RET(F.c);
      break;
    case 0xD9: /* RETI */
      RETI();
      break;
    case 0xDA: /* JP C */
      JP(F.c);
      break;
    case 0xDB: /* unknown */
      abort(op, PC, false);
      break;
    case 0xDC: /* CALL C */
      CALL(F.c);
      break;
    case 0xDD: /* unknown  */
      abort(op, PC, false);
      break;
    case 0xDE: /* SBC A,n */
      SBC(memory.read_byte(PC));
      PC = PC + 1;
      break;
    case 0xDF: /* RST 18 */
      PUSH(PC);
      PC = 0x18;
      break;
    case 0xE0: /* LDH (n),A */
      memory.writehi(memory.read_byte(PC), A);
      PC = PC + 1;
      break;
    case 0xE1: /* POP HL */
      POP(HL);
      break;
    case 0xE2: /* LDH (C),A */
      memory.writehi(C, A);
      break;
    case 0xE3: /* unknown  */
      abort(op, PC, false);
      break;
    case 0xE4: /* unknown  */
      abort(op, PC, false);
      break;
    case 0xE5: /* PUSH HL */
      PUSH(HL);
      break;
    case 0xE6: /* AND n */
      AND(memory.read_byte(PC));
      PC = PC + 1;
      break;
    case 0xE7: /* RST 20 */
      PUSH(PC);
      PC = 0x20;
      break;
    case 0xE8: /* ADD SP,n */
      ADDW(SP, static_cast<sbyte>(memory.read_byte(PC)));
      PC = PC + 1;
      F.z = false;
      break;
    case 0xE9: /* JP HL */
      PC = HL;
      break;
    case 0xEA: /* LD (n),A */
      memory.write_byte(memory.read_word(PC), A);
      PC = PC + 2;
      break;
    case 0xEB:  /* unknown  */
      abort(op, PC, false);
      break;
    case 0xEC: /* unknown  */
      abort(op, PC, false);
      break;
    case 0xED: /* unknown  */
      abort(op, PC, false);
      break;
    case 0xEE: /* XOR n */
      XOR(memory.read_byte(PC));
      PC = PC + 1;
      break;
    case 0xEF: /* RST 28 */
      PUSH(PC);
      PC = 0x28;
      break;
    case 0xF0: /* LDH A,(n) */
      A = memory.readhi(memory.read_byte(PC));
      PC = PC + 1;
      break;
    case 0xF1: /* POP AF */
      POP(AF);
      break;
    case 0xF2: /* LDH A,(C) */
      A = memory.readhi(C);
      break;
    case 0xF3: /* DI */
      pending_interupt_disabled = true;
      break;
    case 0xF4: /* unknown  */
      abort(op, PC, false);
      break;
    case 0xF5: /* PUSH AF */
      PUSH(AF);
      break;
    case 0xF6: /* OR n  */
      OR(memory.read_byte(PC));
      PC = PC + 1;
      break;
    case 0xF7: /* RST 30 */
      PUSH(PC);
      PC = 0x30;
      break;
    case 0xF8: /* LD HL,SP+n */
      LD_HL_SP_n();
      break;
    case 0xF9: /* LD SP,HL */
      SP = HL;
      break;
    case 0xFA: /* LD A,(n) */
      A = memory.read_byte(memory.read_word(PC));
      PC = PC + 2;
      break;
    case 0xFB: /* EI */
      pending_interupt_enabled = true;
      break;
    case 0xFC: /* unknown */
      abort(op, PC, false);
      break;
    case 0xFD: /* unknown */
      abort(op, PC, false);
      break;
    case 0xFE: /* CP n */
      CP(memory.read_byte(PC));
      PC = PC + 1;
      break;
    case 0xFF: /* RST 38 */
      PUSH(PC);
      PC = 0x38;
      break;
    case 0xCB: /* CB prefix */
      cbop = memory.read_byte(PC);
      PC = PC + 1;
      is_cbop = true;
      switch(cbop) {
      case 0x00: /* RLC B */
        RLC(B);
        break;
      case 0x01: /* RLC C */
        RLC(C);
        break;
      case 0x02: /* RLC D */
        RLC(D);
        break;
      case 0x03: /* RLC E */
        RLC(E);
        break;
      case 0x04: /* RLC H */
        RLC(H);
        break;
      case 0x05: /* RLC L */
        RLC(L);
        break;
      case 0x06: // RLC (HL)
        T1 = memory.read_byte(HL);
        RLC(T1);
        memory.write_byte(HL, T1);
        break;
      case 0x07: /* RLC A */
        RLC(A);
        break;
      case 0x08: /* RRC B */
        RRC(B);
        break;
      case 0x09: /* RRC C */
        RRC(C);
        break;
      case 0x0A: /* RRC D */
        RRC(D);
        break;
      case 0x0B: /* RRC E */
        RRC(E);
        break;
      case 0x0C: /* RRC H */
        RRC(H);
        break;
      case 0x0D: /* RRC L */
        RRC(L);
        break;
      case 0x0E: /* RRC (HL) */
        T1 = memory.read_byte(HL);
        RRC(T1);
        memory.write_byte(HL, T1);
        break;
      case 0x0F: /* RRC A */
        RRC(A);
        break;
      case 0x10: /* RL B */
        RL(B);
        break;
      case 0x11: /* RL C */
        RL(C);
        break;
      case 0x12: /* RL D */
        RL(D);
        break;
      case 0x13: /* RL E */
        RL(E);
        break;
      case 0x14: /* RL H */
        RL(H);
        break;
      case 0x15: /* RL L */
        RL(L);
        break;
      case 0x16: // RL (HL)
        T1 = memory.read_byte(HL);
        RL(T1);
        memory.write_byte(HL, T1);
        break;
      case 0x17: /* RL A */
        RL(A);
        break;
      case 0x18: /* RR B */
        RR(B);
        break;
      case 0x19: /* RR C */
        RR(C);
        break;
      case 0x1A: /* RR D */
        RR(D);
        break;
      case 0x1B: /* RR E */
        RR(E);
        break;
      case 0x1C: /* RR H */
        RR(H);
        break;
      case 0x1D: /* RR L */
        RR(L);
        break;
      case 0x1E: /* RR (HL) */
        T1 = memory.read_byte(HL);
        RR(T1);
        memory.write_byte(HL, T1);
        break;
      case 0x1F: /* RR A */
        RR(A);
        break;
      case 0x20: /* SLA B */
        SLA(B);
        break;
      case 0x21: /* SLA C */
        SLA(C);
        break;
      case 0x22: /* SLA D */
        SLA(D);
        break;
      case 0x23: /* SLA E */
        SLA(E);
        break;
      case 0x24: /* SLA H */
        SLA(H);
        break;
      case 0x25: /* SLA L */
        SLA(L);
        break;
      case 0x26: /* SLA (HL) */
        T1 = memory.read_byte(HL);
        SLA(T1);
        memory.write_byte(HL, T1);
        break;
      case 0x27: /* SLA A */
        SLA(A);
        break;
      case 0x28: /* SRA B */
        SRA(B);
        break;
      case 0x29: /* SRA C */
        SRA(C);
        break;
      case 0x2A: /* SRA D */
        SRA(D);
        break;
      case 0x2B: /* SRA E */
        SRA(E);
        break;
      case 0x2C: /* SRA H */
        SRA(H);
        break;
      case 0x2D: /* SRA L */
        SRA(L);
        break;
      case 0x2E: /* SRA (HL) */
        T1 = memory.read_byte(HL);
        SRA(T1);
        memory.write_byte(HL, T1);
        break;
      case 0x2F: /* SRA A */
        SRA(A);
        break;
      case 0x30: /* SWAP B */
        SWAP(B);
        break;
      case 0x31: /* SWAP C */
        SWAP(C);
        break;
      case 0x32: /* SWAP D */
        SWAP(D);
        break;
      case 0x33: /* SWAP E */
        SWAP(E);
        break;
      case 0x34: /* SWAP H */
        SWAP(H);
        break;
      case 0x35: /* SWAP L */
        SWAP(L);
        break;
      case 0x36: /* SWAP (HL) */
        T1 = memory.read_byte(HL);
        SWAP(T1);
        memory.write_byte(HL, T1);
        break;
      case 0x37: /* SWAP A */
        SWAP(A);
        break;
      case 0x38: /* SRL B */
        SRL(B);
        break;
      case 0x39: /* SRL C */
        SRL(C);
        break;
      case 0x3A: /* SRL D */
        SRL(D);
        break;
      case 0x3B: /* SRL E */
        SRL(E);
        break;
      case 0x3C: /* SRL H */
        SRL(H);
        break;
      case 0x3D: /* SRL L */
        SRL(L);
        break;
      case 0x3E: /* SRL (HL) */
        T1 = memory.read_byte(HL);
        SRL(T1);
        memory.write_byte(HL, T1);
        break;
      case 0x3F: /* SRL A */
        SRL(A);
        break;
      case 0x40: /* BIT 0 B */
        BIT(B, 0);
        break;
      case 0x41: /* BIT 0 C */
        BIT(C, 0);
        break;
      case 0x42: /* BIT 0 D */
        BIT(D, 0);
        break;
      case 0x43: /* BIT 0 E */
        BIT(E, 0);
        break;
      case 0x44: /* BIT 0 H */
        BIT(H, 0);
        break;
      case 0x45: /* BIT 0 L */
        BIT(L, 0);
        break;
      case 0x46: /* BIT 0 (HL) */
        BIT(memory.read_byte(HL), 0);
        break;
      case 0x47: /* BIT 0 A */
        BIT(A, 0);
        break;
      case 0x48: /* BIT 1 B */
        BIT(B, 1);
        break;
      case 0x49: /* BIT 1 C */
        BIT(C, 1);
        break;
      case 0x4A: /* BIT 1 D */
        BIT(D, 1);
        break;
      case 0x4B: /* BIT 1 E */
        BIT(E, 1);
        break;
      case 0x4C: /* BIT 1 H */
        BIT(H, 1);
        break;
      case 0x4D: /* BIT 1 L */
        BIT(L, 1);
        break;
      case 0x4E: /* BIT 1 (HL) */
        BIT(memory.read_byte(HL), 1);
        break;
      case 0x4F: /* BIT 1 A */
        BIT(A, 1);
        break;
      case 0x50: /* BIT 2 B */
        BIT(B, 2);
        break;
      case 0x51: /* BIT 2 C */
        BIT(C, 2);
        break;
      case 0x52: /* BIT 2 D */
        BIT(D, 2);
        break;
      case 0x53: /* BIT 2 E */
        BIT(E, 2);
        break;
      case 0x54: /* BIT 2 H */
        BIT(H, 2);
        break;
      case 0x55: /* BIT 2 L */
        BIT(L, 2);
        break;
      case 0x56: /* BIT 2 (HL) */
        BIT(memory.read_byte(HL), 2);
        break;
      case 0x57: /* BIT 2 A */
        BIT(A, 2);
        break;
      case 0x58: /* BIT 3 B */
        BIT(B, 3);
        break;
      case 0x59: /* BIT 3 C */
        BIT(C, 3);
        break;
      case 0x5A: /* BIT 3 D */
        BIT(D, 3);
        break;
      case 0x5B: /* BIT 3 E */
        BIT(E, 3);
        break;
      case 0x5C: /* BIT 3 H */
        BIT(H, 3);
        break;
      case 0x5D: /* BIT 3 L */
        BIT(L, 3);
        break;
      case 0x5E: /* BIT 3 (HL) */
        BIT(memory.read_byte(HL), 3);
        break;
      case 0x5F: /* BIT 3 A */
        BIT(A, 3);
        break;
      case 0x60: /* BIT 4 B */
        BIT(B, 4);
        break;
      case 0x61: /* BIT 4 C */
        BIT(C, 4);
        break;
      case 0x62: /* BIT 4 D */
        BIT(D, 4);
        break;
      case 0x63: /* BIT 4 E */
        BIT(E, 4);
        break;
      case 0x64: /* BIT 4 H */
        BIT(H, 4);
        break;
      case 0x65: /* BIT 4 L */
        BIT(L, 4);
        break;
      case 0x66: /* BIT 4 (HL) */
        BIT(memory.read_byte(HL), 4);
        break;
      case 0x67: /* BIT 4 A */
        BIT(A, 4);
        break;
      case 0x68: /* BIT 5 B */
        BIT(B, 5);
        break;
      case 0x69: /* BIT 5 C */
        BIT(C, 5);
        break;
      case 0x6A: /* BIT 5 D */
        BIT(D, 5);
        break;
      case 0x6B: /* BIT 5 E */
        BIT(E, 5);
        break;
      case 0x6C: /* BIT 5 H */
        BIT(H, 5);
        break;
      case 0x6D: /* BIT 5 L */
        BIT(L, 5);
        break;
      case 0x6E: /* BIT 5 (HL) */
        BIT(memory.read_byte(HL), 5);
        break;
      case 0x6F: /* BIT 5 A */
        BIT(A, 5);
        break;
      case 0x70: /* BIT 6 B */
        BIT(B, 6);
        break;
      case 0x71: /* BIT 6 C */
        BIT(C, 6);
        break;
      case 0x72: /* BIT 6 D */
        BIT(D, 6);
        break;
      case 0x73: /* BIT 6 E */
        BIT(E, 6);
        break;
      case 0x74: /* BIT 6 H */
        BIT(H, 6);
        break;
      case 0x75: /* BIT 6 L */
        BIT(L, 6);
        break;
      case 0x76: /* BIT 6 (HL) */
        BIT(memory.read_byte(HL), 6);
        break;
      case 0x77: /* BIT 6 A */
        BIT(A, 6);
        break;
      case 0x78: /* BIT 7 B */
        BIT(B, 7);
        break;
      case 0x79: /* BIT 7 C */
        BIT(C, 7);
        break;
      case 0x7A: /* BIT 7 D */
        BIT(D, 7);
        break;
      case 0x7B: /* BIT 7 E */
        BIT(E, 7);
        break;
      case 0x7C: /* BIT 7 H */
        BIT(H, 7);
        break;
      case 0x7D: /* BIT 7 L */
        BIT(L, 7);
        break;
      case 0x7E: /* BIT 7 (HL) */
        BIT(memory.read_byte(HL), 7);
        break;
      case 0x7F: /* BIT 7 A */
        BIT(A, 7);
        break;

      case 0x80: /* RES 0 B */
        RES(B, 0);
        break;
      case 0x81: /* RES 0 C */
        RES(C, 0);
        break;
      case 0x82: /* RES 0 D */
        RES(D, 0);
        break;
      case 0x83: /* RES 0 E */
        RES(E, 0);
        break;
      case 0x84: /* RES 0 H */
        RES(H, 0);
        break;
      case 0x85: /* RES 0 L */
        RES(L, 0);
        break;
      case 0x86: /* RES 0 (HL) */
        T1 = memory.read_byte(HL);
        RES(T1, 0);
        memory.write_byte(HL, T1);
        break;
      case 0x87: /* RES 0 A */
        RES(A, 0);
        break;
      case 0x88: /* RES 1 B */
        RES(B, 1);
        break;
      case 0x89: /* RES 1 C */
        RES(C, 1);
        break;
      case 0x8A: /* RES 1 D */
        RES(D, 1);
        break;
      case 0x8B: /* RES 1 E */
        RES(E, 1);
        break;
      case 0x8C: /* RES 1 H */
        RES(H, 1);
        break;
      case 0x8D: /* RES 1 L */
        RES(L, 1);
        break;
      case 0x8E: /* RES 1 (HL) */
        T1 = memory.read_byte(HL);
        RES(T1, 1);
        memory.write_byte(HL, T1);
        break;
      case 0x8F: /* RES 1 A */
        RES(A, 1);
        break;
      case 0x90: /* RES 2 B */
        RES(B, 2);
        break;
      case 0x91: /* RES 2 C */
        RES(C, 2);
        break;
      case 0x92: /* RES 2 D */
        RES(D, 2);
        break;
      case 0x93: /* RES 2 E */
        RES(E, 2);
        break;
      case 0x94: /* RES 2 H */
        RES(H, 2);
        break;
      case 0x95: /* RES 2 L */
        RES(L, 2);
        break;
      case 0x96: /* RES 2 (HL) */
        T1 = memory.read_byte(HL);
        RES(T1, 2);
        memory.write_byte(HL, T1);
        break;
      case 0x97: /* RES 2 A */
        RES(A, 2);
        break;
      case 0x98: /* RES 3 B */
        RES(B, 3);
        break;
      case 0x99: /* RES 3 C */
        RES(C, 3);
        break;
      case 0x9A: /* RES 3 D */
        RES(D, 3);
        break;
      case 0x9B: /* RES 3 E */
        RES(E, 3);
        break;
      case 0x9C: /* RES 3 H */
        RES(H, 3);
        break;
      case 0x9D: /* RES 3 L */
        RES(L, 3);
        break;
      case 0x9E: /* RES 3 (HL) */
        T1 = memory.read_byte(HL);
        RES(T1, 3);
        memory.write_byte(HL, T1);
        break;
      case 0x9F: /* RES 3 A */
        RES(A, 3);
        break;
      case 0xA0: /* RES 4 B */
        RES(B, 4);
        break;
      case 0xA1: /* RES 4 C */
        RES(C, 4);
        break;
      case 0xA2: /* RES 4 D */
        RES(D, 4);
        break;
      case 0xA3: /* RES 4 E */
        RES(E, 4);
        break;
      case 0xA4: /* RES 4 H */
        RES(H, 4);
        break;
      case 0xA5: /* RES 4 L */
        RES(L, 4);
        break;
      case 0xA6: /* RES 4 (HL) */
        T1 = memory.read_byte(HL);
        RES(T1, 4);
        memory.write_byte(HL, T1);
        break;
      case 0xA7: /* RES 4 A */
        RES(A, 4);
        break;
      case 0xA8: /* RES 5 B */
        RES(B, 5);
        break;
      case 0xA9: /* RES 5 C */
        RES(C, 5);
        break;
      case 0xAA: /* RES 5 D */
        RES(D, 5);
        break;
      case 0xAB: /* RES 5 E */
        RES(E, 5);
        break;
      case 0xAC: /* RES 5 H */
        RES(H, 5);
        break;
      case 0xAD: /* RES 5 L */
        RES(L, 5);
        break;
      case 0xAE: /* RES 5 (HL) */
        T1 = memory.read_byte(HL);
        RES(T1, 5);
        memory.write_byte(HL, T1);
        break;
      case 0xAF: /* RES 5 A */
        RES(A, 5);
        break;
      case 0xB0: /* RES 6 B */
        RES(B, 6);
        break;
      case 0xB1: /* RES 6 C */
        RES(C, 6);
        break;
      case 0xB2: /* RES 6 D */
        RES(D, 6);
        break;
      case 0xB3: /* RES 6 E */
        RES(E, 6);
        break;
      case 0xB4: /* RES 6 H */
        RES(H, 6);
        break;
      case 0xB5: /* RES 6 L */
        RES(L, 6);
        break;
      case 0xB6: /* RES 6 (HL) */
        T1 = memory.read_byte(HL);
        RES(T1, 6);
        memory.write_byte(HL, T1);
        break;
      case 0xB7: /* RES 6 A */
        RES(A, 6);
        break;
      case 0xB8: /* RES 7 B */
        RES(B, 7);
        break;
      case 0xB9: /* RES 7 C */
        RES(C, 7);
        break;
      case 0xBA: /* RES 7 D */
        RES(D, 7);
        break;
      case 0xBB: /* RES 7 E */
        RES(E, 7);
        break;
      case 0xBC: /* RES 7 H */
        RES(H, 7);
        break;
      case 0xBD: /* RES 7 L */
        RES(L, 7);
        break;
      case 0xBE: /* RES 7 (HL) */
        T1 = memory.read_byte(HL);
        RES(T1, 7);
        memory.write_byte(HL, T1);
        break;
      case 0xBF: /* RES 7 A */
        RES(A, 7);
        break;

      case 0xC0: /* SET 0 B */
        SET(B, 0);
        break;
      case 0xC1: /* SET 0 C */
        SET(C, 0);
        break;
      case 0xC2: /* SET 0 D */
        SET(D, 0);
        break;
      case 0xC3: /* SET 0 E */
        SET(E, 0);
        break;
      case 0xC4: /* SET 0 H */
        SET(H, 0);
        break;
      case 0xC5: /* SET 0 L */
        SET(L, 0);
        break;
      case 0xC6: /* SET 0 (HL) */
        T1 = memory.read_byte(HL);
        SET(T1, 0);
        memory.write_byte(HL, T1);
        break;
      case 0xC7: /* SET 0 A */
        SET(A, 0);
        break;
      case 0xC8: /* SET 1 B */
        SET(B, 1);
        break;
      case 0xC9: /* SET 1 C */
        SET(C, 1);
        break;
      case 0xCA: /* SET 1 D */
        SET(D, 1);
        break;
      case 0xCB: /* SET 1 E */
        SET(E, 1);
        break;
      case 0xCC: /* SET 1 H */
        SET(H, 1);
        break;
      case 0xCD: /* SET 1 L */
        SET(L, 1);
        break;
      case 0xCE: /* SET 1 (HL) */
        T1 = memory.read_byte(HL);
        SET(T1, 1);
        memory.write_byte(HL, T1);
        break;
      case 0xCF: /* SET 1 A */
        SET(A, 1);
        break;
      case 0xD0: /* SET 2 B */
        SET(B, 2);
        break;
      case 0xD1: /* SET 2 C */
        SET(C, 2);
        break;
      case 0xD2: /* SET 2 D */
        SET(D, 2);
        break;
      case 0xD3: /* SET 2 E */
        SET(E, 2);
        break;
      case 0xD4: /* SET 2 H */
        SET(H, 2);
        break;
      case 0xD5: /* SET 2 L */
        SET(L, 2);
        break;
      case 0xD6: /* SET 2 (HL) */
        T1 = memory.read_byte(HL);
        SET(T1, 2);
        memory.write_byte(HL, T1);
        break;
      case 0xD7: /* SET 2 A */
        SET(A, 2);
        break;
      case 0xD8: /* SET 3 B */
        SET(B, 3);
        break;
      case 0xD9: /* SET 3 C */
        SET(C, 3);
        break;
      case 0xDA: /* SET 3 D */
        SET(D, 3);
        break;
      case 0xDB: /* SET 3 E */
        SET(E, 3);
        break;
      case 0xDC: /* SET 3 H */
        SET(H, 3);
        break;
      case 0xDD: /* SET 3 L */
        SET(L, 3);
        break;
      case 0xDE: /* SET 3 (HL) */
        T1 = memory.read_byte(HL);
        SET(T1, 3);
        memory.write_byte(HL, T1);
        break;
      case 0xDF: /* SET 3 A */
        SET(A, 3);
        break;
      case 0xE0: /* SET 4 B */
        SET(B, 4);
        break;
      case 0xE1: /* SET 4 C */
        SET(C, 4);
        break;
      case 0xE2: /* SET 4 D */
        SET(D, 4);
        break;
      case 0xE3: /* SET 4 E */
        SET(E, 4);
        break;
      case 0xE4: /* SET 4 H */
        SET(H, 4);
        break;
      case 0xE5: /* SET 4 L */
        SET(L, 4);
        break;
      case 0xE6: /* SET 4 (HL) */
        T1 = memory.read_byte(HL);
        SET(T1, 4);
        memory.write_byte(HL, T1);
        break;
      case 0xE7: /* SET 4 A */
        SET(A, 4);
        break;
      case 0xE8: /* SET 5 B */
        SET(B, 5);
        break;
      case 0xE9: /* SET 5 C */
        SET(C, 5);
        break;
      case 0xEA: /* SET 5 D */
        SET(D, 5);
        break;
      case 0xEB: /* SET 5 E */
        SET(E, 5);
        break;
      case 0xEC: /* SET 5 H */
        SET(H, 5);
        break;
      case 0xED: /* SET 5 L */
        SET(L, 5);
        break;
      case 0xEE: /* SET 5 (HL) */
        T1 = memory.read_byte(HL);
        SET(T1, 5);
        memory.write_byte(HL, T1);
        break;
      case 0xEF: /* SET 5 A */
        SET(A, 5);
        break;
      case 0xF0: /* SET 6 B */
        SET(B, 6);
        break;
      case 0xF1: /* SET 6 C */
        SET(C, 6);
        break;
      case 0xF2: /* SET 6 D */
        SET(D, 6);
        break;
      case 0xF3: /* SET 6 E */
        SET(E, 6);
        break;
      case 0xF4: /* SET 6 H */
        SET(H, 6);
        break;
      case 0xF5: /* SET 6 L */
        SET(L, 6);
        break;
      case 0xF6: /* SET 6 (HL) */
        T1 = memory.read_byte(HL);
        SET(T1, 6);
        memory.write_byte(HL, T1);
        break;
      case 0xF7: /* SET 6 A */
        SET(A, 6);
        break;
      case 0xF8: /* SET 7 B */
        SET(B, 7);
        break;
      case 0xF9: /* SET 7 C */
        SET(C, 7);
        break;
      case 0xFA: /* SET 7 D */
        SET(D, 7);
        break;
      case 0xFB: /* SET 7 E */
        SET(E, 7);
        break;
      case 0xFC: /* SET 7 H */
        SET(H, 7);
        break;
      case 0xFD: /* SET 7 L */
        SET(L, 7);
        break;
      case 0xFE: /* SET 7 (HL) */
        T1 = memory.read_byte(HL);
        SET(T1, 7);
        memory.write_byte(HL, T1);
        break;
      case 0xFF: /* SET 7 A */
        SET(A, 7);
        break;
      default:
        abort(cbop, PC, true);
        break;
      }
      break;
    default:
      abort(op, PC, false);
      break;
    }
    total_cycles = handle_interrupts();
    if(is_cbop) {
      total_cycles += cb_cycles_table[cbop];
    } else {
      total_cycles += cycles_table[op];
    }
    update_timers(total_cycles);
    cpu_time += total_cycles;
    executed_instructions++;

    return total_cycles;
  }

  void Cpu::update_timers(const int cycles) {
    do_divider_register(cycles);

    if(!is_clock_enabled()) {
      return;
    }
    timer_counter += cycles;

    if(timer_counter >= clock_speed) {
      set_clock_frequency();
      if(memory.read_byte(Memory::TIMA) == 0xFF) {
        memory.write_byte(Memory::TIMA, memory.read_byte(Memory::TMA));
        request_interrupt(TIMER_INTERRUPT);
      } else {
        memory.increment_tima();
      }
    }
  }

  byte Cpu::get_clock_frequency() const {
    return memory.read_byte(Memory::TAC) & 0x3;
  }

  void Cpu::reset_divider_counter() {
    if(divider_counter > MAX_DIVIDER_COUNTER) {
      divider_counter -= MAX_DIVIDER_COUNTER;
    } else {
      divider_counter = 0;
    }
  }

  void Cpu::reset_timer_counter() {
    int cfreq = clock_speed_available[get_clock_frequency()];
    if(timer_counter > cfreq) {
      timer_counter -= cfreq;
    } else {
      timer_counter = 0;
    }
  }

  void Cpu::do_divider_register(const int cycles) {
    divider_counter += cycles;
    if(divider_counter > MAX_DIVIDER_COUNTER) {
      reset_divider_counter();
      memory.increment_div();
    }
  }

  bool Cpu::is_clock_enabled() {
    return test_bit(memory.read_byte(Memory::TAC), 2);
  }

  void Cpu::set_clock_frequency() {
    reset_timer_counter(); // must be called before set new clock speed
    clock_speed = clock_speed_available[get_clock_frequency()];
  }

  /**
   * Emmit an error and die
   * @param op opcode
   * @param tpc program counter
   */
  inline void Cpu::abort(const int op, const Register16 tpc, const bool is_cbop) const {
    std::cerr << std::setiosflags(ios::uppercase)
              << "Invalid opcode " << (is_cbop ? "CBOP " : "")
              << "0x" << std::hex << op
              << " at address 0x" << std::hex << tpc << std::endl;
    exit(EXIT_FAILURE);
  }

  void Cpu::pause() {
    // TODO: How to pause
  }

  // Handle interrups and its priorities
  int Cpu::handle_interrupts() {
    int int_cycles = 0;
    if(!ime) {
      return int_cycles;
    }
    byte interrupts = memory.read_byte(Memory::IF);
    if(interrupts <= 0) {
      return int_cycles;
    }
    for(int i = 0; i < 5; i++) {
      if(test_bit(interrupts, i)) {
        if(test_bit(memory.read_byte(Memory::IE), i)) {
          perform_interrupt(i);
          int_cycles += 32; // 32 cycles to execute an interrupt
        }
      }
    }
    return int_cycles;
  }

  // perform an interrupt
  // must be called only by handle_interrupts method
  void Cpu::perform_interrupt(const int id) {
    if(id != 0) {
      //printf("Processing Interrupt %d\n", id);
    }

    ime = false;
    halt = false;
    byte req = memory.read_byte(Memory::IF);
    clear_bit(req, id);
    memory.write_byte(Memory::IF, req);

    PUSH(PC);
    PC = ISR[id];
  }

  // interface to request interrupts
  void Cpu::request_interrupt(const int id) {
    if(id != 0) {
      //printf("Requesting Interrupt %d\n", id);
    }
    byte interrupts = memory.read_byte(Memory::IF);
    set_bit(interrupts, id);
    memory.write_byte(Memory::IF, interrupts);
  }

  void Cpu::debug(const Register16 PC) const {
    /*printf("PC: 0x%x\n", pc);
    printf("SP: 0x%x\n", sp);
    //printf("OP: 0x%x\n", memory.read_byte(pc));
    printf("A: 0x%x\n", a);
    printf("B: 0x%x\n", B);
    printf("C: 0x%x\n", C);
    printf("D: 0x%x\n", D);
    printf("E: 0x%x\n", e);
    printf("F: 0x%x\n", F);
    printf("F: %s\n", as_binary(F, 8).c_str());
    printf("H: 0x%x\n", H);
    printf("L: 0x%x\n\n", L);*/
  }

  string Cpu::as_binary(const unsigned int number, const int len) const {
    string result;
    for(int i = len - 1; i >= 0; i--) {
      if(test_bit(number, i)) {
        result += "1";
      } else {
        result += "0";
      }
    }
    return result;
  }

  Cpu& Cpu::get_instance() {
    static Cpu inst;
    return inst;
  }
}
