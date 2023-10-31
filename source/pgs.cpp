/*
 * SQLRDD Postgres native access routines
 * Copyright (c) 2003 - Marcelo Lombardo  <lombardo@uol.com.br>
 *
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

#include "compat.h"

#include <libpq-fe.h>

#include "sqlrddsetup.ch"
#include "sqlprototypes.h"
#include "pgs.ch"
#include "sqlodbc.ch"

#include <assert.h>

static PHB_DYNS s_pSym_SR_DESERIALIZE = nullptr;
static PHB_DYNS s_pSym_SR_FROMXML = nullptr;
static PHB_DYNS s_pSym_SR_FROMJSON = nullptr;
#define LOGFILE               "pgs.log"
struct _PSQL_SESSION
{
   int status;                   // Execution return value
   int numcols;                  // Result set columns
   int ifetch;                   // Fetch position in result set
   PGconn * dbh;                 // Connection handler
   PGresult * stmt;              // Current statement handler
   int iAffectedRows;            // Number of affected rows by command
};

using PSQL_SESSION = _PSQL_SESSION;

// culik 11/9/2010 variavel para setar o comportamento do postgresql


using PPSQL_SESSION = PSQL_SESSION *;

static void myNoticeProcessor(void * arg, const char * message)
{
   HB_SYMBOL_UNUSED(arg);
   HB_SYMBOL_UNUSED(message);
   // TraceLog("sqlerror.log", "%s", message);
}

HB_FUNC( PGSCONNECT ) /* PGSConnect(ConnectionString) => ConnHandle */
{
   // PPSQL_SESSION session = static_cast<PPSQL_SESSION>(hb_xgrab(sizeof(PSQL_SESSION)));
   auto session = static_cast<PPSQL_SESSION>(hb_xgrabz(sizeof(PSQL_SESSION)));
   const char * szConn = hb_parc(1);

//    memset(session, 0, sizeof(PSQL_SESSION));
   session->iAffectedRows = 0;
   session->dbh = PQconnectdb(szConn);

   session->ifetch = -2;
   /* Setup Postgres Notice Processor */
   PQsetNoticeProcessor(session->dbh, myNoticeProcessor, nullptr);
   hb_retptr(static_cast<void*>(session));
}

HB_FUNC( PGSFINISH )    /* PGSFinish(ConnHandle) */
{
   auto session = static_cast<PPSQL_SESSION>(hb_itemGetPtr(hb_param(1, HB_IT_POINTER)));
   assert(session->dbh != nullptr);
   PQfinish(session->dbh);
   hb_xfree(session);
   hb_ret();
}

HB_FUNC( PGSSTATUS ) /* PGSStatus(ConnHandle) => nStatus */
{
   auto session = static_cast<PPSQL_SESSION>(hb_itemGetPtr(hb_param(1, HB_IT_POINTER)));
   assert(session->dbh != nullptr);

   if( PQstatus(session->dbh) == CONNECTION_OK ) {
      hb_retni(SQL_SUCCESS);
   } else {
      hb_retni(SQL_ERROR);
   }
}

HB_FUNC( PGSSTATUS2 ) /* PGSStatus(ConnHandle) => nStatus */
{
   auto session = static_cast<PPSQL_SESSION>(hb_itemGetPtr(hb_param(1, HB_IT_POINTER)));
   assert(session->dbh != nullptr);
   hb_retni(static_cast<int>(PQstatus(session->dbh)));
}

HB_FUNC( PGSRESULTSTATUS ) /* PGSResultStatus(ResultSet) => nStatus */
{
   auto res = static_cast<PGresult*>(hb_itemGetPtr(hb_param(1, HB_IT_POINTER)));
   assert(res != nullptr);
   int ret = static_cast<int>(PQresultStatus(res));

   switch( ret ) {
      case PGRES_EMPTY_QUERY:
         ret = SQL_ERROR;
         break;
      case PGRES_COMMAND_OK:
         ret = SQL_SUCCESS;
         break;
      case PGRES_TUPLES_OK:
         ret = SQL_SUCCESS;
         break;
      case PGRES_BAD_RESPONSE:
         ret = SQL_ERROR;
         break;
      case PGRES_NONFATAL_ERROR:
         ret = SQL_SUCCESS_WITH_INFO;
         break;
      case PGRES_FATAL_ERROR :
         ret = SQL_ERROR;
         break;
   }

   hb_retni(ret);
}

