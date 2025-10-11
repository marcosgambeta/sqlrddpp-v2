/*
 * SQLRDD Mysql native connection
 * Copyright (c) 2006 - Marcelo Lombardo <lombardo@uol.com.br>
 *
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA (or visit the web site http://www.gnu.org/).
 *
 * As a special exception, the xHarbour Project gives permission for
 * additional uses of the text contained in its release of xHarbour.
 *
 * The exception is that, if you link the xHarbour libraries with other
 * files to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the xHarbour library code into it.
 *
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 *
 * This exception applies only to the code released by the xHarbour
 * Project under the name xHarbour.  If you copy code from other
 * xHarbour Project or Free Software Foundation releases into a copy of
 * xHarbour, as the General Public License permits, the exception does
 * not apply to the code that you add in this way.  To avoid misleading
 * anyone as to the status of such modified files, you must delete
 * this exception notice from them.
 *
 * If you write modifications of your own for xHarbour, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice.
 *
 */

/* this is workaround for problems with xHarbour core header files which
 * define _WINSOCKAPI_ what effectively breaks compilation of code using
 * sockets. It means that we have to include windows.h before xHarbour
 * header files.
 */
#if defined(WINNT) || defined(_Windows) || defined(__NT__) || defined(_WIN32) || defined(_WINDOWS_) ||                 \
    defined(__WINDOWS_386__) || defined(__WIN32__)
#include <windows.h>
#endif

#include "compat.h"

#include "sqlrddsetup.ch"
#include "sqlprototypes.h"

#include "mysql.ch"
#include <mysql.h>
#include <mysqld_error.h>
#include <errmsg.h>
#include "sqlodbc.ch"

#include <assert.h>

#define MYSQL_OK 0

#define CLIENT_ALL_FLAGS (CLIENT_COMPRESS | CLIENT_MULTI_RESULTS | CLIENT_MULTI_STATEMENTS)
#define CLIENT_ALL_FLAGS2 (CLIENT_MULTI_RESULTS | CLIENT_MULTI_STATEMENTS)
static PHB_DYNS s_pSym_SR_DESERIALIZE = nullptr;
static PHB_DYNS s_pSym_SR_FROMJSON = nullptr;
static int iConnectionCount = 0;
#define LOGFILE "mysql.log"
struct _MYSQL_SESSION
{
  int status;                   // Execution return value
  int numcols;                  // Result set columns
  int ifetch;                   // Fetch position in result set
  MYSQL *dbh;                   // Connection handler
  MYSQL_RES *stmt;              // Current statement handler
  HB_ULONGLONG ulAffected_rows; // Number of affected rows
};

using MYSQL_SESSION = _MYSQL_SESSION;
using PMYSQL_SESSION = MYSQL_SESSION *;

HB_FUNC(MYSCONNECT)
{
  // auto session = static_cast<PMYSQL_SESSION>(hb_xgrab(sizeof(MYSQL_SESSION)));
  auto session = static_cast<PMYSQL_SESSION>(hb_xgrabz(sizeof(MYSQL_SESSION)));
  auto szHost = hb_parc(1);
  auto szUser = hb_parc(2);
  auto szPass = hb_parc(3);
  auto szDb = hb_parc(4);
  HB_UINT uiPort = HB_ISNUM(5) ? hb_parnl(5) : MYSQL_PORT;
  HB_UINT uiTimeout = HB_ISNUM(7) ? hb_parnl(7) : 3600;
  bool lCompress = HB_ISLOG(8) ? hb_parl(8) : false;
  mysql_library_init(0, nullptr, nullptr);
  // memset(session, 0, sizeof(MYSQL_SESSION));

  session->dbh = mysql_init(static_cast<MYSQL *>(0));
  session->ifetch = -2;

  if (session->dbh != nullptr) {
    iConnectionCount++;
    mysql_options(session->dbh, MYSQL_OPT_CONNECT_TIMEOUT, reinterpret_cast<const char *>(&uiTimeout));
    if (lCompress) {
      mysql_real_connect(session->dbh, szHost, szUser, szPass, szDb, uiPort, nullptr, CLIENT_ALL_FLAGS);
    } else {
      mysql_real_connect(session->dbh, szHost, szUser, szPass, szDb, uiPort, nullptr, CLIENT_ALL_FLAGS2);
    }
    hb_retptr(static_cast<void *>(session));
  } else {
    mysql_close(nullptr);
    if (iConnectionCount == 0) {
      mysql_library_end();
    }
    hb_retptr(nullptr);
  }
}

