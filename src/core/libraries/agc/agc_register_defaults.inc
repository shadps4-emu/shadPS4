// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

static RegisterDefaultInfo g_cx_reg_info1[] = {
    /* 0 */ {0xE24F806D, {{Cx(mmCB_COLOR_CONTROL), 0x00cc0010}}},
    /* 1 */ {0xF6C28182, {{Cx(mmCB_DCC_CONTROL__VI), 0x00000000}}},
    /* 2 */ {0x6F6E55A5, {{0x00000104u, 0x00000000}}},
    /* 3 */ {0x0BC65DA4, {{Cx(mmCB_SHADER_MASK), 0x00000000}}},
    /* 4 */ {0x9E5AD592, {{Cx(mmCB_TARGET_MASK), 0x00000000}}},
    /* 5 */ {0xBB513B98, {{Cx(mmDB_ALPHA_TO_MASK), 0x0000aa00}}},
    /* 6 */ {0xAB64B23B, {{Cx(mmDB_COUNT_CONTROL), 0x00000000}}},
    /* 7 */ {0x53C39964, {{Cx(mmDB_DEPTH_CONTROL), 0x00000000}}},
    /* 8 */ {0x01396B11, {{Cx(mmDB_EQAA), 0x00000000}}},
    /* 9 */ {0x7D42019A, {{Cx(mmDB_RENDER_CONTROL), 0x00000000}}},
    /* 10 */ {0x3548F523, {{0x00000006u, 0x00000000}}},
    /* 11 */ {0xF43AD28A, {{0x0000001Fu, 0x00000000}}},
    /* 12 */ {0x6DE4C312, {{Cx(mmDB_SHADER_CONTROL), 0x00000000}}},
    /* 13 */ {0x00A77AE0, {{Cx(mmDB_SRESULTS_COMPARE_STATE0), 0x00000000}}},
    /* 14 */ {0x00A779B7, {{Cx(mmDB_SRESULTS_COMPARE_STATE1), 0x00000000}}},
    /* 15 */ {0x5100100C, {{Cx(mmDB_STENCILREFMASK), 0x00000000}}},
    /* 16 */ {0x59958BBA, {{Cx(mmDB_STENCILREFMASK_BF), 0x00000000}}},
    /* 17 */ {0x0C06F17C, {{Cx(mmDB_STENCIL_CONTROL), 0x00000000}}},
    /* 18 */ {0x6F104B72, {{0x000001FFu, 0x00000000}}},
    /* 19 */ {0x25C70D9C, {{Cx(mmPA_CL_CLIP_CNTL), 0x00000000}}},
    /* 20 */ {0x3881201E, {{0x0000020Du, 0x00000000}}},
    /* 21 */ {0x09AFDDAF, {{Cx(mmPA_CL_VTE_CNTL), 0x0000043f}}},
    /* 22 */ {0x367D63CF, {{Cx(mmPA_SC_AA_CONFIG), 0x00000000}}},
    /* 23 */ {0x43707DB8, {{Cx(mmPA_SC_CLIPRECT_RULE), 0x0000ffff}}},
    /* 24 */ {0xF6AE26BA, {{0x00000313u, 0x00000000}}},
    /* 25 */ {0x1B917652, {{0x800003FEu, 0x00000000}}},
    /* 26 */ {0x94B1E4F7, {{0x000000EAu, 0x00000000}}},
    /* 27 */ {0xE3661B6C, {{0x000000E9u, 0x00000000}}},
    /* 28 */ {0x1EB8D73A, {{Cx(mmPA_SC_MODE_CNTL_0), 0x00000002}}},
    /* 29 */ {0x15051FA3, {{Cx(mmPA_SC_MODE_CNTL_1), 0x00000000}}},
    /* 30 */ {0x9C51A7F1, {{0x000000E8u, 0x00000000}}},
    /* 31 */ {0xA20EFC70, {{Cx(mmPA_SC_WINDOW_OFFSET), 0x00000000}}},
    /* 32 */ {0x0EC09F6E, {{0x00000211u, 0x00000000}}},
    /* 33 */ {0x34A7D6D3, {{0x00000210u, 0x00000000}}},
    /* 34 */ {0xCE831B94, {{Cx(mmPA_SU_HARDWARE_SCREEN_OFFSET), 0x00000000}}},
    /* 35 */ {0x5CC72A74, {{Cx(mmPA_SU_LINE_CNTL), 0x00000008}}},
    /* 36 */ {0x3B77713C, {{Cx(mmPA_SU_POINT_MINMAX), 0xffff0000}}},
    /* 37 */ {0x40F64410, {{Cx(mmPA_SU_POINT_SIZE), 0x00080008}}},
    /* 38 */ {0x69441268, {{Cx(mmPA_SU_POLY_OFFSET_CLAMP), 0x00000000}}},
    /* 39 */ {0x2E418B83, {{Cx(mmPA_SU_POLY_OFFSET_DB_FMT_CNTL), 0x000001e9}}},
    /* 40 */ {0xA00D0C8D, {{Cx(mmPA_SU_SC_MODE_CNTL), 0x00000240}}},
    /* 41 */ {0xB1289FB3, {{0x0000020Cu, 0x00000001}}},
    /* 42 */ {0x144832FB, {{Cx(mmPA_SU_VTX_CNTL), 0x0000002d}}},
    /* 43 */ {0x9890D9FA, {{Cx(mmSPI_TMPRING_SIZE), 0x00000000}}},
    /* 44 */ {0x9016FAF1, {{0x000002A6u, 0x00000000}}},
    /* 45 */ {0x4B73CE27, {{Cx(mmVGT_GS_MAX_VERT_OUT), 0x00000400}}},
    /* 46 */ {0x5F5A3E7B, {{Cx(mmVGT_GS_OUT_PRIM_TYPE), 0x00000002}}},
    /* 47 */ {0xD4AF3A51, {{Cx(mmVGT_LS_HS_CONFIG), 0x00000000}}},
    /* 48 */ {0x6CF4F543, {{Cx(mmVGT_PRIMITIVEID_RESET), 0xffffffff}}},
    /* 49 */ {0x5FB86CCB, {{Cx(mmVGT_PRIMITIVEID_EN), 0x00000000}}},
    /* 50 */ {0xEDEFA188, {{Cx(mmVGT_REUSE_OFF), 0x00000000}}},
    /* 51 */ {0xD0DE9EE6, {{Cx(mmVGT_SHADER_STAGES_EN), 0x00000000}}},
    /* 52 */ {0xC5831803, {{Cx(mmVGT_TESS_DISTRIBUTION__VI), 0x88101000}}},
    /* 53 */ {0x8E6DE84B, {{Cx(mmVGT_TF_PARAM), 0x00000000}}},
    /* 54 */
    {0xD0771662,
     {
         {Cx(mmPA_SC_CENTROID_PRIORITY_0), 0x00000000},
         {Cx(mmPA_SC_CENTROID_PRIORITY_1), 0x00000000},
     }},
    /* 55 */ {0x569F7444, {{Cx(mmPA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0), 0x00000000}}},
    /* 56 */
    {0x5C6637CD,
     {
         {Cx(mmPA_SC_AA_MASK_X0Y0_X1Y0), 0xffffffff},
         {Cx(mmPA_SC_AA_MASK_X0Y1_X1Y1), 0xffffffff},
     }},
    /* 57 */
    {0xCAE3E690,
     {
         {0x00000311u, 0x00000002},
         {0x00000312u, 0x03ff0080},
     }},
    /* 58 */
    {0x43FBD769,
     {
         {Cx(mmCB_BLEND_RED), 0x00000000},
         {Cx(mmCB_BLEND_BLUE), 0x00000000},
         {Cx(mmCB_BLEND_GREEN), 0x00000000},
         {Cx(mmCB_BLEND_ALPHA), 0x00000000},
     }},
    /* 59 */ {0xEF550356, {{Cx(mmCB_BLEND0_CONTROL), 0x20010001}}},
    /* 60 */
    {0x8F52E279,
     {
         {Cx(mmTA_BC_BASE_ADDR), 0x00000000},
         {Cx(mmTA_BC_BASE_ADDR_HI__CI__VI), 0x00000000},
     }},
    /* 61 */
    {0x1F2D8149,
     {
         {Cx(mmPA_SC_CLIPRECT_0_TL), 0x00000000},
         {Cx(mmPA_SC_CLIPRECT_0_BR), 0x20002000},
     }},
    /* 62 */ {0x853D0614, {{0x800003FFu, 0x00000000}}},
    /* 63 */
    {0x4413C6F9,
     {
         {Cx(mmDB_DEPTH_BOUNDS_MIN), 0x00000000},
         {Cx(mmDB_DEPTH_BOUNDS_MAX), 0x00000000},
     }},
    /* 64 */
    {0x67096014,
     {
         {Cx(mmDB_Z_INFO), 0x80000000},
         {Cx(mmDB_STENCIL_INFO), 0x20000000},
         {Cx(mmDB_Z_READ_BASE), 0x00000000},
         {Cx(mmDB_STENCIL_READ_BASE), 0x00000000},
         {Cx(mmDB_Z_WRITE_BASE), 0x00000000},
         {Cx(mmDB_STENCIL_WRITE_BASE), 0x00000000},
         {0x0000001Au, 0x00000000},
         {0x0000001Bu, 0x00000000},
         {0x0000001Cu, 0x00000000},
         {0x0000001Du, 0x00000000},
         {0x0000001Eu, 0x00000000},
         {Cx(mmDB_DEPTH_VIEW), 0x00000000},
         {Cx(mmDB_HTILE_DATA_BASE), 0x00000000},
         {0x00000007u, 0x00000000},
         {Cx(mmDB_DEPTH_CLEAR), 0x00000000},
         {Cx(mmDB_STENCIL_CLEAR), 0x00000000},
     }},
    /* 65 */
    {0x88F5E915,
     {
         {0x000000EBu, 0xff00ff00},
         {0x000000ECu, 0x00000000},
     }},
    /* 66 */
    {0x033F1EFF,
     {
         {0x800003FCu, 0x00000000},
         {0x800003FDu, 0x00000000},
     }},
    /* 67 */
    {0x918106BB,
     {
         {Cx(mmPA_SC_GENERIC_SCISSOR_TL), 0x80000000},
         {Cx(mmPA_SC_GENERIC_SCISSOR_BR), 0x40004000},
     }},
    /* 68 */
    {0x95F0E7AC,
     {
         {Cx(mmPA_CL_GB_VERT_CLIP_ADJ), 0x4e7e0000},
         {Cx(mmPA_CL_GB_VERT_DISC_ADJ), 0x4e7e0000},
         {Cx(mmPA_CL_GB_HORZ_CLIP_ADJ), 0x4e7e0000},
         {Cx(mmPA_CL_GB_HORZ_DISC_ADJ), 0x4e7e0000},
     }},
    /* 69 */
    {0xB48CBAB2,
     {
         {Cx(mmPA_SU_POLY_OFFSET_BACK_SCALE), 0x00000000},
         {Cx(mmPA_SU_POLY_OFFSET_BACK_OFFSET), 0x00000000},
     }},
    /* 70 */
    {0x05BB3BC6,
     {
         {Cx(mmPA_SU_POLY_OFFSET_FRONT_SCALE), 0x00000000},
         {Cx(mmPA_SU_POLY_OFFSET_FRONT_OFFSET), 0x00000000},
     }},
    /* 71 */
    {0x94FABA07,
     {
         {Cx(mmDB_RENDER_OVERRIDE), 0x00000000},
         {Cx(mmDB_RENDER_OVERRIDE2), 0x00000000},
     }},
    /* 72 */
    {0x38E92C91,
     {
         {Cx(mmCB_COLOR0_BASE), 0x00000000},
         {Cx(mmCB_COLOR0_VIEW), 0x00000000},
         {Cx(mmCB_COLOR0_INFO), 0x00000000},
         {Cx(mmCB_COLOR0_ATTRIB), 0x00000000},
         {Cx(mmCB_COLOR0_DCC_CONTROL__VI), 0x00000048},
         {Cx(mmCB_COLOR0_CMASK), 0x00000000},
         {Cx(mmCB_COLOR0_FMASK), 0x00000000},
         {Cx(mmCB_COLOR0_CLEAR_WORD0), 0x00000000},
         {Cx(mmCB_COLOR0_CLEAR_WORD1), 0x00000000},
         {Cx(mmCB_COLOR0_DCC_BASE__VI), 0x00000000},
         {0x00000390u, 0x00000000},
         {0x00000398u, 0x00000000},
         {0x000003A0u, 0x00000000},
         {0x000003A8u, 0x00000000},
         {0x000003B0u, 0x00000000},
         {0x000003B8u, 0x0006c000},
     }},
    /* 73 */
    {0x0B177B43,
     {
         {Cx(mmPA_SC_SCREEN_SCISSOR_TL), 0x00000000},
         {Cx(mmPA_SC_SCREEN_SCISSOR_BR), 0x40004000},
     }},
    /* 74 */ {0x48531062, {{Cx(mmSPI_PS_INPUT_CNTL_0), 0x00000000}}},
    /* 75 */
    {0xAAA964B9,
     {
         {Cx(mmPA_CL_UCP_0_X), 0x00000000},
         {Cx(mmPA_CL_UCP_0_Y), 0x00000000},
         {Cx(mmPA_CL_UCP_0_Z), 0x00000000},
         {Cx(mmPA_CL_UCP_0_W), 0x00000000},
     }},
    /* 76 */
    {0x7690AF6F,
     {
         {Cx(mmPA_CL_VPORT_XSCALE), 0x4e7e0000},
         {Cx(mmPA_CL_VPORT_YSCALE), 0x4e7e0000},
         {Cx(mmPA_CL_VPORT_ZSCALE), 0x4e7e0000},
         {Cx(mmPA_CL_VPORT_XOFFSET), 0x00000000},
         {Cx(mmPA_CL_VPORT_YOFFSET), 0x00000000},
         {Cx(mmPA_CL_VPORT_ZOFFSET), 0x00000000},
         {Cx(mmPA_SC_VPORT_SCISSOR_0_TL), 0x80000000},
         {Cx(mmPA_SC_VPORT_SCISSOR_0_BR), 0x40004000},
         {Cx(mmPA_SC_VPORT_ZMIN_0), 0x00000000},
         {Cx(mmPA_SC_VPORT_ZMAX_0), 0x00000000},
     }},
    /* 77 */
    {0x078D7060,
     {
         {Cx(mmPA_SC_WINDOW_SCISSOR_TL), 0x80000000},
         {Cx(mmPA_SC_WINDOW_SCISSOR_BR), 0x40004000},
     }},
};

