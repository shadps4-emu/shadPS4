// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "shader_recompiler/frontend/opcodes.h"

#include <ranges>

using OpcodeSOP1 = Shader::Gcn::OpcodeSOP1;
using OpcodeSOP2 = Shader::Gcn::OpcodeSOP2;
using OpcodeSOPK = Shader::Gcn::OpcodeSOPK;
using OpcodeVOP1 = Shader::Gcn::OpcodeVOP1;
using OpcodeVOP2 = Shader::Gcn::OpcodeVOP2;
using OpcodeVOP3 = Shader::Gcn::OpcodeVOP3;
using OpcodeVOP3P = Shader::Gcn::OpcodeVOP3P;

enum class VOperand8 : u8 {
    V0 = 0,
    V1 = 1,
    V2 = 2,
    V3 = 3,
    V4 = 4,
    V5 = 5,
    V6 = 6,
    V7 = 7,
    V8 = 8,
    V9 = 9,
    V10 = 10,
    V11 = 11,
    V12 = 12,
    V13 = 13,
    V14 = 14,
    V15 = 15,
    V16 = 16,
    V17 = 17,
    V18 = 18,
    V19 = 19,
    V20 = 20,
    V21 = 21,
    V22 = 22,
    V23 = 23,
    V24 = 24,
    V25 = 25,
    V26 = 26,
    V27 = 27,
    V28 = 28,
    V29 = 29,
    V30 = 30,
    V31 = 31,
    V32 = 32,
    V33 = 33,
    V34 = 34,
    V35 = 35,
    V36 = 36,
    V37 = 37,
    V38 = 38,
    V39 = 39,
    V40 = 40,
    V41 = 41,
    V42 = 42,
    V43 = 43,
    V44 = 44,
    V45 = 45,
    V46 = 46,
    V47 = 47,
    V48 = 48,
    V49 = 49,
    V50 = 50,
    V51 = 51,
    V52 = 52,
    V53 = 53,
    V54 = 54,
    V55 = 55,
    V56 = 56,
    V57 = 57,
    V58 = 58,
    V59 = 59,
    V60 = 60,
    V61 = 61,
    V62 = 62,
    V63 = 63,
    V64 = 64,
    V65 = 65,
    V66 = 66,
    V67 = 67,
    V68 = 68,
    V69 = 69,
    V70 = 70,
    V71 = 71,
    V72 = 72,
    V73 = 73,
    V74 = 74,
    V75 = 75,
    V76 = 76,
    V77 = 77,
    V78 = 78,
    V79 = 79,
    V80 = 80,
    V81 = 81,
    V82 = 82,
    V83 = 83,
    V84 = 84,
    V85 = 85,
    V86 = 86,
    V87 = 87,
    V88 = 88,
    V89 = 89,
    V90 = 90,
    V91 = 91,
    V92 = 92,
    V93 = 93,
    V94 = 94,
    V95 = 95,
    V96 = 96,
    V97 = 97,
    V98 = 98,
    V99 = 99,
    V100 = 100,
    V101 = 101,
    V102 = 102,
    V103 = 103,
    V104 = 104,
    V105 = 105,
    V106 = 106,
    V107 = 107,
    V108 = 108,
    V109 = 109,
    V110 = 110,
    V111 = 111,
    V112 = 112,
    V113 = 113,
    V114 = 114,
    V115 = 115,
    V116 = 116,
    V117 = 117,
    V118 = 118,
    V119 = 119,
    V120 = 120,
    V121 = 121,
    V122 = 122,
    V123 = 123,
    V124 = 124,
    V125 = 125,
    V126 = 126,
    V127 = 127,
    V128 = 128,
    V129 = 129,
    V130 = 130,
    V131 = 131,
    V132 = 132,
    V133 = 133,
    V134 = 134,
    V135 = 135,
    V136 = 136,
    V137 = 137,
    V138 = 138,
    V139 = 139,
    V140 = 140,
    V141 = 141,
    V142 = 142,
    V143 = 143,
    V144 = 144,
    V145 = 145,
    V146 = 146,
    V147 = 147,
    V148 = 148,
    V149 = 149,
    V150 = 150,
    V151 = 151,
    V152 = 152,
    V153 = 153,
    V154 = 154,
    V155 = 155,
    V156 = 156,
    V157 = 157,
    V158 = 158,
    V159 = 159,
    V160 = 160,
    V161 = 161,
    V162 = 162,
    V163 = 163,
    V164 = 164,
    V165 = 165,
    V166 = 166,
    V167 = 167,
    V168 = 168,
    V169 = 169,
    V170 = 170,
    V171 = 171,
    V172 = 172,
    V173 = 173,
    V174 = 174,
    V175 = 175,
    V176 = 176,
    V177 = 177,
    V178 = 178,
    V179 = 179,
    V180 = 180,
    V181 = 181,
    V182 = 182,
    V183 = 183,
    V184 = 184,
    V185 = 185,
    V186 = 186,
    V187 = 187,
    V188 = 188,
    V189 = 189,
    V190 = 190,
    V191 = 191,
    V192 = 192,
    V193 = 193,
    V194 = 194,
    V195 = 195,
    V196 = 196,
    V197 = 197,
    V198 = 198,
    V199 = 199,
    V200 = 200,
    V201 = 201,
    V202 = 202,
    V203 = 203,
    V204 = 204,
    V205 = 205,
    V206 = 206,
    V207 = 207,
    V208 = 208,
    V209 = 209,
    V210 = 210,
    V211 = 211,
    V212 = 212,
    V213 = 213,
    V214 = 214,
    V215 = 215,
    V216 = 216,
    V217 = 217,
    V218 = 218,
    V219 = 219,
    V220 = 220,
    V221 = 221,
    V222 = 222,
    V223 = 223,
    V224 = 224,
    V225 = 225,
    V226 = 226,
    V227 = 227,
    V228 = 228,
    V229 = 229,
    V230 = 230,
    V231 = 231,
    V232 = 232,
    V233 = 233,
    V234 = 234,
    V235 = 235,
    V236 = 236,
    V237 = 237,
    V238 = 238,
    V239 = 239,
    V240 = 240,
    V241 = 241,
    V242 = 242,
    V243 = 243,
    V244 = 244,
    V245 = 245,
    V246 = 246,
    V247 = 247,
    V248 = 248,
    V249 = 249,
    V250 = 250,
    V251 = 251,
    V252 = 252,
    V253 = 253,
    V254 = 254,
    V255 = 255
};

