#include "sqliteInt.h"
#include "btreeInt.h"
#include "fts3Int.h"
#include "vdbeInt.h"
/*
** 2009 November 25
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
**
** This file contains code used to insert the values of host parameters
** (aka "wildcards") into the SQL text output by sqlite3_trace().
*/

#ifndef SQLITE_OMIT_TRACE

/*
** zSql is a zero-terminated string of UTF-8 SQL text.  Return the number of
** bytes in this text up to but excluding the first character in
** a host parameter.  If the text contains no host parameters, return
** the total number of bytes in the text.
*/
static int findNextHostParameter(const char *zSql, int *pnToken){
  int tokenType;
  int nTotal = 0;
  int n;

  *pnToken = 0;
  while( zSql[0] ){
    n = sqlite3GetToken((u8*)zSql, &tokenType);
    assert( n>0 && tokenType!=TK_ILLEGAL );
    if( tokenType==TK_VARIABLE ){
      *pnToken = n;
      break;
    }
    nTotal += n;
    zSql += n;
  }
  return nTotal;
}

/*
** Return a pointer to a string in memory obtained form sqlite3DbMalloc() which
** holds a copy of zRawSql but with host parameters expanded to their
** current bindings.
**
** The calling function is responsible for making sure the memory returned
** is eventually freed.
**
** ALGORITHM:  Scan the input string looking for host parameters in any of
** these forms:  ?, ?N, $A, @A, :A.  Take care to avoid text within
** string literals, quoted identifier names, and comments.  For text forms,
** the host parameter index is found by scanning the perpared
** statement for the corresponding OP_Variable opcode.  Once the host
** parameter index is known, locate the value in p->aVar[].  Then render
** the value as a literal in place of the host parameter name.
*/
SQLITE_PRIVATE char *sqlite3VdbeExpandSql(
  Vdbe *p,                 /* The prepared statement being evaluated */
  const char *zRawSql      /* Raw text of the SQL statement */
){
  sqlite3 *db;             /* The database connection */
  int idx = 0;             /* Index of a host parameter */
  int nextIndex = 1;       /* Index of next ? host parameter */
  int n;                   /* Length of a token prefix */
  int nToken;              /* Length of the parameter token */
  int i;                   /* Loop counter */
  Mem *pVar;               /* Value of a host parameter */
  StrAccum out;            /* Accumulate the output here */
  char zBase[100];         /* Initial working space */

  db = p->db;
  sqlite3StrAccumInit(&out, zBase, sizeof(zBase), 
                      db->aLimit[SQLITE_LIMIT_LENGTH]);
  out.db = db;
  while( zRawSql[0] ){
    n = findNextHostParameter(zRawSql, &nToken);
    assert( n>0 );
    sqlite3StrAccumAppend(&out, zRawSql, n);
    zRawSql += n;
    assert( zRawSql[0] || nToken==0 );
    if( nToken==0 ) break;
    if( zRawSql[0]=='?' ){
      if( nToken>1 ){
        assert( sqlite3Isdigit(zRawSql[1]) );
        sqlite3GetInt32(&zRawSql[1], &idx);
      }else{
        idx = nextIndex;
      }
    }else{
      assert( zRawSql[0]==':' || zRawSql[0]=='$' || zRawSql[0]=='@' );
      testcase( zRawSql[0]==':' );
      testcase( zRawSql[0]=='$' );
      testcase( zRawSql[0]=='@' );
      idx = sqlite3VdbeParameterIndex(p, zRawSql, nToken);
      assert( idx>0 );
    }
    zRawSql += nToken;
    nextIndex = idx + 1;
    assert( idx>0 && idx<=p->nVar );
    pVar = &p->aVar[idx-1];
    if( pVar->flags & MEM_Null ){
      sqlite3StrAccumAppend(&out, "NULL", 4);
    }else if( pVar->flags & MEM_Int ){
      sqlite3XPrintf(&out, "%lld", pVar->u.i);
    }else if( pVar->flags & MEM_Real ){
      sqlite3XPrintf(&out, "%!.15g", pVar->r);
    }else if( pVar->flags & MEM_Str ){
#ifndef SQLITE_OMIT_UTF16
      u8 enc = ENC(db);
      if( enc!=SQLITE_UTF8 ){
        Mem utf8;
        memset(&utf8, 0, sizeof(utf8));
        utf8.db = db;
        sqlite3VdbeMemSetStr(&utf8, pVar->z, pVar->n, enc, SQLITE_STATIC);
        sqlite3VdbeChangeEncoding(&utf8, SQLITE_UTF8);
        sqlite3XPrintf(&out, "'%.*q'", utf8.n, utf8.z);
        sqlite3VdbeMemRelease(&utf8);
      }else
#endif
      {
        sqlite3XPrintf(&out, "'%.*q'", pVar->n, pVar->z);
      }
    }else if( pVar->flags & MEM_Zero ){
      sqlite3XPrintf(&out, "zeroblob(%d)", pVar->u.nZero);
    }else{
      assert( pVar->flags & MEM_Blob );
      sqlite3StrAccumAppend(&out, "x'", 2);
      for(i=0; i<pVar->n; i++){
        sqlite3XPrintf(&out, "%02x", pVar->z[i]&0xff);
      }
      sqlite3StrAccumAppend(&out, "'", 1);
    }
  }
  return sqlite3StrAccumFinish(&out);
}

#endif /* #ifndef SQLITE_OMIT_TRACE */