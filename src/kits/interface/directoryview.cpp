/*  libcosmoe.so - the interface to the Cosmoe UI
 *  Portions Copyright (C) 2001-2002 Bill Hayden
 *  Portions Copyright (C) 1999-2001 Kurt Skauen
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 *  MA 02111-1307, USA
 */


#include <assert.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

#include <OS.h>

#include <DirectoryView.h>
#include <Window.h>
#include <Bitmap.h>
#include <StringView.h>
#include <Button.h>
#include <Alert.h>
//#include <TableView.h>
#include <Requesters.h>
#include <Icon.h>
#include <Font.h>
#include <Looper.h>
#include <Message.h>
#include <Directory.h>
#include <NodeMonitor.h>
#include <String.h>

#include <list>
#include <set>
#include <sstream>

#include <macros.h>





namespace cosmoe_private
{
	enum Error_e { E_OK, E_SKIP, E_CANCEL, E_ALL };
	struct CopyFileParams_s
	{
		CopyFileParams_s( const std::vector<std::string> cDstPaths,
						const std::vector<std::string> cSrcPaths, const BMessenger& cViewTarget ) :
				m_cDstPaths(cDstPaths), m_cSrcPaths(cSrcPaths), m_cViewTarget(cViewTarget) {}

		std::vector<std::string> m_cDstPaths;
		std::vector<std::string> m_cSrcPaths;
		BMessenger               m_cViewTarget;
	};

	struct DeleteFileParams_s
	{
		DeleteFileParams_s( const std::vector<std::string> cPaths, const BMessenger& cViewTarget ) :
				m_cPaths(cPaths), m_cViewTarget(cViewTarget) {}
		std::vector<std::string> m_cPaths;
		BMessenger                      m_cViewTarget;
	};

	class DirKeeper : public BLooper
	{
	public:
		enum { M_CHANGE_DIR = 1, M_COPY_FILES, M_MOVE_FILES, M_DELETE_FILES, M_RENAME_FILES, M_ENTRIES_ADDED, M_ENTRIES_REMOVED, M_ENTRIES_UPDATED };
		DirKeeper( const BMessenger& cTarget, const std::string& cPath );
		virtual void MessageReceived( BMessage* pcMessage );
//		virtual bool Idle();
	private:
		void SendAddMsg( const std::string& cName );
		void SendRemoveMsg( const std::string& cName, dev_t nDevice, ino_t nInode );
		void SendUpdateMsg( const std::string& cName, dev_t nDevice, ino_t nInode );

		enum state_t { S_IDLE, S_READDIR };
		BMessenger    m_cTarget;
		BDirectory*   m_pcCurrentDir;
		state_t       m_eState;
		bool          m_bWaitForAddReply;
		bool          m_bWaitForRemoveReply;
		bool          m_bWaitForUpdateReply;

		struct MonitoringFileNode;
		struct FileNode
		{
			FileNode( const std::string& cName, dev_t nDevice, ino_t nInode ) : m_cName(cName), m_nDevice(nDevice), m_nInode(nInode) {}

			bool operator<(const FileNode& cNode ) const
			{
				return( m_nInode == cNode.m_nInode ? (m_nDevice < cNode.m_nDevice) : (m_nInode < cNode.m_nInode) );
			}
			bool operator<(const MonitoringFileNode& cNode ) const;

			mutable std::string m_cName;
			dev_t        m_nDevice;
			ino_t        m_nInode;
		};

		struct MonitoringFileNode : public FileNode
		{
			MonitoringFileNode( const std::string& cName, dev_t nDevice, ino_t nInode ) : FileNode( cName, nDevice, nInode ) {}
		};


		std::set<MonitoringFileNode> m_cFileMap;
		std::set<FileNode> m_cAddMap;
		std::set<FileNode> m_cUpdateMap;
		std::set<FileNode> m_cRemoveMap;
	};

	bool DirKeeper::FileNode::operator<(const DirKeeper::MonitoringFileNode& cNode ) const
	{
		return( m_nInode == cNode.m_nInode ? (m_nDevice < cNode.m_nDevice) : (m_nInode < cNode.m_nInode) );
	}

}

using namespace cosmoe_private;

bool load_icon( const char* pzPath, void* pBuffer, bool bLarge )
{
	IconDir sDir;
	IconHeader sHeader;

	FILE* hFile = fopen( pzPath, "r" );

	if ( hFile == NULL )
	{
		printf( "Failed to open file %s\n", pzPath );
		return( false );
	}

	if ( fread( &sDir, sizeof( sDir ), 1, hFile ) != 1 )
	{
		printf( "Failed to read icon dir\n" );
	}

	if ( sDir.nIconMagic != ICON_MAGIC )
	{
		printf( "Files %s is not an icon\n", pzPath );
		return( false );
	}

	for ( int i = 0 ; i < sDir.nNumImages ; ++i )
	{
		if ( fread( &sHeader, sizeof( sHeader ), 1, hFile ) != 1 )
		{
			printf( "Failed to read icon header\n" );
		}

		if ( sHeader.nWidth == 32 )
		{
			if ( bLarge )
			{
				fread( pBuffer, 32*32*4, 1, hFile );
			}
			else
			{
				fseek( hFile, 32*32*4, SEEK_CUR );
			}
		}
		else if ( sHeader.nWidth == 16 )
		{
			if ( bLarge == false )
			{
				fread( pBuffer, 16*16*4, 1, hFile );
			}
			else
			{
				fseek( hFile, 16*16*4, SEEK_CUR );
			}
		}
	}

	fclose( hFile );

	return( true );
}


DirKeeper::DirKeeper( const BMessenger& cTarget, const std::string& cPath ) : BLooper( "dir_worker" ),  m_cTarget( cTarget )
{
	m_pcCurrentDir = NULL;

	try
	{
		m_pcCurrentDir = new BDirectory( cPath.c_str() );
	}
	catch( std::exception& )
	{
	}

	if ( m_pcCurrentDir != NULL )
	{
		m_eState = S_READDIR;
	}
	else
	{
		m_eState = S_IDLE;
	}

	m_bWaitForAddReply    = false;
	m_bWaitForRemoveReply = false;
	m_bWaitForUpdateReply = false;
}

