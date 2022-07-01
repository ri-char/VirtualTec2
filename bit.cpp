#include "bit.h"

// 返回F的第p位
bool getBit(int F, int p) {
    return (F & (1 << p)) > 0;
}

// 返回F的low与high之间所有位
int getBits(int F, int high, int low) {
    F >>= low;
    //F_ &= (0xffff>>(16-(high-low+1)));
    F &= (1 << (high - low + 1)) - 1;
    return F;
}

// 设置F的第p位为bit
void setBit(int &F, int p, bool bit) {
    if (bit) F |= (1 << p);
    else F &= ~(1 << p);
}
