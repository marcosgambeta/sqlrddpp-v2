PROCEDURE Main()

   LOCAL a
   LOCAL n

   a := SR_ListODBCDataSources()

   ? "len(a)=", len(a)

   FOR n := 1 TO len(a)
      ? "DSN=", a[n, 1], "DRIVER=", a[n, 2]
   NEXT n

   WAIT

RETURN
