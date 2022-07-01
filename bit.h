#ifndef C_BIT_H
#define C_BIT_H

// 返回F的第p位
bool getBit(int F, int p);

// 返回F的low与high之间所有位
int getBits(int F, int high, int low);

// 设置F的第p位为bit
void setBit(int &F, int p, bool bit);

#endif
