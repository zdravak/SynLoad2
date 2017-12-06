#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <mpi.h>
#include <string> // for filename modifications
#include <iomanip> // for setting the number of decimals in stdout-ing doubles
#include "task.h"
#include "utility.h"
#include "data.h"

//using namespace std;

/****
 * MAIN FUNCTION
 * ****/
int main (int argc, char **argv)
{

	// for setting the number of decimals in stdout
	std::cout << std::fixed;
	std::cout << std::setprecision(7);

	// MPI
    MPI_Init(&argc, &argv);
    int size, rank, len;
	MPI_Comm_size(MPI_COMM_WORLD, &size); // comm size
	MPI_Comm_rank(MPI_COMM_WORLD, &rank); // comm rank

	// The master process reports if the program was run via the required number of processes
	// TODO: Reporting is not ok, the program should be stopped if this happens!
	if (rank == 0)
	{
		if (size != numOfHPUs)
			std::cout << "The program was not run on the correct amount of processes!\nRequired processes: " << numOfHPUs << "\nProcesses used: " << size << "\n";
	}


	// getting the initial data from the files && sending to slaves
	if (rank == 0)
	{
		getParamsFromTxtFiles("TaskCompAmounts.txt","TaskOrder.txt",numOfTasks, numOfHPUs, numOfCompTypes, maxNumOfTasksInSchedule);

		// send data to slaves
		for (int i = 1; i < size; i++)
		{
			MPI_Send(&numOfTasks, 1, MPI_INT, i, i, MPI_COMM_WORLD);
			MPI_Send(&numOfHPUs, 1, MPI_INT, i, i, MPI_COMM_WORLD);
			MPI_Send(&numOfCompTypes, 1, MPI_INT, i, i, MPI_COMM_WORLD);
			MPI_Send(&maxNumOfTasksInSchedule, 1, MPI_INT, i, i, MPI_COMM_WORLD);
		}
	}
	// slaves receive data
	else
	{
		MPI_Recv(&numOfTasks, 1, MPI_INT, 0, rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(&numOfHPUs, 1, MPI_INT, 0, rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(&numOfCompTypes, 1, MPI_INT, 0, rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(&maxNumOfTasksInSchedule, 1, MPI_INT, 0, rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}

	// create & initialize data containers
	int **compAmounts;
	compAmounts = new int*[numOfTasks];
	for (std::size_t i = 0; i < numOfTasks; i++)
		compAmounts[i] = new int[numOfCompTypes];
	initialize2DArray(compAmounts, numOfTasks, numOfCompTypes, 0);

//    int **compAmounts = (int **)malloc(numOfTasks * sizeof(int *));
//    for (int i=0; i<numOfTasks; i++)
//    	compAmounts[i] = (int *)malloc(numOfCompTypes * sizeof(int));

	int **schedule;
	schedule = new int*[numOfHPUs];
	for (std::size_t i = 0; i < numOfHPUs; i++)
		schedule[i] = new int[maxNumOfTasksInSchedule];
	initialize2DArray(schedule, numOfHPUs, maxNumOfTasksInSchedule, -1);

	double **commMatrix;
	commMatrix = new double*[numOfTasks];
	for (std::size_t i = 0; i < numOfTasks; i++)
		commMatrix[i] = new double[numOfTasks];
	initialize2DArray(commMatrix, numOfTasks, numOfTasks, -1.0);

	// create temporary data containers (packed to 1D arrays) to send via MPI (all processes)
	int *packedCompAmounts;
	packedCompAmounts = new int[numOfTasks * numOfCompTypes];
	int *packedSchedule; // = (int *)malloc( (numOfHPUs * maxNumOfTasksInSchedule) * sizeof(int));
	packedSchedule = new int[numOfHPUs * maxNumOfTasksInSchedule];
	double *packedCommMatrix;
	packedCommMatrix = new double[numOfTasks * numOfTasks];

	// master loads containers with data from files & sends it to slaves
	if (rank == 0)
	{
		getCompAmountsFromTxtFile("SynLoadTaskCompAmounts.txt", compAmounts);
		getScheduleFromTxtFile("SynLoadTaskOrder.txt", schedule);
		getCommMatrixFromTxtFile("SynLoadTaskCommMatrix.txt", commMatrix);

		// pack to 1D arrays (to have contiguous memory, otherwise MPI comm does not work)
		pack2Dto1DArray(compAmounts, packedCompAmounts, numOfTasks, numOfCompTypes);
		pack2Dto1DArray(schedule, packedSchedule, numOfHPUs, maxNumOfTasksInSchedule);
		pack2Dto1DArray(commMatrix, packedCommMatrix, numOfTasks, numOfTasks);

		// send data to slaves
		for (int i = 1; i < size; i++)
		{
			MPI_Send(packedCompAmounts, (numOfTasks * numOfCompTypes), MPI_INT, i, i, MPI_COMM_WORLD);
			MPI_Send(packedSchedule, (numOfHPUs * maxNumOfTasksInSchedule), MPI_INT, i, i, MPI_COMM_WORLD);
			MPI_Send(packedCommMatrix, (numOfTasks * numOfTasks), MPI_DOUBLE, i, i, MPI_COMM_WORLD);
		}
	}
	// slaves receive and unpack data
	else
	{
		MPI_Recv(packedCompAmounts, (numOfTasks * numOfCompTypes), MPI_INT, 0, rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(packedSchedule, (numOfHPUs * maxNumOfTasksInSchedule), MPI_INT, 0, rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(packedCommMatrix, (numOfTasks * numOfTasks), MPI_DOUBLE, 0, rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

		// unpack data
		unpack1Dto2DArray(compAmounts, packedCompAmounts, numOfTasks, numOfCompTypes);
		unpack1Dto2DArray(schedule, packedSchedule, numOfHPUs, maxNumOfTasksInSchedule);
		unpack1Dto2DArray(commMatrix, packedCommMatrix, numOfTasks, numOfTasks);
	}

	// delete temp packed containers
	delete[] packedCompAmounts;
	delete[] packedSchedule;
	delete[] packedCommMatrix;

//	// printouts
//	if (rank != 0)
//	{
//		print2DArray(compAmounts, numOfTasks, numOfCompTypes);
//		print2DArray(schedule, numOfHPUs, maxNumOfTasksInSchedule);
//		print2DArray(commMatrix, numOfTasks, numOfTasks);
//	}

	// the rest of the relevant data declarations
	int* commDataChunk = new int[COMM_CHUNK_SIZE]; // 10MB of data to use as send/recv container by each process, no need to populate
	double* compTimes = new double[numOfCompTypes]; // computation times for 1 unit of each of the computation types
	double* sendToTimes = new double[numOfHPUs]; // times to send a chunk of data to each of the other processes (processors)
	double* recvFromTimes = new double[numOfHPUs]; // times to receive a chunk of data from each of the other processes (processors)

	// create send/receive buffers for all processes
	// necessary to have the place from where the data will be sent and received (to simulate comm)
	int* sendBuffer = new int[COMM_CHUNK_SIZE]; // 10MB of data to use as send/recv container by each process, no need to populate
	int* recvBuffer = new int[COMM_CHUNK_SIZE]; // 10MB of data to use as send/recv container by each process, no need to populate


	// initializations
	for (int i = 0; i < numOfCompTypes; i++)
		compTimes[i] = 0.0;
	for (int i = 0; i < numOfHPUs; i++)
	{
		sendToTimes[i] = 0.0;
		recvFromTimes[i] = 0.0;
	}

	// Process info
	int numOfAllocatedTasks; // number of allocated tasks to a process
	double stamp; // variable to use as a timestamp
    char processorName[MPI_MAX_PROCESSOR_NAME]; // container for processor names

	/**** EVERY RANK does the same
	 * searches for the number of tasks allocated to it
	 * creates a vector to store task IDs dedicated to it
	 * fills that vector
	 * creates actual tasks (objects)
	 * (optional) do benchmarks
	 * runs procedures for every task
	 * ****/


    numOfAllocatedTasks = calculateNumOfAllocTasks(rank, schedule); // get number of tasks allocated to this process

    std::cout << "Num of allocated tasks:" << numOfAllocatedTasks << std::endl;

	int *members = new int[numOfAllocatedTasks]; // create an empty vector of task ids allocated to this process
	MPI_Request *requests = new MPI_Request[numOfAllocatedTasks]; // create an empty vector of MPI requests for nonblocking send (unlocks)

	allocateTasks(members, rank, schedule);

	print1DArray(members, numOfAllocatedTasks);

	// create an array of task objects allocated
	Task* taskArray = new Task[numOfAllocatedTasks];

	//MPI::COMM_WORLD.Barrier(); //barrier to sync timestamps
	MPI_Barrier(MPI_COMM_WORLD);
	//stamp = MPI::Wtime(); // reference time
	stamp = MPI_Wtime();

	/**** Output header for each process
	 *
	 */
	//MPI::Get_processor_name(proc_name, len); // store the processor name
	MPI_Get_processor_name(processorName, &len);
	std::cout << "Node name: " << processorName << std::endl; //output of the processor name
	printTimestamp(stamp); //initial timestamp for each process
	std::cout << "\tP" << rank << " started." << std::endl;

	// for every task run the usual
	for (int i=0; i<numOfAllocatedTasks;i++)
	{
		runSchedule(rank, taskArray[i], i, members, requests[i], commMatrix, schedule, compAmounts, commDataChunk, stamp);
	}

	for (int i=0; i<num_allocated;i++)
	{
		std::cout << "P" << rank << ": waits for request " << i << std::endl;
		// wait only for the unlocks sent to other processes (the process does not send unlock to itself)
		// we check if the rank of the calling process is different than the rank of the process to which the task to be unlocked is allocated to
		if (rank != allocation[members[i]])
			MPI_Wait(&requests[i], MPI_STATUS_IGNORE);
	}

	printTimestamp(stamp);




	/*********************************************/
	/****************INPUT DATA*******************/

	/**** The amount of computation for each task
	 * x 10^8
	 * dimesnionality: numOfTasks x numOfCompTypes
	 * each row is one task
	 * each column is the amount of computation for a computation type
	 * ****/
//	int comp_amount[N] = {6, 7, 2, 4, 6};
//	int comp_amount[numOfTasks] = {5, 3, 2, 3, 6, 7, 7, 9, 3, 7, 9, 1};

	/**** The type of computation for each task
	 * for use in function compute
	 * ****/
	// TODO: obsolete, this data is included in compAmounts
//	int comp_type[N] = {1, 1, 1, 1, 1};
//	int comp_type[numOfTasks] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

	/**** Allocation of tasks to processes
	 * size = num_of_tasks)
	 * process rank = processing unit id
	 * starts from 0, core 1 -> 0
	 * position in this vector is the task id ****/
	// TODO: introducing the order of execution also
	/****
	 * dimesnionality: numOfHPUs x maxNumTasksInSchedule
	 * each row is a schedule for a single HPU
	 * each column is a task in a schedule of a single HPU
	 */
//	int allocation[N] = {2, 1, 0, 2, 2};
//	int allocation[numOfTasks] = {2, 0, 2, 0, 1, 3, 4, 0, 5, 4, 1, 2};


	/****Task interdependency matrix
	 * -1 - no relation (no edge or arc)
	 *  0 - precedence (arc)
	 *  >0 - communication amount (edge)****/
						 //1   2   3   4   5   6   7   8
//
//	int task_dep[N][N] = {-1, -1,  0,  0, -1, //-1, -1, -1, //1
//						  -1, -1, 10, -1, -1, //-1, -1, -1, //2
//						  -1, 10, -1, -1,  0, // 0, -1, -1, //3
//						  -1, -1, -1, -1,  0, //-1, -1,  0, //4
//						  -1, -1, -1, -1, -1 //-1, -1, -1, //5
////						  -1, -1, -1, -1, -1, -1, -1, -1, //6
////						  -1, -1, -1, -1, -1, -1, -1,  0, //7
////						  -1, -1, -1, -1, -1, -1, -1, -1//8
//						 };
//
//	int task_dep[numOfTasks][numOfTasks] =
//	{
//		-1, 0, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//		-1, -1, -1, -1, 0, -1, -1, -1, -1, -1, -1, -1,
//		-1, -1, -1, -1, 0, 5, -1, -1, -1, -1, -1, -1,
//		-1, -1, -1, -1, -1, -1, 1, -1, -1, -1, -1, -1,
//		-1, -1, -1, -1, -1, -1, 3, 0, -1, -1, -1, -1,
//		-1, -1, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1,
//		-1, -1, -1, 1, 3, -1, -1, -1, 4, 0, -1, -1,
//		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, -1,
//		-1, -1, -1, -1, -1, -1, 4, -1, -1, -1, -1, 0,
//		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 8, 0,
//		-1, -1, -1, -1, -1, -1, -1, 10, -1, 8, -1, 0,
//		-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
//	};



	/*********************************************/
	/*******DECLARATIONS & INITIALIZATIONS********/


////
////    freopen(filename_creator(rank).c_str(),"w",stdout); //open file for redirecting cout to it
////
////	/** Do benchmarks
////	 * time for the execution of various computing loads
////	 * time for transferring a 10MB file between all pairs of HPUs
////	 */
////	MPI_Get_processor_name(proc_name, &len);
////	std::cout << "Node name: " << proc_name << std::endl; //output of the processor name
////	array_Task[0].comp_bench(rank, comp_times); // computation benchmark
////	array_Task[0].comm_bench(rank, size, comm_chunk, COMM_CHUNK_SIZE, sendTo_times, recvFrom_times); // computation benchmark
////
////	std::cout << "P" << rank << " comp times: ";
////	for (int i = 0; i < numOfCompTypes; i++)
////		std::cout << comp_times[i] << "\t";
////	std::cout << "\nP" << rank << " send to times: ";
////	for (int i = 0; i < numOfHPUs; i++)
////	{
////		std::cout << sendTo_times[i] << "\t";
////	}
////	std::cout << "\nP" << rank << " recv from times: ";
////	for (int i = 0; i < numOfHPUs; i++)
////	{
////		std::cout << recvFrom_times[i] << "\t";
////	}
////	std::cout << "\n";
////

//	//MPI::Finalize();
//

	/**** DELETING ****/
	for (std::size_t i = 0; i < numOfTasks; i++)
		delete[] compAmounts[i];
	delete[] compAmounts;
	for (std::size_t i = 0; i < numOfHPUs; i++)
		delete[] schedule[i];
	delete[] schedule;
	for (std::size_t i = 0; i < numOfTasks; i++)
		delete[] commMatrix[i];
	delete[] commMatrix;

	delete[] members;

	std::cout << "\tP" << rank << " finished." << std::endl;

	MPI_Finalize();
    return 0;
}
