#ifndef C_CHIP_H
#define C_CHIP_H

// 16位寄存器
class Reg {
public:
    Reg();

    void update();

    void input(int D);

    int Y{}; // 输出端
private:
    int D_{};
};

// Am2901运算器(这里把AM2901直接扩充成16位)
class Am2901 {
public:
    int Y{}; // 向外送出的数据信号
    bool Sign, Zero, Over, Carry; // 状态标志位
    Reg Register[16]; // 寄存器组

    Am2901();

    bool compute(int D, int I, int A, int B, bool Cn,
                 bool &RAM0, bool &RAM15, bool &Q0, bool &Q15);

    void update();

private:
    Reg Q_; // Q寄存器
    int F_{}; // 计算结果
    bool Modify_Q_{}, Modify_B_{};
    int B_{};
};

// Am2910控制器
class Am2910 {
public:
    int Y{}; // 下地址
    bool FULL{}; // 微堆栈满信号，低电平有效
    bool PL, MAP, VECT;

    Am2910();

    void combine(int D, int I, bool CC, int CI);

    void schedule();

private:
    Reg uPC_, uSP_, uStack_[5];
    Reg R_; // 寄存器/计数器
};

#endif
