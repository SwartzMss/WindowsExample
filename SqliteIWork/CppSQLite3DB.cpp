#include "CppSQLite3DB.h"

// Named constant for passing to CppSQLite3Exception when passing it a string
// that cannot be deleted.
static const bool DONT_DELETE_MSG = false;

////////////////////////////////////////////////////////////////////////////////
// Prototypes for SQLite functions not included in SQLite DLL, but copied below
// from SQLite encode.c
////////////////////////////////////////////////////////////////////////////////
int sqlite3_encode_binary(const unsigned char *in, int n, unsigned char *out);
int sqlite3_decode_binary(const unsigned char *in, unsigned char *out);

CppSQLite3Exception::CppSQLite3Exception(const int nErrCode, char* szErrMess, bool bDeleteMsg /*= true*/)
{
	mpszErrMess = sqlite3_mprintf("%s[%d]: %s",
		errorCodeAsString(nErrCode),
		nErrCode,
		szErrMess ? szErrMess : "");

	if (bDeleteMsg && szErrMess)
	{
		sqlite3_free(szErrMess);
	}
}

CppSQLite3Exception::CppSQLite3Exception(const CppSQLite3Exception& e)
{
	mpszErrMess = 0;
	if (e.mpszErrMess)
	{
		mpszErrMess = sqlite3_mprintf("%s", e.mpszErrMess);
	}
}

CppSQLite3Exception::~CppSQLite3Exception()
{
	if (mpszErrMess)
	{
		sqlite3_free(mpszErrMess);
		mpszErrMess = 0;
	}
}

const char* CppSQLite3Exception::errorCodeAsString(int nErrCode)
{
	switch (nErrCode)
	{
	case SQLITE_OK:		
		return "SQLITE_OK";
	case SQLITE_ERROR:
		return "SQLITE_ERROR";
	case SQLITE_INTERNAL:
		return "SQLITE_INTERNAL";
	case SQLITE_PERM: 
		return "SQLITE_PERM";
	case SQLITE_ABORT: 
		return "SQLITE_ABORT";
	case SQLITE_BUSY: 
		return "SQLITE_BUSY";
	case SQLITE_LOCKED: 
		return "SQLITE_LOCKED";
	case SQLITE_NOMEM: 
		return "SQLITE_NOMEM";
	case SQLITE_READONLY: 
		return "SQLITE_READONLY";
	case SQLITE_INTERRUPT: 
		return "SQLITE_INTERRUPT";
	case SQLITE_IOERR: 
		return "SQLITE_IOERR";
	case SQLITE_CORRUPT: 
		return "SQLITE_CORRUPT";
	case SQLITE_NOTFOUND: 
		return "SQLITE_NOTFOUND";
	case SQLITE_FULL: 
		return "SQLITE_FULL";
	case SQLITE_CANTOPEN: 
		return "SQLITE_CANTOPEN";
	case SQLITE_PROTOCOL:
		return "SQLITE_PROTOCOL";
	case SQLITE_EMPTY: 
		return "SQLITE_EMPTY";
	case SQLITE_SCHEMA: 
		return "SQLITE_SCHEMA";
	case SQLITE_TOOBIG: 
		return "SQLITE_TOOBIG";
	case SQLITE_CONSTRAINT: 
		return "SQLITE_CONSTRAINT";
	case SQLITE_MISMATCH: 
		return "SQLITE_MISMATCH";
	case SQLITE_MISUSE: 
		return "SQLITE_MISUSE";
	case SQLITE_NOLFS:
		return "SQLITE_NOLFS";
	case SQLITE_AUTH: 
		return "SQLITE_AUTH";
	case SQLITE_FORMAT: 
		return "SQLITE_FORMAT";
	case SQLITE_RANGE: 
		return "SQLITE_RANGE";
	case SQLITE_ROW: 
		return "SQLITE_ROW";
	case SQLITE_DONE:
		return "SQLITE_DONE";
	case CPPSQLITE_ERROR: 
		return "CPPSQLITE_ERROR";
	default: 
		return "UNKNOWN_ERROR";
	}
}

