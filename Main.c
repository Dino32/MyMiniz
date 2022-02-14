#pragma warning (push, 0)

#include <Windows.h>

#include "miniz.h"

#include <string.h>

#pragma warning(pop)

#include <stdio.h>

#include <stdlib.h>

#define OPERATION_NONE 0

#define OPERATION_ADD 1

#define OPERATION_EXTRACT 2

BOOL FileExists(_In_ char* FileName)
{
	DWORD Attributes = GetFileAttributesA(FileName);

	return (Attributes != INVALID_FILE_ATTRIBUTES && !(Attributes & FILE_ATTRIBUTE_DIRECTORY));
} // return ture only if the file exists and it is not directory

int main(int argc, char* argv[])
{
	char* ArchiveName = NULL;

	char* FullyQualifiedFileName = NULL;

	int Operation = OPERATION_NONE;

	if (argc != 4)
	{
		printf("Adds or extracts files from a compressed archive.\n");

		printf("Usage: myminiz.exe <archive_file> <[+|-]> <filename>\n");

		return 0;
	}

	ArchiveName = argv[1];

	FullyQualifiedFileName = argv[3];

	if (_strcmpi(argv[2], "+") == 0)
	{
		Operation = OPERATION_ADD;
	}
	else if (_strcmpi(argv[2], "-") == 0)
	{
		Operation = OPERATION_EXTRACT;
	}
	else
	{
		printf("Adds or extracts files from a compressed archive.\n");

		printf("Usage: myminiz.exe <archive_file <[+|-]> <filename>\n");

		return 0;
	}

	printf("Operation %d", Operation);

	if (Operation == OPERATION_ADD)
	{
		DWORD BytesRead = 0;

		BYTE* FileBuffer = NULL;

		HANDLE FileHandle = INVALID_HANDLE_VALUE;

		LARGE_INTEGER FileSize = { 0 };

		DWORD Error = ERROR_SUCCESS;

		char FileName[MAX_PATH] = { 0 };

		char FileExtenstion[MAX_PATH] = { 0 };

		if ((FileHandle = CreateFileA(FullyQualifiedFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE)
		{
			Error = GetLastError();

			printf("ERROR: The file %s cannot be found, or could not be opened for reading! 0x%08lx\n", FileName, GetLastError());

			return Error;
		}

		printf("[+] File %s opened for reading.\n", FullyQualifiedFileName);

		if (GetFileSizeEx(FileHandle, &FileSize) == 0)
		{
			Error = GetLastError();

			return Error;
		}

		printf("[+] File size: %lld\n", FileSize.QuadPart);

		if ((FileBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, FileSize.QuadPart)) == NULL)
		{
			Error = ERROR_OUTOFMEMORY;

			return Error;
		}

		if ((ReadFile(FileHandle, FileBuffer, (DWORD)FileSize.QuadPart, &BytesRead, NULL)) == 0)
		{
			Error = GetLastError();

			printf("ERROR: ReadFile failed! 0x%08lx!\n", Error);

			return Error;
		}

		if (BytesRead != FileSize.QuadPart)
		{
			Error = ERROR_READ_FAULT;

			printf("ERROR: BytesRead into memory did not match file size.\n");

			return Error;
		}

		Error = _splitpath_s(FullyQualifiedFileName, NULL, 0, NULL, 0, FileName, sizeof(FileName), FileExtenstion, sizeof(FileExtenstion));

		strcat_s(FileName, sizeof(FileName), FileExtenstion);

		printf("[+] FileName: %s\n", FileName);

		if (mz_zip_add_mem_to_archive_file_in_place(ArchiveName, FileName, FileBuffer, FileSize.QuadPart, "", 1, MZ_BEST_COMPRESSION) == MZ_FALSE)
		{
			Error = ERROR_COMPRESSED_FILE_NOT_SUPPORTED;

			printf("ERROR: Failed to add File %s to archive %s! 0x%08lx\n", FileName, ArchiveName, Error);

			return Error;
		}

		printf("[+] File %s successfully added to archive %s", FileName, ArchiveName);


		if (FileHandle && FileHandle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(FileHandle);
		}
	}
	else if (Operation == OPERATION_EXTRACT)
	{
		BOOL FileFoundInArchive = FALSE;

		mz_zip_archive ZipArchive = { 0 };

		size_t ExtractedSize = 0;

		HANDLE ExtractedFileHandle = INVALID_HANDLE_VALUE;

		mz_zip_error MZError = MZ_ZIP_NO_ERROR;

		if (mz_zip_reader_init_file(&ZipArchive, ArchiveName, 0) == FALSE)
		{
			MZError = mz_zip_get_last_error(&ZipArchive);

			printf("ERROR: mz_zip_reader_init_file failed with error code %d\n", MZError);

			return MZError;
		}

		if (FileExists(ArchiveName) == FALSE)
		{
			printf("ERROR: Archive %s does not exist.\n", ArchiveName);

			return ERROR_FILE_NOT_FOUND;
		}

		printf("[-] Archive %s opened for reading.\n", ArchiveName);

		for (int FileIndex = 0; FileIndex < (int)mz_zip_reader_get_num_files(&ZipArchive); FileIndex++)
		{
			mz_zip_archive_file_stat CompressedFileStatistics = { 0 };

			DWORD BytesWritten = 0;

			if (mz_zip_reader_file_stat(&ZipArchive, FileIndex, &CompressedFileStatistics) == FALSE)
			{
				MZError = mz_zip_get_last_error(&ZipArchive);

				printf("ERROR: mz_zip_reader_file_stat failed with error code %d\n", MZError);

				return MZError;
			}

			if (_stricmp(CompressedFileStatistics.m_filename, FullyQualifiedFileName) == 0)
			{
				FileFoundInArchive = TRUE;

				printf("[-] File %s found in archive %s.\n", FullyQualifiedFileName, ArchiveName);

				if (mz_zip_reader_extract_to_file(&ZipArchive, FileIndex, FullyQualifiedFileName, 0) == FALSE)
				{
					MZError = mz_zip_get_last_error(&ZipArchive);

					printf("ERROR: mz_zip_reader_extract_to_file failed with error code %d\n", MZError);

					return MZError;
				}
				else
				{
					printf("[-] Successfully extracted file %s!", FullyQualifiedFileName);
				}

				break;
			}
		}

		if (FileFoundInArchive == FALSE)
		{
			printf("ERROR: File %s not found in archive %s!\n", FullyQualifiedFileName, ArchiveName);
		}
	}
	else
	{
		printf("ERROR: No operation!\n");
	}
}