static RegisterDefaultInfo g_sh_reg_info1[] = {
    /* 0 */ {0x5D6E3EC7, {{Sh(mmCOMPUTE_PGM_RSRC1), 0x00000000}}},
    /* 1 */ {0x57E7079A, {{Sh(mmCOMPUTE_PGM_RSRC2), 0x00000000}}},
    /* 2 */ {0x7467FAFD, {{0x00000228u, 0x00000000}}},
    /* 3 */ {0x9E826B50, {{Sh(mmCOMPUTE_RESOURCE_LIMITS), 0x00000000}}},
    /* 4 */ {0xDC484F18, {{Sh(mmCOMPUTE_TMPRING_SIZE), 0x00000000}}},
    /* 5 */ {0x5DA8BCA3, {{Sh(mmSPI_SHADER_PGM_RSRC1_GS), 0x00000000}}},
    /* 6 */ {0x5CA726D8, {{Sh(mmSPI_SHADER_PGM_RSRC1_HS), 0x00000000}}},
    /* 7 */ {0x5DD28360, {{Sh(mmSPI_SHADER_PGM_RSRC1_PS), 0x00000000}}},
    /* 8 */ {0x57EFA0BE, {{Sh(mmSPI_SHADER_PGM_RSRC2_GS), 0x00000000}}},
    /* 9 */ {0x502363D5, {{Sh(mmSPI_SHADER_PGM_RSRC2_HS), 0x00000000}}},
    /* 10 */ {0x506D14BD, {{Sh(mmSPI_SHADER_PGM_RSRC2_PS), 0x00000000}}},
    /* 11 */ {0xB2609506, {{0x00000224u, 0x00000000}}},
    /* 12 */
    {0x9E5CFB8A,
     {
         {Sh(mmSPI_SHADER_PGM_RSRC3_HS__CI__VI), 0x00000000},
         {Sh(mmSPI_SHADER_PGM_RSRC3_GS__CI__VI), 0x00000000},
         {Sh(mmSPI_SHADER_PGM_RSRC3_PS__CI__VI), 0x00000000},
     }},
    /* 13 */
    {0xC918DF3E,
     {
         {Sh(mmCOMPUTE_PGM_LO), 0x00000000},
         {Sh(mmCOMPUTE_PGM_HI), 0x00000000},
     }},
    /* 14 */
    {0xC9751C9C,
     {
         {Sh(mmSPI_SHADER_PGM_LO_ES), 0x00000000},
         {Sh(mmSPI_SHADER_PGM_HI_ES), 0x00000000},
     }},
    /* 15 */
    {0xC97EF77A,
     {
         {Sh(mmSPI_SHADER_PGM_LO_GS), 0x00000000},
         {Sh(mmSPI_SHADER_PGM_HI_GS), 0x00000000},
     }},
    /* 16 */
    {0xC927C6B9,
     {
         {Sh(mmSPI_SHADER_PGM_LO_HS), 0x00000000},
         {Sh(mmSPI_SHADER_PGM_HI_HS), 0x00000000},
     }},
    /* 17 */
    {0xC92A1EC5,
     {
         {Sh(mmSPI_SHADER_PGM_LO_LS), 0x00000000},
         {Sh(mmSPI_SHADER_PGM_HI_LS), 0x00000000},
     }},
    /* 18 */
    {0xC9E01B31,
     {
         {Sh(mmSPI_SHADER_PGM_LO_PS), 0x00000000},
         {Sh(mmSPI_SHADER_PGM_HI_PS), 0x00000000},
     }},
    /* 19 */ {0x50685F29, {{0x800002FFu, 0x00000000}}},
    /* 20 */ {0xB26219CA, {{0x000000B2u, 0x00000000}}},
    /* 21 */ {0xB25B6CF9, {{0x00000132u, 0x00000000}}},
    /* 22 */ {0xB2F86101, {{0x00000032u, 0x00000000}}},
    /* 23 */
    {0x07E3B155,
     {
         {0x00000082u, 0x00000000},
         {0x00000083u, 0x00000000},
     }},
    /* 24 */
    {0x07E383C6,
     {
         {0x00000102u, 0x00000000},
         {0x00000103u, 0x00000000},
     }},
    /* 25 */ {0xBDA98653, {{Sh(mmCOMPUTE_USER_DATA_0), 0x00000000}}},
    /* 26 */ {0xBDBD1D0F, {{Sh(mmSPI_SHADER_USER_DATA_GS_0), 0x00000000}}},
    /* 27 */ {0xBD946FD4, {{Sh(mmSPI_SHADER_USER_DATA_HS_0), 0x00000000}}},
    /* 28 */ {0xBDF02A4C, {{Sh(mmSPI_SHADER_USER_DATA_PS_0), 0x00000000}}},
};

