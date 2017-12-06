/*
 * task.h
 *
 *  Created on: Mar 3, 2014
 *      Author: zdravko
 */

#ifndef TASK_H_
#define TASK_H_

#include <iostream> // because of writing
#include <stdio.h>
#include <ctime> // for the timestamp() function
#include "utility.h"

class Task
{
	private:
		int comp_amount;
		int id;
		void execute_load(int); // called by compute()
	public:
		Task ();
		void setID(int); // set id
		int getID(); // return id
		int getProcessID(); // return rank of the process running this task
		void waitUnlocksViaRecv(int **commMatrix, int **schedule, double stamp);
		void sendUnlocks(int **commMatrix, int **schedule, MPI_Request& request, double);
		void communicate(int **commMatrix, int **schedule, int *comDataChunk, double stamp); // comm phase (MIXED GRAPH ONLY)
		void recvCommDAG(int **commMatrix, int **schedule, int *comDataChunk, double stamp); // comm phase (DAG ONLY)
		void sendCommDAG(int **commMatrix, int **schedule, int *comDataChunk, double stamp); // comm phase (DAG ONLY)
		void compute(int **compAmounts, double stamp); // compute phase
		void comp_bench(int mpi_rank, double*);
		void comm_bench(int mpi_rank, int mpi_size, int* comm_chunk, int comm_chunk_size, double*, double*);
		void load_data(int); // load from storage [FUTURE]
		void store_data(int); // store to storage [FUTURE]
};

/****
 * Initialize computation load if no load is given
 * ****/
Task::Task()
{
	comp_amount = 1000;
	id=0;
}

/**** Function which sets object (Task) id
 * (identifies from the allocation vector)
 * ****/
void Task::setID(int id)
{
	this->id=id;
}

/**** Function which returns the object (Task) id
 * ****/
int Task::getID()
{
	return this->id;
}

/**** Returning rank of the process executing task
 * not in use still
 * ****/
int Task::getProcessID()
{
	//return MPI::COMM_WORLD.Get_rank();
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	return rank;
}

/**** Receive unlock
 * TODO: Only for MIXED GRAPH for now
 * Receive all precedence constraints by a task calling this function
 * Checks column of an alloc matrix, if a value is 0 this means that task depends
 * on the task corresponding to the row number,
 * then receive prec from the process assigned to the precedence task
 *
 * This method should be invoked per task!
 * ****/
