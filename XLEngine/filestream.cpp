#include "filestream.h"
#include <errno.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <assert.h>

//Work buffers for handling special cases like std::string without allocating memory (beyond what the strings needs itself).
static u32  s_workBufferU32[1024];		//4k buffer.
static char s_workBufferChar[32768];	//32k buffer.

FileStream::FileStream() : Stream()
{
	m_file = NULL;
	m_mode = MODE_INVALID;
}

FileStream::~FileStream()
{
	close();
}

bool FileStream::exists(const char* filename)
{
	bool res = open(filename, MODE_READ);
	close();

	return res;
}

bool FileStream::open(const char* filename, FileMode mode)
{
	const char* modeStrings[] = { "rb", "wb", "rb+" };
	m_file = fopen(filename, modeStrings[mode]);
  if (!m_file)
  {
    std::cerr << strerror(errno) << " : " << filename << std::endl;
    return false;
  }
	m_mode = mode;

	return true;
}

void FileStream::close()
{
	if (m_file)
	{
		if (m_mode == MODE_WRITE || m_mode == MODE_READWRITE)
		{
			fflush(m_file);
		}

		fclose(m_file);
		m_file = NULL;
		m_mode = MODE_INVALID;
	}
}

bool FileStream::isOpen() const
{
	return (m_file != NULL);
}

void FileStream::flush()
{
	if (m_file)
	{
		fflush(m_file);
	}
}

void* FileStream::getFileHandle()
{
	return m_file;
}

//derived from Stream
void FileStream::seek(u32 offset, Origin origin/*=ORIGIN_START*/)
{
	const s32 forigin[] = { SEEK_SET, SEEK_END, SEEK_CUR };
	if (m_file)
	{
		fseek(m_file, offset, forigin[origin]);
	}
}

size_t FileStream::getLoc()
{
	if (!m_file)
	{
		return 0;
	}
	return ftell(m_file);
}

size_t FileStream::getSize()
{
	if (!m_file)
	{
		return 0;
	}

	seek(0, FileStream::ORIGIN_END);
	size_t filesize = getLoc();
	seek(0, FileStream::ORIGIN_START);

	return filesize;
}

void FileStream::readBuffer(void* ptr, u32 size, u32 count)
{
	assert(m_mode == MODE_READ || m_mode == MODE_READWRITE);
	fread(ptr, size, count, m_file);
}

void FileStream::writeBuffer(const void* ptr, u32 size, u32 count)
{
	assert(m_mode == MODE_WRITE || m_mode == MODE_READWRITE);
	fwrite(ptr, size, count, m_file);
}

//internal
void FileStream::readTypeString(std::string* ptr, u32 count)
{
	assert(m_mode == MODE_READ || m_mode == MODE_READWRITE);
	assert(count <= 256);
	//first read the length.
	fread(s_workBufferU32, sizeof(u32), count, m_file);

	//then read the string data.
	for (u32 s=0; s<count; s++)
	{
		fread(s_workBufferChar, 1, s_workBufferU32[s], m_file);
		s_workBufferChar[ s_workBufferU32[s] ] = 0;

		ptr[s] = s_workBufferChar;
	}
}

void FileStream::writeTypeString(const std::string* ptr, u32 count)
{
	assert(m_mode == MODE_WRITE || m_mode == MODE_READWRITE);
	assert(count <= 256);
	//first read the length.
	for (u32 s=0; s<count; s++)
	{
		s_workBufferU32[s] = (u32)ptr[s].length();
	}
	fwrite(s_workBufferU32, sizeof(u32), count, m_file);

	//then write the string data.
	for (u32 s=0; s<count; s++)
	{
		fwrite(ptr[s].data(), 1, s_workBufferU32[s], m_file);
	}
}