enum class SOperand7 : u8 {
    S0 = 0,
    S1 = 1,
    S2 = 2,
    S3 = 3,
    S4 = 4,
    S5 = 5,
    S6 = 6,
    S7 = 7,
    S8 = 8,
    S9 = 9,
    S10 = 10,
    S11 = 11,
    S12 = 12,
    S13 = 13,
    S14 = 14,
    S15 = 15,
    S16 = 16,
    S17 = 17,
    S18 = 18,
    S19 = 19,
    S20 = 20,
    S21 = 21,
    S22 = 22,
    S23 = 23,
    S24 = 24,
    S25 = 25,
    S26 = 26,
    S27 = 27,
    S28 = 28,
    S29 = 29,
    S30 = 30,
    S31 = 31,
    S32 = 32,
    S33 = 33,
    S34 = 34,
    S35 = 35,
    S36 = 36,
    S37 = 37,
    S38 = 38,
    S39 = 39,
    S40 = 40,
    S41 = 41,
    S42 = 42,
    S43 = 43,
    S44 = 44,
    S45 = 45,
    S46 = 46,
    S47 = 47,
    S48 = 48,
    S49 = 49,
    S50 = 50,
    S51 = 51,
    S52 = 52,
    S53 = 53,
    S54 = 54,
    S55 = 55,
    S56 = 56,
    S57 = 57,
    S58 = 58,
    S59 = 59,
    S60 = 60,
    S61 = 61,
    S62 = 62,
    S63 = 63,
    S64 = 64,
    S65 = 65,
    S66 = 66,
    S67 = 67,
    S68 = 68,
    S69 = 69,
    S70 = 70,
    S71 = 71,
    S72 = 72,
    S73 = 73,
    S74 = 74,
    S75 = 75,
    S76 = 76,
    S77 = 77,
    S78 = 78,
    S79 = 79,
    S80 = 80,
    S81 = 81,
    S82 = 82,
    S83 = 83,
    S84 = 84,
    S85 = 85,
    S86 = 86,
    S87 = 87,
    S88 = 88,
    S89 = 89,
    S90 = 90,
    S91 = 91,
    S92 = 92,
    S93 = 93,
    S94 = 94,
    S95 = 95,
    S96 = 96,
    S97 = 97,
    S98 = 98,
    S99 = 99,
    S100 = 100,
    S101 = 101,
    S102 = 102,
    S103 = 103,
    VccLo = 106,
    VccHi = 107,
    M0 = 124,
    ExecLo = 126,
    ExecHi = 127,
};

