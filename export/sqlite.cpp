#define UTIL_CORE
#include "sqlite3.h"
#include "sqlite.h"
#include <set>
#include <vector>
#include <string>
#include <cstring>
#include <cctype>

namespace external
{
	class SQLiteHandle
	{
	public:
		sqlite3 *sqlite;
		std::set<SQLiteStmt *> stmts;

	public:
		SQLiteHandle();
		~SQLiteHandle();
	};

	class SQLiteColumn
	{
	public:
		std::vector<std::string> names;
	};

	class SQLiteRecord
	{
	public:
		virtual int Type(int N) = 0;
		virtual bool IsNull(int N) = 0;
		virtual bool IsInteger(int N) = 0;
		virtual bool IsFloat(int N) = 0;
		virtual bool IsText(int N) = 0;
		virtual bool IsBlob(int N) = 0;
		virtual i64 ToInteger(int N) { return 0; }
		virtual double ToFloat(int N) { return 0.0; }
		virtual const char * ToText(int N, unsigned int *len) { return NULL; }
		virtual const void * ToBlob(int N, unsigned int *len) { return NULL; }
		virtual sqlite3_value * ToValue(int N) { return NULL; }
	};

	class SQLiteRecordReal : public SQLiteRecord
	{
	public:
		SQLiteStmt *stmt;

	public:
		virtual int Type(int N);
		virtual bool IsNull(int N);
		virtual bool IsInteger(int N);
		virtual bool IsFloat(int N);
		virtual bool IsText(int N);
		virtual bool IsBlob(int N);
		virtual i64 ToInteger(int N);
		virtual double ToFloat(int N);
		virtual const char * ToText(int N, unsigned int *len);
		virtual const void * ToBlob(int N, unsigned int *len);
		virtual sqlite3_value * ToValue(int N);
	};

	class SQLiteStmt
	{
	public:
		SQLiteHandle *root;
		bool isinit;
		bool iseof;
		sqlite3_stmt *stmt;
		SQLiteColumn column;
		SQLiteRecordReal record;
		std::string nextsql;

	public:
		SQLiteStmt(SQLiteHandle *handle);
		~SQLiteStmt();
	};

	SQLiteHandle::SQLiteHandle()
	{
		sqlite = NULL;
	}

	SQLiteHandle::~SQLiteHandle()
	{
		if (sqlite != NULL)
		{
			for (std::set<SQLiteStmt *>::iterator i = stmts.begin(); i != stmts.end(); ++i)
			{
				sqlite3_finalize((*i)->stmt);
				(*i)->stmt = NULL;
				(*i)->root = NULL;
			}
			sqlite3_close(sqlite);
		}
	}

	int SQLiteRecordReal::Type(int N)
	{
		if (stmt->stmt == NULL)
		{
			return SQL_NULL;
		}
		switch (sqlite3_column_type(stmt->stmt, N))
		{
		case SQLITE_INTEGER:
			return SQL_INTEGER;
		case SQLITE_FLOAT:
			return SQL_FLOAT;
		case SQLITE_TEXT:
			return SQL_TEXT;
		case SQLITE_BLOB:
			return SQL_BLOB;
		default:
			return SQL_NULL;
		}
	}

	bool SQLiteRecordReal::IsNull(int N)
	{
		return Type(N) == SQL_NULL;
	}

	bool SQLiteRecordReal::IsInteger(int N)
	{
		return Type(N) == SQL_INTEGER;
	}

	bool SQLiteRecordReal::IsFloat(int N)
	{
		return Type(N) == SQL_FLOAT;
	}

	bool SQLiteRecordReal::IsText(int N)
	{
		return Type(N) == SQL_TEXT;
	}

	bool SQLiteRecordReal::IsBlob(int N)
	{
		return Type(N) == SQL_BLOB;
	}

	i64 SQLiteRecordReal::ToInteger(int N)
	{
		return sqlite3_column_int64(stmt->stmt, N);
	}

	double SQLiteRecordReal::ToFloat(int N)
	{
		return sqlite3_column_double(stmt->stmt, N);
	}

	const char * SQLiteRecordReal::ToText(int N, unsigned int *len)
	{
		const char *str = (const char *)sqlite3_column_text(stmt->stmt, N);
		if (str == NULL)
			return NULL;
		if (len != NULL)
			*len = (unsigned int)strlen(str);
		return str;
	}

	const void * SQLiteRecordReal::ToBlob(int N, unsigned int *len)
	{
		const void *blob = sqlite3_column_blob(stmt->stmt, N);
		if (blob == NULL)
			return NULL;
		if (len != NULL)
			*len = sqlite3_column_bytes(stmt->stmt, N);
		return blob;
	}

	sqlite3_value * SQLiteRecordReal::ToValue(int N)
	{
		return sqlite3_column_value(stmt->stmt, N);
	}

	SQLiteStmt::SQLiteStmt(SQLiteHandle *handle)
	{
		root = handle;
		isinit = false;
		iseof = false;
		stmt = NULL;
		record.stmt = this;
		root->stmts.insert(this);
	}

