/* C++ code produced by gperf version 3.0.3 */
/* Command-line:
 * /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/gperf
 * -t Keywords.gperf  */
/* Computed positions: -k'1,$' */

#if !(                                                                         \
    (' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) && ('%' == 37) && \
    ('&' == 38) && ('\'' == 39) && ('(' == 40) && (')' == 41) &&               \
    ('*' == 42) && ('+' == 43) && (',' == 44) && ('-' == 45) && ('.' == 46) && \
    ('/' == 47) && ('0' == 48) && ('1' == 49) && ('2' == 50) && ('3' == 51) && \
    ('4' == 52) && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) && \
    ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) && ('=' == 61) && \
    ('>' == 62) && ('?' == 63) && ('A' == 65) && ('B' == 66) && ('C' == 67) && \
    ('D' == 68) && ('E' == 69) && ('F' == 70) && ('G' == 71) && ('H' == 72) && \
    ('I' == 73) && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) && \
    ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) && ('R' == 82) && \
    ('S' == 83) && ('T' == 84) && ('U' == 85) && ('V' == 86) && ('W' == 87) && \
    ('X' == 88) && ('Y' == 89) && ('Z' == 90) && ('[' == 91) &&                \
    ('\\' == 92) && (']' == 93) && ('^' == 94) && ('_' == 95) &&               \
    ('a' == 97) && ('b' == 98) && ('c' == 99) && ('d' == 100) &&               \
    ('e' == 101) && ('f' == 102) && ('g' == 103) && ('h' == 104) &&            \
    ('i' == 105) && ('j' == 106) && ('k' == 107) && ('l' == 108) &&            \
    ('m' == 109) && ('n' == 110) && ('o' == 111) && ('p' == 112) &&            \
    ('q' == 113) && ('r' == 114) && ('s' == 115) && ('t' == 116) &&            \
    ('u' == 117) && ('v' == 118) && ('w' == 119) && ('x' == 120) &&            \
    ('y' == 121) && ('z' == 122) && ('{' == 123) && ('|' == 124) &&            \
    ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error                                                                         \
    "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 9 "Keywords.gperf"

#include "Token.h"
#include <cstring>
namespace tpy::Parse {
#line 15 "Keywords.gperf"
struct Keyword {
    const char *name;
    TokenKind kind;
};

#define TOTAL_KEYWORDS 35
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 8
#define MIN_HASH_VALUE 2
#define MAX_HASH_VALUE 53
/* maximum key range = 52, duplicates = 0 */

class KeywordLookup {
  private:
    static inline unsigned int hash(const char *str, unsigned int len);

  public:
    static struct Keyword *is_keyword(const char *str, unsigned int len);
};

inline unsigned int KeywordLookup::hash(const char *str, unsigned int len) {
    static unsigned char asso_values[] = {
        54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
        54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
        54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
        54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 15, 54,
        54, 54, 54, 54, 54, 54, 25, 54, 54, 54, 54, 54, 20, 54, 54, 54, 54, 54,
        54, 54, 54, 54, 54, 54, 54, 10, 0,  25, 0,  5,  0,  0,  5,  0,  54, 0,
        30, 0,  15, 5,  0,  54, 20, 20, 0,  54, 54, 10, 54, 5,  54, 54, 54, 54,
        54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
        54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
        54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
        54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
        54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
        54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
        54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
        54, 54, 54, 54};
    return len + asso_values[(unsigned char)str[len - 1]] +
           asso_values[(unsigned char)str[0]];
}

struct Keyword *KeywordLookup::is_keyword(const char *str, unsigned int len) {
    static struct Keyword wordlist[] = {
#line 42 "Keywords.gperf"
        {"if", TokenKind::KeywordIf},
#line 33 "Keywords.gperf"
        {"def", TokenKind::KeywordDef},
#line 40 "Keywords.gperf"
        {"from", TokenKind::KeywordFrom},
#line 30 "Keywords.gperf"
        {"break", TokenKind::KeywordBreak},
#line 43 "Keywords.gperf"
        {"import", TokenKind::KeywordImport},
#line 53 "Keywords.gperf"
        {"try", TokenKind::KeywordTry},
#line 35 "Keywords.gperf"
        {"elif", TokenKind::KeywordElif},
#line 56 "Keywords.gperf"
        {"yield", TokenKind::KeywordYield},
#line 37 "Keywords.gperf"
        {"except", TokenKind::KeywordExcept},
#line 38 "Keywords.gperf"
        {"finally", TokenKind::KeywordFinally},
#line 25 "Keywords.gperf"
        {"and", TokenKind::KeywordAnd},
#line 36 "Keywords.gperf"
        {"else", TokenKind::KeywordElse},
#line 29 "Keywords.gperf"
        {"await", TokenKind::KeywordAwait},
#line 27 "Keywords.gperf"
        {"assert", TokenKind::KeywordAssert},
#line 44 "Keywords.gperf"
        {"in", TokenKind::KeywordIn},
#line 48 "Keywords.gperf"
        {"not", TokenKind::KeywordNot},
#line 55 "Keywords.gperf"
        {"with", TokenKind::KeywordWith},
#line 54 "Keywords.gperf"
        {"while", TokenKind::KeywordWhile},
#line 45 "Keywords.gperf"
        {"is", TokenKind::KeywordIs},
#line 39 "Keywords.gperf"
        {"for", TokenKind::KeywordFor},
#line 50 "Keywords.gperf"
        {"pass", TokenKind::KeywordPass},
#line 22 "Keywords.gperf"
        {"False", TokenKind::KeywordFalse},
#line 49 "Keywords.gperf"
        {"or", TokenKind::KeywordOr},
#line 24 "Keywords.gperf"
        {"True", TokenKind::KeywordTrue},
#line 51 "Keywords.gperf"
        {"raise", TokenKind::KeywordRaise},
#line 26 "Keywords.gperf"
        {"as", TokenKind::KeywordAs},
#line 34 "Keywords.gperf"
        {"del", TokenKind::KeywordDel},
#line 23 "Keywords.gperf"
        {"None", TokenKind::KeywordNone},
#line 41 "Keywords.gperf"
        {"global", TokenKind::KeywordGlobal},
#line 32 "Keywords.gperf"
        {"continue", TokenKind::KeywordContinue},
#line 28 "Keywords.gperf"
        {"async", TokenKind::KeywordAsync},
#line 52 "Keywords.gperf"
        {"return", TokenKind::KeywordReturn},
#line 46 "Keywords.gperf"
        {"labmda", TokenKind::KeywordLambda},
#line 31 "Keywords.gperf"
        {"class", TokenKind::KeywordClass},
#line 47 "Keywords.gperf"
        {"nonlocal", TokenKind::KeywordNonlocal}};