enum class SOperand8 : u16 {
    S0 = 0,
    S1 = 1,
    S2 = 2,
    S3 = 3,
    S4 = 4,
    S5 = 5,
    S6 = 6,
    S7 = 7,
    S8 = 8,
    S9 = 9,
    S10 = 10,
    S11 = 11,
    S12 = 12,
    S13 = 13,
    S14 = 14,
    S15 = 15,
    S16 = 16,
    S17 = 17,
    S18 = 18,
    S19 = 19,
    S20 = 20,
    S21 = 21,
    S22 = 22,
    S23 = 23,
    S24 = 24,
    S25 = 25,
    S26 = 26,
    S27 = 27,
    S28 = 28,
    S29 = 29,
    S30 = 30,
    S31 = 31,
    S32 = 32,
    S33 = 33,
    S34 = 34,
    S35 = 35,
    S36 = 36,
    S37 = 37,
    S38 = 38,
    S39 = 39,
    S40 = 40,
    S41 = 41,
    S42 = 42,
    S43 = 43,
    S44 = 44,
    S45 = 45,
    S46 = 46,
    S47 = 47,
    S48 = 48,
    S49 = 49,
    S50 = 50,
    S51 = 51,
    S52 = 52,
    S53 = 53,
    S54 = 54,
    S55 = 55,
    S56 = 56,
    S57 = 57,
    S58 = 58,
    S59 = 59,
    S60 = 60,
    S61 = 61,
    S62 = 62,
    S63 = 63,
    S64 = 64,
    S65 = 65,
    S66 = 66,
    S67 = 67,
    S68 = 68,
    S69 = 69,
    S70 = 70,
    S71 = 71,
    S72 = 72,
    S73 = 73,
    S74 = 74,
    S75 = 75,
    S76 = 76,
    S77 = 77,
    S78 = 78,
    S79 = 79,
    S80 = 80,
    S81 = 81,
    S82 = 82,
    S83 = 83,
    S84 = 84,
    S85 = 85,
    S86 = 86,
    S87 = 87,
    S88 = 88,
    S89 = 89,
    S90 = 90,
    S91 = 91,
    S92 = 92,
    S93 = 93,
    S94 = 94,
    S95 = 95,
    S96 = 96,
    S97 = 97,
    S98 = 98,
    S99 = 99,
    S100 = 100,
    S101 = 101,
    S102 = 102,
    S103 = 103,
    VccLo = 106,
    VccHi = 107,
    M0 = 124,
    ExecLo = 126,
    ExecHi = 127,

