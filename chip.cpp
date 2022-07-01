#include "chip.h"
#include "bit.h"

Reg::Reg() {
    Y &= 0xffff;
    D_ &= 0xffff;
}

void Reg::input(int D) {
    this->D_ = D;
}

void Reg::update() {
    Y = D_;
}

Am2901::Am2901() : Sign(false), Zero(false), Over(false), Carry(false) {
    Y &= 0xffff;
    F_ &= 0xffff;
    B_ &= 0xf;
}

bool Am2901::compute(int D, int I, int A, int B, bool Cn,
                     bool &RAM0, bool &RAM15, bool &Q0, bool &Q15) {
    int R = 0, S = 0, tmp;
    bool changed = false, btmp;
    this->B_ = B;
    Modify_Q_ = false;
    Modify_B_ = true;

    // 根据 I2-0 选择操作数
    switch (getBits(I, 2, 0)) {
        case 0:
            R = Register[A].Y;
            S = Q_.Y;
            break;
        case 1:
            R = Register[A].Y;
            S = Register[B].Y;
            break;
        case 2:
            R = 0;
            S = Q_.Y;
            break;
        case 3:
            R = 0;
            S = Register[B].Y;
            break;
        case 4:
            R = 0;
            S = Register[A].Y;
            break;
        case 5:
            R = D;
            S = Register[A].Y;
            break;
        case 6:
            R = D;
            S = Q_.Y;
            break;
        case 7:
            R = D;
            S = 0;
    }

    btmp = Over;
    Over = false;
    // 根据 I5-3 选择运算类型
    switch (getBits(I, 5, 3)) {
        case 0:
            F_ = R + S + Cn;
            if (R <= 32767 && S <= 32767 && F_ > 32767) Over = true;
            break;
        case 1:
            F_ = S - R - Cn;
            if (F_ < 0) F_ += 65536;
            if (S > 32767 && R > 0 && F_ <= 32767) Over = true;
            break;
        case 2:
            F_ = R - S - Cn;
            if (F_ < 0) F_ += 65536;
            if (R > 32767 && S > 0 && F_ <= 32767) Over = true;
            break;
        case 3:
            F_ = R | S;
            break;
        case 4:
            F_ = R & S;
            break;
        case 5:
            F_ = R & S;
            break;
        case 6:
            F_ = R ^ S;
            break;
        case 7:
            F_ = ~(R ^ S) & 0xffff;
    }
    if (btmp != Over) changed = true;

    btmp = Carry;
    Carry = getBit(F_, 16);
    setBit(F_, 16, false);
    if (btmp != Carry) changed = true;

    btmp = Zero;
    Zero = (F_ == 0);
    if (btmp != Zero) changed = true;

    btmp = Sign;
    Sign = getBit(F_, 15);
    if (btmp != Sign) changed = true;

    // 根据 I8-6 选择结果处理方式
    switch (getBits(I, 8, 6)) {
        case 0:
            Q_.input(F_);
            Modify_Q_ = true;
            Modify_B_ = false;
            break;
        case 1:
            Modify_B_ = false;
            break;
        case 2:
            Register[B].input(F_);
            break;
        case 3:
            Register[B].input(F_);
            break;
        case 4:
            RAM0 = getBit(F_, 0);
            tmp = F_ >> 1;
            setBit(tmp, 15, RAM15);
            Register[B].input(tmp);

            Q0 = getBit(Q_.Y, 0);
            tmp = Q_.Y >> 1;
            setBit(tmp, 15, Q15);
            Q_.input(tmp);
            Modify_Q_ = true;
            break;
        case 5:
            RAM0 = getBit(F_, 0);
            tmp = F_ >> 1;
            setBit(tmp, 15, RAM15);
            Register[B].input(tmp);
            break;
        case 6:
            RAM15 = getBit(F_, 15);
            tmp = F_;
            setBit(tmp, 15, false);
            tmp <<= 1;
            setBit(tmp, 0, RAM0);
            Register[B].input(tmp);

            Q15 = getBit(Q_.Y, 15);
            tmp = Q_.Y;
            setBit(tmp, 15, false);
            tmp <<= 1;
            setBit(tmp, 0, Q0);
            Q_.input(tmp);
            Modify_Q_ = true;
            break;
        case 7:
            RAM15 = getBit(F_, 15);
            tmp = F_;
            setBit(tmp, 15, false);
            tmp <<= 1;
            setBit(tmp, 0, RAM0);
            Register[B].input(tmp);
    }

    tmp = Y;
    if (getBits(I, 8, 6) == 2) Y = Register[A].Y;
    else Y = F_;
    if (tmp != Y) changed = true;

    return changed;
}

void Am2901::update() {
    if (Modify_B_) Register[B_].update();
    if (Modify_Q_) Q_.update();
}

Am2910::Am2910() {
    int i;
    uPC_.input(0);
    uPC_.update();
    uSP_.input(-1);
    uSP_.update();
    PL = false;
    MAP = VECT = true;
    for (i = 0; i < 5; i++) {
        uStack_[i].input(0);
        uStack_[i].update();
    }
}

void Am2910::combine(int D, int I, bool CC, int CI) {
    // int tmp = PL<<2 | MAP<<1 | VECT;
    // PL = MAP = VECT = 1;
    switch (I) {
        case 0:
            Y = 0;
            uSP_.Y = -1;
            break;
        case 1:
            if (CC) Y = uPC_.Y;
            else {
                Y = D;
                uStack_[uSP_.Y + 1].input(uPC_.Y);
                uSP_.input(uSP_.Y + 1);
            }
            break;
        case 2:
            Y = D;
            break;
        case 3:
            Y = (CC ? uPC_.Y : D);
            break;
        case 4:
            if (!CC) R_.input(D);
            Y = uPC_.Y;
            uStack_[uSP_.Y + 1].input(uPC_.Y);
            uSP_.input(uSP_.Y + 1);
            break;
        case 6:
            Y = (CC ? uPC_.Y : D);
            break;
        case 8:
            if (R_.Y != 0) {
                R_.input(R_.Y - 1);
                Y = uStack_[uSP_.Y].Y;
            } else {
                Y = uPC_.Y;
                uSP_.input(uSP_.Y - 1);
            }
            break;
        case 10:
            if (CC) Y = uPC_.Y;
            else {
                Y = uStack_[uSP_.Y].Y;
                uSP_.input(uSP_.Y - 1);
            }
            break;
        case 14:
            Y = uPC_.Y;
            break;
        case 15:
            if (R_.Y != 0) {
                R_.input(R_.Y - 1);
                if (CC) Y = uStack_[uSP_.Y].Y;
                else {
                    Y = uPC_.Y;
                    uSP_.input(uSP_.Y - 1);
                }
            } else {
                Y = (CC ? D : uPC_.Y);
                uSP_.input(uSP_.Y - 1);
            }
            break;
        default:
            break;
    }
    uPC_.input(Y + CI);
    // return tmp != (PL<<2 | MAP<<1 | VECT);
}

void Am2910::schedule() {
    uPC_.update();
    uSP_.update();
    if (uSP_.Y >= 0) uStack_[uSP_.Y].update();
    R_.update();
}
