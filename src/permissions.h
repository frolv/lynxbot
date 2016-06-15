#define _REG 0x0
#define _SUB 0x1
#define _MOD 0x2
#define _OWN 0x4

typedef unsigned char perm_t;

#define P_RESET(p) ((p) = _REG)

#define P_STSUB(p) ((p) |= _SUB)
#define P_STMOD(p) ((p) |= _MOD)
#define P_STOWN(p) ((p) |= _OWN)

#define P_ISREG(p) ((p) == _REG)
#define P_ISSUB(p) (((p) & _SUB) == _SUB)
#define P_ISMOD(p) (((p) & _MOD) == _MOD)
#define P_ISOWN(p) (((p) & _OWN) == _OWN)

#define P_ALSUB(p) ((p) != _REG)
#define P_ALMOD(p) (((p) >> 1) != _REG)