    Const0 = 128,
    Const1 = 129,
    Const2 = 130,
    Const3 = 131,
    Const4 = 132,
    Const5 = 133,
    Const6 = 134,
    Const7 = 135,
    Const8 = 136,
    Const9 = 137,
    Const10 = 138,
    Const11 = 139,
    Const12 = 140,
    Const13 = 141,
    Const14 = 142,
    Const15 = 143,
    Const16 = 144,
    Const17 = 145,
    Const18 = 146,
    Const19 = 147,
    Const20 = 148,
    Const21 = 149,
    Const22 = 150,
    Const23 = 151,
    Const24 = 152,
    Const25 = 153,
    Const26 = 154,
    Const27 = 155,
    Const28 = 156,
    Const29 = 157,
    Const30 = 158,
    Const31 = 159,
    Const32 = 160,
    Const33 = 161,
    Const34 = 162,
    Const35 = 163,
    Const36 = 164,
    Const37 = 165,
    Const38 = 166,
    Const39 = 167,
    Const40 = 168,
    Const41 = 169,
    Const42 = 170,
    Const43 = 171,
    Const44 = 172,
    Const45 = 173,
    Const46 = 174,
    Const47 = 175,
    Const48 = 176,
    Const49 = 177,
    Const50 = 178,
    Const51 = 179,
    Const52 = 180,
    Const53 = 181,
    Const54 = 182,
    Const55 = 183,
    Const56 = 184,
    Const57 = 185,
    Const58 = 186,
    Const59 = 187,
    Const60 = 188,
    Const61 = 189,
    Const62 = 190,
    Const63 = 191,
    Const64 = 192,
    ConstNeg1 = 193,
    ConstNeg2 = 194,
    ConstNeg3 = 195,
    ConstNeg4 = 196,
    ConstNeg5 = 197,
    ConstNeg6 = 198,
    ConstNeg7 = 199,
    ConstNeg8 = 200,
    ConstNeg9 = 201,
    ConstNeg10 = 202,
    ConstNeg11 = 203,
    ConstNeg12 = 204,
    ConstNeg13 = 205,
    ConstNeg14 = 206,
    ConstNeg15 = 207,
    ConstNeg16 = 208,

    Const0p5 = 240,
    ConstNeg0p5 = 241,
    Const1p0 = 242,
    ConstNeg1p0 = 243,
    Const2p0 = 244,
    ConstNeg2p0 = 245,
    Const4p0 = 246,
    ConstNeg4p0 = 247,

    ConstInv2Pi = 248,
    Sdwa = 249,
    Dpp = 250,
    Vccz = 251,
    Execz = 252,
    Scc = 253,
    LdsDirect = 254,
    LiteralConstant = 255
};

enum class SOperand9 : u16 {
    S0 = 0,
    S1 = 1,
    S2 = 2,
    S3 = 3,
    S4 = 4,
    S5 = 5,
    S6 = 6,
    S7 = 7,
    S8 = 8,
    S9 = 9,
    S10 = 10,
    S11 = 11,
    S12 = 12,
    S13 = 13,
    S14 = 14,
    S15 = 15,
    S16 = 16,
    S17 = 17,
    S18 = 18,
    S19 = 19,
    S20 = 20,
    S21 = 21,
    S22 = 22,
    S23 = 23,
    S24 = 24,
    S25 = 25,
    S26 = 26,
    S27 = 27,
    S28 = 28,
    S29 = 29,
    S30 = 30,
    S31 = 31,
    S32 = 32,
    S33 = 33,
    S34 = 34,
    S35 = 35,
    S36 = 36,
    S37 = 37,
    S38 = 38,
    S39 = 39,
    S40 = 40,
    S41 = 41,
    S42 = 42,
    S43 = 43,
    S44 = 44,
    S45 = 45,
    S46 = 46,
    S47 = 47,
    S48 = 48,
    S49 = 49,
    S50 = 50,
    S51 = 51,
    S52 = 52,
    S53 = 53,
    S54 = 54,
    S55 = 55,
    S56 = 56,
    S57 = 57,
    S58 = 58,
    S59 = 59,
    S60 = 60,
    S61 = 61,
    S62 = 62,
    S63 = 63,
    S64 = 64,
    S65 = 65,
    S66 = 66,
    S67 = 67,
    S68 = 68,
    S69 = 69,
    S70 = 70,
    S71 = 71,
    S72 = 72,
    S73 = 73,
    S74 = 74,
    S75 = 75,
    S76 = 76,
    S77 = 77,
    S78 = 78,
    S79 = 79,
    S80 = 80,
    S81 = 81,
    S82 = 82,
    S83 = 83,
    S84 = 84,
    S85 = 85,
    S86 = 86,
    S87 = 87,
    S88 = 88,
    S89 = 89,
    S90 = 90,
    S91 = 91,
    S92 = 92,
    S93 = 93,
    S94 = 94,
    S95 = 95,
    S96 = 96,
    S97 = 97,
    S98 = 98,
    S99 = 99,
    S100 = 100,
    S101 = 101,
    S102 = 102,
    S103 = 103,
    VccLo = 106,
    VccHi = 107,
    M0 = 124,
    ExecLo = 126,
    ExecHi = 127,

