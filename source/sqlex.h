// ODBCRDD C Header
// Copyright (c) 2008 - Marcelo Lombardo  <lombardo@uol.com.br>

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

#ifndef ODBCRDD_H
#define ODBCRDD_H

// TODO: to be revised
#if _MSC_VER >= 1900
#pragma comment(lib, "legacy_stdio_definitions.lib")
#endif

#include <hbsetup.h>
#include <hbapi.h>
#include <hbapirdd.h>
#include <hbapiitm.h>
#include "sqlrdd.ch"
#include <string>

#define SUPERTABLE (&sqlExSuper)

//#define MAX_INDEXES 51    // 50 + Natural Order
constexpr int MAX_INDEXES = 51;
//#define MAX_INDEX_COLS 10 // Seek will work on indexes with up to MAX_INDEX_COLS columns
constexpr int MAX_INDEX_COLS = 10;
//#define PREPARED_SQL_LEN 400
constexpr int PREPARED_SQL_LEN = 400;
//#define RECORD_LIST_SIZE 250
constexpr int RECORD_LIST_SIZE = 250;
//#define COLUMN_BLOCK_SIZE 64
constexpr int COLUMN_BLOCK_SIZE = 64;
//#define FIELD_LIST_SIZE 6000
constexpr int FIELD_LIST_SIZE = 6000;
//#define FIELD_LIST_SIZE_PARAM 600
constexpr int FIELD_LIST_SIZE_PARAM = 600;
//#define MAX_SQL_QUERY_LEN 32000
constexpr int MAX_SQL_QUERY_LEN = 32000;
//#define PAGE_READ_SIZE 50
constexpr int PAGE_READ_SIZE = 50;
//#define BUFFER_POOL_SIZE 250
constexpr int BUFFER_POOL_SIZE = 250;
//#define DEFAULT_INDEX_COLUMN_MAX_LEN 200
constexpr int DEFAULT_INDEX_COLUMN_MAX_LEN = 200;
//#define INITIAL_MEMO_ALLOC 256
constexpr int INITIAL_MEMO_ALLOC = 256;

//                                                    1 1 11 1 1 11 1 12 2 2
//                                 0 1 2 34 5 6 7 8 9 0 1 23 4 5 67 8 90 1 2
static const char *openQuotes = "\"\"\"[\"\"\"\"\"\"\"\"`\"\"\"`\"\"`\"\"\"";
static const char *closeQuotes = "\"\"\"]\"\"\"\"\"\"\"\"`\"\"\"`\"\"`\"\"\"";

#define CHECK_SQL_N_OK(res) ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
#define OPEN_QUALIFIER(thiswa) (openQuotes[thiswa->nSystemID])
#define CLOSE_QUALIFIER(thiswa) (closeQuotes[thiswa->nSystemID])
#define RESULTSET_OK HB_SUCCESS + 10
#define RESULTSET_NOK HB_SUCCESS + 11

#undef HB_TRACE
#define HB_TRACE(x, y)

#define SQL_NVARCHAR -9
#define SQL_DB2_CLOB -99
#define SQL_FAKE_LOB -100
#define SQL_FAKE_DATE -101
#define SQL_FAKE_NUM -102

#ifdef __XHARBOUR__
#define HB_LONG LONG
#define HB_ULONG ULONG
#endif

// ODBC Column binding. I know that some information in structure below does exist in
// other parts... I mean, it's duplicated, but I prefer to waste a few bytes more and have
// things at hand, making this RDD faster

struct SQL_CHAR_STRUCT
{
  SQLCHAR *value;
  int size;
  int size_alloc;
};