HB_FUNC(MYSFINISH)
{
  auto session = static_cast<PMYSQL_SESSION>(hb_itemGetPtr(hb_param(1, Harbour::Item::POINTER)));
  assert(session != nullptr);
  assert(session->dbh != nullptr);
  mysql_close(session->dbh);

  hb_xfree(session);
  if (iConnectionCount > 0) {
    iConnectionCount--;
  }
  if (iConnectionCount == 0) {
    mysql_library_end();
  }
  hb_ret();
}

HB_FUNC(MYSGETCONNID)
{
  auto session = static_cast<PMYSQL_SESSION>(hb_itemGetPtr(hb_param(1, Harbour::Item::POINTER)));
  assert(session != nullptr);
  assert(session->dbh != nullptr);
  hb_retnl(mysql_thread_id(session->dbh));
}

HB_FUNC(MYSKILLCONNID)
{
  auto session = static_cast<PMYSQL_SESSION>(hb_itemGetPtr(hb_param(1, Harbour::Item::POINTER)));
  auto ulThreadID = static_cast<HB_ULONG>(hb_itemGetNL(hb_param(2, Harbour::Item::LONG)));
  assert(session != nullptr);
  assert(session->dbh != nullptr);
  hb_retni(mysql_kill(session->dbh, ulThreadID));
}

HB_FUNC(MYSEXEC)
{
  /* sr_TraceLog(nullptr, "mysqlExec : %s\n", hb_parc(2)); */
  auto session = static_cast<PMYSQL_SESSION>(hb_itemGetPtr(hb_param(1, Harbour::Item::POINTER)));
  auto szQuery = hb_parc(2);
  assert(session != nullptr);
  assert(session->dbh != nullptr);
  session->ulAffected_rows = 0;
  // mysql_query(session->dbh, szQuery);
  mysql_real_query(session->dbh, szQuery, hb_parclen(2));
  session->stmt = mysql_store_result(session->dbh);
  session->ulAffected_rows = mysql_affected_rows(session->dbh);
  if (session->stmt) {
    session->numcols = mysql_num_fields(session->stmt);
  } else {
    session->numcols = 0;
  }
  hb_retptr(static_cast<void *>(session->stmt));
  session->ifetch = -1;
}

HB_FUNC(MYSFETCH) /* MYSFetch(ConnHandle, ResultSet) => nStatus */
{
  auto session = static_cast<PMYSQL_SESSION>(hb_itemGetPtr(hb_param(1, Harbour::Item::POINTER)));

  assert(session != nullptr);
  assert(session->dbh != nullptr);
  assert(session->stmt != nullptr);

  session->status = mysql_errno(session->dbh);
  int rows;

  if (session->status != MYSQL_OK) {
    hb_retni(SQL_INVALID_HANDLE);
  } else {
    if (session->ifetch >= -1) {
      session->ifetch++;
      rows = static_cast<int>(mysql_num_rows(session->stmt) - 1);

      if (session->ifetch > rows) {
        hb_retni(SQL_NO_DATA_FOUND);
      } else {
        hb_retni(SQL_SUCCESS);
      }
    } else {
      hb_retni(SQL_INVALID_HANDLE);
    }
  }
}

//-----------------------------------------------------------------------------//