    Const0 = 128,
    Const1 = 129,
    Const2 = 130,
    Const3 = 131,
    Const4 = 132,
    Const5 = 133,
    Const6 = 134,
    Const7 = 135,
    Const8 = 136,
    Const9 = 137,
    Const10 = 138,
    Const11 = 139,
    Const12 = 140,
    Const13 = 141,
    Const14 = 142,
    Const15 = 143,
    Const16 = 144,
    Const17 = 145,
    Const18 = 146,
    Const19 = 147,
    Const20 = 148,
    Const21 = 149,
    Const22 = 150,
    Const23 = 151,
    Const24 = 152,
    Const25 = 153,
    Const26 = 154,
    Const27 = 155,
    Const28 = 156,
    Const29 = 157,
    Const30 = 158,
    Const31 = 159,
    Const32 = 160,
    Const33 = 161,
    Const34 = 162,
    Const35 = 163,
    Const36 = 164,
    Const37 = 165,
    Const38 = 166,
    Const39 = 167,
    Const40 = 168,
    Const41 = 169,
    Const42 = 170,
    Const43 = 171,
    Const44 = 172,
    Const45 = 173,
    Const46 = 174,
    Const47 = 175,
    Const48 = 176,
    Const49 = 177,
    Const50 = 178,
    Const51 = 179,
    Const52 = 180,
    Const53 = 181,
    Const54 = 182,
    Const55 = 183,
    Const56 = 184,
    Const57 = 185,
    Const58 = 186,
    Const59 = 187,
    Const60 = 188,
    Const61 = 189,
    Const62 = 190,
    Const63 = 191,
    Const64 = 192,
    ConstNeg1 = 193,
    ConstNeg2 = 194,
    ConstNeg3 = 195,
    ConstNeg4 = 196,
    ConstNeg5 = 197,
    ConstNeg6 = 198,
    ConstNeg7 = 199,
    ConstNeg8 = 200,
    ConstNeg9 = 201,
    ConstNeg10 = 202,
    ConstNeg11 = 203,
    ConstNeg12 = 204,
    ConstNeg13 = 205,
    ConstNeg14 = 206,
    ConstNeg15 = 207,
    ConstNeg16 = 208,

    Const0p5 = 240,
    ConstNeg0p5 = 241,
    Const1p0 = 242,
    ConstNeg1p0 = 243,
    Const2p0 = 244,
    ConstNeg2p0 = 245,
    Const4p0 = 246,
    ConstNeg4p0 = 247,

    ConstInv2Pi = 248,
    Sdwa = 249,
    Dpp = 250,
    Vccz = 251,
    Execz = 252,
    Scc = 253,
    LdsDirect = 254,
    LiteralConstant = 255,

