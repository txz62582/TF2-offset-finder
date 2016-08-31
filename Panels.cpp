#include "SDK.h"
#include "Panels.h"

#include "Netvar.h"

#include <string>
#include <iostream>
#include <thread>
#include <fstream>

CScreenSize gScreenSize;
//===================================================================================
void __fastcall Hooked_PaintTraverse( PVOID pPanels, int edx, unsigned int vguiPanel, bool forceRepaint, bool allowForce )
{
	try
	{
		VMTManager& hook = VMTManager::GetHook(pPanels); //Get a pointer to the instance of your VMTManager with the function GetHook.
		hook.GetMethod<void(__thiscall*)(PVOID, unsigned int, bool, bool)>(gOffsets.iPaintTraverseOffset)(pPanels, vguiPanel, forceRepaint, allowForce); //Call the original.

		static unsigned int vguiMatSystemTopPanel;

		if (vguiMatSystemTopPanel == NULL)
		{
			const char* szName = gInts.Panels->GetName(vguiPanel);
			if( szName[0] == 'M' && szName[3] == 'S' ) //Look for MatSystemTopPanel without using slow operations like strcmp or strstr.
			{
				vguiMatSystemTopPanel = vguiPanel;
				Intro();
			}
		}
	}
	catch(...)
	{
		Log::Fatal("Failed PaintTraverse");
	}
}
//===================================================================================

std::string TypeToString( SendPropType type )
{
	switch( type )
	{
	case DPT_Int:
		return "INT";
	case DPT_Float:
		return "FLOAT";
	case DPT_Vector:
		return "VECTOR3";
	case DPT_VectorXY:
		return "VECTOR2";
	case DPT_String:
		return "STRING";
	case DPT_Array:
		return "ARRAY";
	case DPT_DataTable:
		return "TABLE";
	}

	return "UNKNOWN";
}

void DumpTable( RecvTable *pTable, int iLevel, std::ostream &m_file )
{
	if( !pTable )
		return;

	for( int j = 0; j < iLevel; j++ )
		m_file << " ";

	m_file << pTable->GetName() << std::endl;

	iLevel += 2;

	for( int i = 0; i < pTable->GetNumProps(); ++i )
	{
		RecvProp *pProp = pTable->GetProp( i );

		if( !pProp )
			continue;

		if( isdigit( pProp->GetName()[ 0 ] ) )
			continue;

		if( pProp->GetDataTable() )
		{
			DumpTable( pProp->GetDataTable(), iLevel + 1, m_file );
		}

		for( int j = 0; j < iLevel; j++ )
			m_file << " ";

		m_file << pProp->GetName() << " : 0x" << std::hex << pProp->GetOffset() << " [" << TypeToString( pProp->GetType() ) << "]" << std::endl;
	}

	if( iLevel == 2 )
		m_file << std::endl;
}

void SaveDump(std::ostream &f)
{
	ClientClass *pList = gInts.Client->GetAllClasses();

	while( pList )
	{
		DumpTable( pList->Table, 0, f);

		pList = pList->pNextClass;
	}
}

void findOffsetRecursive( RecvTable *pTable, std::string name )
{
	if( !pTable )
		return;

	for( int i = 0; i < pTable->GetNumProps(); ++i )
	{
		RecvProp *pProp = pTable->GetProp( i );

		if( !pProp )
			continue;

		if( isdigit( pProp->GetName()[ 0 ] ) )
			continue;

		if( pProp->GetDataTable() )
		{
			findOffsetRecursive( pProp->GetDataTable(), name );
		}
		
		if( name == pProp->GetName() )
		{
			cout << pTable->GetName() << endl;
			cout << " " << pProp->GetName() << " : 0x" << std::hex << pProp->GetOffset() << " [" << TypeToString( pProp->GetType() ) << "]" << std::endl;
			
		}

	}

	return;
}

void findOffset( std::string name )
{
	cout << name << endl;

	ClientClass *pList = gInts.Client->GetAllClasses();

	while( pList )
	{
		findOffsetRecursive( pList->Table, name );

		pList = pList->pNextClass;
	}
	return;
}
//===================================================================================

void Intro( void )
{
	try
	{
		Log::Msg("Injection Successful"); //If the module got here without crashing, it is good day.


		// start the semi-cli interface

		// first alloc console
		if( AllocConsole() )
		{
			freopen( "CONOUT$", "w", stdout );
			freopen( "CONOUT$", "w", stderr );
			freopen( "CONIN$", "r", stdin );
		}
		else
		{
			return;
		}

		auto ioThread = [&]()
		{
			cout << "F1 Offset Dumper (2016)" << endl;

			HMODULE clientDLL = gSignatures.GetModuleHandleSafe( "client.dll" );

			while( true )
			{
				cout << "> ";
				// blocking wait for input
				string in;
				getline( cin, in );
				//cin >> in;

				//cout << endl;

				// check the command

				if( in == "netvars" )
				{
					static bool oneTime = false;
					if( oneTime == false )
					{
						cout << "Initialising Netvars..." << endl;
						gNetvars.init();
						oneTime = true;
					}

					while( true )
					{
						cout << "netvars> ";
						getline( cin, in );

						if( in == "exit" )
							break;
						else if( in == "all" )
						{
							cout << "Enter file name or leave blank to dump to console: ";
							getline( cin, in );

							if( in != "" )
							{
								std::ofstream f;
								f.open( in );
								if( f.is_open() )
								{
									SaveDump( f );
								}
								else
								{
									cout << "Invalid file name." << endl;
								}
							}
							else
							{
								SaveDump( cout );
							}
						}
						else if( in == "help" )
						{
							cout << "Netvar Commands:" << endl;
							cout << "\"all\": dumps all netvars to a file or to the console" << endl;
							cout << "\"<netvar>\": dumps all netvars with this name" << endl;
						}
						else
						{
							// try to match for individual netvars

							if( in[ 0 ] == 'D' )
							{
								// user wants a table, give them the table
								auto pProp = gNetvars.get_prop( in.c_str() );

								if( pProp == nullptr )
									cout << "Invalid datatable" << endl;
								else if( pProp->GetType() == SendPropType::DPT_DataTable )
								{
									DumpTable( pProp->GetDataTable(), 0, cout );
								}
							}
							else
							{
								findOffset( in );
							}
						}

					}
				}
				else if( in == "exit" )
				{
					std::terminate();
				}
				else if( in == "localPlayer" )
				{
					CBaseEntity *pBaseEntity = GetBaseEntity( me );

					if( pBaseEntity == NULL )
					{
						cout << "Please go in game before trying to access this" << endl;
					}
					else
					{
						cout << "LocalEntity @ 0x" << hex << (*(DWORD *) pBaseEntity ) - ( DWORD ) clientDLL << dec << endl;
					}
				}
				else if( in == "entList" )
				{
					cout << "EntList @ 0x" << hex << ( *( DWORD * ) gInts.EntList ) - ( DWORD ) clientDLL << dec << endl;
				}
				else if( in == "help" )
				{
					cout << "Commands:" << endl;
					cout << "\"netvars\": enables netvar mode (use help in netvar mode for more details)" << endl;
					cout << "\"localPlayer\": gets the local player offset" << endl;
					cout << "\"entList\": gets the entity list" << endl;
				}
				else
				{
					cout << "Invalid Command" << endl;
				}

			}
		};

		std::thread io{ ioThread };

		io.detach();
	}
	catch(...)
	{
		Log::Fatal("Failed Intro");
	}
}