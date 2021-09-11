#include <raylib.h>

#include <cstdint>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>

constexpr uint16_t MEM_SIZE = 4096;
constexpr uint8_t GFX_ROWS = 32;
constexpr uint8_t GFX_COLS = 64;
constexpr uint16_t GFX_SIZE = GFX_ROWS * GFX_COLS;
constexpr uint8_t STACK_SIZE = 16;
constexpr uint8_t KEY_SIZE = 16;
constexpr uint8_t REG_NUM = 16;
constexpr uint16_t START_OF_MEM = 0x200;
constexpr uint16_t INDEX_START = 0;
constexpr uint16_t FONT_SET_LEN = 16 * 5;
constexpr uint8_t FONT_SET[FONT_SET_LEN] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
    0x20, 0x60, 0x20, 0x20, 0x70,  // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
    0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
    0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
    0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
    0xF0, 0x80, 0xF0, 0x80, 0x80   // F
};

constexpr int PIXEL_SIZE = 8;
constexpr int CANVAS_WIDTH = GFX_COLS * PIXEL_SIZE;
constexpr int CANVAS_HEIGHT = GFX_ROWS * PIXEL_SIZE;
constexpr Vector2 ZERO_VEC = {0, 0};
RenderTexture2D canvas;

struct OpCode {
  uint16_t opcode;
  uint16_t type;
  uint8_t x;
  uint8_t y;
  uint8_t n;
  uint8_t kk;
  uint16_t nnn;

  explicit OpCode(const uint8_t* memory) {
    opcode = memory[0] << 8 | memory[1];  // big endian
    type = opcode & 0xF000;
    x = (opcode >> 8) & 0x000F;  // the lower 4 bits of the high byte
    y = (opcode >> 4) & 0x000F;  // the upper 4 bits of the low byte
    n = opcode & 0x000F;         // the lowest 4 bits
    kk = opcode & 0x00FF;        // the lowest 8 bits
    nnn = opcode & 0x0FFF;       // the lowest 12 bits
  }
};

void LoadRom(char* memory, const std::string& rom_path) {
  std::ifstream fd(rom_path);
  fd.read(memory, MEM_SIZE);
}

uint8_t GetBit(uint8_t byte, uint8_t pos) {
  return (byte >> pos) & 0x1;
}

class Machine {
  uint16_t index_reg_;
  uint16_t pc_;  // program counter
  uint8_t sp_;   // stack_ pointer
  uint8_t regs_[REG_NUM]{};
  uint8_t memory_[MEM_SIZE]{};
  uint16_t stack_[STACK_SIZE]{};
  uint8_t display_[GFX_ROWS][GFX_COLS];

  const uint8_t DRAW_FLAG = 0xF;

  void InitMachine() {
    index_reg_ = INDEX_START;
    pc_ = START_OF_MEM;  // ???
    sp_ = 0;

    std::memset(regs_, 0, REG_NUM);
    std::memset(memory_, 0, MEM_SIZE);
    std::memcpy(memory_, FONT_SET, FONT_SET_LEN);
    std::memset(display_, 0, GFX_SIZE);
  }

  void UnknownOpCode(OpCode opcode) {
    std::cout << "Unknown opcode '" << std::hex << opcode.opcode << "'"
              << std::endl;
    //    exit(-1);
  }

  void LD_I(OpCode opcode) {
    index_reg_ = opcode.nnn;
  }

  void RND_Vx(OpCode opcode) {
    regs_[opcode.x] = static_cast<uint8_t>(rand() % 256) & opcode.kk;
  }

  void SE_Vx(OpCode opcode) {
    if (regs_[opcode.x] == opcode.kk) {
      pc_ = pc_ + 2;
    }
  }

  void SNE_Vx(OpCode opcode) {
    if (regs_[opcode.x] != opcode.kk) {
      pc_ = pc_ + 2;
    }
  }

  void CLS() {
    // TODO: clear screen
    std::memset(display_, 0, GFX_SIZE);
  }

  void RET() {
    pc_ = stack_[sp_];
    std::cerr << "SP overflow at RET"
              << std::endl;        // a warning if it's overflow?
    sp_ = (sp_ - 1) % STACK_SIZE;  // I made this behaviour to be specified
  }

  void LD_Vx(OpCode opcode) {
    regs_[opcode.x] = opcode.kk;
  }

  void DRW(OpCode op_code) {
    regs_[DRAW_FLAG] = 0;  // VF register flag

    uint8_t x = regs_[op_code.x] % GFX_COLS;
    uint8_t y = regs_[op_code.y] % GFX_ROWS;

    for (uint8_t j = 0; j < op_code.n; j++) {
      uint8_t pixels = memory_[index_reg_ + j];
      for (uint8_t i = 0; i < 8; i++) {
        auto pixel = GetBit(pixels, 7 - i);
        auto& display_pixel = display_[(y + j) % GFX_ROWS][(x + i) % GFX_COLS];
        // TODO: test it. It seems to be wrong.
        regs_[DRAW_FLAG] =
            regs_[DRAW_FLAG] || ((pixel ^ display_pixel) != display_pixel);

        display_pixel ^= pixel;
      }
    }
  }

  void Add(const OpCode& op_code) {
    regs_[op_code.x] += op_code.kk;
  }

  void JP(const OpCode& op_code) {
    pc_ = op_code.nnn - 2;
  }