HB_FUNC( PGSEXEC )      /* PGSExec(ConnHandle, cCommand) => ResultSet */
{
   /* TraceLog(nullptr, "PGSExec : %s\n", hb_parc(2)); */
   auto session = static_cast<PPSQL_SESSION>(hb_itemGetPtr(hb_param(1, HB_IT_POINTER)));
   assert(session->dbh != nullptr);

   session->stmt = PQexec(session->dbh, hb_parc(2));
   hb_retptr(static_cast<void*>(session->stmt));

   session->ifetch = -1;
   session->numcols = PQnfields(session->stmt);
   int ret = static_cast<int>(PQresultStatus(session->stmt));

   switch( ret ) {
      case PGRES_COMMAND_OK:
         session->iAffectedRows = static_cast<int>(atoi(PQcmdTuples(session->stmt)));
         break;
      default:
         session->iAffectedRows =0;
   }

}

HB_FUNC( PGSFETCH )     /* PGSFetch(ResultSet) => nStatus */
{
   auto session = static_cast<PPSQL_SESSION>(hb_itemGetPtr(hb_param(1, HB_IT_POINTER)));
   assert(session->dbh  != nullptr);
   assert(session->stmt != nullptr);

   int iTpl = PQresultStatus(session->stmt);
   session->iAffectedRows = 0;
   if( iTpl != PGRES_TUPLES_OK ) {
      hb_retni(SQL_INVALID_HANDLE);
   } else {
      if( session->ifetch >= -1 ) {
         session->ifetch++;
         iTpl = PQntuples(session->stmt) - 1;
         if( session->ifetch > iTpl ) {
            hb_retni(SQL_NO_DATA_FOUND);
         } else {
            session->iAffectedRows = static_cast<int>(iTpl);
            hb_retni(SQL_SUCCESS);
         }
      } else {
         hb_retni(SQL_INVALID_HANDLE);
      }
   }
}

HB_FUNC( PGSRESSTATUS ) /* PGSResStatus(ResultSet) => cErrMessage */
{
   auto session = static_cast<PPSQL_SESSION>(hb_itemGetPtr(hb_param(1, HB_IT_POINTER)));
   assert(session->dbh != nullptr);
   assert(session->stmt != nullptr);
   hb_retc(PQresStatus(PQresultStatus(session->stmt)));
}

HB_FUNC( PGSCLEAR )        /* PGSClear(ResultSet) */
{
   auto session = static_cast<PPSQL_SESSION>(hb_itemGetPtr(hb_param(1, HB_IT_POINTER)));
   assert(session->dbh != nullptr);
   if( session->stmt ) {
      PQclear(session->stmt);
      session->stmt = nullptr;
      session->ifetch = -2;
   }
}

HB_FUNC( PGSGETDATA )      /* PGSGetData(ResultSet, nColumn) => cValue */
{
   auto session = static_cast<PPSQL_SESSION>(hb_itemGetPtr(hb_param(1, HB_IT_POINTER)));
   assert(session->dbh != nullptr);
   assert(session->stmt != nullptr);
   hb_retc(PQgetvalue(session->stmt, session->ifetch, hb_parnl(2) - 1));
}

HB_FUNC( PGSCOLS ) /* PGSCols(ResultSet) => nColsInQuery */
{
   auto res = static_cast<PGresult*>(hb_itemGetPtr(hb_param(1, HB_IT_POINTER)));
   assert(res != nullptr);
   hb_retnl(static_cast<long>(PQnfields(res)));
}

HB_FUNC( PGSERRMSG ) /* PGSErrMsg(ConnHandle) => cErrorMessage */
{
   auto session = static_cast<PPSQL_SESSION>(hb_itemGetPtr(hb_param(1, HB_IT_POINTER)));
   assert(session->dbh != nullptr);
   hb_retc(PQerrorMessage(session->dbh));
}