    V0 = 256,
    V1 = 257,
    V2 = 258,
    V3 = 259,
    V4 = 260,
    V5 = 261,
    V6 = 262,
    V7 = 263,
    V8 = 264,
    V9 = 265,
    V10 = 266,
    V11 = 267,
    V12 = 268,
    V13 = 269,
    V14 = 270,
    V15 = 271,
    V16 = 272,
    V17 = 273,
    V18 = 274,
    V19 = 275,
    V20 = 276,
    V21 = 277,
    V22 = 278,
    V23 = 279,
    V24 = 280,
    V25 = 281,
    V26 = 282,
    V27 = 283,
    V28 = 284,
    V29 = 285,
    V30 = 286,
    V31 = 287,
    V32 = 288,
    V33 = 289,
    V34 = 290,
    V35 = 291,
    V36 = 292,
    V37 = 293,
    V38 = 294,
    V39 = 295,
    V40 = 296,
    V41 = 297,
    V42 = 298,
    V43 = 299,
    V44 = 300,
    V45 = 301,
    V46 = 302,
    V47 = 303,
    V48 = 304,
    V49 = 305,
    V50 = 306,
    V51 = 307,
    V52 = 308,
    V53 = 309,
    V54 = 310,
    V55 = 311,
    V56 = 312,
    V57 = 313,
    V58 = 314,
    V59 = 315,
    V60 = 316,
    V61 = 317,
    V62 = 318,
    V63 = 319,
    V64 = 320,
    V65 = 321,
    V66 = 322,
    V67 = 323,
    V68 = 324,
    V69 = 325,
    V70 = 326,
    V71 = 327,
    V72 = 328,
    V73 = 329,
    V74 = 330,
    V75 = 331,
    V76 = 332,
    V77 = 333,
    V78 = 334,
    V79 = 335,
    V80 = 336,
    V81 = 337,
    V82 = 338,
    V83 = 339,
    V84 = 340,
    V85 = 341,
    V86 = 342,
    V87 = 343,
    V88 = 344,
    V89 = 345,
    V90 = 346,
    V91 = 347,
    V92 = 348,
    V93 = 349,
    V94 = 350,
    V95 = 351,
    V96 = 352,
    V97 = 353,
    V98 = 354,
    V99 = 355,
    V100 = 356,
    V101 = 357,
    V102 = 358,
    V103 = 359,
    V104 = 360,
    V105 = 361,
    V106 = 362,
    V107 = 363,
    V108 = 364,
    V109 = 365,
    V110 = 366,
    V111 = 367,
    V112 = 368,
    V113 = 369,
    V114 = 370,
    V115 = 371,
    V116 = 372,
    V117 = 373,
    V118 = 374,
    V119 = 375,
    V120 = 376,
    V121 = 377,
    V122 = 378,
    V123 = 379,
    V124 = 380,
    V125 = 381,
    V126 = 382,
    V127 = 383,
    V128 = 384,
    V129 = 385,
    V130 = 386,
    V131 = 387,
    V132 = 388,
    V133 = 389,
    V134 = 390,
    V135 = 391,
    V136 = 392,
    V137 = 393,
    V138 = 394,
    V139 = 395,
    V140 = 396,
    V141 = 397,
    V142 = 398,
    V143 = 399,
    V144 = 400,
    V145 = 401,
    V146 = 402,
    V147 = 403,
    V148 = 404,
    V149 = 405,
    V150 = 406,
    V151 = 407,
    V152 = 408,
    V153 = 409,
    V154 = 410,
    V155 = 411,
    V156 = 412,
    V157 = 413,
    V158 = 414,
    V159 = 415,
    V160 = 416,
    V161 = 417,
    V162 = 418,
    V163 = 419,
    V164 = 420,
    V165 = 421,
    V166 = 422,
    V167 = 423,
    V168 = 424,
    V169 = 425,
    V170 = 426,
    V171 = 427,
    V172 = 428,
    V173 = 429,
    V174 = 430,
    V175 = 431,
    V176 = 432,
    V177 = 433,
    V178 = 434,
    V179 = 435,
    V180 = 436,
    V181 = 437,
    V182 = 438,
    V183 = 439,
    V184 = 440,
    V185 = 441,
    V186 = 442,
    V187 = 443,
    V188 = 444,
    V189 = 445,
    V190 = 446,
    V191 = 447,
    V192 = 448,
    V193 = 449,
    V194 = 450,
    V195 = 451,
    V196 = 452,
    V197 = 453,
    V198 = 454,
    V199 = 455,
    V200 = 456,
    V201 = 457,
    V202 = 458,
    V203 = 459,
    V204 = 460,
    V205 = 461,
    V206 = 462,
    V207 = 463,
    V208 = 464,
    V209 = 465,
    V210 = 466,
    V211 = 467,
    V212 = 468,
    V213 = 469,
    V214 = 470,
    V215 = 471,
    V216 = 472,
    V217 = 473,
    V218 = 474,
    V219 = 475,
    V220 = 476,
    V221 = 477,
    V222 = 478,
    V223 = 479,
    V224 = 480,
    V225 = 481,
    V226 = 482,
    V227 = 483,
    V228 = 484,
    V229 = 485,
    V230 = 486,
    V231 = 487,
    V232 = 488,
    V233 = 489,
    V234 = 490,
    V235 = 491,
    V236 = 492,
    V237 = 493,
    V238 = 494,
    V239 = 495,
    V240 = 496,
    V241 = 497,
    V242 = 498,
    V243 = 499,
    V244 = 500,
    V245 = 501,
    V246 = 502,
    V247 = 503,
    V248 = 504,
    V249 = 505,
    V250 = 506,
    V251 = 507,
    V252 = 508,
    V253 = 509,
    V254 = 510,
    V255 = 511
};

