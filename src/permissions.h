#define _SUB 1
#define _MOD 2
#define _OWN 4

typedef unsigned char perm_t;

#define P_RESET(p) ((p) = 0)

#define P_STSUB(p) ((p) |= _SUB)
#define P_STMOD(p) ((p) |= _MOD)
#define P_STOWN(p) ((p) |= _OWN)

#define P_ISSUB(p) (((p) & _SUB) == _SUB)
#define P_ISMOD(p) (((p) & _MOD) == _MOD)
#define P_ISOWN(p) (((p) & _OWN) == _OWN)

#define P_ALSUB(p) ((p) != 0)
#define P_ALMOD(p) (((p) >> 1) != 0)
