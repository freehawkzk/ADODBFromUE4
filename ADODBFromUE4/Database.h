#pragma once
#include "stdafx.h"
#include <vector>
/**
* Enums for Database types.  Each Database has their own set of DB types and
*/
enum EDataBaseUnrealTypes
{
	DBT_UNKOWN,
	DBT_FLOAT,
	DBT_INT,
	DBT_STRING,
};


/**
* This struct holds info relating to a column.  Specifically, we need to get back
* certain meta info from a RecordSet so we can "Get" data from it.
*/
struct FDatabaseColumnInfo
{
	/** Default constructor **/
	FDatabaseColumnInfo() : DataType(DBT_UNKOWN) {}

	/** The name of the column **/
	CString ColumnName;

	/** This is the type of data in this column.  (e.g. so you can do GetFloat or GetInt on the column **/
	EDataBaseUnrealTypes DataType;


	bool operator==(const FDatabaseColumnInfo& OtherFDatabaseColumnInfo) const
	{
		return (ColumnName == OtherFDatabaseColumnInfo.ColumnName) && (DataType == OtherFDatabaseColumnInfo.DataType);
	}

};

/**
* Empty base class for iterating over database records returned via query. Used on platforms not supporting
* a direct database connection.
*/
class FDataBaseRecordSet
{
	// Protected functions used internally for iteration.

protected:
	/** Moves to the first record in the set. */
	virtual void MoveToFirst()
	{}
	/** Moves to the next record in the set. */
	virtual void MoveToNext()
	{}
	/**
	* Returns whether we are at the end.
	*
	* @return true if at the end, false otherwise
	*/
	virtual bool IsAtEnd() const
	{
		return true;
	}

public:

	/**
	*   Returns a count of the number of records in the record set
	*/
	virtual int GetRecordCount() const
	{
		return 0;
	}

	/**
	* Returns a string associated with the passed in field/ column for the current row.
	*
	* @param	Column	Name of column to retrieve data for in current row
	*/
	virtual CString GetString(const TCHAR* Column) const
	{
		return TEXT("No database connection compiled in.");
	}

	/**
	* Returns an integer associated with the passed in field/ column for the current row.
	*
	* @param	Column	Name of column to retrieve data for in current row
	*/
	virtual int GetInt(const TCHAR* Column) const
	{
		return 0;
	}

	/**
	* Returns a float associated with the passed in field/ column for the current row.
	*
	* @param	Column	Name of column to retrieve data for in current row
	*/
	virtual float GetFloat(const TCHAR* Column) const
	{
		return 0;
	}

	/**
	* Returns a int64 associated with the passed in field/ column for the current row.
	*
	* @param	Column	Name of column to retrieve data for in current row
	*/
	virtual LONG64 GetBigInt(const TCHAR* Column) const
	{
		return 0;
	}

	/**
	* Returns the set of column names for this Recordset.  This is useful for determining
	* what you can actually ask the record set for without having to hard code those ahead of time.
	*/
	virtual std::vector<FDatabaseColumnInfo> GetColumnNames() const
	{
		std::vector<FDatabaseColumnInfo> Retval;
		return Retval;
	}

	/** Virtual destructor as class has virtual functions. */
	virtual ~FDataBaseRecordSet()
	{}

	/**
	* Iterator helper class based on FObjectIterator.
	*/
	class TIterator
	{
	public:
		/**
		* Initialization constructor.
		*
		* @param	InRecordSet		RecordSet to iterate over
		*/
		TIterator(FDataBaseRecordSet* InRecordSet)
			: RecordSet(InRecordSet)
		{
			RecordSet->MoveToFirst();
		}

		/**
		* operator++ used to iterate to next element.
		*/
		void operator++()
		{
			RecordSet->MoveToNext();
		}

		/** Conversion to "bool" returning true if the iterator is valid. */
		FORCEINLINE explicit operator bool() const
		{
			return !RecordSet->IsAtEnd();
		}
		/** inverse of the "bool" operator */
		FORCEINLINE bool operator !() const
		{
			return !(bool)*this;
		}

