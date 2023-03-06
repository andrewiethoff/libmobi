// mobiconverttest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <stdio.h>

__declspec(dllimport) int __cdecl ConvertMobiToEpub(unsigned char* buffer, long buf_len, unsigned char** out_buffer, long* out_buffer_len);

int main()
{
	const char* inputFileName = "test.azw3";
	int fileSize = get_file_size(inputFileName);

	FILE* inputFile = fopen(inputFileName, "rb");
	unsigned char* buffer = LocalAlloc(LPTR, fileSize);
	memset(buffer, 0, fileSize);
	long fileLength = fread(buffer, 1, fileSize, inputFile);
	fclose(inputFile);

	unsigned char* out_buffer;
	long out_buffer_len;

	ConvertMobiToEpub(buffer, fileLength, &out_buffer, &out_buffer_len);

	FILE* outputFile = fopen("test.epub", "wb");
	fwrite(out_buffer, 1, out_buffer_len, outputFile);
	fclose(outputFile);

	LocalFree(out_buffer);
	LocalFree(buffer);
}

int get_file_size(char* filename) // path to file
{
	FILE* p_file = NULL;
	p_file = fopen(filename, "rb");
	fseek(p_file, 0, SEEK_END);
	int size = ftell(p_file);
	fclose(p_file);
	return size;
}