HB_FUNC( PGSCOMMIT ) /* PGSCommit(ConnHandle) => nError */
{
   auto session = static_cast<PPSQL_SESSION>(hb_itemGetPtr(hb_param(1, HB_IT_POINTER)));
   assert(session->dbh != nullptr);
   PGresult * res = PQexec(session->dbh, "COMMIT");
   if( PQresultStatus(res) == PGRES_COMMAND_OK ) {
      hb_retni(SQL_SUCCESS);
   } else {
      hb_retni(SQL_ERROR);
   }
}

HB_FUNC( PGSROLLBACK )     /* PGSRollBack(ConnHandle) => nError */
{
   auto session = static_cast<PPSQL_SESSION>(hb_itemGetPtr(hb_param(1, HB_IT_POINTER)));
   assert(session->dbh != nullptr);
   PGresult * res = PQexec(session->dbh, "ROLLBACK");
   if( PQresultStatus(res) == PGRES_COMMAND_OK ) {
      hb_retni(SQL_SUCCESS);
   } else {
      hb_retni(SQL_ERROR);
   }
}

HB_FUNC( PGSQUERYATTR )     /* PGSQueryAttr(ResultSet) => aStruct */
{
   if( hb_pcount() != 1 ) {
      hb_retnl(-2);
      return;
   }

   auto session = static_cast<PPSQL_SESSION>(hb_itemGetPtr(hb_param(1, HB_IT_POINTER)));

   assert(session->dbh != nullptr);
   assert(session->stmt != nullptr);

   int rows = PQnfields(session->stmt);
   auto ret = hb_itemNew(nullptr);
   auto temp = hb_itemNew(nullptr);
   auto atemp = hb_itemNew(nullptr);

   hb_arrayNew(ret, rows);

   int type;
   HB_LONG typmod;

   for( int row = 0; row < rows; row++ ) {
      // long nullable;
      /* Column name */
      hb_arrayNew(atemp, 11);
      hb_itemPutC(temp, hb_strupr(PQfname(session->stmt, row)));
      hb_arraySetForward(atemp, FIELD_NAME, temp);
      hb_arraySetNL(atemp, FIELD_ENUM, row + 1);

      /* Data type, len, dec */
      type = static_cast<int>(PQftype(session->stmt, row));
      typmod = PQfmod(session->stmt, row);

      //nullable = PQgetisnull(session->stmt, row,PQfnumber(session->stmt, PQfname(session->stmt, row)));

      if( typmod < 0L ) {
         typmod = static_cast<HB_LONG>(PQfsize(session->stmt, row));
      }
/*
      if( typmod < 0L ) {
         typmod = 20L;
      }
*/
      switch( type ) {
         case CHAROID:
         case NAMEOID:
         case BPCHAROID:
         case VARCHAROID:
         case BYTEAOID:
         case ABSTIMEOID:
         case RELTIMEOID:
         case TINTERVALOID:
         case CASHOID:
         case MACADDROID:
         case INETOID:
         case CIDROID:
         case TIMETZOID:
//         case TIMESTAMPOID:
         //case TIMESTAMPTZOID:
            hb_itemPutC(temp, "C");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, typmod - 4));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_CHAR));
            break;
         case UNKNOWNOID:
            hb_itemPutC(temp, "C");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, PQgetlength(session->stmt, 0, row)));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_CHAR));
            break;
         case NUMERICOID:
            hb_itemPutC(temp, "N");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            if( typmod > 0 ) {
               hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, ((typmod - 4L) >> 16L)));
               hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, ((typmod - 4L) & 0xffff)));
            } else {
               hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 18));
               hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 6));
            }
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
            break;
         case BOOLOID:
            hb_itemPutC(temp, "L");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 1));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_BIT));
            break;
         case TEXTOID:
            hb_itemPutC(temp, "M");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 10));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_LONGVARCHAR));
            break;
         case XMLOID:
            hb_itemPutC(temp, "M");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 4));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_LONGVARCHARXML));
            break;

         case DATEOID:
            hb_itemPutC(temp, "D");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 8));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_DATE));
            break;
         case INT2OID:
            hb_itemPutC(temp, "N");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 6));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
            break;
         case INT8OID:
         case OIDOID:
            hb_itemPutC(temp, "N");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 20));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
            break;
         case INT4OID:
            hb_itemPutC(temp, "N");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 11));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
            break;
         case FLOAT4OID:
            hb_itemPutC(temp, "N");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 18));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 2));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
            break;
         case FLOAT8OID:
            hb_itemPutC(temp, "N");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 18));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 6));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
            break;
        // teste datetime
         case TIMESTAMPOID:
         case TIMESTAMPTZOID:
            hb_itemPutC(temp, "T");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 8));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_DATETIME));
            break;

         default:
            TraceLog(LOGFILE, "Strange data type returned in query: %i\n", type);
            break;
      }

      /* Nullable */
      hb_arraySetForward(atemp, FIELD_NULLABLE, hb_itemPutL(temp, false));
      /* add to main array */
      hb_arraySetForward(ret, row + 1, atemp);
   }
   hb_itemRelease(atemp);
   hb_itemRelease(temp);
   hb_itemReturnForward(ret);
   hb_itemRelease(ret);
}

