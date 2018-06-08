# 【UE4源代码分析】-007 仿UE4使用ADO进行数据库操作
&emsp;&emsp;由于工作的关系，主要开发平台是在windows上。在一些小项目中需要使用数据库时，经常会先使用ACCESS制作项目原型，等验证之后再转移到MySQL下。通常使用的连接Access的方式是ADO，由于公司以往没有对ADO进行封装，使用时颇为不便。
&emsp;&emsp;最近看UE4的源代码的时候发现其中有对ADO的一个封装，阅读之后觉得挺好，因此想着把这部分从UE4中独立出来，看看在普通场景下的应用效果如何。
[TOC]
## 1、UE4的DataBase支持
&emsp;&emsp;**UE4**对数据库的支持主要使用`DatabaseSupport` Module。其实现文件在`\Engine\Source\Runtime\DatabaseSupport\`路径中，主要包括Database.h\Database.cpp,DatabaseSupport.h\DatabaseSupport.cpp四个文件，其中DatabaseSupport.h\DatabaseSupport.cpp主要用于实现数据库支持模块，具体的数据库操作都实现在Database.h\Database.cpp中。
&emsp;&emsp;因此，在这里我主要讨论Database.h\Database.cpp这两个文件。
&emsp;&emsp;在Database.h文件开头处，首先进行了三个宏定义：
```C++
/**
 * Whether to compile in support for database connectivity and SQL execution.
 */
#ifndef WITH_DATABASE_SUPPORT
	#define WITH_DATABASE_SUPPORT (!UE_BUILD_MINIMAL && !UE_BUILD_SHIPPING)
#endif

// Only use ADO on windows, if support is enabled and not for shipping games.
// @todo clang: #import is not supported by Clang on Windows platform, but we currently need this for ADO symbol importing.  For now we disable ADO support in Clang builds.
#define USE_ADO_INTEGRATION (PLATFORM_WINDOWS && !(UE_BUILD_SHIPPING && WITH_EDITOR) && WITH_DATABASE_SUPPORT && !PLATFORM_COMPILER_CLANG)
#define USE_REMOTE_INTEGRATION (!USE_ADO_INTEGRATION && !(UE_BUILD_SHIPPING && WITH_EDITOR) && WITH_DATABASE_SUPPORT && !PLATFORM_COMPILER_CLANG)
```
&emsp;&emsp;从第一个宏`WITH_DATABASE_SUPPORT`可以发现，UE4默认情况下，只有在非发行模式并且是非最小模式编译时才能启动数据库支持。
&emsp;&emsp;第二个宏`USE_ADO_INTEGRATION`可以理解为启用ADO数据库支持需要同时满足四个条件：
- Windows平台；
- 不是带编辑器的发行版；
- 启用了数据库支持；
- 编译器不是CLang。（*CLang编译器在windows平台上不支持#import操作，目前暂时禁用了windows平台上CLang编译时的ADO支持*）。
&emsp;&emsp;第三个宏`USE_REMOTE_INTEGRATION`是启用远程数据库支持的宏，在这里我们不做详细讨论。
## 2、UE4 ADO封装解析
&emsp;&emsp;一般的数据库操作主要关心连接以及记录集，在UE4中，分别为连接和记录集定义了支持的类`FDataBaseRecordSet`和`FDataBaseConnection`类，为了方便操作记录集，还定义了辅助结构`FDatabaseColumnInfo`和内部迭代器类`TIterator`。在以上这些类的基础上，分别为ADO操作数据库派生了记录集类和数据库连接类。
<img src='007-Database-RecordSet.png'/>
<div align=center>图1 UE4数据库操作记录集类图</div>

<img src='007-Database-Connection.png'/>
<div algin=center>图2 UE4数据库操作数据库连接类图</div>

### 2.1 基类
#### 2.1.1 FDatabaseColumnInfo
&emsp;&emsp;`FDatabaseColumnInfo`主要用于辅助操作记录集，用于记录记录集中列的名称和数据类型。
```C++
/**   
 * This struct holds info relating to a column.  Specifically, we need to get back  
 * certain meta info from a RecordSet so we can "Get" data from it.  
 */  
