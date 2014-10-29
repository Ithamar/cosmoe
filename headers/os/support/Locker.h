//
//	$Id: Locker.h,v 1.1 2002/07/09 12:24:33 ejakowatz Exp $
//
//	This is the BLocker interface for OpenBeOS.  It has been created to
//	be source and binary compatible with the BeOS version of BLocker.
//


#ifndef	_OPENBEOS_LOCKER_H
#define	_OPENBEOS_LOCKER_H


#include <OS.h>
#include <SupportDefs.h>
#include <Debug.h>


#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

class BLocker {
public:
	BLocker();
	BLocker(const char *name);
	BLocker(bool benaphore_style);
	BLocker(const char *name, bool benaphore_style);
	
	// The following constructor is not documented in the BeBook
	// and is only listed here to ensure binary compatibility.
	// DO NOT USE THIS CONSTRUCTOR!
	BLocker(const char *name, bool benaphore_style, bool);

	virtual ~BLocker();

	bool Lock(void);
	status_t LockWithTimeout(bigtime_t timeout);
	void Unlock(void);

	thread_id LockingThread(void) const;
	bool IsLocked(void) const;
	int32 CountLocks(void) const;
	int32 CountLockRequests(void) const;
	sem_id Sem(void) const;


private:
	void InitLocker(const char *name, bool benaphore_style);
	bool AcquireLock(bigtime_t timeout, status_t *error);

	int32 fBenaphoreCount;
	sem_id fSemaphoreID;
	thread_id fLockOwner;
	int32 fRecursiveCount;
	
	// Reserved space for future changes to BLocker
	int32 fReservedSpace[4];
};


/**
 * \ingroup support
 * \par Description:
 *
 * \sa
 * \author	Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Gate
{
public:
	Gate(const char* name)
	{
	    sem = create_sem(100000000, name);
		if (!sem)
			debugger("FATAL: Gate could not be created!\n");
	}
	~Gate()				{ delete_sem( sem );			}
	int Lock() const	{ return( acquire_sem( sem ) );	}
	int Unlock() const	{ return( release_sem( sem ) );	}

	int Close() const	{ return( acquire_sem_etc( sem, 100000000, 0, B_INFINITE_TIMEOUT ) ); }
	int Open() const	{ return( release_sem_etc( sem, 100000000, 0 ) ); }
  
private:
	sem_id				sem;
};


#ifdef USE_OPENBEOS_NAMESPACE
}
#endif

#endif // _OPENBEOS_LOCKER_H