HB_FUNC( PGSTABLEATTR )     /* PGSTableAttr(ConnHandle, cTableName) => aStruct */
{
   if( hb_pcount() < 3 ) {
      hb_retnl(-2);
      return;
   }

   char attcmm[512];
   PPSQL_SESSION session = static_cast<PPSQL_SESSION>(hb_itemGetPtr(hb_param(1, HB_IT_POINTER)));

   assert(session->dbh != nullptr);

   sprintf(attcmm, "select a.attname, a.atttypid, a.atttypmod, a.attnotnull from pg_attribute a left join pg_class b on a.attrelid = b.oid left join pg_namespace c on b.relnamespace = c.oid where a.attisdropped IS FALSE and a.attnum > 0 and b.relname = '%s' and c.nspname = '%s' order by attnum", hb_parc(2), hb_parc(3));

   PGresult * stmtTemp = PQexec(session->dbh, attcmm);

   if( PQresultStatus(stmtTemp) != PGRES_TUPLES_OK ) {
      TraceLog(LOGFILE, "Query error : %i - %s\n", PQresultStatus(stmtTemp), PQresStatus(PQresultStatus(stmtTemp)));
      PQclear(stmtTemp);
   }

   int rows = PQntuples(stmtTemp);
   auto ret = hb_itemNew(nullptr);
   auto atemp = hb_itemNew(nullptr);
   auto temp = hb_itemNew(nullptr);

   hb_arrayNew(ret, rows);

   for( int row = 0; row < rows; row++ ) {
      /* Column name */
      hb_arrayNew(atemp, 11);
      hb_itemPutC(temp, hb_strupr(PQgetvalue(stmtTemp, row, 0)));
      hb_arraySetForward(atemp, 1, temp);
      hb_arraySetNL(atemp, FIELD_ENUM, row + 1);

      /* Data type, len, dec */

      int type = atoi(PQgetvalue(stmtTemp, row, 1));
      long typmod = atol(PQgetvalue(stmtTemp, row, 2));
      long nullable;

      if( sr_iOldPgsBehavior() ) {
         nullable = 0;
      } else {
         if( strcmp(PQgetvalue(stmtTemp, row, 3), "f")  == 0 ) {
            nullable = 1;
         } else {
            nullable = 0;
         }
      }

      switch( type ) {
         case CHAROID:
         case NAMEOID:
         case BPCHAROID:
         case VARCHAROID:
         case BYTEAOID:
         case ABSTIMEOID:
         case RELTIMEOID:
         case TINTERVALOID:
         case CASHOID:
         case MACADDROID:
         case INETOID:
         case CIDROID:
         case TIMETZOID:
            hb_itemPutC(temp, "C");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, typmod - 4));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_CHAR));
            break;
         case NUMERICOID:
            hb_itemPutC(temp, "N");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, ((typmod - 4L) >> 16L)));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, ((typmod - 4L) & 0xffff)));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
            break;
         case BOOLOID:
            hb_itemPutC(temp, "L");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 1));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_BIT));
            break;
         case TEXTOID:
            hb_itemPutC(temp, "M");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 10));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_LONGVARCHAR));
            break;
         case XMLOID:
            hb_itemPutC(temp, "M");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 4));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_LONGVARCHARXML));
            break;

         case DATEOID:
            hb_itemPutC(temp, "D");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 8));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_DATE));
            break;
         case INT2OID:
            hb_itemPutC(temp, "N");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 6));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
            break;
         case INT8OID:
            hb_itemPutC(temp, "N");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 20));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
            break;
         case INT4OID:
            hb_itemPutC(temp, "N");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 11));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
            break;
         case FLOAT4OID:
            hb_itemPutC(temp, "N");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 18));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 2));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
            break;
         case FLOAT8OID:
            hb_itemPutC(temp, "N");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 18));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 6));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_NUMERIC));
            break;
         case TIMESTAMPOID:
         case TIMESTAMPTZOID:
            hb_itemPutC(temp, "T");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 8));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_DATETIME));
            break;
         case TIMEOID:
            hb_itemPutC(temp, "T");
            hb_arraySetForward(atemp, FIELD_TYPE, temp);
            hb_arraySetForward(atemp, FIELD_LEN, hb_itemPutNI(temp, 4));
            hb_arraySetForward(atemp, FIELD_DEC, hb_itemPutNI(temp, 0));
            hb_arraySetForward(atemp, FIELD_DOMAIN, hb_itemPutNI(temp, SQL_TIME));
            break;
         default:
            TraceLog(LOGFILE, "Strange data type returned: %i\n", type);
            break;
      }

      /* Nullable */

      hb_arraySetForward(atemp, FIELD_NULLABLE, hb_itemPutL(temp, nullable));

      /* add to main array */
      hb_arraySetForward(ret, row + 1, atemp);
   }
   hb_itemRelease(atemp);
   hb_itemRelease(temp);
   hb_itemReturnForward(ret);
   hb_itemRelease(ret);
   PQclear(stmtTemp);
}

