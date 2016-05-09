#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"
#include "config.h"

namespace external
{
	class SQLiteHandle;
	class SQLiteStmt;
	class SQLiteColumn;
	class SQLiteRecord;

	enum
	{
		SQL_NULL = 0,
		SQL_INTEGER,
		SQL_FLOAT,
		SQL_TEXT,
		SQL_BLOB,
	};
}

using namespace external;

struct Mem;
typedef struct Mem sqlite3_value;

EXPORTS void			ext_sqlite_close(SQLiteHandle *handle);
EXPORTS void			ext_sqlite_finalize(SQLiteStmt *stmt);
EXPORTS SQLiteHandle *	ext_sqlite_open(const char *path, int readonly);
EXPORTS SQLiteHandle *	ext_sqlite_handle(SQLiteStmt *stmt);
EXPORTS const char *	ext_sqlite_errmsg(SQLiteHandle *handle, unsigned int *len);
EXPORTS int				ext_sqlite_exec(SQLiteHandle *handle, const char *sql, int (*callback)(void*, SQLiteColumn *, SQLiteRecord *), void *ud);
EXPORTS SQLiteStmt *	ext_sqlite_prepare(SQLiteHandle *handle, const char *sql);
EXPORTS int				ext_sqlite_valid(SQLiteStmt *stmt);
EXPORTS SQLiteColumn *	ext_sqlite_column(SQLiteStmt *stmt);
EXPORTS SQLiteRecord *	ext_sqlite_step(SQLiteStmt *stmt);
EXPORTS int				ext_sqlite_eof(SQLiteStmt *stmt);
EXPORTS SQLiteStmt *	ext_sqlite_nextstmt(SQLiteStmt *stmt);
EXPORTS int				ext_sqlite_reset(SQLiteStmt *stmt);
EXPORTS int				ext_sqlite_clear_binds(SQLiteStmt *stmt);
EXPORTS int				ext_sqlite_bind_null(SQLiteStmt *stmt, int N);
EXPORTS int				ext_sqlite_bind_int(SQLiteStmt *stmt, int N, int i);
EXPORTS int				ext_sqlite_bind_long(SQLiteStmt *stmt, int N, i64 l);
EXPORTS int				ext_sqlite_bind_double(SQLiteStmt *stmt, int N, double d);
EXPORTS int				ext_sqlite_bind_string(SQLiteStmt *stmt, int N, const char *str, unsigned int len);
EXPORTS int				ext_sqlite_bind_blob(SQLiteStmt *stmt, int N, const void *blob, unsigned int len);
EXPORTS int				ext_sqlite_bind_value(SQLiteStmt *stmt, int N, const sqlite3_value *val);
EXPORTS int				ext_sqlite_column_count(SQLiteColumn *column);
EXPORTS const char *	ext_sqlite_column_name(SQLiteColumn *column, int N, unsigned int *len);
EXPORTS int				ext_sqlite_record_type(SQLiteRecord *record, int N);
EXPORTS int				ext_sqlite_record_isnull(SQLiteRecord *record, int N);
EXPORTS int				ext_sqlite_record_isinteget(SQLiteRecord *record, int N);
EXPORTS int				ext_sqlite_record_isfloat(SQLiteRecord *record, int N);
EXPORTS int				ext_sqlite_record_istext(SQLiteRecord *record, int N);
EXPORTS int				ext_sqlite_record_isblob(SQLiteRecord *record, int N);
EXPORTS i64				ext_sqlite_record_tointeget(SQLiteRecord *record, int N);
EXPORTS double			ext_sqlite_record_tofloat(SQLiteRecord *record, int N);
EXPORTS const char *	ext_sqlite_record_totext(SQLiteRecord *record, int N, unsigned int *len);
EXPORTS const void *	ext_sqlite_record_toblob(SQLiteRecord *record, int N, unsigned int *len);
