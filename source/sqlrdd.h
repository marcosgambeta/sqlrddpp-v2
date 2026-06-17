// SQLRDD C Header
// Copyright (c) 2003 - Marcelo Lombardo  <lombardo@uol.com.br>

// $BEGIN_LICENSE$
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software; see the file COPYING.  If not, write to
// the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
// Boston, MA 02111-1307 USA (or visit the web site http://www.gnu.org/).
//
// As a special exception, the xHarbour Project gives permission for
// additional uses of the text contained in its release of xHarbour.
//
// The exception is that, if you link the xHarbour libraries with other
// files to produce an executable, this does not by itself cause the
// resulting executable to be covered by the GNU General Public License.
// Your use of that executable is in no way restricted on account of
// linking the xHarbour library code into it.
//
// This exception does not however invalidate any other reasons why
// the executable file might be covered by the GNU General Public License.
//
// This exception applies only to the code released by the xHarbour
// Project under the name xHarbour.  If you copy code from other
// xHarbour Project or Free Software Foundation releases into a copy of
// xHarbour, as the General Public License permits, the exception does
// not apply to the code that you add in this way.  To avoid misleading
// anyone as to the status of such modified files, you must delete
// this exception notice from them.
//
// If you write modifications of your own for xHarbour, it is your choice
// whether to permit this exception to apply to your modifications.
// If you do not wish that, delete this exception notice.
// $END_LICENSE$

#ifndef SQLRDD_H
#define SQLRDD_H

// TODO: to be revised
#if _MSC_VER >= 1900
#pragma comment(lib, "legacy_stdio_definitions.lib")
#endif

#include "compat.h"
#include <hbsetup.h>
#include <hbapi.h>
#include <hbapirdd.h>
#include <hbapiitm.h>
#include "sqlrdd.ch"

#define SUPERTABLE (&sqlrddSuper)

#ifdef __XHARBOUR__
#define HB_LONG LONG
#define HB_ULONG ULONG
#endif

// SQL WORKAREA

struct SQLAREA
{
  AREA area;

  //  SQLRDD's additions to the workarea structure
  //
  //  Warning: The above section MUST match WORKAREA exactly!  Any
  //  additions to the structure MUST be added below

  // Area's codepage convert pointer
  PHB_CODEPAGE cdPageCnv;
  // file name
  char *szDataFileName;
  // current index order
  HB_LONG hOrdCurrent;
  //
  bool shared;
  // only SELECT allowed
  bool readonly;
  // true when creating table
  bool creating;
  // true when workarea was not used yet
  bool firstinteract;
  // ISAM Simulator ?
  bool isam;
  //
  bool wasdel;
  // Workarea Initialization done
  bool initialized;
  // SET FILTER converted to SQL
  bool sqlfilter;

  // SQL Workarea object
  PHB_ITEM oWorkArea;
  // Status array
  PHB_ITEM aInfo;
  // Record buffer
  PHB_ITEM aBuffer;
  // Indexes
  PHB_ITEM aOrders;
  // Table xBase structure
  PHB_ITEM aStruct;
  // Locked lines
  PHB_ITEM aLocked;
  // Structure received by dbCreate()
  PHB_ITEM aCreate;
  // Workarea recordset cache
  PHB_ITEM aCache;
  // Last workarea buffer
  PHB_ITEM aOldBuffer;
  // Empty buffer to be in eof()+1
  PHB_ITEM aEmptyBuff;
  //
  PHB_ITEM aSelectList;

  // Recno position in field list
  HB_ULONG ulhRecno;
  // Deleted position in field list
  HB_ULONG ulhDeleted;

  // Field offset in fields array
  int *uiBufferIndex;
  // Keeps a field list for SELECT statements
  int *uiFieldList;
  // field list status - see sqlprototypes.h
  int iFieldListStatus;

  // Pointer to parent rel struct
  LPDBRELINFO lpdbPendingRel;
  // Flags if a column was updated - must be cleared on every GO_COLD - USED BY ODBCRDD
  char editMask[MAX_FIELDS];
};

//using SQLAREA = _SQLAREA; (deprecated)
//using LPSQLAREA = SQLAREA *; (deprecated)

// deprecated
//#ifndef SQLAREAP
//#define SQLAREAP LPSQLAREA
//#endif

// prototypes

namespace SQLRDD {
void commonError(AREAP ThisDb, const HB_USHORT uiGenCode, const HB_USHORT uiSubCode, const char *filename);
}

#endif // SQLRDD_H