		// Access operators
		FORCEINLINE FDataBaseRecordSet* operator*() const
		{
			return RecordSet;
		}
		FORCEINLINE FDataBaseRecordSet* operator->() const
		{
			return RecordSet;
		}

	protected:
		/** Database record set being iterated over. */
		FDataBaseRecordSet* RecordSet;
	};
};



/**
* Empty base class for database access via executing SQL commands. Used on platforms not supporting
* a direct database connection.
*/
class FDataBaseConnection
{
public:
	/** Virtual destructor as we have virtual functions. */
	virtual ~FDataBaseConnection()
	{}

	/**
	* Opens a connection to the database.
	*
	* @param	ConnectionString	Connection string passed to database layer
	* @param   RemoteConnectionIP  The IP address which the RemoteConnection should connect to
	* @param   RemoteConnectionStringOverride  The connection string which the RemoteConnection is going to utilize
	*
	* @return	true if connection was successfully established, false otherwise
	*/
	virtual bool Open(const TCHAR* ConnectionString, const TCHAR* RemoteConnectionIP, const TCHAR* RemoteConnectionStringOverride)
	{
		return false;
	}

	/**
	* Closes connection to database.
	*/
	virtual void Close()
	{}

	/**
	* Executes the passed in command on the database.
	*
	* @param CommandString		Command to execute
	*
	* @return true if execution was successful, false otherwise
	*/
	virtual bool Execute(const TCHAR* CommandString)
	{
		return false;
	}

	/**
	* Executes the passed in command on the database. The caller is responsible for deleting
	* the created RecordSet.
	*
	* @param CommandString		Command to execute
	* @param RecordSet			Reference to recordset pointer that is going to hold result
	*
	* @return true if execution was successful, false otherwise
	*/
	virtual bool Execute(const TCHAR* CommandString, FDataBaseRecordSet*& RecordSet)
	{
		RecordSet = nullptr;
		return false;
	}

	/**
	* Static function creating appropriate database connection object.
	*
	* @return	instance of platform specific database connection object
	*/
	static FDataBaseConnection* CreateObject()
	{
		return new FDataBaseConnection();
	}
};

// Using import allows making use of smart pointers easily. Please post to the list if a workaround such as
// using %COMMONFILES% works to hide the localization issues and non default program file folders.
//#import "C:\Program files\Common Files\System\Ado\msado15.dll" rename("EOF", "ADOEOF")

#pragma warning(push)
#pragma warning(disable: 4471) // a forward declaration of an unscoped enumeration must have an underlying type (int assumed)
#import "C:\Program files\Common Files\System\ADO\msado15.dll" rename("EOF", "ADOEOF") //lint !e322
#pragma warning(pop)

/*-----------------------------------------------------------------------------
FADODataBaseRecordSet implementation.
-----------------------------------------------------------------------------*/

/**
* ADO implementation of database record set.
*/
class FADODataBaseRecordSet : public FDataBaseRecordSet
{
private:
	ADODB::_RecordsetPtr ADORecordSet;

protected:
	/** Moves to the first record in the set. */
	virtual void MoveToFirst()
	{
		if (!ADORecordSet->BOF || !ADORecordSet->ADOEOF)
		{
			ADORecordSet->MoveFirst();
		}
	}
	/** Moves to the next record in the set. */
	virtual void MoveToNext()
	{
		if (!ADORecordSet->ADOEOF)
		{
			ADORecordSet->MoveNext();
		}
	}
	/**
	* Returns whether we are at the end.
	*
	* @return true if at the end, false otherwise
	*/
	virtual bool IsAtEnd() const
	{
		return !!ADORecordSet->ADOEOF;
	}

public:

	/**
	*   Returns a count of the number of records in the record set
	*/
	virtual int GetRecordCount() const
	{
		return ADORecordSet->RecordCount;
	}

