#include <stdlib.h>
#include <errno.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "vapor/EasyThreads.h"

#include <cassert>
using namespace VetsUtil;

EasyThreads::EasyThreads(
	int	nthreads
) {
#ifndef ENABLE_THREADS
	
#else
	int	rc;
	threads_c = NULL;
	if (nthreads < 1) nthreads = NProc();
#endif
	

	nthreads_c = nthreads;
	
	block_c = 0;
	count_c = 0;
#ifdef ENABLE_THREADS
	rc = pthread_attr_init(&attr_c);
	if (rc < 0) {
		SetErrMsg("pthread_attr_init() : %s", strerror(errno));
		return;
	}

	rc = pthread_cond_init(&cond_c, NULL);
	if (rc < 0) {
		SetErrMsg("pthread_cond_init() : %s", strerror(errno));
		return;
	}

	rc = pthread_mutex_init(&barrier_lock_c, NULL);
	if (rc < 0) {
		SetErrMsg("pthread_mutex_init() : %s", strerror(errno));
		return;
	}

	rc = pthread_mutex_init(&mutex_lock_c, NULL);
	if (rc < 0) {
		SetErrMsg("pthread_mutex_init() : %s", strerror(errno));
		return;
	}

	pthread_attr_setdetachstate(&attr_c,PTHREAD_CREATE_JOINABLE);
	if (rc < 0) {
		SetErrMsg("pthread_attr_setdetachstate() : %s",strerror(errno));
		return;
	}

#ifdef __sgi
	rc = pthread_attr_setscope(&attr_c,PTHREAD_SCOPE_BOUND_NP);
#else
	rc = pthread_attr_setscope(&attr_c,PTHREAD_SCOPE_SYSTEM);
#endif
	if (rc < 0) {
		SetErrMsg("pthread_attr_setscope() : %s", strerror(errno));
		return;
	}

	threads_c = new pthread_t[nthreads_c];
#endif //ENABLE_THREADS
}

EasyThreads::~EasyThreads(
) {
#ifdef ENABLE_THREADS
	pthread_attr_destroy(&attr_c);
	if (threads_c) delete [] threads_c;
	threads_c = NULL;
#endif
}

int	EasyThreads::ParRun(
	void *(*start)(void *), 
	void **arg
) {
#ifdef ENABLE_THREADS
	int	i;
	int	rc;
	int	status = 0;

	for(i=0; i<nthreads_c; i++) {
		rc = pthread_create(&threads_c[i], &attr_c, start, arg[i]);
		if (rc < 0) {
			SetErrMsg("pthread_create() : %s", strerror(errno));
			return(-1);
		}
	}

	for(i=0; i<nthreads_c; i++) {
		rc = pthread_join(threads_c[i], NULL);
		if (rc < 0) {
			SetErrMsg("pthread_join() : %s", strerror(errno));
			status = rc;
		}
	}
	return(status);
#else
	return 0;
#endif
}

int	EasyThreads::Barrier()
{
#ifdef ENABLE_THREADS
	int	local;
	int	rc;

	if(nthreads_c>1) {
		rc = pthread_mutex_lock(&barrier_lock_c);
		if (rc < 0) {
			SetErrMsg("pthread_mutex_lock() : %s", strerror(errno));
			return(-1);
		}
		local=count_c;
		block_c++;
		if(block_c==nthreads_c) {
			block_c=0;
			count_c++;
			rc = pthread_cond_broadcast(&cond_c);
			if (rc < 0) {
				SetErrMsg("pthread_cond_broadcast() : %s", strerror(errno));
				return(-1);
			}
		}
		while(local==count_c) {
			rc = pthread_cond_wait(&cond_c,&barrier_lock_c);
			if (rc < 0) {
				SetErrMsg("pthread_cond_wait() : %s", strerror(errno));
				return(-1);
			}
		}
		rc = pthread_mutex_unlock(&barrier_lock_c);
		if (rc < 0) {
			SetErrMsg("pthread_mutex_unlock() : %s", strerror(errno));
			return(-1);
		}
	}
#endif //ENABLE_THREADS
	return(0);
}

int	EasyThreads::MutexLock()
{
#ifdef ENABLE_THREADS
	if(nthreads_c>1) {
		int rc = pthread_mutex_lock(&mutex_lock_c);
		if (rc < 0) {
			SetErrMsg("pthread_mutex_lock() : %s", strerror(errno));
			return(-1);
		}
	}
#endif
	return(0);
}

int	EasyThreads::MutexUnlock()
{
#ifdef ENABLE_THREADS
	if(nthreads_c>1) {
		int rc = pthread_mutex_unlock(&mutex_lock_c);
		if (rc < 0) {
			SetErrMsg("pthread_mutex_unlock() : %s", strerror(errno));
			return(-1);
		}
	}
#endif
	return(0);
}

void	EasyThreads::Decompose(
	int n, 
	int size, 
	int rank, 
	int *offset, 
	int *length
) {
	int remainder=n%size;
	int chunk=n/size;

	if(rank<remainder) {
		*length=chunk+1;
		*offset=(chunk+1)*rank;
	}
	else {
		*length=chunk;
		*offset=(chunk+1)*remainder+(rank-remainder)*chunk;
	}
}
int	EasyThreads::NProc() 
{
#ifdef ENABLE_THREADS
#ifdef	__sgi
	return(sysconf(_SC_NPROC_ONLN));
#else
	return(sysconf(_SC_NPROCESSORS_ONLN));
#endif
#else
	return 1;
#endif //ENABLE_THREADS
}
