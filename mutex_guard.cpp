/*
 * mutex_guard.cpp
 *
 *  Created on: 2013-3-21
 *      Author: shanhongwei
 */

#include <stdio.h>
#include "mutex_guard.h"

mutex_guard::mutex_guard(pthread_mutex_t& mutex) :
	m_mutex(mutex)
{
	pthread_mutex_lock(&m_mutex);
	//printf("mutex lock\n");
}

mutex_guard::~mutex_guard()
{
	//printf("mutex unlock\n");
	pthread_mutex_unlock(&m_mutex);
}
