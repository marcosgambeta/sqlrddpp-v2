//
// SQLRDD Main File
// Copyright (c) 2003 - Marcelo Lombardo  <lombardo@uol.com.br>
//

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

#include "compat.h"
#include <hbapilng.h>

#include "sqlrdd.h"
#include "msg.ch"
#include <rddsys.ch>
#include "sqlrddsetup.ch"
#include "sqlprototypes.h"

// #undef HB_IS_OBJECT
// #define HB_IS_OBJECT(p)  ( ( p ) && HB_IS_OF_TYPE(p, Harbour::Item::OBJECT) && ( p )->item.asArray.value->uiClass !=
// 0 )

#include <ctype.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#undef HB_TRACE
#define HB_TRACE(x, y)

static RDDFUNCS sqlrddSuper;

static void startSQLRDDSymbols(void);

static bool ProcessFields(SQLAREAP ThisDb);
static bool SetFields(SQLAREAP ThisDb);

/*
static PHB_ITEM loadTag(SQLAREAP thiswa, LPDBORDERINFO pInfo, HB_LONG * lorder);
*/
HB_EXTERN_BEGIN
PHB_ITEM loadTagDefault(SQLAREAP thiswa, LPDBORDERINFO pInfo, HB_LONG *lorder);
HB_EXTERN_END

HB_FUNC_EXTERN(SR_END);
HB_FUNC_EXTERN(SR_INIT);
HB_FUNC_EXTERN(__SR_STARTSQL);

/*------------------------------------------------------------------------*/

static PHB_DYNS s_pSym_SQLGOBOTTOM;
static PHB_DYNS s_pSym_SQLGOTO;
static PHB_DYNS s_pSym_SQLGOTOP;
static PHB_DYNS s_pSym_SQLSEEK;
static PHB_DYNS s_pSym_SETBOF;
static PHB_DYNS s_pSym_SQLDELETEREC;
static PHB_DYNS s_pSym_SQLFLUSH;
static PHB_DYNS s_pSym_SQLRECALL;
static PHB_DYNS s_pSym_SQLCLOSE;
static PHB_DYNS s_pSym_SQLCREATE;
static PHB_DYNS s_pSym_SQLOPEN;
static PHB_DYNS s_pSym_SQLOPENALLINDEXES;
static PHB_DYNS s_pSym_SQLPACK;
static PHB_DYNS s_pSym_SQLZAP;
static PHB_DYNS s_pSym_SQLORDERLISTADD;
static PHB_DYNS s_pSym_SQLORDERLISTCLEAR;
static PHB_DYNS s_pSym_SQLORDERLISTFOCUS;
static PHB_DYNS s_pSym_SQLORDERCREATE;
static PHB_DYNS s_pSym_SQLORDERDESTROY;
static PHB_DYNS s_pSym_SQLORDERCONDITION;
static PHB_DYNS s_pSym_SQLORDERLISTNUM;
static PHB_DYNS s_pSym_SQLSETSCOPE;
static PHB_DYNS s_pSym_SQLLOCK;
static PHB_DYNS s_pSym_SQLUNLOCK;
static PHB_DYNS s_pSym_SQLDROP;
static PHB_DYNS s_pSym_SQLEXISTS;
static PHB_DYNS s_pSym_SQLKEYCOUNT;
static PHB_DYNS s_pSym_SQLRECSIZE;
static PHB_DYNS s_pSym_WRITEBUFFER;
static PHB_DYNS s_pSym_READPAGE;
static PHB_DYNS s_pSym_STABILIZE;
static PHB_DYNS s_pSym_NORMALIZE;
static PHB_DYNS s_pSym_SQLGETVALUE;
static PHB_DYNS s_pSym_SQLSETFILTER;
static PHB_DYNS s_pSym_SQLCLEARFILTER;
static PHB_DYNS s_pSym_SQLFILTERTEXT;

static PHB_DYNS s_pSym_SQLINIT = nullptr;
static PHB_DYNS s_pSym_SQLEXIT = nullptr;
static PHB_DYNS s_pSym_WORKAREA = nullptr;

/*------------------------------------------------------------------------*/

static void fixCachePointer(HB_LONG *lPosCache)
{
  if (*lPosCache < 1) {
    *lPosCache += (CAHCE_PAGE_SIZE * 3);
  } else if (*lPosCache > (CAHCE_PAGE_SIZE * 3)) {
    *lPosCache -= (CAHCE_PAGE_SIZE * 3);
  }
}

/*------------------------------------------------------------------------*/

HB_FUNC(SR_FIXCACHEPOINTER)
{
  auto lPos = hb_parnl(1);
  fixCachePointer(&lPos);
  hb_retnl(lPos);
}

/*------------------------------------------------------------------------*/

static bool isCachePointerInRange(HB_LONG lPosCache, HB_LONG lBegin, HB_LONG lEnd)
{
  if (lBegin == lEnd) {
    return lPosCache == lBegin;
  }
  if (lBegin < lEnd) {
    return (lPosCache >= lBegin && lPosCache <= lEnd);
  }
  return (lPosCache >= lBegin || lPosCache <= lEnd);
}

/*------------------------------------------------------------------------*/

static HB_LONG searchCacheFWD(SQLAREAP thiswa, HB_LONG lPreviousCacheStatus)
{
  auto lPosCache = hb_arrayGetNL(thiswa->aInfo, AINFO_NPOSCACHE);

  if (!lPosCache) {
    return 0;
  }

  auto lBegin = hb_arrayGetNL(thiswa->aInfo, AINFO_NCACHEBEGIN);
  auto lEnd = hb_arrayGetNL(thiswa->aInfo, AINFO_NCACHEEND);

  if (lPreviousCacheStatus) {
    lPosCache++;
  }
  fixCachePointer(&lPosCache);

  while (isCachePointerInRange(lPosCache, lBegin, lEnd)) {
    if (!HB_IS_NIL(hb_arrayGetItemPtr(thiswa->aCache, lPosCache))) {
      return lPosCache;
    }
    lPosCache++;
    fixCachePointer(&lPosCache);
  }
  return 0;
}

/*------------------------------------------------------------------------*/

static HB_LONG searchCacheBWD(SQLAREAP thiswa, HB_LONG lPreviousCacheStatus)
{
  auto lPosCache = hb_arrayGetNL(thiswa->aInfo, AINFO_NPOSCACHE);

  if (!lPosCache) {
    return 0;
  }

  auto lBegin = hb_arrayGetNL(thiswa->aInfo, AINFO_NCACHEBEGIN);
  auto lEnd = hb_arrayGetNL(thiswa->aInfo, AINFO_NCACHEEND);

  if (lPreviousCacheStatus) {
    lPosCache--;
  }
  fixCachePointer(&lPosCache);

  while (isCachePointerInRange(lPosCache, lBegin, lEnd)) {
    if (!HB_IS_NIL(hb_arrayGetItemPtr(thiswa->aCache, lPosCache))) {
      return lPosCache;
    }
    lPosCache--;
    fixCachePointer(&lPosCache);
  }
  return 0;
}

/*------------------------------------------------------------------------*/

static void readCachePageFWD(SQLAREAP thiswa)
{
  auto pOrd = hb_itemNew(nullptr);
  auto pDel = hb_itemNew(nullptr);
  hb_itemPutNL(pOrd, ORD_DIR_FWD);
  hb_itemPutL(pDel, thiswa->wasdel);
  hb_objSendMessage(thiswa->oWorkArea, s_pSym_READPAGE, 2, pOrd, pDel);
  hb_itemRelease(pOrd);
  hb_itemRelease(pDel);
}

/*------------------------------------------------------------------------*/

static void readCachePageBWD(SQLAREAP thiswa)
{
  auto pOrd = hb_itemNew(nullptr);
  auto pDel = hb_itemNew(nullptr);
  hb_itemPutNL(pOrd, ORD_DIR_BWD);
  hb_itemPutL(pDel, thiswa->wasdel);
  hb_objSendMessage(thiswa->oWorkArea, s_pSym_READPAGE, 2, pOrd, pDel);
  hb_itemRelease(pOrd);
  hb_itemRelease(pDel);
}

/*------------------------------------------------------------------------*/

static void setCurrentFromCache(SQLAREAP thiswa, HB_LONG lPos)
{
  HB_SIZE nPos, nLen;

  hb_arraySetNL(thiswa->aInfo, AINFO_NPOSCACHE, lPos);

  auto pCacheRecord = hb_arrayGetItemPtr(thiswa->aCache, lPos);

  auto pCol = hb_itemNew(nullptr);

  for (nPos = 1, nLen = hb_arrayLen(pCacheRecord); nPos <= nLen; nPos++) {

    hb_arrayGet(pCacheRecord, nPos, pCol);
    hb_arraySet(thiswa->aOldBuffer, nPos, pCol);

    if (nPos == thiswa->ulhRecno) {
      hb_arraySet(thiswa->aInfo, AINFO_RECNO, pCol);
    }

    if (nPos == thiswa->ulhDeleted) {
      auto deleted = hb_itemGetCPtr(pCol);
      if (*deleted == 'T' || *deleted == '*') {
        hb_arraySetL(thiswa->aInfo, AINFO_DELETED, true);
      } else {
        hb_arraySetL(thiswa->aInfo, AINFO_DELETED, false);
      }
    }
    hb_arraySetForward(thiswa->aBuffer, nPos, pCol);
  }

  if (thiswa->ulhDeleted == 0) {
    hb_arraySetL(thiswa->aInfo, AINFO_DELETED, false);
  }

  hb_itemRelease(pCol);
}

/*------------------------------------------------------------------------*/

static void sqlGetBufferFromCache2(SQLAREAP thiswa, HB_LONG lPos)
{
  HB_SIZE nPos, nLen;

  auto pCacheRecord = hb_arrayGetItemPtr(thiswa->aCache, lPos);

  auto pCol = hb_itemNew(nullptr);

  for (nPos = 1, nLen = hb_arrayLen(pCacheRecord); nPos <= nLen; nPos++) {
    hb_arrayGet(pCacheRecord, nPos, pCol);
    hb_arraySet(thiswa->aOldBuffer, nPos, pCol);
    if (nPos == thiswa->ulhRecno) {
      hb_arraySet(thiswa->aInfo, AINFO_RECNO, pCol);
    }
    if (nPos == thiswa->ulhDeleted) {
      auto deleted = hb_itemGetCPtr(pCol);
      if (*deleted == 'T' || *deleted == '*') {
        hb_arraySetL(thiswa->aInfo, AINFO_DELETED, true);
      } else {
        hb_arraySetL(thiswa->aInfo, AINFO_DELETED, false);
      }
    }
    hb_arraySetForward(thiswa->aBuffer, nPos, pCol);
  }

  if (thiswa->ulhDeleted == 0) {
    hb_arraySetL(thiswa->aInfo, AINFO_DELETED, false);
  }

  hb_itemRelease(pCol);
  hb_arraySetNL(thiswa->aInfo, AINFO_NPOSCACHE, lPos);
}

/*------------------------------------------------------------------------*/

static void sqlGetCleanBuffer(SQLAREAP thiswa)
{
  HB_SIZE nPos, nLen;

  auto pCol = hb_itemNew(nullptr);
  for (nPos = 1, nLen = hb_arrayLen(thiswa->aEmptyBuff); nPos <= nLen; nPos++) {
    hb_arrayGet(thiswa->aEmptyBuff, nPos, pCol);
    hb_arraySet(thiswa->aOldBuffer, nPos, pCol);
    hb_arraySetForward(thiswa->aBuffer, nPos, pCol);
  }
  hb_arraySetL(thiswa->aInfo, AINFO_DELETED, false);
  hb_arraySetL(thiswa->aInfo, AINFO_EOF, true);
  hb_arrayGet(thiswa->aInfo, AINFO_RCOUNT, pCol);
  hb_itemPutNL(pCol, hb_itemGetNL(pCol) + 1);
  hb_arraySetForward(thiswa->aInfo, AINFO_RECNO, pCol);
  hb_itemRelease(pCol);
  if (!thiswa->isam) {
    hb_arraySetNL(thiswa->aInfo, AINFO_NPOSCACHE, static_cast<long>(hb_arrayLen(thiswa->aCache)) + 1);
  }
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlBof(SQLAREAP thiswa, HB_BOOL *bof) // RDDFUNC
{
  if (thiswa->firstinteract) {
    SELF_GOTOP(&thiswa->area);
    thiswa->firstinteract = false;
  }

  if (thiswa->lpdbPendingRel) {
    SELF_FORCEREL(&thiswa->area);
  }
  thiswa->area.fBof = hb_arrayGetL(thiswa->aInfo, AINFO_BOF);
  *bof = thiswa->area.fBof;

  // sr_TraceLog(nullptr, "sqlBof, returning %i\n", thiswa->area.fBof);

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlEof(SQLAREAP thiswa, HB_BOOL *eof) // RDDFUNC
{
  if (thiswa->firstinteract) {
    SELF_GOTOP(&thiswa->area);
    thiswa->firstinteract = false;
  }

  if (thiswa->lpdbPendingRel)
    SELF_FORCEREL(&thiswa->area);

  if (hb_arrayGetL(thiswa->aInfo, AINFO_ISINSERT) && hb_arrayGetL(thiswa->aInfo, AINFO_HOT)) {
    *eof = false;
  } else {
    thiswa->area.fEof = hb_arrayGetL(thiswa->aInfo, AINFO_EOF);
    *eof = thiswa->area.fEof;
  }

  // sr_TraceLog(nullptr, "sqlEof, returning %i\n", thiswa->area.fEof);

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlFound(SQLAREAP thiswa, HB_BOOL *found) // RDDFUNC
{
  if (thiswa->lpdbPendingRel) {
    SELF_FORCEREL(&thiswa->area);
  }

  thiswa->area.fFound = hb_arrayGetL(thiswa->aInfo, AINFO_FOUND);
  *found = thiswa->area.fFound;

  // sr_TraceLog(nullptr, "sqlFound, returning %i\n", thiswa->area.fFound);

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlGoBottom(SQLAREAP thiswa) // RDDFUNC
{
  auto eofat = hb_itemNew(nullptr);

  // sr_TraceLog(nullptr, "sqlGoBottom\n");

  thiswa->lpdbPendingRel = nullptr;
  thiswa->firstinteract = false;
  thiswa->wasdel = false;

  if (hb_arrayGetL(thiswa->aInfo, AINFO_HOT)) {
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_WRITEBUFFER, 0);
  }

  auto leof = hb_arrayGetNL(thiswa->aInfo, AINFO_EOF_AT);

  if (hb_arrayGetNL(thiswa->aInfo, AINFO_EOF_AT)) {
    hb_itemPutNL(eofat, leof);
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLGOTO, 1, eofat);
  } else {
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLGOBOTTOM, 0);
  }

  thiswa->area.fTop = false;
  thiswa->area.fBottom = true;
  thiswa->area.fEof = hb_arrayGetL(thiswa->aInfo, AINFO_EOF);
  thiswa->area.fBof = hb_arrayGetL(thiswa->aInfo, AINFO_BOF);
  hb_itemRelease(eofat);

  SELF_SKIPFILTER(&thiswa->area, -1);

  if (thiswa->area.lpdbRelations) {
    return SELF_SYNCCHILDREN(&thiswa->area);
  } else {
    return Harbour::SUCCESS;
  }
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlGoTo(SQLAREAP thiswa, HB_LONG recno) // RDDFUNC
{
  // sr_TraceLog(nullptr, "sqlGoTo %i\n", recno);

  /* Reset parent rel struct */
  thiswa->lpdbPendingRel = nullptr;
  thiswa->firstinteract = false;
  thiswa->wasdel = false;

  auto pParam1 = hb_itemPutNL(nullptr, recno);
  if (hb_arrayGetL(thiswa->aInfo, AINFO_HOT)) {
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_WRITEBUFFER, 0);
  }
  hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLGOTO, 1, pParam1);
  hb_itemRelease(pParam1);

  thiswa->area.fEof = hb_arrayGetL(thiswa->aInfo, AINFO_EOF);
  thiswa->area.fBof = hb_arrayGetL(thiswa->aInfo, AINFO_BOF);

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlGoToId(SQLAREAP thiswa, PHB_ITEM pItem) // RDDFUNC
{

  HB_TRACE(HB_TR_DEBUG, ("sqlGoToId1(%p, %p)", thiswa, pItem));

  // sr_TraceLog(nullptr, "sqlGoToId\n");

  thiswa->firstinteract = false;
  thiswa->wasdel = false;

  if (HB_IS_NUMERIC(pItem)) {
    return SELF_GOTO(&thiswa->area, hb_itemGetNL(pItem));
  } else {
    if (hb_arrayGetL(thiswa->aInfo, AINFO_HOT)) {
      hb_objSendMessage(thiswa->oWorkArea, s_pSym_WRITEBUFFER, 0);
    }

    hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLGOTO, 1, pItem);

    thiswa->area.fEof = hb_arrayGetL(thiswa->aInfo, AINFO_EOF);
    thiswa->area.fBof = hb_arrayGetL(thiswa->aInfo, AINFO_BOF);

    return Harbour::SUCCESS;
  }
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlGoTop(SQLAREAP thiswa) // RDDFUNC
{
  // sr_TraceLog(nullptr, "sqlGoTop\n");

  thiswa->lpdbPendingRel = nullptr;
  thiswa->firstinteract = false;
  thiswa->wasdel = false;

  if (hb_arrayGetL(thiswa->aInfo, AINFO_HOT)) {
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_WRITEBUFFER, 0);
  }

  auto lbof = hb_arrayGetNL(thiswa->aInfo, AINFO_BOF_AT);

  if (lbof) {
    auto pBOF = hb_itemPutNL(nullptr, lbof);
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLGOTO, 1, pBOF);
    hb_itemRelease(pBOF);
  } else {
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLGOTOP, 0);
  }

  thiswa->area.fTop = true;
  thiswa->area.fBottom = false;
  thiswa->area.fEof = hb_arrayGetL(thiswa->aInfo, AINFO_EOF);
  thiswa->area.fBof = hb_arrayGetL(thiswa->aInfo, AINFO_BOF);

  SELF_SKIPFILTER(&thiswa->area, 1);

  if (thiswa->area.lpdbRelations) {
    return SELF_SYNCCHILDREN(&thiswa->area);
  } else {
    return Harbour::SUCCESS;
  }
}

