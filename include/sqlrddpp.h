// SQLRDD++ Project
// Copyright (c) 2025 Marcos Antonio Gambeta <marcosgambeta@outlook.com>

#ifndef SQLRDDPP_H
#define SQLRDDPP_H

// Define SR_NULLPTR:
#define SR_NULLPTR nullptr
#if 0
// If the compiler is a C++ compiler and the standard is C++11 or upper, define SR_NULLPTR as nullptr.
// If the compiler is a C compiler and the standard is C23 or upper, define SR_NULLPTR as nullptr.
// Otherwise, define SR_NULLPTR as '((void *)0)'.
#if defined(__cplusplus)
#if __cplusplus >= 201103L
#define SR_NULLPTR nullptr
#else
#define SR_NULLPTR ((void *)0)
#endif
#else
#ifdef __STDC_VERSION__
#if __STDC_VERSION__ >= 202311L
#define SR_NULLPTR nullptr
#else
#define SR_NULLPTR ((void *)0)
#endif
#else
#define SR_NULLPTR ((void *)0)
#endif
#endif
#endif

namespace SQLRDD {
  namespace RDBMS {
    // NOTE: needs to be kept in sync with the sqlrddpp.ch file
    enum ID : int {
      UNKNOW = 0,
      ORACLE = 1,
      MSSQL6 = 2,
      MSSQL7 = 3,
      SQLANY = 4,
      SYBASE = 5,
      ACCESS = 6,
      INGRES = 7,
      SQLBAS = 8,
      ADABAS = 9,
      INFORM = 10,
      IBMDB2 = 11,
      MYSQL = 12,
      POSTGR = 13,
      FIREBR = 14,
      CACHE = 15,
      OTERRO = 16,
      PERVASIVE = 17,
      AZURE = 18,
      MARIADB = 19,
      FIREBR3 = 20,
      FIREBR4 = 21,
      FIREBR5 = 22
    };
  }
}

#endif // SQLRDDPP_H