struct FDatabaseColumnInfo  
{  
	/** Default constructor **/  
	FDatabaseColumnInfo(): DataType(DBT_UNKOWN) {}  

	/** The name of the column **/  
	FString ColumnName;  

	/** This is the type of data in this column.  (e.g. so you can do GetFloat or GetInt on the column **/  
	EDataBaseUnrealTypes DataType;  


	bool operator==(const FDatabaseColumnInfo& OtherFDatabaseColumnInfo) const  
	{  
		return (ColumnName==OtherFDatabaseColumnInfo.ColumnName) && (DataType==OtherFDatabaseColumnInfo.DataType);  
	}  

};  
```
&emsp;&emsp;在`FDataBaseRecordSet::GetColumnNames`接口中，返回的是`FDatabaseColumnInfo`对象组成的数组。
#### 2.1.2 FDataBaseRecordSet
&emsp;&emsp;`FDataBaseRecordSet`类是数据库记录集的基类，主要定义了数据库记录集对象需要具备的接口函数，并实现了一个空版本的接口，并为遍历记录集提供了迭代器。
```C++
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
	virtual int32 GetRecordCount() const
	{ 
		return 0;
	}

	/**
	 * Returns a string associated with the passed in field/ column for the current row.
	 *
	 * @param	Column	Name of column to retrieve data for in current row
	 */
	virtual FString GetString( const TCHAR* Column ) const
	{
		return TEXT("No database connection compiled in.");
	}

	/**
	 * Returns an integer associated with the passed in field/ column for the current row.
	 *
	 * @param	Column	Name of column to retrieve data for in current row
	 */
	virtual int32 GetInt( const TCHAR* Column ) const
	{
		return 0;
	}

	/**
	 * Returns a float associated with the passed in field/ column for the current row.
	 *
	 * @param	Column	Name of column to retrieve data for in current row
	 */
	virtual float GetFloat( const TCHAR* Column ) const
	{
		return 0;
	}

	/**
	 * Returns a int64 associated with the passed in field/ column for the current row.
	 *
	 * @param	Column	Name of column to retrieve data for in current row
	 */
	virtual int64 GetBigInt( const TCHAR* Column ) const
	{
		return 0;
	}

	/**  
      * Returns the set of column names for this Recordset.  This is useful for determining  
      * what you can actually ask the record set for without having to hard code those ahead of time.  
      */  
     virtual TArray<FDatabaseColumnInfo> GetColumnNames() const  
     {  
          TArray<FDatabaseColumnInfo> Retval;  
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
		TIterator( FDataBaseRecordSet* InRecordSet )
		: RecordSet( InRecordSet )
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
```
#### 2.1.3 FDataBaseConnection
&emsp;&emsp;`FDataBaseConnection`是数据库连接类的基类，主要定义了数据库操作的连接、关闭、执行SQL语句等接口。
```C++
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
	virtual bool Open( const TCHAR* ConnectionString, const TCHAR* RemoteConnectionIP, const TCHAR* RemoteConnectionStringOverride )
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
	virtual bool Execute( const TCHAR* CommandString )
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
	virtual bool Execute( const TCHAR* CommandString, FDataBaseRecordSet*& RecordSet )
	{
		RecordSet = nullptr;
		return false;
	}

	/**
	 * Static function creating appropriate database connection object.
	 *
	 * @return	instance of platform specific database connection object
	 */
	DATABASESUPPORT_API static FDataBaseConnection* CreateObject();
};
```
### 2.2 派生类
&emsp;&emsp;基类实现的都是空操作，因此，需要对基类进行有效的派生之后才能用于实际使用。UE4的ADO支持就是通过对基类的有效派生，将数据库操作与ADO操作数据库相结合，完成数据库的操作。
#### 2.2.1 ADO的引入
&emsp;&emsp;通常情况下，在C++中使用ADO操作数据库都会通过`#import`指令引入msado15.dll，UE4也不例外引入了msado15.dll。不同之处在于引入时的文件路径设置。
```C++
// Using import allows making use of smart pointers easily. Please post to the list if a workaround such as
// using %COMMONFILES% works to hide the localization issues and non default program file folders.
//#import "C:\Program files\Common Files\System\Ado\msado15.dll" rename("EOF", "ADOEOF")

#pragma warning(push)
#pragma warning(disable: 4471) // a forward declaration of an unscoped enumeration must have an underlying type (int assumed)
#import "System\ADO\msado15.dll" rename("EOF", "ADOEOF") //lint !e322
#pragma warning(pop)
```
&emsp;&emsp;一般情况下，会采用`//#import "C:\Program files\Common Files\System\Ado\msado15.dll" rename("EOF", "ADOEOF")`的方式，采用绝对路径的方式引入，但由于系统安装不同的原因，可能导致msado15.dll并不在C盘下，而导致运行编译出错。
#### 2.2.2 FADODataBaseRecordSet
&emsp;&emsp;`FADODataBaseRecordSet`类派生自`FDataBaseRecordSet`类，主要完成ADO记录集的遍历、字段数据的提取的功能。限于篇幅，文章中对类的实现进行了删减，读者可以自行查看UE4的源代码。
```C++
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
	}
	/** Moves to the next record in the set. */
	virtual void MoveToNext()
	{
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
	virtual int32 GetRecordCount() const
	{
		return ADORecordSet->RecordCount;
	}

	/**
	 * Returns a string associated with the passed in field/ column for the current row.
	 *
	 * @param	Column	Name of column to retrieve data for in current row
	 */
	virtual FString GetString( const TCHAR* Column ) const
	{
		return ReturnString;
	}

	/**
	 * Returns an integer associated with the passed in field/ column for the current row.
	 *
	 * @param	Column	Name of column to retrieve data for in current row
	 */
	virtual int32 GetInt( const TCHAR* Column ) const
	{
		return ReturnValue;
	}

	/**
	 * Returns a float associated with the passed in field/ column for the current row.
	 *
	 * @param	Column	Name of column to retrieve data for in current row
	 */
	virtual float GetFloat( const TCHAR* Column ) const
	{
		return ReturnValue;
	}

	/**
	 * Returns an int64 associated with the passed in field/ column for the current row.
	 *
	 * @param	Column	Name of column to retrieve data for in current row
	 */
	virtual int64 GetBigInt( const TCHAR* Column ) const
	{
		return ReturnValue;
	}

	/**
	 * Returns the set of column names for this Recordset.  This is useful for determining  
	 * what you can actually ask the record set for without having to hard code those ahead of time.  
	 */  
	virtual TArray<FDatabaseColumnInfo> GetColumnNames() const
	{  
		return Retval;  
	}   


	/**
	 * Constructor, used to associate ADO record set with this class.
	 *
	 * @param InADORecordSet	ADO record set to use
	 */
	FADODataBaseRecordSet( ADODB::_RecordsetPtr InADORecordSet )
	:	ADORecordSet( InADORecordSet )
	{
	}

	/** Destructor, cleaning up ADO record set. */
	virtual ~FADODataBaseRecordSet()
	{
		if(ADORecordSet && (ADORecordSet->State & ADODB::adStateOpen))
		{
			// We're using smart pointers so all we need to do is close and assign NULL.
			ADORecordSet->Close();
		}

		ADORecordSet = NULL;
	}
};
```
#### 2.2.3 FADODataBaseConnection
&emsp;&emsp;`FADODataBaseConnection`派生自`FDataBaseConnection`，主要完成数据库的连接工作。
```C++
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
	virtual bool Open( const TCHAR* ConnectionString, const TCHAR* RemoteConnectionIP, const TCHAR* RemoteConnectionStringOverride )
	{
		return true;
	}

	/**
	 * Closes connection to database.
	 */
	virtual void Close()
	{
	}

	/**
	 * Executes the passed in command on the database.
	 *
	 * @param CommandString		Command to execute
	 *
	 * @return true if execution was successful, false otherwise
	 */
	virtual bool Execute( const TCHAR* CommandString )
	{
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
	virtual bool Execute( const TCHAR* CommandString, FDataBaseRecordSet*& RecordSet )
	{
		return RecordSet != NULL;
	}
};
```
&emsp;&emsp;从以上的类的实现可以看出，UE4中数据库对ADO的封装主要是将数据库连接封装成了`FADODataBaseConnection`，记录集封装成了`FADODataBaseRecordSet`类，并且在这两个类的实现中，涉及的UE基本类型主要是`FString`和`TArray`，这两个类都可以通过使用普通C++类进行替代，因此，将代码从UE4中分离出来难度将不会太大。
## 3、独立于UE4的封装
&emsp;&emsp;正如在第2节中所表明的，可以参考UE4 ADO操作数据库的方式对ADO进行封装，只要解决在无UE4代码的情况下替代FString和TARRY两个类即可。
&emsp;&emsp;在实际编译中发现在#import引入msado15.dll时，会报找不到文件的错误，我没有仔细去分析什么原因了，直接采用了绝对路径，等下次有时间再去找原因了。
&emsp;&emsp;我直接将其封装成了一个头文件，方便使用。**注意**，我是在MFC环境下封装的，所以头文件里会带上stdafx.h，需要使用的话请自己酌情修改。
&emsp;&emsp;代码比较长，在**附录**中。
## 4、测试
&emsp;&emsp;我尝试使用一个MFC工程，利用刚才封装的ADO操作数据库代码去连接并读取数据库里的内容。我使用的数据库是Access数据库，数据库表结构如下：
<img src='database.PNG'/>
<div align=center>图3 数据库表结构</div>

