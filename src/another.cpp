//#include <stdio.h>
//#include <string>
//#include <iostream>
//#include <mpi.h>
//
//int main(int argc, char** argv) {
//    int myrank, nprocs, len;
//
//    MPI_Init(&argc, &argv);
//    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
//    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
//    char proc_name[MPI_MAX_PROCESSOR_NAME];
//    MPI_Get_processor_name(proc_name, &len);
//    freopen("Cubie.txt","w",stdout); //open file for redirecting cout to it
//    std::cout << "Hello from processor " << myrank << " of " << nprocs << ", name: " << proc_name << std::endl;
//    MPI_Finalize();
//    return 0;
//}
//