void Task::waitUnlocksViaRecv(int **commMatrix, int **schedule, double stamp)
{
	int dummy = 0; //variable received as a precedence semaphore
	int taskID = this->getID();
	for (int i = 0; i < numOfTasks; i++)
	{
		if (commMatrix[i][taskID] == 0)
		{
			printTimestamp(stamp);
			int hpuOfPrecedentTask = findHPUForTaskInSchedule(schedule, numOfHPUs, maxNumOfTasksInSchedule, i);
/*DEBUG*/std::cout << "\tP" << this->getProcessID() << ":\tT" << taskID << ": waits for unlock from T" << i << " (P" << hpuOfPrecedentTask << ")" << std::endl;
			if ( this->getProcessID() != hpuOfPrecedentTask ) // receive unlock only if sent from another process (not to receive from itself)
				MPI_Recv(&dummy, 1, MPI_INT, hpuOfPrecedentTask, taskID, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			printTimestamp(stamp);
/*DEBUG*/std::cout << "\tP" << this->getProcessID() << ":\tT" << taskID << ": got unlocked by T" << i << " (P" << hpuOfPrecedentTask << ")" << std::endl;
		}
	}
}

/**** Send unlock
 * TODO: Only for MIXED GRAPH for now
 * Send every successor a signal that his precedence is unlocked by a task calling this function
 * Checks column of an alloc matrix, if a value is 0 this means that task depends
 * on the task corresponding to the row number,
 * then receive prec from the PROCESS assigned to the precedence task
 * ****/
void Task::sendUnlocks(int **commMatrix, int **schedule, MPI_Request& request, double stamp)
{
	int dummy = 0; //variable sent as a precedence semaphore
	int taskID = this->getID();
	for (int i = 0; i < numOfTasks; i++)
	{
		if (commMatrix[taskID][i] == 0)
		{
			printTimestamp(stamp);
			int hpuOfChildTask = findHPUForTaskInSchedule(schedule, numOfHPUs, maxNumOfTasksInSchedule, i);
/*DEBUG*/	std::cout << "\tP" << ":\tT" << taskID << ": unlocking T" << i << " (P" << hpuOfChildTask << ")" << std::endl;
			if (this->getProcessID() != hpuOfChildTask) // send unlock only to another process (not to send to itself)
				MPI_Isend(&dummy, 1, MPI_INT, hpuOfChildTask, i, MPI_COMM_WORLD, &request);
			printTimestamp(stamp);
/*DEBUG*/std::cout << "\tP" << this->getProcessID() << ":\tT" << taskID << ": unlocked T" << i << " (P" << hpuOfChildTask << ")" << std::endl;
		}
	}
}

/****Communication phase of a task
 * TODO: Only for MIXED GRAPH for now
 * ASSUMPTION: the task computes first, than communicates.
 * 				Having communication started first (non-blocking) and then computing would be much better
 * 				But that depends on what we want to show.
 * SEND if lower number than one inspected
 * RECEIVE if higher number than one inspected
 * TODO: The problem is that the comm amount is double:
 * 			The whole part determines how many times we send a 10MB data chunk
 * 			The decimal part has to be converted so that only a fraction of 10MB is sent
 * TODO: NOT FINISHED! A lot to think about here:
 * 						Start all comm (non-blocking) or
 * 						Start all com (blocking) or
 * 						Start certain comm (blocking or not)
 * 						etc...
 * ****/
void Task::communicate(int **commMatrix, int **schedule, int *commDataChunk, double stamp)
{
//	int taskID = this->getID();
//	for (int i=0; i<numOfTasks; i++)
//	{
//		if (commMatrix[taskID][i] > 0) // > 0 is comm
//		{
//			if (taskID < i)
//			{
//				printTimestamp(stamp);
//	/*DEBUG*/	std::cout << "\tT" << taskID << " start send to T" << i << std::endl;
//				int hpuOfReceiverTask = findHPUForTaskInSchedule(schedule, numOfHPUs, maxNumOfTasksInSchedule, i);
//
//				// create enough request handlers
//				int commWholePart = (long)commMatrix[taskID][i];
//				float commDecimalPart = commMatrix[taskID][i] - (long)commMatrix[taskID][i];
//				MPI_Request *commSendRequests = new MPI_Request[commWholePart + 1]; // N sends for whole parts + 1 send for decimal part
//				// send the amount of data necessary (the whole)
//				for (std::size_t j = 0; j < commWholePart; j++)
//					MPI_Isend(commDataChunk, COMM_CHUNK_SIZE, MPI_INT, hpuOfReceiverTask, j, MPI_COMM_WORLD, &commSendRequests[j]);
//				// send the decimal part
//				MPI_Isend(commDataChunk, (long)((commDecimalPart)*COMM_CHUNK_SIZE), MPI_INT, hpuOfReceiverTask, commWholePart, MPI_COMM_WORLD, &commSendRequests[commWholePart]);
//
//
//
//				printTimestamp(stamp);
//	/*DEBUG*/	std::cout << "\tT" << taskID << " finish send to T" << i << std::endl;
//			}
//			else if (taskID > i) // we don't take into account the elements on a main diagonal, or?
//			{
//				printTimestamp(stamp);
//	/*DEBUG*/	std::cout << "\tT" << taskID << " start receive from T" << i << std::endl;
//				int hpuOfSenderTask = findHPUForTaskInSchedule(schedule, numOfHPUs, maxNumOfTasksInSchedule, i);
//				MPI_Recv(commDataChunk, commMatrix[taskID][i], MPI_INT, hpuOfSenderTask, taskID, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
//				printTimestamp(stamp);
//	/*DEBUG*/	std::cout << "\tT" << taskID << " finish receive from T" << i << std::endl;
//			}
//		}
//	}
}

/**** Receive comm and unlock
 * For DAG receiving comm is also an unlock
 *
 * TODO: Only for DAG for now
 * Receive all precedence constraints by a task calling this function
 * Checks column of an alloc matrix, if a value is 0 this means that task depends
 * on the task corresponding to the row number,
 * then receive prec from the process assigned to the precedence task
 *
 * TODO: rotate receives from all predecessors (to receive data ASAP)
 * 		 (when waiting for unlocks only,  the order of receives is not relevant (because receive time is negligible))
 * This method should be invoked per task!
 * ****/
void Task::recvCommDAG(int **commMatrix, int **schedule, double stamp)
{
	int taskID = this->getID();
	for (int i = 0; i < numOfTasks; i++)
	{
		if (commMatrix[i][taskID] > -1)
		{
			printTimestamp(stamp);
			int hpuOfPrecedentTask = findHPUForTaskInSchedule(schedule, numOfHPUs, maxNumOfTasksInSchedule, i);
			/*DEBUG*/std::cout << "\tP" << this->getProcessID() << ":\tT" << taskID << ": waits for comm & unlock from T" << i << " (P" << hpuOfPrecedentTask << ")" << std::endl;
			if ( this->getProcessID() != hpuOfPrecedentTask ) // receive data&unlock only if sent from another process (not to receive from itself)
				MPI_Recv(&dummy, 1, MPI_INT, hpuOfPrecedentTask, taskID, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			printTimestamp(stamp);
/*DEBUG*/std::cout << "\tP" << this->getProcessID() << ":\tT" << taskID << ": got unlocked by T" << i << " (P" << hpuOfPrecedentTask << ")" << std::endl;
		}
	}
}

/**** Send unlock
 * TODO: Only for MIXED GRAPH for now
 * Send every successor a signal that his precedence is unlocked by a task calling this function
 * Checks column of an alloc matrix, if a value is 0 this means that task depends
 * on the task corresponding to the row number,
 * then receive prec from the PROCESS assigned to the precedence task
 * ****/
void Task::sendCommDAG(int **commMatrix, int **schedule, MPI_Request& request, double stamp)
{
	int dummy = 0; //variable sent as a precedence semaphore
	int taskID = this->getID();
	for (int i = 0; i < numOfTasks; i++)
	{
		if (commMatrix[taskID][i] == 0)
		{
			printTimestamp(stamp);
			int hpuOfChildTask = findHPUForTaskInSchedule(schedule, numOfHPUs, maxNumOfTasksInSchedule, i);
/*DEBUG*/	std::cout << "\tP" << ":\tT" << taskID << ": unlocking T" << i << " (P" << hpuOfChildTask << ")" << std::endl;
			if (this->getProcessID() != hpuOfChildTask) // send unlock only to another process (not to send to itself)
				MPI_Isend(&dummy, 1, MPI_INT, hpuOfChildTask, i, MPI_COMM_WORLD, &request);
			printTimestamp(stamp);
/*DEBUG*/std::cout << "\tP" << this->getProcessID() << ":\tT" << taskID << ": unlocked T" << i << " (P" << hpuOfChildTask << ")" << std::endl;
		}
	}
}

/****Task computation phase, amount and type of the computation are inputs ****/
void Task::compute(int **compAmounts, double stamp)
{
	printTimestamp(stamp);
	/*DEBUG*/
	std::cout << "\tP" << this->getProcessID() << ":\tT" << this->getID() << " comp amounts";
	// printout of the comp amounts for this task
	for (int i = 0; i < numOfCompTypes; i++)
		std::cout << "\t" << compAmounts[this->getID()][i];
	std::cout << " started." << std::endl;
	/*******/


	for (int i = 0; i < numOfCompTypes; i++)
	{
		for (int j = 0; j < compAmounts[this->getID()][i]; j++)
			execute_load(i);
	}
	printTimestamp(stamp);


	/*DEBUG*/
	std::cout << "\tP" << this->getProcessID() << ":\tT" << this->getID() << " comp amounts";
	// printout of the comp amounts for this task
	for (int i = 0; i < numOfCompTypes; i++)
		std::cout << "\t" << compAmounts[this->getID()][i];
	std::cout << " finished." << std::endl;
	/*******/
}

/**** Process computation benchmark
 * Running amount of 1 of each of the computation types (function should be called per process)
 */
void Task::comp_bench(int mpi_rank, double comp_times[])
{
	double wtime = 0.0;
	for (int i = 0; i < numOfCompTypes; i++)
	{
		wtime = MPI_Wtime();
		execute_load(i);
		wtime = MPI_Wtime() - wtime;
		comp_times[i] = wtime;
		//std::cout << "\tP" << mpi_rank << ",\tcomputation type \t" << i << ":\t" << wtime << std::endl;
	}
}

/**** Process communication benchmark
 * Each process sends and receives 1 comm_chunk from/to all the other processes
 */
void Task::comm_bench(int mpi_rank, int mpi_size, int comm_chunk[], int comm_chunk_size, double sendTo_times[], double recvFrom_times[])
{
	double startTime = 0.0, stopTime = 0.0;
	for (int i = 0; i < mpi_size; i++)
	{
		{
			for (int j = 0; j < mpi_size; j++)
			{
				if ((i == mpi_rank) && (j != mpi_rank))
				{
					startTime = MPI_Wtime();
					MPI_Send(comm_chunk, comm_chunk_size, MPI_INT, j, j, MPI_COMM_WORLD);
					stopTime = MPI_Wtime();
					sendTo_times[j] = stopTime - startTime;
					//std::cout << "Comm Bench, P" << mpi_rank << ": sends to " << j << "\t" << stopTime - startTime << std::endl;
				}
				else if ((i != mpi_rank) && (j == mpi_rank))
				{
					startTime = MPI_Wtime();
					MPI_Recv(comm_chunk, comm_chunk_size, MPI_INT, i, j, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					stopTime = MPI_Wtime();
					recvFrom_times[i] = stopTime - startTime;
					//std::cout << "Comm Bench, P" << mpi_rank << ": receives from " << i << "\t" << stopTime - startTime << std::endl;

				}
				MPI_Barrier(MPI_COMM_WORLD);
			}
		}
	}
}

/****Synthetic load, load type is the input
 * type 0 - 10^8 short operations (STILL CHECK)
 * type 1 - 10^8 integer operations
 * type 2 - 10^8 float operations
 * type 3 - 10^8 double operations
 * ****/
void Task::execute_load(int comp_type)
{
	short temps1 = 1, temps2 = 2, temps3; //short entites (still check)
	int tempi1 = 1, tempi2 = 2, tempi3; //int entities
	float tempf1 = 1.00, tempf2 = 2.00, tempf3; // float entities
	double tempd1 = 1.00, tempd2 = 2.00, tempd3; // double entities
	if (comp_type == 0)
	{
		for (int i=0; i<10000; i++)
		{
			for (int j=0; j<10000; j++)
			{
				temps3 += temps1 + temps2;
			}
		}
	}
	else if (comp_type == 1)
	{
		for (int i=0; i<10000; i++)
		{
			for (int j=0; j<10000; j++)
			{
				tempi3 += tempi1 + tempi2;
			}
		}
	}
	else if (comp_type == 2)
	{
		for (int i=0; i<10000; i++)
		{
			for (int j=0; j<10000; j++)
			{
				tempf3 += tempf1 + tempf2;
			}
		}
	}
	else
	{
		for (int i=0; i<10000; i++)
		{
			for (int j=0; j<10000; j++)
			{
				tempd3 += tempd1 + tempd2;
			}
		}
	}
}

/****Load data phase of a task [FUTURE UPGRADE]
 * amount - the amount of data from file (large dummy file)
 * ****/
void Task::load_data (int amount)
{
}

/****Store data phase of a task [FUTURE UPGRADE]
 * amount - the amount of data from file (large dummy file)
 * ****/
void Task::store_data (int amount)
{
}

#endif /* TASK_H_ */
