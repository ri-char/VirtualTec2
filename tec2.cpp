#include <cstdio>
#include <cctype>
#include <ctime>
#include "tec2.h"
#include "bit.h"

Tec2::Tec2() : A_(0), B_(0), INT_(false), FS_(0), ONLINE_(false), bp(-1), bping(false) {
    FLAGS_.input(0xf);
    FLAGS_.update();
    IB_ &= 0xffff;
    AB_ &= 0xffff;
    D_ &= 0xffff;
    for (int &i: MEM) i &= 0xffff;
}

void Tec2::offline(int D, int SW2, int SW1, int CK) {
    int B1, B0;
    B1 = 1 << 15 | getBits(SW2, 11, 9) << 12 | getBits(SW2, 8, 6) << 8 |
         1 << 7 | getBits(SW2, 5, 3) << 4 | getBits(SW1, 11, 8);
    B0 = getBits(SW1, 7, 0) << 8;
    setStatus(8, D, 0, 1, B1, B0);
    run(CK);
}

bool Tec2::cont(int address) {
    auto start = clock();
    static int OrgSP;

    if (!bping) OrgSP = ALU.Register[4].Y;
    // CALL address
    if (address > 0) {
        ALU.Register[4].Y = (OrgSP - 1) & 0xffff;
        MEM[ALU.Register[4].Y] = ALU.Register[5].Y;
        ALU.Register[5].Y = address & 0xffff;
        setStatus(5, 0, 0x29, 0x0300, 0x90f0, 0);
    }

    do {
        if (clock() - start > 2000) {
            ALU.Register[4].Y = OrgSP;
            ALU.Register[5].Y = MEM[(OrgSP - 1) & 0xffff];
            bping = false;
            return false;
        }
        run(1);
        if (ALU.Register[5].Y == bp) {
            bping = true;
            while (MCTL.Y != 0xa4) run(1);
            break;
        }
        if (ALU.Register[4].Y == OrgSP) {
            bping = false;
            break;
        }
    } while (true); // (MCTL.Y != 0x84);	// RET

    run(1);
    return true;
}

void Tec2::step() {
    setStatus(5, 0, 0x29, 0x0300, 0x90F0, 0);
    run(1);
    do {
        run(1);
    } while (MCTL.Y != 0xa4);
}

void Tec2::step2() {
    setStatus(5, 0, 0x29, 0x0300, 0x90F0, 0);
    run(1);
    do {
        run(1);
        if (MCTL.Y == 0x74)        // CALL SR
        {
            cont();
            setStatus(5, 0, 0x29, 0x0300, 0x90F0, 0);
            run(1);
        }
    } while (MCTL.Y != 0xa4);
}

// B3_, B2_, B1_, B0_ 分别是微指令中的 63-48, 47-32, 31-16, 15-0 位
void Tec2::setStatus(int FS, int D, int B3, int B2, int B1, int B0) {
    int MI876, MI543, MI210, MI0, REQ, WE;

    FS_ = FS;
    D_ = D;
    B3_ = B3;
    B2_ = B2;
    B1_ = B1;
    B0_ = B0;
    PLR[0].Y = B0;
    PLR[1].Y = B1;
    PLR[2].Y = B2;
    PLR[3].Y = B3;
    ONLINE_ = !getBit(FS, 3);
    MRA_ = getBits(B3, 7, 0) << 2 | getBits(B2, 15, 14);
    CI_ = getBits(B2, 11, 8);
    SCC_ = getBits(B2, 7, 5);
    SC_ = getBit(B2, 4);
    SST_ = getBits(B2, 2, 0);
    MI0 = getBit(B1, 15);
    REQ = getBit(B1, 11);
    WE = getBit(B1, 7);
    MRW_ = MI0 << 2 | REQ << 1 | WE;
    MI876 = getBits(B1, 14, 12);
    MI543 = getBits(B1, 10, 8);
    MI210 = getBits(B1, 6, 4);
    MI_ = MI876 << 6 | MI543 << 3 | MI210;
    A_ = getBits(B1, 3, 0);
    B_ = getBits(B0, 15, 12);
    SCI_ = getBits(B0, 11, 10);
    SSH_ = getBits(B0, 9, 8);
    SA_ = getBit(B0, 7);
    SB_ = getBit(B0, 3);
    DC1_ = getBits(B0, 6, 4);
    DC2_ = getBits(B0, 2, 0);
    // SW1_ = A_<<8 | B_<<4 | SCI_<<2 | SSH_;
    // SW2_ = MI_<<3 | SST_;
}

