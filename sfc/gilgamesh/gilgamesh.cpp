#ifdef DEBUGGER
#include <sfc/sfc.hpp>
namespace SuperFamicom {


Gilgamesh gilgamesh;

constexpr unsigned Instruction::type[];
constexpr int Instruction::size[];

Instruction::Instruction() {
  // Basic features:
  this->pc = cpu.regs.pc.d;
  this->op = cpu.dreadb(pc.d);

  // Processor status:
  this->a8 = cpu.regs.e || cpu.regs.p.m;
  this->x8 = cpu.regs.e || cpu.regs.p.x;

  // Get argument:
  decode();
}

void Instruction::decode() {
  this->arg = 0;

  // Possible argument sizes:
  uint8   op8 = cpu.dreadb(pc.d + 1);
  uint16 op16 = cpu.dreadw(pc.d + 1);
  uint24 op24 = cpu.dreadl(pc.d + 1);

  // Retrieve the argument:
  switch (size[type[op]]) {
    case -1:
      if (type[op] == CPU::OPTYPE_IMM_A)
        this->arg = a8 ? op8 : op16;
      else
        this->arg = x8 ? op8 : op16;
      break;

    case 1: this->arg = op8;  break;
    case 2: this->arg = op16; break;
    case 3: this->arg = op24; break;
  }
}

void Instruction::decode_ref() {
  // Default values:
  this->unstd_ret = false;
  this->ret = this->ref = this->ind_ref = -1;

  // Retrieve references:
  switch (type[op]) {
    // No reference:
    case CPU::OPTYPE_NONE:
    case CPU::OPTYPE_A:
    case CPU::OPTYPE_IMM_8:
    case CPU::OPTYPE_IMM_A:
    case CPU::OPTYPE_IMM_X:
      break;

    // Relative, don't log:
    case CPU::OPTYPE_DP:
    case CPU::OPTYPE_DPX:
    case CPU::OPTYPE_DPY:
    case CPU::OPTYPE_SR:
      break;

    // Direct is relative, log only indirect:
    case CPU::OPTYPE_IDP:
    case CPU::OPTYPE_IDPX:
    case CPU::OPTYPE_IDPY:
    case CPU::OPTYPE_ISRY:
    case CPU::OPTYPE_ILDP:
    case CPU::OPTYPE_ILDPY:
      this->ind_ref = cpu.decode(type[op], arg); break;

    // Indirect jumps:
    case CPU::OPTYPE_IADDRX:
    case CPU::OPTYPE_IADDR_PC:
      this->ref = cpu.decode(type[op], arg);
      this->ind_ref = (cpu.regs.pc.b << 16) | cpu.dreadw(ref); break;

    // Indirect long jump:
    case CPU::OPTYPE_ILADDR:
      this->ref = cpu.decode(type[op], arg);
      this->ind_ref = cpu.dreadl(ref); break;

    // TODO:
    case CPU::OPTYPE_MV:
      break;

    // Trust the original function:
    default:
      this->ref = cpu.decode(type[op], arg); break;
  }

  // Special instruction handling:
  auto& stack_tags = gilgamesh.stack_tags;
  switch (op) {
    // JSR, JSL:
    case 0x20: case 0x22: case 0xFC:
      // Save the location in the stack and the return address:
      stack_tags[cpu.regs.s.w - size[type[op]] + 1] = pc.d + size[type[op]] + 1; break;

    // RTS, RTL:
    case 0x60: case 0x6B:
      // Get the return address:
      CPU::reg24_t r;
      r.l = cpu.dreadb(cpu.regs.s.w + 1);
      r.h = cpu.dreadb(cpu.regs.s.w + 2);
      r.b = (op == 0x6B) ? cpu.dreadb(cpu.regs.s.w + 3) : pc.b;
      r.w++;
      this->ret = r.d;

      // Check if the address in the stack and the content match what we saved before:
      auto search = stack_tags.find(cpu.regs.s.w + 1);
      if (search == stack_tags.end()) {
        this->unstd_ret = true;
      } else {
        stack_tags.erase(search);
        if (ret != search->second)
          this->unstd_ret = true;
      }
      break;
  }
}

// Trace the current instruction:
void Gilgamesh::trace() {
  Instruction *i;

  // Have we encountered this instruction already?
  auto i_search = instructions.find(cpu.regs.pc.d);
  if (i_search != instructions.end()) {
    i = i_search->second;  // Yes, retrieve it.
  } else {
    // No, create a new one and record it:
    i = new Instruction();
    instructions[i->pc.d] = i;
    trace_vectors();  // Check if we have encounterd a interrupt handler.
  }

  i->decode_ref();  // Get the instruction's references.

  // Log a new direct reference, if any:
  if (i->ref != -1)
    references.insert(std::make_pair(i->pc.d, i->ref));

  // Log a new indirect reference, if any:
  if (i->ind_ref != -1)
    ind_references.insert(std::make_pair(i->pc.d, i->ind_ref));

  // Log a new non-standard return, if any:
  if (i->unstd_ret)
    unstd_returns.insert(std::make_pair(i->pc.d, i->ret));
}

// Check if we have encountered a interrupt handler and log it:
void Gilgamesh::trace_vectors() {
  if (cpu.regs.pc.d == cpu.dreadw(cpu.regs.vector))
    switch (cpu.regs.vector) {
      case 0xFFEA:  nmi_handler   = cpu.regs.pc.d; break;
      case 0xFFEE:  irq_handler   = cpu.regs.pc.d; break;
      case 0xFFFC:  reset_handler = cpu.regs.pc.d; break;
    }
}


}
#endif // DEBUGGER
