

/* this ALWAYS GENERATED file contains the RPC server stubs */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Mon Aug 10 07:30:17 2009
 */
/* Compiler settings for ..\RPC VFS\RPC VFS.Idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#if !defined(_M_IA64) && !defined(_M_AMD64)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/
#pragma warning( disable: 4100 ) /* unreferenced arguments in x86 call */

#pragma optimize("", off ) 

#include <string.h>
#include "RPC VFS_h.h"

#define TYPE_FORMAT_STRING_SIZE   127                               
#define PROC_FORMAT_STRING_SIZE   2795                              
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   0            

typedef struct _RPC_VFS_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } RPC_VFS_MIDL_TYPE_FORMAT_STRING;

typedef struct _RPC_VFS_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } RPC_VFS_MIDL_PROC_FORMAT_STRING;

typedef struct _RPC_VFS_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } RPC_VFS_MIDL_EXPR_FORMAT_STRING;


static RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};

extern const RPC_VFS_MIDL_TYPE_FORMAT_STRING RPC_VFS__MIDL_TypeFormatString;
extern const RPC_VFS_MIDL_PROC_FORMAT_STRING RPC_VFS__MIDL_ProcFormatString;
extern const RPC_VFS_MIDL_EXPR_FORMAT_STRING RPC_VFS__MIDL_ExprFormatString;

/* Standard interface: RPC_VFS, ver. 1.0,
   GUID={0xD38A2BC0,0xF95F,0x11DD,{0x87,0xAF,0x08,0x00,0x20,0x0C,0x9A,0x66}} */


extern const MIDL_SERVER_INFO RPC_VFS_ServerInfo;

extern RPC_DISPATCH_TABLE RPC_VFS_v1_0_DispatchTable;

