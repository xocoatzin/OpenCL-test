#pragma warning(disable : 4996)

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <CL/cl.h>


#define MAX_SOURCE_SIZE (0x100000)

int main(void) {

	LARGE_INTEGER frequency, t1, t2, t3,t4,t5,t6;
	double elapsedTime1,elapsedTime2,elapsedTime3,elapsedTime4,elapsedTime5;
	QueryPerformanceFrequency(&frequency);
	

	// Create the two input vectors
	int i;
	const int LIST_SIZE = 25000;
	int *A = (int*)malloc(sizeof(int)*LIST_SIZE);
	int *B = (int*)malloc(sizeof(int)*LIST_SIZE);
	int *CB = (int*)malloc(sizeof(int)*LIST_SIZE);
	for(i = 0; i < LIST_SIZE; i++) {
		A[i] = i;
		B[i] = LIST_SIZE - i;
	}
	QueryPerformanceCounter(&t1);
		for(i = 0; i < LIST_SIZE; i++) {
			CB[i] = A[i] + B[i];
		}
	QueryPerformanceCounter(&t2);
	
	// Load the kernel source code into the array source_str
	FILE *fp;
	char *source_str;
	size_t source_size;

	fp = fopen("vector_add_kernel.cl", "r");
	if (!fp) {
		fprintf(stderr, "Failed to load kernel.\n");
		exit(1);
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread( source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose( fp );

	// Get platform and device information
	cl_platform_id platform_id = NULL;
	cl_device_id device_id = NULL;   
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;
	cl_int ret = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	// CL_DEVICE_TYPE_DEFAULT
	// CL_DEVICE_TYPE_CPU
	// CL_DEVICE_TYPE_GPU
	ret = clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_CPU, 1, 
		&device_id, &ret_num_devices);

	// Create an OpenCL context
	cl_context context = clCreateContext( NULL, 1, &device_id, NULL, NULL, &ret);

	// Create a command queue
	cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &ret);

	// Create memory buffers on the device for each vector 
	cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY, 
		LIST_SIZE * sizeof(int), NULL, &ret);
	cl_mem b_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY,
		LIST_SIZE * sizeof(int), NULL, &ret);
	cl_mem c_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
		LIST_SIZE * sizeof(int), NULL, &ret);

	// Create a program from the kernel source
	cl_program program = clCreateProgramWithSource(context, 1, 
		(const char **)&source_str, (const size_t *)&source_size, &ret);

	// Build the program
	ret = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

	// Create the OpenCL kernel
	cl_kernel kernel = clCreateKernel(program, "vector_add", &ret);

	// Set the arguments of the kernel
	ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&a_mem_obj);
	ret = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&b_mem_obj);
	ret = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&c_mem_obj);

QueryPerformanceCounter(&t5);
	// Copy the lists A and B to their respective memory buffers
	ret = clEnqueueWriteBuffer(command_queue, a_mem_obj, CL_TRUE, 0,
		LIST_SIZE * sizeof(int), A, 0, NULL, NULL);
	ret = clEnqueueWriteBuffer(command_queue, b_mem_obj, CL_TRUE, 0, 
		LIST_SIZE * sizeof(int), B, 0, NULL, NULL);

	QueryPerformanceCounter(&t3);
	// Execute the OpenCL kernel on the list
	size_t global_item_size = LIST_SIZE; // Process the entire lists
	size_t local_item_size = 64; // Divide work items into groups of 64
	ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, 
		&global_item_size, &local_item_size, 0, NULL, NULL);
	QueryPerformanceCounter(&t4);

	// Read the memory buffer C on the device to the local variable C
	int *C = (int*)malloc(sizeof(int)*LIST_SIZE);
	ret = clEnqueueReadBuffer(command_queue, c_mem_obj, CL_TRUE, 0, 
		LIST_SIZE * sizeof(int), C, 0, NULL, NULL);

QueryPerformanceCounter(&t6);
	// Display the result to the screen
	//for(i = 0; i < LIST_SIZE; i++)
	//	printf("%d + %d = %d\n", A[i], B[i], C[i]);

	// Clean up
	ret = clFlush(command_queue);
	ret = clFinish(command_queue);
	ret = clReleaseKernel(kernel);
	ret = clReleaseProgram(program);
	ret = clReleaseMemObject(a_mem_obj);
	ret = clReleaseMemObject(b_mem_obj);
	ret = clReleaseMemObject(c_mem_obj);
	ret = clReleaseCommandQueue(command_queue);
	ret = clReleaseContext(context);
	
	free(A);
	free(B);
	free(C);
	free(CB);

	elapsedTime1 = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
	elapsedTime2 = (t4.QuadPart - t3.QuadPart) * 1000.0 / frequency.QuadPart;
	elapsedTime3 = (t6.QuadPart - t5.QuadPart) * 1000.0 / frequency.QuadPart;
	return 0;
}