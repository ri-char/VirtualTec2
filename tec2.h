#ifndef C_TEC2_H
#define C_TEC2_H

#include "chip.h"

class Tec2 {
public:
    int LED{}; // 显示灯的状态
    int MEM[65536]{}; // 主存
    Am2901 ALU;
    Am2910 MCTL;
    Reg PLR[4]; // 微指令寄存器
    int bp; // 断点
    bool bping;

    Tec2();

    void offline(int D, int SW2, int SW1, int CK);

    bool cont(int address = -1);

    void step();

    void step2();

    // B3_, B2_, B1_, B0_ 分别是微指令中的 63-48, 47-32, 31-16, 15-0 位
    void setStatus(int FS, int D, int B3, int B2, int B1, int B0);

    void setLED(int S);

    int readFile(char *filename);

    int writeFile(char *filename, int address, int n);

    void saveMcm();

    bool ldmcReset();

    void run(int CK);

    int getY() const;

    int getFlags() const;

    void getReg(int *R);

    int getAr() const;

    void setReg(int R, int value);

private:
    Reg FLAGS_; // 标志寄存器，从高到低位依次为：C,Z,V,S,INTE,P2,P1,P0
    Reg AR_; // 地址寄存器
    Reg IR_; // 指令寄存器
    Reg LDR_[4];

    int IB_{}; // 内部总线
    int AB_{}; // 地址总线
    int D_{}; // 手动开关输入
    int FS_;
    bool CIN_{}; // 进位输入
    bool RAM0_{}, RAM15{};
    bool Q0_{}, Q15_{};
    int MCM_[4096]{}; // 控存
    int MapRom_[64]{};
    bool WAIT_{}, INT_;
    bool ONLINE_;

    int B3_{}, B2_{}, B1_{}, B0_{};
    int MRA_{}; // 下地址
    int CI_{}; // AM2910命令
    int SCC_{}; // AM2910条件码
    int SC_{}; // AM2910条件码
    bool CC_{}; // AM2910条件码
    int SST_{}; // 状态位产生
    int MRW_{}; // 控制内存外设读写及微码装入
    int MI_{}; // AM2901控制码
    int A_; // A口地址
    int B_; // B口地址
    int SCI_{}; // AM2901最低位进位
    int SSH_{}; // 移位控制
    int SA_{};
    int SB_{};
    int DC1_{}; // 向IB总线发送控制
    int DC2_{}; // 寄存器接收控制
    int SW1_{}; // 微型开关SW1
    int SW2_{}; // 微型开关SW2

    bool procSci();

    bool procSsh();

    void procSst();

    bool procMrw();

    bool procDc1();

    void procDc2();

    void generateCc();

    int selectD(int CI);

    void setPlr();

    bool loadMcr();

    bool loadMapRom();

    void runCombine();

    void runSchedule();
};

#endif
