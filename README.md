# SQLRDD++ v2

SQLRDD++ for Harbour++

## Requisites

Harbour++  
C++ compiler  
C++11 or upper  

## Notes

### Windows - How to compile
```Batch
cd sqlrddpp-v2
hbmk2 sqlrddpp-v2.hbp
```

### Ubuntu - How to get and compile
```Batch
sudo apt install unixodbc-dev
git clone https://github.com/marcosgambeta/sqlrddpp-v2
cd sqlrddpp-v2
hbmk2 sqlrddpp-v2.hbp
```

### OpenSuse - How to get and compile
```Batch
sudo zypper install unixODBC-devel
git clone https://github.com/marcosgambeta/sqlrddpp-v2
cd sqlrddpp-v2
hbmk2 sqlrddpp-v2.hbp
```

### Compiling with SQLRDD++ and MySQL
```Batch
hbmk2 <filename> sqlrddpp-v2.hbc -llibmysql
```

### Compiling with SQLRDD++ and PostgreSQL
```Batch
hbmk2 <filename> sqlrddpp-v2.hbc -llibpq
```

### Compiling with SQLRDD++ and Firebird
```Batch
hbmk2 <filename> sqlrddpp-v2.hbc -lfbclient
```

## Links

SQLRDD for xHarbour and Harbour  
https://github.com/xHarbour-org/xharbour  
https://github.com/xHarbour-org/xharbour/tree/main/xHarbourBuilder/xHarbour-SQLRDD  

Bison  
https://gnuwin32.sourceforge.net/packages/bison.htm  

MySQL  
https://www.mysql.com  

MariaDB  
https://mariadb.org  

PostgreSQL  
https://www.postgresql.org  

Firebird  
https://firebirdsql.org  

Harbour++  
https://github.com/marcosgambeta/harbourpp-v1
