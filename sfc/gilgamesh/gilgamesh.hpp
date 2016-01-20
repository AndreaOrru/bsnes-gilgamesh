#ifdef DEBUGGER


// Custom hash function for references (pair of integers):
struct hash_ref {
  size_t operator() (const std::pair<unsigned,unsigned>& p) const {
    return std::hash<unsigned>()(p.first) ^ std::hash<unsigned>()(p.second);
  }
};

// Structure representing an instruction:
struct Instruction {
  Instruction();
  void decode();
  void decode_ref();

  CPU::reg24_t pc;  // Address.
  uint8 op;         // Opcode.

  bool a8;          // 8-bit accumulator?
  bool x8;          // 8-bit index registers?

  unsigned arg;     // Argument.

  int  ret;         // Return address;
  bool unstd_ret;   // Unstandard return?

  int ref;          // Reference.
  int ind_ref;      // Indirect reference.

  // Type of every instruction:
  static constexpr unsigned type[] = {
    CPU::OPTYPE_IMM_8   , CPU::OPTYPE_IDPX , CPU::OPTYPE_IMM_8, CPU::OPTYPE_SR   ,  // $00
    CPU::OPTYPE_DP      , CPU::OPTYPE_DP   , CPU::OPTYPE_DP   , CPU::OPTYPE_ILDP ,  // $04
    CPU::OPTYPE_NONE    , CPU::OPTYPE_IMM_A, CPU::OPTYPE_A    , CPU::OPTYPE_NONE ,  // $08
    CPU::OPTYPE_ADDR    , CPU::OPTYPE_ADDR , CPU::OPTYPE_ADDR , CPU::OPTYPE_LONG ,  // $0C
    CPU::OPTYPE_RELB    , CPU::OPTYPE_IDPY , CPU::OPTYPE_IDP  , CPU::OPTYPE_ISRY ,  // $10
    CPU::OPTYPE_DP      , CPU::OPTYPE_DPX  , CPU::OPTYPE_DPX  , CPU::OPTYPE_ILDPY,  // $14
    CPU::OPTYPE_NONE    , CPU::OPTYPE_ADDRY, CPU::OPTYPE_NONE , CPU::OPTYPE_NONE ,  // $18
    CPU::OPTYPE_ADDR    , CPU::OPTYPE_ADDRX, CPU::OPTYPE_ADDRX, CPU::OPTYPE_LONGX,  // $1C
    CPU::OPTYPE_ADDR_PC , CPU::OPTYPE_IDPX , CPU::OPTYPE_LONG , CPU::OPTYPE_SR   ,  // $20
    CPU::OPTYPE_DP      , CPU::OPTYPE_DP   , CPU::OPTYPE_DP   , CPU::OPTYPE_ILDP ,  // $24
    CPU::OPTYPE_NONE    , CPU::OPTYPE_IMM_A, CPU::OPTYPE_A    , CPU::OPTYPE_NONE ,  // $28
    CPU::OPTYPE_ADDR    , CPU::OPTYPE_ADDR , CPU::OPTYPE_ADDR , CPU::OPTYPE_LONG ,  // $2C
    CPU::OPTYPE_RELB    , CPU::OPTYPE_IDPY , CPU::OPTYPE_IDP  , CPU::OPTYPE_ISRY ,  // $30
    CPU::OPTYPE_DPX     , CPU::OPTYPE_DPX  , CPU::OPTYPE_DPX  , CPU::OPTYPE_ILDPY,  // $34
    CPU::OPTYPE_NONE    , CPU::OPTYPE_ADDRY, CPU::OPTYPE_NONE , CPU::OPTYPE_NONE ,  // $38
    CPU::OPTYPE_ADDRX   , CPU::OPTYPE_ADDRX, CPU::OPTYPE_ADDRX, CPU::OPTYPE_LONGX,  // $3C
    CPU::OPTYPE_NONE    , CPU::OPTYPE_IDPX , CPU::OPTYPE_NONE , CPU::OPTYPE_SR   ,  // $40
    CPU::OPTYPE_MV      , CPU::OPTYPE_DP   , CPU::OPTYPE_DP   , CPU::OPTYPE_ILDP ,  // $44
    CPU::OPTYPE_NONE    , CPU::OPTYPE_IMM_A, CPU::OPTYPE_A    , CPU::OPTYPE_NONE ,  // $48
    CPU::OPTYPE_ADDR_PC , CPU::OPTYPE_ADDR , CPU::OPTYPE_ADDR , CPU::OPTYPE_LONG ,  // $4C
    CPU::OPTYPE_RELB    , CPU::OPTYPE_IDPY , CPU::OPTYPE_IDP  , CPU::OPTYPE_ISRY ,  // $50
    CPU::OPTYPE_MV      , CPU::OPTYPE_DPX  , CPU::OPTYPE_DPX  , CPU::OPTYPE_ILDPY,  // $54
    CPU::OPTYPE_NONE    , CPU::OPTYPE_ADDRY, CPU::OPTYPE_NONE , CPU::OPTYPE_NONE ,  // $58
    CPU::OPTYPE_LONG    , CPU::OPTYPE_ADDRX, CPU::OPTYPE_ADDRX, CPU::OPTYPE_LONGX,  // $5C
    CPU::OPTYPE_NONE    , CPU::OPTYPE_IDPX , CPU::OPTYPE_ADDR , CPU::OPTYPE_SR   ,  // $60
    CPU::OPTYPE_DP      , CPU::OPTYPE_DP   , CPU::OPTYPE_DP   , CPU::OPTYPE_ILDP ,  // $64
    CPU::OPTYPE_NONE    , CPU::OPTYPE_IMM_A, CPU::OPTYPE_A    , CPU::OPTYPE_NONE ,  // $68
    CPU::OPTYPE_IADDR_PC, CPU::OPTYPE_ADDR , CPU::OPTYPE_ADDR , CPU::OPTYPE_LONG ,  // $6C
    CPU::OPTYPE_RELB    , CPU::OPTYPE_IDPY , CPU::OPTYPE_IDP  , CPU::OPTYPE_ISRY ,  // $70
    CPU::OPTYPE_DPX     , CPU::OPTYPE_DPX  , CPU::OPTYPE_DPX  , CPU::OPTYPE_ILDPY,  // $74
    CPU::OPTYPE_NONE    , CPU::OPTYPE_ADDRY, CPU::OPTYPE_NONE , CPU::OPTYPE_NONE ,  // $78
    CPU::OPTYPE_IADDRX  , CPU::OPTYPE_ADDRX, CPU::OPTYPE_ADDRX, CPU::OPTYPE_LONGX,  // $7C
    CPU::OPTYPE_RELB    , CPU::OPTYPE_IDPX , CPU::OPTYPE_RELW , CPU::OPTYPE_SR   ,  // $80
    CPU::OPTYPE_DP      , CPU::OPTYPE_DP   , CPU::OPTYPE_DP   , CPU::OPTYPE_ILDP ,  // $84
    CPU::OPTYPE_NONE    , CPU::OPTYPE_IMM_A, CPU::OPTYPE_NONE , CPU::OPTYPE_NONE ,  // $88
    CPU::OPTYPE_ADDR    , CPU::OPTYPE_ADDR , CPU::OPTYPE_ADDR , CPU::OPTYPE_LONG ,  // $8C
    CPU::OPTYPE_RELB    , CPU::OPTYPE_IDPY , CPU::OPTYPE_IDP  , CPU::OPTYPE_ISRY ,  // $90
    CPU::OPTYPE_DPX     , CPU::OPTYPE_DPX  , CPU::OPTYPE_DPY  , CPU::OPTYPE_ILDPY,  // $94
    CPU::OPTYPE_NONE    , CPU::OPTYPE_ADDRY, CPU::OPTYPE_NONE , CPU::OPTYPE_NONE ,  // $98
    CPU::OPTYPE_ADDR    , CPU::OPTYPE_ADDRX, CPU::OPTYPE_ADDRX, CPU::OPTYPE_LONGX,  // $9C
    CPU::OPTYPE_IMM_X   , CPU::OPTYPE_IDPX , CPU::OPTYPE_IMM_X, CPU::OPTYPE_SR   ,  // $A0
    CPU::OPTYPE_DP      , CPU::OPTYPE_DP   , CPU::OPTYPE_DP   , CPU::OPTYPE_ILDP ,  // $A4
    CPU::OPTYPE_NONE    , CPU::OPTYPE_IMM_A, CPU::OPTYPE_NONE , CPU::OPTYPE_NONE ,  // $A8
    CPU::OPTYPE_ADDR    , CPU::OPTYPE_ADDR , CPU::OPTYPE_ADDR , CPU::OPTYPE_LONG ,  // $AC
    CPU::OPTYPE_RELB    , CPU::OPTYPE_IDPY , CPU::OPTYPE_IDP  , CPU::OPTYPE_ISRY ,  // $B0
    CPU::OPTYPE_DPX     , CPU::OPTYPE_DPX  , CPU::OPTYPE_DPY  , CPU::OPTYPE_ILDPY,  // $B4
    CPU::OPTYPE_NONE    , CPU::OPTYPE_ADDRY, CPU::OPTYPE_NONE , CPU::OPTYPE_NONE ,  // $B8
    CPU::OPTYPE_ADDRX   , CPU::OPTYPE_ADDRX, CPU::OPTYPE_ADDRY, CPU::OPTYPE_LONGX,  // $BC
    CPU::OPTYPE_IMM_X   , CPU::OPTYPE_IDPX , CPU::OPTYPE_IMM_8, CPU::OPTYPE_SR   ,  // $C0
    CPU::OPTYPE_DP      , CPU::OPTYPE_DP   , CPU::OPTYPE_DP   , CPU::OPTYPE_ILDP ,  // $C4
    CPU::OPTYPE_NONE    , CPU::OPTYPE_IMM_A, CPU::OPTYPE_NONE , CPU::OPTYPE_NONE ,  // $C8
    CPU::OPTYPE_ADDR    , CPU::OPTYPE_ADDR , CPU::OPTYPE_ADDR , CPU::OPTYPE_LONG ,  // $CC
    CPU::OPTYPE_RELB    , CPU::OPTYPE_IDPY , CPU::OPTYPE_IDP  , CPU::OPTYPE_ISRY ,  // $D0
    CPU::OPTYPE_IDP     , CPU::OPTYPE_DPX  , CPU::OPTYPE_DPX  , CPU::OPTYPE_ILDPY,  // $D4
    CPU::OPTYPE_NONE    , CPU::OPTYPE_ADDRY, CPU::OPTYPE_NONE , CPU::OPTYPE_NONE ,  // $D8
    CPU::OPTYPE_ILADDR  , CPU::OPTYPE_ADDRX, CPU::OPTYPE_ADDRX, CPU::OPTYPE_LONGX,  // $DC
    CPU::OPTYPE_IMM_X   , CPU::OPTYPE_IDPX , CPU::OPTYPE_IMM_8, CPU::OPTYPE_SR   ,  // $E0
    CPU::OPTYPE_DP      , CPU::OPTYPE_DP   , CPU::OPTYPE_DP   , CPU::OPTYPE_ILDP ,  // $E4
    CPU::OPTYPE_NONE    , CPU::OPTYPE_IMM_A, CPU::OPTYPE_NONE , CPU::OPTYPE_NONE ,  // $E8
    CPU::OPTYPE_ADDR    , CPU::OPTYPE_ADDR , CPU::OPTYPE_ADDR , CPU::OPTYPE_LONG ,  // $EC
    CPU::OPTYPE_RELB    , CPU::OPTYPE_IDPY , CPU::OPTYPE_IDP  , CPU::OPTYPE_ISRY ,  // $F0
    CPU::OPTYPE_ADDR    , CPU::OPTYPE_DPX  , CPU::OPTYPE_DPX  , CPU::OPTYPE_ILDPY,  // $F4
    CPU::OPTYPE_NONE    , CPU::OPTYPE_ADDRY, CPU::OPTYPE_NONE , CPU::OPTYPE_NONE ,  // $F8
    CPU::OPTYPE_IADDRX  , CPU::OPTYPE_ADDRX, CPU::OPTYPE_ADDRX, CPU::OPTYPE_LONGX,  // $FC
  };