void Tec2::run(int CK) {
    runCombine();
    if (CK) {
        runSchedule();
        // runCombine();
    }
}

bool Tec2::ldmcReset() {
    if (!loadMcr() || !loadMapRom()) return false;
    PLR[2].input(0);
    PLR[2].update();
    CI_ = 0;
    return true;
}

// 更新组合电路状态
void Tec2::runCombine() {
    bool changed;
    int AA, BB;
    if (ONLINE_) setStatus(FS_, D_, PLR[3].Y, PLR[2].Y, PLR[1].Y, PLR[0].Y);
    AA = (SA_ ? getBits(IR_.Y, 3, 0) : A_);
    BB = (SB_ ? getBits(IR_.Y, 7, 4) : B_);
    do {
        changed =
                procSci() |
                procSsh() |
                        ALU.compute(IB_, MI_, AA, BB, CIN_, RAM0_, RAM15, Q0_, Q15_) |
                procMrw() |
                procDc1();
    } while (changed);
    procSst();
    procDc2();

    if (ONLINE_) {
        generateCc();
        MCTL.combine(selectD(CI_), CI_, CC_, 1);
        setPlr();
    }
}

// 触发时序电路
void Tec2::runSchedule() {
    int i;
    ALU.update();
    FLAGS_.update();
    AR_.update();
    AB_ = AR_.Y;
    IR_.update();
    if (ONLINE_) {
        MCTL.schedule();
        for (i = 0; i < 4; i++) {
            PLR[i].update();
            LDR_[i].update();
        }
    }
}

// 加载微码到控存
bool Tec2::loadMcr() {
    FILE *f = fopen("MCR.ROM", "r");
    char line[1024];
    int i, j, p = 0, data;
    if (f == nullptr) return false;
    while (fgets(line, 1024, f) != nullptr) {
        j = 0;
        for (i = 0; i < 4; i++) {
            data = 0;
            while (isdigit(line[j]) || (line[j] >= 'A' && line[j] <= 'F')) {
                line[j] = char(toupper(line[j]));
                data <<= 4;
                data += (isdigit(line[j]) ? line[j] - '0' : line[j] - 'A' + 10);
                j++;
            }
            j++;
            MCM_[p++] = data;
        }
    }
    fclose(f);
    return true;
}

bool Tec2::loadMapRom() {
    FILE *f = fopen("MAPROM.ROM", "r");
    int data, p = 0, i;
    char temp[10];
    if (f == nullptr) return false;
    while (fgets(temp, 10, f) != nullptr) {
        data = i = 0;
        while (isdigit(temp[i]) || (temp[i] >= 'A' && temp[i] <= 'F')) {
            data <<= 4;
            data += (isdigit(temp[i]) ? temp[i] - '0' : temp[i] - 'A' + 10);
            i++;
        }
        MapRom_[p++] = data;
    }
    fclose(f);
    return true;
}

int Tec2::readFile(char *filename) {
    FILE *f = fopen(filename, "rb");
    int data, address, n, bytes;
    unsigned char high, low;
    if (f == nullptr) return -1;
    bytes = 5;
    fseek(f, 1, 0);
    fscanf(f, "%c%c", &low, &high);
    address = high << 8 | low;
    fscanf(f, "%c%c", &low, &high);
    n = high << 8 | low;
    while (n--) {
        fscanf(f, "%c%c", &high, &low);
        bytes += 2;
        data = high << 8 | low;
        MEM[address++] = data;
    }
    fclose(f);
    return bytes;
}

