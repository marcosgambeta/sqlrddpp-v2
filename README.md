# SQLRDD++ v2

SQLRDD++ for Harbour, xHarbour, Harbour 3.4 and Harbour++.

## Requisites

C++ compiler with support for C++11 or upper

## Supported database systems

| SGBD               | Status         |
| ------------------ | -------------- |
| SYSTEMID_ACCESS    |                |
| SYSTEMID_ADABAS    |                |
| SYSTEMID_AZURE     |                |
| SYSTEMID_CACHE     |                |
| SYSTEMID_FIREBR    |                |
| SYSTEMID_FIREBR3   |                |
| SYSTEMID_FIREBR4   |                |
| SYSTEMID_FIREBR5   |                |
| SYSTEMID_IBMDB2    |                |
| SYSTEMID_INFORM    |                |
| SYSTEMID_INGRES    |                |
| SYSTEMID_MARIADB   |                |
| SYSTEMID_MSSQL6    |                |
| SYSTEMID_MSSQL7    |                |
| SYSTEMID_MYSQL     |                |
| SYSTEMID_ORACLE    |                |
| SYSTEMID_OTERRO    |                |
| SYSTEMID_PERVASIVE |                |
| SYSTEMID_POSTGR    |                |
| SYSTEMID_SQLANY    |                |
| SYSTEMID_SQLBAS    |                |
| SYSTEMID_SYBASE    |                |

## C++ Compilers Compatibility

| Project   | C++ compiler     | 32/64-bit | Status                  | Extra parameters    |
| --------- | ---------------- | --------- | ----------------------- | ------------------- |
| Harbour   | MinGW            | 32-bit    | Compiling with warnings | ...                 |
| Harbour   | MinGW            | 64-bit    | Compiling with warnings | ...                 |
| Harbour   | MSVC 2019        | 32-bit    | Compiling with warnings | ...                 |
| Harbour   | MSVC 2019        | 64-bit    | Compiling with warnings | ...                 |
| Harbour   | MSVC 2022        | 32-bit    | Compiling with warnings | ...                 |
| Harbour   | MSVC 2022        | 64-bit    | Compiling with warnings | ...                 |
| Harbour   | MSVC 2026        | 32-bit    | Compiling with warnings | ...                 |
| Harbour   | MSVC 2026        | 64-bit    | Compiling with warnings | ...                 |
| Harbour   | Clang            | 32-bit    | ?                       | ...                 |
| Harbour   | Clang            | 64-bit    | ?                       | ...                 |
| Harbour   | BCC 7.3          | 32-bit    | ?                       | ...                 |
| Harbour   | BCC 7.3          | 64-bit    | ?                       | ...                 |
| Harbour++ | MinGW            | 32-bit    | Compiling with warnings | ...                 |
| Harbour++ | MinGW            | 64-bit    | Compiling with warnings | ...                 |
| Harbour++ | MSVC             | 32-bit    | Compiling with warnings | ...                 |
| Harbour++ | MSVC             | 64-bit    | Compiling with warnings | ...                 |
| Harbour++ | Clang            | 32-bit    | Compiling with warnings | ...                 |
| Harbour++ | Clang            | 64-bit    | Compiling with warnings | ...                 |
| Harbour++ | BCC 7.3          | 32-bit    | ...                     | ...                 |
| Harbour++ | BCC 7.3          | 64-bit    | ...                     | ...                 |

## Building

### Windows - How to compile
```Batch
git clone https://github.com/marcosgambeta/sqlrddpp-v2
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

## Using

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

## Dedicated libraries

In addition to the main library, there are dedicated libraries for Firebird, MySQL/MariaDB,
Oracle, and PostgreSQL. Follow these steps to compile these libraries:

1. To compile all the libraries (main and dedicated):

```Batch
hbmk2 sqlrddpp-v2-alllibs.hbp
```

2. To compile the Firebird library:

```Batch
hbmk2 sqlrddpp-v2-firebird.hbp
```

3. To compile the MySQL/MariaDB library:

```Batch
hbmk2 sqlrddpp-v2-mysql.hbp
```

4. To compile the Oracle library:

```Batch
hbmk2 sqlrddpp-v2-oracle.hbp
```

5. To compile the PostgreSQL library:

```Batch
hbmk2 sqlrddpp-v2-postgresql.hbp
```

6. Use the equivalent .hbc file to compile your programs:

```Batch
hbmk2 <filename> sqlrddpp-v2-firebird.hbc ...
hbmk2 <filename> sqlrddpp-v2-mysql.hbc ...
hbmk2 <filename> sqlrddpp-v2-oracle.hbc ...
hbmk2 <filename> sqlrddpp-v2-postgresql.hbc ...
```

The dedicated libraries have the same behaviour of the main library, but the code
is reduced and do not need to check which RDBMS is being used all the time.

## Links

SQLRDD for xHarbour and Harbour  
https://github.com/xHarbour-org/xharbour  
https://github.com/xHarbour-org/xharbour/tree/main/xHarbourBuilder/xHarbour-SQLRDD  

SQLRDD++ v1  
https://github.com/marcosgambeta/sqlrddpp  
C version for Harbour and Harbour++

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