  void SE_VxVy(const OpCode& op_code) {
    if (regs_[op_code.x] == regs_[op_code.y]) {
      pc_ = pc_ + 2;
    }
  }

  void SHR(const OpCode& op_code) {

  }

  void SHL(const OpCode& op_code) {

  }

 public:
  explicit Machine(const std::string& rom_path)
      : index_reg_(0), pc_(START_OF_MEM), sp_(0) {
    std::srand(std::time(nullptr));
    InitMachine();
    LoadRom(reinterpret_cast<char*>(memory_ + START_OF_MEM), rom_path);
  }

  void Restart() {
    // TODO:
  }

  void PrintMemory() {
    std::cout << std::setfill('=') << std::setw(80) << "";
    std::cout << "\nOpCodes\n";
    for (int i = START_OF_MEM; i < MEM_SIZE; i += 2) {
      OpCode opcode(&memory_[i]);
      std::cout << std::setfill('0') << std::setw(4) << std::hex
                << opcode.opcode << "\n";
      if (opcode.opcode == 0) {
        break;
      }
    }
    std::cout << std::setfill('=') << std::setw(80) << "";
    std::cout << std::endl;
  }

  void Step() {
    OpCode opcode(&memory_[pc_]);
    //    std::cout << std::dec << "PC=" << pc << " OPCODE=" <<
    //    std::setfill('0')
    //              << std::setw(4) << std::hex << opcode.opcode << std::endl;
    switch (opcode.type) {
      case 0x0000:
        switch (opcode.opcode) {
          case 0x00E0:
            CLS();
            break;
          case 0x00EE:
            RET();
            break;
          default:
            UnknownOpCode(opcode);
            break;
        }
        break;
      case 0x1000:
        JP(opcode);
        break;
      case 0x3000:
        SE_Vx(opcode);
        break;
      case 0x4000:
        SNE_Vx(opcode);
        break;
      case 0x5000:
        SE_VxVy(opcode);
        break;
      case 0x6000:
        LD_Vx(opcode);
        break;
      case 0x7000:
        Add(opcode);
        break;
      case 0x8000:
        switch (opcode.n) {
          case 0x6:
            SHR(opcode);
            break;
          case 0xE:
            SHL(opcode);
            break;
          default:
            UnknownOpCode(opcode);
            break;
        }
        break;
      case 0xA000:
        LD_I(opcode);
        break;
      case 0xC000:
        RND_Vx(opcode);
        break;
      case 0xD000:
        DRW(opcode);
        break;
      case 0xF000:
        switch (opcode.kk) {
          default:
            UnknownOpCode(opcode);
            break;
        }
        break;
      default:
        UnknownOpCode(opcode);
        break;
    }
    pc_ = (pc_ + 2) % MEM_SIZE;
  }

  bool GetPixel(int row, int col) const {
    if (row >= 0 && col >= 0 && col < GFX_COLS && row < GFX_ROWS) {
      return display_[row][col] == 1;
    }
    return false;
  }

  bool CheckRedrawAndSwapFlag() {
    bool flag = regs_[DRAW_FLAG];
    regs_[DRAW_FLAG] = 0;
    return flag;
  }
};

Rectangle GetCanvasTarget() {
  float sh = static_cast<float>(GetScreenHeight());
  float sw = static_cast<float>(GetScreenWidth());
  float scale = fminf(sw / CANVAS_WIDTH, sh / CANVAS_HEIGHT);
  Rectangle rec = {0, 0, CANVAS_WIDTH * scale, CANVAS_HEIGHT * scale};
  return rec;
}

void Draw(Machine& machine) {
  if (!machine.CheckRedrawAndSwapFlag()) {
    return;
  }

  BeginTextureMode(canvas);
  ClearBackground(BLACK);
  for (int row = 0; row < GFX_ROWS; row++) {
    for (int col = 0; col < GFX_COLS; col++) {
      if (machine.GetPixel(row, col)) {
        DrawRectangle(col * PIXEL_SIZE, row * PIXEL_SIZE, PIXEL_SIZE,
                      PIXEL_SIZE, WHITE);
      }
    }
  }
  EndTextureMode();
}

void Update(Machine& machine) {
  machine.Step();
}

void GameLoop(Machine& machine) {
  BeginDrawing();
  ClearBackground(GREEN);
  Draw(machine);
  Rectangle canvas_field = {0, 0, static_cast<float>(canvas.texture.width),
                            -static_cast<float>(canvas.texture.height)};
  Rectangle canvas_target = GetCanvasTarget();
  DrawTexturePro(canvas.texture, canvas_field, canvas_target, ZERO_VEC, 0.0f,
                 WHITE);
  EndDrawing();

  Update(machine);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "No ROM is specified\nCHIP8.exe path_to_rom" << std::endl;
    return -1;
  }
  Machine machine(argv[1]);
  machine.PrintMemory();

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(CANVAS_WIDTH, CANVAS_HEIGHT, "CHIP8");
  //  SetTargetFPS(60);
  canvas = LoadRenderTexture(CANVAS_WIDTH, CANVAS_HEIGHT);
  SetTextureFilter(canvas.texture, FILTER_POINT);

  while (!WindowShouldClose()) {
    GameLoop(machine);
  }

  CloseWindow();

  return 0;
}