static const RPC_SERVER_INTERFACE RPC_VFS___RpcServerInterface =
    {
    sizeof(RPC_SERVER_INTERFACE),
    {{0xD38A2BC0,0xF95F,0x11DD,{0x87,0xAF,0x08,0x00,0x20,0x0C,0x9A,0x66}},{1,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    &RPC_VFS_v1_0_DispatchTable,
    0,
    0,
    0,
    &RPC_VFS_ServerInfo,
    0x04000000
    };
RPC_IF_HANDLE RPC_VFS_v1_0_s_ifspec = (RPC_IF_HANDLE)& RPC_VFS___RpcServerInterface;

extern const MIDL_STUB_DESC RPC_VFS_StubDesc;


#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT50_OR_LATER)
#error You need a Windows 2000 or later to run this stub because it uses these features:
#error   /robust command line switch.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will fail with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const RPC_VFS_MIDL_PROC_FORMAT_STRING RPC_VFS__MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure VFS_RPC_GetVersion */

			0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 10 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 12 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 14 */	NdrFcShort( 0x1c ),	/* 28 */
/* 16 */	NdrFcShort( 0x24 ),	/* 36 */
/* 18 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 20 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */
/* 26 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 28 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 30 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 32 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pVersion */

/* 34 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 36 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 38 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_PartialFlush */


	/* Return value */

/* 40 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 42 */	NdrFcLong( 0x0 ),	/* 0 */
/* 46 */	NdrFcShort( 0x1 ),	/* 1 */
/* 48 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 50 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 52 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 54 */	NdrFcShort( 0x0 ),	/* 0 */
/* 56 */	NdrFcShort( 0x8 ),	/* 8 */
/* 58 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 60 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 62 */	NdrFcShort( 0x0 ),	/* 0 */
/* 64 */	NdrFcShort( 0x0 ),	/* 0 */
/* 66 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 68 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 70 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 72 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_FullFlush */


	/* Return value */

/* 74 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 76 */	NdrFcLong( 0x0 ),	/* 0 */
/* 80 */	NdrFcShort( 0x2 ),	/* 2 */
/* 82 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 84 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 86 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 88 */	NdrFcShort( 0x0 ),	/* 0 */
/* 90 */	NdrFcShort( 0x8 ),	/* 8 */
/* 92 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 94 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 96 */	NdrFcShort( 0x0 ),	/* 0 */
/* 98 */	NdrFcShort( 0x0 ),	/* 0 */
/* 100 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 102 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 104 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 106 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_Open */


	/* Return value */

/* 108 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 110 */	NdrFcLong( 0x0 ),	/* 0 */
/* 114 */	NdrFcShort( 0x3 ),	/* 3 */
/* 116 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 118 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 120 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 122 */	NdrFcShort( 0x10 ),	/* 16 */
/* 124 */	NdrFcShort( 0x8 ),	/* 8 */
/* 126 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 128 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 130 */	NdrFcShort( 0x0 ),	/* 0 */
/* 132 */	NdrFcShort( 0x0 ),	/* 0 */
/* 134 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 136 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 138 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 140 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pFilename */

/* 142 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 144 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 146 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter UserVersion */

/* 148 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 150 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 152 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ReadOnly */

/* 154 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 156 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 158 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_Create */


	/* Return value */

/* 160 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 162 */	NdrFcLong( 0x0 ),	/* 0 */
/* 166 */	NdrFcShort( 0x4 ),	/* 4 */
/* 168 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 170 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 172 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 174 */	NdrFcShort( 0x10 ),	/* 16 */
/* 176 */	NdrFcShort( 0x8 ),	/* 8 */
/* 178 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 180 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 182 */	NdrFcShort( 0x0 ),	/* 0 */
/* 184 */	NdrFcShort( 0x0 ),	/* 0 */
/* 186 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 188 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 190 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 192 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pFilename */

/* 194 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 196 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 198 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter UserVersion */

/* 200 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 202 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 204 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ClusterSize */

/* 206 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 208 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 210 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_Close */


	/* Return value */

/* 212 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 214 */	NdrFcLong( 0x0 ),	/* 0 */
/* 218 */	NdrFcShort( 0x5 ),	/* 5 */
/* 220 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 222 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 224 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 226 */	NdrFcShort( 0x0 ),	/* 0 */
/* 228 */	NdrFcShort( 0x8 ),	/* 8 */
/* 230 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 232 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 234 */	NdrFcShort( 0x0 ),	/* 0 */
/* 236 */	NdrFcShort( 0x0 ),	/* 0 */
/* 238 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 240 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 242 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 244 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_GetLastError */


	/* Return value */

/* 246 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 248 */	NdrFcLong( 0x0 ),	/* 0 */
/* 252 */	NdrFcShort( 0x6 ),	/* 6 */
/* 254 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 256 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 258 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 260 */	NdrFcShort( 0x1c ),	/* 28 */
/* 262 */	NdrFcShort( 0x24 ),	/* 36 */
/* 264 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 266 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 268 */	NdrFcShort( 0x0 ),	/* 0 */
/* 270 */	NdrFcShort( 0x0 ),	/* 0 */
/* 272 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 274 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 276 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 278 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pResult */

/* 280 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 282 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 284 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSCreateFile */


	/* Return value */

/* 286 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 288 */	NdrFcLong( 0x0 ),	/* 0 */
/* 292 */	NdrFcShort( 0x7 ),	/* 7 */
/* 294 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 296 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 298 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 300 */	NdrFcShort( 0x2c ),	/* 44 */
/* 302 */	NdrFcShort( 0x2c ),	/* 44 */
/* 304 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 306 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 308 */	NdrFcShort( 0x0 ),	/* 0 */
/* 310 */	NdrFcShort( 0x0 ),	/* 0 */
/* 312 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 314 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 316 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 318 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pName */

/* 320 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 322 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 324 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Mode */

/* 326 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 328 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 330 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter phFile */

/* 332 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 334 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 336 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSCreateFiledataFile */


	/* Return value */

/* 338 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 340 */	NdrFcLong( 0x0 ),	/* 0 */
/* 344 */	NdrFcShort( 0x8 ),	/* 8 */
/* 346 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 348 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 350 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 352 */	NdrFcShort( 0x3c ),	/* 60 */
/* 354 */	NdrFcShort( 0x2c ),	/* 44 */
/* 356 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x4,		/* 4 */
/* 358 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 360 */	NdrFcShort( 0x0 ),	/* 0 */
/* 362 */	NdrFcShort( 0x0 ),	/* 0 */
/* 364 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 366 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 368 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 370 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pFiledata */

/* 372 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 374 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 376 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Mode */

/* 378 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 380 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 382 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter phFile */

/* 384 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 386 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 388 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSCreateUIDFile */


	/* Return value */

/* 390 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 392 */	NdrFcLong( 0x0 ),	/* 0 */
/* 396 */	NdrFcShort( 0x9 ),	/* 9 */
/* 398 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 400 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 402 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 404 */	NdrFcShort( 0x3c ),	/* 60 */
/* 406 */	NdrFcShort( 0x2c ),	/* 44 */
/* 408 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x4,		/* 4 */
/* 410 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 412 */	NdrFcShort( 0x0 ),	/* 0 */
/* 414 */	NdrFcShort( 0x0 ),	/* 0 */
/* 416 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 418 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 420 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 422 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter UID */

/* 424 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 426 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 428 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Mode */

/* 430 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 432 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 434 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter phFile */

/* 436 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 438 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 440 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSCloseFile */


	/* Return value */

/* 442 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 444 */	NdrFcLong( 0x0 ),	/* 0 */
/* 448 */	NdrFcShort( 0xa ),	/* 10 */
/* 450 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 452 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 454 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 456 */	NdrFcShort( 0x10 ),	/* 16 */
/* 458 */	NdrFcShort( 0x8 ),	/* 8 */
/* 460 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 462 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 464 */	NdrFcShort( 0x0 ),	/* 0 */
/* 466 */	NdrFcShort( 0x0 ),	/* 0 */
/* 468 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 470 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 472 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 474 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter hFile */

/* 476 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 478 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 480 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSRead */


	/* Return value */

/* 482 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 484 */	NdrFcLong( 0x0 ),	/* 0 */
/* 488 */	NdrFcShort( 0xb ),	/* 11 */
/* 490 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 492 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 494 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 496 */	NdrFcShort( 0x18 ),	/* 24 */
/* 498 */	NdrFcShort( 0x24 ),	/* 36 */
/* 500 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 502 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 504 */	NdrFcShort( 0x1 ),	/* 1 */
/* 506 */	NdrFcShort( 0x0 ),	/* 0 */
/* 508 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 510 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 512 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 514 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter hFile */

/* 516 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 518 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 520 */	NdrFcShort( 0x12 ),	/* Type Offset=18 */

	/* Parameter pBuffer */

/* 522 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 524 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 526 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Count */

/* 528 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 530 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 532 */	NdrFcShort( 0x22 ),	/* Type Offset=34 */

	/* Parameter pCB */

/* 534 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 536 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 538 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pRead */

/* 540 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 542 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 544 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSWrite */


	/* Return value */

/* 546 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 548 */	NdrFcLong( 0x0 ),	/* 0 */
/* 552 */	NdrFcShort( 0xc ),	/* 12 */
/* 554 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 556 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 558 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 560 */	NdrFcShort( 0x18 ),	/* 24 */
/* 562 */	NdrFcShort( 0x24 ),	/* 36 */
/* 564 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 566 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 568 */	NdrFcShort( 0x0 ),	/* 0 */
/* 570 */	NdrFcShort( 0x1 ),	/* 1 */
/* 572 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 574 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 576 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 578 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter hFile */

/* 580 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 582 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 584 */	NdrFcShort( 0x12 ),	/* Type Offset=18 */

	/* Parameter pBuffer */

/* 586 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 588 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 590 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Count */

/* 592 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 594 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 596 */	NdrFcShort( 0x22 ),	/* Type Offset=34 */

	/* Parameter pCB */

/* 598 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 600 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 602 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pWritten */

/* 604 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 606 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 608 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSSeek */


	/* Return value */

/* 610 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 612 */	NdrFcLong( 0x0 ),	/* 0 */
/* 616 */	NdrFcShort( 0xd ),	/* 13 */
/* 618 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 620 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 622 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 624 */	NdrFcShort( 0x28 ),	/* 40 */
/* 626 */	NdrFcShort( 0x2c ),	/* 44 */
/* 628 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x5,		/* 5 */
/* 630 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 632 */	NdrFcShort( 0x0 ),	/* 0 */
/* 634 */	NdrFcShort( 0x0 ),	/* 0 */
/* 636 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 638 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 640 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 642 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter hFile */

/* 644 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 646 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 648 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter Offset */

/* 650 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 652 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 654 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Origin */

/* 656 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 658 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 660 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pNewOffset */

/* 662 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 664 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 666 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSTell */


	/* Return value */

/* 668 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 670 */	NdrFcLong( 0x0 ),	/* 0 */
/* 674 */	NdrFcShort( 0xe ),	/* 14 */
/* 676 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 678 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 680 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 682 */	NdrFcShort( 0x10 ),	/* 16 */
/* 684 */	NdrFcShort( 0x2c ),	/* 44 */
/* 686 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x3,		/* 3 */
/* 688 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 690 */	NdrFcShort( 0x0 ),	/* 0 */
/* 692 */	NdrFcShort( 0x0 ),	/* 0 */
/* 694 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 696 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 698 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 700 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter hFile */

/* 702 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 704 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 706 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pOffset */

/* 708 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 710 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 712 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSSize */


	/* Return value */

/* 714 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 716 */	NdrFcLong( 0x0 ),	/* 0 */
/* 720 */	NdrFcShort( 0xf ),	/* 15 */
/* 722 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 724 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 726 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 728 */	NdrFcShort( 0x10 ),	/* 16 */
/* 730 */	NdrFcShort( 0x2c ),	/* 44 */
/* 732 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x3,		/* 3 */
/* 734 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 736 */	NdrFcShort( 0x0 ),	/* 0 */
/* 738 */	NdrFcShort( 0x0 ),	/* 0 */
/* 740 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 742 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 744 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 746 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter hFile */

/* 748 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 750 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 752 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pSize */

/* 754 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 756 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 758 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSEndOfFile */


	/* Return value */

/* 760 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 762 */	NdrFcLong( 0x0 ),	/* 0 */
/* 766 */	NdrFcShort( 0x10 ),	/* 16 */
/* 768 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 770 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 772 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 774 */	NdrFcShort( 0x10 ),	/* 16 */
/* 776 */	NdrFcShort( 0x8 ),	/* 8 */
/* 778 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 780 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 782 */	NdrFcShort( 0x0 ),	/* 0 */
/* 784 */	NdrFcShort( 0x0 ),	/* 0 */
/* 786 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 788 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 790 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 792 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter hFile */

/* 794 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 796 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 798 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSSetEndOfFile */


	/* Return value */

/* 800 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 802 */	NdrFcLong( 0x0 ),	/* 0 */
/* 806 */	NdrFcShort( 0x11 ),	/* 17 */
/* 808 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 810 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 812 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 814 */	NdrFcShort( 0x10 ),	/* 16 */
/* 816 */	NdrFcShort( 0x8 ),	/* 8 */
/* 818 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 820 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 822 */	NdrFcShort( 0x0 ),	/* 0 */
/* 824 */	NdrFcShort( 0x0 ),	/* 0 */
/* 826 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 828 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 830 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 832 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter hFile */

/* 834 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 836 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 838 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSMemCache */


	/* Return value */

/* 840 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 842 */	NdrFcLong( 0x0 ),	/* 0 */
/* 846 */	NdrFcShort( 0x12 ),	/* 18 */
/* 848 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 850 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 852 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 854 */	NdrFcShort( 0x10 ),	/* 16 */
/* 856 */	NdrFcShort( 0x35 ),	/* 53 */
/* 858 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x3,		/* 3 */
/* 860 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 862 */	NdrFcShort( 0x0 ),	/* 0 */
/* 864 */	NdrFcShort( 0x0 ),	/* 0 */
/* 866 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 868 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 870 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 872 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter hFile */

/* 874 */	NdrFcShort( 0x2012 ),	/* Flags:  must free, out, srv alloc size=8 */
/* 876 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 878 */	NdrFcShort( 0x36 ),	/* Type Offset=54 */

	/* Parameter pCache */

/* 880 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 882 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 884 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSGetUIDFromFileHandle */


	/* Return value */

/* 886 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 888 */	NdrFcLong( 0x0 ),	/* 0 */
/* 892 */	NdrFcShort( 0x13 ),	/* 19 */
/* 894 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 896 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 898 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 900 */	NdrFcShort( 0x10 ),	/* 16 */
/* 902 */	NdrFcShort( 0x2c ),	/* 44 */
/* 904 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x3,		/* 3 */
/* 906 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 908 */	NdrFcShort( 0x0 ),	/* 0 */
/* 910 */	NdrFcShort( 0x0 ),	/* 0 */
/* 912 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 914 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 916 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 918 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter hFile */

/* 920 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 922 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 924 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pUID */

/* 926 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 928 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 930 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSDeleteFiledataDirectory */


	/* Return value */

/* 932 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 934 */	NdrFcLong( 0x0 ),	/* 0 */
/* 938 */	NdrFcShort( 0x14 ),	/* 20 */
/* 940 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 942 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 944 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 946 */	NdrFcShort( 0x10 ),	/* 16 */
/* 948 */	NdrFcShort( 0x8 ),	/* 8 */
/* 950 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 952 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 954 */	NdrFcShort( 0x0 ),	/* 0 */
/* 956 */	NdrFcShort( 0x0 ),	/* 0 */
/* 958 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 960 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 962 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 964 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pDir */

/* 966 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 968 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 970 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSDeleteFiledataFile */


	/* Return value */

/* 972 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 974 */	NdrFcLong( 0x0 ),	/* 0 */
/* 978 */	NdrFcShort( 0x15 ),	/* 21 */
/* 980 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 982 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 984 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 986 */	NdrFcShort( 0x10 ),	/* 16 */
/* 988 */	NdrFcShort( 0x8 ),	/* 8 */
/* 990 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 992 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 994 */	NdrFcShort( 0x0 ),	/* 0 */
/* 996 */	NdrFcShort( 0x0 ),	/* 0 */
/* 998 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1000 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1002 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1004 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pFile */

/* 1006 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1008 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1010 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSEnum */


	/* Return value */

/* 1012 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1014 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1018 */	NdrFcShort( 0x16 ),	/* 22 */
/* 1020 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 1022 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1024 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1026 */	NdrFcShort( 0x41 ),	/* 65 */
/* 1028 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1030 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x7,		/* 7 */
/* 1032 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1034 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1036 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1038 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1040 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1042 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1044 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pDirectory */

/* 1046 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 1048 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1050 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Parameter pCB */

/* 1052 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1054 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1056 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter wParam */

/* 1058 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1060 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1062 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter lParam */

/* 1064 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1066 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 1068 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter what */

/* 1070 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1072 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 1074 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pResult */

/* 1076 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1078 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 1080 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSSingleEnum */


	/* Return value */

/* 1082 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1084 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1088 */	NdrFcShort( 0x17 ),	/* 23 */
/* 1090 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 1092 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1094 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1096 */	NdrFcShort( 0x49 ),	/* 73 */
/* 1098 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1100 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x6,		/* 6 */
/* 1102 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1104 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1106 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1108 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1110 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1112 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1114 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pFileData */

/* 1116 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 1118 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1120 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Parameter pCB */

/* 1122 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1124 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1126 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter wParam */

/* 1128 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1130 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1132 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter lParam */

/* 1134 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1136 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 1138 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pResult */

/* 1140 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1142 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 1144 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSEmbedFile */


	/* Return value */

/* 1146 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1148 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1152 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1154 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1156 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1158 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1160 */	NdrFcShort( 0x22 ),	/* 34 */
/* 1162 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1164 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 1166 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1168 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1170 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1172 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1174 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1176 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1178 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pSrcFile */

/* 1180 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 1182 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1184 */	0x5,		/* FC_WCHAR */
			0x0,		/* 0 */

	/* Parameter pDestFile */

/* 1186 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1188 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1190 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OverWrite */

/* 1192 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1194 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1196 */	NdrFcShort( 0x22 ),	/* Type Offset=34 */

	/* Parameter pCB */

/* 1198 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1200 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1202 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSExtractFile */


	/* Return value */

/* 1204 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1206 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1210 */	NdrFcShort( 0x19 ),	/* 25 */
/* 1212 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1214 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1216 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1218 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1220 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1222 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 1224 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1226 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1228 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1230 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1232 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1234 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1236 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pSrcFile */

/* 1238 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1240 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1242 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pDestFile */

/* 1244 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1246 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1248 */	NdrFcShort( 0x22 ),	/* Type Offset=34 */

	/* Parameter pCB */

/* 1250 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1252 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1254 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSEmbedDir */


	/* Return value */

/* 1256 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1258 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1262 */	NdrFcShort( 0x1a ),	/* 26 */
/* 1264 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1266 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1268 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1270 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1272 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1274 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 1276 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1278 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1280 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1282 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1284 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1286 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1288 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pSrcDir */

/* 1290 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1292 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1294 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pDestDir */

/* 1296 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1298 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1300 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OverWrite */

/* 1302 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1304 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1306 */	NdrFcShort( 0x22 ),	/* Type Offset=34 */

	/* Parameter pCB */

/* 1308 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1310 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1312 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSExtractDir */


	/* Return value */

/* 1314 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1316 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1320 */	NdrFcShort( 0x1b ),	/* 27 */
/* 1322 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1324 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1326 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1328 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1330 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1332 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 1334 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1336 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1338 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1340 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1342 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1344 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1346 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pSrcDir */

/* 1348 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1350 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1352 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pDestDir */

/* 1354 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1356 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1358 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OverWrite */

/* 1360 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1362 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1364 */	NdrFcShort( 0x22 ),	/* Type Offset=34 */

	/* Parameter pCB */

/* 1366 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1368 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1370 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSEmbedFiledataFile */


	/* Return value */

/* 1372 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1374 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1378 */	NdrFcShort( 0x1c ),	/* 28 */
/* 1380 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 1382 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1384 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1386 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1388 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1390 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 1392 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1394 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1396 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1398 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1400 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1402 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1404 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pSrcFile */

/* 1406 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1408 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1410 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pDestDir */

/* 1412 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1414 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1416 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OverWrite */

/* 1418 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1420 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1422 */	NdrFcShort( 0x22 ),	/* Type Offset=34 */

	/* Parameter pCB */

/* 1424 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1426 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1428 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSExtractFiledataFile */


	/* Return value */

/* 1430 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1432 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1436 */	NdrFcShort( 0x1d ),	/* 29 */
/* 1438 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1440 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1442 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1444 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1446 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1448 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 1450 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1452 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1454 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1456 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1458 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1460 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1462 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pSrcFile */

/* 1464 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1466 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1468 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pDestFile */

/* 1470 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1472 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1474 */	NdrFcShort( 0x22 ),	/* Type Offset=34 */

	/* Parameter pCB */

/* 1476 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1478 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1480 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSEmbedFiledataDir */


	/* Return value */

/* 1482 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1484 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1488 */	NdrFcShort( 0x1e ),	/* 30 */
/* 1490 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 1492 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1494 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1496 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1498 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1500 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 1502 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1504 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1506 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1508 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1510 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1512 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1514 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pSrcDir */

/* 1516 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1518 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1520 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pDestDir */

/* 1522 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1524 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1526 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OverWrite */

/* 1528 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1530 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1532 */	NdrFcShort( 0x22 ),	/* Type Offset=34 */

	/* Parameter pCB */

/* 1534 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1536 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1538 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSExtractFiledataDir */


	/* Return value */

/* 1540 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1542 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1546 */	NdrFcShort( 0x1f ),	/* 31 */
/* 1548 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 1550 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1552 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1554 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1556 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1558 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 1560 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1562 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1564 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1566 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1568 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1570 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1572 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pSrcDir */

/* 1574 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1576 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1578 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pDestDir */

/* 1580 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1582 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1584 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OverWrite */

/* 1586 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1588 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1590 */	NdrFcShort( 0x22 ),	/* Type Offset=34 */

	/* Parameter pCB */

/* 1592 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1594 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1596 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSQueryFileInfo */


	/* Return value */

/* 1598 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1600 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1604 */	NdrFcShort( 0x20 ),	/* 32 */
/* 1606 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 1608 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1610 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1612 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1614 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1616 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 1618 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 1620 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1622 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1624 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1626 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 1628 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1630 */	NdrFcShort( 0x50 ),	/* Type Offset=80 */

	/* Parameter pFileInfo */

/* 1632 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1634 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1636 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pFileData */

/* 1638 */	NdrFcShort( 0x11b ),	/* Flags:  must size, must free, in, out, simple ref, */
/* 1640 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1642 */	NdrFcShort( 0x6e ),	/* Type Offset=110 */

	/* Parameter pDest */

/* 1644 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1646 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1648 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter DestSize */

/* 1650 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1652 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1654 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSSetFileUserData */


	/* Return value */

/* 1656 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1658 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1662 */	NdrFcShort( 0x21 ),	/* 33 */
/* 1664 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1666 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1668 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1670 */	NdrFcShort( 0x31 ),	/* 49 */
/* 1672 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1674 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x4,		/* 4 */
/* 1676 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1678 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1680 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1682 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1684 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1686 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1688 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pFileData */

/* 1690 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 1692 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1694 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Parameter pUserData */

/* 1696 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1698 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1700 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter size */

/* 1702 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1704 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1706 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSGetFileUserData */


	/* Return value */

/* 1708 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1710 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1714 */	NdrFcShort( 0x22 ),	/* 34 */
/* 1716 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1718 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1720 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1722 */	NdrFcShort( 0x31 ),	/* 49 */
/* 1724 */	NdrFcShort( 0x21 ),	/* 33 */
/* 1726 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x4,		/* 4 */
/* 1728 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1730 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1732 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1734 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1736 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1738 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1740 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pFileData */

/* 1742 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 1744 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1746 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Parameter pUserData */

/* 1748 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1750 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1752 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter size */

/* 1754 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1756 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1758 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSSetRuntimeUserData */


	/* Return value */

/* 1760 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1762 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1766 */	NdrFcShort( 0x23 ),	/* 35 */
/* 1768 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 1770 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1772 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1774 */	NdrFcShort( 0x20 ),	/* 32 */
/* 1776 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1778 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x3,		/* 3 */
/* 1780 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1782 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1784 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1786 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1788 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1790 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1792 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pFileData */

/* 1794 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1796 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1798 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pUserData */

/* 1800 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1802 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1804 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSGetRuntimeUserData */


	/* Return value */

/* 1806 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1808 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1812 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1814 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1816 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1818 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1820 */	NdrFcShort( 0x34 ),	/* 52 */
/* 1822 */	NdrFcShort( 0x2c ),	/* 44 */
/* 1824 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x3,		/* 3 */
/* 1826 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1828 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1830 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1832 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1834 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1836 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1838 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pFileData */

/* 1840 */	NdrFcShort( 0x158 ),	/* Flags:  in, out, base type, simple ref, */
/* 1842 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1844 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pUserData */

/* 1846 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1848 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1850 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSGetRootFiledata */


	/* Return value */

/* 1852 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1854 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1858 */	NdrFcShort( 0x25 ),	/* 37 */
/* 1860 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1862 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1864 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1866 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1868 */	NdrFcShort( 0x2c ),	/* 44 */
/* 1870 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 1872 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1874 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1876 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1878 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1880 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1882 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1884 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter ppFileData */

/* 1886 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1888 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1890 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSGetFileData */


	/* Return value */

/* 1892 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1894 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1898 */	NdrFcShort( 0x26 ),	/* 38 */
/* 1900 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1902 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1904 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1906 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1908 */	NdrFcShort( 0x2c ),	/* 44 */
/* 1910 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 1912 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1914 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1916 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1918 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1920 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1922 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1924 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pPath */

/* 1926 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1928 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1930 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter ppFileData */

/* 1932 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1934 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1936 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSFileExists */


	/* Return value */

/* 1938 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1940 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1944 */	NdrFcShort( 0x27 ),	/* 39 */
/* 1946 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1948 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1950 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1952 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1954 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1956 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1958 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1960 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1962 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1964 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 1966 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 1968 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1970 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pFile */

/* 1972 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1974 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1976 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSPathExists */


	/* Return value */

/* 1978 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 1980 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1984 */	NdrFcShort( 0x28 ),	/* 40 */
/* 1986 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1988 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 1990 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 1992 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1994 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1996 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1998 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2000 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2002 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2004 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 2006 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2008 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2010 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pPath */

/* 2012 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2014 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2016 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSFileSize */


	/* Return value */

/* 2018 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2020 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2024 */	NdrFcShort( 0x29 ),	/* 41 */
/* 2026 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2028 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 2030 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 2032 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2034 */	NdrFcShort( 0x2c ),	/* 44 */
/* 2036 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 2038 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2040 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2042 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2044 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 2046 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2048 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2050 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pPath */

/* 2052 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2054 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2056 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pSize */

/* 2058 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2060 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2062 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSSetNoEnum */


	/* Return value */

/* 2064 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2066 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2070 */	NdrFcShort( 0x2a ),	/* 42 */
/* 2072 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 2074 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 2076 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 2078 */	NdrFcShort( 0x18 ),	/* 24 */
/* 2080 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2082 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x4,		/* 4 */
/* 2084 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2086 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2088 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2090 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 2092 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2094 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2096 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pFileData */

/* 2098 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2100 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2102 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter noenum */

/* 2104 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2106 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2108 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pOldValue */

/* 2110 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2112 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2114 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSGetNoEnum */


	/* Return value */

/* 2116 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2118 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2122 */	NdrFcShort( 0x2b ),	/* 43 */
/* 2124 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2126 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 2128 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 2130 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2132 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2134 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x3,		/* 3 */
/* 2136 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2138 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2140 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2142 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 2144 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2146 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2148 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pFileData */

/* 2150 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2152 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2154 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pValue */

/* 2156 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2158 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2160 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSSetFileOption */


	/* Return value */

/* 2162 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2164 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2168 */	NdrFcShort( 0x2c ),	/* 44 */
/* 2170 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 2172 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 2174 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 2176 */	NdrFcShort( 0x39 ),	/* 57 */
/* 2178 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2180 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x5,		/* 5 */
/* 2182 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2184 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2186 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2188 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 2190 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2192 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2194 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter hFile */

/* 2196 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2198 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2200 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter fileoption */

/* 2202 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 2204 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2206 */	0x2,		/* FC_CHAR */
			0x0,		/* 0 */

	/* Parameter pData */

/* 2208 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2210 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2212 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter size */

/* 2214 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2216 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 2218 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSGetFileUID */


	/* Return value */

/* 2220 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2222 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2226 */	NdrFcShort( 0x2d ),	/* 45 */
/* 2228 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2230 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 2232 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 2234 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2236 */	NdrFcShort( 0x2c ),	/* 44 */
/* 2238 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 2240 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2242 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2244 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2246 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 2248 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2250 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2252 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pPath */

/* 2254 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2256 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2258 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pUID */

/* 2260 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2262 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2264 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSGetDirectoryUID */


	/* Return value */

/* 2266 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2268 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2272 */	NdrFcShort( 0x2e ),	/* 46 */
/* 2274 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2276 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 2278 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 2280 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2282 */	NdrFcShort( 0x2c ),	/* 44 */
/* 2284 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 2286 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2288 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2290 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2292 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 2294 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2296 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2298 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pPath */

/* 2300 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2302 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2304 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pUID */

/* 2306 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2308 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2310 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSGetEntryUID */


	/* Return value */

/* 2312 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2314 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2318 */	NdrFcShort( 0x2f ),	/* 47 */
/* 2320 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2322 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 2324 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 2326 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2328 */	NdrFcShort( 0x2c ),	/* 44 */
/* 2330 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 2332 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2334 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2336 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2338 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 2340 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2342 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2344 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pPath */

/* 2346 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2348 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2350 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pUID */

/* 2352 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2354 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2356 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSDeleteFile */


	/* Return value */

/* 2358 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2360 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2364 */	NdrFcShort( 0x30 ),	/* 48 */
/* 2366 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2368 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 2370 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 2372 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2374 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2376 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 2378 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2380 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2382 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2384 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 2386 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2388 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2390 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pFile */

/* 2392 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2394 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2396 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSDeleteDirectory */


	/* Return value */

/* 2398 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2400 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2404 */	NdrFcShort( 0x31 ),	/* 49 */
/* 2406 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2408 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 2410 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 2412 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2414 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2416 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 2418 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2420 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2422 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2424 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 2426 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2428 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2430 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pDir */

/* 2432 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2434 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2436 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSGetSettings */


	/* Return value */

/* 2438 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2440 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2444 */	NdrFcShort( 0x32 ),	/* 50 */
/* 2446 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2448 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 2450 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 2452 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2454 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2456 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2458 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2460 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2462 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2464 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 2466 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2468 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2470 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pSettings */

/* 2472 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2474 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2476 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSGetClusterSize */


	/* Return value */

/* 2478 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2480 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2484 */	NdrFcShort( 0x33 ),	/* 51 */
/* 2486 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2488 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 2490 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 2492 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2494 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2496 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2498 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2500 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2502 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2504 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 2506 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2508 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2510 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pClusterSize */

/* 2512 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2514 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2516 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_DumpFTInfo */


	/* Return value */

/* 2518 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2520 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2524 */	NdrFcShort( 0x34 ),	/* 52 */
/* 2526 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2528 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 2530 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 2532 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2534 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2536 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 2538 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2540 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2542 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2544 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 2546 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2548 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2550 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_TranslateErrorCode */


	/* Return value */

/* 2552 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2554 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2558 */	NdrFcShort( 0x35 ),	/* 53 */
/* 2560 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2562 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 2564 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 2566 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2568 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2570 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x3,		/* 3 */
/* 2572 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2574 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2576 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2578 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 2580 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2582 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2584 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter error */

/* 2586 */	NdrFcShort( 0x2013 ),	/* Flags:  must size, must free, out, srv alloc size=8 */
/* 2588 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2590 */	NdrFcShort( 0x76 ),	/* Type Offset=118 */

	/* Parameter pError */

/* 2592 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2594 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2596 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSGetMD5 */


	/* Return value */

/* 2598 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2600 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2604 */	NdrFcShort( 0x36 ),	/* 54 */
/* 2606 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2608 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 2610 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 2612 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2614 */	NdrFcShort( 0x28 ),	/* 40 */
/* 2616 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 2618 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2620 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2622 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2624 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 2626 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2628 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2630 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pFile */

/* 2632 */	NdrFcShort( 0x12 ),	/* Flags:  must free, out, */
/* 2634 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2636 */	NdrFcShort( 0x4a ),	/* Type Offset=74 */

	/* Parameter pMD5 */

/* 2638 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2640 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2642 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Store */

/* 2644 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2646 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2648 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSGetFileDataMD5 */


	/* Return value */

/* 2650 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2652 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2656 */	NdrFcShort( 0x37 ),	/* 55 */
/* 2658 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 2660 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 2662 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 2664 */	NdrFcShort( 0x18 ),	/* 24 */
/* 2666 */	NdrFcShort( 0x28 ),	/* 40 */
/* 2668 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x4,		/* 4 */
/* 2670 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2672 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2674 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2676 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 2678 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2680 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2682 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pFileData */

/* 2684 */	NdrFcShort( 0x12 ),	/* Flags:  must free, out, */
/* 2686 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2688 */	NdrFcShort( 0x4a ),	/* Type Offset=74 */

	/* Parameter pMD5 */

/* 2690 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2692 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2694 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Store */

/* 2696 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2698 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2700 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSQueryMD5 */


	/* Return value */

/* 2702 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2704 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2708 */	NdrFcShort( 0x38 ),	/* 56 */
/* 2710 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2712 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 2714 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 2716 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2718 */	NdrFcShort( 0x28 ),	/* 40 */
/* 2720 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 2722 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2724 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2726 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2728 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 2730 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2732 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2734 */	NdrFcShort( 0x8 ),	/* Type Offset=8 */

	/* Parameter pFile */

/* 2736 */	NdrFcShort( 0x12 ),	/* Flags:  must free, out, */
/* 2738 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2740 */	NdrFcShort( 0x4a ),	/* Type Offset=74 */

	/* Parameter pMD5 */

/* 2742 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2744 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2746 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure VFS_RPC_VFSQueryFileDataMD5 */


	/* Return value */

/* 2748 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 2750 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2754 */	NdrFcShort( 0x39 ),	/* 57 */
/* 2756 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2758 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 2760 */	NdrFcShort( 0x0 ),	/* x86 Stack size/offset = 0 */
/* 2762 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2764 */	NdrFcShort( 0x28 ),	/* 40 */
/* 2766 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x3,		/* 3 */
/* 2768 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2770 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2772 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2774 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter IDL_handle */

/* 2776 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2778 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2780 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Parameter pFileData */

/* 2782 */	NdrFcShort( 0x12 ),	/* Flags:  must free, out, */
/* 2784 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2786 */	NdrFcShort( 0x4a ),	/* Type Offset=74 */

	/* Parameter pMD5 */

/* 2788 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2790 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2792 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const RPC_VFS_MIDL_TYPE_FORMAT_STRING RPC_VFS__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/*  4 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/*  6 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/*  8 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 10 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 12 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 14 */	
			0x11, 0x0,	/* FC_RP */
/* 16 */	NdrFcShort( 0x2 ),	/* Offset= 2 (18) */
/* 18 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 20 */	NdrFcShort( 0x1 ),	/* 1 */
/* 22 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 24 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 26 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 28 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 30 */	
			0x11, 0x0,	/* FC_RP */
/* 32 */	NdrFcShort( 0x2 ),	/* Offset= 2 (34) */
/* 34 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 36 */	NdrFcShort( 0x10 ),	/* 16 */
/* 38 */	NdrFcShort( 0x0 ),	/* 0 */
/* 40 */	NdrFcShort( 0x0 ),	/* Offset= 0 (40) */
/* 42 */	0xb,		/* FC_HYPER */
			0x8,		/* FC_LONG */
/* 44 */	0x40,		/* FC_STRUCTPAD4 */
			0x5b,		/* FC_END */
/* 46 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 48 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 50 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 52 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 54 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 56 */	NdrFcShort( 0x2 ),	/* Offset= 2 (58) */
/* 58 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 60 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 62 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 64 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 66 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 68 */	0x5,		/* FC_WCHAR */
			0x5c,		/* FC_PAD */
/* 70 */	
			0x11, 0x0,	/* FC_RP */
/* 72 */	NdrFcShort( 0x8 ),	/* Offset= 8 (80) */
/* 74 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 76 */	NdrFcShort( 0x10 ),	/* 16 */
/* 78 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 80 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 82 */	NdrFcShort( 0x60 ),	/* 96 */
/* 84 */	NdrFcShort( 0x0 ),	/* 0 */
/* 86 */	NdrFcShort( 0x0 ),	/* Offset= 0 (86) */
/* 88 */	0xb,		/* FC_HYPER */
			0xb,		/* FC_HYPER */
/* 90 */	0x2,		/* FC_CHAR */
			0x43,		/* FC_STRUCTPAD7 */
/* 92 */	0xb,		/* FC_HYPER */
			0xb,		/* FC_HYPER */
/* 94 */	0xb,		/* FC_HYPER */
			0x8,		/* FC_LONG */
/* 96 */	0x40,		/* FC_STRUCTPAD4 */
			0xb,		/* FC_HYPER */
/* 98 */	0xb,		/* FC_HYPER */
			0x8,		/* FC_LONG */
/* 100 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 102 */	NdrFcShort( 0xffe4 ),	/* Offset= -28 (74) */
/* 104 */	0x40,		/* FC_STRUCTPAD4 */
			0x5b,		/* FC_END */
/* 106 */	
			0x11, 0x0,	/* FC_RP */
/* 108 */	NdrFcShort( 0x2 ),	/* Offset= 2 (110) */
/* 110 */	
			0x25,		/* FC_C_WSTRING */
			0x44,		/* FC_STRING_SIZED */
/* 112 */	0x28,		/* Corr desc:  parameter, FC_LONG */
			0x0,		/*  */
/* 114 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 116 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 118 */	
			0x11, 0x14,	/* FC_RP [alloced_on_stack] [pointer_deref] */
/* 120 */	NdrFcShort( 0x2 ),	/* Offset= 2 (122) */
/* 122 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 124 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */

			0x0
        }
    };

