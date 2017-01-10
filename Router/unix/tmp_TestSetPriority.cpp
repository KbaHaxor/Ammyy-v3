

bool stopped = true;

LPVOID WorkerThread(LPVOID p)
{
	int& i = *((int*)p);
	while(stopped);
	while(!stopped) {
		i++;
		//usleep(1);
	}
	return 0;
}


int main()
{
	const int COUNT_THREAD = 3;

	unsigned long thread[COUNT_THREAD];
	unsigned int  a[COUNT_THREAD];

	for (int i=0; i<COUNT_THREAD; i++)
	{
		thread[i] = 0;
		a[i] = 0;	

		pthread_attr_t attribs = {};
		pthread_attr_init(&attribs);
		//attribs.stacksize = 10000;
		pthread_attr_setschedpolicy(&attribs, SCHED_RR);
		struct sched_param taskschedparam;
		taskschedparam.__sched_priority = (i==1) ? 1 : 50;
		pthread_attr_setschedparam(&attribs, &taskschedparam);

		int err = pthread_create(&thread[i], &attribs, WorkerThread, &a[i]);
		if (err) printf("pthread_create(i=%d) error=%d", i, err);
	}

	struct sched_param sp = { };
	sp. sched_priority = 1;
	const int res = ::pthread_setschedparam (thread[1], SCHED_RR, & sp);

	sp. sched_priority = 50;
	::pthread_setschedparam (thread[0], SCHED_RR, & sp);
	::pthread_setschedparam (thread[2], SCHED_RR, & sp);

	printf("set before\n");
	//int err1 = pthread_getschedprio(thread[1]);
	//int err2 = 0; //pthread_setschedprio(SCHED_RR, 99);
	//printf("set %d %d\n", err1, err2);


	stopped = false;

	//err = ::pthread_getschedparam (m_thread2, SCHED_OTHER, & sp);

	const int DIVIDER = 1000000;
	
	for (int i=0; i<30; i++) {
		printf("-%u-%u-%u-\n", a[0]/DIVIDER, a[1]/DIVIDER, a[2]/DIVIDER);
		usleep(1000000);
	}

	stopped = true;
	
	printf("Ended\n");

	return 0;
}