int Tec2::writeFile(char *filename, int address, int n) {
    FILE *f = fopen(filename, "wb");
    int i, bytes;
    unsigned char high, low;
    if (f == nullptr) return -1;
    bytes = 5;
    fprintf(f, "%c", 1);
    high = getBits(address, 15, 8);
    low = getBits(address, 7, 0);
    fprintf(f, "%c%c", low, high);
    high = getBits(n, 15, 8);
    low = getBits(n, 7, 0);
    fprintf(f, "%c%c", low, high);
    for (i = 0; i < n && address < 65536; i++) {
        high = getBits(MEM[address], 15, 8);
        low = getBits(MEM[address], 7, 0);
        fprintf(f, "%c%c", high, low);
        address++;
        address &= 0xffff;
        bytes += 2;
    }
    fclose(f);
    return bytes;
}

void Tec2::saveMcm() {
    FILE *f = fopen("MCR.ROM", "wb");
    int i;
    for (i = 0; i < 4096; i++) {
        if (i > 0 && i % 4 == 0) fprintf(f, "\n");
        fprintf(f, "%04X ", MCM_[i]);
    }
    fclose(f);
}

void Tec2::setLED(int S) {
    int i;
    switch (S) {
        case 0:
            LED = 0;
            for (i = 0; i < 4; i++) setBit(LED, i, getBit(FLAGS_.Y, 7 - i));
            LED |= (getBits(FLAGS_.Y, 3, 0) << 4 | getBits(B0_, 7, 0) << 8);
            break;
        case 1:
            LED = getBits(B1_, 7, 0) << 8 | getBits(B0_, 15, 8);
            break;
        case 2:
            LED = getBits(B2_, 7, 0) << 8 | getBits(B1_, 15, 8);
            break;
        case 3:
            LED = getBits(B3_, 7, 0) << 8 | getBits(B2_, 15, 8);
            break;
        case 4:
            LED = MCTL.Y;
            break;
        case 5:
            LED = AB_;
            break;
        case 6:
            LED = ALU.Y;
            break;
        case 7:
            LED = IB_;
        default:
            break;
    }
}

// AM2901最低位进位
bool Tec2::procSci() {
    bool orgCin = CIN_;
    switch (SCI_) {
        case 0:
            CIN_ = false;
            break;
        case 1:
            CIN_ = true;
            break;
        case 2:
            CIN_ = getBit(FLAGS_.Y, 7);
            break;
        case 3:
            CIN_ = false; // 进位为方波，待完善
    }
    return orgCin != CIN_;
}

// 移位控制
bool Tec2::procSsh() {
    bool tRAM0 = RAM0_, tRAM15 = RAM15, tQ0 = Q0_, tQ15 = Q15_;
    bool shl = (getBits(MI_, 8, 6) > 5 ? 1 : 0);
    switch (SSH_) {
        case 0:
            (shl ? RAM0_ : RAM15) = false;
            break;
        case 1:
            (shl ? RAM0_ : RAM15) = getBit(FLAGS_.Y, 7);
            break;
        case 2:
            if (shl) Q0_ = !ALU.Sign, RAM0_ = Q15_;
            else Q15_ = RAM0_, RAM15 = ALU.Carry;
            break;
        case 3:
            if (!shl) {
                RAM15 = ALU.Sign ^ ALU.Over;
                Q15_ = RAM0_;
            }
    }
    return (tRAM0 != RAM0_ || tRAM15 != RAM15 || tQ0 != Q0_ || tQ15 != Q15_);
}

// 状态位产生
void Tec2::procSst() {
    int f = FLAGS_.Y;
    switch (SST_) {
        case 0:
            break;
        case 1:
            f = getBits(f, 3, 0) |
                int(ALU.Sign) << 4 |
                int(ALU.Over) << 5 |
                int(ALU.Zero) << 6 |
                int(ALU.Carry) << 7;
            break;
        case 2:
            f = getBits(IB_, 7, 4) << 4 | getBits(f, 3, 0);
            break;
        case 3:
            setBit(f, 7, false);
            break;
        case 4:
            setBit(f, 7, true);
            break;
        case 5:
            setBit(f, 7, RAM0_);
            break;
        case 6:
            setBit(f, 7, RAM15);
            break;
        case 7:
            setBit(f, 7, Q0_);
    }
    FLAGS_.input(f);
}