CppSQLite3Buffer::CppSQLite3Buffer()
{
	mpBuf = 0;
}

CppSQLite3Buffer::~CppSQLite3Buffer()
{
	clear();
}

void CppSQLite3Buffer::clear()
{
	if (mpBuf)
	{
		sqlite3_free(mpBuf);
		mpBuf = 0;
	}

}

const char* CppSQLite3Buffer::format(const char* szFormat, ...)
{
	clear();
	va_list va;
	va_start(va, szFormat);
	mpBuf = sqlite3_vmprintf(szFormat, va);
	va_end(va);
	return mpBuf;
}

CppSQLite3Query::CppSQLite3Query()
{
	mpVM = 0;
	mbEof = true;
	mnCols = 0;
	mbOwnVM = false;
}

CppSQLite3Query::CppSQLite3Query(const CppSQLite3Query& rQuery)
{
	mpVM = rQuery.mpVM;
	const_cast<CppSQLite3Query&>(rQuery).mpVM = 0;
	mbEof = rQuery.mbEof;
	mnCols = rQuery.mnCols;
	mbOwnVM = rQuery.mbOwnVM;
}

CppSQLite3Query::CppSQLite3Query(sqlite3* pDB,
	sqlite3_stmt* pVM,
	bool bEof,
	bool bOwnVM/*=true*/)
{
	mpDB = pDB;
	mpVM = pVM;
	mbEof = bEof;
	mnCols = sqlite3_column_count(mpVM);
	mbOwnVM = bOwnVM;
}

CppSQLite3Query::~CppSQLite3Query()
{
	try
	{
		finalize();
	}
	catch (...)
	{
	}
}

CppSQLite3Query& CppSQLite3Query::operator=(const CppSQLite3Query& rQuery)
{
	try
	{
		finalize();
	}
	catch (...)
	{
	}
	mpVM = rQuery.mpVM;
	const_cast<CppSQLite3Query&>(rQuery).mpVM = 0;
	mbEof = rQuery.mbEof;
	mnCols = rQuery.mnCols;
	mbOwnVM = rQuery.mbOwnVM;
	return *this;
}

int CppSQLite3Query::numFields()
{
	checkVM();
	return mnCols;
}

const char* CppSQLite3Query::fieldValue(int nField)
{
	checkVM();

	if (nField < 0 || nField > mnCols - 1)
	{
		throw CppSQLite3Exception(CPPSQLITE_ERROR,
			"Invalid field index requested",
			DONT_DELETE_MSG);
	}

	return (const char*)sqlite3_column_text(mpVM, nField);
}

const char* CppSQLite3Query::fieldValue(const char* szField)
{
	int nField = fieldIndex(szField);
	return (const char*)sqlite3_column_text(mpVM, nField);
}


int CppSQLite3Query::getIntField(int nField, int nNullValue/*=0*/)
{
	if (fieldDataType(nField) == SQLITE_NULL)
	{
		return nNullValue;
	}
	else
	{
		return sqlite3_column_int(mpVM, nField);
	}
}


int CppSQLite3Query::getIntField(const char* szField, int nNullValue/*=0*/)
{
	int nField = fieldIndex(szField);
	return getIntField(nField, nNullValue);
}


double CppSQLite3Query::getFloatField(int nField, double fNullValue/*=0.0*/)
{
	if (fieldDataType(nField) == SQLITE_NULL)
	{
		return fNullValue;
	}
	else
	{
		return sqlite3_column_double(mpVM, nField);
	}
}


double CppSQLite3Query::getFloatField(const char* szField, double fNullValue/*=0.0*/)
{
	int nField = fieldIndex(szField);
	return getFloatField(nField, fNullValue);
}


const char* CppSQLite3Query::getStringField(int nField, const char* szNullValue/*=""*/)
{
	if (fieldDataType(nField) == SQLITE_NULL)
	{
		return szNullValue;
	}
	else
	{
		return (const char*)sqlite3_column_text(mpVM, nField);
	}
}