void MSQLFieldGet(PHB_ITEM pField, PHB_ITEM pItem, char *bBuffer, HB_SIZE lLenBuff, HB_BOOL bQueryOnly,
                  HB_ULONG ulSystemID, HB_BOOL bTranslate)
{
  HB_SYMBOL_UNUSED(bQueryOnly);
  HB_SYMBOL_UNUSED(ulSystemID);

  auto lType = hb_arrayGetNL(pField, FIELD_DOMAIN);
  HB_SIZE lLen = hb_arrayGetNL(pField, FIELD_LEN);
  HB_SIZE lDec = hb_arrayGetNL(pField, FIELD_DEC);

  PHB_ITEM pTemp;

  if (lLenBuff <= 0) { // database content is NULL
    switch (lType) {
    case SQL_CHAR: {
      auto szResult = static_cast<char *>(hb_xgrab(lLen + 1));
      hb_xmemset(szResult, ' ', lLen);
      szResult[lLen] = '\0';
      hb_itemPutCLPtr(pItem, szResult, lLen);
      break;
    }
    case SQL_NUMERIC:
    case SQL_FAKE_NUM: {
      char szResult[2] = {' ', '\0'};
      sr_escapeNumber(szResult, lLen, lDec, pItem);
      break;
    }
    case SQL_DATE: {
      char dt[9] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\0'};
      hb_itemPutDS(pItem, dt);
      break;
    }
    case SQL_LONGVARCHAR: {
      hb_itemPutCL(pItem, bBuffer, 0);
      break;
    }
    case SQL_BIT: {
      hb_itemPutL(pItem, false);
      break;
    }

#ifdef SQLRDD_TOPCONN
    case SQL_FAKE_DATE: {
      hb_itemPutDS(pItem, bBuffer);
      break;
    }
#endif
    case SQL_DATETIME: {
      hb_itemPutTDT(pItem, 0, 0);
      break;
    }
    case SQL_TIME: {
      hb_itemPutTDT(pItem, 0, 0);
      break;
    }

    default:
      sr_TraceLog(LOGFILE, "Invalid data type detected: %i\n", lType);
    }
  } else {
    switch (lType) {
    case SQL_CHAR: {
      auto szResult = static_cast<char *>(hb_xgrab(lLen + 1));
      memset(szResult, ' ', lLen);
      hb_xmemcpy(szResult, bBuffer, (lLen < lLenBuff ? lLen : lLenBuff));

      for (HB_SIZE lPos = lLenBuff; lPos < lLen; lPos++) {
        szResult[lPos] = ' ';
      }
      szResult[lLen] = '\0';
      hb_itemPutCLPtr(pItem, szResult, lLen);
      break;
    }
    case SQL_NUMERIC: {
      sr_escapeNumber(bBuffer, lLen, lDec, pItem);
      break;
    }
    case SQL_DATE: {
      char dt[9];
      dt[0] = bBuffer[0];
      dt[1] = bBuffer[1];
      dt[2] = bBuffer[2];
      dt[3] = bBuffer[3];
      dt[4] = bBuffer[5];
      dt[5] = bBuffer[6];
      dt[6] = bBuffer[8];
      dt[7] = bBuffer[9];
      dt[8] = '\0';
      hb_itemPutDS(pItem, dt);
      break;
    }
    case SQL_LONGVARCHAR: {
      if (lLenBuff > 0 && (strncmp(bBuffer, "[", 1) == 0 || strncmp(bBuffer, "[]", 2)) &&
          (sr_lSerializeArrayAsJson())) {
        if (s_pSym_SR_FROMJSON == nullptr) {
          s_pSym_SR_FROMJSON = hb_dynsymFindName("HB_JSONDECODE");
          if (s_pSym_SR_FROMJSON == nullptr) {
            printf("Could not find Symbol HB_JSONDECODE\n");
          }
        }
        hb_vmPushDynSym(s_pSym_SR_FROMJSON);
        hb_vmPushNil();
        hb_vmPushString(bBuffer, lLenBuff);
        pTemp = hb_itemNew(nullptr);
        hb_vmPush(pTemp);
        hb_vmDo(2);
        hb_itemMove(pItem, pTemp);
        hb_itemRelease(pTemp);
      } else if (lLenBuff > 10 && strncmp(bBuffer, SQL_SERIALIZED_SIGNATURE, 10) == 0 && (!sr_lSerializedAsString())) {
        if (s_pSym_SR_DESERIALIZE == nullptr) {
          s_pSym_SR_DESERIALIZE = hb_dynsymFindName("SR_DESERIALIZE");
          if (s_pSym_SR_DESERIALIZE == nullptr) {
            printf("Could not find Symbol SR_DESERIALIZE\n");
          }
        }
        hb_vmPushDynSym(s_pSym_SR_DESERIALIZE);
        hb_vmPushNil();
        hb_vmPushString(bBuffer, lLenBuff);
        hb_vmDo(1);

        pTemp = hb_itemNew(nullptr);
        hb_itemMove(pTemp, hb_stackReturnItem());

        if (HB_IS_HASH(pTemp) && sr_isMultilang() && bTranslate) {
          auto pLangItem = hb_itemNew(nullptr);
          HB_SIZE ulPos;
          if (hb_hashScan(pTemp, sr_getBaseLang(pLangItem), &ulPos) ||
              hb_hashScan(pTemp, sr_getSecondLang(pLangItem), &ulPos) ||
              hb_hashScan(pTemp, sr_getRootLang(pLangItem), &ulPos)) {
            hb_itemCopy(pItem, hb_hashGetValueAt(pTemp, ulPos));
          }
          hb_itemRelease(pLangItem);
        } else {
          hb_itemMove(pItem, pTemp);
        }
        hb_itemRelease(pTemp);
      } else {
        hb_itemPutCL(pItem, bBuffer, lLenBuff);
      }
      break;
    }
    case SQL_BIT: {
      hb_itemPutL(pItem, bBuffer[0] == '1' ? true : false);
      break;
    }
#ifdef SQLRDD_TOPCONN
    case SQL_FAKE_DATE: {
      hb_itemPutDS(pItem, bBuffer);
      break;
    }
#endif
    case SQL_DATETIME: {
      long lJulian, lMilliSec;
      hb_timeStampStrGetDT(bBuffer, &lJulian, &lMilliSec);
      hb_itemPutTDT(pItem, lJulian, lMilliSec);
      break;
    }
    case SQL_TIME: {
      long lMilliSec;
      lMilliSec = hb_timeUnformat(bBuffer, nullptr); // TOCHECK:
      hb_itemPutTDT(pItem, 0, lMilliSec);
      break;
    }
    default:
      sr_TraceLog(LOGFILE, "Invalid data type detected: %i\n", lType);
    }
  }
}

