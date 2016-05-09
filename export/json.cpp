#define UTIL_CORE
#include <cmath>
#include <vector>
#include "rapidjson/document.h"
#include "rapidjson/memorystream.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/error/en.h"
#include "json.h"

using namespace rapidjson;

namespace external
{
	class JSONWriter
	{
	protected:
		StringBuffer buffer;

	public:
		virtual ~JSONWriter() {}
		virtual size_t length()
		{
			return buffer.GetSize();
		}
		virtual size_t flush(char *output, size_t len)
		{
			size_t size = buffer.GetSize();
			const char *str = buffer.GetString();
			if (size < len)
			{
				memcpy(output, str, size);
				return size;
			}
			else
			{
				memcpy(output, str, len);
				return len;
			}
		}
		virtual void reset() = 0;
		virtual bool write_null() = 0;
		virtual bool write_bool(bool b) = 0;
		virtual bool write_int(int i) = 0;
		virtual bool write_uint(unsigned int u) = 0;
		virtual bool write_double(double d) = 0;
		virtual bool write_string(const char *str, size_t len) = 0;
		virtual bool write_newobject() = 0;
		virtual bool write_key(const char *str, size_t len) = 0;
		virtual bool write_closeobject(size_t count) = 0;
		virtual bool write_newarray() = 0;
		virtual bool write_closearray(size_t count) = 0;
	};
}

namespace
{
	struct ExternalHandles
	{
		int (*Null)(void *ud);
		int (*Bool)(void *ud, int b);
		int (*Int)(void *ud, int i);
		int (*Uint)(void *ud, unsigned int u);
		int (*Double)(void *ud, double d);
		int (*String)(void *ud, const char* str, unsigned int length);
		int (*StartObject)(void *ud);
		int (*Key)(void *ud, const char* str, unsigned int length);
		int (*EndObject)(void *ud, unsigned int memberCount);
		int (*StartArray)(void *ud);
		int (*EndArray)(void *ud, unsigned int elementCount);
	};

	class EmptyHandle
	{
	public:
		bool Null()
		{
			return true;
		}
		bool Bool(bool b)
		{
			return true;
		}
		bool Int(int i)
		{
			return true;
		}
		bool Uint(unsigned u)
		{
			return true;
		}
		bool Double(double d)
		{
			return true;
		}
		bool String(const char* str, SizeType length, bool copy)
		{ 
			return true;
		}
		bool StartObject()
		{
			return true;
		}
		bool Key(const char* str, SizeType length, bool copy)
		{
			return true;
		}
		bool EndObject(SizeType memberCount)
		{
			return true;
		}
		bool StartArray()
		{
			return true;
		}
		bool EndArray(SizeType elementCount)
		{
			return true;
		}
	};

	class ExternalHandle
	{
	protected:
		void *_ud;
		const ExternalHandles &_handles;
	public:
		ExternalHandle(void *ud, const ExternalHandles &handles) : _ud(ud), _handles(handles) {}
		bool Null()
		{
			return _handles.Null(_ud) != 0;
		}
		bool Bool(bool b)
		{
			return _handles.Bool(_ud, b ? 1 : 0) != 0;
		}
		bool Int(int i)
		{
			return _handles.Int(_ud, i) != 0;
		}
		bool Uint(unsigned u)
		{
			return _handles.Uint(_ud, u) != 0;
		}
		bool Double(double d)
		{
			return _handles.Double(_ud, d) != 0;
		}
		bool String(const char* str, SizeType length, bool copy)
		{ 
			return _handles.String(_ud, str, (unsigned int)length) != 0;
		}
		bool StartObject()
		{
			return _handles.StartObject(_ud) != 0;
		}
		bool Key(const char* str, SizeType length, bool copy)
		{
			return _handles.Key(_ud, str, (unsigned int)length) != 0;
		}
		bool EndObject(SizeType memberCount)
		{
			return _handles.EndObject(_ud, (unsigned int)memberCount) != 0;
		}
		bool StartArray()
		{
			return _handles.StartArray(_ud) != 0;
		}
		bool EndArray(SizeType elementCount)
		{
			return _handles.EndArray(_ud, (unsigned int)elementCount) != 0;
		}
	};

	struct EscapeUTF8 : public UTF8<>
	{
		enum { supportUnicode = 0 };
	};

	template <class Writer>
	class JSONWriterImpl : public JSONWriter
	{
	protected:
		Writer writer;