void DirKeeper::MessageReceived( BMessage* pcMessage )
{
	switch( pcMessage->what )
	{
		case M_CHANGE_DIR:
		{
			BString cNewPath;
			pcMessage->FindString( "path", &cNewPath );
			try {
				BDirectory* pcNewDir = new BDirectory( cNewPath.String() );
				delete m_pcCurrentDir;
				m_pcCurrentDir = pcNewDir;
				m_eState = S_READDIR;
				m_cTarget.SendMessage( DirectoryView::M_CLEAR );
				m_cFileMap.clear();
				m_cAddMap.clear();
				m_cRemoveMap.clear();
				m_cUpdateMap.clear();
				m_bWaitForAddReply    = false;
				m_bWaitForRemoveReply = false;
				m_bWaitForUpdateReply = false;
			}
			catch ( std::exception& )
			{
			}
			break;
		}
		case M_ENTRIES_ADDED:
		{
			m_bWaitForAddReply = false;
			if ( m_cAddMap.empty() == false )
			{
				BMessage cMsg( DirectoryView::M_ADD_ENTRY );
				int nCount = 0;

				while ( m_cAddMap.empty() == false )
				{
					std::set<FileNode>::iterator i = m_cAddMap.begin();
					try {
						BNode cNode( m_pcCurrentDir, (*i).m_cName.c_str()/*, O_RDONLY | O_NONBLOCK*/ );
						struct stat sStat;
						cNode.GetStat( &sStat );
						uint32 anIcon[16*16];
						const char* pzBaseDir = getenv( "COSMOE_SYS" );
						if( pzBaseDir == NULL )
						{
							pzBaseDir = "/cosmoe";
						}
						char* pzPath = new char[ strlen(pzBaseDir) + 80 ];
						if ( S_ISDIR( sStat.st_mode ) == false )
						{
							strcpy( pzPath, pzBaseDir );
							strcat( pzPath, "/icons/file.icon" );
							load_icon( pzPath, anIcon, false );
						}
						else
						{
							strcpy( pzPath, pzBaseDir );
							strcat( pzPath, "/icons/folder.icon" );
							load_icon( pzPath, anIcon, false );
						}
						delete[] pzPath; //FIXME: mem leak on exception
						cMsg.AddString( "name", (*i).m_cName.c_str() );
						cMsg.AddData( "stat", B_ANY_TYPE, &sStat, sizeof( sStat ) );
						cMsg.AddData( "small_icon", B_ANY_TYPE, anIcon, 16*16*4 );

					} catch( std::exception& )
					{
					}
					m_cAddMap.erase( i );
					if ( ++nCount > 5 )
					{
						break;
					}
				}
				m_bWaitForAddReply = true;
				m_cTarget.SendMessage( &cMsg );
			}
			break;
		}
		case M_ENTRIES_UPDATED:
		{
			m_bWaitForUpdateReply = false;
			if ( m_cUpdateMap.empty() == false )
			{
				BMessage cMsg( DirectoryView::M_UPDATE_ENTRY );
				int nCount = 0;
				while ( m_cUpdateMap.empty() == false )
				{
					std::set<FileNode>::iterator i = m_cUpdateMap.begin();
					try {
						BNode cNode( m_pcCurrentDir, (*i).m_cName.c_str()/*, O_RDONLY | O_NONBLOCK*/ );
						struct stat sStat;
						cNode.GetStat( &sStat );
						uint32 anIcon[16*16];

						const char* pzBaseDir = getenv( "COSMOE_SYS" );
						if( pzBaseDir == NULL )
						{
							pzBaseDir = "/cosmoe";
						}
						
						char* pzPath = new char[ strlen(pzBaseDir) + 80 ];

						if ( S_ISDIR( sStat.st_mode ) == false )
						{
							strcpy( pzPath, pzBaseDir );
							strcat( pzPath, "/icons/file.icon" );
							load_icon( pzPath, anIcon, false );
						}
						else
						{
							strcpy( pzPath, pzBaseDir );
							strcat( pzPath, "/icons/folder.icon" );
							load_icon( pzPath, anIcon, false );
						}
						delete[] pzPath; //FIXME: mem leak on exception
						cMsg.AddString( "name", (*i).m_cName.c_str() );
						cMsg.AddData( "stat", B_ANY_TYPE, &sStat, sizeof( sStat ) );
						cMsg.AddData( "small_icon", B_ANY_TYPE, anIcon, 16*16*4 );

					} catch( std::exception& )
					{
					}
					m_cUpdateMap.erase( i );
					if ( ++nCount > 5 )
					{
						break;
					}
				}
				m_bWaitForUpdateReply = true;
				m_cTarget.SendMessage( &cMsg );
			}
			break;
		}
		case M_ENTRIES_REMOVED:
		{
			m_bWaitForRemoveReply = false;
			if ( m_cRemoveMap.empty() == false )
			{
				BMessage cMsg( DirectoryView::M_REMOVE_ENTRY );
				int nCount = 0;
				while ( m_cRemoveMap.empty() == false )
				{
					std::set<FileNode>::iterator i = m_cRemoveMap.begin();

					cMsg.AddInt32( "device", (*i).m_nDevice );
					cMsg.AddInt64( "node", (*i).m_nInode );
					m_cRemoveMap.erase( i );
					if ( ++nCount > 10 )
					{
						break;
					}
				}
				m_cTarget.SendMessage( &cMsg );
				m_bWaitForRemoveReply = true;
			}
			break;
		}
		default:
			BLooper::MessageReceived( pcMessage );
			break;
	}
}

void DirKeeper::SendUpdateMsg( const std::string& cName, dev_t nDevice, ino_t nInode )
{
	MonitoringFileNode cFileNode( cName, nDevice, nInode );
	std::set<MonitoringFileNode>::iterator i = m_cFileMap.find( cFileNode );
	if ( i != m_cFileMap.end() )
	{
		try {
			if ( cName.size() > 0 )
			{
				(*i).m_cName = cName;
			}
			BNode cNode( m_pcCurrentDir, (*i).m_cName.c_str()/*, O_RDONLY | O_NONBLOCK*/ );
			struct stat sStat;
			cNode.GetStat( &sStat );

			FileNode cFileNode( (*i).m_cName, sStat.st_dev, sStat.st_ino );

//            std::set<FileNode>::iterator i = m_cRemoveMap.find( cFileNode );

			if ( m_bWaitForUpdateReply )
			{
				m_cUpdateMap.insert( cFileNode );
			}
			else
			{
				uint32 anIcon[16*16];

				const char* pzBaseDir = getenv( "COSMOE_SYS" );
				if( pzBaseDir == NULL )
				{
					pzBaseDir = "/cosmoe";
				}
				char* pzPath = new char[ strlen(pzBaseDir) + 80 ];
				if ( S_ISDIR( sStat.st_mode ) == false )
				{
					strcpy( pzPath, pzBaseDir );
					strcat( pzPath, "/icons/file.icon" );
					load_icon( pzPath, anIcon, false );
				}
				else
				{
					strcpy( pzPath, pzBaseDir );
					strcat( pzPath, "/icons/folder.icon" );
					load_icon( pzPath, anIcon, false );
				}
				delete[] pzPath; //FIXME: mem leak on exception

				BMessage cMsg( DirectoryView::M_UPDATE_ENTRY );

				cMsg.AddString( "name", (*i).m_cName.c_str() );
				cMsg.AddData( "stat", B_ANY_TYPE, &sStat, sizeof( sStat ) );
				cMsg.AddData( "small_icon", B_ANY_TYPE, anIcon, 16*16*4 );

				m_cTarget.SendMessage( &cMsg );
				m_bWaitForUpdateReply = true;
			}
		}
		catch( std::exception& )
		{
		}
	}
	else
	{
		printf( "Error: DirKeeper::SendUpdateMsg() got update message on non-existing node\n" );
	}
}

void DirKeeper::SendAddMsg( const std::string& cName )
{
	try
	{
		BNode cNode( m_pcCurrentDir, cName.c_str()/*, O_RDONLY | O_NONBLOCK*/ );
		struct stat sStat;
		cNode.GetStat( &sStat );

		FileNode cFileNode( cName, sStat.st_dev, sStat.st_ino );
		MonitoringFileNode cMFileNode( cName, sStat.st_dev, sStat.st_ino );

		std::set<FileNode>::iterator i = m_cRemoveMap.find( cFileNode );
		if ( i != m_cRemoveMap.end() )
		{
			m_cRemoveMap.erase( i );
		}

		std::set<MonitoringFileNode>::iterator j = m_cFileMap.find( cMFileNode );
		if ( j != m_cFileMap.end() )
		{
			SendUpdateMsg( cName, sStat.st_dev, sStat.st_ino );
			return;
		}

		if ( m_bWaitForAddReply )
		{
			m_cAddMap.insert( cFileNode );
		}
		else
		{
			uint32 anIcon[16*16];

			const char* pzBaseDir = getenv( "COSMOE_SYS" );
			if( pzBaseDir == NULL )
			{
				pzBaseDir = "/cosmoe";
			}
			char* pzPath = new char[ strlen(pzBaseDir) + 80 ];
			if ( S_ISDIR( sStat.st_mode ) == false )
			{
				strcpy( pzPath, pzBaseDir );
				strcat( pzPath, "/icons/file.icon" );
				load_icon( pzPath, anIcon, false );
			}
			else
			{
				strcpy( pzPath, pzBaseDir );
				strcat( pzPath, "/icons/folder.icon" );
				load_icon( pzPath, anIcon, false );
			}
			delete[] pzPath; //FIXME: mem leak on exception

			BMessage cMsg( DirectoryView::M_ADD_ENTRY );

			cMsg.AddString( "name", cName.c_str() );
			cMsg.AddData( "stat", B_ANY_TYPE, &sStat, sizeof( sStat ) );
			cMsg.AddData( "small_icon", B_ANY_TYPE, anIcon, 16*16*4 );

			m_cTarget.SendMessage( &cMsg );
			m_bWaitForAddReply = true;
		}
	} catch( .../*std::exception&*/ )
	{
	}
}