//-----------------------------------------------------------------------------//

HB_FUNC(MYSLINEPROCESSED)
{
  auto session = static_cast<PMYSQL_SESSION>(hb_itemGetPtr(hb_param(1, Harbour::Item::POINTER)));
  int col, cols;
  PHB_ITEM temp;
  MYSQL_ROW thisrow;
  HB_ULONG *lens;
  HB_LONG lIndex;

  auto pFields = hb_param(3, Harbour::Item::ARRAY);
  bool bQueryOnly = hb_parl(4);
  HB_ULONG ulSystemID = hb_parnl(5);
  bool bTranslate = hb_parl(6);
  auto pRet = hb_param(7, Harbour::Item::ARRAY);

  assert(session != nullptr);
  assert(session->dbh != nullptr);
  assert(session->stmt != nullptr);

  session->status = mysql_errno(session->dbh);

  if (session->status != MYSQL_OK) {
    hb_retni(SQL_INVALID_HANDLE);
  } else {
    if (session->ifetch >= -1) {
      cols = hb_arrayLen(pFields);

      mysql_data_seek(session->stmt, session->ifetch);
      thisrow = mysql_fetch_row(session->stmt);
      lens = mysql_fetch_lengths(session->stmt);

      for (col = 0; col < cols; col++) {
        temp = hb_itemNew(nullptr);
        lIndex = hb_arrayGetNL(hb_arrayGetItemPtr(pFields, col + 1), FIELD_ENUM);

        if (lIndex != 0) {
          if (thisrow[lIndex - 1]) {
            MSQLFieldGet(hb_arrayGetItemPtr(pFields, col + 1), temp, static_cast<char *>(thisrow[lIndex - 1]),
                         lens[lIndex - 1], bQueryOnly, ulSystemID, bTranslate);
          } else {
            MSQLFieldGet(hb_arrayGetItemPtr(pFields, col + 1), temp, const_cast<char *>(""), 0, bQueryOnly, ulSystemID,
                         bTranslate);
          }
        }
        hb_arraySetForward(pRet, col + 1, temp);
        hb_itemRelease(temp);
      }
      hb_retni(SQL_SUCCESS);
    } else {
      hb_retni(SQL_INVALID_HANDLE);
    }
  }
}

HB_FUNC(MYSSTATUS)
{
  auto session = static_cast<PMYSQL_SESSION>(hb_itemGetPtr(hb_param(1, Harbour::Item::POINTER)));

  assert(session != nullptr);
  assert(session->dbh != nullptr);

  int ret = mysql_errno(session->dbh);

  if (ret == MYSQL_OK) {
    ret = SQL_SUCCESS;
  } else {
    ret = SQL_ERROR;
  }
  hb_retni(ret);
}