static RegisterDefaultInfo g_uc_reg_info1[] = {
    /* 0 */ {0x19E93E85, {{Uc(mmGDS_OA_ADDRESS__CI__VI), 0x00000000}}},
    /* 1 */ {0x3B5C2AF3, {{Uc(mmGDS_OA_CNTL__CI__VI), 0x00000000}}},
    /* 2 */ {0x47974A35, {{Uc(mmGDS_OA_COUNTER__CI__VI), 0x00000000}}},
    /* 3 */ {0x105971C2, {{0x0000025Bu, 0x00000000}}},
    /* 4 */ {0x7D137765, {{0x0000024Au, 0x00000000}}},
    /* 5 */ {0xD187FEBC, {{0x0000024Bu, 0x00000000}}},
    /* 6 */ {0x12F854AC, {{0x0000025Fu, 0x00000000}}},
    /* 7 */ {0x40D49AD1, {{0x00000262u, 0x00000000}}},
    /* 8 */ {0x8C0923DA, {{0x80003FF4u, 0x00000000}}},
    /* 9 */ {0xBB8DF494, {{0x80003FFDu, 0x00000000}}},
    /* 10 */ {0xF6D8A76E, {{0x00000382u, 0x40000040}}},
    /* 11 */ {0x7620F1E9, {{0x00000248u, 0x00000000}}},
    /* 12 */ {0x9EBFAB10, {{Uc(mmVGT_PRIMITIVE_TYPE__CI__VI), 0x00000000}}},
    /* 13 */
    {0x98A09D0E,
     {
         {Uc(mmTA_CS_BC_BASE_ADDR__CI__VI), 0x00000000},
         {Uc(mmTA_CS_BC_BASE_ADDR_HI__CI__VI), 0x00000000},
     }},
    /* 14 */
    {0x195D37D2,
     {
         {0x80003FF5u, 0x00000000},
         {0x80003FF6u, 0x00000000},
     }},
    /* 15 */
    {0xF9EC4F85,
     {
         {0x80003FF7u, 0x00000000},
         {0x80003FF8u, 0x00000000},
         {0x80003FF9u, 0x00000000},
         {0x80003FFAu, 0x00000000},
     }},
    /* 16 */
    {0x4626B750,
     {
         {0x80003FFBu, 0x00000000},
         {0x80003FFCu, 0x00000000},
     }},
    /* 17 */ {0x4CC673A0, {{0x80003FFEu, 0x00000000}}},
    /* 18 */ {0xDE5B3431, {{0x80003FFFu, 0x00000000}}},
    /* 19 */ {0x036AC8A6, {{0x0000025Cu, 0x00000000}}},
};

