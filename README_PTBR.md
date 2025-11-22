# SQLRDD++ v2

SQLRDD++ para Harbour++

## Requisitos

Harbour++  
Compilador C++  
Standard C++11 ou superior  

## SGBD suportados

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

## Compiladores C++ suportados

| Projeto   | Compilador C++   | 32/64-bit | Status                  | Parâmetros extras   |
| --------- | ---------------- | --------- | ----------------------- | ------------------- |
| Harbour++ | MinGW            | 32-bit    | Compilando com avisos   | ...                 |
| Harbour++ | MinGW            | 64-bit    | Compilando com avisos   | ...                 |
| Harbour++ | MSVC             | 32-bit    | Compilando com avisos   | ...                 |
| Harbour++ | MSVC             | 64-bit    | Compilando com avisos   | ...                 |
| Harbour++ | Clang            | 32-bit    | Compilando com avisos   | ...                 |
| Harbour++ | Clang            | 64-bit    | Compilando com avisos   | ...                 |
| Harbour++ | BCC 7.3          | 32-bit    | ...                     | ...                 |
| Harbour++ | BCC 7.3          | 64-bit    | ...                     | ...                 |

## Compilando o projeto

### Windows - Como obter o código-fonte e compilar
```Batch
git clone https://github.com/marcosgambeta/sqlrddpp-v2
cd sqlrddpp-v2
hbmk2 sqlrddpp-v2.hbp
```

### Ubuntu - Como obter o código-fonte e compilar
```Batch
sudo apt install unixodbc-dev
git clone https://github.com/marcosgambeta/sqlrddpp-v2
cd sqlrddpp-v2
hbmk2 sqlrddpp-v2.hbp
```

### OpenSuse - Como obter o código-fonte e compilar
```Batch
sudo zypper install unixODBC-devel
git clone https://github.com/marcosgambeta/sqlrddpp-v2
cd sqlrddpp-v2
hbmk2 sqlrddpp-v2.hbp
```

## Usando o projeto

### Compilando com SQLRDD++ e MySQL
```Batch
hbmk2 <filename> sqlrddpp-v2.hbc -llibmysql
```

### Compilando com SQLRDD++ e PostgreSQL
```Batch
hbmk2 <filename> sqlrddpp-v2.hbc -llibpq
```

### Compilando com SQLRDD++ e Firebird
```Batch
hbmk2 <filename> sqlrddpp-v2.hbc -lfbclient
```

## Problemas na utilização

Caso tenha problemas na utilização deste projeto, informe na seção 'Issues':

https://github.com/marcosgambeta/sqlrddpp-v2/issues  

## Contato com o desenvolvedor

Envie sua mensagem usando uma das opções abaixo:

E-mail:  
marcosgambeta@outlook.com

Telegram:  
https://t.me/marcosgambeta

## Informações e conteúdo extra

Acompanhe o blog abaixo para mais informações e conteúdo extra:

https://magsoftinfo.com/blog/

## Links

SQLRDD para xHarbour e Harbour  
https://github.com/xHarbour-org/xharbour  
https://github.com/xHarbour-org/xharbour/tree/main/xHarbourBuilder/xHarbour-SQLRDD  

SQLRDD++ v1  
https://github.com/marcosgambeta/sqlrddpp  
Versão em C para Harbour e Harbour++

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