// 控制内存外设读写及微码装入
bool Tec2::procMrw() {
    bool changed = false;
    switch (MRW_) {
        case 0:
            MEM[AB_] = IB_;
            break; // 存储器写
        case 1: // 存储器读
            if (IB_ != MEM[AB_]) changed = true, IB_ = MEM[AB_];
            break;
        case 2:
            break; // IO写
        case 3:
            break; // IO读
        case 4:
        case 5:
            break; // 不操作
        case 6:
        case 7: // 装入微码
            for (int i = 3; i >= 0; i--) MCM_[(AB_ << 2) + 3 - i] = LDR_[i].Y;
            break;
    }
    return changed;
}

// 向IB总线发送控制
bool Tec2::procDc1() {
    int orgIB = IB_;
    switch (DC1_) {
        case 0: // SWTOIB, 开关手拔数据
            if (MRW_ != 1 && MRW_ != 3) IB_ = D_;
            break;
        case 1:
            IB_ = ALU.Y;
            break; // RTOIB, 运算器的输出
        case 2: // ITOIB, 指令的低8位
            IB_ = getBits(IR_.Y, 7, 0);
            if (getBit(IB_, 7)) IB_ |= 0xff00;
            break;
        case 3:
            IB_ = FLAGS_.Y;
            break; // FTOIB, 状态寄存器
        case 4:
            break; // INTA, 中断向量	待加！！
        case 5:
            break; // NC, 未使用
        case 6:
            break; // EI, 转用于开中断	待加！！
        case 7:
            break; // DI, 转用于开中断	待加！！
    }
    return orgIB != IB_;
}

// 寄存器接收控制
void Tec2::procDc2() {
    switch (DC2_) {
        case 0:
            break; // NC, 未使用
        case 1:
            IR_.input(IB_);
            break; // GIR, 指令寄存器IR
        case 2:
            AR_.input(ALU.Y);
            break; // GAR, 地址寄存器AR
        case 3:
            break; // GINTP, 中断优先级
        case 4:
            LDR_[0].input(IB_);
            break;
        case 5:
            LDR_[1].input(IB_);
            break;
        case 6:
            LDR_[2].input(IB_);
            break;
        case 7:
            LDR_[3].input(IB_);
    }
}

// CC条件码形成
void Tec2::generateCc() {
    switch (SCC_) {
        case 0:
            CC_ = false;
            break;
        case 1:
            CC_ = true;
            break;
        case 2:
            CC_ = !(SC_ ? getBit(FLAGS_.Y, 7) : getBit(FS_, 0));
            break;
        case 3:
            CC_ = !(SC_ ? getBit(FLAGS_.Y, 6) : getBit(FS_, 1));
            break;
        case 4:
            CC_ = !(SC_ ? getBit(FLAGS_.Y, 5) : getBit(FS_, 2));
            break;
        case 5:
            CC_ = !(SC_ ? getBit(FLAGS_.Y, 4) : WAIT_);
            break;
        case 6:
            CC_ = !INT_;
            break;
        case 7:
            switch (getBits(IR_.Y, 9, 8)) {
                case 0:
                    CC_ = getBit(FLAGS_.Y, 7);
                    break;
                case 1:
                    CC_ = getBit(FLAGS_.Y, 6);
                    break;
                case 2:
                    CC_ = getBit(FLAGS_.Y, 5);
                    break;
                case 3:
                    CC_ = getBit(FLAGS_.Y, 4);
                    break;
            }
            if (getBit(IR_.Y, 10) == 0) CC_ = !CC_;
    }
}

int Tec2::selectD(int CI) {
    if (CI == 2) return MapRom_[getBits(IR_.Y, 15, 10)];
    else if (CI == 6) return D_;
    else return MRA_;
    // if (!MCTL.PL) return MRA_;
    // if (!MCTL.MAP) return MapRom_[getBits(IR_.Y, 15, 10)];
    // return D_;
}

void Tec2::setPlr() {
    int i, p = MCTL.Y << 2;
    for (i = 3; i >= 0; i--) PLR[i].input(MCM_[p++]);
}


int Tec2::getY() const { return ALU.Y; }

int Tec2::getFlags() const { return FLAGS_.Y; }

void Tec2::getReg(int *R) {
    int i;
    for (i = 0; i < 16; i++) R[i] = ALU.Register[i].Y;
}

int Tec2::getAr() const {
    return AR_.Y;
};

void Tec2::setReg(int R, int value) {
    ALU.Register[R].input(value);
    ALU.Register[R].update();
}