static const unsigned short RPC_VFS_FormatStringOffsetTable[] =
    {
    0,
    40,
    74,
    108,
    160,
    212,
    246,
    286,
    338,
    390,
    442,
    482,
    546,
    610,
    668,
    714,
    760,
    800,
    840,
    886,
    932,
    972,
    1012,
    1082,
    1146,
    1204,
    1256,
    1314,
    1372,
    1430,
    1482,
    1540,
    1598,
    1656,
    1708,
    1760,
    1806,
    1852,
    1892,
    1938,
    1978,
    2018,
    2064,
    2116,
    2162,
    2220,
    2266,
    2312,
    2358,
    2398,
    2438,
    2478,
    2518,
    2552,
    2598,
    2650,
    2702,
    2748
    };


static const MIDL_STUB_DESC RPC_VFS_StubDesc = 
    {
    (void *)& RPC_VFS___RpcServerInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    0,
    0,
    0,
    0,
    0,
    RPC_VFS__MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x70001f4, /* MIDL Version 7.0.500 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0
    };

static RPC_DISPATCH_FUNCTION RPC_VFS_table[] =
    {
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    0
    };
RPC_DISPATCH_TABLE RPC_VFS_v1_0_DispatchTable = 
    {
    58,
    RPC_VFS_table
    };

static const SERVER_ROUTINE RPC_VFS_ServerRoutineTable[] = 
    {
    (SERVER_ROUTINE)VFS_RPC_GetVersion,
    (SERVER_ROUTINE)VFS_RPC_PartialFlush,
    (SERVER_ROUTINE)VFS_RPC_FullFlush,
    (SERVER_ROUTINE)VFS_RPC_Open,
    (SERVER_ROUTINE)VFS_RPC_Create,
    (SERVER_ROUTINE)VFS_RPC_Close,
    (SERVER_ROUTINE)VFS_RPC_GetLastError,
    (SERVER_ROUTINE)VFS_RPC_VFSCreateFile,
    (SERVER_ROUTINE)VFS_RPC_VFSCreateFiledataFile,
    (SERVER_ROUTINE)VFS_RPC_VFSCreateUIDFile,
    (SERVER_ROUTINE)VFS_RPC_VFSCloseFile,
    (SERVER_ROUTINE)VFS_RPC_VFSRead,
    (SERVER_ROUTINE)VFS_RPC_VFSWrite,
    (SERVER_ROUTINE)VFS_RPC_VFSSeek,
    (SERVER_ROUTINE)VFS_RPC_VFSTell,
    (SERVER_ROUTINE)VFS_RPC_VFSSize,
    (SERVER_ROUTINE)VFS_RPC_VFSEndOfFile,
    (SERVER_ROUTINE)VFS_RPC_VFSSetEndOfFile,
    (SERVER_ROUTINE)VFS_RPC_VFSMemCache,
    (SERVER_ROUTINE)VFS_RPC_VFSGetUIDFromFileHandle,
    (SERVER_ROUTINE)VFS_RPC_VFSDeleteFiledataDirectory,
    (SERVER_ROUTINE)VFS_RPC_VFSDeleteFiledataFile,
    (SERVER_ROUTINE)VFS_RPC_VFSEnum,
    (SERVER_ROUTINE)VFS_RPC_VFSSingleEnum,
    (SERVER_ROUTINE)VFS_RPC_VFSEmbedFile,
    (SERVER_ROUTINE)VFS_RPC_VFSExtractFile,
    (SERVER_ROUTINE)VFS_RPC_VFSEmbedDir,
    (SERVER_ROUTINE)VFS_RPC_VFSExtractDir,
    (SERVER_ROUTINE)VFS_RPC_VFSEmbedFiledataFile,
    (SERVER_ROUTINE)VFS_RPC_VFSExtractFiledataFile,
    (SERVER_ROUTINE)VFS_RPC_VFSEmbedFiledataDir,
    (SERVER_ROUTINE)VFS_RPC_VFSExtractFiledataDir,
    (SERVER_ROUTINE)VFS_RPC_VFSQueryFileInfo,
    (SERVER_ROUTINE)VFS_RPC_VFSSetFileUserData,
    (SERVER_ROUTINE)VFS_RPC_VFSGetFileUserData,
    (SERVER_ROUTINE)VFS_RPC_VFSSetRuntimeUserData,
    (SERVER_ROUTINE)VFS_RPC_VFSGetRuntimeUserData,
    (SERVER_ROUTINE)VFS_RPC_VFSGetRootFiledata,
    (SERVER_ROUTINE)VFS_RPC_VFSGetFileData,
    (SERVER_ROUTINE)VFS_RPC_VFSFileExists,
    (SERVER_ROUTINE)VFS_RPC_VFSPathExists,
    (SERVER_ROUTINE)VFS_RPC_VFSFileSize,
    (SERVER_ROUTINE)VFS_RPC_VFSSetNoEnum,
    (SERVER_ROUTINE)VFS_RPC_VFSGetNoEnum,
    (SERVER_ROUTINE)VFS_RPC_VFSSetFileOption,
    (SERVER_ROUTINE)VFS_RPC_VFSGetFileUID,
    (SERVER_ROUTINE)VFS_RPC_VFSGetDirectoryUID,
    (SERVER_ROUTINE)VFS_RPC_VFSGetEntryUID,
    (SERVER_ROUTINE)VFS_RPC_VFSDeleteFile,
    (SERVER_ROUTINE)VFS_RPC_VFSDeleteDirectory,
    (SERVER_ROUTINE)VFS_RPC_VFSGetSettings,
    (SERVER_ROUTINE)VFS_RPC_VFSGetClusterSize,
    (SERVER_ROUTINE)VFS_RPC_DumpFTInfo,
    (SERVER_ROUTINE)VFS_RPC_TranslateErrorCode,
    (SERVER_ROUTINE)VFS_RPC_VFSGetMD5,
    (SERVER_ROUTINE)VFS_RPC_VFSGetFileDataMD5,
    (SERVER_ROUTINE)VFS_RPC_VFSQueryMD5,
    (SERVER_ROUTINE)VFS_RPC_VFSQueryFileDataMD5
    };

static const MIDL_SERVER_INFO RPC_VFS_ServerInfo = 
    {
    &RPC_VFS_StubDesc,
    RPC_VFS_ServerRoutineTable,
    RPC_VFS__MIDL_ProcFormatString.Format,
    RPC_VFS_FormatStringOffsetTable,
    0,
    0,
    0,
    0};
#pragma optimize("", on )
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