/*------------------------------------------------------------------------*/

int sqlKeyCompare(AREAP thiswa, PHB_ITEM pKey, HB_BOOL fExact) // TODO: static ?
{
  auto lorder = 0L;
  PHB_ITEM pTag, pKeyVal, itemTemp;
  int iLimit, iResult = 0;
  HB_BYTE len1, len2;
  char *valbuf = nullptr;
  const char *val1, *val2;

  // sr_TraceLog(nullptr, "sqlKeyCompare\n");

  pTag = loadTagDefault(reinterpret_cast<SQLAREAP>(thiswa), nullptr, &lorder);
  if (pTag) {
    if ((reinterpret_cast<SQLAREAP>(thiswa))->firstinteract) {
      SELF_GOTOP(static_cast<AREAP>(thiswa));
      (reinterpret_cast<SQLAREAP>(thiswa))->firstinteract = false;
    }
    itemTemp = hb_itemArrayGet(pTag, INDEX_KEY_CODEBLOCK);
    if (HB_IS_NUMBER(itemTemp)) {
      pKeyVal =
          hb_itemArrayGet((reinterpret_cast<SQLAREAP>(thiswa))->aBuffer, hb_arrayGetNL(pTag, INDEX_KEY_CODEBLOCK));
      len1 = static_cast<HB_BYTE>(hb_strRTrimLen(hb_itemGetCPtr(pKeyVal), hb_itemGetCLen(pKeyVal), false)) - 15;
      val1 = hb_itemGetCPtr(pKeyVal);
    } else {
      HB_EVALINFO info;
      hb_evalNew(&info, hb_itemArrayGet(pTag, INDEX_KEY_CODEBLOCK));
      pKeyVal = hb_evalLaunch(&info);
      hb_evalRelease(&info);
      len1 = static_cast<HB_BYTE>(hb_itemGetCLen(pKeyVal));
      val1 = hb_itemGetCPtr(pKeyVal);
    }
    hb_itemRelease(itemTemp);
    hb_itemRelease(pTag);
  } else {
    return 0;
  }

  if (HB_IS_DATE(pKey)) {
    len2 = 8;
    valbuf = static_cast<char *>(hb_xgrab(9));
    val2 = hb_itemGetDS(pKey, valbuf);
  } else if (HB_IS_NUMBER(pKey)) {
    auto pLen = hb_itemPutNL(nullptr, static_cast<HB_LONG>(len1));
    val2 = valbuf = hb_itemStr(pKey, pLen, nullptr);
    len2 = static_cast<HB_BYTE>(strlen(val2));
    hb_itemRelease(pLen);
  } else if (HB_IS_LOGICAL(pKey)) {
    len2 = 1;
    val2 = hb_itemGetL(pKey) ? "T" : "F";
  } else {
    len2 = static_cast<HB_BYTE>(hb_itemGetCLen(pKey));
    val2 = hb_itemGetCPtr(pKey);
  }

  iLimit = (len1 > len2) ? len2 : len1;

  if (HB_IS_STRING(pKeyVal)) {
    if (iLimit > 0) {
      iResult = memcmp(val1, val2, iLimit);
    }

    if (iResult == 0) {
      if (len1 >= len2) {
        iResult = 1;
      } else if (len1 < len2 && fExact) {
        iResult = -1;
      }
    } else {
      iResult = 0;
    }
  } else {
    if (iLimit == 0 || (iResult = memcmp(val1, val2, iLimit)) == 0) {
      if (len1 >= len2) {
        iResult = 1;
      } else if (len1 < len2) {
        iResult = -1;
      }
    }
  }

  if (valbuf) {
    hb_xfree(valbuf);
  }

  hb_itemRelease(pKeyVal);

  return iResult;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlSeek(SQLAREAP thiswa, HB_BOOL bSoftSeek, PHB_ITEM pKey, HB_BOOL bFindLast) // RDDFUNC
{
  PHB_ITEM pNewKey = nullptr;
  HB_ERRCODE retvalue = Harbour::SUCCESS;

  // sr_TraceLog(nullptr, "sqlSeek(%p, %d, %p, %d)", thiswa, bSoftSeek, pKey, bFindLast);

  thiswa->lpdbPendingRel = nullptr;
  thiswa->firstinteract = false;
  thiswa->wasdel = false;

  auto pItem = hb_itemPutL(nullptr, bSoftSeek);
  auto pItem2 = hb_itemPutL(nullptr, bFindLast);

#ifndef HB_CDP_SUPPORT_OFF
  if (HB_IS_STRING(pKey)) {
    PHB_CODEPAGE cdpSrc = thiswa->cdPageCnv ? thiswa->cdPageCnv : hb_vmCDP();
    if (thiswa->area.cdPage && thiswa->area.cdPage != cdpSrc) {
      HB_SIZE nLen = hb_itemGetCLen(pKey);
      char *pszVal = hb_cdpnDup(hb_itemGetCPtr(pKey), &nLen, cdpSrc, thiswa->area.cdPage);
      pKey = pNewKey = hb_itemPutCLPtr(nullptr, pszVal, nLen);
    }
  }
#endif

  hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLSEEK, 3, pKey, pItem, pItem2);

  thiswa->area.fFound = hb_arrayGetL(thiswa->aInfo, AINFO_FOUND);
  thiswa->area.fBof = hb_arrayGetL(thiswa->aInfo, AINFO_BOF);
  thiswa->area.fEof = hb_arrayGetL(thiswa->aInfo, AINFO_EOF);

  if ((hb_setGetDeleted() || thiswa->area.dbfi.itmCobExpr != nullptr) && !thiswa->area.fEof) {
    retvalue = SELF_SKIPFILTER(&thiswa->area, (bFindLast ? -1 : 1));

    if (thiswa->area.fEof) {
      thiswa->area.fFound = false;
      hb_itemPutL(pItem, thiswa->area.fFound);
      hb_arraySetForward(thiswa->aInfo, AINFO_FOUND, pItem);
    } else {
      if (sqlKeyCompare(&thiswa->area, pKey, false) != 0) {
        thiswa->area.fFound = true;
        hb_itemPutL(pItem, thiswa->area.fFound);
        hb_arraySetForward(thiswa->aInfo, AINFO_FOUND, pItem);
      } else {
        thiswa->area.fFound = false;
        hb_itemPutL(pItem, thiswa->area.fFound);
        hb_arraySetForward(thiswa->aInfo, AINFO_FOUND, pItem);

        if (!bSoftSeek) {
          sqlGetCleanBuffer(thiswa);
        }
      }
    }
  }

  hb_itemRelease(pItem);
  hb_itemRelease(pItem2);
  if (pNewKey) {
    hb_itemRelease(pNewKey);
  }

  if (thiswa->area.lpdbRelations && retvalue == Harbour::SUCCESS) {
    return SELF_SYNCCHILDREN(&thiswa->area);
  } else {
    return retvalue;
  }
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlSkip(SQLAREAP thiswa, HB_LONG lToSkip) // RDDFUNC
{
  HB_ERRCODE ret;

  // sr_TraceLog(nullptr, "sqlSkip %i\n", lToSkip);

  if (thiswa->lpdbPendingRel) {
    SELF_FORCEREL(&thiswa->area);
  } else if (thiswa->firstinteract) {
    SELF_GOTOP(&thiswa->area);
    thiswa->firstinteract = false;
  }

  if (SELF_GOCOLD(&thiswa->area) == Harbour::FAILURE) {
    return Harbour::FAILURE;
  }

  thiswa->area.fTop = thiswa->area.fBottom = false;

  ret = SUPER_SKIP(&thiswa->area, lToSkip); // This will call SKIPRAW

  hb_arraySetL(thiswa->aInfo, AINFO_BOF, thiswa->area.fBof);
  hb_arraySetL(thiswa->aInfo, AINFO_EOF, thiswa->area.fEof);
  thiswa->wasdel = false;

  return ret;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlSkipFilter(SQLAREAP thiswa, HB_LONG lUpDown) // RDDFUNC
{
  bool bOutOfRange;
  HB_BOOL bDeleted;
  PHB_ITEM pResult;
  HB_ERRCODE uiError;

  // sr_TraceLog(nullptr, "sqlSkipFilter %i\n", lUpDown);

  if (!hb_setGetDeleted() && thiswa->area.dbfi.itmCobExpr == nullptr) {
    return Harbour::SUCCESS;
  }

  lUpDown = (lUpDown > 0 ? 1 : -1);
  bOutOfRange = false;

  while (true) {
    if (thiswa->area.fBof || thiswa->area.fEof) {
      bOutOfRange = true;
      break;
    }

    /* SET FILTER TO */
    if (thiswa->area.dbfi.itmCobExpr) {
      pResult = hb_vmEvalBlock(thiswa->area.dbfi.itmCobExpr);
      if (HB_IS_LOGICAL(pResult) && !hb_itemGetL(pResult)) {
        SELF_SKIPRAW(&thiswa->area, lUpDown);
        continue;
      }
    }

    /* SET DELETED */
    if (hb_setGetDeleted()) {
      SELF_DELETED(&thiswa->area, &bDeleted);
      if (bDeleted) {
        SELF_SKIPRAW(&thiswa->area, lUpDown);
        continue;
      }
    }
    break;
  }

  if (bOutOfRange) {
    if (lUpDown < 0) {
      thiswa->area.fEof = false;
      uiError = SELF_GOTOP(&thiswa->area);
      hb_objSendMessage(thiswa->oWorkArea, s_pSym_SETBOF, 0);
      thiswa->area.fBof = true;
    } else {
      thiswa->area.fBof = false;
      uiError = Harbour::SUCCESS;
    }
  } else {
    uiError = Harbour::SUCCESS;
  }

  return uiError;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE ConcludeSkipraw(SQLAREAP thiswa, HB_LONG lToSkip)
{
  /*
     if( lToSkip != 0 ) {
        thiswa->area.fBof = hb_arrayGetL(thiswa->aInfo, AINFO_BOF);
        thiswa->area.fEof = hb_arrayGetL(thiswa->aInfo, AINFO_EOF);
     }
  */
  HB_SYMBOL_UNUSED(lToSkip);

  /* Force relational movement in child WorkAreas */

  if (thiswa->area.lpdbRelations) {
    return SELF_SYNCCHILDREN(&thiswa->area);
  } else {
    return Harbour::SUCCESS;
  }
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlSkipRaw(SQLAREAP thiswa, HB_LONG lToSkip) // RDDFUNC
{
  bool bEof, bBof;
  PHB_ITEM pToSkip;

  // sr_TraceLog(nullptr, "sqlSkipRaw %i\n", lToSkip);

  bEof = hb_arrayGetL(thiswa->aInfo, AINFO_EOF);
  bBof = hb_arrayGetL(thiswa->aInfo, AINFO_BOF);

  thiswa->area.fFound = false;

  if ((bEof && bBof) && lToSkip == -1) {
    return SELF_GOBOTTOM(&thiswa->area); // <===========| RETURNING
  }

  if ((bEof && bBof) || lToSkip == 0) {
    // Table is empty
    thiswa->area.fEof = bEof;
    thiswa->area.fBof = bBof;
    return Harbour::SUCCESS; // <===========| RETURNING
  }

  hb_arraySetNL(thiswa->aInfo, AINFO_SKIPCOUNT, hb_arrayGetNL(thiswa->aInfo, AINFO_SKIPCOUNT) + lToSkip);

  if (thiswa->isam) {
    HB_LONG lFound, lPreviousCacheStatus;
    bool bCurrentDeleted;

    auto lPosCache = hb_arrayGetNL(thiswa->aInfo, AINFO_NPOSCACHE);
    bCurrentDeleted = lPosCache && HB_IS_NIL(hb_arrayGetItemPtr(thiswa->aCache, lPosCache));

    if (lToSkip > 0) {
      if (bEof) {
        return Harbour::SUCCESS; // <===========| RETURNING
      }
      if (iTemCompEqual(hb_arrayGetItemPtr(thiswa->aInfo, AINFO_EOF_AT),
                        hb_arrayGetItemPtr(thiswa->aInfo, AINFO_RECNO))) {
        hb_arraySetL(thiswa->aInfo, AINFO_EOF, true);
        thiswa->area.fEof = true;
        sqlGetCleanBuffer(thiswa);
        return ConcludeSkipraw(thiswa, lToSkip);
      }

      lFound = searchCacheFWD(thiswa, 1);

      if (lFound) {
        setCurrentFromCache(thiswa, lFound);
        thiswa->area.fBof = false;
        return ConcludeSkipraw(thiswa, lToSkip);
      }
      lPreviousCacheStatus = lPosCache;
      readCachePageFWD(thiswa);
      lFound = searchCacheFWD(thiswa, lPreviousCacheStatus);
      if (lFound) {
        setCurrentFromCache(thiswa, lFound);
        thiswa->area.fBof = false;
        return ConcludeSkipraw(thiswa, lToSkip);
      }
      hb_arraySetL(thiswa->aInfo, AINFO_EOF, true);
      thiswa->area.fEof = true;
      sqlGetCleanBuffer(thiswa);
      return ConcludeSkipraw(thiswa, lToSkip);
    } else { // lToSkip < 0
      if (bCurrentDeleted) {
        if (!bBof) {
          if (bEof) {
            SELF_GOBOTTOM(&thiswa->area);
            return ConcludeSkipraw(thiswa, lToSkip);
          }
          lFound = searchCacheBWD(thiswa, 1);
          if (lFound) {
            setCurrentFromCache(thiswa, lFound);
            return ConcludeSkipraw(thiswa, lToSkip);
          }
          lPreviousCacheStatus = lPosCache;
          readCachePageBWD(thiswa);
          lFound = searchCacheBWD(thiswa, lPreviousCacheStatus);
          if (lFound) {
            setCurrentFromCache(thiswa, lFound);
            return ConcludeSkipraw(thiswa, lToSkip);
          }
        }
        lFound = searchCacheFWD(thiswa, 1);
        if (lFound) {
          setCurrentFromCache(thiswa, lFound);
          hb_arraySetL(thiswa->aInfo, AINFO_BOF, true);
          thiswa->area.fBof = true;
          return ConcludeSkipraw(thiswa, lToSkip);
        }
        readCachePageFWD(thiswa);
        lFound = searchCacheFWD(thiswa, 1);
        if (lFound) {
          setCurrentFromCache(thiswa, lFound);
          hb_arraySetL(thiswa->aInfo, AINFO_BOF, true);
          thiswa->area.fBof = true;
          return ConcludeSkipraw(thiswa, lToSkip);
        }
        hb_arraySetL(thiswa->aInfo, AINFO_BOF, true);
        thiswa->area.fBof = true;
        hb_arraySetL(thiswa->aInfo, AINFO_EOF, true);
        thiswa->area.fEof = true;
        sqlGetCleanBuffer(thiswa);
      } else {
        if (bBof) {
          return ConcludeSkipraw(thiswa, lToSkip);
        }
        if (iTemCompEqual(hb_arrayGetItemPtr(thiswa->aInfo, AINFO_BOF_AT),
                          hb_arrayGetItemPtr(thiswa->aInfo, AINFO_RECNO))) {
          // Checking optimizer
          hb_arraySetL(thiswa->aInfo, AINFO_BOF, true);
          thiswa->area.fBof = true;
          return ConcludeSkipraw(thiswa, lToSkip);
        }
        if (bEof) {
          if (iTemCompEqual(hb_arrayGetItemPtr(thiswa->aInfo, AINFO_EOF_AT),
                            hb_arrayGetItemPtr(thiswa->aInfo, AINFO_RECNO))) {
            SELF_GOTOID(&thiswa->area, hb_arrayGetItemPtr(thiswa->aInfo, AINFO_EOF_AT));
          } else {
            SELF_GOBOTTOM(&thiswa->area);
          }
          return ConcludeSkipraw(thiswa, lToSkip);
        }
        lFound = searchCacheBWD(thiswa, 1);
        if (lFound) {
          setCurrentFromCache(thiswa, lFound);
          return ConcludeSkipraw(thiswa, lToSkip);
        }
        lPreviousCacheStatus = lPosCache;
        readCachePageBWD(thiswa);
        lFound = searchCacheBWD(thiswa, lPreviousCacheStatus);
        if (lFound) {
          setCurrentFromCache(thiswa, lFound);
          return ConcludeSkipraw(thiswa, lToSkip);
        }
        hb_arraySetL(thiswa->aInfo, AINFO_BOF, true);
        thiswa->area.fBof = true;
        return ConcludeSkipraw(thiswa, lToSkip);
      }
    }
  } else { // !ISAM
    if (hb_arrayLen(thiswa->aCache) == 0) {
      sqlGetCleanBuffer(thiswa);
      if (lToSkip != 0) {
        thiswa->area.fBof = hb_arrayGetL(thiswa->aInfo, AINFO_BOF);
        thiswa->area.fEof = hb_arrayGetL(thiswa->aInfo, AINFO_EOF);
      }
      return ConcludeSkipraw(thiswa, lToSkip);
    }

    hb_objSendMessage(thiswa->oWorkArea, s_pSym_STABILIZE, 0);

    if (lToSkip == 0) {
      hb_arraySetL(thiswa->aInfo, AINFO_BOF, bBof);
      hb_arraySetL(thiswa->aInfo, AINFO_EOF, bEof);
      return ConcludeSkipraw(thiswa, lToSkip);
    } else if (hb_arrayGetNL(thiswa->aInfo, AINFO_NPOSCACHE) + lToSkip > 0 &&
               hb_arrayGetNL(thiswa->aInfo, AINFO_NPOSCACHE) + lToSkip <=
                   static_cast<HB_LONG>(hb_arrayLen(thiswa->aCache))) {
      sqlGetBufferFromCache2(thiswa, hb_arrayGetNL(thiswa->aInfo, AINFO_NPOSCACHE) + lToSkip);
      hb_arraySetL(thiswa->aInfo, AINFO_BOF, false);
      hb_arraySetL(thiswa->aInfo, AINFO_EOF, false);
      hb_arraySetL(thiswa->aInfo, AINFO_FOUND, false);
      pToSkip = hb_itemNew(nullptr);
      hb_objSendMessage(thiswa->oWorkArea, s_pSym_NORMALIZE, 1, pToSkip);
      hb_itemRelease(pToSkip);
      thiswa->area.fBof = false;
      thiswa->area.fEof = false;
      return ConcludeSkipraw(thiswa, lToSkip);
    } else if (hb_arrayGetNL(thiswa->aInfo, AINFO_NPOSCACHE) + lToSkip < 1) {
      hb_arraySetNL(thiswa->aInfo, AINFO_NPOSCACHE, 1);
      sqlGetBufferFromCache2(thiswa, 1);
      hb_arraySetL(thiswa->aInfo, AINFO_BOF, true);
      hb_arraySetL(thiswa->aInfo, AINFO_EOF, false);
      hb_arraySetL(thiswa->aInfo, AINFO_FOUND, false);
      thiswa->area.fBof = true;
      thiswa->area.fEof = false;
      return ConcludeSkipraw(thiswa, lToSkip);
    } else if (hb_arrayGetNL(thiswa->aInfo, AINFO_NPOSCACHE) + lToSkip >
               static_cast<HB_LONG>(hb_arrayLen(thiswa->aCache))) {
      hb_arraySetL(thiswa->aInfo, AINFO_BOF, false);
      sqlGetCleanBuffer(thiswa);
    }
  }
  if (lToSkip == 0) {
    thiswa->area.fBof = bBof;
    thiswa->area.fEof = bEof;
    return Harbour::SUCCESS;
  }
  if (lToSkip != 0) {
    thiswa->area.fBof = hb_arrayGetL(thiswa->aInfo, AINFO_BOF);
    thiswa->area.fEof = hb_arrayGetL(thiswa->aInfo, AINFO_EOF);
  }
  return ConcludeSkipraw(thiswa, lToSkip);
}

/*------------------------------------------------------------------------*/

#define sqlAddField nullptr

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlAppend(SQLAREAP thiswa) // RDDFUNC
{
  // sr_TraceLog(nullptr, "sqlAppend\n");

  /* Reset parent rel struct */
  thiswa->lpdbPendingRel = nullptr;
  thiswa->firstinteract = false;
  thiswa->wasdel = false;

  hb_arraySize(thiswa->aLocked, 0);

  if (hb_arrayGetL(thiswa->aInfo, AINFO_HOT)) {
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_WRITEBUFFER, 0);
  }

  thiswa->area.fEof = hb_arrayGetL(thiswa->aInfo, AINFO_EOF);
  thiswa->area.fBof = hb_arrayGetL(thiswa->aInfo, AINFO_BOF);

  auto pItem = hb_itemPutL(nullptr, true);
  hb_arraySet(thiswa->aInfo, AINFO_HOT, pItem);
  hb_arraySet(thiswa->aInfo, AINFO_ISINSERT, pItem);
  hb_itemPutNI(pItem, 0);
  hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLGOTO, 1, pItem);
  hb_itemRelease(pItem);

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlCreateFields(SQLAREAP thiswa, PHB_ITEM pStruct) // RDDFUNC
{
  thiswa->aCreate = pStruct;
  return SUPER_CREATEFIELDS(&thiswa->area, pStruct);
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlDeleteRec(SQLAREAP thiswa) // RDDFUNC
{
  if (thiswa->firstinteract) {
    SELF_GOTOP(&thiswa->area);
    thiswa->firstinteract = false;
  }

  if (thiswa->lpdbPendingRel) {
    SELF_FORCEREL(&thiswa->area);
  }

  if (hb_arrayGetL(thiswa->aInfo, AINFO_HOT)) {
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_WRITEBUFFER, 0);
  }
  hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLDELETEREC, 0);
  thiswa->wasdel = true;

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlDeleted(SQLAREAP thiswa, HB_BOOL *isDeleted) // RDDFUNC
{
  if (thiswa->lpdbPendingRel) {
    SELF_FORCEREL(&thiswa->area);
  } else if (thiswa->firstinteract) {
    SELF_GOTOP(&thiswa->area);
    thiswa->firstinteract = false;
  }

  *isDeleted = hb_arrayGetL(thiswa->aInfo, AINFO_DELETED);

  // sr_TraceLog(nullptr, "sqlDeleted, returning %i\n", *isDeleted);

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlFieldCount(SQLAREAP thiswa, HB_USHORT *fieldCount) // RDDFUNC
{
  *fieldCount = thiswa->area.uiFieldCount;
  // sr_TraceLog(nullptr, "sqlFieldCount, returning %i\n", thiswa->area.uiFieldCount);
  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

#define sqlFieldDisplay nullptr
#define sqlFieldInfo nullptr
#define sqlFieldName nullptr

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlFlush(SQLAREAP thiswa) // RDDFUNC
{
  // sr_TraceLog(nullptr, "sqlFlush\n");
  hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLFLUSH, 0);
  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

#define sqlGetRec nullptr /* leave it unUsed */

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlGetValue(SQLAREAP thiswa, HB_USHORT fieldNum, PHB_ITEM value) // RDDFUNC
{
  PHB_ITEM itemTemp, itemTemp3;
  PHB_ITEM pFieldNum;
  HB_SIZE nPos;
  LPFIELD pField;

  if (thiswa->lpdbPendingRel) {
    SELF_FORCEREL(&thiswa->area);
  } else if (thiswa->firstinteract) {
    SELF_GOTOP(&thiswa->area);
    thiswa->firstinteract = false;
  }
  pField = thiswa->area.lpFields + fieldNum - 1;
  itemTemp = hb_itemArrayGet(thiswa->aBuffer, thiswa->uiBufferIndex[fieldNum - 1]);

  if (HB_IS_NIL(itemTemp)) {
    hb_itemRelease(itemTemp);
    pFieldNum = hb_itemNew(nullptr);
    hb_itemPutNI(pFieldNum, thiswa->uiBufferIndex[fieldNum - 1]);
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLGETVALUE, 1, pFieldNum);
    hb_itemRelease(pFieldNum);
    itemTemp = hb_itemArrayGet(thiswa->aBuffer, thiswa->uiBufferIndex[fieldNum - 1]);
  }

  if (!thiswa->uiFieldList[fieldNum - 1]) {
    hb_arraySetNL(thiswa->aSelectList, thiswa->uiBufferIndex[fieldNum - 1], 1);
    thiswa->uiFieldList[fieldNum - 1] = 1;
    thiswa->iFieldListStatus = FIELD_LIST_NEW_VALUE_READ;
  }

  if (HB_IS_ARRAY(itemTemp)) {
    hb_arrayCloneTo(value, itemTemp);
  } else if (HB_IS_HASH(itemTemp) && sr_isMultilang()) {
    pField = thiswa->area.lpFields + fieldNum - 1;

    if (pField->uiType == Harbour::DB::Field::MEMO) {
      auto pLangItem = hb_itemNew(nullptr);
      if (hb_hashScan(itemTemp, sr_getBaseLang(pLangItem), &nPos) ||
          hb_hashScan(itemTemp, sr_getSecondLang(pLangItem), &nPos) ||
          hb_hashScan(itemTemp, sr_getRootLang(pLangItem), &nPos)) {
        hb_itemCopy(value, hb_hashGetValueAt(itemTemp, nPos));
      } else {
        hb_itemPutC(value, nullptr);
      }
      hb_itemRelease(pLangItem);
    } else {
      auto pLangItem = hb_itemNew(nullptr);
      HB_SIZE nLen = pField->uiLen, nSrcLen;
      auto empty = static_cast<char *>(hb_xgrab(nLen + 1));

      if (hb_hashScan(itemTemp, sr_getBaseLang(pLangItem), &nPos) ||
          hb_hashScan(itemTemp, sr_getSecondLang(pLangItem), &nPos) ||
          hb_hashScan(itemTemp, sr_getRootLang(pLangItem), &nPos)) {
        itemTemp3 = hb_hashGetValueAt(itemTemp, nPos);
        nSrcLen = hb_itemGetCLen(itemTemp3);
        hb_xmemcpy(empty, hb_itemGetCPtr(itemTemp3), HB_MIN(nLen, nSrcLen));
        if (nLen > nSrcLen) {
          memset(empty + nSrcLen, ' ', nLen - nSrcLen);
        }
#ifndef HB_CDP_SUPPORT_OFF
        if (pField->uiType == Harbour::DB::Field::STRING) {
          PHB_CODEPAGE cdpDest = thiswa->cdPageCnv ? thiswa->cdPageCnv : hb_vmCDP();
          if (thiswa->area.cdPage && thiswa->area.cdPage != cdpDest) {
            char *pszVal = hb_cdpnDup(empty, &nLen, thiswa->area.cdPage, cdpDest);
            hb_xfree(empty);
            empty = pszVal;
          }
        }
#endif
      } else {
        memset(empty, ' ', nLen);
      }
      empty[nLen] = '\0';
      hb_itemPutCLPtr(value, empty, nLen);
      hb_itemRelease(pLangItem);
    }
  } else {
    /*
    if( HB_IS_NIL(itemTemp) ) {
       sr_TraceLog(nullptr, "Empty buffer found at position %i, fieldpos %i\n",
    static_cast<int>(thiswa->uiBufferIndex[fieldNum - 1]), static_cast<int>(fieldNum));
    }
    */
#ifndef HB_CDP_SUPPORT_OFF
    LPFIELD pField_ = thiswa->area.lpFields + fieldNum - 1;
    if (pField_->uiType == Harbour::DB::Field::STRING) {
      PHB_CODEPAGE cdpDest = thiswa->cdPageCnv ? thiswa->cdPageCnv : hb_vmCDP();
      if (thiswa->area.cdPage && thiswa->area.cdPage != cdpDest) {
        HB_SIZE nLen = hb_itemGetCLen(itemTemp);
        char *pszVal = hb_cdpnDup(hb_itemGetCPtr(itemTemp), &nLen, thiswa->area.cdPage, cdpDest);
        hb_itemPutCLPtr(itemTemp, pszVal, nLen);
      }
    }
#endif
    hb_itemMove(value, itemTemp);
  }
  hb_itemRelease(itemTemp);
  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

#define sqlGetVarLen nullptr

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlGoCold(SQLAREAP thiswa) // RDDFUNC
{
  if (hb_arrayGetL(thiswa->aInfo, AINFO_HOT) && (!hb_arrayGetL(thiswa->aInfo, AINFO_DELETED))) {
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_WRITEBUFFER, 0); // GoCold
  }
  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

#define sqlGoHot nullptr
#define sqlPutRec nullptr

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlPutValue(SQLAREAP thiswa, HB_USHORT fieldNum, PHB_ITEM value) // RDDFUNC
{
  PHB_ITEM pDest;
  LPFIELD pField;
  char *cfield;
  double dNum;
  HB_USHORT len, dec, fieldindex;
  PHB_ITEM pFieldNum;
  // bool bOk = true;
  // PHB_DYNS s_pSym_SR_FROMXML = nullptr;
  // sr_TraceLog(nullptr, "sqlPutValue, writing column %i\n", fieldNum);

  if (thiswa->firstinteract) {
    SELF_GOTOP(&thiswa->area);
    thiswa->firstinteract = false;
  }

  if (thiswa->lpdbPendingRel) {
    SELF_FORCEREL(&thiswa->area);
  }

  fieldindex = static_cast<HB_USHORT>(thiswa->uiBufferIndex[fieldNum - 1]);
  pDest = hb_itemArrayGet(thiswa->aBuffer, fieldindex);
  //                if( s_pSym_SR_FROMXML == nullptr ) {
  //                   s_pSym_SR_FROMXML = hb_dynsymFindName("ESCREVE");
  //                   if( s_pSym_SR_FROMXML  == nullptr ) {
  //                      printf("Could not find Symbol SR_DESERIALIZE\n");
  //                   }
  //                }
  //
  //                hb_vmPushDynSym(s_pSym_SR_FROMXML);
  //                hb_vmPushNil();
  //                hb_vmPush(thiswa->aBuffer);
  //                hb_vmDo(1);
  //
  if (HB_IS_NIL(pDest)) {
    hb_itemRelease(pDest);
    pFieldNum = hb_itemNew(nullptr);
    hb_itemPutNI(pFieldNum, thiswa->uiBufferIndex[fieldNum - 1]);
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLGETVALUE, 1, pFieldNum);
    hb_itemRelease(pFieldNum);
    pDest = hb_itemArrayGet(thiswa->aBuffer, fieldindex);
  }

  if (!thiswa->uiFieldList[fieldNum - 1]) {
    hb_arraySetNL(thiswa->aSelectList, thiswa->uiBufferIndex[fieldNum - 1], 1);
    thiswa->uiFieldList[fieldNum - 1] = 1;
  }

  pField = thiswa->area.lpFields + fieldNum - 1;

  /* test compatible datatypes */
  // if (HB_IS_TIMESTAMP(value)) //|| HB_IS_DATE(pDest))
  //{
  // bOk = false;
  // hb_arraySet(thiswa->aBuffer, fieldindex, value);
  //}
  if ((HB_IS_NUMBER(pDest) && HB_IS_NUMBER(value)) || (HB_IS_STRING(pDest) && HB_IS_STRING(value)) ||
      (HB_IS_LOGICAL(pDest) && HB_IS_LOGICAL(value)) || (HB_IS_DATE(pDest) && HB_IS_DATE(value)) ||
      (HB_IS_TIMESTAMP(pDest) && HB_IS_DATETIME(value)) || (HB_IS_DATETIME(pDest) && HB_IS_DATETIME(value))) {

    if (pField->uiType == Harbour::DB::Field::STRING) {
      HB_SIZE nSize = hb_itemGetCLen(value), nLen = pField->uiLen;

      cfield = static_cast<char *>(hb_xgrab(nLen + 1));
#ifndef HB_CDP_SUPPORT_OFF
      hb_cdpnDup2(hb_itemGetCPtr(value), nSize, cfield, &nLen, thiswa->cdPageCnv ? thiswa->cdPageCnv : hb_vmCDP(),
                  thiswa->area.cdPage);
      nSize = nLen;
      nLen = pField->uiLen;
#else
      memcpy(cfield, hb_itemGetCPtr(value), HB_MIN(nLen, nSize));
#endif
      if (nLen > nSize) {
        memset(cfield + nSize, ' ', nLen - nSize);
      }
      cfield[nLen] = '\0';
      hb_itemPutCLPtr(value, cfield, nLen);
    } else if (pField->uiType == Harbour::DB::Field::LONG) {
      len = pField->uiLen;
      dec = pField->uiDec;
      if (dec > 0) {
        len -= (dec + 1);
      }
      dNum = hb_itemGetND(value);
      hb_itemPutNLen(value, dNum, len, dec);
    }

    hb_arraySet(thiswa->aBuffer, fieldindex, value);
  } else if (HB_IS_STRING(value) && HB_IS_HASH(pDest) && sr_isMultilang()) {
    auto pLangItem = hb_itemNew(nullptr);
    hb_hashAdd(pDest, sr_getBaseLang(pLangItem), value);
    hb_itemRelease(pLangItem);
  } else if (pField->uiType == Harbour::DB::Field::MEMO) { // Memo fields can hold ANY datatype
    hb_arraySet(thiswa->aBuffer, fieldindex, value);
  } else {
#ifdef SQLRDD_NWG_SPECIFIC
    hb_itemPutL(pDest, true);
    hb_arraySetForward(thiswa->aInfo, AINFO_HOT, pDest);
    hb_itemRelease(pDest);
    return Harbour::SUCCESS;
#else
    char type_err[128];
    sprintf(type_err, "data type origin: %i - data type target %i", hb_itemType(value), hb_itemType(pDest));
    hb_itemRelease(pDest);
    commonError(&thiswa->area, EG_DATATYPE, ESQLRDD_DATATYPE, type_err);
    return Harbour::FAILURE;
#endif
  }

  hb_itemPutL(pDest, true);
  hb_arraySetForward(thiswa->aInfo, AINFO_HOT, pDest);
  hb_itemRelease(pDest);

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlRecall(SQLAREAP thiswa) // RDDFUNC
{
  // sr_TraceLog(nullptr, "sqlRecall\n");

  if (thiswa->lpdbPendingRel) {
    SELF_FORCEREL(&thiswa->area);
  } else if (thiswa->firstinteract) {
    SELF_GOTOP(&thiswa->area);
    thiswa->firstinteract = false;
  }

  hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLRECALL, 0);

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlRecCount(SQLAREAP thiswa, HB_ULONG *recCount) // RDDFUNC
{
  if (thiswa->lpdbPendingRel) {
    SELF_FORCEREL(&thiswa->area);
  }

  if (hb_arrayGetL(thiswa->aInfo, AINFO_ISINSERT) && hb_arrayGetL(thiswa->aInfo, AINFO_HOT)) {
    *recCount = static_cast<HB_ULONG>(hb_arrayGetNL(thiswa->aInfo, AINFO_RCOUNT) + 1);
  } else {
    *recCount = static_cast<HB_ULONG>(hb_arrayGetNL(thiswa->aInfo, AINFO_RCOUNT));
  }

  // sr_TraceLog(nullptr, "sqlRecCount, returning %i\n", *recCount);

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

#define sqlRecInfo nullptr

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlRecNo(SQLAREAP thiswa, HB_ULONG *recno) // RDDFUNC
{
#ifdef SQLRDD_NWG_SPECIFIC
  if (hb_arrayGetNL(thiswa->aInfo, hb_arrayGetL(thiswa->aInfo, AINFO_ISINSERT))) {
    commonError(&thiswa->area, EG_ARG, ESQLRDD_NOT_COMMITED_YET, nullptr);
    return Harbour::FAILURE;
  }
#endif
  if (thiswa->lpdbPendingRel) {
    SELF_FORCEREL(&thiswa->area);
  } else if (thiswa->firstinteract) {
    SELF_GOTOP(&thiswa->area);
    thiswa->firstinteract = false;
  }

  *recno = static_cast<HB_ULONG>(hb_arrayGetNL(thiswa->aInfo, AINFO_RECNO));

  // sr_TraceLog(nullptr, "sqlRecNo %i\n", hb_arrayGetNI(thiswa->aInfo, AINFO_RECNO));

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlRecId(SQLAREAP thiswa, PHB_ITEM recno) // RDDFUNC
{
  if (thiswa->lpdbPendingRel) {
    SELF_FORCEREL(&thiswa->area);
  } else if (thiswa->firstinteract) {
    SELF_GOTOP(&thiswa->area);
    thiswa->firstinteract = false;
  }

  if (thiswa->initialized) {
    hb_arrayGet(thiswa->aInfo, AINFO_RECNO, recno);
  } else {
    hb_itemPutNL(recno, 0);
  }

  // sr_TraceLog(nullptr, "sqlRecID %i\n", hb_arrayGetNI(thiswa->aInfo, AINFO_RECNO));

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlSetFieldExtent(SQLAREAP thiswa, HB_USHORT uiFieldExtent)// RDDFUNC
{
  HB_TRACE(HB_TR_DEBUG, ("sqlSetFieldExtent(%p, %hu)", thiswa, uiFieldExtent));

  if (SUPER_SETFIELDEXTENT(&thiswa->area, uiFieldExtent) == Harbour::FAILURE) {
    return Harbour::FAILURE;
  }

  if (!sr_lHideRecno()) {
    uiFieldExtent++;
  }

  if (!sr_lHideHistoric()) {
    uiFieldExtent++;
  }

  // thiswa->uiBufferIndex = static_cast<int*>(hb_xgrab(uiFieldExtent * sizeof(int)));
  // thiswa->uiFieldList = static_cast<int*>(hb_xgrab(uiFieldExtent * sizeof(int)));
  // memset(thiswa->uiBufferIndex, 0, uiFieldExtent * sizeof(int));
  // memset(thiswa->uiFieldList,  0, uiFieldExtent * sizeof(int));
  thiswa->uiBufferIndex = static_cast<int *>(hb_xgrabz(uiFieldExtent * sizeof(int)));
  thiswa->uiFieldList = static_cast<int *>(hb_xgrabz(uiFieldExtent * sizeof(int)));
  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

#define sqlAlias nullptr

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlClose(SQLAREAP thiswa) // RDDFUNC
{
  HB_ERRCODE uiError;

  // sr_TraceLog(nullptr, "sqlClose\n");

  /* Reset parent rel struct */
  thiswa->lpdbPendingRel = nullptr;

  if (thiswa->oWorkArea && HB_IS_OBJECT(thiswa->oWorkArea)) {
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLCLOSE, 0);
  } else {
    return SUPER_CLOSE(&thiswa->area);
  }

  uiError = SUPER_CLOSE(&thiswa->area);

  /*
     else {
        commonError(&thiswa->area, EG_DATATYPE, ESQLRDD_DATATYPE, nullptr);
        return Harbour::FAILURE;
     }
  */
  // Release the used objects

  hb_itemRelease(thiswa->aStruct);
  hb_itemRelease(thiswa->aInfo);
  hb_itemRelease(thiswa->aBuffer);
  hb_itemRelease(thiswa->aOrders);
  hb_itemRelease(thiswa->aSelectList);
  hb_itemRelease(thiswa->aLocked);
  hb_itemRelease(thiswa->aOldBuffer);
  hb_itemRelease(thiswa->aCache);
  hb_itemRelease(thiswa->aEmptyBuff);

  if (thiswa->szDataFileName) {
    hb_xfree(thiswa->szDataFileName);
    thiswa->szDataFileName = nullptr;
  }

  if (thiswa->uiBufferIndex) {
    hb_xfree(thiswa->uiBufferIndex);
    thiswa->uiBufferIndex = nullptr;
  }

  if (thiswa->uiFieldList) {
    hb_xfree(thiswa->uiFieldList);
    thiswa->uiFieldList = nullptr;
  }

  if (thiswa->oWorkArea && HB_IS_OBJECT(thiswa->oWorkArea)) {
    hb_itemRelease(thiswa->oWorkArea);
    thiswa->oWorkArea = nullptr;
  }
  return uiError;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlCreate(SQLAREAP thiswa, LPDBOPENINFO pCreateInfo) // RDDFUNC
{
  auto pTable = hb_itemNew(nullptr);
  auto pAlias = hb_itemNew(nullptr);
  auto pArea = hb_itemNew(nullptr);
  HB_ERRCODE errCode;

  // sr_TraceLog(nullptr, "sqlCreate(%p, %p)", thiswa, pCreateInfo);

  thiswa->creating = true;

  thiswa->szDataFileName = static_cast<char *>(hb_xgrab(strlen(const_cast<char *>(pCreateInfo->abName)) + 1));
  strcpy(thiswa->szDataFileName, const_cast<char *>(pCreateInfo->abName));

  pTable = hb_itemPutC(pTable, thiswa->szDataFileName);
  if (pCreateInfo->atomAlias) {
    hb_itemPutC(pAlias, const_cast<char *>(pCreateInfo->atomAlias));
  }
  hb_itemPutNL(pArea, pCreateInfo->uiArea);

  thiswa->area.fTop = 0;
  thiswa->area.fBottom = 1;
  thiswa->area.fBof = 0;
  thiswa->area.fEof = 1;

  //   thiswa->valResult = nullptr;

  /* Create and run class object */
  hb_vmPushDynSym(s_pSym_WORKAREA);
  hb_vmPushNil();
  hb_vmDo(0);

  thiswa->oWorkArea = hb_itemNew(nullptr);
  hb_itemCopy(thiswa->oWorkArea, hb_stackReturnItem());
  if (s_pSym_SQLCREATE) {
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLCREATE, 4, thiswa->aCreate, pTable, pAlias, pArea);
  }

  hb_objSendMsg(thiswa->oWorkArea, "AINFO", 0);
  thiswa->aInfo = hb_itemNew(hb_stackReturnItem());

  hb_objSendMsg(thiswa->oWorkArea, "ACACHE", 0);
  thiswa->aCache = hb_itemNew(hb_stackReturnItem());

  hb_objSendMsg(thiswa->oWorkArea, "AOLDBUFFER", 0);
  thiswa->aOldBuffer = hb_itemNew(hb_stackReturnItem());

  hb_objSendMsg(thiswa->oWorkArea, "ALOCALBUFFER", 0);
  thiswa->aBuffer = hb_itemNew(hb_stackReturnItem());

  hb_objSendMsg(thiswa->oWorkArea, "AEMPTYBUFFER", 0);
  thiswa->aEmptyBuff = hb_itemNew(hb_stackReturnItem());

  hb_objSendMsg(thiswa->oWorkArea, "AINDEX", 0);
  thiswa->aOrders = hb_itemNew(hb_stackReturnItem());

  hb_objSendMsg(thiswa->oWorkArea, "ASELECTLIST", 0);
  thiswa->aSelectList = hb_itemNew(hb_stackReturnItem());

  hb_objSendMsg(thiswa->oWorkArea, "AINIFIELDS", 0);
  thiswa->aStruct = hb_itemNew(hb_stackReturnItem());

  hb_objSendMsg(thiswa->oWorkArea, "ALOCKED", 0);
  thiswa->aLocked = hb_itemNew(hb_stackReturnItem());

  hb_itemRelease(pTable);
  hb_itemRelease(pAlias);
  hb_itemRelease(pArea);

  hb_objSendMsg(thiswa->oWorkArea, "LOPENED", 0);

  if (!hb_itemGetL(hb_stackReturnItem())) {
    return Harbour::FAILURE;
  }

  if (!SetFields(thiswa)) {
    return Harbour::FAILURE;
  }

  /* If successful call SUPER_CREATE to finish system jobs */
  errCode = SUPER_CREATE(&thiswa->area, pCreateInfo);

  hb_objSendMsg(thiswa->oWorkArea, "LISAM", 0);
  thiswa->isam = hb_itemGetL(hb_stackReturnItem());

  hb_objSendMsg(thiswa->oWorkArea, "HNRECNO", 0);
  thiswa->ulhRecno = static_cast<HB_ULONG>(hb_itemGetNL(hb_stackReturnItem()));

  hb_objSendMsg(thiswa->oWorkArea, "HNDELETED", 0);
  thiswa->ulhDeleted = static_cast<HB_ULONG>(hb_itemGetNL(hb_stackReturnItem()));

  if (errCode != Harbour::SUCCESS) {
    SELF_CLOSE(&thiswa->area);
    return errCode;
  } else {
    SELF_GOTOP(&thiswa->area);
    thiswa->firstinteract = false;
  }

  thiswa->wasdel = false;

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlInfo(SQLAREAP thiswa, HB_USHORT uiIndex, PHB_ITEM pItem) // RDDFUNC
{
  HB_BOOL flag = true;

  // sr_TraceLog(nullptr, "sqlInfo(%p, %hu, %p)", thiswa, uiIndex, pItem);

  switch (uiIndex) {
  case DBI_ISDBF:
  case DBI_CANPUTREC: {
    hb_itemPutL(pItem, false);
    break;
  }
  case DBI_GETHEADERSIZE: {
    hb_itemPutNL(pItem, 0);
    break;
  }
  case DBI_LASTUPDATE: {
    hb_itemPutD(pItem, 2003, 12, 31);
    break;
  }
  case DBI_GETDELIMITER:
  case DBI_SETDELIMITER: {
    break;
  }
  case DBI_GETRECSIZE: {
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLRECSIZE, 0);
    hb_itemPutNI(pItem, hb_itemGetNI(hb_stackReturnItem()));
    break;
  }
  case DBI_GETLOCKARRAY: {
    hb_itemCopy(pItem, thiswa->aLocked);
    break;
  }
  case DBI_TABLEEXT: {
    hb_itemPutC(pItem, "");
    break;
  }
  case DBI_MEMOEXT: {
    hb_itemPutC(pItem, "");
    break;
  }
  case DBI_MEMOBLOCKSIZE: {
    hb_itemPutNI(pItem, 0);
    break;
  }
  case DBI_FULLPATH: {
    hb_itemPutC(pItem, thiswa->szDataFileName);
    break;
  }
  case DBI_FILEHANDLE: {
    hb_itemPutNL(pItem, 0);
    break;
  }
  case DBI_BOF: {
    SELF_BOF(&thiswa->area, &flag);
    hb_itemPutL(pItem, flag);
    break;
  }
  case DBI_EOF: {
    SELF_EOF(&thiswa->area, &flag);
    hb_itemPutL(pItem, flag);
    break;
  }
  case DBI_ISFLOCK: {
    if (HB_IS_OBJECT(thiswa->oWorkArea)) {
      hb_objSendMsg(thiswa->oWorkArea, "LTABLELOCKED", 0);
    } else {
      commonError(&thiswa->area, EG_DATATYPE, ESQLRDD_DATATYPE, nullptr);
      return Harbour::FAILURE;
    }
    hb_itemMove(pItem, hb_stackReturnItem());
    break;
  }
  case DBI_MEMOHANDLE: {
    hb_itemPutNL(pItem, 0);
    break;
  }
  case DBI_RDD_BUILD: {
    hb_itemPutNL(pItem, HB_SQLRDD_BUILD);
    break;
  }
  case DBI_RDD_VERSION: {
    if (HB_IS_OBJECT(thiswa->oWorkArea)) {
      hb_objSendMsg(thiswa->oWorkArea, "OSQL", 0);
      hb_objSendMsg(hb_stackReturnItem(), "CMGMNTVERS", 0);
    } else {
      commonError(&thiswa->area, EG_DATATYPE, ESQLRDD_DATATYPE, nullptr);
      return Harbour::FAILURE;
    }
    hb_itemMove(pItem, hb_stackReturnItem());
    break;
  }
  case DBI_DB_VERSION: {
    if (HB_IS_OBJECT(thiswa->oWorkArea)) {
      hb_objSendMsg(thiswa->oWorkArea, "OSQL", 0);
      hb_objSendMsg(hb_stackReturnItem(), "NSYSTEMID", 0);
    } else {
      commonError(&thiswa->area, EG_DATATYPE, ESQLRDD_DATATYPE, nullptr);
      return Harbour::FAILURE;
    }
    hb_itemMove(pItem, hb_stackReturnItem());
    break;
  }
  case DBI_INTERNAL_OBJECT: {
    hb_itemCopy(pItem, thiswa->oWorkArea);
    break;
  }
  case DBI_ISREADONLY: {
    hb_itemPutL(pItem, thiswa->readonly);
    break;
  }
  case DBI_SHARED: {
    hb_itemPutL(pItem, thiswa->shared);
    break;
  }
  case DBI_CPCONVERTTO: {
    if (HB_IS_STRING(pItem)) {
      PHB_CODEPAGE cdpage = hb_cdpFind(hb_itemGetCPtr(pItem));
      if (cdpage) {
        thiswa->cdPageCnv = cdpage;
      }
    }
    if (thiswa->cdPageCnv) {
      hb_itemPutC(pItem, const_cast<char *>(thiswa->cdPageCnv->id));
    } else {
      hb_itemPutC(pItem, "");
    }
    break;
  }
  }
  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static void startSQLRDDSymbols()
{
  if (s_pSym_WORKAREA == nullptr) {
    s_pSym_WORKAREA = hb_dynsymFindName(WORKAREA_CLASS);
    s_pSym_SQLGOBOTTOM = hb_dynsymFindName("SQLGOBOTTOM");
    s_pSym_SQLGOTO = hb_dynsymFindName("SQLGOTO");
    s_pSym_SQLGOTOP = hb_dynsymFindName("SQLGOTOP");
    s_pSym_SQLSEEK = hb_dynsymFindName("SQLSEEK");
    s_pSym_SETBOF = hb_dynsymFindName("SETBOF");
    s_pSym_SQLDELETEREC = hb_dynsymFindName("SQLDELETEREC");
    s_pSym_SQLFLUSH = hb_dynsymFindName("SQLFLUSH");
    s_pSym_SQLRECALL = hb_dynsymFindName("SQLRECALL");
    s_pSym_SQLCLOSE = hb_dynsymFindName("SQLCLOSE");
    s_pSym_SQLCREATE = hb_dynsymFindName("SQLCREATE");
    s_pSym_SQLOPEN = hb_dynsymFindName("SQLOPENAREA");
    s_pSym_SQLOPENALLINDEXES = hb_dynsymFindName("SQLOPENALLINDEXES");
    s_pSym_SQLPACK = hb_dynsymFindName("SQLPACK");
    s_pSym_SQLZAP = hb_dynsymFindName("SQLZAP");
    s_pSym_SQLORDERLISTADD = hb_dynsymFindName("SQLORDERLISTADD");
    s_pSym_SQLORDERLISTCLEAR = hb_dynsymFindName("SQLORDERLISTCLEAR");
    s_pSym_SQLORDERLISTFOCUS = hb_dynsymFindName("SQLORDERLISTFOCUS");
    s_pSym_SQLORDERCREATE = hb_dynsymFindName("SQLORDERCREATE");
    s_pSym_SQLORDERDESTROY = hb_dynsymFindName("SQLORDERDESTROY");
    s_pSym_SQLORDERCONDITION = hb_dynsymFindName("SQLORDERCONDITION");
    s_pSym_SQLORDERLISTNUM = hb_dynsymFindName("SQLORDERLISTNUM");
    s_pSym_SQLSETSCOPE = hb_dynsymFindName("SQLSETSCOPE");
    s_pSym_SQLLOCK = hb_dynsymFindName("SQLLOCK");
    s_pSym_SQLUNLOCK = hb_dynsymFindName("SQLUNLOCK");
    s_pSym_SQLDROP = hb_dynsymFindName("SQLDROP");
    s_pSym_SQLEXISTS = hb_dynsymFindName("SQLEXISTS");
    s_pSym_WRITEBUFFER = hb_dynsymFindName("WRITEBUFFER");
    s_pSym_READPAGE = hb_dynsymFindName("READPAGE");
    s_pSym_STABILIZE = hb_dynsymFindName("STABILIZE");
    s_pSym_NORMALIZE = hb_dynsymFindName("NORMALIZE");
    s_pSym_SQLKEYCOUNT = hb_dynsymFindName("SQLKEYCOUNT");
    s_pSym_SQLRECSIZE = hb_dynsymFindName("SQLRECSIZE");
    s_pSym_SQLGETVALUE = hb_dynsymFindName("SQLGETVALUE");
    s_pSym_SQLSETFILTER = hb_dynsymFindName("SQLSETFILTER");
    s_pSym_SQLCLEARFILTER = hb_dynsymFindName("SQLCLEARFILTER");
    s_pSym_SQLFILTERTEXT = hb_dynsymFindName("SQLFILTERTEXT");

    if (s_pSym_WORKAREA == nullptr)
      printf("Could not find Symbol %s\n", WORKAREA_CLASS);
    if (s_pSym_SQLGOBOTTOM == nullptr)
      printf("Could not find Symbol %s\n", "SQLGOBOTTOM");
    if (s_pSym_SQLGOTO == nullptr)
      printf("Could not find Symbol %s\n", "SQLGOTO");
    if (s_pSym_SQLGOTOP == nullptr)
      printf("Could not find Symbol %s\n", "SQLGOTOP");
    if (s_pSym_SQLSEEK == nullptr)
      printf("Could not find Symbol %s\n", "SQLSEEK");
    if (s_pSym_SETBOF == nullptr)
      printf("Could not find Symbol %s\n", "SETBOF");
    if (s_pSym_SQLDELETEREC == nullptr)
      printf("Could not find Symbol %s\n", "SQLDELETEREC");
    if (s_pSym_SQLFLUSH == nullptr)
      printf("Could not find Symbol %s\n", "SQLFLUSH");
    if (s_pSym_SQLRECALL == nullptr)
      printf("Could not find Symbol %s\n", "SQLRECALL");
    if (s_pSym_SQLCLOSE == nullptr)
      printf("Could not find Symbol %s\n", "SQLCLOSE");
    if (s_pSym_SQLCREATE == nullptr)
      printf("Could not find Symbol %s\n", "SQLCREATE");
    if (s_pSym_SQLOPEN == nullptr)
      printf("Could not find Symbol %s\n", "SQLOPENAREA");
    if (s_pSym_SQLOPENALLINDEXES == nullptr)
      printf("Could not find Symbol %s\n", "SQLOPENALLINDEXES");
    if (s_pSym_SQLPACK == nullptr)
      printf("Could not find Symbol %s\n", "SQLPACK");
    if (s_pSym_SQLZAP == nullptr)
      printf("Could not find Symbol %s\n", "SQLZAP");
    if (s_pSym_SQLORDERLISTADD == nullptr)
      printf("Could not find Symbol %s\n", "SQLORDERLISTADD");
    if (s_pSym_SQLORDERLISTCLEAR == nullptr)
      printf("Could not find Symbol %s\n", "SQLORDERLISTCLEAR");
    if (s_pSym_SQLORDERLISTFOCUS == nullptr)
      printf("Could not find Symbol %s\n", "SQLORDERLISTFOCUS");
    if (s_pSym_SQLORDERCREATE == nullptr)
      printf("Could not find Symbol %s\n", "SQLORDERCREATE");
    if (s_pSym_SQLORDERDESTROY == nullptr)
      printf("Could not find Symbol %s\n", "SQLORDERDESTROY");
    if (s_pSym_SQLORDERCONDITION == nullptr)
      printf("Could not find Symbol %s\n", "SQLORDERCONDITION");
    if (s_pSym_SQLORDERLISTNUM == nullptr)
      printf("Could not find Symbol %s\n", "SQLORDERLISTNUM");
    if (s_pSym_SQLSETSCOPE == nullptr)
      printf("Could not find Symbol %s\n", "SQLSETSCOPE");
    if (s_pSym_SQLLOCK == nullptr)
      printf("Could not find Symbol %s\n", "SQLLOCK");
    if (s_pSym_SQLUNLOCK == nullptr)
      printf("Could not find Symbol %s\n", "SQLUNLOCK");
    if (s_pSym_SQLDROP == nullptr)
      printf("Could not find Symbol %s\n", "SQLDROP");
    if (s_pSym_SQLEXISTS == nullptr)
      printf("Could not find Symbol %s\n", "SQLEXISTS");
    if (s_pSym_SQLKEYCOUNT == nullptr)
      printf("Could not find Symbol %s\n", "SQLKEYCOUNT");
    if (s_pSym_WRITEBUFFER == nullptr)
      printf("Could not find Symbol %s\n", "WRITEBUFFER");
    if (s_pSym_READPAGE == nullptr)
      printf("Could not find Symbol %s\n", "READPAGE");
    if (s_pSym_STABILIZE == nullptr)
      printf("Could not find Symbol %s\n", "STABILIZE");
    if (s_pSym_NORMALIZE == nullptr)
      printf("Could not find Symbol %s\n", "NORMALIZE");
    if (s_pSym_SQLGETVALUE == nullptr)
      printf("Could not find Symbol %s\n", "SQLGETVALUE");
    if (s_pSym_SQLSETFILTER == nullptr)
      printf("Could not find Symbol %s\n", "SQLSETFILTER");
    if (s_pSym_SQLCLEARFILTER == nullptr)
      printf("Could not find Symbol %s\n", "SQLCLEARFILTER");
    if (s_pSym_SQLFILTERTEXT == nullptr)
      printf("Could not find Symbol %s\n", "SQLFILTERTEXT");
  }
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlNewArea(SQLAREAP thiswa) // RDDFUNC
{
  if (SUPER_NEW(&thiswa->area) == Harbour::FAILURE) {
    return Harbour::FAILURE;
  }

  thiswa->uiBufferIndex = nullptr;
  thiswa->uiFieldList = nullptr;
  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlOpen(SQLAREAP thiswa, LPDBOPENINFO pOpenInfo) // RDDFUNC
{
  auto pConnection = hb_itemNew(nullptr);
  auto pTable = hb_itemNew(nullptr);
  auto pArea = hb_itemNew(nullptr);
  auto pAlias = hb_itemNew(nullptr);
  auto pShared = hb_itemNew(nullptr);
  auto pReadOnly = hb_itemNew(nullptr);
  HB_ERRCODE errCode;

  char szAlias[HB_RDD_MAX_ALIAS_LEN + 1];

  // sr_TraceLog(nullptr, "sqlOpen\n");

  thiswa->szDataFileName = static_cast<char *>(hb_xgrab(strlen(const_cast<char *>(pOpenInfo->abName)) + 1));
  strcpy(thiswa->szDataFileName, const_cast<char *>(pOpenInfo->abName));

  /* Create default alias if necessary */
  if (!pOpenInfo->atomAlias) {
    PHB_FNAME pFileName;
    pFileName = hb_fsFNameSplit(const_cast<char *>(pOpenInfo->abName));
    hb_strncpyUpperTrim(szAlias, pFileName->szName, HB_RDD_MAX_ALIAS_LEN);
    pOpenInfo->atomAlias = szAlias;
    hb_xfree(pFileName);
  }

  errCode = SUPER_OPEN(&thiswa->area, pOpenInfo);

  if (errCode != Harbour::SUCCESS) {
    SELF_CLOSE(&thiswa->area);
    return errCode;
  }

  thiswa->shared = pOpenInfo->fShared;
  thiswa->readonly = pOpenInfo->fReadonly;
  thiswa->hOrdCurrent = 0;
  thiswa->creating = false;
  thiswa->initialized = false;
  thiswa->sqlfilter = false;

  thiswa->area.uiMaxFieldNameLength = 64;

  hb_itemPutNL(pConnection, pOpenInfo->ulConnection);
  hb_itemPutC(pTable, thiswa->szDataFileName);
  hb_itemPutNL(pArea, pOpenInfo->uiArea);
  hb_itemPutL(pShared, thiswa->shared);
  hb_itemPutL(pReadOnly, thiswa->readonly);
  hb_itemPutC(pAlias, const_cast<char *>(pOpenInfo->atomAlias));

#ifndef HB_CDP_SUPPORT_OFF
  if (pOpenInfo->cdpId) {
    thiswa->area.cdPage = hb_cdpFind(const_cast<char *>(pOpenInfo->cdpId));
    if (!thiswa->area.cdPage) {
      thiswa->area.cdPage = hb_vmCDP();
    }
  } else {
    thiswa->area.cdPage = hb_vmCDP();
  }
#endif

  hb_vmPushDynSym(s_pSym_WORKAREA);
  hb_vmPushNil();
  hb_vmDo(0);
  thiswa->oWorkArea = hb_itemNew(hb_stackReturnItem());

  hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLOPEN, 6, pTable, pArea, pShared, pReadOnly, pAlias, pConnection);

  hb_objSendMsg(thiswa->oWorkArea, "AINFO", 0);
  thiswa->aInfo = hb_itemNew(hb_stackReturnItem());

  hb_objSendMsg(thiswa->oWorkArea, "ACACHE", 0);
  thiswa->aCache = hb_itemNew(hb_stackReturnItem());

  hb_objSendMsg(thiswa->oWorkArea, "AOLDBUFFER", 0);
  thiswa->aOldBuffer = hb_itemNew(hb_stackReturnItem());

  hb_objSendMsg(thiswa->oWorkArea, "ALOCALBUFFER", 0);
  thiswa->aBuffer = hb_itemNew(hb_stackReturnItem());

  hb_objSendMsg(thiswa->oWorkArea, "AEMPTYBUFFER", 0);
  thiswa->aEmptyBuff = hb_itemNew(hb_stackReturnItem());

  hb_objSendMsg(thiswa->oWorkArea, "AINDEX", 0);
  thiswa->aOrders = hb_itemNew(hb_stackReturnItem());

  hb_objSendMsg(thiswa->oWorkArea, "ASELECTLIST", 0);
  thiswa->aSelectList = hb_itemNew(hb_stackReturnItem());

  hb_objSendMsg(thiswa->oWorkArea, "AINIFIELDS", 0);
  thiswa->aStruct = hb_itemNew(hb_stackReturnItem());

  hb_objSendMsg(thiswa->oWorkArea, "ALOCKED", 0);
  thiswa->aLocked = hb_itemNew(hb_stackReturnItem());

  hb_objSendMsg(thiswa->oWorkArea, "LGOTOPONFIRSTINTERACT", 0);
  thiswa->firstinteract = hb_itemGetL(hb_stackReturnItem());

  thiswa->initialized = true;
  thiswa->wasdel = false;

  hb_itemRelease(pTable);
  hb_itemRelease(pArea);
  hb_itemRelease(pAlias);
  hb_itemRelease(pShared);
  hb_itemRelease(pReadOnly);
  hb_itemRelease(pConnection);

  hb_objSendMsg(thiswa->oWorkArea, "LOPENED", 0);

  if (!hb_itemGetL(hb_stackReturnItem())) {
    return Harbour::FAILURE;
  }

  hb_objSendMsg(thiswa->oWorkArea, "LISAM", 0);
  thiswa->isam = hb_itemGetL(hb_stackReturnItem());

  if (!ProcessFields(thiswa)) {
    return Harbour::FAILURE;
  }

  hb_objSendMsg(thiswa->oWorkArea, "HNRECNO", 0);
  thiswa->ulhRecno = static_cast<HB_ULONG>(hb_itemGetNL(hb_stackReturnItem()));

  hb_objSendMsg(thiswa->oWorkArea, "HNDELETED", 0);
  thiswa->ulhDeleted = static_cast<HB_ULONG>(hb_itemGetNL(hb_stackReturnItem()));

  if (hb_setGetAutOpen()) {
    if (HB_IS_OBJECT(thiswa->oWorkArea)) {
      hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLOPENALLINDEXES, 0);
    } else {
      commonError(&thiswa->area, EG_DATATYPE, ESQLRDD_DATATYPE, nullptr);
      return Harbour::FAILURE;
    }

    if (hb_itemGetNL(hb_stackReturnItem())) {
      thiswa->hOrdCurrent = hb_itemGetNL(hb_stackReturnItem());
    }
  }
  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

#define sqlRelease nullptr

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlStructSize(SQLAREAP thiswa, HB_USHORT *StructSize) // RDDFUNC
{
  HB_SYMBOL_UNUSED(thiswa); /* Avoid compiler warning */
  *StructSize = sizeof(SQLAREA);
  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

#define sqlSysName nullptr
#define sqlEval nullptr

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlPack(SQLAREAP thiswa) // RDDFUNC
{
  // sr_TraceLog(nullptr, "sqlPack\n");
  hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLPACK, 0);
  SELF_GOTOP(&thiswa->area);
  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

#define sqlPackRec nullptr
#define sqlSort nullptr
#define sqlTrans nullptr
#define sqlTransRec nullptr

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlZap(SQLAREAP thiswa) // RDDFUNC
{
  // sr_TraceLog(nullptr, "sqlZap\n");

  hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLZAP, 0);

  thiswa->area.fEof = hb_arrayGetL(thiswa->aInfo, AINFO_EOF);
  thiswa->area.fBof = hb_arrayGetL(thiswa->aInfo, AINFO_BOF);
  thiswa->firstinteract = false;
  thiswa->wasdel = false;

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlChildEnd(SQLAREAP thiswa, LPDBRELINFO pRelInfo) // RDDFUNC
{
  HB_ERRCODE uiError;

  // sr_TraceLog(nullptr, "sqlChildEnd\n");

  HB_TRACE(HB_TR_DEBUG, ("sqlChildEnd(%p, %p)", thiswa, pRelInfo));

  if (thiswa->lpdbPendingRel == pRelInfo) {
    uiError = SELF_FORCEREL(&thiswa->area);
  } else {
    uiError = Harbour::SUCCESS;
  }
  SUPER_CHILDEND(&thiswa->area, pRelInfo);
  return uiError;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlChildStart(SQLAREAP thiswa, LPDBRELINFO pRelInfo) // RDDFUNC
{
  HB_TRACE(HB_TR_DEBUG, ("sqlChildStart(%p, %p)", thiswa, pRelInfo));

  // sr_TraceLog(nullptr, "sqlChildStart\n");

  if (thiswa->firstinteract) {
    SELF_GOTOP(&thiswa->area);
    thiswa->firstinteract = false;
  }

  SELF_CHILDSYNC(&thiswa->area, pRelInfo);
  return SUPER_CHILDSTART(&thiswa->area, pRelInfo);
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlChildSync(SQLAREAP thiswa, LPDBRELINFO pRelInfo) // RDDFUNC
{
  HB_TRACE(HB_TR_DEBUG, ("sqlChildSync(%p, %p)", thiswa, pRelInfo));

  // sr_TraceLog(nullptr, "sqlChildSync\n");

  thiswa->lpdbPendingRel = pRelInfo;
  SELF_SYNCCHILDREN(&thiswa->area);

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

#define sqlSyncChildren nullptr

/*------------------------------------------------------------------------*/

#define sqlClearRel nullptr

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlForceRel(SQLAREAP thiswa) // RDDFUNC
{
  LPDBRELINFO lpdbPendingRel;
  HB_ERRCODE uiError;

  HB_TRACE(HB_TR_DEBUG, ("sqlForceRel(%p)", thiswa));

  if (thiswa->lpdbPendingRel) {
    lpdbPendingRel = thiswa->lpdbPendingRel;
    thiswa->lpdbPendingRel = nullptr;
    uiError = SELF_RELEVAL(&thiswa->area, lpdbPendingRel);
    thiswa->firstinteract = false;
    thiswa->wasdel = false;
    return uiError;
  }
  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

#define sqlRelArea nullptr
#define sqlRelEval nullptr
#define sqlRelText nullptr
#define sqlSetRel nullptr

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlOrderListAdd(SQLAREAP thiswa, LPDBORDERINFO pOrderInfo) // RDDFUNC
{
  PHB_ITEM pNIL = nullptr, pIndex, pTag;

  // sr_TraceLog(nullptr, "sqlOrderListAdd\n");

  pIndex = pOrderInfo->atomBagName;
  pTag = pOrderInfo->itmOrder;
  if (!pIndex || !pTag) {
    pNIL = hb_itemNew(nullptr);
    if (!pIndex) {
      pIndex = pNIL;
    }
    if (!pTag) {
      pTag = pNIL;
    }
  }

  hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLORDERLISTADD, 2, pIndex, pTag);

  if (pNIL) {
    hb_itemRelease(pNIL);
  }

  if (hb_itemGetNL(hb_stackReturnItem())) {
    thiswa->hOrdCurrent = hb_itemGetNL(hb_stackReturnItem());
    return Harbour::SUCCESS;
  }

  return Harbour::FAILURE;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlOrderListClear(SQLAREAP thiswa) // RDDFUNC
{
  // sr_TraceLog(nullptr, "sqlOrderListClear\n");

  hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLORDERLISTCLEAR, 0);

  if (hb_itemGetL(hb_stackReturnItem())) {
    thiswa->hOrdCurrent = 0;
    return Harbour::SUCCESS;
  }

  return Harbour::FAILURE;
}

/*------------------------------------------------------------------------*/

#define sqlOrderListDelete nullptr

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlOrderListFocus(SQLAREAP thiswa, LPDBORDERINFO pOrderInfo) // RDDFUNC
{
  PHB_ITEM pTag;
  auto lorder = 0L;

  // sr_TraceLog(nullptr, "sqlOrderListFocus\n");

  // BagName.type = Harbour::Item::NIL;
  pTag = loadTagDefault(thiswa, nullptr, &lorder);

  if (pTag) {
    hb_arrayGet(pTag, ORDER_TAG, pOrderInfo->itmResult);
    hb_itemRelease(pTag);
  } else {
    hb_itemPutC(pOrderInfo->itmResult, "");
  }

  if (pOrderInfo->itmOrder) {
    auto pBagName = hb_itemNew(pOrderInfo->atomBagName);
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLORDERLISTFOCUS, 2, pOrderInfo->itmOrder, pBagName);
    hb_itemRelease(pBagName);
    thiswa->hOrdCurrent = hb_itemGetNL(hb_stackReturnItem());
    return Harbour::SUCCESS;
  } else {
    return Harbour::SUCCESS;
  }
}

/*------------------------------------------------------------------------*/

#define sqlOrderListRebuild nullptr

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlOrderCondition(SQLAREAP thiswa, LPDBORDERCONDINFO lpdbOrdCondInfo) // RDDFUNC
{
  auto pItemFor = hb_itemNew(nullptr);
  auto pItemWhile = hb_itemNew(nullptr);
  auto pItemStart = hb_itemNew(nullptr);
  auto pItemNext = hb_itemNew(nullptr);
  auto pItemRecno = hb_itemNew(nullptr);
  auto pItemRest = hb_itemNew(nullptr);
  auto pItemDesc = hb_itemNew(nullptr);

  if (lpdbOrdCondInfo) {
    if (lpdbOrdCondInfo->abFor) {
      hb_itemPutC(pItemFor, lpdbOrdCondInfo->abFor);
    }
    if (lpdbOrdCondInfo->abWhile) {
      hb_itemPutC(pItemWhile, lpdbOrdCondInfo->abWhile);
    }
    if (lpdbOrdCondInfo->itmStartRecID) {
      hb_itemCopy(pItemStart, lpdbOrdCondInfo->itmStartRecID);
    }
    hb_itemPutNL(pItemNext, lpdbOrdCondInfo->lNextCount);
    if (lpdbOrdCondInfo->itmRecID) {
      hb_itemCopy(pItemRecno, lpdbOrdCondInfo->itmRecID);
    }
    hb_itemPutL(pItemRest, lpdbOrdCondInfo->fRest);
    hb_itemPutL(pItemDesc, lpdbOrdCondInfo->fDescending);
  }

  hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLORDERCONDITION, 7, pItemFor, pItemWhile, pItemStart, pItemNext,
                    pItemRecno, pItemRest, pItemDesc);

  hb_itemRelease(pItemFor);
  hb_itemRelease(pItemWhile);
  hb_itemRelease(pItemStart);
  hb_itemRelease(pItemNext);
  hb_itemRelease(pItemRecno);
  hb_itemRelease(pItemRest);
  hb_itemRelease(pItemDesc);

  SUPER_ORDSETCOND(&thiswa->area, lpdbOrdCondInfo);
  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlOrderCreate(SQLAREAP thiswa, LPDBORDERCREATEINFO pOrderInfo) // RDDFUNC
{
  // sr_TraceLog(nullptr, "sqlOrderCreate\n");

  if (SELF_GOCOLD(&thiswa->area) == Harbour::FAILURE) {
    return Harbour::FAILURE;
  }

  auto pBagName = hb_itemPutC(nullptr, pOrderInfo->abBagName);
  auto pAtomBagName = hb_itemPutC(nullptr, pOrderInfo->atomBagName);

  if (pOrderInfo->lpdbConstraintInfo) {
    auto pConstrName = hb_itemPutC(nullptr, pOrderInfo->lpdbConstraintInfo->abConstrName);
    auto pTarget = hb_itemPutC(nullptr, pOrderInfo->lpdbConstraintInfo->abTargetName);
    auto pEnable = hb_itemPutL(nullptr, pOrderInfo->lpdbConstraintInfo->fEnabled);

    hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLORDERCREATE, 7, pBagName, pOrderInfo->abExpr, pAtomBagName,
                      pConstrName, pTarget, pOrderInfo->lpdbConstraintInfo->itmRelationKey, pEnable);

    hb_itemRelease(pConstrName);
    hb_itemRelease(pTarget);
    hb_itemRelease(pEnable);
  } else {
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLORDERCREATE, 3, pBagName, pOrderInfo->abExpr, pAtomBagName);
  }

  hb_itemRelease(pBagName);
  hb_itemRelease(pAtomBagName);

  if (hb_itemGetNL(hb_stackReturnItem())) {
    thiswa->hOrdCurrent = hb_itemGetNL(hb_stackReturnItem());
    return Harbour::SUCCESS;
  }
  return Harbour::FAILURE;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlOrderDestroy(SQLAREAP thiswa, LPDBORDERINFO pOrderInfo) // RDDFUNC
{
  PHB_ITEM pTag;
  auto lorder = 0L;

  // sr_TraceLog(nullptr, "sqlOrderDestroy\n");

  if (SELF_GOCOLD(&thiswa->area) == Harbour::FAILURE) {
    return Harbour::FAILURE;
  }

  pTag = loadTagDefault(thiswa, nullptr, &lorder);

  if (!pTag) {
    return Harbour::FAILURE;
  }

  if (pOrderInfo->itmOrder) {
    auto pBagName = hb_itemNew(pOrderInfo->atomBagName);

    hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLORDERDESTROY, 2, pOrderInfo->itmOrder, pBagName);
    hb_itemRelease(pBagName);

    if (hb_itemGetNL(hb_stackReturnItem())) {
      thiswa->hOrdCurrent = hb_itemGetNL(hb_stackReturnItem());
      return Harbour::SUCCESS;
    }
  } else {
    return Harbour::SUCCESS;
  }

  return Harbour::FAILURE;
}

/*------------------------------------------------------------------------*/
/*
static PHB_ITEM loadTag(SQLAREAP thiswa, LPDBORDERINFO pInfo, HB_LONG * lorder)
{
   PHB_ITEM pTag = nullptr;

   if( pInfo && pInfo->itmOrder ) {
      if( HB_IS_OBJECT(thiswa->oWorkArea) ) {
         hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLORDERLISTNUM, 1, pInfo->itmOrder);
      } else {
         commonError(&thiswa->area, EG_DATATYPE, ESQLRDD_DATATYPE, nullptr);
         return nullptr;
      }

      if( hb_itemGetNL(hb_stackReturnItem()) ) {
         *lorder = hb_itemGetNL(hb_stackReturnItem());
         pTag = hb_itemArrayGet(thiswa->aOrders, static_cast<HB_ULONG>(*lorder));
      }
   }
   return pTag;
}
*/
/*------------------------------------------------------------------------*/

PHB_ITEM loadTagDefault(SQLAREAP thiswa, LPDBORDERINFO pInfo, HB_LONG *lorder)
{
  auto pOrder = hb_itemNew(nullptr);
  PHB_ITEM pTag = nullptr;

  //   Order.type = Harbour::Item::NIL;

  if (pInfo) {
    if (pInfo->itmOrder) {
      if (HB_IS_OBJECT(thiswa->oWorkArea)) {
        hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLORDERLISTNUM, 1, pInfo->itmOrder);
      } else {
        commonError(&thiswa->area, EG_DATATYPE, ESQLRDD_DATATYPE, nullptr);
        return nullptr;
      }
    } else {
      if (HB_IS_OBJECT(thiswa->oWorkArea)) {
        hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLORDERLISTNUM, 1, pOrder);
      } else {
        hb_itemRelease(pOrder);
        commonError(&thiswa->area, EG_DATATYPE, ESQLRDD_DATATYPE, nullptr);
        return nullptr;
      }
    }
  } else {
    if (HB_IS_OBJECT(thiswa->oWorkArea)) {
      hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLORDERLISTNUM, 1, pOrder);
    } else {
      hb_itemRelease(pOrder);
      commonError(&thiswa->area, EG_DATATYPE, ESQLRDD_DATATYPE, nullptr);
      return nullptr;
    }
  }

  *lorder = hb_itemGetNL(hb_stackReturnItem());

  if (*lorder) {
    pTag = hb_itemArrayGet(thiswa->aOrders, static_cast<HB_ULONG>(*lorder));
  }

  hb_itemRelease(pOrder);

  return pTag;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlSetServerSideIndexScope(SQLAREAP thiswa, int nScope, PHB_ITEM scopeValue)
{
  int res;

  auto scopeval = hb_itemNew(scopeValue);
  auto scopetype = hb_itemPutNI(nullptr, nScope);
  hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLSETSCOPE, 2, scopetype, scopeval);
  res = hb_itemGetNI(hb_stackReturnItem());
  hb_itemRelease(scopetype);
  hb_itemRelease(scopeval);

  if ((!res) && sr_GoTopOnScope()) {
    thiswa->firstinteract = true;
  }

  return (res == 0) ? Harbour::SUCCESS : Harbour::FAILURE;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlOrderInfo(SQLAREAP thiswa, HB_USHORT uiIndex, LPDBORDERINFO pInfo) // RDDFUNC
{
  HB_LONG lIndexes;
  auto lorder = 0L;
  PHB_ITEM pTag, pTemp;
  PHB_MACRO pMacro;

  // sr_TraceLog(nullptr, "sqlOrderInfo, order: %i\n", uiIndex);

  HB_TRACE(HB_TR_DEBUG, ("sqlOrderInfo(%p, %hu, %p)", thiswa, uiIndex, pInfo));

  switch (uiIndex) {
  case DBOI_BAGEXT: {
    hb_itemPutC(pInfo->itmResult, "");
    return Harbour::SUCCESS;
  }
  }

  lIndexes = static_cast<HB_LONG>(hb_itemSize(thiswa->aOrders));

  if (lIndexes) { /* Exists opened orders ? */
    switch (uiIndex) {
    case DBOI_KEYCOUNTRAW: {
      HB_ULONG ulRecCount = 0;
      SELF_RECCOUNT(&thiswa->area, &ulRecCount);
      hb_itemPutNL(pInfo->itmResult, ulRecCount);
      break;
    }
    case DBOI_KEYCOUNT: {
      hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLKEYCOUNT, 0);
      hb_itemPutNL(pInfo->itmResult, hb_itemGetNL(hb_stackReturnItem()));
      break;
    }
    case DBOI_CONDITION: {
      pTag = loadTagDefault(thiswa, pInfo, &lorder);
      if (pTag) {
        pTemp = hb_itemArrayGet(pTag, FOR_CLAUSE);
        hb_itemCopy(pInfo->itmResult, pTemp);
        hb_itemRelease(pTemp);
        hb_itemRelease(pTag);
      } else {
        hb_itemPutC(pInfo->itmResult, "");
      }
      break;
    }
    case DBOI_EXPRESSION: {
      pTag = loadTagDefault(thiswa, pInfo, &lorder);

      if (pTag) {
        pTemp = hb_itemArrayGet(pTag, INDEX_KEY);
        hb_itemCopy(pInfo->itmResult, pTemp);
        hb_itemRelease(pTemp);
        hb_itemRelease(pTag);
      } else {
        hb_itemPutC(pInfo->itmResult, "");
      }
      break;
    }
    case DBOI_NUMBER: {
      hb_itemPutNILen(pInfo->itmResult, hb_arrayGetNI(thiswa->aInfo, AINFO_INDEXORD), 3);
      break;
    }
    case DBOI_BAGNAME: {
      pTag = loadTagDefault(thiswa, pInfo, &lorder);
      if (pTag) {
        pTemp = hb_itemArrayGet(pTag, ORDER_NAME);
        hb_itemCopy(pInfo->itmResult, pTemp);
        hb_itemRelease(pTemp);
        hb_itemRelease(pTag);
      } else {
        hb_itemPutC(pInfo->itmResult, "");
      }
      break;
    }
    case DBOI_NAME: {
      pTag = loadTagDefault(thiswa, pInfo, &lorder);
      if (pTag) {
        pTemp = hb_itemArrayGet(pTag, ORDER_TAG);
        hb_itemCopy(pInfo->itmResult, pTemp);
        hb_itemRelease(pTemp);
        hb_itemRelease(pTag);
      } else {
        hb_itemPutC(pInfo->itmResult, "");
      }
      break;
    }
    case DBOI_POSITION:
    case DBOI_KEYNORAW: {
      hb_itemPutND(pInfo->itmResult, 0);
      break;
    }
    case DBOI_ISCOND: {
      pTag = loadTagDefault(thiswa, pInfo, &lorder);
      if (pTag) {
        pTemp = hb_itemArrayGet(pTag, FOR_CLAUSE);
        hb_itemPutL(pInfo->itmResult, !HB_IS_NIL(pTemp));
        hb_itemRelease(pTemp);
        hb_itemRelease(pTag);
      } else {
        hb_itemPutL(pInfo->itmResult, 0);
      }
      break;
    }
    case DBOI_ISDESC: {
      pTag = loadTagDefault(thiswa, pInfo, &lorder);
      if (pTag) {
        hb_itemPutL(pInfo->itmResult, hb_arrayGetL(pTag, DESCEND_INDEX_ORDER));

        if (pInfo->itmNewVal && HB_IS_LOGICAL(pInfo->itmNewVal)) {
          hb_itemPutL(pInfo->itmResult, hb_arrayGetL(thiswa->aInfo, AINFO_REVERSE_INDEX));
          hb_itemPutL(hb_arrayGetItemPtr(pTag, DESCEND_INDEX_ORDER), hb_itemGetL(pInfo->itmNewVal));
          hb_itemPutL(hb_arrayGetItemPtr(thiswa->aInfo, AINFO_REVERSE_INDEX), hb_itemGetL(pInfo->itmNewVal));
          hb_arraySetForward(pTag, ORDER_SKIP_UP, hb_itemNew(nullptr));
          hb_arraySetForward(pTag, ORDER_SKIP_DOWN, hb_itemNew(nullptr));
          hb_itemPutNL(hb_arrayGetItemPtr(thiswa->aInfo, AINFO_BOF_AT), 0);
          hb_itemPutNL(hb_arrayGetItemPtr(thiswa->aInfo, AINFO_EOF_AT), 0);
        }
        hb_itemRelease(pTag);
      } else {
        hb_itemPutL(pInfo->itmResult, false);
      }
      break;
    }
    case DBOI_UNIQUE: {
      pTag = loadTagDefault(thiswa, pInfo, &lorder);
      if (pTag) {
        hb_itemPutL(pInfo->itmResult, hb_arrayGetL(pTag, AORDER_UNIQUE));
        hb_itemRelease(pTag);
      } else {
        hb_itemPutL(pInfo->itmResult, false);
      }
      break;
    }
    case DBOI_CUSTOM: {
      hb_itemPutL(pInfo->itmResult, false);
      break;
    }
    case DBOI_SCOPETOP: {
      pTag = loadTagDefault(thiswa, pInfo, &lorder);
      if (pTag) {
        if (pInfo->itmResult) {
          pTemp = hb_itemArrayGet(pTag, TOP_SCOPE);
          hb_itemCopy(pInfo->itmResult, pTemp);
          hb_itemRelease(pTemp);
        }
        if (pInfo->itmNewVal) {
          sqlSetServerSideIndexScope(static_cast<SQLAREAP>(thiswa), 0, pInfo->itmNewVal);
        }
        hb_itemRelease(pTag);
      } else {
        hb_itemClear(pInfo->itmResult);
      }
      break;
    }
    case DBOI_SCOPEBOTTOM: {
      pTag = loadTagDefault(thiswa, pInfo, &lorder);
      if (pTag) {
        if (pInfo->itmResult) {
          pTemp = hb_itemArrayGet(pTag, BOTTOM_SCOPE);
          hb_itemCopy(pInfo->itmResult, pTemp);
          hb_itemRelease(pTemp);
        }
        if (pInfo->itmNewVal) {
          sqlSetServerSideIndexScope(static_cast<SQLAREAP>(thiswa), 1, pInfo->itmNewVal);
        }
        hb_itemRelease(pTag);
      } else {
        hb_itemClear(pInfo->itmResult);
      }
      break;
    }
    case DBOI_SCOPETOPCLEAR: {
      pTag = loadTagDefault(thiswa, pInfo, &lorder);
      if (pTag) {
        sqlSetServerSideIndexScope(static_cast<SQLAREAP>(thiswa), 0, nullptr);
      }
      break;
    }
    case DBOI_SCOPEBOTTOMCLEAR: {
      pTag = loadTagDefault(thiswa, pInfo, &lorder);
      if (pTag) {
        sqlSetServerSideIndexScope(static_cast<SQLAREAP>(thiswa), 1, nullptr);
      }
      break;
    }
    case DBOI_SCOPESET: {
      pTag = loadTagDefault(thiswa, pInfo, &lorder);
      if (pTag) {
        if (pInfo->itmResult) {
          pTemp = hb_itemArrayGet(pTag, TOP_SCOPE);
          hb_itemCopy(pInfo->itmResult, pTemp);
          hb_itemRelease(pTemp);
        }
        if (pInfo->itmNewVal) {
          sqlSetServerSideIndexScope(static_cast<SQLAREAP>(thiswa), TOP_BOTTOM_SCOPE, pInfo->itmNewVal);
        }
        hb_itemRelease(pTag);
      } else {
        hb_itemClear(pInfo->itmResult);
      }
      break;
    }
    case DBOI_KEYADD: {
      hb_itemPutL(pInfo->itmResult, false);
      break;
    }
    case DBOI_KEYDELETE: {
      hb_itemPutL(pInfo->itmResult, false);
      break;
    }
    case DBOI_KEYVAL: {
      pTag = loadTagDefault(thiswa, pInfo, &lorder);
      if (pTag) {
        if (thiswa->firstinteract) {
          SELF_GOTOP(&thiswa->area);
          thiswa->firstinteract = false;
        }

        pTemp = hb_itemArrayGet(pTag, INDEX_KEY);
        pMacro = hb_macroCompile(hb_itemGetCPtr(pTemp));
        hb_macroRun(pMacro);
        hb_macroDelete(pMacro);
        hb_itemMove(pInfo->itmResult, hb_stackItemFromTop(-1));
        hb_itemRelease(pTemp);
        hb_itemRelease(pTag);
      } else {
        hb_itemClear(pInfo->itmResult);
      }
      break;
    }
    case DBOI_ORDERCOUNT: {
      hb_itemPutNI(pInfo->itmResult, lIndexes);
      break;
    }
    }
  } else {
    switch (uiIndex) {
    case DBOI_KEYCOUNTRAW: {
      HB_ULONG ulRecCount = 0;
      SELF_RECCOUNT(&thiswa->area, &ulRecCount);
      hb_itemPutND(pInfo->itmResult, ulRecCount);
      break;
    }
    case DBOI_KEYCOUNT: {
      hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLKEYCOUNT, 0);
      hb_itemPutND(pInfo->itmResult, hb_itemGetND(hb_stackReturnItem()));
      break;
    }
    case DBOI_POSITION:
    case DBOI_KEYNORAW: {
      hb_itemPutND(pInfo->itmResult, 0);
      SELF_RECID(&thiswa->area, pInfo->itmResult);
      break;
    }
    case DBOI_ISCOND:
    case DBOI_ISDESC:
    case DBOI_UNIQUE:
    case DBOI_CUSTOM:
    case DBOI_KEYADD:
    case DBOI_KEYDELETE: {
      hb_itemPutL(pInfo->itmResult, false);
      break;
    }
    case DBOI_KEYVAL:
    case DBOI_SCOPETOP:
    case DBOI_SCOPEBOTTOM:
    case DBOI_SCOPETOPCLEAR:
    case DBOI_SCOPEBOTTOMCLEAR: {
      hb_itemClear(pInfo->itmResult);
      break;
    }
    case DBOI_ORDERCOUNT:
    case DBOI_NUMBER: {
      hb_itemPutNI(pInfo->itmResult, 0);
      break;
    }
    default: {
      hb_itemPutC(pInfo->itmResult, "");
    }
    }
  }

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlClearFilter(SQLAREAP thiswa) // RDDFUNC
{
  if (thiswa->sqlfilter) {
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLCLEARFILTER, 0);
    thiswa->sqlfilter = false;
  }
  return SUPER_CLEARFILTER(&thiswa->area);
}

/*------------------------------------------------------------------------*/

#define sqlClearLocate nullptr

/*------------------------------------------------------------------------*/

#define sqlClearScope nullptr

/*------------------------------------------------------------------------*/

#define sqlCountScope nullptr

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlFilterText(SQLAREAP thiswa, PHB_ITEM pFilter) // RDDFUNC
{
  if (thiswa->sqlfilter) {
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLFILTERTEXT, 0);
    hb_itemCopy(pFilter, hb_stackReturnItem());
    return Harbour::SUCCESS;
  } else {
    return SUPER_FILTERTEXT(&thiswa->area, pFilter);
  }
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlScopeInfo(SQLAREAP thiswa, HB_USHORT nScope, PHB_ITEM pItem) // RDDFUNC
{
  HB_LONG lIndexes, lorder;
  PHB_ITEM pTag, pTemp;

  // sr_TraceLog(nullptr, "sqlScopeInfo, nScope: %i\n", nScope);

  hb_itemClear(pItem);
  lIndexes = static_cast<HB_LONG>(hb_itemSize(thiswa->aOrders));

  if (lIndexes) { /* Exists opened orders ? */
    if (HB_IS_OBJECT(thiswa->oWorkArea)) {
      auto pOrder = hb_itemNew(nullptr);
      hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLORDERLISTNUM, 1, pOrder);
      hb_itemRelease(pOrder);
    } else {
      commonError(&thiswa->area, EG_DATATYPE, ESQLRDD_DATATYPE, nullptr);
      return Harbour::FAILURE;
    }
    lorder = hb_itemGetNL(hb_stackReturnItem());
    if (lorder) {
      pTag = hb_itemArrayGet(thiswa->aOrders, static_cast<HB_ULONG>(lorder));
      pTemp = hb_itemArrayGet(pTag, nScope ? BOTTOM_SCOPE : TOP_SCOPE);
      hb_itemCopy(pItem, pTemp);
      hb_itemRelease(pTag);
      hb_itemRelease(pTemp);
    }
  }
  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlSetFilter(SQLAREAP thiswa, LPDBFILTERINFO pFilterInfo) // RDDFUNC
{
  HB_ERRCODE res;

  if (thiswa->lpdbPendingRel) {
    if (SELF_FORCEREL(&thiswa->area) != Harbour::SUCCESS) {
      return Harbour::FAILURE;
    }
  }

  // Try to translate the filter expression into a valid SQL statement

  SUPER_CLEARFILTER(&thiswa->area);

  auto filtertext = hb_itemNew(pFilterInfo->abFilterText);

  hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLSETFILTER, 1, filtertext);
  hb_itemRelease(filtertext);
  res = hb_itemGetNI(hb_stackReturnItem());

  if (res == Harbour::SUCCESS) {
    thiswa->sqlfilter = true;
    return Harbour::SUCCESS;
  } else {
    return SUPER_SETFILTER(&thiswa->area, pFilterInfo);
  }
}

/*------------------------------------------------------------------------*/

#define sqlSetLocate nullptr

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlSetScope(SQLAREAP thiswa, LPDBORDSCOPEINFO sInfo) // RDDFUNC
{
  int res;

  // sr_TraceLog(nullptr, "sqlSetScope\n");

  auto scopetype = hb_itemPutNI(nullptr, sInfo->nScope);
  auto scopeval = hb_itemNew(sInfo->scopeValue);

#ifndef HB_CDP_SUPPORT_OFF
  if (HB_IS_STRING(scopeval)) {
    PHB_CODEPAGE cdpSrc = thiswa->cdPageCnv ? thiswa->cdPageCnv : hb_vmCDP();
    if (thiswa->area.cdPage && thiswa->area.cdPage != cdpSrc) {
      HB_SIZE nLen = hb_itemGetCLen(scopeval);
      char *pszVal = hb_cdpnDup(hb_itemGetCPtr(scopeval), &nLen, cdpSrc, thiswa->area.cdPage);
      hb_itemPutCLPtr(scopeval, pszVal, nLen);
    }
  }
#endif

  hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLSETSCOPE, 2, scopetype, scopeval);
  res = hb_itemGetNI(hb_stackReturnItem());
  hb_itemRelease(scopetype);
  hb_itemRelease(scopeval);

  if ((!res) && sr_GoTopOnScope()) {
    thiswa->firstinteract = true;
  }

  return (res == 0) ? Harbour::SUCCESS : Harbour::FAILURE;
}

/*------------------------------------------------------------------------*/

#define sqlSkipScope nullptr

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlLocate(SQLAREAP thiswa, HB_BOOL fContinue) // RDDFUNC
{
  HB_ERRCODE err;

  err = SUPER_LOCATE(&thiswa->area, fContinue);
  hb_arraySetL(thiswa->aInfo, AINFO_FOUND, thiswa->area.fFound);

  return err;
}

/*------------------------------------------------------------------------*/

#define sqlCompile nullptr
#define sqlError nullptr
#define sqlEvalBlock nullptr
#define sqlRawLock nullptr

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlLock(SQLAREAP thiswa, LPDBLOCKINFO pLockInfo)
{
  PHB_ITEM pRecord;

  // sr_TraceLog(nullptr, "sqlLock\n");

  if (thiswa->firstinteract) {
    SELF_GOTOP(&thiswa->area);
    thiswa->firstinteract = false;
  }

  if (hb_arrayGetL(thiswa->aInfo, AINFO_ISINSERT) || hb_arrayGetL(thiswa->aInfo, AINFO_EOF)) {
    pLockInfo->fResult = true;
    return Harbour::SUCCESS;
  }

  if (thiswa->shared) {
    auto pMethod = hb_itemPutNI(nullptr, pLockInfo->uiMethod);

    switch (pLockInfo->uiMethod) {
    case DBLM_EXCLUSIVE: {
      pRecord = hb_itemNew(nullptr);
      hb_arrayGet(thiswa->aInfo, AINFO_RECNO, pRecord);
      hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLLOCK, 2, pMethod, pRecord);
      hb_itemRelease(pRecord);
      pLockInfo->fResult = static_cast<HB_USHORT>(hb_itemGetL(hb_stackReturnItem()));
      break;
    }
    case DBLM_MULTIPLE: {
      hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLLOCK, 2, pMethod, pLockInfo->itmRecID);
      pLockInfo->fResult = static_cast<HB_USHORT>(hb_itemGetL(hb_stackReturnItem()));
      break;
    }
    case DBLM_FILE: {
      hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLLOCK, 1, pMethod);
      pLockInfo->fResult = static_cast<HB_USHORT>(hb_itemGetL(hb_stackReturnItem()));
      break;
    }
    default: {
      pLockInfo->fResult = false;
    }
    }
    hb_itemRelease(pMethod);
  } else {
    pLockInfo->fResult = true;
  }

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlUnLock(SQLAREAP thiswa, PHB_ITEM pRecNo)
{
  if (thiswa->firstinteract) {
    SELF_GOTOP(&thiswa->area);
    thiswa->firstinteract = false;
  }

  if (pRecNo) {
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLUNLOCK, 1, pRecNo);
  } else {
    hb_objSendMessage(thiswa->oWorkArea, s_pSym_SQLUNLOCK, 0);
  }

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

#define sqlCloseMemFile nullptr
#define sqlCreateMemFile nullptr
#define sqlGetValueFile nullptr
#define sqlOpenMemFile nullptr
#define sqlPutValueFile nullptr
#define sqlReadDBHeader nullptr
#define sqlWriteDBHeader nullptr
#define sqlInit nullptr

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlExit(LPRDDNODE pRDD) // RDDFUNC
{
  HB_SYMBOL_UNUSED(pRDD);
  HB_TRACE(HB_TR_DEBUG, ("sqlExit(%p)", pRDD));

  if (!s_pSym_SQLEXIT) {
    s_pSym_SQLEXIT = hb_dynsymFindName("SR_END");
  }
  hb_vmPushDynSym(s_pSym_SQLEXIT);
  hb_vmPushNil();
  hb_vmDo(0);
  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlDrop(PHB_ITEM pItemTable) // RDDFUNC
{
  // sr_TraceLog(nullptr, "sqlDrop\n");

  hb_vmPushDynSym(s_pSym_WORKAREA);
  hb_vmPushNil();
  hb_vmDo(0);

  if (hb_stackReturnItem()) {
    hb_objSendMessage(hb_stackReturnItem(), s_pSym_SQLDROP, 1, pItemTable);
  } else {
    commonError(nullptr, EG_DATATYPE, ESQLRDD_DATATYPE, nullptr);
    return Harbour::FAILURE;
  }

  return Harbour::SUCCESS;
}

/* returns 1 if exists, 0 else */
/*------------------------------------------------------------------------*/

static HB_BOOL sqlExists(PHB_ITEM pItemTable, PHB_ITEM pItemIndex) // RDDFUNC
{
  // sr_TraceLog(nullptr, "sqlExists\n");

  hb_vmPushDynSym(s_pSym_WORKAREA);
  hb_vmPushNil();
  hb_vmDo(0);

  if (HB_IS_OBJECT(hb_stackReturnItem())) {
    if (pItemTable) {
      hb_objSendMessage(hb_stackReturnItem(), s_pSym_SQLEXISTS, 1, pItemTable);
    } else if (pItemIndex) {
      hb_objSendMessage(hb_stackReturnItem(), s_pSym_SQLEXISTS, 1, pItemIndex);
    } else {
      return Harbour::FAILURE;
    }
  } else {
    commonError(nullptr, EG_DATATYPE, ESQLRDD_DATATYPE, nullptr);
    return Harbour::FAILURE;
  }

  return hb_itemGetL(hb_stackReturnItem());
}

/*------------------------------------------------------------------------*/

static HB_ERRCODE sqlRddInfo(LPRDDNODE pRDD, HB_USHORT uiIndex, HB_ULONG ulConnect, PHB_ITEM pItem) // RDDFUNC
{
  HB_TRACE(HB_TR_DEBUG, ("sqlRddInfo(%p, %hu, %lu, %p)", pRDD, uiIndex, ulConnect, pItem));

  switch (uiIndex) {
  case RDDI_ORDBAGEXT:
  case RDDI_ORDEREXT:
  case RDDI_ORDSTRUCTEXT: {
    hb_itemPutC(pItem, nullptr);
    break;
  }
  case RDDI_MULTITAG:
  case RDDI_SORTRECNO:
  case RDDI_STRUCTORD:
  case RDDI_MULTIKEY: {
    hb_itemPutL(pItem, true);
    break;
  }
  default: {
    return SUPER_RDDINFO(pRDD, uiIndex, ulConnect, pItem);
  }
  }

  return Harbour::SUCCESS;
}

/*------------------------------------------------------------------------*/

#define sqlWhoCares nullptr

/*------------------------------------------------------------------------*/

static bool ProcessFields(SQLAREAP thiswa)
{
  if (hb_itemType(thiswa->aStruct) != Harbour::Item::ARRAY) {
    HB_TRACE(HB_TR_ALWAYS, ("SQLRDD: Invalid structure array"));
    return false;
  }

  auto numFields = static_cast<HB_LONG>(hb_itemSize(thiswa->aStruct));

  if (!numFields) {
    HB_TRACE(HB_TR_ALWAYS, ("SQLRDD: Empty structure array"));
    return false;
  }

  SELF_SETFIELDEXTENT(&thiswa->area, static_cast<HB_USHORT>(numFields));

  DBFIELDINFO field;
  HB_BYTE *fieldType;
  PHB_ITEM thisfield;

  for (HB_USHORT i = 1; i <= static_cast<HB_USHORT>(numFields); i++) {
    thisfield = hb_itemArrayGet(thiswa->aStruct, i);

    if (hb_itemType(thisfield) != Harbour::Item::ARRAY) {
      HB_TRACE(HB_TR_ALWAYS, ("SQLRDD: Empty structure field array: %i", i));
      return false;
    }

    /* Clear out the field */
    memset(&field, 0, sizeof(field));

    field.uiTypeExtended = 0;
    field.atomName = hb_arrayGetC(thisfield, static_cast<HB_USHORT>(1));
    field.uiDec = static_cast<HB_USHORT>(0);
    field.uiLen = static_cast<HB_USHORT>(hb_arrayGetNI(thisfield, static_cast<HB_USHORT>(3)));

    thiswa->uiBufferIndex[i - 1] = static_cast<int>(hb_arrayGetNI(thisfield, static_cast<HB_USHORT>(5)));

    fieldType =
        reinterpret_cast<unsigned char *>(const_cast<char *>(hb_arrayGetCPtr(thisfield, static_cast<HB_USHORT>(2))));

    switch (*fieldType) {
    case 'c':
    case 'C': {
      field.uiType = Harbour::DB::Field::STRING;
      break;
    }
    case 'm':
    case 'M': {
      field.uiType = Harbour::DB::Field::MEMO;
      break;
    }
    case 'n':
    case 'N': {
      field.uiType = Harbour::DB::Field::LONG;
      field.uiDec = static_cast<HB_USHORT>(hb_arrayGetNI(thisfield, static_cast<HB_USHORT>(4)));
      break;
    }
    case 'l':
    case 'L': {
      field.uiType = Harbour::DB::Field::LOGICAL;
      break;
    }
    case 'd':
    case 'D': {
      field.uiType = Harbour::DB::Field::DATE;
      break;
    }
    case 'v':
    case 'V': {
      field.uiType = Harbour::DB::Field::ANY;
      break;
    }
    // new field type
    case 't':
    case 'T': {
      if (field.uiLen == 4) {
        field.uiType = Harbour::DB::Field::TIME;
      } else {
        field.uiType = Harbour::DB::Field::TIMESTAMP;
      }
      break;
    }
    default: {
      field.uiType = Harbour::Item::NIL;
      break;
    }
    }

    if (field.uiType == Harbour::Item::NIL) {
      HB_TRACE(HB_TR_ALWAYS, ("SQLRDD: Unsuported datatype on field: %i", i));
      return false;
    }

    // Add the field

    if (SELF_ADDFIELD(&thiswa->area, (LPDBFIELDINFO)&field) != Harbour::SUCCESS) {
      HB_TRACE(HB_TR_ALWAYS, ("SQLRDD: Could not add field: %i", i));
    }
    hb_itemRelease(thisfield);
    hb_xfree((void *)field.atomName);
  }

  return true;
}

/*------------------------------------------------------------------------*/

static bool SetFields(SQLAREAP thiswa)
{
  if (hb_itemType(thiswa->aStruct) != Harbour::Item::ARRAY) {
    HB_TRACE(HB_TR_ALWAYS, ("SQLRDD: Invalid structure array"));
    return false;
  }

  auto numFields = static_cast<HB_LONG>(hb_itemSize(thiswa->aStruct));

  if (!numFields) {
    HB_TRACE(HB_TR_ALWAYS, ("SQLRDD: Empty structure array"));
    return false;
  }

  PHB_ITEM thisfield;

  for (HB_USHORT i = 1; i <= static_cast<HB_USHORT>(numFields); i++) {
    thisfield = hb_itemArrayGet(thiswa->aStruct, i);

    if (hb_itemType(thisfield) != Harbour::Item::ARRAY) {
      HB_TRACE(HB_TR_ALWAYS, ("SQLRDD: Empty structure field array: %i", i));
      return false;
    }

    thiswa->uiBufferIndex[i - 1] = static_cast<int>(hb_arrayGetNI(thisfield, static_cast<HB_USHORT>(5)));
    hb_itemRelease(thisfield);
  }

  return true;
}

/*------------------------------------------------------------------------*/

void commonError(AREAP thiswa, HB_USHORT uiGenCode, HB_USHORT uiSubCode, const char *filename)
{
  PHB_ITEM pError = hb_errNew();

  hb_errPutGenCode(pError, uiGenCode);
  hb_errPutSubCode(pError, uiSubCode);
  hb_errPutSeverity(pError, ES_ERROR);
  hb_errPutTries(pError, EF_NONE);
  hb_errPutSubSystem(pError, "SQLRDD");
#ifdef __XHARBOUR__
  hb_errPutModuleName(pError, "SQLRDD");
#endif

  hb_errPutDescription(pError, hb_langDGetErrorDesc(uiGenCode));

  if (filename) {
    hb_errPutFileName(pError, filename);
  }

  SUPER_ERROR(thiswa, pError);

  hb_itemRelease(pError);

  return;
}

/*------------------------------------------------------------------------*/

HB_FUNC(ITEMCMP) /* ITEMCMP(cItem1, cItem2, nLenToCompare) ==> 0 == identical, < 0 if cItem1 < cIten2, > 0 == cItem1 >
                    cIten2 */
{
  int ret;

  auto val1 = hb_itemGetCPtr(hb_param(1, Harbour::Item::ANY));
  auto val2 = hb_itemGetCPtr(hb_param(2, Harbour::Item::ANY));
  ret = strncmp(val1, val2, hb_parnl(3));

  hb_retni(ret);
}

/*------------------------------------------------------------------------*/

HB_BOOL iTemCompEqual(PHB_ITEM pItem1, PHB_ITEM pItem2) // TODO: static ?
{
  if (HB_IS_NIL(pItem1) || HB_IS_NIL(pItem2)) {
    return false;
  }

  if (HB_IS_STRING(pItem1) && HB_IS_STRING(pItem2)) {
    return hb_itemStrCmp(pItem1, pItem2, false) == 0;
  } else if (HB_IS_NUMINT(pItem1) && HB_IS_NUMINT(pItem2)) {
    return hb_itemGetNInt(pItem1) == hb_itemGetNInt(pItem2);
  } else if (HB_IS_NUMERIC(pItem1) && HB_IS_NUMERIC(pItem2)) {
    return hb_itemGetND(pItem1) == hb_itemGetND(pItem2);
  } else if (HB_IS_DATE(pItem1) && HB_IS_DATE(pItem2)) {
    return hb_itemGetDL(pItem1) == hb_itemGetDL(pItem2);
  } else {
    return false;
  }
}

/*------------------------------------------------------------------------*/

#ifdef _WIN32

HB_FUNC(SR_GETCURRINSTANCEID)
{
  hb_retni(GetCurrentProcessId());
}

#else

HB_FUNC(SR_GETCURRINSTANCEID)
{
  pid_t Thispid = getpid();
  hb_retni(Thispid);
}

#endif

/*------------------------------------------------------------------------*/

// clang-format off
static const RDDFUNCS sqlTable = {

  /* Movement and positioning methods */

  (DBENTRYP_BP)sqlBof,
  (DBENTRYP_BP)sqlEof,
  (DBENTRYP_BP)sqlFound,
  (DBENTRYP_V)sqlGoBottom,
  (DBENTRYP_UL)sqlGoTo,
  (DBENTRYP_I)sqlGoToId,
  (DBENTRYP_V)sqlGoTop,
  (DBENTRYP_BIB)sqlSeek,
  (DBENTRYP_L)sqlSkip,
  (DBENTRYP_L)sqlSkipFilter,
  (DBENTRYP_L)sqlSkipRaw,

  /* Data management */

  (DBENTRYP_VF)sqlAddField,
  (DBENTRYP_B)sqlAppend,
  (DBENTRYP_I)sqlCreateFields,
  (DBENTRYP_V)sqlDeleteRec,
  (DBENTRYP_BP)sqlDeleted,
  (DBENTRYP_SP)sqlFieldCount,
  (DBENTRYP_VF)sqlFieldDisplay,
  (DBENTRYP_SSI)sqlFieldInfo,
  (DBENTRYP_SCP)sqlFieldName,
  (DBENTRYP_V)sqlFlush,
  (DBENTRYP_PP)sqlGetRec,
  (DBENTRYP_SI)sqlGetValue,
  (DBENTRYP_SVL)sqlGetVarLen,
  (DBENTRYP_V)sqlGoCold,
  (DBENTRYP_V)sqlGoHot,
  (DBENTRYP_P)sqlPutRec,
  (DBENTRYP_SI)sqlPutValue,
  (DBENTRYP_V)sqlRecall,
  (DBENTRYP_ULP)sqlRecCount,
  (DBENTRYP_ISI)sqlRecInfo,
  (DBENTRYP_ULP)sqlRecNo,
  (DBENTRYP_I)sqlRecId,
  (DBENTRYP_S)sqlSetFieldExtent,

  /* WorkArea/Database management */

  (DBENTRYP_CP)sqlAlias,
  (DBENTRYP_V)sqlClose,
  (DBENTRYP_VO)sqlCreate,
  (DBENTRYP_SI)sqlInfo,
  (DBENTRYP_V)sqlNewArea,
  (DBENTRYP_VO)sqlOpen,
  (DBENTRYP_V)sqlRelease,
  (DBENTRYP_SP)sqlStructSize,
  (DBENTRYP_CP)sqlSysName,
  (DBENTRYP_VEI)sqlEval,
  (DBENTRYP_V)sqlPack,
  (DBENTRYP_LSP)sqlPackRec,
  (DBENTRYP_VS)sqlSort,
  (DBENTRYP_VT)sqlTrans,
  (DBENTRYP_VT)sqlTransRec,
  (DBENTRYP_V)sqlZap,

  /* Relational Methods */

  (DBENTRYP_VR)sqlChildEnd,
  (DBENTRYP_VR)sqlChildStart,
  (DBENTRYP_VR)sqlChildSync,
  (DBENTRYP_V)sqlSyncChildren,
  (DBENTRYP_V)sqlClearRel,
  (DBENTRYP_V)sqlForceRel,
  (DBENTRYP_SSP)sqlRelArea,
  (DBENTRYP_VR)sqlRelEval,
  (DBENTRYP_SI)sqlRelText,
  (DBENTRYP_VR)sqlSetRel,

  /* Order Management */

  (DBENTRYP_VOI)sqlOrderListAdd,
  (DBENTRYP_V)sqlOrderListClear,
  (DBENTRYP_VOI)sqlOrderListDelete,
  (DBENTRYP_VOI)sqlOrderListFocus,
  (DBENTRYP_V)sqlOrderListRebuild,
  (DBENTRYP_VOO)sqlOrderCondition,
  (DBENTRYP_VOC)sqlOrderCreate,
  (DBENTRYP_VOI)sqlOrderDestroy,
  (DBENTRYP_SVOI)sqlOrderInfo,

  /* Filters and Scope Settings */

  (DBENTRYP_V)sqlClearFilter,
  (DBENTRYP_V)sqlClearLocate,
  (DBENTRYP_V)sqlClearScope,
  (DBENTRYP_VPLP)sqlCountScope,
  (DBENTRYP_I)sqlFilterText,
  (DBENTRYP_SI)sqlScopeInfo,
  (DBENTRYP_VFI)sqlSetFilter,
  (DBENTRYP_VLO)sqlSetLocate,
  (DBENTRYP_VOS)sqlSetScope,
  (DBENTRYP_VPL)sqlSkipScope,
  (DBENTRYP_B)sqlLocate,

  /* Miscellaneous */

  (DBENTRYP_CC)sqlCompile,
  (DBENTRYP_I)sqlError,
  (DBENTRYP_I)sqlEvalBlock,

  /* Network operations */

  (DBENTRYP_VSP)sqlRawLock,
  (DBENTRYP_VL)sqlLock,
  (DBENTRYP_I)sqlUnLock,

  /* Memofile functions */

  (DBENTRYP_V)sqlCloseMemFile,
  (DBENTRYP_VO)sqlCreateMemFile,
  (DBENTRYP_SCCS)sqlGetValueFile,
  (DBENTRYP_VO)sqlOpenMemFile,
  (DBENTRYP_SCCS)sqlPutValueFile,

  /* Database file header handling */

  (DBENTRYP_V)sqlReadDBHeader,
  (DBENTRYP_V)sqlWriteDBHeader,

  /* non WorkArea functions       */
  (DBENTRYP_R)sqlInit,
  (DBENTRYP_R)sqlExit,
  (DBENTRYP_RVVL)sqlDrop,
  (DBENTRYP_RVVL)sqlExists,
  (DBENTRYP_RVVVL) nullptr, /* sqlRename */
  (DBENTRYP_RSLV)sqlRddInfo,

  /* Special and reserved methods */

  (DBENTRYP_SVP)sqlWhoCares
};
// clang-format on

HB_FUNC(SQLRDD)
{
}

HB_FUNC(SQLRDD_GETFUNCTABLE)
{
  RDDFUNCS *pTable;
  HB_USHORT *uiCount;

  startSQLRDDSymbols();

  uiCount = static_cast<HB_USHORT *>(hb_parptr(1));
  pTable = static_cast<RDDFUNCS *>(hb_parptr(2));

  HB_TRACE(HB_TR_DEBUG, ("SQLRDD_GETFUNCTABLE(%p, %p)", uiCount, pTable));

  if (pTable) {
    HB_ERRCODE errCode;

    if (uiCount) {
      *uiCount = RDDFUNCSCOUNT;
    }
    errCode = hb_rddInherit(pTable, &sqlTable, &sqlrddSuper, nullptr);
    hb_retni(errCode);
  } else {
    hb_retni(Harbour::FAILURE);
  }
}

#define __PRG_SOURCE__ __FILE__

#ifdef HB_PCODE_VER
#undef HB_PRG_PCODE_VER
#define HB_PRG_PCODE_VER HB_PCODE_VER
#endif

static void hb_sqlrddRddInit(void *cargo)
{
  int usResult;
  HB_SYMBOL_UNUSED(cargo);

  usResult = hb_rddRegister("SQLRDD", RDT_FULL);
  if (usResult <= 1) {
    if (usResult == 0) {
      if (!s_pSym_SQLINIT) {
        s_pSym_SQLINIT = hb_dynsymFindName("SR_INIT");
      }
      hb_vmPushDynSym(s_pSym_SQLINIT);
      hb_vmPushNil();
      hb_vmDo(0);

      auto pDynSym = hb_dynsymFind("__SR_STARTSQL");

      if (pDynSym && hb_dynsymIsFunction(pDynSym)) {
        hb_vmPushDynSym(pDynSym);
        hb_vmPushNil();
        hb_vmDo(0);
      }
    }
    return;
  }

  hb_errInternal(HB_EI_RDDINVALID, nullptr, nullptr, nullptr);
}

HB_INIT_SYMBOLS_BEGIN(sqlrdd1__InitSymbols){"SQLRDD", {HB_FS_PUBLIC | HB_FS_LOCAL}, {HB_FUNCNAME(SQLRDD)}, nullptr},
    {"SQLRDD_GETFUNCTABLE",
     {HB_FS_PUBLIC | HB_FS_LOCAL},
     {HB_FUNCNAME(SQLRDD_GETFUNCTABLE)},
     nullptr} HB_INIT_SYMBOLS_END(sqlrdd1__InitSymbols)

        HB_CALL_ON_STARTUP_BEGIN(_hb_sqlrdd_rdd_init_) hb_vmAtInit(hb_sqlrddRddInit, nullptr);
HB_CALL_ON_STARTUP_END(_hb_sqlrdd_rdd_init_)

#if defined(HB_PRAGMA_STARTUP)
#pragma startup sqlrdd1__InitSymbols
#pragma startup _hb_sqlrdd_rdd_init_
#elif defined(HB_DATASEG_STARTUP)
#define HB_DATASEG_BODY                                                                                                \
  HB_DATASEG_FUNC(sqlrdd1__InitSymbols)                                                                                \
  HB_DATASEG_FUNC(_hb_sqlrdd_rdd_init_)
#include "hbiniseg.h"

#endif

HB_FUNC(SR_SETFOUND)
{
  auto pArea = static_cast<SQLAREAP>(hb_rddGetCurrentWorkAreaPointer());

  if (pArea) {
    auto pFound = hb_param(1, Harbour::Item::LOGICAL);
    if (pFound) {
      pArea->area.fFound = hb_itemGetL(pFound);
      hb_arraySetForward(pArea->aInfo, AINFO_FOUND, pFound);
    }
  } else {
    hb_errRT_DBCMD(EG_NOTABLE, EDBCMD_NOTABLE, nullptr, HB_ERR_FUNCNAME);
  }
}
