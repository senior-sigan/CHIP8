#include <cstdint>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

const char* PATH =
    "/Users/ilya/Documents/gamedev/chip8/chip8-roms/demos/Maze [David Winter, "
    "199x].ch8";

#define MEM_SIZE 4096
#define GFX_ROWS 32
#define GFX_COLS 64
#define GFX_SIZE (GFX_ROWS * GFX_COLS)
#define STACK_SIZE 16
#define KEY_SIZE 16
#define REG_NUM 16

struct OpCode {
  uint16_t opcode;
  uint16_t type;
  uint8_t x;
  uint8_t y;
  uint8_t n;
  uint8_t kk;
  uint16_t nnn;

  OpCode(const uint8_t* memory) {
    opcode = memory[0] << 8 | memory[1];  // big endian
    type = opcode & 0xF000;
    x = (opcode >> 8) & 0x000F;  // the lower 4 bits of the high byte
    y = (opcode >> 4) & 0x000F;  // the upper 4 bits of the low byte
    n = opcode & 0x000F;         // the lowest 4 bits
    kk = opcode & 0x00FF;        // the lowest 8 bits
    nnn = opcode & 0x0FFF;       // the lowest 12 bits
  }
};

void load_rom(char* memory, std::string rom_path) {
  std::ifstream fd(rom_path);
  fd.read(&memory[0x200], MEM_SIZE);
}

class Machine {
  uint16_t index_reg;
  int8_t data_regs[REG_NUM];
  uint16_t pc;
  uint8_t memory[MEM_SIZE];

  void init_machine() {
    index_reg = 0;
    for (int i = 0; i < REG_NUM; i++) {
      data_regs[i] = 0;
    }
    pc = 0x200;  // ???
    std::memset(memory, 0, MEM_SIZE);
  }

  void UnknowOpCode(OpCode opcode) {
    std::cout << "Unknown opcode type='" << std::hex << opcode.type << "'"
              << std::endl;
  }

  void LD_I(OpCode opcode) { index_reg = opcode.nnn; }

  void RND_Vx(OpCode opcode) {
    data_regs[opcode.x] = (rand() % 256) & opcode.kk;
  }

  void SE_Vx(OpCode opcode) {
    if (data_regs[opcode.x] == opcode.kk) {
      pc = pc + 2;
    }
  }

 public:
  Machine(std::string rom_path) {
    std::srand(std::time(nullptr));
    init_machine();
    load_rom((char*)memory, rom_path);
  }

  void step() {
    OpCode opcode(&memory[pc]);
    std::cout << "PC=" << pc << " OPCODE=" << std::hex << opcode.opcode
              << std::endl;
    switch (opcode.type) {
      case 0xA000:
        LD_I(opcode);
        break;
      case 0xC000:
        RND_Vx(opcode);
        break;
      case 0x3000:
        SE_Vx(opcode);
        break;
      default:
        UnknowOpCode(opcode);
        exit(-1);
    }
    pc = (pc + 2) % MEM_SIZE;
  }
};

int main() {
  Machine machine(PATH);
  while (true) {
    machine.step();
  }

  return 0;
}