&emsp;&emsp;数据库中已有的记录集如所示。
<img src='records.PNG'/>
<div align=center>图4 6条数据记录</div>

&emsp;&emsp;在对话框窗口初始化的时候连接并读取数据库表中的记录，将记录总数和其中的username字段内容显示在界面上。
```C++
FDataBaseConnection* pConn = new FADODataBaseConnection;
	bool hr = pConn->Open(L"Provider=Microsoft.Jet.OLEDB.4.0;Data Source=DatabaseFromUE4.mdb", NULL, NULL);
	if (hr)
	{
		FDataBaseRecordSet* pRes = NULL;
		pConn->Execute(L"SELECT * FROM usernameS", pRes);
		if (pRes)
		{
			int nCount = pRes->GetRecordCount();
			CString strInfo;
			strInfo.Format(L"表中有%d条记录,名字分别是：", nCount);
			FDataBaseRecordSet::TIterator itor(pRes);
			while (itor)
			{
				CString temp = itor->GetString(L"username");
				TRACE(L"%s\n", temp);
				strInfo = strInfo + L" " + temp;
				++itor;
			}
			GetDlgItem(IDC_INFO)->SetWindowText(strInfo);
		}
		delete pRes;
		pRes = NULL;
	}
	pConn->Close();
	delete pConn;
	pConn = NULL;
```
&emsp;&emsp;程序运行界面如下所示。
<img src='run.PNG'/>
<div align=center>图5 程序运行界面</div>

&emsp;&emsp;从上图可以看出，通过封装的数据库操作，将数据库中的数据记录成功的读取出来了。
## 5、总结
&emsp;&emsp;参考UE4的数据库操作，对ADO操作数据库的过程进行了封装，能够完成数据库的查询，数据记录集的遍历，数据内容的获取。下次可以在项目中实际测试使用。
&emsp;&emsp;**开发时多总结，闲暇时多阅读代码，总会有收获的。**
## 6、附录
```C++
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
```
