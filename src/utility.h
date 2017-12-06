/*
 * utility.h
 *
 *  Created on: Mar 5, 2014
 *      Author: zdravko
 */

#ifndef UTILITY_H_
#define UTILITY_H_

#include <sstream>
#include <string>
#include "task.h"

//using namespace std;

std::size_t numOfTasks = 0;
std::size_t numOfCompTypes = 0; // 4 for now
std::size_t numOfHPUs = 0;
std::size_t maxNumOfTasksInSchedule = 0;

#define COMM_CHUNK_SIZE 2621440

/****
 * FILENAME_CREATOR creates unique output filename for each process.
 * EXAMPLE: out1.txt, out23.txt
 * PARAMETER: rank - process rank
 * OUTPUTS: filename as a string type
 */
std::string filename_creator(int rank)
{
	std::stringstream sstr;
	std::string c;
	sstr << "out";
	sstr << rank;
	sstr << ".txt";
	c = sstr.str();
	return c;
}

/****
 * MPI TIMESTAMP prints the current process MPI wallclock.
 * EXAMPLE:
 */
void printTimestamp (double stamp)
{
	//std::cout << MPI::Wtime() - stamp;
	printf("%.4f", MPI_Wtime() - stamp);
	//std::cout << MPI_Wtime() - stamp;
}

/****
 * TIMESTAMP prints the current YMDHMS date as a time stamp.
 * EXAMPLE: 31 May 2001 09:45:54 AM
 */
void timestamp2 ( )
{

# define TIME_SIZE 40

	static char time_buffer[TIME_SIZE];
	const struct std::tm *tm_ptr;
	size_t len;
	std::time_t now;

	now = std::time ( NULL );
	tm_ptr = std::localtime ( &now );

	len = std::strftime ( time_buffer, TIME_SIZE, "%I:%M:%S ", tm_ptr );

	std::cout << time_buffer; // << "\n";

	return;

# undef TIME_SIZE

}

/**** INITIALIZE 2D DYNAMIC ARRAY
 *
 */
template<typename t>
void initialize2DArray(t **array, int rows, int columns, t initValue)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			array[i][j] = initValue;
		}
	}
}

/**** INITIALIZE 1D DYNAMIC ARRAY
 *
 */
template<typename t>
void initialize1DArray(t *array, int numOfElements, t initValue)
{
	for (int i = 0; i < numOfElements; i++)
	{
		array[i] = initValue;
	}
}

/**** CHECK IF 2D DYNAMIC ARRAY CONTAINS A CERTAIN ELEMENT
 *
 */
template<typename t>
bool does2DArrayContainElement(t **array, int rows, int columns, t searchValue)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			if (array[i][j] == searchValue) return true;
		}
	}
	return false;
}

/**** CHECK IF 1D DYNAMIC ARRAY CONTAINS A CERTAIN ELEMENT
 *
 */
template<typename t>
bool does1DArrayContainElement(t *array, int numOfElements, t searchValue)
{
	for (int i = 0; i < numOfElements; i++)
	{
		if (array[i] == searchValue) return true;
	}
	return false;
}


/**** PRINT THE 2D DYNAMIC ARRAY
 *
 */
template<typename t>
void print2DArray(t **array, int rows, int columns)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			std::cout << array[i][j] << "\t";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

/**** PRINT THE 1D DYNAMIC ARRAY
 *
 */
template<typename t>
void print1DArray(t *array, int numOfElements)
{
	for (int i = 0; i < numOfElements; i++)
	{
		std::cout << array[i] << "\t";
	}
	std::cout << std::endl;
}

/**** PACK 2D DATA TO 1D DATA
 *
 */
template<typename t>
void pack2Dto1DArray(t **array2D, t *array1D, int rows, int columns)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			array1D[(i * columns) + j] = array2D[i][j];
		}
	}
}

/**** PACK 2D DATA TO 1D DATA
 *
 */
template<typename t>
void unpack1Dto2DArray(t **array2D, t *array1D, int rows, int columns)
{
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			array2D[i][j] = array1D[(i * columns) + j];
		}
	}
}



#endif /* UTILITY_H_ */