const char* CppSQLite3Query::getStringField(const char* szField, const char* szNullValue/*=""*/)
{
	int nField = fieldIndex(szField);
	return getStringField(nField, szNullValue);
}


const unsigned char* CppSQLite3Query::getBlobField(int nField, int& nLen)
{
	checkVM();

	if (nField < 0 || nField > mnCols - 1)
	{
		throw CppSQLite3Exception(CPPSQLITE_ERROR,
			"Invalid field index requested",
			DONT_DELETE_MSG);
	}

	nLen = sqlite3_column_bytes(mpVM, nField);
	return (const unsigned char*)sqlite3_column_blob(mpVM, nField);
}


const unsigned char* CppSQLite3Query::getBlobField(const char* szField, int& nLen)
{
	int nField = fieldIndex(szField);
	return getBlobField(nField, nLen);
}


bool CppSQLite3Query::fieldIsNull(int nField)
{
	return (fieldDataType(nField) == SQLITE_NULL);
}


bool CppSQLite3Query::fieldIsNull(const char* szField)
{
	int nField = fieldIndex(szField);
	return (fieldDataType(nField) == SQLITE_NULL);
}
bool CppSQLite3Query::fieldIsExist(const char* szField)
{
	checkVM();
	int nID = 0;
	if (szField)
	{
		for (int nField = 0; nField < mnCols; nField++)
		{
			const char* szTemp = sqlite3_column_name(mpVM, nField);

			if (strcmp(szField, szTemp) == 0)
			{
				nID = nField;
				return nField;
			}
		}
	}
	return nID;
}

int CppSQLite3Query::fieldIndex(const char* szField)
{
	checkVM();

	if (szField)
	{
		for (int nField = 0; nField < mnCols; nField++)
		{
			const char* szTemp = sqlite3_column_name(mpVM, nField);

			if (strcmp(szField, szTemp) == 0)
			{
				return nField;
			}
		}
	}

	throw CppSQLite3Exception(CPPSQLITE_ERROR,
		"Invalid field name requested",
		DONT_DELETE_MSG);
}


const char* CppSQLite3Query::fieldName(int nCol)
{
	checkVM();

	if (nCol < 0 || nCol > mnCols - 1)
	{
		throw CppSQLite3Exception(CPPSQLITE_ERROR,
			"Invalid field index requested",
			DONT_DELETE_MSG);
	}

	return sqlite3_column_name(mpVM, nCol);
}


const char* CppSQLite3Query::fieldDeclType(int nCol)
{
	checkVM();

	if (nCol < 0 || nCol > mnCols - 1)
	{
		throw CppSQLite3Exception(CPPSQLITE_ERROR,
			"Invalid field index requested",
			DONT_DELETE_MSG);
	}

	return sqlite3_column_decltype(mpVM, nCol);
}


int CppSQLite3Query::fieldDataType(int nCol)
{
	checkVM();

	if (nCol < 0 || nCol > mnCols - 1)
	{
		throw CppSQLite3Exception(CPPSQLITE_ERROR,
			"Invalid field index requested",
			DONT_DELETE_MSG);
	}

	return sqlite3_column_type(mpVM, nCol);
}


bool CppSQLite3Query::eof()
{
	checkVM();
	return mbEof;
}


void CppSQLite3Query::nextRow()
{
	checkVM();

	int nRet = sqlite3_step(mpVM);

	if (nRet == SQLITE_DONE)
	{
		// no rows
		mbEof = true;
	}
	else if (nRet == SQLITE_ROW)
	{
		// more rows, nothing to do
	}
	else
	{
		nRet = sqlite3_finalize(mpVM);
		mpVM = 0;
		const char* szError = sqlite3_errmsg(mpDB);
		throw CppSQLite3Exception(nRet,
			(char*)szError,
			DONT_DELETE_MSG);
	}
}