struct INDEXBIND
{
  // Relative field position in aFields
  HB_LONG lFieldPosDB;
  // Index order
  HB_LONG hIndexOrder;
  // The current column in index
  int iLevel;
  // How many index columns in index
  int iIndexColumns;
  // Index stmt handle for SQL phrase in this level for FWD movment
  HSTMT SkipFwdStmt;
  // Index stmt handle for SQL phrase in this level for BWD movment
  HSTMT SkipBwdStmt;
  // Index stmt handle for SQL phrase in this level for FWD movment
  HSTMT SeekFwdStmt;
  // Index stmt handle for SQL phrase in this level for BWD movment
  HSTMT SeekBwdStmt;
  // Partial prepared query for debugging pourposes
  char SkipFwdSql[PREPARED_SQL_LEN];
  // Partial prepared query for debugging pourposes
  char SkipBwdSql[PREPARED_SQL_LEN];
  // Partial prepared query for debugging pourposes
  char SeekFwdSql[PREPARED_SQL_LEN];
  // Partial prepared query for debugging pourposes
  char SeekBwdSql[PREPARED_SQL_LEN];
};

struct COLUMNBIND
{
  // Parameter number in binded parameters
  int iParNum;
  // SQL data type of column
  int iSQLType;
  // C data type of the argument. It determines
  // the as* member to be used, like xHB iTem API 'type'
  int iCType;

  // Relative field position in aFields
  HB_LONG lFieldPosDB;
  // Relative field position in aBuffer. NULL if RECNO or DELETED column
  HB_LONG lFieldPosWA;
  // Fully qualified column name to be used in queries
  char *colName;
  // Support for char data types
  SQL_CHAR_STRUCT asChar;
  // Support for logical data type
  SQLCHAR asLogical;
  // I suppose ODBC driver will suport default
  // convertion to TIMESTAMP when needed
  SQL_DATE_STRUCT asDate;

  // Timestamp support, always converted to DATE
  SQL_TIMESTAMP_STRUCT asTimestamp;
  // I suppose all ODBC drivers has build in
  // convertion from this type to all types
  // of numeric variables in SQL
  SQLDOUBLE asNumeric;

  // To make an easy bind
  SQLUINTEGER ColumnSize;
  // To make an easy bind
  SQLSMALLINT DecimalDigits;
  // Is this column NULLABLE ?
  bool isNullable;
  // Value to be bound is NULL ?
  bool isArgumentNull;
  // Field was bound as NULL ?
  bool isBoundNULL;
  // Buffer lenght pointer to be used in SQLBindParam()
  SQLLEN lIndPtr;
  // Field is MEMO ?
  bool isMemo;
  // Fiels is multi language ?
  bool isMultiLang;
};

// SQL WORKAREA

struct SQLEXAREA
{
  AREA area;

  // SQLRDD's additions to the workarea structure
  // Warning: The above section MUST match WORKAREA exactly! Any
  // additions to the structure MUST be added below

  // Area's codepage convert pointer
  PHB_CODEPAGE cdPageCnv;
  // file name
  char *szDataFileName;
  // current index order
  HB_LONG hOrdCurrent;

  bool shared;
  // only SELECT allowed
  bool readonly;
  // TRUE when creating table
  bool creating;
  // TRUE when workarea was not used yet
  bool firstinteract;
  // ISAM Simulator ?
  bool isam;
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
  int iColumnListStatus;

  // Pointer to parent rel struct
  LPDBRELINFO lpdbPendingRel;

  // SQLEX's additions to the SQLRDD workarea structure

  // SQL connection object
  PHB_ITEM oSql;
  // Table structure in DB
  PHB_ITEM aFields;
  // Hash containing the Buffer Pool
  PHB_ITEM hBufferPool;
  // Existing Indexes in database catalog (SR_MGMNTINDEXES) array
  PHB_ITEM pIndexMgmnt;