//-----------------------------------------------------------------------------//

void PGSFieldGet(PHB_ITEM pField, PHB_ITEM pItem, char * bBuffer, HB_SIZE lLenBuff, HB_BOOL bQueryOnly, HB_ULONG ulSystemID, HB_BOOL bTranslate)
{
   HB_SYMBOL_UNUSED(bQueryOnly);
   HB_SYMBOL_UNUSED(ulSystemID);

   HB_LONG lType = static_cast<HB_LONG>(hb_arrayGetNL(pField, 6));
   HB_SIZE lLen = hb_arrayGetNL(pField, 3);
   HB_SIZE lDec = hb_arrayGetNL(pField, 4);

   PHB_ITEM pTemp;
   PHB_ITEM pTemp1;

   if( lLenBuff <= 0 ) { // database content is NULL
      switch( lType ) {
         case SQL_CHAR: {
            char * szResult = static_cast<char*>(hb_xgrab(lLen + 1));
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
            char dt[9] = {' ',' ',' ',' ',' ',' ',' ',' ','\0'};
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
            TraceLog(LOGFILE, "Invalid data type detected: %i\n", lType);
      }
   } else {
      switch( lType ) {
         case SQL_CHAR: {
            char * szResult = static_cast<char*>(hb_xgrab(lLen + 1));
            hb_xmemcpy(szResult, bBuffer, (lLen < lLenBuff ? lLen : lLenBuff));

            for( HB_SIZE lPos = lLenBuff; lPos < lLen; lPos++ ) {
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

            if( lLenBuff > 0 && (strncmp(bBuffer, "[", 1) == 0 || strncmp(bBuffer, "[]", 2) )&& (sr_lSerializeArrayAsJson()) ) {
               if( s_pSym_SR_FROMJSON == nullptr ) {
                  s_pSym_SR_FROMJSON = hb_dynsymFindName("HB_JSONDECODE");
                  if( s_pSym_SR_FROMJSON  == nullptr ) {
                     printf("Could not find Symbol HB_JSONDECODE\n");
                  }
               }
               hb_vmPushDynSym(s_pSym_SR_FROMJSON);
               hb_vmPushNil();
               hb_vmPushString(bBuffer, lLenBuff);
               pTemp = hb_itemNew(nullptr);
               hb_vmPush(pTemp);
               hb_vmDo(2);
               /* TOFIX: */
               hb_itemMove(pItem, pTemp);
               hb_itemRelease(pTemp);

            } else if( lLenBuff > 10 && strncmp(bBuffer, SQL_SERIALIZED_SIGNATURE, 10) == 0 && (!sr_lSerializedAsString()) ) {
               if( s_pSym_SR_DESERIALIZE == nullptr ) {
                  s_pSym_SR_DESERIALIZE = hb_dynsymFindName("SR_DESERIALIZE");
                  if( s_pSym_SR_DESERIALIZE  == nullptr ) {
                     printf("Could not find Symbol SR_DESERIALIZE\n");
                  }
               }
               hb_vmPushDynSym(s_pSym_SR_DESERIALIZE);
               hb_vmPushNil();
               hb_vmPushString(bBuffer, lLenBuff);

               hb_vmDo(1);

               pTemp = hb_itemNew(nullptr);
               hb_itemMove(pTemp, hb_stackReturnItem());

               if( HB_IS_HASH(pTemp) && sr_isMultilang() && bTranslate ) {
                  PHB_ITEM pLangItem = hb_itemNew(nullptr);
                  HB_SIZE ulPos;
                  if(    hb_hashScan(pTemp, sr_getBaseLang(pLangItem), &ulPos)
                      || hb_hashScan(pTemp, sr_getSecondLang(pLangItem), &ulPos)
                      || hb_hashScan(pTemp, sr_getRootLang(pLangItem), &ulPos) ) {
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
         // xmltoarray
         case SQL_LONGVARCHARXML: {

               if( s_pSym_SR_FROMXML == nullptr ) {
                  s_pSym_SR_FROMXML = hb_dynsymFindName("SR_FROMXML");
                  if( s_pSym_SR_FROMXML  == nullptr ) {
                     printf("Could not find Symbol SR_DESERIALIZE\n");
                  }
               }
               pTemp1 = hb_itemArrayNew(0);
               hb_vmPushDynSym(s_pSym_SR_FROMXML);
               hb_vmPushNil();
               hb_vmPushNil();
               hb_vmPush(pTemp1);
               hb_vmPushLong(-1);
               hb_vmPushString(bBuffer, lLenBuff);
               hb_vmDo(4);

               pTemp = hb_itemNew(nullptr);
               hb_itemMove(pTemp, hb_stackReturnItem());


               hb_itemMove(pItem, pTemp);

               hb_itemRelease(pTemp);
            break;
         }

         case SQL_BIT: {
            hb_itemPutL(pItem, bBuffer[0] == 't' ? true : false);
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
           long  lMilliSec;
            lMilliSec = hb_timeUnformat(bBuffer, nullptr); // TOCHECK:
            hb_itemPutTDT(pItem, 0, lMilliSec);
            break;
         }
         default:
            TraceLog(LOGFILE, "Invalid data type detected: %i\n", lType);
      }
   }
}

//-----------------------------------------------------------------------------//

HB_FUNC( PGSLINEPROCESSED )
{
   auto session = static_cast<PPSQL_SESSION>(hb_itemGetPtr(hb_param(1, HB_IT_POINTER)));
   PHB_ITEM temp;
   HB_USHORT i;
   char * col;
   PHB_ITEM pFields = hb_param(3, HB_IT_ARRAY);
   bool bQueryOnly = hb_parl(4);
   HB_ULONG ulSystemID = hb_parnl(5);
   bool bTranslate = hb_parl(6);
   PHB_ITEM pRet = hb_param(7, HB_IT_ARRAY);
   HB_LONG lIndex, cols;

   assert(session->dbh != nullptr);
   assert(session->stmt != nullptr);

   if( session ) {
      cols = hb_arrayLen(pFields);

      for( i = 0; i < cols; i++ ) {
         temp = hb_itemNew(nullptr);
         lIndex = hb_arrayGetNL(hb_arrayGetItemPtr(pFields, i + 1), FIELD_ENUM);

         if( lIndex != 0 ) {
            col = PQgetvalue(session->stmt, session->ifetch, lIndex - 1);
            PGSFieldGet(hb_arrayGetItemPtr(pFields, i + 1), temp, static_cast<char*>(col), strlen(col), bQueryOnly, ulSystemID, bTranslate);
         }
         hb_arraySetForward(pRet, i + 1, temp);
         hb_itemRelease(temp);
      }
   }
}

HB_FUNC( PGSAFFECTEDROWS )
{
   auto session = static_cast<PPSQL_SESSION>(hb_itemGetPtr(hb_param(1, HB_IT_POINTER)));
   if( session ) {
      hb_retni(session->iAffectedRows);
      return;
   }
   hb_retni(0);
}

//-----------------------------------------------------------------------------//