  // Size of the argument, per type of opcode:
  static constexpr int size[] = {
     0,  // OPTYPE_NONE
     0,  // OPTYPE_A
     1,  // OPTYPE_IMM_8
    -1,  // OPTYPE_IMM_A
    -1,  // OPTYPE_IMM_X
     1,  // OPTYPE_DP
     1,  // OPTYPE_DPX
     1,  // OPTYPE_DPY
     1,  // OPTYPE_IDP
     1,  // OPTYPE_IDPX
     1,  // OPTYPE_IDPY
     1,  // OPTYPE_ILDP
     1,  // OPTYPE_ILDPY
     2,  // OPTYPE_ADDR
     2,  // OPTYPE_ADDRX
     2,  // OPTYPE_ADDRY
     2,  // OPTYPE_IADDRX
     2,  // OPTYPE_ILADDR
     3,  // OPTYPE_LONG
     3,  // OPTYPE_LONGX
     1,  // OPTYPE_SR
     1,  // OPTYPE_ISRY
     2,  // OPTYPE_ADDR_PC
     2,  // OPTYPE_IADDR_PC
     1,  // OPTYPE_RELB
     2,  // OPTYPE_RELW
     2,  // OPTYPE_MV
  };
};

// Tracer class:
struct Gilgamesh {
  void trace();
  void trace_vectors();

  std::unordered_map<unsigned, Instruction*> instructions;

  std::unordered_set<std::pair<unsigned, unsigned>, hash_ref> references;
  std::unordered_set<std::pair<unsigned, unsigned>, hash_ref> ind_references;
  std::unordered_set<std::pair<unsigned, unsigned>, hash_ref> unstd_returns;

  std::unordered_map<unsigned, unsigned> stack_tags;

  unsigned nmi_handler;
  unsigned irq_handler;
  unsigned reset_handler;
};

extern Gilgamesh gilgamesh;


#endif // DEBUGGER