static RegisterDefaultInfo g_cx_reg_info2[] = {
    /* 0 */ {0x8FB4EDB5, {{0x0000000Eu, 0x00000000}}},
    /* 1 */ {0xB994AD29, {{Cx(mmDB_HTILE_SURFACE), 0x00000000}}},
    /* 2 */ {0xD427322F, {{0x00000314u, 0x00000000}}},
    /* 3 */ {0xF58FEA31, {{Cx(mmSPI_INTERP_CONTROL_0), 0x00000000}}},
};

static RegisterDefaultInfo g_sh_reg_info2[] = {
    /* 0 */ {0x6AC156EF, {{0x00000216u, 0x00000000}}},
    /* 1 */ {0x6AC15610, {{0x00000217u, 0x00000000}}},
    /* 2 */ {0x6AC15009, {{0x00000219u, 0x00000000}}},
    /* 3 */ {0x6AC153BA, {{0x0000021Au, 0x00000000}}},
    /* 4 */ {0xBE7DCD73, {{0x0000027Du, 0x00000000}}},
    /* 5 */ {0x0C4B1438, {{0x0000022Au, 0x00000000}}},
    /* 6 */ {0xDB00D71A, {{Sh(mmCOMPUTE_START_X), 0x00000000}}},
    /* 7 */ {0xDB00D249, {{Sh(mmCOMPUTE_START_Y), 0x00000000}}},
    /* 8 */ {0xDB00EC60, {{Sh(mmCOMPUTE_START_Z), 0x00000000}}},
    /* 9 */ {0x0C4D6FE4, {{0x00000080u, 0x00000000}}},
    /* 10 */ {0x0C4A80EF, {{0x00000100u, 0x00000000}}},
    /* 11 */ {0x0DD283E7, {{0x00000006u, 0x00000000}}},
    /* 12 */ {0xC620E68C, {{0x00000081u, 0x00000000}}},
    /* 13 */ {0xC67EFACF, {{0x00000101u, 0x00000000}}},
    /* 14 */ {0xD9E6D9F7, {{0x00000001u, 0x00000000}}},
};

static RegisterDefaultInfo g_uc_reg_info2[] = {
    /* 0 */ {0x31F34B9F, {{Uc(mmVGT_HS_OFFCHIP_PARAM__CI__VI), 0x00000000}}},
    /* 1 */ {0xAC0F9E76, {{0x80003FFFu, 0x00000000}}},
    /* 2 */ {0x929FD95D, {{Uc(mmVGT_TF_MEMORY_BASE__CI__VI), 0x00000000}}},
};