  // Statement handle
  HSTMT hStmt;
  // Statement handle with prepared statement to retrieve line
  HSTMT hStmtBuffer;
  // Statement handle with prepared phrase to insert a new record
  HSTMT hStmtInsert;
  // Statement handle with prepared phrase to get inserted record
  HSTMT hStmtNextval;
  // Statement handle with prepared phrase to insert a new record
  HSTMT hStmtUpdate;
  // Database connection handle
  HDBC hDbc;
  // Connected database ID
  int nSystemID;
  // Should be filled by SR_GetCurrentRecordNum() to be used in SKIP bindings
  HB_ULONG lCurrentRecord;
  // Should be filled by SR_GetCurrentRecordNum() to be used in UPDATE bindings
  HB_ULONG lUpdatedRecord;
  // BOF Record optimizer
  HB_ULONG lBofAt;
  // EOF Record optimizer
  HB_ULONG lEofAt;
  // LastRec() + 1
  HB_ULONG lLastRec;
  // To be used with Binded Parameter
  HB_ULONG *lRecordToRetrieve;
  // record list to skip on
  HB_ULONG *recordList;
  // deleted list relative to record list
  char *deletedList;
  // record list position
  int recordListPos;
  // record list size
  int recordListSize;
  // LIST_FORWARD or LIST_BACKWARD
  int recordListDirection;
  // Top Connect compatible
  int iTCCompat;

  // current index column list lenght
  int indexColumns;
  // index column list level in current skip sequence
  int indexLevel;

  // 1 - FWD, (-1) - BWD, 0 - none
  int skipDirection;

  // field list string
  char *sFields;
  // Last SQL phrase
  char *sSql;
  // SQL Statement to get WA buffer
  char *sSqlBuffer;
  // Qualified table name
  std::string sTable; //char *sTable;
  // Database schema included in sTable
  char *sOwner;
  // Where clause
  char *sWhere;
  // Order By clause
  char *sOrderBy;
  // Recno column name
  std::string sRecnoName; //char *sRecnoName;
  // Deleted column name
  std::string sDeletedName; //char *sDeletedName;
  // String for recordset limit
  char sLimit1[50];
  // String for recordset limit
  char sLimit2[50];

  // Does it have to write buffer down to database ?
  bool bufferHot;
  // true if appending a new record
  bool bIsInsert;
  // Already checked for ODBC connection ?
  bool bConnVerified;
  // If current index is in DESCENDING order
  bool bReverseIndex;
  // Index column and prepared SQL expression handles for SKIP
  // INDEXBIND *IndexBindings[MAX_INDEXES];
  // Index column and prepared SQL expression handles for SKIP
  INDEXBIND **IndexBindings;

  // Single column retrieving statements
  HSTMT *colStmt;
  // If any of conditions like filters, scope, historic, has
  // changed, prepared statements handles for Record List
  // are no longer valid - USED FOR SKIP / GO TOP / BO BOTTOM
  bool bConditionChanged1;

  // If any of conditions like filters, scope, historic, has
  // changed, prepared statements handles for Record List
  // are no longer valid - USED FOR SEEK
  bool bConditionChanged2;

  // If order has changed, we should fix column bindings
  // before use then
  bool bOrderChanged;

  // If query for Seek must be recreated due to NULL interference
  bool bRebuildSeekQuery;
  // TRUE if workarea has historic
  bool bHistoric;
  // Column bindings to INSERT
  COLUMNBIND *InsertRecord;
  // Current record bindings for SKIP / UPDATE
  COLUMNBIND *CurrRecord;
  // Flags if a column was updated - must be cleared on every GO_COLD
  char editMask[MAX_FIELDS];
  // Copy of updateMask in currently prepared UPDATE stmt
  char updatedMask[MAX_FIELDS];
  // Same of updateMask but for special cols (INDKEY_xx and FORKEY_xx)
  char specialMask[MAX_FIELDS];
  // If any index column is affected by UPDATE
  bool bIndexTouchedInUpdate;
  // Table open is an select statement
  bool bIsSelect;
};

// prototypes