void CppSQLite3Query::finalize()
{
	if (mpVM && mbOwnVM)
	{
		int nRet = sqlite3_finalize(mpVM);
		mpVM = 0;
		if (nRet != SQLITE_OK)
		{
			const char* szError = sqlite3_errmsg(mpDB);
			throw CppSQLite3Exception(nRet, (char*)szError, DONT_DELETE_MSG);
		}
	}
}


void CppSQLite3Query::checkVM()
{
	if (mpVM == 0)
	{
		throw CppSQLite3Exception(CPPSQLITE_ERROR,
			"Null Virtual Machine pointer",
			DONT_DELETE_MSG);
	}
}

CppSQLite3DB::CppSQLite3DB()
{
	mpDB = 0;
	mnBusyTimeoutMs = 60000; // 60 seconds
}

CppSQLite3DB::~CppSQLite3DB()
{
	close();
}

int CppSQLite3DB::OpenDataBase(const char* pDBFileName)
{
	if (NULL == pDBFileName)
	{
		return -1;
	}

	open(pDBFileName);  ///< �����ݿ�
}

void CppSQLite3DB::open(const char* szFile)
{
	InitializeCriticalSection(&mpCS);
	int nRet = sqlite3_open(szFile, &mpDB);

	if (nRet != SQLITE_OK)
	{
		const char* szError = sqlite3_errmsg(mpDB);
		throw CppSQLite3Exception(nRet, (char*)szError, DONT_DELETE_MSG);
	}

	sqlite3_busy_timeout(mpDB, mnBusyTimeoutMs);
}

void CppSQLite3DB::close()
{
	if (mpDB)
	{
		DeleteCriticalSection(&mpCS);
		sqlite3_close(mpDB);
		mpDB = 0;
	}

}

void CppSQLite3DB::setBusyTimeout(int nMillisecs)
{
	mnBusyTimeoutMs = nMillisecs;
	sqlite3_busy_timeout(mpDB, mnBusyTimeoutMs);
}

bool CppSQLite3DB::tableExists(const char* szTable)
{

	char szSQL[128];
	sprintf_s(szSQL,
		"select count(*) from sqlite_master where type='table' and name='%s'",
		szTable);
	int nRet = execScalar(szSQL);
	return (nRet > 0);
}

int CppSQLite3DB::execScalar(const char* szSQL)
{
	CppSQLite3Query q = execQuery(szSQL);

	if (q.eof() || q.numFields() < 1)
	{
		throw CppSQLite3Exception(CPPSQLITE_ERROR,
			"Invalid scalar query",
			DONT_DELETE_MSG);
	}

	int count = atoi(q.fieldValue(0));

	q.finalize();
	return count;
}

CppSQLite3Query CppSQLite3DB::execQuery(const char* szSQL)
{
	checkDB();

	sqlite3_stmt* pVM = compile(szSQL);

	int nRet = sqlite3_step(pVM);

	if (nRet == SQLITE_DONE)
	{
		// no rows
		return CppSQLite3Query(mpDB, pVM, true/*eof*/);
	}
	else if (nRet == SQLITE_ROW)
	{
		// at least 1 row
		return CppSQLite3Query(mpDB, pVM, false/*eof*/);
	}
	else
	{
		nRet = sqlite3_finalize(pVM);
		const char* szError = sqlite3_errmsg(mpDB);
		throw CppSQLite3Exception(nRet, (char*)szError, DONT_DELETE_MSG);
	}
}

void CppSQLite3DB::checkDB()
{

	if (!mpDB)
	{
		throw CppSQLite3Exception(CPPSQLITE_ERROR,
			"Database not open",
			DONT_DELETE_MSG);
	}
}

sqlite3_stmt* CppSQLite3DB::compile(const char* szSQL)
{
	checkDB();

	char* szError = 0;
	const char* szTail = 0;
	sqlite3_stmt* pVM;

	int nRet = sqlite3_prepare(mpDB, szSQL, -1, &pVM, &szTail);

	if (nRet != SQLITE_OK)
	{
		throw CppSQLite3Exception(nRet, szError);
	}
	return pVM;
}