HB_FUNC(MYSRESULTSTATUS)
{
  auto session = static_cast<PMYSQL_SESSION>(hb_itemGetPtr(hb_param(1, Harbour::Item::POINTER)));

  assert(session != nullptr);
  assert(session->dbh != nullptr);

  auto ret = static_cast<HB_UINT>(mysql_errno(session->dbh));

  switch (ret) {
  case MYSQL_OK:
    ret = SQL_SUCCESS;
    break;
  case CR_COMMANDS_OUT_OF_SYNC:
  case CR_UNKNOWN_ERROR:
  case CR_SERVER_GONE_ERROR:
  case CR_SERVER_LOST:
  case ER_NO_DB_ERROR:
    ret = static_cast<HB_UINT>(SQL_ERROR);
    break;
  }
  hb_retnl(static_cast<HB_LONG>(ret));
}

HB_FUNC(MYSRESSTATUS)
{
  auto session = static_cast<PMYSQL_SESSION>(hb_itemGetPtr(hb_param(1, Harbour::Item::POINTER)));
  assert(session != nullptr);
  assert(session->dbh != nullptr);
  hb_retc(const_cast<char *>(mysql_error(session->dbh)));
}

HB_FUNC(MYSCLEAR)
{
  auto session = static_cast<PMYSQL_SESSION>(hb_itemGetPtr(hb_param(1, Harbour::Item::POINTER)));

  assert(session != nullptr);
  assert(session->dbh != nullptr);

  if (session->stmt) {
    mysql_free_result(session->stmt);
    session->stmt = nullptr;
    session->ifetch = -2;
  }
}

HB_FUNC(MYSCOLS)
{
  auto session = static_cast<PMYSQL_SESSION>(hb_itemGetPtr(hb_param(1, Harbour::Item::POINTER)));
  assert(session != nullptr);
  assert(session->dbh != nullptr);
  hb_retni(session->numcols);
}

HB_FUNC(MYSVERS) /* MYSVERS(hConnection) => nVersion */
{
  auto session = static_cast<PMYSQL_SESSION>(hb_itemGetPtr(hb_param(1, Harbour::Item::POINTER)));
  assert(session != nullptr);
  assert(session->dbh != nullptr);
  hb_retnl(static_cast<long>(mysql_get_server_version(session->dbh)));
}

HB_FUNC(MYSERRMSG)
{
  auto session = static_cast<PMYSQL_SESSION>(hb_itemGetPtr(hb_param(1, Harbour::Item::POINTER)));
  assert(session != nullptr);
  assert(session->dbh != nullptr);
  hb_retc(const_cast<char *>(mysql_error(session->dbh)));
}

HB_FUNC(MYSCOMMIT)
{
  auto session = static_cast<PMYSQL_SESSION>(hb_itemGetPtr(hb_param(1, Harbour::Item::POINTER)));

  assert(session != nullptr);
  assert(session->dbh != nullptr);

  if (mysql_commit(session->dbh)) {
    hb_retni(SQL_SUCCESS);
  } else {
    hb_retni(SQL_ERROR);
  }
}

HB_FUNC(MYSROLLBACK)
{
  auto session = static_cast<PMYSQL_SESSION>(hb_itemGetPtr(hb_param(1, Harbour::Item::POINTER)));

  assert(session != nullptr);
  assert(session->dbh != nullptr);

  if (mysql_rollback(session->dbh)) {
    hb_retni(SQL_SUCCESS);
  } else {
    hb_retni(SQL_ERROR);
  }
}

