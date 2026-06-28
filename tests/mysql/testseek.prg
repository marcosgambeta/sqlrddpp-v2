// SQLRDD++
// test with MySQL
// To compile:
// hbmk2 testseek -llibmysql

#ifdef __XHARBOUR__
#xtranslate HB_PVALUE([<x,...>]) => PVALUE(<x>)
#endif

#include "sqlrdd.ch"

// To run the test:
// testseek <command line parameters>
// --server <servername>
// --port <port>
// --uid <username>
// --pwd <userpassword>
// --dtb <databasename>
// --table <tablename>
// --newtable
// --droptable
// --records <records>
// --times <times>
// NOTE: the database must exist before runnning the test.

#define RDD_NAME "SQLRDD"

REQUEST SQLRDD
REQUEST SR_MYSQL

STATIC s_SERVER     := "localhost"
STATIC s_PORT       := "3306"
STATIC s_UID        := "root"
STATIC s_PWD        := ""
STATIC s_DTB        := "dbtest"
STATIC s_TABLE_NAME := "testseek"
STATIC s_NEW_TABLE  := .F.
STATIC s_DROP_TABLE := .F.
STATIC s_RECORDS    := 1000
STATIC s_TIMES      := 2

PROCEDURE Main()

   LOCAL nConnection
   LOCAL n
   LOCAL nSeekFound
   LOCAL nSeekNotFound
   LOCAL nSeekFailed
   LOCAL nId
   LOCAL cName
   LOCAL dDate
   LOCAL nRand

   hb_RandomSeed()

   //SetMode(25, maxcol() + 1)

   //CLS

   n := 1
   DO WHILE n <= PCount()
      DO CASE
      CASE HB_PValue(n) == "--server"    ; s_SERVER := HB_PValue(++n)
      CASE HB_PValue(n) == "--port"      ; s_PORT := HB_PValue(++n)
      CASE HB_PValue(n) == "--uid"       ; s_UID := HB_PValue(++n)
      CASE HB_PValue(n) == "--pwd"       ; s_PWD := HB_PValue(++n)
      CASE HB_PValue(n) == "--dtb"       ; s_DTB := HB_PValue(++n)
      CASE HB_PValue(n) == "--table"     ; s_TABLE_NAME := HB_PValue(++n)
      CASE HB_PValue(n) == "--newtable"  ; s_NEW_TABLE := .T.
      CASE HB_PValue(n) == "--droptable" ; s_DROP_TABLE := .T.
      CASE HB_PValue(n) == "--records"   ; s_RECORDS := val(HB_PValue(++n))
      CASE HB_PValue(n) == "--times"     ; s_TIMES := val(HB_PValue(++n))
      ENDCASE
      ++n
   ENDDO

   IF s_RECORDS < 300
      ? "The minimum value for --records is 300"
      WAIT
      QUIT
   ENDIF

   rddSetDefault(RDD_NAME)

   nConnection := sr_AddConnection(CONNECT_MYSQL, "MySQL=" + s_SERVER + ";PORT=" + s_PORT + ";UID=" + s_UID + ";PWD=" + s_PWD + ";DTB=" + s_DTB)

   IF nConnection < 0
      ? "Connection error. See sqlerror.log for details."
      WAIT
      QUIT
   ENDIF

   sr_StartLog(nConnection)

   IF s_NEW_TABLE .AND. sr_ExistTable(s_TABLE_NAME)
      sr_DropIndex("index1")
      sr_DropIndex("index2")
      sr_DropIndex("index3")
      sr_DropIndex("index4")
      sr_DropTable(s_TABLE_NAME)
   ENDIF

   // TODO: add more data types
   IF !sr_ExistTable(s_TABLE_NAME)
      ? "Creating table"
      dbCreate(s_TABLE_NAME, {{"ID        ", "N", 10, 0}, ;
                              {"NAME      ", "C", 10, 0}, ;
                              {"DATE      ", "D",  8, 0}}, RDD_NAME)
   ENDIF

   ? "Opening table"
   USE (s_TABLE_NAME) EXCLUSIVE VIA (RDD_NAME)
   ? "bof()", bof()
   ? "eof()", eof()
   ? "reccount()", reccount()

   IF s_NEW_TABLE
      ? "Creating indexes"
      INDEX ON ID TO index1
      INDEX ON NAME TO index2
      INDEX ON DATE TO index3
      INDEX ON NAME + dtos(DATE) TO index4
   ENDIF

   IF reccount() == 0 // < s_RECORDS
      ? "Adding records"
      FOR n := 1 TO s_RECORDS
         APPEND BLANK
         REPLACE ID WITH n
         REPLACE NAME WITH strzero(n, 10)
         REPLACE DATE WITH date() - n
      NEXT n
   ENDIF

   ? "Closing table"
   USE

   ? "Opening table and indexes"
   USE (s_TABLE_NAME) INDEX index1,index2,index3,index4 EXCLUSIVE VIA (RDD_NAME)

   //-------------------------------------------------------------------------//

   ? "Testing index 1 (ID)"
   SET INDEX TO index1
   ? "ordkeycount=", ordkeycount(), iif(ordkeycount() == s_RECORDS, "OK", "ERROR")
   nSeekFound := 0
   nSeekNotFound := 0
   nSeekFailed := 0
   ? time()
   FOR n := 1 TO s_RECORDS * s_TIMES
      // create a valid id
      nID := hb_RandomInt(1, s_RECORDS)
      SEEK nID
      IF found() .AND. !bof() .AND. !eof() .AND. FIELD->ID == nId
         ++nSeekFound
      ELSE
         ++nSeekFailed
      ENDIF
      // create a invalid id
      nID := hb_RandomInt(s_RECORDS + 1, s_RECORDS + s_RECORDS)
      SEEK nId
      IF !found() .AND. !bof() .AND. eof()
         ++nSeekNotFound
      ELSE
         ++nSeekFailed
      ENDIF
   NEXT n
   ? time()
   // must be s_RECORDS * s_TIMES
   ? "nSeekFound", nSeekFound, iif(nSeekFound == s_RECORDS * s_TIMES, "OK", "ERROR")
   // must be s_RECORDS * s_TIMES
   ? "nSeekNotFound", nSeekNotFound, iif(nSeekNotFound == s_RECORDS * s_TIMES, "OK", "ERROR")
   // must be 0
   ? "nSeekFailed", nSeekFailed, iif(nSeekFailed == 0, "OK", "ERROR")

   //-------------------------------------------------------------------------//

   ? "Testing index 1 (ID) with scope"
   SET INDEX TO index1
   ? "ordkeycount (before scope)=", ordkeycount(), iif(ordkeycount() == s_RECORDS, "OK", "ERROR")
   SET SCOPE TO 101, (s_RECORDS - 100)
   ? "ordkeycount (after scope)=", ordkeycount(), iif(ordkeycount() == s_RECORDS - 200, "OK", "ERROR")
   nSeekFound := 0
   nSeekNotFound := 0
   nSeekFailed := 0
   ? time()
   FOR n := 1 TO s_RECORDS * s_TIMES
      // create a valid id
      nID := hb_RandomInt(101, s_RECORDS - 100)
      SEEK nID
      IF found() .AND. !bof() .AND. !eof() .AND. FIELD->ID == nId
         ++nSeekFound
      ELSE
         ++nSeekFailed
      ENDIF
      // create a invalid id
      SWITCH hb_RandomInt(1, 3)
      CASE 1; nID := hb_RandomInt(1, 100); EXIT // key is valid, but out of scope
      CASE 2; nID := hb_RandomInt(s_RECORDS - 100 + 1, s_RECORDS); EXIT // key is valid, but out of scope
      CASE 3; nID := hb_RandomInt(s_RECORDS + 1, s_RECORDS + s_RECORDS); EXIT // key is invalid
      ENDSWITCH
      SEEK nId
      IF !found() .AND. !bof() .AND. eof()
         ++nSeekNotFound
      ELSE
         ++nSeekFailed
      ENDIF
   NEXT n
   ? time()
   // must be s_RECORDS * s_TIMES
   ? "nSeekFound", nSeekFound, iif(nSeekFound == s_RECORDS * s_TIMES, "OK", "ERROR")
   // must be s_RECORDS * s_TIMES
   ? "nSeekNotFound", nSeekNotFound, iif(nSeekNotFound == s_RECORDS * s_TIMES, "OK", "ERROR")
   // must be 0
   ? "nSeekFailed", nSeekFailed, iif(nSeekFailed == 0, "OK", "ERROR")
   SET SCOPE TO

   //-------------------------------------------------------------------------//

   ? "Testing index 1 (ID) with filter"
   SET INDEX TO index1
   ? "ordkeycount (before filter)=", ordkeycount(), iif(ordkeycount() == s_RECORDS, "OK", "ERROR")
   SET FILTER TO ID/2==int(ID/2)
   ? "ordkeycount (after filter)=", ordkeycount(), iif(ordkeycount() == s_RECORDS, "OK", "ERROR")
   nSeekFound := 0
   nSeekNotFound := 0
   nSeekFailed := 0
   ? time()
   FOR n := 1 TO s_RECORDS * s_TIMES
      // create a valid id
      DO WHILE .T.
         nID := hb_RandomInt(1, s_RECORDS)
         IF nID / 2 == Int(nID / 2)
            EXIT
         ENDIF
      ENDDO
      SEEK nID
      IF found() .AND. !bof() .AND. !eof() .AND. FIELD->ID == nId
         ++nSeekFound
      ELSE
         ++nSeekFailed
      ENDIF
      // create a invalid id
      DO WHILE .T.
         nID := hb_RandomInt(1, s_RECORDS)
         IF nID/2 != int(nID/2)
            EXIT
         ENDIF
      ENDDO
      SEEK nId
      IF !found() .AND. !bof() .AND. eof()
         ++nSeekNotFound
      ELSE
         ++nSeekFailed
      ENDIF
   NEXT n
   ? time()
   // must be s_RECORDS * s_TIMES
   ? "nSeekFound", nSeekFound, iif(nSeekFound == s_RECORDS * s_TIMES, "OK", "ERROR")
   // must be s_RECORDS * s_TIMES
   ? "nSeekNotFound", nSeekNotFound, iif(nSeekNotFound == s_RECORDS * s_TIMES, "OK", "ERROR")
   // must be 0
   ? "nSeekFailed", nSeekFailed, iif(nSeekFailed == 0, "OK", "ERROR")
   SET SCOPE TO
   SET FILTER TO

   //-------------------------------------------------------------------------//

   ? "Testing index 2 (NAME)"
   SET INDEX TO index2
   ? "ordkeycount=", ordkeycount(), iif(ordkeycount() == s_RECORDS, "OK", "ERROR")
   nSeekFound := 0
   nSeekNotFound := 0
   nSeekFailed := 0
   ? time()
   FOR n := 1 TO s_RECORDS * s_TIMES
      // create a valid name
      cName := strzero(hb_RandomInt(1, s_RECORDS), 10)
      SEEK cName
      IF found() .AND. !bof() .AND. !eof() .AND. FIELD->NAME == cName
         ++nSeekFound
      ELSE
         ++nSeekFailed
      ENDIF
      // create a invalid name
      cName := strzero(hb_RandomInt(s_RECORDS + 1, s_RECORDS + s_RECORDS), 10)
      SEEK cName
      IF !found() .AND. !bof() .AND. eof()
         ++nSeekNotFound
      ELSE
         ++nSeekFailed
      ENDIF
   NEXT n
   ? time()
   // must be s_RECORDS * s_TIMES
   ? "nSeekFound", nSeekFound, iif(nSeekFound == s_RECORDS * s_TIMES, "OK", "ERROR")
   // must be s_RECORDS * s_TIMES
   ? "nSeekNotFound", nSeekNotFound, iif(nSeekNotFound == s_RECORDS * s_TIMES, "OK", "ERROR")
   // must be 0
   ? "nSeekFailed", nSeekFailed, iif(nSeekFailed == 0, "OK", "ERROR")

   //-------------------------------------------------------------------------//

   ? "Testing index 3 (DATE)"
   SET INDEX TO index3
   ? "ordkeycount=", ordkeycount(), iif(ordkeycount() == s_RECORDS, "OK", "ERROR")
   nSeekFound := 0
   nSeekNotFound := 0
   nSeekFailed := 0
   ? time()
   FOR n := 1 TO s_RECORDS * s_TIMES
      // create a valid date
      dDate := date() - hb_RandomInt(1, s_RECORDS)
      SEEK dDate
      IF found() .AND. !bof() .AND. !eof() .AND. FIELD->DATE == dDate
         ++nSeekFound
      ELSE
         ++nSeekFailed
      ENDIF
      // create a invalid date
      dDate := date() + hb_RandomInt(1, s_RECORDS)
      SEEK dDate
      IF !found() .AND. !bof() .AND. eof()
         ++nSeekNotFound
      ELSE
         ++nSeekFailed
      ENDIF
   NEXT n
   ? time()
   // must be s_RECORDS * s_TIMES
   ? "nSeekFound", nSeekFound, iif(nSeekFound == s_RECORDS * s_TIMES, "OK", "ERROR")
   // must be s_RECORDS * s_TIMES
   ? "nSeekNotFound", nSeekNotFound, iif(nSeekNotFound == s_RECORDS * s_TIMES, "OK", "ERROR")
   // must be 0
   ? "nSeekFailed", nSeekFailed, iif(nSeekFailed == 0, "OK", "ERROR")

   //-------------------------------------------------------------------------//

   ? "Testing index 4 (NAME+DTOS(DATE))"
   SET INDEX TO index4
   ? "ordkeycount=", ordkeycount(), iif(ordkeycount() == s_RECORDS, "OK", "ERROR")
   nSeekFound := 0
   nSeekNotFound := 0
   nSeekFailed := 0
   ? time()
   FOR n := 1 TO s_RECORDS * s_TIMES
      nRand := hb_RandomInt(1, s_RECORDS)
      // create a valid name
      cName := strzero(nRand, 10)
      // create a valid date
      dDate := date() - nRand
      SEEK cName + dtos(dDate)
      IF found() .AND. !bof() .AND. !eof() .AND. FIELD->NAME == cName .AND. FIELD->DATE == dDate
         ++nSeekFound
      ELSE
         ++nSeekFailed
      ENDIF
      // create a invalid name
      cName := strzero(hb_RandomInt(s_RECORDS + 1, s_RECORDS + s_RECORDS), 10)
      // create a invalid date
      dDate := date() + hb_RandomInt(1, s_RECORDS)
      SEEK cName + dtos(dDate)
      IF !found() .AND. !bof() .AND. eof()
         ++nSeekNotFound
      ELSE
         ++nSeekFailed
      ENDIF
   NEXT n
   ? time()
   // must be s_RECORDS * s_TIMES
   ? "nSeekFound", nSeekFound, iif(nSeekFound == s_RECORDS * s_TIMES, "OK", "ERROR")
   // must be s_RECORDS * s_TIMES
   ? "nSeekNotFound", nSeekNotFound, iif(nSeekNotFound == s_RECORDS * s_TIMES, "OK", "ERROR")
   // must be 0
   ? "nSeekFailed", nSeekFailed, iif(nSeekFailed == 0, "OK", "ERROR")

   //-------------------------------------------------------------------------//

   IF s_DROP_TABLE
      ? "Removing indexes"
      sr_DropIndex("index1")
      sr_DropIndex("index2")
      sr_DropIndex("index3")
      sr_DropIndex("index4")
   ENDIF

   ? "Closing table"
   CLOSE DATABASE

   IF s_DROP_TABLE .AND. sr_ExistTable(s_TABLE_NAME)
      ? "Removing table"
      sr_DropTable(s_TABLE_NAME)
   ENDIF

   sr_StopLog(nConnection)

   sr_EndConnection(nConnection)

   WAIT

RETURN