//int SR_sqlKeyCompare(AREAP thiswa, PHB_ITEM pKey, HB_BOOL fExact); NOTE: changed to static
//void SR_odbcFieldGet(PHB_ITEM pField, PHB_ITEM pItem, char *bBuffer, HB_ISIZ lLenBuff, HB_BOOL bQueryOnly,
//                  HB_ULONG ulSystemID, HB_BOOL bTranslate); NOTE: changed to static
char *SR_QuoteTrimEscapeString(char *FromBuffer, HB_ULONG iSize, int idatabase, HB_BOOL bRTrim, HB_ULONG *iSizeOut);
char *SR_quotedNull(PHB_ITEM pFieldData, PHB_ITEM pFieldLen, PHB_ITEM pFieldDec, HB_BOOL bNullable, int nSystemID,
                 HB_BOOL bTCCompat, HB_BOOL bMemo, HB_BOOL *bNullArgument);
HB_BOOL SR_itemEmpty(PHB_ITEM pItem);

namespace SQLRDD {
void commonError(AREAP ThisDb, const HB_USHORT uiGenCode, const HB_USHORT uiSubCode, const char *filename);
char *QualifyName(char *szName, SQLEXAREA *thiswa);
void odbcErrorDiag(HSTMT hStmt, const char *routine, const char *szSql, int line);
void odbcErrorDiagRTE(HSTMT hStmt, const char *routine, const char *szSql, SQLRETURN res, int line, const char *module);
}

HB_ERRCODE SR_SetBindEmptylValue(COLUMNBIND *BindStructure);
HB_ERRCODE SR_SetBindValue(PHB_ITEM pFieldData, COLUMNBIND *BindStructure, HSTMT hStmt);
COLUMNBIND *SR_GetBindStruct(SQLEXAREA *thiswa, INDEXBIND *IndexBind);
HB_BOOL SR_getColumnList(SQLEXAREA *thiswa);
void SR_SolveFilters(SQLEXAREA *thiswa, bool bWhere);
void SR_getOrderByExpression(SQLEXAREA *thiswa, HB_BOOL bUseOptimizerHints);
void SR_setResultSetLimit(SQLEXAREA *thiswa, int iRows);
void SR_SetIndexBindStructure(SQLEXAREA *thiswa);
void SR_SetInsertRecordStructure(SQLEXAREA *thiswa);
HB_ULONG SR_GetCurrentRecordNum(SQLEXAREA *thiswa);
extern void SR_odbcGetData(SQLHSTMT hStmt, PHB_ITEM pField, PHB_ITEM pItem, HB_BOOL bQueryOnly, HB_ULONG ulSystemID,
                        HB_BOOL bTranslate, HB_USHORT ui);

// INSERT and UPDATE prototypes

void SR_CreateInsertStmt(SQLEXAREA *thiswa);
void SR_SetCurrRecordStructure(SQLEXAREA *thiswa);
HB_ERRCODE SR_CreateUpdateStmt(SQLEXAREA *thiswa);
HB_ERRCODE SR_PrepareInsertStmt(SQLEXAREA *thiswa);
HB_ERRCODE SR_BindInsertColumns(SQLEXAREA *thiswa);
HB_ERRCODE SR_FeedRecordCols(SQLEXAREA *thiswa, HB_BOOL bUpdate);
HB_ERRCODE SR_ExecuteInsertStmt(SQLEXAREA *thiswa);
HB_ERRCODE SR_ExecuteUpdateStmt(SQLEXAREA *thiswa);

// SEEK Prototypes

HB_ERRCODE SR_FeedSeekKeyToBindings(SQLEXAREA *thiswa, PHB_ITEM pKey, int *queryLevel);
bool SR_CreateSeekStmt(SQLEXAREA *thiswa, int queryLevel);
void SR_BindSeekStmt(SQLEXAREA *thiswa, int queryLevel);
HB_ERRCODE SR_getPreparedSeek(SQLEXAREA *thiswa, int queryLevel, HB_USHORT *iIndex, HSTMT *hStmt);

#endif // ODBCRDD_H