HB_FUNC(MYSQUERYATTR)
{
  if (hb_pcount() != 1) {
    hb_retnl(-2);
  }

  auto session = static_cast<PMYSQL_SESSION>(hb_itemGetPtr(hb_param(1, Harbour::Item::POINTER)));
  assert(session->dbh != nullptr);
  assert(session->stmt != nullptr);

  int rows = session->numcols;
  auto ret = hb_itemNew(nullptr);
  auto temp = hb_itemNew(nullptr);
  auto atemp = hb_itemNew(nullptr);

  hb_arrayNew(ret, rows);

  MYSQL_FIELD *field;
  int type;

  for (int row = 0; row < rows; row++) {

    /* Column name */
    field = mysql_fetch_field_direct(session->stmt, row);
    hb_arrayNew(atemp, FIELD_INFO_SIZE);
    hb_arraySetForward(atemp, FIELD_NAME, hb_itemPutC(temp, hb_strupr(field->name)));

    /* Data type, len, dec */
    type = field->type;
    switch (type) {
    case MYSQL_STRING_TYPE:
    case MYSQL_VAR_STRING_TYPE:
      // case MYSQL_DATETIME_TYPE:
      hb_arraySetForward(atemp, FIELD_TYPE, hb_itemPutC(temp, "C"));
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, static_cast<int>(field->length)));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_CHAR));
      break;
    case MYSQL_TINY_TYPE:
      hb_arraySetForward(atemp, FIELD_TYPE, hb_itemPutC(temp, "L"));
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 1));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_BIT));
      break;
    case MYSQL_TINY_BLOB_TYPE:
    case MYSQL_MEDIUM_BLOB_TYPE:
    case MYSQL_LONG_BLOB_TYPE:
    case MYSQL_BLOB_TYPE:
      hb_arraySetForward(atemp, FIELD_TYPE, hb_itemPutC(temp, "M"));
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 10));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_LONGVARCHAR));
      break;
    case MYSQL_DATE_TYPE:
      hb_arraySetForward(atemp, FIELD_TYPE, hb_itemPutC(temp, "D"));
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 8));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_DATE));
      break;
    case MYSQL_DATETIME_TYPE:
    case MYSQL_TIMESTAMP_TYPE:
      hb_arraySetForward(atemp, FIELD_TYPE, hb_itemPutC(temp, "T"));
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 8));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_DATETIME));
      break;
    case MYSQL_TIME_TYPE:
      hb_arraySetForward(atemp, FIELD_TYPE, hb_itemPutC(temp, "T"));
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 4));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_TIME));
      break;

    case MYSQL_SHORT_TYPE:
      hb_arraySetForward(atemp, FIELD_TYPE, hb_itemPutC(temp, "N"));
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 6));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
      break;
    case MYSQL_LONGLONG_TYPE:
      hb_arraySetForward(atemp, FIELD_TYPE, hb_itemPutC(temp, "N"));
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 20));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
      break;
    case MYSQL_LONG_TYPE:
      hb_arraySetForward(atemp, FIELD_TYPE, hb_itemPutC(temp, "N"));
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, HB_MIN(11, static_cast<int>(field->length))));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
      break;
    case MYSQL_INT24_TYPE:
      hb_arraySetForward(atemp, FIELD_TYPE, hb_itemPutC(temp, "N"));
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, HB_MIN(8, static_cast<int>(field->length))));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
      break;
    case MYSQL_FLOAT_TYPE:
    case MYSQL_DECIMAL_TYPE:
    case MYSQL_DOUBLE_TYPE:
    case MYSQL_NEWDECIMAL_TYPE:
      hb_arraySetForward(atemp, FIELD_TYPE, hb_itemPutC(temp, "N"));
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, static_cast<int>(field->length)));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, field->decimals));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
      break;
    default:
      sr_TraceLog(LOGFILE, "Invalid data type in query : %i\n", type);
    }

    /* Nullable */
    hb_arraySetForward(atemp, FIELD_NULLABLE, hb_itemPutL(temp, IS_NOT_NULL(field->flags) ? false : true));
    /* add to main array */
    hb_arraySetForward(ret, row + 1, atemp);
  }
  hb_itemRelease(atemp);
  hb_itemRelease(temp);
  hb_itemReturnForward(ret);
  hb_itemRelease(ret);
}