	SQLiteStmt::~SQLiteStmt()
	{
		if (root != NULL)
		{
			std::set<SQLiteStmt *>::iterator i = root->stmts.find(this);
			if (i != root->stmts.end())
			{
				root->stmts.erase(i);
			}
			sqlite3_finalize(stmt);
		}
	}
}

void ext_sqlite_close(SQLiteHandle *handle)
{
	delete handle;
}

void ext_sqlite_finalize(SQLiteStmt *stmt)
{
	delete stmt;
}

SQLiteHandle * ext_sqlite_open(const char *path, int readonly)
{
	sqlite3 *sqlite = NULL;
	if (SQLITE_OK != sqlite3_open_v2(path, &sqlite, readonly ? SQLITE_OPEN_READONLY : SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, 0))
	{
		if (sqlite != NULL)
		{
			sqlite3_close(sqlite);
		}
		return NULL;
	}
	SQLiteHandle *handle = new SQLiteHandle();
	handle->sqlite = sqlite;
	return handle;
}

SQLiteHandle * ext_sqlite_handle(SQLiteStmt *stmt)
{
	return stmt->root;
}

const char * ext_sqlite_errmsg(SQLiteHandle *handle, unsigned int *len)
{
	const char *result = sqlite3_errmsg(handle->sqlite);
	if (len != NULL)
	{
		*len = (unsigned int)strlen(result);
	}
	return result;
}

namespace
{
	class SQLiteRecordText : public SQLiteRecord
	{
	public:
		std::vector<std::string>	values;
		std::vector<bool>			nulls;

	public:
		virtual int Type(int N)
		{
			if (N < 0 || N >= (int)nulls.size())
				return SQL_NULL;
			return nulls[N] ? SQL_NULL : SQL_TEXT;
		}
		virtual bool IsNull(int N)
		{
			return Type(N) == SQL_NULL;
		}
		virtual bool IsInteger(int N)
		{
			return false;
		}
		virtual bool IsFloat(int N)
		{
			return false;
		}
		virtual bool IsText(int N)
		{
			return !IsNull(N);
		}
		virtual bool IsBlob(int N)
		{
			return false;
		}
		virtual const char * ToText(int N, unsigned int *len)
		{
			if (IsNull(N))
				return NULL;
			std::string &str = values[N];
			if (len != NULL)
				*len = (unsigned int)str.length();
			return str.c_str();
		}
	};
	struct sqlite_transfer_data
	{
		int (*transfer)(void*, SQLiteColumn *, SQLiteRecord *);
		void *ud;
		bool first;
		SQLiteColumn column;
		SQLiteRecordText record;
	};
	static int sqlite_custom_callback(void *pArg, int count, char **values, char **columns)
	{
		sqlite_transfer_data *data = (sqlite_transfer_data *)pArg;
		if (data->transfer == NULL)
			return 1;
		if (data->first)
		{
			data->first = false;
			std::vector<std::string> &names = data->column.names;
			names.resize(count);
			for (int i = 0; i < count; ++i)
			{
				names[i] = columns[i];
			}
		}
		data->record.values.resize(count);
		data->record.nulls.resize(count);
		for (int i = 0; i < count; ++i)
		{
			if (values[i] == NULL)
			{
				data->record.nulls[i] = true;
				data->record.values[i].clear();
			}
			else
			{
				data->record.nulls[i] = false;
				data->record.values[i] = values[i];
			}
		}
		return data->transfer(data->ud, &(data->column), &(data->record)) == 0 ? 1 : 0;
	}
}

int ext_sqlite_exec(SQLiteHandle *handle, const char *sql, int (*callback)(void*, SQLiteColumn *, SQLiteRecord *), void *ud)
{
	sqlite_transfer_data data;
	data.transfer = callback;
	data.ud = ud;
	data.first = true;
	return SQLITE_OK == sqlite3_exec(handle->sqlite, sql, sqlite_custom_callback, &data, NULL);
}

SQLiteStmt * ext_sqlite_prepare(SQLiteHandle *handle, const char *sql)
{
	if (sql == NULL) sql = "";
	sqlite3_stmt *sqlite_stmt = NULL;
	const char *left_over = NULL;
	while (*sql)
	{
		while (isspace(*sql)) ++sql;
		int result = sqlite3_prepare(handle->sqlite, sql, -1, &sqlite_stmt, &left_over);
		if (result != SQLITE_OK)
			return NULL;
		if (sqlite_stmt != NULL)
			break;
		sql = left_over;
	}
	if (sqlite_stmt == NULL)
	{
		return NULL;
	}
	SQLiteStmt *stmt = new SQLiteStmt(handle);
	stmt->stmt = sqlite_stmt;
	if (left_over != NULL)
	{
		stmt->nextsql = left_over;
	}
	return stmt;
}

int ext_sqlite_valid(SQLiteStmt *stmt)
{
	return stmt->stmt == NULL ? 0 : 1;
}