	public:
		JSONWriterImpl() : writer(buffer)
		{
		}
		virtual void reset()
		{
			buffer.Clear();
			writer.Reset(buffer);
		}
		virtual bool write_null()
		{
			return writer.Null();
		}
		virtual bool write_bool(bool b)
		{
			return writer.Bool(b);
		}
		virtual bool write_int(int i)
		{
			return writer.Int(i);
		}
		virtual bool write_uint(unsigned int u)
		{
			return writer.Uint(u);
		}
		virtual bool write_double(double d)
		{
			return writer.Double(d);
		}
		virtual bool write_string(const char *str, size_t len)
		{
			return writer.String(str, len, true);
		}
		virtual bool write_newobject()
		{
			return writer.StartObject();
		}
		virtual bool write_key(const char *str, size_t len)
		{
			return writer.Key(str, len, true);
		}
		virtual bool write_closeobject(size_t count)
		{
			return writer.EndObject(count);
		}
		virtual bool write_newarray()
		{
			return writer.StartArray();
		}
		virtual bool write_closearray(size_t count)
		{
			return writer.EndArray(count);
		}
	};
}

int ext_json_parse(const char *content, unsigned int len, void *ud, int (*Null)(void *), int (*Bool)(void *, int), int (*Int)(void *, int), int (*Uint)(void *, unsigned int), int (*Double)(void *, double), int (*String)(void *, const char *, unsigned int), int (*StartObject)(void *), int (*Key)(void *, const char *, unsigned int), int (*EndObject)(void *, unsigned int), int (*StartArray)(void *), int (*EndArray)(void *, unsigned int), void (*Error)(void *, int, int, const char *, unsigned int))
{
	ExternalHandles handles = {
		Null, Bool, Int, Uint, Double, String, StartObject, Key, EndObject, StartArray, EndArray
	};

	MemoryStream mstream(content, len);
	Reader reader;
    ExternalHandle handle = ExternalHandle(ud, handles);
	reader.Parse(mstream, handle);
	if (reader.HasParseError())
	{
		int row = 1, column = 1;
		for (size_t i = 0, j = reader.GetErrorOffset(); i <= j; ++i)
		{
			if (content[i] == '\n')
			{
				++row;
				column= 1;
			}
			else
			{
				++column;
			}
		}
		const char *message = GetParseError_En(reader.GetParseErrorCode());
		Error(ud, row, column, message, (unsigned int)strlen(message));
		return 0;
	}
	return 1;
}

EXPORTS JSONWriter * ext_json_writer(int pretty, int escape)
{
	if (pretty)
	{
		if (escape)
		{
			return new JSONWriterImpl< PrettyWriter<StringBuffer, UTF8<>, EscapeUTF8> >;
		}
		return new JSONWriterImpl< PrettyWriter<StringBuffer> >;
	}
	if (escape)
	{
		return new JSONWriterImpl< Writer<StringBuffer, UTF8<>, EscapeUTF8> >;
	}
	return new JSONWriterImpl< Writer<StringBuffer> >;
}

EXPORTS void ext_json_writer_close(JSONWriter *writer)
{
	delete writer;
}

EXPORTS void ext_json_writer_reset(JSONWriter *writer)
{
	writer->reset();
}

EXPORTS unsigned int ext_json_writer_length(JSONWriter *writer)
{
	return (unsigned int)writer->length();
}

EXPORTS unsigned int ext_json_writer_flush(JSONWriter *writer, char *output, unsigned int len)
{
	return (unsigned int)writer->flush(output, (size_t)len);
}

EXPORTS int ext_json_write_null(JSONWriter *writer)
{
	return writer->write_null() != 0;
}

EXPORTS int ext_json_write_bool(JSONWriter *writer, int b)
{
	return writer->write_bool(b != 0) != 0;
}

EXPORTS int ext_json_write_int(JSONWriter *writer, int i)
{
	return writer->write_int(i) != 0;
}

EXPORTS int ext_json_write_uint(JSONWriter *writer, unsigned int u)
{
	return writer->write_uint(u) != 0;
}

EXPORTS int ext_json_write_double(JSONWriter *writer, double d)
{
	return writer->write_double(d) != 0;
}

EXPORTS int ext_json_write_string(JSONWriter *writer, const char *str, unsigned int len)
{
	return writer->write_string(str, (size_t)len) != 0;
}

EXPORTS int ext_json_write_newobject(JSONWriter *writer)
{
	return writer->write_newobject() != 0;
}

EXPORTS int ext_json_write_key(JSONWriter *writer, const char *str, unsigned int len)
{
	return writer->write_key(str, (size_t)len) != 0;
}

EXPORTS int ext_json_write_closeobject(JSONWriter *writer, unsigned int count)
{
	return writer->write_closeobject((size_t)count) != 0;
}

EXPORTS int ext_json_write_newarray(JSONWriter *writer)
{
	return writer->write_newarray() != 0;
}

EXPORTS int ext_json_write_closearray(JSONWriter *writer, unsigned int count)
{
	return writer->write_closearray((size_t)count) != 0;
}
