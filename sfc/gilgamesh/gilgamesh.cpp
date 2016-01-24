#ifdef DEBUGGER
#include <sfc/sfc.hpp>
namespace SuperFamicom {


Gilgamesh gilgamesh;

constexpr const char* Instruction::mnems[];
constexpr unsigned Instruction::types[];
constexpr int Instruction::sizes[];

Instruction::Instruction() {
  // Basic features:
  this->pc = cpu.regs.pc.d;
  this->op = cpu.dreadb(pc.d);

  // Processor status:
  this->a8 = cpu.regs.e || cpu.regs.p.m;
  this->x8 = cpu.regs.e || cpu.regs.p.x;

  decode();  // Get argument.
}

unsigned Instruction::size() {
  int size = sizes[type()];
  if (size != -1) return size;

  if (type() == CPU::OPTYPE_IMM_A)
    return a8 ? 1 : 2;
  else
    return x8 ? 1 : 2;
}

void Instruction::decode() {
  this->arg = 0;

  // Possible argument sizes:
  uint8   op8 = cpu.dreadb(pc.d + 1);
  uint16 op16 = cpu.dreadw(pc.d + 1);
  uint24 op24 = cpu.dreadl(pc.d + 1);

  // Retrieve the argument:
  switch (size()) {
    case 1: this->arg = op8;  break;
    case 2: this->arg = op16; break;
    case 3: this->arg = op24; break;
  }
}

void Instruction::decodeRef() {
  // Default values:
  this->unstdRet = false;
  this->ret = this->ref = this->indRef = -1;

  // Retrieve references:
  switch (type()) {
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
      this->indRef = cpu.decode(type(), arg); break;

    // Indirect jumps:
    case CPU::OPTYPE_IADDRX:
    case CPU::OPTYPE_IADDR_PC:
      this->ref = cpu.decode(type(), arg);
      this->indRef = (cpu.regs.pc.b << 16) | cpu.dreadw(ref); break;

    // Indirect long jump:
    case CPU::OPTYPE_ILADDR:
      this->ref = cpu.decode(type(), arg);
      this->indRef = cpu.dreadl(ref); break;

    // TODO:
    case CPU::OPTYPE_MV:
      break;

    // Trust the original function:
    default:
      this->ref = cpu.decode(type(), arg); break;
  }

  // Special instruction handling:
  auto& stackTags = gilgamesh.stackTags;
  switch (op) {
    // JSR, JSL:
    case 0x20: case 0x22: case 0xFC:
      // Save the location in the stack and the return address:
      stackTags[cpu.regs.s.w - size() + 1] = pc.d + size() + 1; break;

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
      auto search = stackTags.find(cpu.regs.s.w + 1);
      if (search == stackTags.end()) {
        this->unstdRet = true;
      } else {
        stackTags.erase(search);
        if (ret != search->second)
          this->unstdRet = true;
      }
      break;
  }
}

// Trace the current instruction:
void Gilgamesh::trace() {
  Instruction *i;

  // Have we encountered this instruction already?
  auto iSearch = instructions.find(cpu.regs.pc.d);
  if (iSearch != instructions.end()) {
    i = iSearch->second;  // Yes, retrieve it.
  } else {
    // No, create a new one and record it:
    i = new Instruction();
    instructions[i->pc.d] = i;
    traceVectors();  // Check if we have encounterd a interrupt handler.
  }

  i->decodeRef();  // Get the instruction's references.

  // Log a new direct reference, if any:
  if (i->ref != -1)
    references.insert(Reference(i->pc.d, i->ref, REF_DIRECT));

  // Log a new indirect reference, if any:
  if (i->indRef != -1)
    references.insert(Reference(i->pc.d, i->indRef, REF_INDIRECT));

  // Log a new non-standard return, if any:
  if (i->unstdRet)
    references.insert(Reference(i->pc.d, i->ret, REF_UNSTD));
}

// Check if we have encountered a interrupt handler and log it:
void Gilgamesh::traceVectors() {
  if (cpu.regs.pc.d == cpu.dreadw(cpu.regs.vector))
    switch (cpu.regs.vector) {
      case VECTOR_RESET:
      case VECTOR_NMI:
      case VECTOR_IRQ:
        vectors[cpu.regs.vector] = cpu.regs.pc.d;
    }
}

void Gilgamesh::createDatabase(sqlite3* db) {
  const char* sql =
    "CREATE TABLE Instruction(pc       INTEGER NOT NULL,"
                             "opcode   INTEGER NOT NULL,"
                             "mnemonic TEXT,"
                             "argument INTEGER,"
                             "size     INTEGER,"
                             "type     INTEGER,"
                             "PRIMARY KEY (pc));"

    "CREATE TABLE Reference(origin    INTEGER NOT NULL,"
                           "reference INTEGER NOT NULL,"
                           "type      INTEGER,"
                           "PRIMARY KEY (origin, reference),"
                           "FOREIGN KEY (origin) REFERENCES Instruction(pc));"

    "CREATE TABLE Subroutine(pc INTEGER NOT NULL,"
                            "PRIMARY KEY (pc));"

    "CREATE TABLE Vector(vector INTEGER NOT NULL,"
                        "pc     INTEGER NOT NULL,"
                        "PRIMARY KEY (vector));";

  sqlite3_exec(db, sql, NULL, NULL, NULL);
}

void Gilgamesh::writeDatabase(sqlite3* db) {
  static char sql[4096];
  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);

  for (auto keyValue: instructions) {
    auto& i = *(keyValue.second);
    sprintf(sql, "INSERT INTO Instruction VALUES(%u, %u, '%s', %u, %u, %u)", i.pc.d, i.op, i.mnem(), i.arg, i.size(), i.type());
    sqlite3_exec(db, sql, NULL, NULL, NULL);
  }
  for (auto r: references) {
    if (instructions[r.origin]->isCall()) {
      sprintf(sql, "INSERT INTO Subroutine VALUES(%u)", r.reference);
      sqlite3_exec(db, sql, NULL, NULL, NULL);
    }
    sprintf(sql, "INSERT INTO Reference VALUES(%u, %u, %u)", r.origin, r.reference, r.type);
    sqlite3_exec(db, sql, NULL, NULL, NULL);
  }
  for (auto v: vectors) {
    sprintf(sql, "INSERT INTO Vector VALUES(%u, %u)", v.first, v.second);
    sqlite3_exec(db, sql, NULL, NULL, NULL);
  }

  sqlite3_exec(db, "COMMIT TRANSACTION", NULL, NULL, NULL);
}


}
#endif // DEBUGGER