    if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH) {
        unsigned int key = hash(str, len);

        if (key <= MAX_HASH_VALUE && key >= MIN_HASH_VALUE) {
            struct Keyword *resword;

            switch (key - 2) {
            case 0:
                if (len == 2) {
                    resword = &wordlist[0];
                    goto compare;
                }
                break;
            case 1:
                if (len == 3) {
                    resword = &wordlist[1];
                    goto compare;
                }
                break;
            case 2:
                if (len == 4) {
                    resword = &wordlist[2];
                    goto compare;
                }
                break;
            case 3:
                if (len == 5) {
                    resword = &wordlist[3];
                    goto compare;
                }
                break;
            case 4:
                if (len == 6) {
                    resword = &wordlist[4];
                    goto compare;
                }
                break;
            case 6:
                if (len == 3) {
                    resword = &wordlist[5];
                    goto compare;
                }
                break;
            case 7:
                if (len == 4) {
                    resword = &wordlist[6];
                    goto compare;
                }
                break;
            case 8:
                if (len == 5) {
                    resword = &wordlist[7];
                    goto compare;
                }
                break;
            case 9:
                if (len == 6) {
                    resword = &wordlist[8];
                    goto compare;
                }
                break;
            case 10:
                if (len == 7) {
                    resword = &wordlist[9];
                    goto compare;
                }
                break;
            case 11:
                if (len == 3) {
                    resword = &wordlist[10];
                    goto compare;
                }
                break;
            case 12:
                if (len == 4) {
                    resword = &wordlist[11];
                    goto compare;
                }
                break;
            case 13:
                if (len == 5) {
                    resword = &wordlist[12];
                    goto compare;
                }
                break;
            case 14:
                if (len == 6) {
                    resword = &wordlist[13];
                    goto compare;
                }
                break;
            case 15:
                if (len == 2) {
                    resword = &wordlist[14];
                    goto compare;
                }
                break;
            case 16:
                if (len == 3) {
                    resword = &wordlist[15];
                    goto compare;
                }
                break;
            case 17:
                if (len == 4) {
                    resword = &wordlist[16];
                    goto compare;
                }
                break;
            case 18:
                if (len == 5) {
                    resword = &wordlist[17];
                    goto compare;
                }
                break;
            case 20:
                if (len == 2) {
                    resword = &wordlist[18];
                    goto compare;
                }
                break;
            case 21:
                if (len == 3) {
                    resword = &wordlist[19];
                    goto compare;
                }
                break;
            case 22:
                if (len == 4) {
                    resword = &wordlist[20];
                    goto compare;
                }
                break;
            case 23:
                if (len == 5) {
                    resword = &wordlist[21];
                    goto compare;
                }
                break;
            case 25:
                if (len == 2) {
                    resword = &wordlist[22];
                    goto compare;
                }
                break;
            case 27:
                if (len == 4) {
                    resword = &wordlist[23];
                    goto compare;
                }
                break;
            case 28:
                if (len == 5) {
                    resword = &wordlist[24];
                    goto compare;
                }
                break;
            case 30:
                if (len == 2) {
                    resword = &wordlist[25];
                    goto compare;
                }
                break;
            case 31:
                if (len == 3) {
                    resword = &wordlist[26];
                    goto compare;
                }
                break;
            case 32:
                if (len == 4) {
                    resword = &wordlist[27];
                    goto compare;
                }
                break;
            case 34:
                if (len == 6) {
                    resword = &wordlist[28];
                    goto compare;
                }
                break;
            case 36:
                if (len == 8) {
                    resword = &wordlist[29];
                    goto compare;
                }
                break;
            case 38:
                if (len == 5) {
                    resword = &wordlist[30];
                    goto compare;
                }
                break;
            case 39:
                if (len == 6) {
                    resword = &wordlist[31];
                    goto compare;
                }
                break;
            case 44:
                if (len == 6) {
                    resword = &wordlist[32];
                    goto compare;
                }
                break;
            case 48:
                if (len == 5) {
                    resword = &wordlist[33];
                    goto compare;
                }
                break;
            case 51:
                if (len == 8) {
                    resword = &wordlist[34];
                    goto compare;
                }
                break;
            }
            return 0;
        compare: {
            const char *s = resword->name;

            if (*str == *s && !memcmp(str + 1, s + 1, len - 1))
                return resword;
        }
        }
    }
    return 0;
}
#line 57 "Keywords.gperf"

} // namespace tpy::Parse