void DirKeeper::SendRemoveMsg( const std::string& cName, dev_t nDevice, ino_t nInode )
{
	FileNode cFileNode( cName, nDevice, nInode );
	MonitoringFileNode cMFileNode( cName, nDevice, nInode );
	std::set<FileNode>::iterator i = m_cAddMap.find( cFileNode );
	if ( i != m_cAddMap.end() )
	{
		m_cAddMap.erase( i );
		return;
	}

	std::set<MonitoringFileNode>::iterator j = m_cFileMap.find( cMFileNode );

	if ( j != m_cFileMap.end() )
	{
		m_cFileMap.erase( j );

		if ( m_bWaitForRemoveReply )
		{
			m_cRemoveMap.insert( cFileNode );
		}
		else
		{
			BMessage cMsg( DirectoryView::M_REMOVE_ENTRY );
			cMsg.AddInt32( "device", nDevice );
			cMsg.AddInt64( "node", nInode );
			m_cTarget.SendMessage( &cMsg );
			m_bWaitForRemoveReply = true;
		}
	}
}

/*
bool DirKeeper::Idle()
{
	switch( m_eState )
	{
		case S_READDIR:
		{
			if ( m_pcCurrentDir == NULL )
			{
				m_eState = S_IDLE;
				return( false );
			}
			std::string cName;
			if ( m_pcCurrentDir->GetNextEntryName( &cName ) == B_OK )
			{
				if ( cName != "." )
				{
					SendAddMsg( cName );
				}
				return( true );
			}
			else
			{
				m_eState = S_IDLE;
				return( false );
			}
			break;
		}
		case S_IDLE:
			return( false );
	}

	return( false );
}
*/

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int ReadFileDlg( int nFile, void* pBuffer, int nSize, const char* pzPath, Error_e* peError )
{
retry:
	int nLen = read( nFile, pBuffer, nSize );
	if ( nLen >= 0 )
	{
		*peError = E_OK;
		return( nLen );
	}
	else
	{
		if ( errno == EINTR )
		{
			goto retry;
		}
	}
	std::string cMsg("Failed to read from source file: ");

	cMsg += pzPath;
	cMsg += "\nError: ";
	cMsg += strerror( errno );

	BAlert* pcAlert = new BAlert( "Error:",
								cMsg.c_str(),
								"Skip", "Retry", "Cancel" );
	switch( pcAlert->Go() )
	{
		case 0:
			*peError = E_SKIP;
			break;

		case 1:
			goto retry;

		case 2:
			*peError = E_CANCEL;
			break;

		default:
			*peError = E_SKIP;
			break;
	}

	return( nLen );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int WriteFileDlg( int nFile, const void* pBuffer, int nSize, const char* pzPath, Error_e* peError )
{
retry:
	int nLen = write( nFile, pBuffer, nSize );
	if ( nLen >= 0 )
	{
		*peError = E_OK;
		return( nLen );
	}
	else
	{
		if ( errno == EINTR )
		{
			goto retry;
		}
	}
	std::string cMsg("Failed to write to destination file: ");
	cMsg += pzPath;
	cMsg += "\nError: ";
	cMsg += strerror( errno );

	BAlert* pcAlert = new BAlert( "Error:",
								cMsg.c_str(),
								"Skip", "Retry", "Cancel" );
	switch( pcAlert->Go() )
	{
		case 0:
			*peError = E_SKIP;
			break;
		case 1:
			goto retry;
		case 2:
			*peError = E_CANCEL;
			break;
		default:
			*peError = E_SKIP;
			break;
	}

	return( nLen );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool CopyFile( const char* pzDst, const char* pzSrc,
               bool* pbReplaceFiles, bool* pbDontOverwrite, ProgressRequester* pcProgress )
{
	struct stat sSrcStat;
	struct stat sDstStat;
	bool        bDirExists = false;

	if ( pcProgress->DoCancel() )
	{
		return( false );
	}

	while ( lstat( pzSrc, &sSrcStat ) < 0 )
	{
		std::string cMsg("Failed to open source: ");

		cMsg += pzSrc;
		cMsg += "\nError: ";
		cMsg += strerror( errno );
		BAlert* pcAlert = new BAlert( "Error:",
									cMsg.c_str(),
									"Skip", "Retry", "Cancel" );
		switch( pcAlert->Go() )
		{
			case 0:        return( true );
			case 1:        break;
			case 2:        return( false );
			default:       return( true );
		}
	}

	bool bLoop = true;
	while( bLoop && lstat( pzDst, &sDstStat ) >= 0 )
	{
		if ( sSrcStat.st_dev == sDstStat.st_dev && sSrcStat.st_ino == sDstStat.st_ino )
		{
			BAlert* pcAlert = new BAlert( "Error", "Source and destination are the same, can't copy.", "OK");
			pcAlert->Go( NULL );
			return( false );
		}

		if ( S_ISDIR( sDstStat.st_mode ) )
		{
			if ( S_ISDIR( sSrcStat.st_mode ) == false )
			{
				std::string cMsg("Can't replace directory  ");

				cMsg += pzDst;
				cMsg += " with a file\n";

				BAlert* pcAlert = new BAlert( "Error",
											  cMsg.c_str(),
											  "Skip", "Retry", "Cancel" );
				switch( pcAlert->Go() )
				{
					case 0:  return( true );
					case 1:  break;
					case 2:  return( false );
					default: return( true );
				}
			}
			else
			{
				bDirExists = true;
				break;
			}
		}
		else
		{
			if ( *pbDontOverwrite )
			{
				return( true );
			}
			
			if ( *pbReplaceFiles )
			{
				unlink( pzDst );
				break;
			}
			std::string cMsg("The destination file: ");
			cMsg += pzDst;
			cMsg += "already exists\nWould you like to replace it?";

			BAlert* pcAlert = new BAlert( "Error:",
										cMsg.c_str(),
										"Skip", "Replace", "Cancel" );
			switch( pcAlert->Go() )
			{
				case 0:        // Skip
					return( true );
					/*
				case 1: // Skip all
					*pbDontOverwrite = true;
					return( true );
					*/
				case 1:        // Replace
					bLoop = false;
					unlink( pzDst );
					break;
					/*
				case 3:        // Replace all
					*pbReplaceFiles = true;
					bLoop = false;
					unlink( pzDst );
					break;
					*/
				case 2:        // Cancel
					return( false );
				default: // Bad
					return( true );
			}
		}
	}

	if ( S_ISDIR( sSrcStat.st_mode ) )
	{
		pcProgress->Lock();
		pcProgress->SetPathName( pzDst );
		pcProgress->Unlock();

		if ( bDirExists == false )
		{
			while ( mkdir( pzDst, sSrcStat.st_mode ) < 0 )
			{
				std::string cMsg("Failed to create directory: ");

				cMsg += pzDst;
				cMsg += "\nError: ";
				cMsg += strerror( errno );
				BAlert* pcAlert = new BAlert( "Error",
											  cMsg.c_str(),
											  "Skip", "Retry", "Cancel" );
				switch( pcAlert->Go() )
				{
					case 0:     return( true );
					case 1:     break;
					case 2:     return( false );
					default:    return( true );
				}
			}
		}
		DIR* pDir;

		for (;;)
		{
			pDir = opendir( pzSrc );
			if ( pDir != NULL )
			{
				break;
			}
			std::string cMsg("Failed to open directory: ");
			cMsg += pzSrc;
			cMsg += "\nError: ";
			cMsg += strerror( errno );
			BAlert* pcAlert = new BAlert( "Error",
										  cMsg.c_str(),
										  "Skip", "Retry", "Cancel" );
			switch( pcAlert->Go() )
			{
				case 0:   return( true );
				case 1:   break;
				case 2:   return( false );
				default:  return( true );
			}
		}

		struct dirent* psEntry;

		while( (psEntry = readdir( pDir )) != NULL )
		{
			if ( strcmp( psEntry->d_name, "." ) == 0 || strcmp( psEntry->d_name, ".." ) == 0 )
			{
				continue;
			}
			BPath cSrcPath( pzSrc );
			BPath cDstPath( pzDst );
			cSrcPath.Append( psEntry->d_name );
			cDstPath.Append( psEntry->d_name );

			if ( CopyFile( cDstPath.Path(), cSrcPath.Path(),
						pbReplaceFiles, pbDontOverwrite, pcProgress ) == false )
			{
				closedir( pDir );
				return( false );
			}
		}
		closedir( pDir );
	}
	else if ( S_ISLNK( sSrcStat.st_mode ) )
	{
		pcProgress->Lock();
		pcProgress->SetFileName( BPath( pzDst ).Leaf() );
		pcProgress->Unlock();
		printf( "Copy link %s to %s\n", pzSrc, pzDst );
	}
	else
	{
		pcProgress->Lock();
		pcProgress->SetFileName( BPath( pzDst ).Leaf() );
		pcProgress->Unlock();

		int nSrcFile = -1;
		int nDstFile = -1;

		for (;;)
		{
			nSrcFile = open( pzSrc, O_RDONLY );
			if ( nSrcFile >= 0 )
			{
				break;
			}
			std::string cMsg("Failed to open source file: ");

			cMsg += pzSrc;
			cMsg += "\nError: ";
			cMsg += strerror( errno );

			BAlert* pcAlert = new BAlert( "Error:",
										  cMsg.c_str(),
										  "Skip", "Retry", "Cancel" );
			switch( pcAlert->Go() )
			{
				case 0:   return( true );
				case 1:   break;
				case 2:   return( false );
				default:  return( true );
			}
		}

		for (;;)
		{
			nDstFile = open( pzDst, O_WRONLY | O_CREAT | O_TRUNC, sSrcStat.st_mode );
			if ( nDstFile >= 0 )
			{
				break;
			}
			std::string cMsg("Failed to create destination file: ");

			cMsg += pzDst;
			cMsg += "\nError: ";
			cMsg += strerror( errno );

			BAlert* pcAlert = new BAlert( "Error",
										cMsg.c_str(),
										"Skip", "Retry", "Cancel" );
			switch( pcAlert->Go() )
			{
				case 0:
					close( nSrcFile );
					return( true );
				case 1:
					break;
				case 2:
					close( nSrcFile );
					return( false );
				default:
					close( nSrcFile );
					return( true );
			}
		}

		int nTotLen = 0;

		for (;;)
		{
			char anBuffer[4096];
			int  nLen;
			Error_e eError;

			if ( pcProgress->DoSkip() )
			{
				close( nSrcFile );
				close( nDstFile );
				unlink( pzDst );
				return( true );
			}
			
			for (;;)
			{
				nLen = ReadFileDlg( nSrcFile, anBuffer, 4096, pzSrc, &eError );
				if ( nLen >= 0 )
				{
					break;
				}
				close( nSrcFile );
				close( nDstFile );
				unlink( pzDst );

				if ( eError == E_SKIP )
				{
					return( true );
				}
				else if ( eError == E_CANCEL )
				{
					return( false );
				}
				else
				{
					assert( !"Invalid return code from ReadFileDlg()" );
					return( false );
				}
			}
			
			for (;;)
			{
				int nError = WriteFileDlg( nDstFile, anBuffer, nLen, pzDst, &eError );
				if ( nError >= 0 )
				{
					break;
				}

				close( nSrcFile );
				close( nDstFile );
				unlink( pzDst );

				if ( eError == E_SKIP )
				{
					return( true );
				}
				else if ( eError == E_CANCEL )
				{
					return( false );
				}
				else
				{
					assert( !"Invalid return code from WriteFileDlg()" );
					return( false );
				}
			}
			nTotLen += nLen;
			if ( nLen < 4096 )
			{
				break;
			}
		}
		close( nSrcFile );
		close( nDstFile );

		if ( nTotLen != sSrcStat.st_size )
		{
			std::stringstream cMsg;
			cMsg <<
				"Mismatch between number of bytes read (" << nTotLen << ")\n"
				"and file size reported by file system (" << sSrcStat.st_size << ")!\n"
				"It seems like we might end up corrupting the destination file\n";

			BAlert* pcAlert = new BAlert( "Warning", cMsg.str().c_str(), "Sorry" );
			pcAlert->Go();
		}
//    printf( "Copy file %s to %s\n", pzSrc, pzDst );
	}
	return( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool DeleteFile( const char* pzPath, ProgressRequester* pcProgress )
{
	struct stat sStat;

	if ( pcProgress->DoCancel() )
	{
		return( false );
	}

	while ( lstat( pzPath, &sStat ) < 0 )
	{
		std::string cMsg("Failed to stat file: ");

		cMsg += pzPath;
		cMsg += "\nError: ";
		cMsg += strerror( errno );
		BAlert* pcAlert = new BAlert( "Error:",
									cMsg.c_str(),
									"Skip", "Retry", "Cancel" );
		switch( pcAlert->Go() )
		{
			case 0:        return( true );
			case 1:        break;
			case 2:        return( false );
			default:       return( true );
		}
	}

	if ( S_ISDIR( sStat.st_mode ) )
	{
		pcProgress->Lock();
		pcProgress->SetPathName( pzPath );
		pcProgress->Unlock();

		DIR* pDir;
		for (;;)
		{
			pDir = opendir( pzPath );
			if ( pDir != NULL )
			{
				break;
			}

			std::string cMsg("Failed to open directory: ");

			cMsg += pzPath;
			cMsg += "\nError: ";
			cMsg += strerror( errno );

			BAlert* pcAlert = new BAlert( "Error:",
										cMsg.c_str(),
										"Skip", "Retry", "Cancel" );
			switch( pcAlert->Go() )
			{
				case 0:          return( true );
				case 1:          break;
				case 2:          return( false );
				default:  return( true );
			}
		}
		struct dirent* psEntry;

		std::list<std::string> cFileList;

		while( (psEntry = readdir( pDir )) != NULL )
		{
			if ( strcmp( psEntry->d_name, "." ) == 0 || strcmp( psEntry->d_name, ".." ) == 0 )
			{
				continue;
			}
			BPath cPath( pzPath );
			cPath.Append( psEntry->d_name );
			cFileList.push_back( cPath.Path() );
		}
		closedir( pDir );

		while( cFileList.empty() == false )
		{
			std::list<std::string>::iterator i = cFileList.begin();
			if ( DeleteFile( (*i).c_str(), pcProgress ) == false )
			{
				return( false );
			}
			cFileList.erase( i );
		}
//    printf( "delete dir %s\n", pzPath );
		while( rmdir( pzPath ) < 0 )
		{
			std::string cMsg("Failed to delete directory: ");

			cMsg += pzPath;
			cMsg += "\nError: ";
			cMsg += strerror( errno );

			BAlert* pcAlert = new BAlert( "Error:",
										cMsg.c_str(),
										"Skip", "Retry", "Cancel" );
			switch( pcAlert->Go() )
			{
				case 0:   return( true );
				case 1:   break;
				case 2:   return( false );
				default:  return( true );
			}
		}
	}
	else
	{
		pcProgress->Lock();
		pcProgress->SetFileName( BPath( pzPath ).Leaf() );
		pcProgress->Unlock();
//    printf( "delete file %s\n", pzPath );
		while( unlink( pzPath ) < 0 )
		{
			std::string cMsg("Failed to delete file: ");

			cMsg += pzPath;
			cMsg += "\nError: ";
			cMsg += strerror( errno );

			BAlert* pcAlert = new BAlert( "Error",
										  cMsg.c_str(),
										  "Skip", "Retry", "Cancel" );
			switch( pcAlert->Go() )
			{
				case 0:   return( true );
				case 1:   break;
				case 2:   return( false );
				default:  return( true );
			}
		}
	}
	return( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int32 DelFileThread( void* pData )
{
	DeleteFileParams_s* psArgs = (DeleteFileParams_s*) pData;

	std::stringstream cMsg;

	if ( psArgs->m_cPaths.size() == 1 )
	{
		struct stat sStat;
		if ( lstat( psArgs->m_cPaths[0].c_str(), &sStat ) < 0 )
		{
			cMsg << "Failed to open file " << psArgs->m_cPaths[0].c_str() << "\nError: " << strerror( errno );
			BAlert* pcAlert = new BAlert( "Error", cMsg.str().c_str(), "Sorry" );
			pcAlert->Go();
			delete psArgs;
			return( 1 );
		}
		if ( S_ISDIR( sStat.st_mode ) )
		{
			cMsg <<
				"Are you sure you want to delete\n"
				"the directory \"" << BPath( psArgs->m_cPaths[0].c_str() ).Leaf() << "\"\n"
				"and all its sub-directories?";
		}
		else
		{
			cMsg <<
				"Are you sure you want to delete the file\n"
				"\"" << BPath( psArgs->m_cPaths[0].c_str() ).Leaf() << "\"\n";
		}
	}
	else
	{
		cMsg << "Are you sure you want to delete those " << psArgs->m_cPaths.size() << " entries?";
	}

	BAlert* pcAlert = new BAlert( "Verify file deletion", cMsg.str().c_str(), "Cancel", "Delete" );

	int nButton = pcAlert->Go();

	if ( nButton != 1 )
	{
		delete psArgs;
		return( 1 );
	}

	ProgressRequester* pcProgress = new ProgressRequester( BRect( 50, 20, 350, 150 ),
														"Delete file:", false );

	for ( uint i = 0 ; i < psArgs->m_cPaths.size() ; ++i )
	{
		if ( DeleteFile( psArgs->m_cPaths[i].c_str(), pcProgress ) == false )
		{
			break;
		}
		BMessage cMsg( 125 );
		psArgs->m_cViewTarget.SendMessage( &cMsg );
	}

	pcProgress->Quit();

	delete psArgs;
	return( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void StartFileDelete( const std::vector<std::string>& cPaths, const BMessenger& cViewTarget )
{
	DeleteFileParams_s* psParams = new DeleteFileParams_s( cPaths, cViewTarget );
	thread_id hTread = spawn_thread( DelFileThread, "delete_file_thread", 0, psParams );
	if ( hTread >= 0 )
	{
		resume_thread( hTread );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

int32 CopyFileThread( void* pData )
{
	CopyFileParams_s* psParams = (CopyFileParams_s*) pData;

	ProgressRequester* pcProgress = new ProgressRequester( BRect( 50, 20, 350, 150 ),
														"Copy files:", true );

	for ( uint i = 0 ; i < psParams->m_cSrcPaths.size() ; ++i )
	{
		bool bReplaceFiles  = false;
		bool bDontOverwrite = false;

		if ( CopyFile( psParams->m_cDstPaths[i].c_str(), psParams->m_cSrcPaths[i].c_str(),
					&bReplaceFiles, &bDontOverwrite, pcProgress ) == false )
		{
			break;
		}

		BMessage cMsg( 125 );
		psParams->m_cViewTarget.SendMessage( &cMsg );
	}
	pcProgress->Quit();


	delete psParams;
	return( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void StartFileCopy( const std::vector<std::string>& cDstPaths,
                    const std::vector<std::string>& cSrcPaths, const BMessenger& cViewTarget )
{
	CopyFileParams_s* psParams = new CopyFileParams_s( cDstPaths, cSrcPaths, cViewTarget );
	thread_id hTread = spawn_thread( CopyFileThread, "copy_file_thread", 0, psParams );
	if ( hTread >= 0 )
	{
		resume_thread( hTread );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

DirectoryView::State::State( BListView* pcView, const char* pzPath ) : m_cPath( pzPath )
{
	for ( uint i = 0 ; i < pcView->GetRowCount() ; ++i )
	{
		if ( pcView->IsSelected( i ) )
		{
			FileRow* pcRow = dynamic_cast<FileRow*>(pcView->GetRow(i));
			if ( pcRow != NULL )
			{
				m_cSelection.push_back( pcRow->GetName() );
			}
		}
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

DirectoryView::DirectoryView( const BRect& cFrame, const std::string& cPath, uint32 nModeFlags, uint32 nResizeMask, uint32 nViewFlags ) :
    BListView( cFrame, "_list_view", nModeFlags, nResizeMask, nViewFlags ), m_cPath( cPath.c_str() )
{
fprintf( stderr, "DirectoryView::ctor\n" );
	m_pcCurReadDirSession = NULL;

	InsertColumn( "", 16 );  // Icon
	InsertColumn( "Name", 150 );
	InsertColumn( "Size", 50 );
	InsertColumn( "Attr", 70 );
	InsertColumn( "Date", 70 );
	InsertColumn( "Time", 70 );

	m_pcDirChangeMsg = NULL;

	m_nLastKeyDownTime = 0;
	m_pcIconBitmap = new BBitmap( BRect(0, 0, 15, 15), B_RGB32 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

DirectoryView::~DirectoryView()
{
	delete m_pcIconBitmap;
}

void DirectoryView::AttachedToWindow()
{
fprintf( stderr, "DirectoryView::AttachedToWindow\n" );
	BListView::AttachedToWindow();
	m_pcDirKeeper = new DirKeeper( BMessenger( this ), m_cPath.Path() );
	m_pcDirKeeper->Run();
	ReRead();
}

void DirectoryView::DetachedFromWindow()
{
	m_pcDirKeeper->Quit();
	m_pcDirKeeper = NULL;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::KeyDown(const char* bytes, int32 numBytes)
{
	char nChar = bytes[0];

	if ( isprint( nChar ) )
	{
		bigtime_t nTime = system_time();

		if ( nTime < m_nLastKeyDownTime + 1000000 )
		{
			m_cSearchString.append( &nChar, 1 );
		}
		else
		{
			m_cSearchString = std::string( &nChar, 1 );
		}
		
		for ( uint i = 0 ; i < GetRowCount() ; ++i )
		{
			FileRow* pcRow = dynamic_cast<FileRow*>(GetRow(i));
			if ( pcRow != NULL )
			{
				// FIXME: gcc 3.0.4 doesn't like argument 2
				//			someone please tell me if my substitute function is equivalent
				
#if __GNUC__ < 3
				if ( m_cSearchString.compare( pcRow->m_cName, 
											  0UL,
											  m_cSearchString.size() ) == 0 )
#else
				if ( m_cSearchString.compare(0, m_cSearchString.size(),
											 pcRow->m_cName, 0, m_cSearchString.size() ) == 0)
#endif
				{
					Select( i );
					MakeVisible( i, false );
					break;
				}
			}
		}
		m_nLastKeyDownTime = nTime;
	}
	else
	{
		switch( bytes[0] )
		{
			case B_DELETE:
			{
				std::vector<std::string> cPaths;
				for ( int i = GetFirstSelected() ; i <= GetLastSelected() ; ++i )
				{
					if ( IsSelected( i ) )
					{
						FileRow* pcRow = dynamic_cast<FileRow*>(GetRow(i));
						if ( pcRow != NULL )
						{
							BPath cPath = m_cPath;
							cPath.Append( pcRow->m_cName.c_str() );
							cPaths.push_back( cPath.Path() );
						}
					}
				}
				StartFileDelete( cPaths, BMessenger( this ) );
				break;
			}

			case B_BACKSPACE:
				m_cPath.Append( ".." );
				ReRead();
				PopState();
				DirChanged( m_cPath.Path() );
				break;
			
			case B_FUNCTION_KEY:
			{
				BLooper* pcLooper = Looper();
				assert( pcLooper != NULL );
				BMessage* pcMsg = pcLooper->CurrentMessage();
				assert( pcMsg != NULL );
				int32 nKeyCode;

				if ( pcMsg->FindInt32( "_raw_key", &nKeyCode ) != 0 )
				{
					return;
				}
				switch( nKeyCode )
				{
					case 6: // F5
						ReRead();
						break;
				}
				break;
			}
			default:
				BListView::KeyDown(bytes, numBytes);
				break;
		}
	}
}

int32 DirectoryView::ReadDirectory( void* pData )
{
	ReadDirParam* pcParam = (ReadDirParam*) pData;
	DirectoryView* pcView = pcParam->m_pcView;

	BWindow* pcWnd = pcView->Window();

	if ( pcWnd == NULL )
	{
		return(0);
	}

	pcWnd->Lock();
	pcView->Clear();
	pcWnd->Unlock();

fprintf( stderr, "DirectoryView::ReadDirectory: reading %s\n", pcView->GetPath().c_str() );
	DIR* pDir = opendir( pcView->GetPath().c_str() );
	if ( pDir == NULL )
	{
		printf( "Error: DirectoryView::ReadDirectory() Failed to open %s\n", pcView->GetPath().c_str() );
		goto error;
	}
	struct dirent* psEnt;

	while( pcParam->m_bCancel == false && (psEnt = readdir( pDir ) ) != NULL )
	{
fprintf( stderr, "\tfound entry %s\n", psEnt->d_name );
		struct stat sStat;
		if ( strcmp( psEnt->d_name, "." ) == 0 /*|| strcmp( psEnt->d_name, ".." ) == 0*/ )
		{
			continue;
		}
		BPath cFilePath( pcView->GetPath().c_str() );
		cFilePath.Append( psEnt->d_name );

		stat( cFilePath.Path(), &sStat );

		FileRow* pcRow = new FileRow( pcView->m_pcIconBitmap, psEnt->d_name, sStat );

		const char* pzBaseDir = getenv( "COSMOE_SYS" );
		if( pzBaseDir == NULL )
		{
			pzBaseDir = "/cosmoe";
		}
		char* pzPath = new char[ strlen(pzBaseDir) + 80 ];
		if ( S_ISDIR( sStat.st_mode ) == false )
		{
			strcpy( pzPath, pzBaseDir );
			strcat( pzPath, "/icons/file.icon" );
			load_icon( pzPath, pcRow->m_anIcon, false );
		}
		else
		{
			strcpy( pzPath, pzBaseDir );
			strcat( pzPath, "/icons/folder.icon" );
			load_icon( pzPath, pcRow->m_anIcon, false );
		}
		delete[] pzPath;

		pcWnd->Lock();
		pcView->InsertRow( pcRow );
		pcWnd->Unlock();
	}
	closedir( pDir );
error:
	pcWnd->Lock();
	pcView->Sort();
	if ( pcView->m_pcCurReadDirSession == pcParam )
	{
		pcView->m_pcCurReadDirSession = NULL;
	}
	pcWnd->Unlock();
	delete pcParam;
	return( 0 );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::ReRead()
{
fprintf( stderr, "DirectoryView::ReRead() called\n" );
/*
    BMessage cMsg( DirKeeper::M_CHANGE_DIR );
    cMsg.AddString( "path", m_cPath.Path() );
    m_pcDirKeeper->PostMessage( &cMsg );
*/

    if ( m_pcCurReadDirSession != NULL ) {
        m_pcCurReadDirSession->m_bCancel = true;
    }
    m_pcCurReadDirSession = new ReadDirParam( this );

/*
    thread_id hTread = spawn_thread( ReadDirectory, "read_dir_thread", 0, m_pcCurReadDirSession );
    if ( hTread >= 0 ) {
        resume_thread( hTread );
    }
*/
    ReadDirectory(m_pcCurReadDirSession);
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

std::string DirectoryView::GetPath() const
{
	return( std::string( m_cPath.Path() ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::SetPath( const std::string& cPath )
{
	while( m_cStack.size() > 0 )
	{
		m_cStack.pop();
	}
	m_cPath.SetTo( cPath.c_str() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::Invoked( int nFirstRow, int nLastRow )
{
	if ( nFirstRow != nLastRow )
	{
		BListView::Invoked( nFirstRow, nLastRow );
		return;
	}

	FileRow* pcRow = dynamic_cast<FileRow*>(GetRow( nFirstRow ));
	if ( pcRow == NULL )
	{
		BListView::Invoked( nFirstRow, nLastRow );
		return;
	}
	if ( S_ISDIR( pcRow->m_sStat.st_mode ) == false )
	{
		BListView::Invoked( nFirstRow, nLastRow );
		return;
	}
	bool bBack = false;

	if ( strcmp( pcRow->m_cName.c_str(), ".." ) == 0 )
	{
		m_cPath.Append( ".." );
		bBack = true;
	}
	else
	{
		m_cStack.push( State( this, m_cPath.Path() ) );
		m_cPath.Append( pcRow->m_cName.c_str() );
	}

	ReRead();

	if ( bBack )
	{
		PopState();
	}
	DirChanged( m_cPath.Path() );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::PopState()
{
	if ( m_cStack.empty() == false )
	{
		State& cState = m_cStack.top();

		for ( uint i = 0 ; i < cState.m_cSelection.size() ; ++i )
		{
			for ( uint j = 0 ; j < GetRowCount() ; ++j )
			{
				FileRow* pcRow = dynamic_cast<FileRow*>(GetRow(j));
				if ( pcRow != NULL )
				{
					if ( cState.m_cSelection[i] == pcRow->m_cName )
					{
						Select( j );
						MakeVisible( j );
						break;
					}
				}
			}
		}
		m_cStack.pop();
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::SetDirChangeMsg( BMessage* pcMsg )
{
	delete m_pcDirChangeMsg;
	m_pcDirChangeMsg = pcMsg;
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::DirChanged( const std::string& cNewPath )
{
	if ( m_pcDirChangeMsg != NULL )
	{
		Invoke( m_pcDirChangeMsg );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool DirectoryView::DragSelection( const BPoint& cPos )
{
	int nFirstSel = GetFirstSelected();
	int nLastSel  = GetLastSelected();
	int nNumRows  = nLastSel - nFirstSel + 1;

	float vRowHeight = GetRow( 0 )->Height( this ) + 3.0f;

	float vSelWidth = 0;

	BMessage cData(1234);
	for ( int i = nFirstSel ; i <= nLastSel ; ++i )
	{
		FileRow* pcRow = dynamic_cast<FileRow*>(GetRow(i));
		if ( pcRow != NULL && IsSelected( i ) )
		{
			float vLen = StringWidth( pcRow->m_cName.c_str() );
			if ( vLen > vSelWidth )
			{
				vSelWidth = vLen;
			}

			BPath cPath = m_cPath;
			cPath.Append( pcRow->m_cName.c_str() );
			cData.AddString( "file/path", cPath.Path() );
		}
	}

	vSelWidth += 18.0f;
	BRect cSelRect( 0, 0, vSelWidth - 1.0f, vRowHeight * nNumRows - 1 );
	BPoint cHotSpot( cPos.x, cPos.y - GetRowPos( nFirstSel ) );

	if ( nLastSel - nFirstSel < 4 )
	{
		BBitmap cImage( cSelRect, B_RGB32, true, false );
		BView* pcView = new BView( cSelRect, "", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW );
		cImage.AddChild( pcView );
		pcView->SetHighColor( 255, 255, 255, 255 );
		pcView->FillRect( cSelRect );

		BRect cBmRect( 0, 0, 15, 15 );
		BRect cTextRect( 18, 0, vSelWidth - 1.0f, vRowHeight - 1.0f );

		pcView->SetLowColor( 100, 100, 100 );
		for ( int i = nFirstSel ; i <= nLastSel ; ++i )
		{
			FileRow* pcRow = dynamic_cast<FileRow*>(GetRow(i));
			if ( pcRow != NULL && IsSelected( i ) )
			{
				pcView->SetDrawingMode( B_OP_OVER );
				pcRow->Draw( cBmRect, pcView, 0, false, false, false );
				pcView->SetDrawingMode( B_OP_COPY );
				pcRow->Draw( cTextRect, pcView, 6, false, false, false );
			}
			cBmRect.OffsetBy( 0.0f, vRowHeight );
			cTextRect.OffsetBy( 0.0f, vRowHeight );
		}
		cImage.Sync();
		DragMessage( &cData, &cImage, cHotSpot );
	}
	else
	{
		DragMessage( &cData, cHotSpot, cSelRect );
	}

	return( true );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::MessageReceived( BMessage* pcMessage )
{
	switch( pcMessage->what )
	{
		case M_CLEAR:
			Clear();
			break;

		case M_ADD_ENTRY:
		{
//            std::string cName;
			const char* pzName;
			const struct stat* psStat;
			ssize_t nSize;
			int32 nCount = 0;
			pcMessage->GetInfo( "stat", NULL, &nCount );
			for ( int i = 0 ; i < nCount ; ++i )
			{
				if ( pcMessage->FindString( "name", i, &pzName ) != 0 )
				{
					break;
				}
				if ( pcMessage->FindData( "stat", B_ANY_TYPE, i, (const void**) &psStat, &nSize ) != 0 )
				{
					break;
				}
				try {
					FileRow* pcRow = new FileRow( m_pcIconBitmap, pzName, *psStat );

					const void* pIconData;
					if ( pcMessage->FindData( "small_icon", B_ANY_TYPE, i, &pIconData, &nSize ) == 0 )
					{
						memcpy( pcRow->m_anIcon, pIconData, 16 * 16 * 4 );
					}
					InsertRow( pcRow );
				} catch( std::exception& )
				{
				}
			}
			m_pcDirKeeper->PostMessage( DirKeeper::M_ENTRIES_ADDED, m_pcDirKeeper, m_pcDirKeeper );
			break;
		}
		case M_UPDATE_ENTRY:
		{
			const char* pzName;
			const struct stat* psStat;
			ssize_t nSize;
			int32 nCount = 0;
			pcMessage->GetInfo( "stat", NULL, &nCount );
			for ( int i = 0 ; i < nCount ; ++i )
			{
				if ( pcMessage->FindString( "name", i, &pzName ) != 0 )
				{
					break;
				}

				if ( pcMessage->FindData( "stat", B_ANY_TYPE, i, (const void**) &psStat, &nSize ) != 0 )
				{
					break;
				}

				try {
					for ( BListView::const_iterator j = begin() ; j != end() ; ++j )
					{
						FileRow* pcRow = static_cast<FileRow*>(*j);
						if ( pcRow->m_sStat.st_ino == psStat->st_ino && pcRow->m_sStat.st_dev == psStat->st_dev )
						{
							const void* pIconData;
							pcRow->m_cName = pzName;
							pcRow->m_sStat = *psStat;
							if ( pcMessage->FindData( "small_icon", i, B_ANY_TYPE, &pIconData, &nSize ) == 0 )
							{
								memcpy( pcRow->m_anIcon, pIconData, 16 * 16 * 4 );
								InvalidateRow( j - begin(), BListView::INV_VISUAL );
							}
							break;
						}
//                        RemoveRow( j - begin() );
//                        InsertRow( pcRow );
					}
				} catch( std::exception& )
				{
				}
			}
			m_pcDirKeeper->PostMessage( DirKeeper::M_ENTRIES_UPDATED, m_pcDirKeeper, m_pcDirKeeper );
			break;
		}
		case M_REMOVE_ENTRY:
		{
			dev_t nDevice;
			int64 nInode;

			int32 nCount = 0;
			pcMessage->GetInfo( "device", NULL, &nCount );

			for ( int i = 0 ; i < nCount ; ++i )
			{
				pcMessage->FindInt32( "device", i, (int32*)&nDevice );
				pcMessage->FindInt64( "node", i, &nInode );

				for ( BListView::const_iterator j = begin() ; j != end() ; ++j )
				{
					FileRow* pcRow = static_cast<FileRow*>(*j);
					if ( int64(pcRow->m_sStat.st_ino) == nInode &&
						pcRow->m_sStat.st_dev == nDevice )
					{
						delete RemoveRow( j - begin() );
						break;
					}
				}
			}
			m_pcDirKeeper->PostMessage( DirKeeper::M_ENTRIES_REMOVED, m_pcDirKeeper, m_pcDirKeeper );
			break;
		}

		case 125:
			ReRead();

		default:
			BListView::MessageReceived( pcMessage );
	}
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void FileRow::Draw( const BRect& cFrame, BView* pcView, uint nColumn,
                     bool bSelected, bool bHighlighted, bool bHasFocus )
{
	if ( nColumn == 0 )
	{
		pcView->Sync(); // Make sure the previous icon is rendered before we overwrite the bitmap
		memcpy( m_pcIconBitmap->Bits(), m_anIcon, 16*16*4 );
//    ConvertIcon( m_pcIconBitmap, m_anIcon, false );
		pcView->DrawBitmap( m_pcIconBitmap, BRect(0, 0, 15.0f, 15.0f ), cFrame );
		return;
	}

	font_height sHeight;
	pcView->GetFontHeight( &sHeight );

//  if ( bHighlighted ) {
//    pcView->SetHighColor( 0, 50, 200 );
//  } else if ( bSelected ) {
//    pcView->SetHighColor( 0, 0, 0 );
//  } else {
	pcView->SetHighColor( 255, 255, 255 );
//  }
	if ( nColumn != 6 )
	{
		pcView->FillRect( cFrame );
	}

	float vFontHeight = sHeight.ascent + sHeight.descent;
	float vBaseLine = cFrame.top + (cFrame.Height()+1.0f) / 2 - vFontHeight / 2 + sHeight.ascent;
	pcView->MovePenTo( cFrame.left + 3, vBaseLine );

	char  zBuffer[256];
	const char* pzString = zBuffer;

	switch( nColumn )
	{
		case 1:        // name
			pzString = m_cName.c_str();
			break;
		case 2:        // size
			if ( S_ISDIR( m_sStat.st_mode ) )
			{
				strcpy( zBuffer, "<DIR>" );
			}
			else
			{
				sprintf( zBuffer, "%ld", m_sStat.st_size );
			}
			break;
		case 3:        // attributes
			for ( int i = 0 ; i < 10 ; ++i )
			{
				if ( m_sStat.st_mode & (1<<i) )
				{
					zBuffer[i] = "drwxrwxrwx"[9-i];
				}
				else
				{
					zBuffer[i] = '-';
				}
			}
			zBuffer[10] = '\0';
			break;
		case 4:        // date
		{
			time_t nTime = m_sStat.st_ctime;
			strftime( zBuffer, 256, "%d/%b/%Y", localtime( &nTime ) );
			break;
		}
		case 5:        // time
		{
			time_t nTime = m_sStat.st_ctime;
			strftime( zBuffer, 256, "%H:%M:%S", localtime( &nTime ) );
			break;
		}
		case 6:        // name (for drag image)
			pzString = m_cName.c_str();
			break;
		default:
			printf( "Error: FileRow::Draw() - Invalid column %d\n", nColumn );
			return;
	}
/*
if ( bHighlighted || (bSelected && bHasFocus) ) {
pcView->SetHighColor( 255, 255, 255 );
} else {
pcView->SetHighColor( 0, 0, 0 );
}
*/
	if ( bHighlighted && nColumn == 1 )
	{
		pcView->SetHighColor( 255, 255, 255 );
		pcView->SetLowColor( 0, 50, 200 );
	}
	else if ( bSelected && nColumn == 1 )
	{
		pcView->SetHighColor( 255, 255, 255 );
		pcView->SetLowColor( 0, 0, 0 );
	}
	else
	{
		pcView->SetLowColor( 255, 255, 255 );
		pcView->SetHighColor( 0, 0, 0 );
	}

	if ( bSelected && nColumn == 1 )
	{
		rgb_color aColor = { 0, 0, 0, 0 };
		BRect cRect = cFrame;
		cRect.right = cRect.left + pcView->StringWidth( pzString ) +          4;
		cRect.top = vBaseLine - sHeight.ascent - 1;
		cRect.bottom = vBaseLine + sHeight.descent + 1;
		pcView->FillRect( cRect, aColor );
	}
	pcView->DrawString( pzString );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::MouseUp( BPoint cPosition )
{
	Highlight( 0, GetRowCount() - 1, false, false );
	
	BMessage pcData((uint32)0);
	
	// If this wasn't a drag, just leave now
	
	if ( Window()->CurrentMessage()->FindMessage( "_drag_message", &pcData ) != B_OK )
	{
		BListView::MouseUp( cPosition );
		return;
	}
	
	StopScroll();

	const char* pzPath;
	if ( pcData.FindString( "file/path", &pzPath ) != 0 )
	{
		return;
	}

	BPath	temp;
	BPath(pzPath).GetParent(&temp);
	if ( m_cPath == temp )
	{
		return;
	}

	FileRow* pcRow;

	int nSel = HitTest( cPosition );
	if ( nSel != -1 )
	{
		pcRow = dynamic_cast<FileRow*>(GetRow(nSel));
	}
	else
	{
		pcRow = NULL;
	}

	BPath cDstDir;

	if ( pcRow != NULL && S_ISDIR( pcRow->m_sStat.st_mode ) )
	{
		cDstDir = m_cPath;
		cDstDir.Append( pcRow->GetName().c_str() );
	}
	else
	{
		cDstDir = m_cPath;
	}

	std::vector<std::string> cSrcPaths;
	std::vector<std::string> cDstPaths;
	for ( int i = 0 ; pcData.FindString( "file/path", i, &pzPath ) == 0 ; ++i )
	{
		BPath cSrcPath( pzPath );
		BPath cDstPath = cDstDir;
		cDstPath.Append( cSrcPath.Leaf() );
		cSrcPaths.push_back( pzPath );
		cDstPaths.push_back( cDstPath.Path() );
	}
	printf( "Start copy\n" );
	StartFileCopy( cDstPaths, cSrcPaths, BMessenger( this ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

void DirectoryView::MouseMoved( BPoint cNewPos, uint32 nCode, const BMessage* pcData )
{
	if ( pcData == NULL )
	{
		BListView::MouseMoved( cNewPos, nCode, NULL );
		return;
	}

	if ( nCode == B_OUTSIDE_VIEW )
	{
		return;
	}

	if ( nCode == B_EXITED_VIEW )
	{
		Highlight( 0, GetRowCount() - 1, false, false );
		StopScroll();
		return;
	}

	const char* pzPath;
	if ( pcData->FindString( "file/path", &pzPath ) != 0 )
	{
		return;
	}

	BRect cBounds = Bounds();
	if ( cNewPos.y < cBounds.top + AUTOSCROLL_BORDER )
	{
		StartScroll( SCROLL_DOWN, false );
	}
	else if ( cNewPos.y > cBounds.bottom - AUTOSCROLL_BORDER )
	{
		StartScroll( SCROLL_UP, false );
	}
	else
	{
		StopScroll();
	}

	int nSel = HitTest( cNewPos );
	if ( nSel != -1 )
	{
		FileRow* pcRow = dynamic_cast<FileRow*>(GetRow(nSel));
		if ( pcRow != NULL && S_ISDIR( pcRow->m_sStat.st_mode ) )
		{
			BPath cRowPath = m_cPath;
			cRowPath.Append( pcRow->GetName().c_str() );
			if ( cRowPath  != BPath(pzPath) )
			{
				Highlight( nSel, true, true );
				return;
			}
		}
	}

	Highlight( 0, GetRowCount() - 1, false, false );
}

void FileRow::AttachToView( BView* pcView, int nColumn )
{
	if ( nColumn == 0 )
	{
		m_avWidths[0] = 16.0f;
		return;
	}

	char  zBuffer[256];
	const char* pzString = zBuffer;

	switch( nColumn )
	{
		case 1:        // name
			pzString = m_cName.c_str();
			break;
		case 2:        // size
			if ( S_ISDIR( m_sStat.st_mode ) )
			{
				strcpy( zBuffer, "<DIR>" );
			}
			else
			{
				sprintf( zBuffer, "%ld", m_sStat.st_size );
			}
			break;
		case 3:        // attributes
			for ( int i = 0 ; i < 10 ; ++i )
			{
				if ( m_sStat.st_mode & (1<<i) )
				{
					zBuffer[i] = "drwxrwxrwx"[9-i];
				}
				else
				{
					zBuffer[i] = '-';
				}
			}
			zBuffer[10] = '\0';
			break;
		case 4:        // date
		{
			time_t nTime = m_sStat.st_ctime;
			strftime( zBuffer, 256, "%d/%b/%Y", localtime( &nTime ) );
			break;
		}
		case 5:        // time
		{
			time_t nTime = m_sStat.st_ctime;
			strftime( zBuffer, 256, "%H:%M:%S", localtime( &nTime ) );
			break;
		}
		case 6:        // name (for drag image)
			pzString = m_cName.c_str();
			break;
		default:
			printf( "Error: FileRow::AttachToView() - Invalid column %d\n", nColumn );
			return;
	}
	m_avWidths[nColumn] = pcView->StringWidth( pzString ) + 5.0f;
}

void FileRow::SetRect( const BRect& cRect, int nColumn )
{
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

float FileRow::Width( BView* pcView, int nColumn )
{
	return( m_avWidths[nColumn] );
/*    if ( nColumn == 0 ) {
        return( 16.0f );
    }

    char  zBuffer[256];
    const char* pzString = zBuffer;
  
    switch( nColumn )
    {
        case 1:        // name
            pzString = m_cName.c_str();
            break;
        case 2:        // size
            if ( S_ISDIR( m_sStat.st_mode ) ) {
                strcpy( zBuffer, "<DIR>" );
            } else {
                sprintf( zBuffer, "%Ld", m_sStat.st_size );
            }
            break;
        case 3:        // attributes
            for ( int i = 0 ; i < 10 ; ++i ) {
                if ( m_sStat.st_mode & (1<<i) ) {
                    zBuffer[i] = "drwxrwxrwx"[9-i];
                } else {
                    zBuffer[i] = '-';
                }
            }
            zBuffer[10] = '\0';
            break;
        case 4:        // date
        {
            time_t nTime = m_sStat.st_ctime;
            strftime( zBuffer, 256, "%d/%b/%Y", localtime( &nTime ) );
            break;
        }
        case 5:        // time
        {
            time_t nTime = m_sStat.st_ctime;
            strftime( zBuffer, 256, "%H:%M:%S", localtime( &nTime ) );
            break;
        }
        case 6:        // name (for drag image)
            pzString = m_cName.c_str();
            break;
        default:
            printf( "Error: FileRow::Draw() - Invalid column %d\n", nColumn );
            return( 0.0f );
    }
    return( pcView->StringWidth( pzString ) + 5.0f );*/
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

float FileRow::Height( BView* pcView )
{
	font_height sHeight;
	pcView->GetFontHeight( &sHeight );

	return( max_c( 16.0f - 3.0f, sHeight.ascent + sHeight.descent ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool FileRow::HitTest( BView* pcView, const BRect& cFrame, int nColumn, BPoint cPos )
{
	if ( nColumn != 1 )
	{
		return( true );
	}

	font_height sHeight;
	pcView->GetFontHeight( &sHeight );
	float nFontHeight = sHeight.ascent + sHeight.descent;
	float nBaseLine = cFrame.top + (cFrame.Height()+1.0f) * 0.5f - nFontHeight / 2 + sHeight.ascent;
	BRect cRect( cFrame.left,
				 nBaseLine - sHeight.ascent - 1.0f,
				 cFrame.left + pcView->StringWidth( m_cName.c_str() ) + 4.0f,
				 nBaseLine + sHeight.descent + 1.0f );
	return( cRect.Contains( cPos ) );
}

//----------------------------------------------------------------------------
// NAME:
// DESC:
// NOTE:
// SEE ALSO:
//----------------------------------------------------------------------------

bool FileRow::IsLessThan( const BListItem* pcOther, uint nColumn ) const
{
	const FileRow* pcRow = dynamic_cast<const FileRow*>(pcOther);
	if ( NULL == pcRow )
	{
		return( false );
	}

	if ( S_ISDIR( m_sStat.st_mode ) != S_ISDIR( pcRow->m_sStat.st_mode ) )
	{
		return( S_ISDIR( m_sStat.st_mode ) );
	}

	switch( nColumn )
	{
		case 0:        // icon
		case 1:        // name
			return( strcasecmp( m_cName.c_str(), pcRow->m_cName.c_str() ) < 0 );
		case 2:        // size
			return( m_sStat.st_size < pcRow->m_sStat.st_size );
		case 3:        // attributes
			return( strcasecmp( m_cName.c_str(), pcRow->m_cName.c_str() ) < 0 );
		case 4:        // date
		case 5:        // time
			return( m_sStat.st_mtime < pcRow->m_sStat.st_mtime );
		default:
			printf( "Error: FileRow::IsLessThan() - Invalid column %d\n", nColumn );
			return( false );
	}
}