SQLiteColumn * ext_sqlite_column(SQLiteStmt *stmt)
{
	if (!stmt->isinit)
	{
		stmt->isinit = true;
		if (stmt->stmt != NULL)
		{
			int nCol = sqlite3_column_count(stmt->stmt);
			std::vector<std::string> &names = stmt->column.names;
			names.resize(nCol);
			for (int i = 0; i < nCol; ++i)
			{
				names[i] = sqlite3_column_name(stmt->stmt, i);
			}
		}
	}
	return &(stmt->column);
}

SQLiteRecord * ext_sqlite_step(SQLiteStmt *stmt)
{
	if (stmt->stmt == NULL)
	{
		return NULL;
	}
	if (stmt->iseof)
	{
		return NULL;
	}
	int result = sqlite3_step(stmt->stmt);
	if (result == SQLITE_ROW)
	{
		return &(stmt->record);
	}
	if (result == SQLITE_OK || result == SQLITE_DONE)
	{
		stmt->iseof = true;
	}
	return NULL;
}

int ext_sqlite_eof(SQLiteStmt *stmt)
{
	return stmt->iseof ? 1 : 0;
}

SQLiteStmt * ext_sqlite_nextstmt(SQLiteStmt *stmt)
{
	if (stmt->nextsql.empty())
	{
		return NULL;
	}
	return ext_sqlite_prepare(stmt->root, stmt->nextsql.c_str());
}

int ext_sqlite_reset(SQLiteStmt *stmt)
{
	if (SQLITE_OK == sqlite3_reset(stmt->stmt))
	{
		stmt->isinit = false;
		stmt->iseof = false;
		return 1;
	}
	return 0;
}

int ext_sqlite_clear_binds(SQLiteStmt *stmt)
{
	if (stmt->stmt == NULL)
	{
		return 1;
	}
	return SQLITE_OK == sqlite3_clear_bindings(stmt->stmt);
}

int ext_sqlite_column_count(SQLiteColumn *column)
{
	return (int)column->names.size();
}

const char * ext_sqlite_column_name(SQLiteColumn *column, int N, unsigned int *len)
{
	if (N < 0 || N >= (int)column->names.size())
	{
		return NULL;
	}
	std::string &str = column->names[N];
	if (len != NULL)
		*len = (unsigned int)str.length();
	return str.c_str();
}

int ext_sqlite_bind_null(SQLiteStmt *stmt, int N)
{
	return SQLITE_OK == sqlite3_bind_null(stmt->stmt, N + 1) ? 1 : 0;
}

int ext_sqlite_bind_int(SQLiteStmt *stmt, int N, int i)
{
	return SQLITE_OK == sqlite3_bind_int(stmt->stmt, N + 1, i) ? 1 : 0;
}

int ext_sqlite_bind_long(SQLiteStmt *stmt, int N, i64 l)
{
	return SQLITE_OK == sqlite3_bind_int64(stmt->stmt, N + 1, l) ? 1 : 0;
}

int ext_sqlite_bind_double(SQLiteStmt *stmt, int N, double d)
{
	return SQLITE_OK == sqlite3_bind_double(stmt->stmt, N + 1, d) ? 1 : 0;
}

int ext_sqlite_bind_string(SQLiteStmt *stmt, int N, const char *str, unsigned int len)
{
	return SQLITE_OK == sqlite3_bind_text(stmt->stmt, N + 1, str, (int)len, SQLITE_TRANSIENT) ? 1 : 0;
}

int ext_sqlite_bind_blob(SQLiteStmt *stmt, int N, const void *blob, unsigned int len)
{
	return SQLITE_OK == sqlite3_bind_blob(stmt->stmt, N + 1, blob, (int)len, SQLITE_TRANSIENT) ? 1 : 0;
}

int ext_sqlite_bind_value(SQLiteStmt *stmt, int N, const sqlite3_value *val)
{
	return SQLITE_OK == sqlite3_bind_value(stmt->stmt, N + 1, val) ? 1 : 0;
}

int ext_sqlite_record_type(SQLiteRecord *record, int N)
{
	return record->Type(N);
}

int ext_sqlite_record_isnull(SQLiteRecord *record, int N)
{
	return record->IsNull(N) ? 1 : 0;
}

int ext_sqlite_record_isinteget(SQLiteRecord *record, int N)
{
	return record->IsInteger(N) ? 1 : 0;
}

int ext_sqlite_record_isfloat(SQLiteRecord *record, int N)
{
	return record->IsFloat(N) ? 1 : 0;
}

int ext_sqlite_record_istext(SQLiteRecord *record, int N)
{
	return record->IsText(N) ? 1 : 0;
}

int ext_sqlite_record_isblob(SQLiteRecord *record, int N)
{
	return record->IsBlob(N) ? 1 : 0;
}

i64 ext_sqlite_record_tointeget(SQLiteRecord *record, int N)
{
	return record->ToInteger(N);
}

double ext_sqlite_record_tofloat(SQLiteRecord *record, int N)
{
	return record->ToFloat(N);
}

const char * ext_sqlite_record_totext(SQLiteRecord *record, int N, unsigned int *len)
{
	return record->ToText(N, len);
}

const void * ext_sqlite_record_toblob(SQLiteRecord *record, int N, unsigned int *len)
{
	return record->ToBlob(N, len);
}
