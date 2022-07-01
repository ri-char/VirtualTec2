#include <fstream>
#include <cstring>
#include <cctype>
#include <curses.h>
#include <string>
#include <vector>
#include "tec2.h"
#include "bit.h"

using namespace std;

Tec2 tec2;
char buffer[1000];
char inst[64][20];
int num;
bool db;
_win_st *win;

void init();

void procStr(char *str);

int hexValue(char *str, char end = 0);

int getReg(char *str, char end = 0);

void inputHex(char *str, int len = 4);

void errCmd();

int viewInst(int address);

void viewStatus();

void replaceCnd(char *string);

void cmdA();

void cmdB();

void cmdU();

void cmdG();

void cmdT();

void cmdP();

void cmdR(bool u = true);

void cmdD();

void cmdE();

void cmdH();

void cmdS();

void cmdTt();

void cmdHelp();

void onF10();

int main() {
    int c, cp;
    int i;

    win = initscr();
    cbreak();
    noecho();
    scrollok(win, true);

    init();

    printw("TEC-2 CRT MONITOR\nfor macOS / Linux, June.2022\n\n");
    string my_buf="";

    while (true) {
        printw("> ");
        i = 0;
        my_buf.clear();
        while ((c = getch()) != '\n') {
            if (cp == 0xE0 && c == KEY_F(10)) {
                onF10();
                i = 0;
                break;
            } else if ((c == 127 || c == '\b')) {
                if(i>0){
                    delch();
                    addch('\b');
                    addch(' ');
                    addch('\b');
                    my_buf.pop_back();
                    i--;
                }
                continue;
            } else if (isgraph(c) || c == ' ') {
                my_buf+=c;
                addch(c);
                i++;
            } else {
                continue;
            }
            cp = c;
        }
        if (my_buf.length() == 0) {
            continue;
        }
        printw("\n");
        strcpy(buffer,my_buf.c_str());

        procStr(buffer);
        if (buffer[1] && (buffer[0] == 'S' || buffer[0] == 'Q' || buffer[0] == 'C')) {
            errCmd();
            continue;
        }
        switch (buffer[0]) {
            case 0:
                break;
            case 'A':
                cmdA();
                break;
            case 'U':
                cmdU();
                break;
            case 'G':
                cmdG();
                break;
            case 'T':
                if (buffer[1] == 'T') cmdTt();
                else cmdT();
                break;
            case 'P':
                cmdP();
                break;
            case 'R':
                cmdR();
                break;
            case 'D':
                cmdD();
                break;
            case 'E':
                cmdE();
                break;
            case 'S':
                cmdS();
                break;
            case '?':
                cmdHelp();
                break;
            case 'H':
                cmdH();
                break;
            case 'Q':
                endwin();
                return 1;
            case 'V':
                viewStatus();
                break;
            case 'B':
                cmdB();
                break;
            default:
                errCmd();
        }
    }
}

void init() {
    ifstream f("INST.ROM");
    if (!f.is_open()) {
        printw("初使化错误! 找不到文件INST.ROM!\n");
        exit(2);
    }
    f >> num;
    f.getline(inst[0], 20);
    for (int i = 0; i < num; i++) {
        f.getline(inst[i], 20);
        inst[i][strlen(inst[i]) - 1] = '\0';
    }
    f.close();
    if (!tec2.ldmcReset()) {
        printw("初始化错误! 请确认所在目录下有文件 MCM_.ROM 和 MAPROM.ROM\n");
        exit(2);
    }

    db = false;
    tec2.setReg(4, 0xffff);
}

int seekI(const char *ins) {
    int i = 0;
    while (i < num && strcmp(ins, inst[i]) != 0) {
        i++;
    }
    return (i < num ? i : -1);
}

void procStr(char *str) {
    int i, j, pre;
    auto len = strlen(str);
    for (i = 0; i < len; i++) if (str[i] == '\t') str[i] = ' ';
    i = pre = 0;
    // 多个空格合并为一个
    while (i < len) {
        while (i < len && str[i] == ' ') i++;
        for (j = i; j <= len; j++) str[j - i + pre] = str[j];
        len -= (i - pre);
        i = pre;
        while (i < len && str[i] != ' ') i++;
        i++;
        pre = i;
    }
    if (str[len - 1] == ' ') str[--len] = 0; // 去末尾空格
    if (str[1] == ' ') // 去命令后空格
    {
        for (i = 2; i <= len; i++) str[i - 1] = str[i];
        len--;
    }
    i = 0;
    while (i < len && str[i] != ',') i++;
    if (i < len) {
        if (i > 0 && str[i - 1] == ' ') {
            j = i--;
            while (j <= len) str[j - 1] = str[j], j++;
            len--;
        }
        if (i < len - 1 && str[i + 1] == ' ') {
            j = i + 2;
            while (j <= len) str[j - 1] = str[j], j++;
            len--;
        }
    }
    for (i = 0; i < len; i++) str[i] = static_cast<char>(toupper(str[i]));
}