class SOP1 {
public:
    explicit constexpr SOP1(OpcodeSOP1 op, SOperand7 sdst, SOperand8 ssrc0) {
        i.ssrc0 = std::to_underlying(ssrc0);
        i.sdst = std::to_underlying(sdst);
        i.op = std::to_underlying(op);
        i.encoding = 0b101111101;
    }

    u32 Get() {
        return std::bit_cast<u32>(i);
    }

private:
    struct SOP1Internal {
        u32 ssrc0 : 8;
        u32 op : 8;
        u32 sdst : 7;
        u32 encoding : 9;
    } i;

    static_assert(sizeof(SOP1Internal) == sizeof(u32));
};

class SOP2 {
public:
    explicit constexpr SOP2(OpcodeSOP2 op, SOperand7 sdst, SOperand8 ssrc0, SOperand8 ssrc1) {
        i.ssrc0 = std::to_underlying(ssrc0);
        i.ssrc1 = std::to_underlying(ssrc1);
        i.sdst = std::to_underlying(sdst);
        i.op = std::to_underlying(op);
        i.encoding = 0b10;
    }

    u32 Get() {
        return std::bit_cast<u32>(i);
    }

private:
    struct SOP2Internal {
        u32 ssrc0 : 8;
        u32 ssrc1 : 8;
        u32 sdst : 7;
        u32 op : 7;
        u32 encoding : 2;
    } i;

    static_assert(sizeof(SOP2Internal) == sizeof(u32));
};

class SOPK {
public:
    explicit constexpr SOPK(OpcodeSOPK op, SOperand7 sdst, u16 imm) {
        i.imm = imm;
        i.sdst = std::to_underlying(sdst);
        i.op = std::to_underlying(op);
        i.encoding = 0b1011;
    }

    u32 Get() {
        return std::bit_cast<u32>(i);
    }

private:
    struct SOPKInternal {
        u32 imm : 16;
        u32 sdst : 7;
        u32 op : 5;
        u32 encoding : 4;
    } i;

    static_assert(sizeof(SOPKInternal) == sizeof(u32));
};

class VOP1 {
public:
    explicit constexpr VOP1(OpcodeVOP1 op, VOperand8 vdst, SOperand9 src0) {
        i.src0 = std::to_underlying(src0);
        i.vdst = std::to_underlying(vdst);
        i.op = std::to_underlying(op);
        i.encoding = 0b0111111;
    }

    u32 Get() {
        return std::bit_cast<u32>(i);
    }

private:
    struct VOP1Internal {
        u32 src0 : 9;
        u32 op : 8;
        u32 vdst : 8;
        u32 encoding : 7;
    } i;

    static_assert(sizeof(VOP1Internal) == sizeof(u32));
};

class VOP2 {
public:
    explicit constexpr VOP2(OpcodeVOP2 op, VOperand8 vdst, SOperand9 src0, VOperand8 vsrc1) {
        i.src0 = std::to_underlying(src0);
        i.vsrc1 = std::to_underlying(vsrc1);
        i.vdst = std::to_underlying(vdst);
        i.op = std::to_underlying(op);
        i.encoding = 0;
    }

    u32 Get() {
        return std::bit_cast<u32>(i);
    }

private:
    struct VOP2Internal {
        u32 src0 : 9;
        u32 vsrc1 : 8;
        u32 vdst : 8;
        u32 op : 6;
        u32 encoding : 1;
    } i;

    static_assert(sizeof(VOP2Internal) == sizeof(u32));
};

enum class Omod : u8 {
    None = 0,
    Mul2 = 1,
    Mul4 = 2,
    Div2 = 3,
};

class VOP3A {
public:
    explicit constexpr VOP3A(OpcodeVOP3 op, VOperand8 vdst, SOperand9 src0, SOperand9 src1, SOperand9 src2 = SOperand9::S0) {
        i.src0 = std::to_underlying(src0);
        i.src1 = std::to_underlying(src1);
        i.src2 = std::to_underlying(src2);
        i.vdst = std::to_underlying(vdst);
        i.op = std::to_underlying(op) & 0x1FF;
        i.op_msb = std::to_underlying(op) >> 9;
        i.encoding = 0b110100;
    }