HB_FUNC(MYSTABLEATTR)
{
  if (hb_pcount() != 2) {
    hb_retnl(-2);
  }

  char attcmm[256] = {0};

  auto session = static_cast<PMYSQL_SESSION>(hb_itemGetPtr(hb_param(1, Harbour::Item::POINTER)));
  assert(session->dbh != nullptr);

  sprintf(attcmm, "select * from %s where 0 = 1", hb_parc(2));

  // mysql_query(session->dbh, attcmm);
  mysql_real_query(session->dbh, attcmm, strlen(attcmm));
  session->stmt = mysql_store_result(session->dbh);

  if (!session->stmt) {
    sr_TraceLog(LOGFILE, "Query error : %i - %s\n", mysql_errno(session->dbh), mysql_error(session->dbh));
  }

  auto ret = hb_itemNew(nullptr);
  auto temp = hb_itemNew(nullptr);
  auto atemp = hb_itemNew(nullptr);

  int rows = mysql_num_fields(session->stmt);
  hb_arrayNew(ret, rows);

  MYSQL_FIELD *field;
  int type;

  for (int row = 0; row < rows; row++) {
    field = mysql_fetch_field_direct(session->stmt, row);
    /* Column name */
    hb_arrayNew(atemp, 6);
    hb_itemPutC(temp, hb_strupr(field->name));
    hb_arraySetForward(atemp, 1, temp);

    /* Data type, len, dec */
    type = field->type;

    switch (type) {
    case MYSQL_STRING_TYPE:
    case MYSQL_VAR_STRING_TYPE:
      // case MYSQL_DATETIME_TYPE:
      hb_itemPutC(temp, "C");
      hb_arraySetForward(atemp, FIELD_TYPE, temp);
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, static_cast<int>(field->length)));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_CHAR));
      break;
    case MYSQL_TINY_TYPE:
      hb_itemPutC(temp, "L");
      hb_arraySetForward(atemp, FIELD_TYPE, temp);
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 1));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_BIT));
      break;
    case MYSQL_TINY_BLOB_TYPE:
    case MYSQL_MEDIUM_BLOB_TYPE:
    case MYSQL_LONG_BLOB_TYPE:
    case MYSQL_BLOB_TYPE:
      hb_itemPutC(temp, "M");
      hb_arraySetForward(atemp, FIELD_TYPE, temp);
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 10));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_LONGVARCHAR));
      break;
    case MYSQL_DATE_TYPE:
      hb_itemPutC(temp, "D");
      hb_arraySetForward(atemp, FIELD_TYPE, temp);
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 8));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_DATE));
      break;
    case MYSQL_DATETIME_TYPE:
    case MYSQL_TIMESTAMP_TYPE:
      hb_arraySetForward(atemp, FIELD_TYPE, hb_itemPutC(temp, "T"));
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 8));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_DATETIME));
      break;
    case MYSQL_TIME_TYPE:
      hb_arraySetForward(atemp, FIELD_TYPE, hb_itemPutC(temp, "T"));
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 4));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_TIME));
      break;

    case MYSQL_SHORT_TYPE:
      hb_itemPutC(temp, "N");
      hb_arraySetForward(atemp, FIELD_TYPE, temp);
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 6));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
      break;
    case MYSQL_LONGLONG_TYPE:
      hb_itemPutC(temp, "N");
      hb_arraySetForward(atemp, FIELD_TYPE, temp);
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 20));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
      break;
    case MYSQL_LONG_TYPE:
      hb_itemPutC(temp, "N");
      hb_arraySetForward(atemp, FIELD_TYPE, temp);
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, HB_MIN(11, static_cast<int>(field->length))));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
      break;
    case MYSQL_INT24_TYPE:
      hb_itemPutC(temp, "N");
      hb_arraySetForward(atemp, FIELD_TYPE, temp);
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, HB_MIN(8, static_cast<int>(field->length))));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
      break;
    case MYSQL_FLOAT_TYPE:
    case MYSQL_DECIMAL_TYPE:
    case MYSQL_DOUBLE_TYPE:
      hb_arraySetForward(atemp, FIELD_TYPE, hb_itemPutC(temp, "N"));
      hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, static_cast<int>(field->length)));
      hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, field->decimals));
      hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
      break;
    }

    /* Nullable */
    hb_arraySetForward(atemp, FIELD_NULLABLE, hb_itemPutL(temp, IS_NOT_NULL(field->flags) ? false : true));
    /* add to main array */
    hb_arraySetForward(ret, row + 1, atemp);
  }
  hb_itemRelease(atemp);
  hb_itemRelease(temp);
  hb_itemReturnForward(ret);
  hb_itemRelease(ret);
  mysql_free_result(session->stmt);
  session->stmt = nullptr;
}

HB_FUNC(MYSAFFECTEDROWS)
{
  auto session = static_cast<PMYSQL_SESSION>(hb_itemGetPtr(hb_param(1, Harbour::Item::POINTER)));

  if (session) {
    hb_retnll(session->ulAffected_rows);
    return;
  }

  hb_retni(0);
}

//-----------------------------------------------------------------------------//
