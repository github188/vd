/*
 * MutexGuard.h
 *
 *  Created on: 2013-3-21
 *      Author: shanhongwei
 */

#ifndef MUTEX_GUARD_H_
#define MUTEX_GUARD_H_

#include <pthread.h>

/*
 *
 */
class mutex_guard
{
	public:
		mutex_guard(pthread_mutex_t& mutex);
		virtual ~mutex_guard();
	private:
		pthread_mutex_t& m_mutex;
};

#endif /* MUTEX_GUARD_H_ */