    VOP3A& SetAbs(const std::array<bool, 3>& abs) {
        u8 a = 0;
        for (auto x : abs | std::views::reverse) {
            a |= x;
            a <<= 1;
        }
        i.abs = a >> 1;
        return *this;
    }

    VOP3A& SetClamp(bool clamp) {
        i.clmp = clamp;
        return *this;
    }

    VOP3A& SetNeg(const std::array<bool, 3>& neg) {
        u8 n = 0;
        for (auto x : neg | std::views::reverse) {
            n |= x;
            n <<= 1;
        }
        i.neg = n >> 1;
        return *this;
    }

    VOP3A& SetOmod(Omod omod) {
        i.omod = std::to_underlying(omod);
        return *this;
    }

    VOP3A& SetOpSel(const std::array<bool, 4>& op_sel) {
        u8 o = 0;
        for (auto x : op_sel | std::views::reverse) {
            o |= x;
            o <<= 1;
        }
        i.op_sel = o >> 1;
        return *this;
    }

    u64 Get() {
        return std::bit_cast<u64>(i);
    }

private:
    struct VOP3Internal {
        u64 vdst : 8;
        u64 abs : 3;
        u64 clmp : 1;
        u64 op_sel : 4;
        u64 op_msb : 1;
        u64 op : 9;
        u64 encoding : 6;
        u64 src0 : 9;
        u64 src1 : 9;
        u64 src2 : 9;
        u64 omod : 2;
        u64 neg : 3;
    } i;

    static_assert(sizeof(VOP3Internal) == sizeof(u64));
};

class VOP3P {
public:
    explicit constexpr VOP3P(OpcodeVOP3P op, VOperand8 vdst, SOperand9 src0, SOperand9 src1, SOperand9 src2 = SOperand9::S0) {
        i.src0 = std::to_underlying(src0);
        i.src1 = std::to_underlying(src1);
        i.src2 = std::to_underlying(src2);
        i.vdst = std::to_underlying(vdst);
        i.op = std::to_underlying(op);
        // enable op_sel_hi as a sensible default, so both 16-bit chunks are processed
        i.op_sel_hi01 = 0b11;
        i.op_sel_hi2 = 0b1;
        i.encoding = 0b110011;
    }

    VOP3P& SetClamp(bool clamp) {
        i.clmp = clamp;
        return *this;
    }

    VOP3P& SetNeg(const std::array<bool, 3>& neg) {
        u8 n = 0;
        for (auto x : neg | std::views::reverse) {
            n |= x;
            n <<= 1;
        }
        i.neg = n >> 1;
        return *this;
    }

    VOP3P& SetNegHi(const std::array<bool, 3>& neg) {
        u8 n = 0;
        for (auto x : neg | std::views::reverse) {
            n |= x;
            n <<= 1;
        }
        i.neg_hi = n >> 1;
        return *this;
    }

    VOP3P& SetOpSel(const std::array<bool, 3>& op_sel) {
        u8 o = 0;
        for (auto x : op_sel | std::views::reverse) {
            o |= x;
            o <<= 1;
        }
        i.op_sel = o >> 1;
        return *this;
    }

    VOP3P& SetOpSelHi(const std::array<bool, 3>& op_sel) {
        i.op_sel_hi2 = op_sel[2];
        i.op_sel_hi01 = (op_sel[1] << 1) | op_sel[0];
        return *this;
    }

    u64 Get() {
        return std::bit_cast<u64>(i);
    }

private:
    struct VOP3PInternal {
        u64 vdst : 8;
        u64 neg_hi : 3;
        u64 op_sel : 3;
        u64 op_sel_hi2 : 1;
        u64 clmp : 1;
        u64 op : 7;
        u64 : 3;
        u64 encoding : 6;
        u64 src0 : 9;
        u64 src1 : 9;
        u64 src2 : 9;
        u64 op_sel_hi01 : 2;
        u64 neg : 3;
    } i;

    static_assert(sizeof(VOP3PInternal) == sizeof(u64));
};