	/**
	* Returns a string associated with the passed in field/ column for the current row.
	*
	* @param	Column	Name of column to retrieve data for in current row
	*/
	virtual CString GetString(const TCHAR* Column) const
	{
		CString ReturnString;

		// Retrieve specified column field value for selected row.
		_variant_t Value = ADORecordSet->GetCollect(Column);
		// Check variant type for validity and cast to specified type. _variant_t has overloaded cast operators.
		if (Value.vt != VT_NULL)
		{
			ReturnString = (TCHAR*)_bstr_t(Value);
		}
		// Unknown column.
		else
		{
			ReturnString = TEXT("Unknown Column");
		}

		return ReturnString;
	}

	/**
	* Returns an integer associated with the passed in field/ column for the current row.
	*
	* @param	Column	Name of column to retrieve data for in current row
	*/
	virtual int GetInt(const TCHAR* Column) const
	{
		int ReturnValue = 0;

		// Retrieve specified column field value for selected row.
		_variant_t Value = ADORecordSet->GetCollect(Column);
		// Check variant type for validity and cast to specified type. _variant_t has overloaded cast operators.
		if (Value.vt != VT_NULL)
		{
			ReturnValue = (int)Value;
		}
		return ReturnValue;
	}

	/**
	* Returns a float associated with the passed in field/ column for the current row.
	*
	* @param	Column	Name of column to retrieve data for in current row
	*/
	virtual float GetFloat(const TCHAR* Column) const
	{
		float ReturnValue = 0;

		// Retrieve specified column field value for selected row.
		_variant_t Value = ADORecordSet->GetCollect(Column);
		// Check variant type for validity and cast to specified type. _variant_t has overloaded cast operators.
		if (Value.vt != VT_NULL)
		{
			ReturnValue = (float)Value;
		}

		return ReturnValue;
	}

	/**
	* Returns an int64 associated with the passed in field/ column for the current row.
	*
	* @param	Column	Name of column to retrieve data for in current row
	*/
	virtual LONG64 GetBigInt(const TCHAR* Column) const
	{
		LONG64 ReturnValue = 0;

		// Retrieve specified column field value for selected row.
		_variant_t Value = ADORecordSet->GetCollect(Column);
		// Check variant type for validity and cast to specified type. _variant_t has overloaded cast operators.
		if (Value.vt != VT_NULL)
		{
			ReturnValue = (LONG64)Value;
		}

		return ReturnValue;
	}

	/**
	* Returns the set of column names for this Recordset.  This is useful for determining
	* what you can actually ask the record set for without having to hard code those ahead of time.
	*/
	virtual std::vector<FDatabaseColumnInfo> GetColumnNames() const
	{
		std::vector<FDatabaseColumnInfo> Retval;

		if (!ADORecordSet->BOF || !ADORecordSet->ADOEOF)
		{
			ADORecordSet->MoveFirst();

			for (int i = 0; i < ADORecordSet->Fields->Count; ++i)
			{
				_bstr_t bstrName = ADORecordSet->Fields->Item[i]->Name;
				_variant_t varValue = ADORecordSet->Fields->Item[i]->Value;
				ADODB::DataTypeEnum DataType = ADORecordSet->Fields->Item[i]->Type;

				FDatabaseColumnInfo NewInfo;
				NewInfo.ColumnName = CString((TCHAR*)_bstr_t(bstrName));

				// from http://www.w3schools.com/ado/prop_field_type.asp#datatypeenum  
				switch (DataType)
				{
				case ADODB::adInteger:
				case ADODB::adBigInt:
					NewInfo.DataType = DBT_INT;
					break;
				case ADODB::adSingle:
				case ADODB::adDouble:
					NewInfo.DataType = DBT_FLOAT;
					break;

				case ADODB::adWChar:
				case ADODB::adVarWChar:
					NewInfo.DataType = DBT_STRING;
					break;

				default:
					NewInfo.DataType = DBT_UNKOWN;
					break;
				}


				Retval.push_back(NewInfo);
			}
		}

		return Retval;
	}