int hexValue(char *str, char end) {
    bool valid = true;
    int ret = 0;
    if (*str == end) valid = false;
    while (*str != end && valid) {
        if (isdigit(*str) || (*str >= 'A' && *str <= 'F')) {
            ret <<= 4;
            ret += (isdigit(*str) ? *str - '0' : *str - 'A' + 10);
            if (ret > 65535) valid = false;
        } else valid = false;
        str++;
    }
    return (valid ? ret : -1);
}

void inputHex(char *str, int len) {
    int c;
    int j = 0;
    while ((c = getch()) != '\n') {
        if (c == 127 || c == '\b') {
            if (j > 0) {
                delch();
                addch('\b');
                addch(' ');
                addch('\b');
                j--;
            }
        } else if (j < len && (isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            addch(c);
            str[j++] = static_cast<char>(c);
        }
    }
    str[j] = 0;
    procStr(str);
    printw("\n");
}

int getReg(char *str, char end) {
    int n;
    n = -1;
    if (str[0] == 'R') {
        if (isdigit(str[1]) && str[2] == end) n = str[1] - '0';
        else if (str[1] == '1' && str[2] >= '0' && str[2] <= '5' && str[3] == end) {
            n = 10 + str[2] - '0';
        }
    } else if (str[1] && str[2] == end) {
        if (str[0] == 'S' && str[1] == 'P') n = 4;
        else if (str[0] == 'P' && str[1] == 'C') n = 5;
        else if (str[0] == 'I' && str[1] == 'P') n = 6;
    }
    return n;
}

void errCmd() {
    printw("Unknown command! Type ? for help.\n");
}

int viewInst(int address) {
    int j, op, ret = 1, r;
    printw("%04X: ", address);
    printw("%04X ", tec2.MEM[address]);
    if ((op = getBits(tec2.MEM[address], 15, 10)) < num) {
        if ((strstr(inst[op], "ADR") != nullptr && strncmp("JR", inst[op], 2) != 0)
            || strstr(inst[op], "DATA") != nullptr) {
            printw("%04X  ", tec2.MEM[address + 1]);
            ret = 2;
        } else printw("      ");
        auto len = strlen(inst[op]);
        j = 0;
        while (j < len && inst[op][j] != ' ') printw("%c", inst[op][j++]);
        while (j < len) {
            if (strncmp("DR", inst[op]+j, 2) == 0) {
                r = getBits(tec2.MEM[address], 7, 4);
                if (r == 4) printw("SP");
                else if (r == 5) printw("PC");
                else if (r == 6) printw("IP");
                else printw("R_%d", r);
                j += 2;
            } else if (strncmp("SR", inst[op]+j, 2) == 0) {
                r = getBits(tec2.MEM[address], 3, 0);
                if (r == 4) printw("SP");
                else if (r == 5) printw("PC");
                else if (r == 6) printw("IP");
                else printw("R_%d", r);
                j += 2;
            } else if (strncmp("ADR", inst[op]+j, 3) == 0) {
                if (strncmp("JR", inst[op], 2) == 0) {
                    printw("%02X", getBits(tec2.MEM[address], 7, 0));
                } else printw("%04X", tec2.MEM[address + 1]);
                j += 3;
            } else if (strncmp("DATA", inst[op]+j, 4) == 0) {
                printw("%04X", tec2.MEM[address + 1]);
                j += 4;
            } else if (strncmp("PORT", inst[op]+j, 4) == 0) {
                printw("%02X", getBits(tec2.MEM[address], 7, 0));
                j += 4;
            } else if (strncmp("CND", inst[op]+j, 3) == 0) {
                if (!getBit(tec2.MEM[address], 10)) printw("N");
                switch (getBits(tec2.MEM[address], 9, 8)) {
                    case 0:
                        printw("C");
                        break;
                    case 1:
                        printw("Z");
                        break;
                    case 2:
                        printw("V");
                        break;
                    case 3:
                        printw("S");
                }
                j += 3;
            } else {
                if (inst[op][j] == ',') printw(",\t");
                else if (inst[op][j] == ' ') printw("\t");
                else printw("%c", inst[op][j]);
                j++;
            }
        }
        printw("\n");
        
    } else {
        printw("      DW     %04X\n", tec2.MEM[address]);
    }
    return ret;
}

void Replace(char *p, int size, int number) {
    if (p == nullptr) return;
    int i;
    auto len = strlen(p);
    for (i = size; i <= len; i++) p[i - size + 1] = p[i];
    p[0] = char(number + '0');
}

void replaceCnd(char *string) {
    Replace(strstr(string, "NC,"), 2, 0);
    Replace(strstr(string, "NZ,"), 2, 1);
    Replace(strstr(string, "NV,"), 2, 2);
    Replace(strstr(string, "NS,"), 2, 3);
    Replace(strstr(string, "C,"), 1, 4);
    Replace(strstr(string, "Z,"), 1, 5);
    Replace(strstr(string, "V,"), 1, 6);
    Replace(strstr(string, "S,"), 1, 7);
}

void cmdA() {
    int i, j, first, value1, value2, data;
    size_t len;
    static int p = 0x800;
    int ir = 0, op, ir2;
    bool valid;
    if (buffer[1]) {
        i = hexValue(buffer + 1);
        if (i < 0) {
            printw("Error\n");
            return;
        }
        p = i;
    }
    printw("%04X: ", p);
    while (true) {
        nocbreak();
        echo();
        getstr(buffer);
        cbreak();
        noecho();

        procStr(buffer);
        replaceCnd(buffer);
        ir2 = -1;
        valid = true;
        len = strlen(buffer);
        if (len == 0) return;
        i = 0;
        while (i < len && buffer[i] != ' ') i++;
        if (i < len) {
            i++;
            first = i;
            if ((value1 = getReg(buffer + first)) >= 0) {
                if (strncmp("MUL", buffer, 3) == 0 || strncmp("DIV", buffer, 3) == 0
                    || strncmp("JP", buffer, 2) == 0 || strncmp("CALL", buffer, 4) == 0) {
                    strcpy(buffer + first, "SR");
                    if ((op = seekI(buffer)) < 0) valid = false;
                    ir = op << 10 | value1;
                } else {
                    strcpy(buffer + first, "DR");
                    if ((op = seekI(buffer)) < 0) valid = false;
                    ir = op << 10 | value1 << 4;
                }
            } else if ((value1 = getReg(buffer + first, ',')) >= 0) {
                i = first;
                while (buffer[i] != ',') i++;
                i++;
                if ((value2 = getReg(buffer + i)) >= 0) {
                    strcpy(buffer + first, "DR,SR");
                    if ((op = seekI(buffer)) < 0) valid = false;
                    ir = op << 10 | value1 << 4 | value2;
                } else if ((data = hexValue(buffer + i)) >= 0) {
                    strcpy(buffer + first, "DR,DATA");
                    if ((op = seekI(buffer)) < 0) valid = false;
                    ir = op << 10 | value1 << 4;
                    ir2 = data;
                } else if ((data = hexValue(buffer + i, '[')) >= 0) {
                    j = i;
                    while (buffer[j] != '[') j++;
                    if ((value2 = getReg(buffer + j + 1, ']')) >= 0) {
                        strcpy(buffer + first, "DR,DATA[SR]");
                        if ((op = seekI(buffer)) < 0) valid = false;
                        ir = op << 10 | value1 << 4 | value2;
                        ir2 = data;
                    } else valid = false;
                } else if (buffer[i] == '[') {
                    if ((value2 = getReg(buffer + i + 1, ']')) >= 0) {
                        strcpy(buffer + first, "DR,[SR]");
                        if ((op = seekI(buffer)) < 0) valid = false;
                        ir = op << 10 | value1 << 4 | value2;
                    } else if ((data = hexValue(buffer + i + 1, ']')) >= 0) {
                        strcpy(buffer + first, "DR,[ADR]");
                        if ((op = seekI(buffer)) < 0) valid = false;
                        ir = op << 10 | value1 << 4;
                        ir2 = data;
                    } else valid = false;
                } else valid = false;
            } else if ((data = hexValue(buffer + first, '[')) >= 0) {
                i = first;
                while (buffer[i] != '[') i++;
                if ((value2 = getReg(buffer + i + 1, ']')) >= 0) {
                    j = i + 1;
                    while (buffer[j] != ']') j++;
                    if ((buffer[++j] == ',') && (value1 = getReg(buffer + j + 1)) >= 0) {
                        strcpy(buffer + first, "DATA[SR],DR");
                        if ((op = seekI(buffer)) < 0) valid = false;
                        ir = op << 10 | value1 << 4 | value2;
                        ir2 = data;
                    } else valid = false;
                } else valid = false;
            } else if (buffer[first] == '[' && (value1 = getReg(buffer + first + 1, ']')) >= 0) {
                i = first;
                while (buffer[i] != ']') i++;
                if (buffer[++i] == ',' && (value2 = getReg(buffer + i + 1)) >= 0) {
                    strcpy(buffer + first, "[DR],SR");
                    if ((op = seekI(buffer)) < 0) valid = false;
                    ir = op << 10 | value1 << 4 | value2;
                } else valid = false;
            } else if (buffer[first] == '[' && (data = hexValue(buffer + first + 1, ']')) >= 0) {
                i = first;
                while (buffer[i] != ']') i++;
                if (buffer[++i] == ',' && (value2 = getReg(buffer + i + 1)) >= 0) {
                    strcpy(buffer + first, "[ADR],SR");
                    if ((op = seekI(buffer)) < 0) valid = false;
                    ir = op << 10 | value2;
                    ir2 = data;
                }
            } else if ((data = hexValue(buffer + first)) >= 0) {
                if (strncmp("JP", buffer, 2) != 0 && data >= 256) valid = false;
                else if (strncmp("IN", buffer, 2) == 0 || strncmp("OUT", buffer, 3) == 0) {
                    strcpy(buffer + first, "PORT");
                    if ((op = seekI(buffer)) < 0) valid = false;
                    ir = op << 10 | data;
                } else if (strncmp("JP", buffer, 2) == 0) {
                    strcpy(buffer + first, "ADR");
                    if ((op = seekI(buffer)) < 0) valid = false;
                    ir = op << 10;
                    ir2 = data;
                } else {
                    strcpy(buffer + first, "ADR");
                    if ((op = seekI(buffer)) < 0) valid = false;
                    ir = op << 10 | data;
                }
            } else if ((data = hexValue(buffer + first, ',')) >= 0) {
                if (data >= 8) {
                    valid = false;
                } else if ((value1 = getReg(buffer + first + 2)) >= 0) {
                    strcpy(buffer + first, "CND,SR");
                    if ((op = seekI(buffer)) < 0) valid = false;
                    ir = op << 10 | data << 8 | value1;
                } else if ((value1 = hexValue(buffer + first + 2)) >= 0) {
                    strcpy(buffer + first, "CND,ADR");
                    if ((op = seekI(buffer)) < 0) valid = false;
                    ir = op << 10 | data << 8;
                    ir2 = value1;
                } else valid = false;
            } else valid = false;
        } else if ((op = seekI(buffer)) >= 0) {
            ir = op << 10;
        } else valid = false;

        if (!valid) {
            printw("\tError\n");
        } else {
            tec2.MEM[p++] = ir;
            if (ir2 >= 0) tec2.MEM[p++] = ir2;
        }
        printw("%04X: ", p);
    }
}

void cmdU() {
    int i;
    static int p = 0x800;
    if (buffer[1]) {
        if ((i = hexValue(buffer + 1)) < 0) {
            printw("Error\n");
            return;
        }
        p = i;
    }
    for (i = 0; i < 16; i++) {
        p += viewInst(p);
        p &= 0xffff;
    }
}

void cmdB() {
    int i;
    if (buffer[1]) {
        if ((i = hexValue(buffer + 1)) < 0) {
            printw("Error\n");
            return;
        }
        tec2.bp = i;
    } else tec2.bp = -1;
}

void cmdG() {
    int i;
    if (buffer[1]) {
        if ((i = hexValue(buffer + 1)) < 0) {
            printw("Error\n");
            return;
        }
        tec2.setReg(5, i);
    } else {
        i = -1;
    }
    if (!tec2.cont(i)) {
        printw("Dead cycle!\n");
    }
}

void cmdTt() {
    if (buffer[2]) {
        printw("Error\n");
        return;
    }
    if (!db) {
        db = true;
        tec2.setStatus(5, 0, 0x29, 0x0300, 0x90f0, 0);
        while (tec2.MCTL.Y != 0x1a) tec2.run(1);
    }
    tec2.run(1);
    if (tec2.MCTL.Y == 0xa4) {
        db = false;
        while (tec2.MCTL.Y != 0x1a) tec2.run(1);
        tec2.run(1);
    }
    viewStatus();
    // viewInst(tec2.ALU.Reg[6].Y);
    buffer[1] = 0;
    cmdR(false);
}

void viewStatus() {
    int i;
    printw("Next: %03X ", tec2.MCTL.Y);
    for (i = 3; i >= 0; i--) printw("  %04X", tec2.PLR[i].Y);
    printw("\n");
    printw("Analyse: ");

    int B3, B2, B1, B0;
    int MI, MI876, MI543, MI210, MI0, REQ, WE;
    int MRA, CI, SCC, SC, SST, MRW, A, B, SCI, SSH, SA, SB, DC1, DC2;

    B0 = tec2.PLR[0].Y;
    B1 = tec2.PLR[1].Y;
    B2 = tec2.PLR[2].Y;
    B3 = tec2.PLR[3].Y;
    MRA = getBits(B3, 7, 0) << 2 | getBits(B2, 15, 14);
    CI = getBits(B2, 11, 8);
    SCC = getBits(B2, 7, 5);
    SC = getBit(B2, 4);
    SST = getBits(B2, 2, 0);
    MI0 = getBit(B1, 15);
    REQ = getBit(B1, 11);
    WE = getBit(B1, 7);
    MRW = MI0 << 2 | REQ << 1 | WE;
    MI876 = getBits(B1, 14, 12);
    MI543 = getBits(B1, 10, 8);
    MI210 = getBits(B1, 6, 4);
    MI = MI876 << 6 | MI543 << 3 | MI210;
    A = getBits(B1, 3, 0);
    B = getBits(B0, 15, 12);
    SCI = getBits(B0, 11, 10);
    SSH = getBits(B0, 9, 8);
    SA = getBit(B0, 7);
    SB = getBit(B0, 3);
    DC1 = getBits(B0, 6, 4);
    DC2 = getBits(B0, 2, 0);

    printw("AR_=%04X  MEM=%04X  MRA_=%03X  A_=%02X  B_=%02X\n",
           tec2.getAr(), tec2.MEM[tec2.getAr()], MRA, A, B);
    printw("C=%d  Z=%d  V=%d  S=%d\n", getBit(tec2.getFlags(), 7),
           getBit(tec2.getFlags(), 6), getBit(tec2.getFlags(), 5), getBit(tec2.getFlags(), 4));
}

void cmdT() {
    tec2.step();
    cmdR();
}

void cmdP() {
    tec2.step2();
    cmdR();
}

void cmdR(bool u) {
    int i, n;
    int r[16], flags;
    tec2.getReg(r);
    if (!buffer[1]) {
        for (i = 0; i < 4; i++) printw("R_%d=%04X  ", i, r[i]);
        printw("SP=%04X  PC=%04X  IP=%04X  ", r[4], r[5], r[6]);
        printw("R7=%04X  R8=%04X\n", r[7], r[8]);
        for (i = 9; i < 16; i++) printw("R_%d=%04X ", i, r[i]);
        flags = tec2.getFlags();
        printw("  F_=");
        for (i = 7; i >= 0; i--) printw("%d", getBit(flags, i));
        printw("\n");
        if (u) viewInst(r[5]);
        return;
    }
    n = getReg(buffer + 1);
    if (n < 0) {
        printw("Error\n");
        return;
    }
    printw("%04X:-", r[n]);
    inputHex(buffer);
    if (buffer[0]) tec2.setReg(n, hexValue(buffer));
}

void cmdD() {
    static int p = 0x800;
    int i, j;
    char c;
    if (buffer[1]) {
        i = hexValue(buffer + 1);
        if (i < 0) {
            printw("Error!\n");
            return;
        }
        p = i;
    }
    for (i = 0; i < 15; i++) {
        printw("%04X    ", p);
        for (j = 0; j < 8; j++) printw("%04X  ", tec2.MEM[(p + j) & 0xffff]);
        for (j = 0; j < 8; j++) {
            c = static_cast<char>((tec2.MEM[p] & 0xff00) >> 8);
            if (!isgraph(c)) c = '.';
            printw("%c", c);
            c = static_cast<char>((tec2.MEM[p] & 0xff));
            if (!isgraph(c)) c = '.';
            printw("%c", c);
            p++;
            p &= 0xffff;
        }
        printw("\n");
    }
}

void cmdE() {
    int i, j;
    static int p = 0x800;
    int c;
    if (buffer[1]) {
        if ((i = hexValue(buffer + 1)) < 0) {
            printw("Error\n");
            return;
        }
        p = i;
    }
    i = p;
    while (true) {
        if ((i - p) % 5 == 0) {
            if (i > p) {
                printw("\n");
            }
            printw("%04X\t", i);
        }
        printw("%04X:", tec2.MEM[i]);
        j = 0;
        while (c = getch(), c != ' ' && c != '\n') {
            if (c == 127 || c == '\b') {
                if (j > 0) {
                    delch();
                    addch('\b');
                    addch(' ');
                    addch('\b');
                    j--;
                }
            } else if (j < 4 && (isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                addch(c);
                buffer[j++] = static_cast<char>(c);
            }
        }
        buffer[j] = 0;
        procStr(buffer);
        if (j > 0) tec2.MEM[i] = hexValue(buffer);
        if (c == '\n') {
            p = i + 1;
            printw("\n");
            break;
        }
        while (j++ < 4) printw(" ");
        printw("  ");
        i++;
    }
}

void cmdH() {
    int value1, value2, i;
    auto len = strlen(buffer);
    i = 1;
    while (i < len && buffer[i] == ' ') i++;
    if (i < len && (value1 = hexValue(buffer + i, ' ')) >= 0) {
        while (i < len && buffer[i] != ' ') i++;
        while (i < len && buffer[i] == ' ') i++;
        if (i < len && (value2 = hexValue(buffer + i)) >= 0) {
            i = value1 + value2;
            if (i > 65535) i -= 65536;
            printw("%04X  ", i);
            i = value1 - value2;
            if (i < 0) i += 65536;
            printw("%04X\n", i);
        } else {
            printw("Error\n");
        }
    } else {
        printw("Error\n");
    }
}

void cmdS() {
    int c;
    bool yes = false;
    printw("Are you sure to save the MCM_? [N]\b\b");
    while ((c = getch()) != '\n') {
        if (c == 'Y' || c == 'y') {
            printw("Y\b");
            yes = true;
        } else if (c == 'N' || c == 'n') {
            printw("N\b");
            yes = false;
        }
    }
    printw("\n");
    if (yes) tec2.saveMcm();
}

void cmdHelp() {
    printw("Assemble\tA_ [adr]\n"); 
    printw("Unassemble\tU [adr]\n"); 
    printw("Go\t\tG [adr]\n"); 
    printw("Trace\t\tT [adr]\n"); 
    printw("Proceed\t\tP [adr]\n"); 
    printw("Reg\tR_ [reg]\n"); 
    printw("Dump\t\tD [adr]\n"); 
    printw("Enter\t\tE [adr]\n"); 
    printw("Hex\t\tH value1 value2\n"); 
    printw("saveMcm\t\tS\n"); 
    printw("MStep\t\tTT\n"); 
    printw("Clear\t\tC\n"); 
    printw("Quit\t\tQ\n"); 
}

void onF10() {
    int i, bytes, from, len, temp;
    char c, name[256];

    for (i = 0; i < 5; i++) printw("\n"); 
    printw("\t\t0---Return to TEC-2 CRT Monitor\n"); 
    printw("\t\t1---Send a file to TEC-2\n"); 
    printw("\t\t2---Receive a file from TEC-2\n"); 
    printw("\t\t3---Return to PC(MS)-DOS\n\n"); 
    printw("\t\tEnter your choice:[0]\b\b");
    c = '0';
    while (temp = getch(), temp != '\n') {
        if (temp >= '0' && temp <= '3') printw("%c\b", c = static_cast<char>(temp));
    }
    printw("\n");

    switch (c) {
        case '0':
            return;
        case '1':
            printw("\n\t\tFile name for read:");
            getstr(name);
            bytes = tec2.readFile(name);
            if (bytes < 0) {
                printw("\n00000 bytes\n\nFile not found\n\n");
                break;
            } else {
                printw("\n%05d bytes\n\n", bytes);
            }
            break;
        case '2':
            printw("\n\t\tFile name for write:");
            getstr(name);
            printw("\t\tBegin from: ");
            inputHex(buffer);
            from = hexValue(buffer);
            printw("\t\tData length: ");
            inputHex(buffer);
            len = hexValue(buffer);

            bytes = tec2.writeFile(name, from, len);
            if (bytes < 0) {
                printw("\n\nFile not found\n\n");
                break;
            } else {
                printw("\n%05d bytes\n\n", bytes);
            }
            break;
        case '3':
            exit(0);
        default:
            break;
    }
}