	/**
	* Constructor, used to associate ADO record set with this class.
	*
	* @param InADORecordSet	ADO record set to use
	*/
	FADODataBaseRecordSet(ADODB::_RecordsetPtr InADORecordSet)
		: ADORecordSet(InADORecordSet)
	{
	}

	/** Destructor, cleaning up ADO record set. */
	virtual ~FADODataBaseRecordSet()
	{
		if (ADORecordSet && (ADORecordSet->State & ADODB::adStateOpen))
		{
			// We're using smart pointers so all we need to do is close and assign NULL.
			ADORecordSet->Close();
		}

		ADORecordSet = NULL;
	}
};


/*-----------------------------------------------------------------------------
FADODataBaseConnection implementation.
-----------------------------------------------------------------------------*/

/**
* Data base connection class using ADO C++ interface to communicate with SQL server.
*/
class FADODataBaseConnection : public FDataBaseConnection
{
private:
	/** ADO database connection object. */
	ADODB::_ConnectionPtr DataBaseConnection;

public:
	/** Constructor, initializing all member variables. */
	FADODataBaseConnection()
	{
		DataBaseConnection = NULL;
	}

	/** Destructor, tearing down connection. */
	virtual ~FADODataBaseConnection()
	{
		Close();
	}

	/**
	* Opens a connection to the database.
	*
	* @param	ConnectionString	Connection string passed to database layer
	* @param   RemoteConnectionIP  The IP address which the RemoteConnection should connect to
	* @param   RemoteConnectionStringOverride  The connection string which the RemoteConnection is going to utilize
	*
	* @return	true if connection was successfully established, false otherwise
	*/
	virtual bool Open(const TCHAR* ConnectionString, const TCHAR* RemoteConnectionIP, const TCHAR* RemoteConnectionStringOverride)
	{
		if (!::AfxOleInit())
		{
			return false;
		}

		// Create instance of DB connection object.
		HRESULT hr = DataBaseConnection.CreateInstance(__uuidof(ADODB::Connection));
		if (FAILED(hr))
		{
			::CoUninitialize();
			throw _com_error(hr);
		}

		// Open the connection. Operation is synchronous.
		DataBaseConnection->Open(ConnectionString, TEXT(""), TEXT(""), ADODB::adConnectUnspecified);

		return true;
	}

	/**
	* Closes connection to database.
	*/
	virtual void Close()
	{
		// Close database connection if exists and free smart pointer.
		if (DataBaseConnection && (DataBaseConnection->State & ADODB::adStateOpen))
		{
			DataBaseConnection->Close();

			::CoUninitialize();
		}

		DataBaseConnection = NULL;
	}

	/**
	* Executes the passed in command on the database.
	*
	* @param CommandString		Command to execute
	*
	* @return true if execution was successful, false otherwise
	*/
	virtual bool Execute(const TCHAR* CommandString)
	{
		// Execute command, passing in optimization to tell DB to not return records.
		DataBaseConnection->Execute(CommandString, NULL, ADODB::adExecuteNoRecords);

		return true;
	}

	/**
	* Executes the passed in command on the database. The caller is responsible for deleting
	* the created RecordSet.
	*
	* @param CommandString		Command to execute
	* @param RecordSet			Reference to recordset pointer that is going to hold result
	*
	* @return true if execution was successful, false otherwise
	*/
	virtual bool Execute(const TCHAR* CommandString, FDataBaseRecordSet*& RecordSet)
	{
		// Initialize return value.
		RecordSet = NULL;

		// Create instance of record set.
		ADODB::_RecordsetPtr ADORecordSet = NULL;
		ADORecordSet.CreateInstance(__uuidof(ADODB::Recordset));

		// Execute the passed in command on the record set. The recordset returned will be in open state so you can call Get* on it directly.
		ADORecordSet->Open(CommandString, _variant_t((IDispatch *)DataBaseConnection), ADODB::adOpenStatic, ADODB::adLockReadOnly, ADODB::adCmdText);

		// Create record set from returned data.
		RecordSet = new FADODataBaseRecordSet(ADORecordSet);

		return RecordSet != NULL;
	}

};